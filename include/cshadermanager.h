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
extern int csm_load_shader_from_disk(const char *path, const char *output_name, struct csm_shader_t **out);

#ifdef __cplusplus
}
#endif

#endif//__C_SHADERMANAGER_H