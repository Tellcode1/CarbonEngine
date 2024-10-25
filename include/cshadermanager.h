#ifndef __C_SHADERMANAGER_H
#define __C_SHADERMANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CSM_EXECUTABLE
    #define CSM_EXECUTABLE 0
#endif

typedef struct csm_pipeline csm_pipeline;

extern void csm_compile_updated();
extern void csm_compile_all();

extern void csm_set_list_file(const char *path);
extern void csm_set_shader_compiler(const char *exec);
extern void csm_set_shader_compiler_args(const char *args);
extern char const *csm_get_shader_compiler_args(void);

typedef struct csm_shader_t csm_shader_t; // You can still use struct csm_shader_t but it's less effort (Not neccessarily cleaner)

/// @brief csm_shader_t *shadr; csm_load_shader("Unlit/vertex", &shadr);
/// @return -1 on failure. 0 on success
extern int csm_load_shader(const char *name, struct csm_shader_t **out);

/*
    You can pass NULL to output_name to signify that no output file should be created and it should not be added to the cache.
    The shader will then live entirely in memory but can still be referenced(csm_load_shader) by name.
*/
extern int csm_load_shader_from_disk(const char *path, const char *output_name, struct csm_shader_t **out);

#ifdef __cplusplus
}
#endif

#endif//__C_SHADERMANAGER_H