#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>
#include <time.h>
#include <errno.h>
#include <libgen.h>

#ifndef WIN32
    #include <unistd.h>
#endif

#ifdef WIN32
    #define stat _stat
#endif

#include "cshadermanager.h"
#include "cshadermanagerdev.h"

const char *shader_compiler = "glslangValidator";
const char *shader_compiler_args = " -V ";
const char *list = "../compilelist.txt";

struct csm_shader_t *shader_map = NULL;
int nshaders = 0;

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(path) _mkdir(path)
    #define PATH_SEP '\\'
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define MKDIR(path) mkdir(path, 0777)
    #define PATH_SEP '/'
#endif

typedef enum bool {
    false = 0,
    true = 1
} bool;

time_t get_mtime(const char *fpath) {
    struct stat file_stats;
    if (stat(fpath, &file_stats) == 0) {
        return file_stats.st_mtime;
    } else {
        printf("stat %i\n", errno);
    }
    return -1;
}

int compare_shader_t(const void *a, const void *b) {
    const struct csm_shader_t *shader1 = (const struct csm_shader_t *)a;
    const struct csm_shader_t *shader2 = (const struct csm_shader_t *)b;
    return strcmp(shader1->name, shader2->name);
}

void add_shader_to_map(struct shader_cache_entry entry) {

    struct csm_shader_t *new_map = malloc((nshaders + 1) * sizeof(struct csm_shader_t));
    if (nshaders > 0) {
        memcpy(new_map, shader_map, nshaders * sizeof(struct csm_shader_t));
    }
    if (shader_map != NULL)
        free(shader_map);
    shader_map = new_map;

    struct csm_shader_t add;
    strcpy(add.name, entry.name);
    shader_map[ nshaders ] = add;

    nshaders++;

    qsort(shader_map, nshaders, sizeof(struct csm_shader_t), compare_shader_t);
}

struct csm_shader_t *find_shader(const char *name) {
    struct csm_shader_t shader;
    memset(&shader, 0, sizeof(struct csm_shader_t));

    strncpy(shader.name, name, sizeof(shader.name) - 1);
    shader.name[sizeof(shader.name) - 1] = '\0';

    return (struct csm_shader_t *)bsearch(&shader, shader_map, nshaders, sizeof(struct csm_shader_t), compare_shader_t);
}

bool does_shader_exist(const char *name) {
    return find_shader(name) != NULL;
}

void create_parent_dirs(const char *path) {
    char* last_separator = strrchr(path, PATH_SEP);
    if (last_separator != NULL) {
        *last_separator = '\0';
        
        char buffer[256];

        strcpy(buffer, path);
        int len = strlen(buffer);

        if (buffer[len - 1] == PATH_SEP) {
            buffer[len - 1] = 0;
        }

        for (char *p = buffer + 1; *p; p++) {
            if (*p == PATH_SEP) {
                *p = 0;
                MKDIR(buffer);
                *p = PATH_SEP;
            }
        }

        MKDIR(buffer);
    }
}

shader_entry * load_all_entries(const char *shader_list_file_path, int *count) {
    FILE *f = fopen(shader_list_file_path, "r");
    assert(f != NULL);

    int currallocsize = 16;
    shader_entry *entries = malloc(currallocsize * sizeof(shader_entry));

    char line[ 256 ];
    for (int i = 0;  ; i++) {
        if (!fgets(line, 256, f)) {
            break;
        }
        else if (strlen(line) == 1) continue;
        line[ strcspn(line, "\n") ] = 0;

        if (i >= currallocsize) {
            currallocsize *= 2;
            shader_entry *copy = malloc(currallocsize * sizeof(shader_entry));
            memcpy(copy, entries, currallocsize * sizeof(shader_entry));

            entries = copy;
        }

        shader_entry entry;
        memset(&entry, 0, sizeof(shader_entry));

        strncpy(entry.path, line, 256);

        // to get stage + verify that it exists
        FILE *shader_file = fopen(entry.path, "r");
        assert(shader_file != NULL);

        fgets(line, 256, shader_file);

        if (strncmp(line, "//", strlen("//")) == 0) {
            sscanf(line, "// output: %s stage: %s name: %s", entry.output_path, entry.stage, entry.name);
        } else {
            strcpy(entry.stage, "unknown");
            strcpy(entry.output_path, "");
            printf("[warning] for shader on path \"%s\": no output, stage and group header? fmt-> // output: {output} stage: {stage} name: {name}\n", entry.path);
        }

        fclose(shader_file);

        entry.last_modified = get_mtime(entry.path);

        entries[ *count ] = entry;
        (*count)++;
    }

    fclose(f);
    return entries;
}

#define CMD_HELP_MSG "cmd can be any of:\n\
<default> compile: compile only those that have been changed since last ran,\n\
compile-force: forcefully compile all shaders in list file,\n\
\n"\

void compile_shader(char *buffer, struct shader_entry entry) {
    strcat(buffer, shader_compiler);
    strcat(buffer, " ");
    strcat(buffer, shader_compiler_args);

    strcat(buffer, entry.path);

    if (strcmp(entry.output_path, "") == 0)
        {}
    else {
        strcat(buffer, " -o ");
        strcat(buffer, entry.output_path);
    }

    strcat(buffer, " -S ");
    strcat(buffer, entry.stage);

    create_parent_dirs(entry.output_path);

    if (system(buffer) != 0) {
        printf("Error while compiling shader \"%s\".\n", basename(entry.path));
    }
}

shader_cache_entry * load_cache(int *count) {
    FILE *f = fopen("shaders.cache", "r");
    if (f == NULL)
        return NULL;

    shader_cache_entry *entries = (shader_cache_entry *)malloc(16 * sizeof(shader_cache_entry));
    memset(entries, 0, 16 * sizeof(shader_cache_entry));

    char line[256];
    while (fgets(line, 256, f)) {
        sscanf(line, "name: %s path: %s mtime: %ld", entries[*count].name, entries[*count].path, &entries[*count].last_modified);
        (*count)++;
    }

    fclose(f);
    return entries;
}

void update_cache(const shader_cache_entry * restrict entries, int count) {
    FILE *f = fopen("shaders.cache", "w");
    assert(f != NULL);

    for (int i = 0; i < count; i++) {
        fprintf(f, "name: %s path: %s mtime: %ld\n", entries[i].name, entries[i].path, entries[i].last_modified);
    }

    fclose(f);
}

void write_new_cache(const shader_entry * restrict entries, int count) {
    FILE *f = fopen("shaders.cache", "w");
    assert(f != NULL);

    for (int i = 0; i < count; i++) {
        fprintf(f, "name: %s path: %s mtime: %ld\n", entries[i].name, entries[i].path, entries[i].last_modified);
    }

    fclose(f);
}

#define HAS_FLAG(f) strcmp(argv[i], f) == 0

#if CSM_EXECUTABLE

int main(int argc, char *argv[]) {
    for (int i = 0; i < argc; i++) {
        if (HAS_FLAG("help") || HAS_FLAG("--h") || HAS_FLAG("-h") || HAS_FLAG("--help") || HAS_FLAG("-help")) {
            printf("usage: %s <compile list path = \"../compilelist.txt\"> <cmd = compile>\n", argv[0]);
            printf("%s", CMD_HELP_MSG);
            return 0;
        }
    }

    const char *cmd = (argc < 3) ? "compile" : argv[2];

    if (strcmp(cmd, "compile-force") == 0) {
        csm_compile_all();
    } else if (strcmp(cmd, "compile") == 0) {
        csm_compile_updated();
    }

    return 0;
}

#endif // CSM_EXECUTABLE

#if CSM_EXECUTABLE != 1

#ifdef __cplusplus
extern "C" {
#endif

#include "../include/vkcb/stdafx.h"

#ifdef __cplusplus
}
#endif

void read_shader_spirv(const char *output, unsigned **spirv, int *spirvsize) {
    FILE *f = fopen(output, "rb");
    assert(f != NULL);

    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned *buffer = malloc(fsize);
    if (!buffer) {
        fclose(f);
        return;
    }

    fread(buffer, 1, fsize, f);

    fclose(f);

    *spirv = buffer;
    *spirvsize = fsize;
}

void __csm_create_shader(const unsigned *bytes, int nbytes, struct csm_shader_t *out)
{
    VkShaderModuleCreateInfo info = {};
    info.codeSize = nbytes;
    info.pCode = bytes;
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vkCreateShaderModule(device, &info, NULL, (VkShaderModule *)&out->shader_module);
}

void register_all_shaders(struct shader_entry *entries, int nentries) {
    struct csm_shader_t *new_shader_map = malloc((nshaders + nentries) * sizeof(struct csm_shader_t));
    if (shader_map) {
        memcpy(new_shader_map, shader_map, nshaders * sizeof(struct csm_shader_t));
        free(shader_map);
    }
    shader_map = new_shader_map;

    int index = 0;
    for (int i = 0; i < nentries; i++) {
         if (strncmp(entries[i].stage, "vert", 4) == 0) {
            shader_map[nshaders + index].stage = VK_SHADER_STAGE_VERTEX_BIT;
        } else if (strncmp(entries[i].stage, "frag", 4) == 0) {
            shader_map[nshaders + index].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        } else if (strncmp(entries[i].stage, "tese", 4) == 0) {
            shader_map[nshaders + index].stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        } else if (strncmp(entries[i].stage, "geom", 4) == 0) {
            shader_map[nshaders + index].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
        } else if (strncmp(entries[i].stage, "comp", 4) == 0) {
            shader_map[nshaders + index].stage = VK_SHADER_STAGE_COMPUTE_BIT;
        } else {
            printf("Invalid stage for shader \"%s\". It will not be added.\n", entries[i].name);
            continue;
        }
        strncpy(shader_map[nshaders + index].name, entries[i].name, 127);
        shader_map[nshaders + index].name[127] = '\0';

        unsigned *spirv;
        int spirvsize = 0;
        read_shader_spirv(entries[i].output_path, &spirv, &spirvsize);
        __csm_create_shader(spirv, spirvsize, &shader_map[nshaders + index]);

        nshaders++;
    }
    qsort(shader_map, nshaders, sizeof(struct csm_shader_t), compare_shader_t);
}

#endif//CSM_EXECUTABLE != 1

void csm_compile_updated()
{
    char *buffer = malloc(512);

    int count = 0;
    shader_entry *entries = load_all_entries(list, &count);

    #if CSM_EXECUTABLE != 1
        register_all_shaders(entries, count);
    #endif//CSM_EXECUTABLE != 1

    int cachecount = 0;
    shader_cache_entry *cacheentries = load_cache(&cachecount);

    for (int i = 0; i < count; i++) {
        for (int j = 0; j < cachecount; j++) {
            if (strcmp(cacheentries[j].path, entries[i].path) == 0) {
                if (cacheentries[j].last_modified != entries[i].last_modified) {
                    memset(buffer, 0, 512);
                    compile_shader(buffer, entries[i]);
                    cacheentries[j].last_modified = entries[i].last_modified;
                }
                break;
            }
        }
    }

    if (!cacheentries) {
        printf("csm :: No cache. Writing new cache file...\n");
        write_new_cache(entries, count);
    } else {
        update_cache(cacheentries, count);
    }

    free(cacheentries);
    free(buffer);
}

void csm_compile_all()
{
    char *buffer = malloc(512);

    int count = 0;
    shader_entry *entries = load_all_entries(list, &count);

    #if CSM_EXECUTABLE != 1
    register_all_shaders(entries, count);
    #endif//#if CSM_EXECUTABLE != 1

    for (int i = 0; i < count; i++) {
        memset(buffer, 0, 512);
        compile_shader(buffer, entries[i]);
    }

    free(buffer);
    write_new_cache(entries, count);
}

void csm_set_list_file(const char *path)
{
    list = path;
}

void csm_set_shader_compiler(const char *exec)
{
    shader_compiler = exec;
}

void csm_set_shader_compiler_args(const char *args)
{
    shader_compiler_args = args;
}

char const *csm_get_shader_compiler_args(void)
{
    return shader_compiler_args;
}

int csm_load_shader(const char *name, struct csm_shader_t **out)
{
    struct csm_shader_t shader;
    memset(&shader, 0, sizeof(struct csm_shader_t));

    strncpy(shader.name, name, sizeof(shader.name) - 1);
    shader.name[sizeof(shader.name) - 1] = '\0';

    qsort(shader_map, nshaders, sizeof(struct csm_shader_t), compare_shader_t);
    struct csm_shader_t *shaderptr = (struct csm_shader_t *)bsearch(&shader, shader_map, nshaders, sizeof(struct csm_shader_t), compare_shader_t);

    if (shaderptr) {
        *out = shaderptr;
    } else {
        *out = NULL;
        for (int i = 0; i < nshaders; i++) {
            printf("%s\n", shader_map[i].name);
        }
        abort();
    }

    return 0;
}
