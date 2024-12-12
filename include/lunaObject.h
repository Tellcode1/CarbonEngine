#ifndef __LUNA_OBJECT_H__
#define __LUNA_OBJECT_H__

#ifdef __cplusplus
    extern "C" {
#endif

#include <stdbool.h>
#include <SDL2/SDL.h>
#include "math/vec2.h"
#include "lunaGFX.h"

typedef void (*LunaObjectEventFn)(const SDL_Event *evt);
typedef struct luna_Object luna_Object;

void luna_Object_Initialize();

luna_Object *luna_CreateObject(
    const char *name,
    bool is_static, // is body static? or is it dynamic?
    vec2 position,
    vec2 size
);

void luna_ObjectsUpdate();
void luna_ObjectsRender(luna_Renderer_t *rd);

bool luna_ObjectRayCast(const luna_Object *obj, vec2 orig, vec2 dir);

void luna_ObjectMove(luna_Object *obj, vec2 add);
vec2 luna_ObjectGetPosition(const luna_Object *obj);
vec2 luna_ObjectGetVelocity(const luna_Object *obj);
void luna_ObjectSetVelocity(luna_Object *obj, vec2 vel);

void __luna_Object_PassEventToObjects(const SDL_Event *evt);

#ifdef __cplusplus
    }
#endif

#endif//__LUNA_OBJECT_H__