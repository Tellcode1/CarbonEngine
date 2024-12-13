#ifndef __LUNA_OBJECT_H__
#define __LUNA_OBJECT_H__

#ifdef __cplusplus
    extern "C" {
#endif

#include <stdbool.h>
#include <SDL2/SDL.h>
#include "math/vec2.h"
#include "lunaGFX.h"

typedef void (*LunaObjectUpdateFn)(float dt);
typedef void (*LunaObjectRenderFn)(luna_Renderer_t *rd);
typedef struct luna_Object luna_Object;

void luna_Object_Initialize();

luna_Object *luna_CreateObject(
    const char *name,
    bool is_static, // is body static? or is it dynamic?
    vec2 position,
    vec2 size
);

typedef struct luna_ObjectRayHit {
    luna_Object *host;
    luna_Object *other;
    vec2 point_of_contact;
    bool hit;
} luna_ObjectRayHit;

typedef struct luna_SpriteRenderer {
    sprite_t *spr;
    bool flip_horizontal;
    bool flip_vertical;
    vec2 tex_coord_multiplier; // This is multiplied with the texture coordinates while rendering.
} luna_SpriteRenderer;

void luna_ObjectsUpdate();
void luna_ObjectsRender(luna_Renderer_t *rd);

luna_ObjectRayHit luna_ObjectRayCast(const luna_Object *obj, vec2 orig, vec2 dir);

void luna_ObjectAssignOnUpdateFn(luna_Object *obj, LunaObjectUpdateFn fn);
void luna_ObjectAssignOnRenderFn(luna_Object *obj, LunaObjectRenderFn fn);

void luna_ObjectMove(luna_Object *obj, vec2 add);
vec2 luna_ObjectGetPosition(const luna_Object *obj);
vec2 luna_ObjectGetVelocity(const luna_Object *obj);
void luna_ObjectSetVelocity(luna_Object *obj, vec2 vel);

luna_SpriteRenderer *luna_ObjectGetSpriteRenderer(luna_Object *obj);

#ifdef __cplusplus
    }
#endif

#endif//__LUNA_OBJECT_H__