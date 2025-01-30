#ifndef __NVSM_DEV_H
#define __NVSM_DEV_H

#include "../../common/stdafx.h"

NOVA_HEADER_START;

typedef struct VkShaderModule_T VkShaderModule_T;

extern const char *shader_compiler;
extern const char *shader_compiler_args;
extern const char *list;

typedef struct NVSM_shader_entry {
  char path[256];
  char output_path[256];
  char name[128];
  char stage[4];
  long last_modified;
} NVSM_shader_entry;

// how the NVSM shader is stored on the disk
typedef struct NVSM_shader_disk {
  char name[128];
  char path[128];
  long last_modified;
} NVSM_shader_disk;

typedef struct NVSM_shader_t {
  char name[128];
  VkShaderModule_T *shader_module; // the vk shader handle
  unsigned stage;
} NVSM_shader_t;

extern struct NVSM_shader_t *shader_map;
extern int nshaders;

typedef struct NVSM_shader_cache_entry {
  char path[256];
  char output_path[256];
  char name[128];
  long last_modified;
} NVSM_shader_cache_entry;

typedef struct VkDevice_T VkDevice_T;

extern void __NVSM_create_shader(VkDevice_T *__restrict vkdevice, const unsigned *__restrict bytes, int nbytes, struct NVSM_shader_t *__restrict out);
void __NVSM_vk_register_shaders(NVSM_shader_entry *entries, int nentries);

NOVA_HEADER_END;

#endif //__NVSM_DEV_H