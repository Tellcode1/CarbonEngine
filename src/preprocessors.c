#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#ifndef WIN32
#include <unistd.h>
#endif

#ifdef WIN32
#define stat _stat
#endif

#if !(CSM_EXECUTABLE)

struct NVSM_shader_t* shader_map = NULL;
int                   nshaders   = 0;

#endif

#include "../common/stdafx.h"
#include "../include/engine/shadermanagerdev.h"

const char* shader_compiler      = "glslangValidator";
const char* shader_compiler_args = " -V ";
const char* list                 = "../compilelist.txt";

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#define PATH_SEP '\\'
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MKDIR(path) (mkdir(path, 0777))
#define PATH_SEP '/'
#endif

#define CSM_HAS_FLAG(flag) (NV_strcmp(argv[i], flag) == 0)

static inline void
_CSM_NV_LOG_ERROR(const char* fn, const char* fmt, ...)
{
  const char* preceder  = "csm error:";
  const char* succeeder = "\n";
  va_list     args;
  va_start(args, fmt);
  _NV_LOG(args, fn, succeeder, preceder, fmt, 1);
  va_end(args);
}

#define CSM_NV_LOG_ERROR(err, ...) _CSM_NV_LOG_ERROR(__PRETTY_FUNCTION__, err, ##__VA_ARGS__)

#if defined(CSM)

#include "../common/printf.h"
#include "../common/string.h"
#include "../common/timer.h"

#if CSM_EXECUTABLE

#include "../include/engine/shadermanager.h"

#define CMD_HELP_MSG                                                                                                                                                          \
  "cmd can be any of:\n\
<default> compile: compile only those that have been changed since last ran,\n\
compile-force: forcefully compile all shaders in list file,\n\
\n"

int
main(int argc, char* argv[])
{
  for (int i = 0; i < argc; i++)
  {
    if (CSM_HAS_FLAG("help") || CSM_HAS_FLAG("--h") || CSM_HAS_FLAG("-h") || CSM_HAS_FLAG("--help") || CSM_HAS_FLAG("-help"))
    {
      NV_printf("usage: %s <compile list path = \"../compilelist.txt\"> <cmd "
                "= compile>\n",
          argv[0]);
      NV_printf("%s", CMD_HELP_MSG);
      return 0;
    }
  }

  const char* cmd = (argc < 3) ? "compile" : argv[2];

  if (NV_strcmp(cmd, "compile-force") == 0)
  {
    NVSM_compile_all();
  }
  else if (NV_strcmp(cmd, "compile") == 0)
  {
    NVSM_compile_updated();
  }
  else
  {
    NV_printf("usage: %s <compile list path = \"../compilelist.txt\"> <cmd = "
              "compile>\n",
        argv[0]);
    NV_printf("%s", CMD_HELP_MSG);
    return -1;
  }

  return 0;
}

#else

int
compare_shader_t(const void* a, const void* b)
{
  const struct NVSM_shader_t* shader1 = (const struct NVSM_shader_t*)a;
  const struct NVSM_shader_t* shader2 = (const struct NVSM_shader_t*)b;
  return NV_strncmp(shader1->name, shader2->name, 128);
}

void
NVSM_add_shader_to_map(struct NVSM_shader_cache_entry entry, NVSM_shader_t** dst)
{
  struct NVSM_shader_t* new_map = NV_malloc((nshaders + 1) * sizeof(struct NVSM_shader_t));
  if (nshaders > 0)
  {
    NV_memcpy(new_map, shader_map, nshaders * sizeof(struct NVSM_shader_t));
  }
  if (shader_map != NULL)
    NV_free(shader_map);
  shader_map = new_map;

  struct NVSM_shader_t add;
  NV_strcpy(add.name, entry.name);
  shader_map[nshaders] = add;

  *dst                 = &shader_map[nshaders];

  nshaders++;
  // map is sorted after all shaders are registered.

  qsort(shader_map, nshaders, sizeof(struct NVSM_shader_t), compare_shader_t);
}

struct NVSM_shader_t*
find_shader(const char* name)
{
  struct NVSM_shader_t shader = {};

  NV_strncpy(shader.name, name, sizeof(shader.name) - 1);
  shader.name[sizeof(shader.name) - 1] = '\0';

  return (struct NVSM_shader_t*)bsearch(&shader, shader_map, nshaders, sizeof(struct NVSM_shader_t), compare_shader_t);
}

bool
does_shader_exist(const char* name)
{
  return find_shader(name) != NULL;
}

int
NVSM_load_shader(const char* name, struct NVSM_shader_t** out)
{
  if (name == NULL || NV_strlen(name) == 0)
  {
    return -1;
  }

  struct NVSM_shader_t shader = {};

  NV_strncpy(shader.name, name, sizeof(shader.name) - 1);
  shader.name[sizeof(shader.name) - 1] = '\0';

  struct NVSM_shader_t* shaderptr      = (struct NVSM_shader_t*)bsearch(&shader, shader_map, nshaders, sizeof(struct NVSM_shader_t), compare_shader_t);

  if (shaderptr)
  {
    *out = shaderptr;
  }
  else
  {
    // should we check out is NULL before setting it or not
    *out = NULL;
    return -1;
  }

  return 0;
}

int
NVSM_load_shader_from_disk(const char* path, NVSM_shader_t** out)
{
  (void)path;
  (void)out;
  // NVSM_shader_entry entry = {0};

  // char line[256];
  // strcpy(line, path);

  // NVSM_load_shader_file(line, &entry);

  // NVSM_shader_cache_entry cache_e = {};
  // strcpy(cache_e.path, entry.path);
  // strcpy(cache_e.output_path, entry.output_path);
  // strcpy(cache_e.name, entry.name);
  // cache_e.last_modified = entry.last_modified;

  // NVSM_add_shader_to_map(cache_e, out);
  NV_assert(0);
  return 0;
}

int
read_shader_spirv(const char* output, unsigned** spirv, int* spirvsize)
{
  FILE* f = fopen(output, "rb");
  if (f == NULL)
  {
    goto err;
  }

  // I'm sorry i used goto please spare me i have a loving family please no

  fseek(f, 0, SEEK_END);
  size_t fsize = ftell(f);
  if (fsize == (size_t)-1)
  {
    goto err;
  }
  fseek(f, 0, SEEK_SET);

  unsigned* buffer = NV_malloc(fsize);
  if (!buffer)
  {
    goto err;
  }

  fread(buffer, 1, fsize, f);

  fclose(f);

  *spirv     = buffer;
  *spirvsize = fsize;
  return 0;

err:
  CSM_NV_LOG_ERROR("Could not read in spirv for output path \"%s\"", output);
  if (f)
  {
    fclose(f);
    *spirv     = NULL;
    *spirvsize = 0;
  }
  return -1;
}

#include "../external/volk/volk.h"
#include "../include/GPU/pipeline.h"
#include "../include/GPU/vk.h"
#include "../include/GPU/vkstdafx.h"

void
__NVSM_create_shader(VkDevice vkdevice, const unsigned* bytes, int nbytes, struct NVSM_shader_t* out)
{
  // SpvReflectShaderModule reflect_module;
  // spvReflectCreateShaderModule(nbytes, bytes, &reflect_module);

  // reflect_shader_descriptors(&reflect_module, out);

  // spvReflectDestroyShaderModule(&reflect_module);

  const VkShaderModuleCreateInfo info = {
    .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = nbytes,
    .pCode    = bytes,
  };
  NVVK_ResultCheck(vkCreateShaderModule(vkdevice, &info, NOVA_VK_ALLOCATOR, (VkShaderModule*)&out->shader_module));
}

void
NVSM_register_all_shaders(VkDevice vkdevice, struct NVSM_shader_entry* entries, int nentries)
{
  struct NVSM_shader_t* new_shader_map = NV_malloc((nshaders + nentries) * sizeof(struct NVSM_shader_t));
  if (shader_map)
  {
    NV_memcpy(new_shader_map, shader_map, nshaders * sizeof(struct NVSM_shader_t));
    NV_free(shader_map);
  }
  shader_map = new_shader_map;

  int index  = 0;
  for (int i = 0; i < nentries; i++)
  {
    if (NV_strncmp(entries[i].stage, "vert", 4) == 0)
    {
      shader_map[nshaders + index].stage = VK_SHADER_STAGE_VERTEX_BIT;
    }
    else if (NV_strncmp(entries[i].stage, "frag", 4) == 0)
    {
      shader_map[nshaders + index].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    else if (NV_strncmp(entries[i].stage, "tese", 4) == 0)
    {
      shader_map[nshaders + index].stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    }
    else if (NV_strncmp(entries[i].stage, "tesc", 4) == 0)
    {
      shader_map[nshaders + index].stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    }
    else if (NV_strncmp(entries[i].stage, "geom", 4) == 0)
    {
      shader_map[nshaders + index].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
    }
    else if (NV_strncmp(entries[i].stage, "comp", 4) == 0)
    {
      shader_map[nshaders + index].stage = VK_SHADER_STAGE_COMPUTE_BIT;
    }
    else
    {
      CSM_NV_LOG_ERROR("Invalid stage for shader \"%s\". It will not be added.", entries[i].name);
      continue;
    }
    NV_strncpy(shader_map[nshaders + index].name, entries[i].name, 127);
    shader_map[nshaders + index].name[127] = '\0';

    unsigned* spirv;
    int       spirvsize = 0;
    if (read_shader_spirv((const char*)entries[i].output_path, &spirv, &spirvsize) != 0)
    {
      continue;
    }
    __NVSM_create_shader(vkdevice, spirv, spirvsize, &shader_map[nshaders + index]);

    nshaders++;
  }
  qsort(shader_map, nshaders, sizeof(struct NVSM_shader_t), compare_shader_t);
}

#endif // CSM_EXECUTABLE != 1

void
NVSM_set_list_file(const char* path)
{
  list = path;
}

void
NVSM_set_shader_compiler(const char* exec)
{
  shader_compiler = exec;
}

void
NVSM_set_shader_compiler_args(const char* args)
{
  shader_compiler_args = args;
}

char const*
NVSM_get_shader_compiler_args(void)
{
  return shader_compiler_args;
}

NVSM_shader_cache_entry*
load_cache(int* count)
{
  FILE* f = fopen("shaders.cache", "rb");
  if (f == NULL)
  {
    *count = 0;
    return NULL;
  }

  NV_assert(fread(count, sizeof(int), 1, f) == 1);
  NVSM_shader_disk* write = (NVSM_shader_disk*)calloc(*count, sizeof(NVSM_shader_disk));
  NV_assert(fread(write, sizeof(NVSM_shader_disk), *count, f) == (size_t)(*count));

  NVSM_shader_cache_entry* entries = calloc(*count, sizeof(NVSM_shader_cache_entry));
  for (int i = 0; i < (*count); i++)
  {
    NV_strncpy(entries[i].name, write[i].name, 128);
    NV_strncpy(entries[i].path, write[i].path, 128);
    entries[i].last_modified = write[i].last_modified;
  }

  fclose(f);
  return entries;
}

void
update_cache(const NVSM_shader_cache_entry* restrict entries, int count)
{
  FILE* f = fopen("shaders.cache", "wb");
  if (!f)
  {
    CSM_NV_LOG_ERROR("Could not open cache file for update. return.");
    return;
  }

  NVSM_shader_disk* write = NV_malloc(sizeof(NVSM_shader_disk) * count);

  for (int i = 0; i < count; i++)
  {
    NV_strncpy(write[i].name, entries[i].name, 128);
    NV_strncpy(write[i].path, entries[i].path, 128);
    write[i].last_modified = entries[i].last_modified;
  }

  NV_assert(fwrite(&count, sizeof(int), 1, f) == 1);
  NV_assert(fwrite(write, sizeof(NVSM_shader_disk), count, f) == (size_t)count);

  fclose(f);
}

void
write_new_cache(const NVSM_shader_entry* restrict entries, int count)
{
  FILE* f = fopen("shaders.cache", "wb");
  NV_assert(f != NULL);

  NVSM_shader_disk* write = NV_malloc(sizeof(NVSM_shader_disk) * count);

  for (int i = 0; i < count; i++)
  {
    NV_strncpy(write[i].name, entries[i].name, 128);
    NV_strncpy(write[i].path, entries[i].path, 128);
    write[i].last_modified = entries[i].last_modified;
  }

  NV_assert(fwrite(&count, sizeof(int), 1, f) == 1);
  NV_assert(fwrite(write, sizeof(NVSM_shader_disk), count, f) == (size_t)count);

  fclose(f);

  NV_LOG_INFO("CSM cache written successfully");
}

void
create_parent_dirs(const char path[256])
{
  char* last_separator = NV_strrchr(path, PATH_SEP);
  if (last_separator != NULL)
  {
    *last_separator = '\0';

    char buffer[256];

    NV_strcpy(buffer, path);
    int len = NV_strlen(buffer);

    if (buffer[len - 1] == PATH_SEP)
    {
      buffer[len - 1] = 0;
    }

    for (char* p = buffer + 1; *p; p++)
    {
      if (*p == PATH_SEP)
      {
        *p = 0;
        MKDIR(buffer);
        *p = PATH_SEP;
      }
    }

    MKDIR(buffer);
  }
}

#include <errno.h>
time_t
get_mtime(const char* fpath)
{
  struct stat file_stats;
  if (stat(fpath, &file_stats) == 0)
  {
    return file_stats.st_mtime;
  }
  else
  {
    CSM_NV_LOG_ERROR("stat error %i", errno);
  }
  return -1;
}

NVSM_shader_entry*
load_all_entries(const char* shader_list_file_path, int* count)
{
  FILE* f = fopen(shader_list_file_path, "r");
  if (f == NULL)
  {
    CSM_NV_LOG_ERROR("Could not open list file for reading. Is the path correct?");
    return NULL;
  }

  int                currallocsize = 16;
  NVSM_shader_entry* entries       = NV_malloc(currallocsize * sizeof(NVSM_shader_entry));
  NV_assert(entries != NULL);

  char line[256];
  for (int i = 0;; i++)
  {
    if (!fgets(line, 256, f))
    {
      break;
    }
    else if (NV_strlen(line) == 1)
      continue; // line only contains \n
    line[NV_strcspn(line, "\n")] = 0;

    if (i >= currallocsize)
    {
      currallocsize *= 2;
      entries = realloc(entries, currallocsize * sizeof(NVSM_shader_entry));
      NV_assert(entries != NULL);
    }

    entries[*count]          = (NVSM_shader_entry){};
    NVSM_shader_entry* entry = &entries[*count];

    NV_strncpy(entry->path, line, 256);

    // to get stage + verify that it exists
    FILE* shader_file = fopen(entry->path, "r");
    if (shader_file == NULL)
    {
      CSM_NV_LOG_ERROR("Could not open shader file \"%s\". Are you sure that it exists?", entry->path);
      continue;
    }

    NV_assert(fgets(line, 256, shader_file) != NULL);

    const char* li = line;
    while (*li == ' ')
    {
      li++;
    }
    if (NV_strncmp(li, "//", 2) == 0)
    {
      sscanf(li, "// output: %s stage: %s name: %s", entry->output_path, entry->stage, entry->name);
    }
    else
    {
      NV_strcpy(entry->stage, "000");
      NV_strcpy(entry->output_path, "");
      CSM_NV_LOG_ERROR("Shader \"%s\": has invalid or no header.\nHeader Format "
                       "-> // output: "
                       "{output} stage: {stage} name: {name}",
          entry->path);
    }

    fclose(shader_file);

    entry->last_modified = get_mtime(entry->path);

    (*count)++;
  }

  fclose(f);
  return entries;
}

#if defined(__linux)
// int __NVSM_linux_run() {

// }
// #define system __NVSM_linux_run
#endif

static char* g_Buffer = NULL;
int
compile_shader(const struct NVSM_shader_entry* entry)
{
  if (!g_Buffer)
  {
    g_Buffer = calloc(1, 1024);
  }
  char copy[256];
  copy[255] = '\0';
  NV_strncpy(copy, entry->output_path, 255);
  create_parent_dirs(copy);

  NV_snprintf(g_Buffer, 1024, "%s %s %s -o %s -S %s", shader_compiler, shader_compiler_args, entry->path, NV_strcmp(entry->output_path, "") != 0 ? entry->output_path : "",
      entry->stage);

  if (system(g_Buffer) != 0)
  {
    return -1;
  }
  g_Buffer[1023] = 0;

  return 0;
}

void
NVSM_compile_from_cache(NVSM_shader_entry* entries, int nentries, NVSM_shader_cache_entry* cacheentries, int cachecount)
{
  for (int i = 0; i < nentries; i++)
  {
    for (int j = 0; j < cachecount; j++)
    {
      if (NV_strcmp(cacheentries[j].path, entries[i].path) == 0)
      {
        if (cacheentries[j].last_modified != entries[i].last_modified)
        {
          if (compile_shader(&entries[i]) != 0)
          {
            CSM_NV_LOG_ERROR("Error while compiling shader \"%s\".", entries[i].path);
          }
          cacheentries[j].last_modified = entries[i].last_modified;
        }
        break;
      }
    }
  }
}

void
NVSM_compile_without_cache(NVSM_shader_entry* entries, int nentries)
{
  for (int i = 0; i < nentries; i++)
  {
    if (compile_shader(&entries[i]) != 0)
    {
      CSM_NV_LOG_ERROR("Error while compiling shader \"%s\".", entries[i].path);
    }
  }
}

void
NVSM_compile_updated()
{
  NV_LOG_CUSTOM("csm:", "Shader compilation begin");
  timer                    stopwatch    = timer_begin(0.1);

  int                      nentries     = 0;
  NVSM_shader_entry*       entries      = load_all_entries(list, &nentries);

  int                      cachecount   = 0;
  NVSM_shader_cache_entry* cacheentries = load_cache(&cachecount);

  if (cacheentries == NULL)
  {
    CSM_NV_LOG_ERROR("Could not open cache for reading. return.");
    NVSM_compile_without_cache(entries, nentries);
  }
  else
  {
    NVSM_compile_from_cache(entries, nentries, cacheentries, cachecount);
  }

  if (cacheentries == NULL)
  {
    CSM_NV_LOG_ERROR("No cache or modified cache. Writing new cache file...");
    write_new_cache(entries, nentries);
  }
  else
  {
    update_cache(cacheentries, nentries);
  }

#if CSM_EXECUTABLE != 1
  NVSM_register_all_shaders(device, entries, nentries);
#endif // CSM_EXECUTABLE != 1

  NV_free(entries);

  if (cacheentries)
    NV_free(cacheentries);

  NV_LOG_CUSTOM("csm:", "Shader compilation end (Task took %f seconds)", timer_time_since_start(&stopwatch));
}

void
NVSM_compile_all()
{
  NV_LOG_CUSTOM("csm:", "Shader compilation begin");
  timer              stopwatch = timer_begin(0.1);

  int                count     = 0;
  NVSM_shader_entry* entries   = load_all_entries(list, &count);

#if CSM_EXECUTABLE != 1
  NVSM_register_all_shaders(device, entries, count);
#endif // #if CSM_EXECUTABLE != 1

  for (int i = 0; i < count; i++)
  {
    compile_shader(&entries[i]);
  }

  write_new_cache(entries, count);

  NV_free(entries);

  NV_LOG_CUSTOM("csm:", "Shader compilation end (Task took %f seconds)", timer_time_since_start(&stopwatch));
}

void
NVSM_shutdown()
{
#if !(CSM_EXECUTABLE)
  for (int i = 0; i < nshaders; i++)
  {
    NVSM_shader_t* shader = &shader_map[i];
    vkDestroyShaderModule(device, shader->shader_module, NOVA_VK_ALLOCATOR);
  }
#endif
}

#endif // CSM

#if (FONTC)

#include <stdio.h>
#include <stdlib.h>

#include "../common/printf.h"
#include "../common/string.h"
#include "../common/timer.h"

#include "../include/engine/fontc.h"

#if (FONTC_EXECUTABLE)
#include "../common/timer.h"

static const char* FONTC_HELP_MSG = "usage:\n./fontc < Font file path to bake "
                                    "> (optionally, ) -o < output file >";

int
main(int argc, char* argv[])
{
  if (argc < 2)
  {
    NV_LOG_CUSTOM("help", "%s", FONTC_HELP_MSG);
    return -1;
  }

  const char* file = argv[1];
  const char* out;

  if (argc > 2)
  {
    out = argv[3];
  }
  else
  {
    out = "bakedfont";
  }

  timer tm = timer_begin(__FLT_MAX__);
  NV_LOG_INFO("start");
  fontc_bake_font(file, out);
  NV_LOG_INFO("finished in %.2f s", timer_time_since_start(&tm));
  return 0;
}

#endif // FONTC_EXECUTABLE

fontc_atlas_t
fontc_atlas_init(int init_w, int init_h)
{
  fontc_atlas_t atlas      = {};

  atlas.width              = init_w;
  atlas.height             = init_h;
  atlas.next_x             = 0;
  atlas.next_y             = 0;
  atlas.current_row_height = 0;
  atlas.data               = calloc(init_w, init_h);

  return atlas;
}

bool
fontc_atlas_add_image(fontc_atlas_t* __restrict__ atlas, int w, int h, const unsigned char* __restrict__ data, int* __restrict__ x, int* __restrict__ y)
{
  const int padding = 4;
  const int prev_h = atlas->height, prev_w = atlas->width;
  bool      realloc_needed = 0;
  if (w > atlas->width)
  {
    // ! This doesn't work because the old image is not correctly copied by
    // realloc ! It'll be fixed by copying over the data row by row probably
    // doesn't need fixing right now, will delay it for another eon
    // TODO: FIXME
    atlas->width   = w;
    realloc_needed = 1;
  }

  if (atlas->next_x + w + padding > atlas->width)
  {
    atlas->next_x = 0;
    atlas->next_y += atlas->current_row_height + padding;
    atlas->current_row_height = 0;
  }

  if (atlas->next_y + h + padding > atlas->height)
  {
    atlas->height  = NVM_MAX(atlas->height * 2, atlas->next_y + h + padding);
    realloc_needed = 1;
  }

  if (realloc_needed)
  {
    atlas->data = realloc(atlas->data, atlas->width * atlas->height);
    NV_memset(atlas->data + prev_w * prev_h, 0, (atlas->width - prev_w) * (atlas->height - prev_h));
  }

  for (int y = 0; y < h; y++)
  {
    NV_memcpy(atlas->data + (atlas->next_x + (atlas->next_y + y) * atlas->width), data + (y * w), w);
  }

  *x = atlas->next_x;
  *y = atlas->next_y;

  atlas->next_x += w + padding;
  atlas->current_row_height = NVM_MAX(atlas->current_row_height, h + padding);

  return 0;
}

void
fontc_read_font(const char* path, fontc_file* file)
{
  FILE* f = fopen(path, "rb");
  if (!f)
  {
    NV_LOG_ERROR("Failed to open font file for reading");
    NV_LOG_ERROR("Are you passing in the path to the baked file?");
    return;
  }
  fread(&file->header, sizeof(fontc_header), 1, f);
  if (file->header.magic != FONTC_MAGIC)
  {
    NV_LOG_ERROR("Invalid magic number for font file");
    return;
  }

  size_t total_glyph_size          = file->header.numglyphs * sizeof(fontc_glyph);

  file->glyphs                     = NV_malloc(total_glyph_size);
  file->bitmap                     = NV_malloc(file->header.bmpwidth * file->header.bmpheight);
  fontc_glyph*   compressed_glyphs = NV_malloc(file->header.glyphs_compressed_sz);
  unsigned char* compressed_image  = NV_malloc(file->header.img_compressed_sz);

  fread(compressed_glyphs, file->header.glyphs_compressed_sz, 1, f);
  fread(compressed_image, file->header.img_compressed_sz, 1, f);

  NV_assert(NV_bufdecompress(compressed_glyphs, file->header.glyphs_compressed_sz, file->glyphs, total_glyph_size) != -1);
  NV_assert(NV_bufdecompress(compressed_image, file->header.img_compressed_sz, file->bitmap, file->header.bmpwidth * file->header.bmpheight) != -1);

  NV_free(compressed_glyphs);
  NV_free(compressed_image);

  fclose(f);
}

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

void
fontc_bake_font(const char* font_path, const char* out)
{
  FT_Library lib;
  FT_Face    face;
  if (FT_Init_FreeType(&lib))
  {
    NV_LOG_ERROR("Failed to initialize ft");
  }

  if (FT_New_Face(lib, font_path, 0, &face))
  {
    NV_LOG_ERROR("Failed to load font file: %s", font_path);
    FT_Done_FreeType(lib);
    return;
  }
  FT_Set_Pixel_Sizes(face, 0, 256);

  fontc_file    file      = {};
  fontc_atlas_t atlas     = fontc_atlas_init(4096, 4096);

  file.header.magic       = FONTC_MAGIC;

  file.header.line_height = -face->size->metrics.height / (float)face->height;

  fontc_glyph* glyphs     = NV_malloc(sizeof(fontc_glyph) * 256);
  if (!glyphs)
  {
    NV_LOG_ERROR("Failed to allocate memory for glyphs");
    return;
  }

  int glyph_count = 0;

  for (int i = 0; i < 256; i++)
  {
    FT_UInt glyph_index = FT_Get_Char_Index(face, i);
    if (glyph_index == 0)
      continue;

    if (FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT))
      continue;

    if (i == ' ')
    {
      file.header.space_width = (float)face->glyph->metrics.horiAdvance / (float)face->units_per_EM;
      continue;
    }

    FT_Render_Glyph(face->glyph, FT_RENDER_MODE_SDF);
    FT_GlyphSlot         g      = face->glyph;

    const int            w      = g->bitmap.width;
    const int            h      = g->bitmap.rows;
    const unsigned char* buffer = g->bitmap.buffer;

    int                  x, y;
    if (fontc_atlas_add_image(&atlas, w, h, buffer, &x, &y))
    {
      NV_LOG_ERROR("Atlas error");
      continue;
    }

    FT_Glyph gl;
    FT_Get_Glyph(face->glyph, &gl);

    FT_BBox box;
    FT_Glyph_Get_CBox(gl, FT_GLYPH_BBOX_UNSCALED, &box);

    FT_Done_Glyph(gl);

    fontc_glyph glyph   = { .codepoint = i,
        .x0                            = box.xMin / (float)face->units_per_EM,
        .x1                            = box.xMax / (float)face->units_per_EM,
        .y0                            = box.yMin / (float)face->units_per_EM,
        .y1                            = box.yMax / (float)face->units_per_EM,
        .l                             = (float)(x) / atlas.width,
        .r                             = (float)(x + w) / atlas.width,
        .b                             = (float)(y + h) / atlas.height,
        .t                             = (float)(y) / atlas.height,
        .advance                       = (float)face->glyph->metrics.horiAdvance / (float)face->units_per_EM };

    glyphs[glyph_count] = glyph;
    glyph_count++;
  }

  FT_Done_Face(face);
  FT_Done_FreeType(lib);

  file.header.bmpwidth             = atlas.width;
  file.header.bmpheight            = atlas.height;
  file.header.numglyphs            = glyph_count;

  size_t         image_o_size      = atlas.width * atlas.height;
  size_t         glyph_o_size      = file.header.numglyphs * sizeof(fontc_glyph);

  unsigned char* compressed_image  = NV_malloc(image_o_size);
  fontc_glyph*   compressed_glyphs = NV_malloc(glyph_o_size);

  NV_bufcompress(atlas.data, image_o_size, compressed_image, &image_o_size);
  NV_bufcompress(glyphs, glyph_o_size, compressed_glyphs, &glyph_o_size);

  NV_free(atlas.data);
  NV_free(glyphs);

  file.glyphs                      = compressed_glyphs;

  file.header.img_compressed_sz    = image_o_size;
  file.header.glyphs_compressed_sz = glyph_o_size;

  FILE* f                          = fopen(out, "wb");
  fwrite(&file.header, sizeof(fontc_header), 1, f);
  fwrite(compressed_glyphs, glyph_o_size, 1, f);
  fwrite(compressed_image, image_o_size, 1, f);
  fclose(f);

  size_t bytes_written = 0;

  bytes_written += sizeof(fontc_header);
  bytes_written += glyph_o_size;
  bytes_written += image_o_size;

  char buf[128];
  NV_btoa(bytes_written, 1, buf, 127);
  buf[127] = 0;
  NV_LOG_INFO("Wrote %s to %s", buf, out);
}

#endif // FONTC