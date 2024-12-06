#ifndef __CSM_DEV_H
#define __CSM_DEV_H

#include <stdbool.h>

extern const char *shader_compiler;
extern const char *shader_compiler_args;
extern const char *list;

typedef struct csm_shader_entry {
    char path[256];
    char output_path[256];
    char name[128];
    char stage[4];
    long last_modified;
} csm_shader_entry;

typedef struct csm_shader_t {
    char name[128];
    void *shader_module; // the vk shader handle
    unsigned stage;
} csm_shader_t;

extern struct csm_shader_t *shader_map;
extern int nshaders;

typedef struct csm_shader_cache_entry {
    char path[256];
    char output_path[256];
    char name[128];
    long last_modified;
} csm_shader_cache_entry;

extern void __csm_create_shader(struct VkDevice_T *vkdevice, const unsigned *bytes, int nbytes, struct csm_shader_t *out);
void __csm_vk_register_shaders(csm_shader_entry *entries, int nentries);

#endif//__CSM_DEV_H