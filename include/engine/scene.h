#ifndef __NV_SCENE_H__
#define __NV_SCENE_H__

#include "../../common/stdafx.h"

NOVA_HEADER_START;

typedef struct NVScene NVScene;
typedef struct NVRenderer_t NVRenderer_t;

typedef void (*NVSceneLoadFn)(NVScene *scn);

// Called when the scene scn changes
// ie. it's called when scn is being unloaded
typedef void (*NVSceneUnloadFn)(NVScene *scn);

extern NVScene *scene_main;

extern NVScene *NVScene_Init();
extern void NVScene_Update();
extern void NVScene_Render(NVRenderer_t *rd);
extern void NVScene_Destroy(NVScene *scene);
extern void NVScene_AssignLoadFn(NVScene *scene, NVSceneLoadFn fn);
extern void NVScene_AssignUnloadFn(NVScene *scene, NVSceneUnloadFn fn);
extern void NVScene_ChangeToScene(NVScene *scene);

NOVA_HEADER_END;

#endif //__NOVA_SCENE_H__