#ifndef __CSM_DEV_H
#define __CSM_DEV_H

extern const char *shader_compiler;
extern const char *shader_compiler_args;
extern const char *list;

typedef struct csm_shader_t {
    char name[128];
    void *shader_module;
    unsigned stage;
} csm_shader_t;

extern struct csm_shader_t *shader_map;
extern int nshaders;

typedef struct shader_entry {
    char path[256];
    char output_path[256];
    char name[128];
    char stage[4];
    long last_modified;
} shader_entry;

typedef struct shader_cache_entry {
    char path[256];
    char output_path[256];
    char name[128];
    long last_modified;
} shader_cache_entry;

void __csm_create_shader(struct VkDevice_T *vkdevice, const unsigned *bytes, int nbytes, struct csm_shader_t *out);

#endif//__CSM_DEV_H