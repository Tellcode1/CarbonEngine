#ifndef __LUNA_OBJECT_H__
#define __LUNA_OBJECT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "../math/vec2.h"
#include "../math/vec4.h"
#include "lunaCollider.h"
#include "lunaSpriteRenderer.h"

#include <stdbool.h>

typedef struct lunaTransform {
  vec2 position, size;
  vec4 rotation;
} lunaTransform;

typedef void (*lunaObjectUpdateFn)(float dt);
typedef void (*lunaObjectRenderFn)(lunaRenderer_t *rd);
typedef struct lunaObject lunaObject;

typedef enum lunaObject_Flags {
  LUNA_OBJECT_NO_COLLISION = 1,
} lunaObject_Flags;

extern lunaObject *lunaObject_Create(lunaScene *scene, const char *name,
                                     lunaCollider_Type col_type, uint64_t layer, uint64_t mask, vec2 position,
                                     vec2 size, unsigned flags);
extern void lunaObject_Destroy(lunaObject *obj);

extern void lunaObject_AssignOnUpdateFn(lunaObject *obj, lunaObjectUpdateFn fn);
extern void lunaObject_AssignOnRenderFn(lunaObject *obj, lunaObjectRenderFn fn);

extern void lunaObject_Move(lunaObject *obj, vec2 add);

extern vec2 lunaObject_GetPosition(const lunaObject *obj);
extern void lunaObject_SetPosition(lunaObject *obj, vec2 to);

extern vec2 lunaObject_GetSize(const lunaObject *obj);
extern void lunaObject_SetSize(lunaObject *obj, vec2 to);

extern const char *lunaObject_GetName(lunaObject *obj);
extern void lunaObject_SetName(lunaObject *obj, const char *name);

extern lunaTransform *lunaObject_GetTransform(lunaObject *obj);
extern lunaCollider *lunaObject_GetCollider(lunaObject *obj);
extern luna_SpriteRenderer *lunaObject_GetSpriteRenderer(lunaObject *obj);

#ifdef __cplusplus
}
#endif

#endif //__LUNA_OBJECT_H__