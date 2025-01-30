#ifndef __C_SHADERMANAGER_H
#define __C_SHADERMANAGER_H

#include "../../common/stdafx.h"

NOVA_HEADER_START;

#ifndef NVSM_EXECUTABLE
#define NVSM_EXECUTABLE 0
#endif

typedef struct NVSM_pipeline NVSM_pipeline;

extern void NVSM_compile_updated();
extern void NVSM_compile_all();

extern void NVSM_shutdown();

extern void NVSM_set_list_file(const char *path);
extern void NVSM_set_shader_compiler(const char *exec);
extern void NVSM_set_shader_compiler_args(const char *args);
extern char const *NVSM_get_shader_compiler_args(void);

typedef struct NVSM_shader_t NVSM_shader_t; // You can still use struct NVSM_shader_t but it's less effort (Not neccessarily cleaner)

/// @brief NVSM_shader_t *shadr; NVSM_load_shader("Unlit/vertex", &shadr);
/// @return -1 on failure. 0 on success
extern int NVSM_load_shader(const char *name, struct NVSM_shader_t **out);

/*
    You can pass NULL to output_name to signify that no output file should be created and it should not be added to the cache.
    The shader will then live entirely in memory but can still be referenced(NVSM_load_shader) by name.
*/
extern int NVSM_load_shader_from_disk(const char *path, struct NVSM_shader_t **out);

NOVA_HEADER_END;

#endif //__C_SHADERMANAGER_H