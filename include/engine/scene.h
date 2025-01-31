#ifndef __NV_SCENE_H__
#define __NV_SCENE_H__

#include "../../common/stdafx.h"

NOVA_HEADER_START;

typedef struct NV_scene_t NV_scene_t;
typedef struct NV_renderer_t NV_renderer_t;

typedef void (*NV_scene_load_fn)(NV_scene_t *scn);

// Called when the scene scn changes
// ie. it's called when scn is being unloaded
typedef void (*NV_scene_unload_fn)(NV_scene_t *scn);

extern NV_scene_t *scene_main;

extern NV_scene_t *NV_scene_Init();
extern void NV_scene_update();
extern void NV_scene_render(NV_renderer_t *rd);
extern void NV_scene_destroy(NV_scene_t *scene);
extern void NV_scene_assign_load_fn(NV_scene_t *scene, NV_scene_load_fn fn);
extern void NV_scene_assign_unload_fn(NV_scene_t *scene, NV_scene_unload_fn fn);
extern void NV_scene_change_to_scene(NV_scene_t *scene);

NOVA_HEADER_END;

#endif //__NOVA_SCENE_H__