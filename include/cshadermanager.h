#ifndef __C_SHADERMANAGER_H
#define __C_SHADERMANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CSM_EXECUTABLE
    #define CSM_EXECUTABLE 0
#endif

extern void csm_compile_updated();
extern void csm_compile_all();

extern void csm_set_list_file(const char *path);
extern void csm_set_shader_compiler(const char *exec);
extern void csm_set_shader_compiler_args(const char *args);
extern char const *csm_get_shader_compiler_args(void);

struct csm_shader_t;

extern int csm_load_shader(const char *name, struct csm_shader_t **out); // csm_shader_t *shader; csm_load_shader("Generic/Unlit.vert", &shader);

/*
    You can pass NULL to output_name to signify that no output file should be created and it should not be added to the cache.
    The shader will then live entirely in memory but can still be referenced(csm_load_shader) by name.
*/
extern int csm_load_shader_from_disk(const char *path, const char *output_name, struct csm_shader_t **out);

#ifdef __cplusplus
}
#endif

#endif//__C_SHADERMANAGER_H