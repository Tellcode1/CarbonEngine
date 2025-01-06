#ifndef __LUNA_SCENE_H__
#define __LUNA_SCENE_H__

#include "lunaObject.h"

typedef struct lunaScene lunaScene;
typedef struct luna_Renderer_t luna_Renderer_t;

typedef void (*lunaSceneLoadFn)(lunaScene *scn);

// Called when the scene scn changes
// ie. it's called when scn is being unloaded
typedef void (*lunaSceneUnloadFn)(lunaScene *scn);

extern lunaScene *scene_main;

extern lunaScene *lunaScene_Init();
extern void lunaScene_Update();
extern void lunaScene_Render(luna_Renderer_t *rd);
extern void lunaScene_Destroy(lunaScene *scene);
extern void lunaScene_AssignLoadFn(lunaScene *scene, lunaSceneLoadFn fn);
extern void lunaScene_AssignUnloadFn(lunaScene *scene, lunaSceneUnloadFn fn);
extern void lunaScene_ChangeToScene(lunaScene *scene);

#endif//__LUNA_SCENE_H__