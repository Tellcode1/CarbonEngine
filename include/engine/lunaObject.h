#ifndef __LUNA_OBJECT_H__
#define __LUNA_OBJECT_H__

#ifdef __cplusplus
    extern "C" {
#endif

#include <stdbool.h>
#include <SDL2/SDL.h>
#include "../math/vec2.h"
#include "../math/vec4.h"
#include "lunaGFX.h"
#include "lunaSpriteRenderer.h"
#include "../engine/lunaInput.h"

typedef struct lunaTransform {
    vec2 position, size;
    vec4 rotation;
} lunaTransform;

typedef void (*lunaObjectUpdateFn)(float dt);
typedef void (*lunaObjectRenderFn)(lunaRenderer_t *rd);
typedef struct lunaObject lunaObject;
typedef struct lunaScene lunaScene;

typedef enum lunaObject_Flags {
    LUNA_OBJECT_NO_COLLISION = 1,
} lunaObject_Flags;

extern lunaObject *lunaObject_Create(lunaScene *scene, const char *name, bool is_static, /*is body static? or is it dynamic?*/ vec2 position, vec2 size, unsigned flags);
extern void lunaObject_Destroy(lunaObject *obj);

extern void lunaObject_AssignOnUpdateFn(lunaObject *obj, lunaObjectUpdateFn fn);
extern void lunaObject_AssignOnRenderFn(lunaObject *obj, lunaObjectRenderFn fn);

extern void lunaObject_Move(lunaObject *obj, vec2 add);
extern vec2 lunaObject_GetPosition(const lunaObject *obj);
extern void lunaObject_SetPosition(lunaObject *obj, vec2 to);
extern vec2 lunaObject_GetSize(const lunaObject *obj);
extern void lunaObject_SetSize(lunaObject *obj, vec2 to);

extern lunaTransform *lunaObject_GetTransform(lunaObject *obj);

luna_SpriteRenderer *lunaObject_GetSpriteRenderer(lunaObject *obj);

#ifdef __cplusplus
    }
#endif

#endif//__LUNA_OBJECT_H__