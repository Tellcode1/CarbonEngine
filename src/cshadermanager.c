#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>
#include <time.h>
#include <errno.h>
#include <libgen.h>
#include "../external/SPIRV-Reflect/spirv_reflect.h"
#include "../include/cgfxpipeline.h"

#ifndef WIN32
    #include <unistd.h>
#endif

#ifdef WIN32
    #define stat _stat
#endif

#include "../include/vkstdafx.h"

#include "../include/cshadermanager.h"
#include "../include/cshadermanagerdev.h"

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

#define HAS_FLAG(f) strcmp(argv[i], f) == 0

#if CSM_EXECUTABLE

#define CMD_HELP_MSG "cmd can be any of:\n\
<default> compile: compile only those that have been changed since last ran,\n\
compile-force: forcefully compile all shaders in list file,\n\
\n"\

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
        csm_compile_all(NULL);
    } else if (strcmp(cmd, "compile") == 0) {
        csm_compile_updated(NULL);
    }

    return 0;
}

#else

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

#if CSM_EXECUTABLE != 1

void read_shader_spirv(const char *output, unsigned **spirv, int *spirvsize) {
    FILE *f = fopen(output, "rb");
    assert(f != NULL);

    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    if (fsize == (size_t)-1) {
        goto err;
    }
    fseek(f, 0, SEEK_SET);

    unsigned *buffer = malloc(fsize);
    if (!buffer) {
        goto err;
    }

    fread(buffer, 1, fsize, f);

    fclose(f);

    *spirv = buffer;
    *spirvsize = fsize;
    return;

    err:
    printf("csm :: Could not read in spirv for output path \"%s\"\n", output);
    fclose(f);
}

cgfx_descriptor_type spv_desc_type_to_cgfx_desc(SpvReflectDescriptorType reflect_type) {
    switch (reflect_type) {
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            return CGFX_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            return CGFX_DESCRIPTOR_TYPE_SS_BUFFER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            return CGFX_DESCRIPTOR_TYPE_IMAGE;
        default:
            return CGFX_DESCRIPTOR_TYPE_UNKNOWN;
    }
}

void reflect_shader_descriptors(SpvReflectShaderModule *shader_module, csm_shader_t *dst) {
    u32 count = 0;
    cassert( spvReflectEnumerateDescriptorSets(shader_module, &count, NULL) == SPV_REFLECT_RESULT_SUCCESS );
    dst->ndescriptors = 0;
    if (count > 0) {
        SpvReflectDescriptorSet **descriptor_sets = (SpvReflectDescriptorSet**)malloc(count * sizeof(SpvReflectDescriptorSet*));
        cassert( spvReflectEnumerateDescriptorSets(shader_module, &count, descriptor_sets) == SPV_REFLECT_RESULT_SUCCESS);

        for (u32 i = 0; i < count; ++i) {
            SpvReflectDescriptorSet *set = descriptor_sets[i];
            for (u32 j = 0; j < set->binding_count; ++j) {
                SpvReflectDescriptorBinding* binding = set->bindings[j];

                if (dst->ndescriptors < CGFX_MAX_DESCRIPTORS_PER_SHADER) {
                    csm_shader_descriptor *desc = &dst->descriptors_info[dst->ndescriptors++];
                    desc->parent = dst;
                    desc->name = binding->name;
                    desc->binding = binding->binding;
                    desc->set = set->set;
                    desc->type = spv_desc_type_to_cgfx_desc(binding->descriptor_type);
                } else {
                    LOG_AND_ABORT("more shader descriptors than CGFX_MAX_DESCRIPTORS_PER_SHADER(%i)\n", CGFX_MAX_DESCRIPTORS_PER_SHADER);
                }
            }
        }
        free(descriptor_sets);
    }
}

void __csm_create_shader(VkDevice vkdevice, const unsigned *bytes, int nbytes, struct csm_shader_t *out)
{
    SpvReflectShaderModule reflect_module;
    spvReflectCreateShaderModule(nbytes, bytes, &reflect_module);

    reflect_shader_descriptors(&reflect_module, out);

    spvReflectDestroyShaderModule(&reflect_module);

    VkShaderModuleCreateInfo info = {};
    info.codeSize = nbytes;
    info.pCode = bytes;
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vkCreateShaderModule(vkdevice, &info, NULL, (VkShaderModule *)&out->shader_module);
}

void register_all_shaders(VkDevice vkdevice, struct shader_entry *entries, int nentries) {
    struct csm_shader_t *new_shader_map = malloc((nshaders + nentries) * sizeof(struct csm_shader_t));
    if (shader_map) {
        memcpy(new_shader_map, shader_map, nshaders * sizeof(struct csm_shader_t));
        free(shader_map);
    }
    shader_map = new_shader_map;

    int index = 0;
    for (int i = 0; i < nentries; i++) {
         if (strncmp(entries[i].stage, "vert", 4) == 0) {
            shader_map[nshaders + index].stage = 1;
        } else if (strncmp(entries[i].stage, "frag", 4) == 0) {
            shader_map[nshaders + index].stage = 16;
        } else if (strncmp(entries[i].stage, "tese", 4) == 0) {
            shader_map[nshaders + index].stage = 4;
        } else if (strncmp(entries[i].stage, "geom", 4) == 0) {
            shader_map[nshaders + index].stage = 8;
        } else if (strncmp(entries[i].stage, "comp", 4) == 0) {
            shader_map[nshaders + index].stage = 32;
        } else {
            printf("csm :: Invalid stage for shader \"%s\". It will not be added.\n", entries[i].name);
            continue;
        }
        strncpy(shader_map[nshaders + index].name, entries[i].name, 127);
        shader_map[nshaders + index].name[127] = '\0';

        unsigned *spirv;
        int spirvsize = 0;
        read_shader_spirv((const char *)entries[i].output_path, &spirv, &spirvsize);
        __csm_create_shader(vkdevice, spirv, spirvsize, &shader_map[nshaders + index]);

        nshaders++;
    }
    qsort(shader_map, nshaders, sizeof(struct csm_shader_t), compare_shader_t);
}

#endif//CSM_EXECUTABLE != 1

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
    if (name == NULL || strlen(name) == 0) {
        return -1;
    }

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
        return -1;
    }

    return 0;
}

#endif // CSM_EXECUTABLE

shader_cache_entry * load_cache(int *count) {
    FILE *f = fopen("shaders.cache", "r");
    if (f == NULL) {
        *count = 0;
        return NULL;
    }

    shader_cache_entry *entries = (shader_cache_entry *)calloc(16, sizeof(shader_cache_entry));

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

void create_parent_dirs(const char path[256]) {
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

time_t get_mtime(const char *fpath) {
    struct stat file_stats;
    if (stat(fpath, &file_stats) == 0) {
        return file_stats.st_mtime;
    } else {
        printf("stat %i\n", errno);
    }
    return -1;
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
            entries = realloc(entries, currallocsize * sizeof(shader_entry));
            assert(entries != NULL);
        }

        shader_entry entry;
        memset(&entry, 0, sizeof(shader_entry));

        strncpy(entry.path, line, 256);

        // to get stage + verify that it exists
        FILE *shader_file = fopen(entry.path, "r");
        if (shader_file == NULL) {
            printf("csm :: Could not open shader file \"%s\". Are you sure that it exists?\n", entry.path);
            continue;
        }

        fgets(line, 256, shader_file);

        if (strncmp(line, "//", 2) == 0) {
            sscanf(line, "// output: %s stage: %s name: %s", entry.output_path, entry.stage, entry.name);
        } else {
            strcpy(entry.stage, "unknown");
            strcpy(entry.output_path, "");
            printf("csm :: [warning] for shader on path \"%s\": no output, stage and group header? fmt-> // output: {output} stage: {stage} name: {name}\n", entry.path);
        }

        fclose(shader_file);

        entry.last_modified = get_mtime(entry.path);

        entries[ *count ] = entry;
        (*count)++;
    }

    fclose(f);
    return entries;
}

void compile_shader(char *buffer, const struct shader_entry *entry) {
    char copy[256] = {};
    strncpy(copy, entry->output_path, 256);
    create_parent_dirs(copy);

    strcat(buffer, shader_compiler);
    strcat(buffer, " ");
    strcat(buffer, shader_compiler_args);

    strcat(buffer, entry->path);

    if (strcmp(entry->output_path, "") == 0)
        {}
    else {
        strcat(buffer, " -o ");
        strcat(buffer, entry->output_path);
    }

    strcat(buffer, " -S ");
    strcat(buffer, entry->stage);

    if (system(buffer) != 0) {
        printf("Error while compiling shader \"%s\".\n", entry->path);
    }
}

void csm_compile_updated()
{
    char *buffer = malloc(512);

    int count = 0;
    shader_entry *entries = load_all_entries(list, &count);

    int cachecount = 0;
    shader_cache_entry *cacheentries = load_cache(&cachecount);

    if (cacheentries != NULL) {
        for (int i = 0; i < count; i++) {
            for (int j = 0; j < cachecount; j++) {
                if (strcmp(cacheentries[j].path, entries[i].path) == 0) {
                    if (cacheentries[j].last_modified != entries[i].last_modified) {
                        memset(buffer, 0, 512);
                        compile_shader(buffer, &entries[i]);
                        cacheentries[j].last_modified = entries[i].last_modified;
                    }
                    break;
                }
            }
        }
    } else {
        for (int i = 0; i < count; i++) {
            memset(buffer, 0, 512);
            compile_shader(buffer, &entries[i]);
        }
    }

    if (!cacheentries) {
        printf("csm :: No cache or modified cache. Writing new cache file...\n");
        write_new_cache(entries, count);
    } else {
        update_cache(cacheentries, count);
    }

    #if CSM_EXECUTABLE != 1
        register_all_shaders(device, entries, count);
    #endif//CSM_EXECUTABLE != 1

    free(cacheentries);
    free(buffer);
}

void csm_compile_all()
{
    char *buffer = malloc(512);

    int count = 0;
    shader_entry *entries = load_all_entries(list, &count);

    #if CSM_EXECUTABLE != 1
       register_all_shaders(device, entries, count);
    #endif//#if CSM_EXECUTABLE != 1

    for (int i = 0; i < count; i++) {
        memset(buffer, 0, 512);
        compile_shader(buffer, &entries[i]);
    }

    free(buffer);
    write_new_cache(entries, count);
}