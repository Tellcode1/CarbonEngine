#ifndef __SCENE_H__
#define __SCENE_H__

#include "lunaObject.h"

typedef struct lunaScene lunaScene;
typedef struct luna_Renderer_t luna_Renderer_t;

extern lunaScene *g_scenes;
extern int g_nscenes;

extern lunaScene *lunaScene_Init();
extern void lunaScene_Update(lunaScene *scene);
extern void lunaScene_Render(lunaScene *scene, luna_Renderer_t *rd);

#endif//__SCENE_H__