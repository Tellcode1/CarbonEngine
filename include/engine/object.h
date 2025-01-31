#ifndef __NV_OBJECT_H__
#define __NV_OBJECT_H__

#include "../../common/math/vec2.h"
#include "../../common/math/vec4.h"
#include "collider.h"
#include "sprite_renderer.h"

NOVA_HEADER_START;

typedef struct NVTransform {
  vec2 position, size;
  vec4 rotation;
} NVTransform;

typedef void (*NVObjectUpdateFn)(float dt);
typedef void (*NVObjectRenderFn)(NV_renderer_t *rd);
typedef struct NVObject NVObject;

typedef enum NVObject_Flags {
  NOVA_OBJECT_NO_COLLISION = 1,
} NVObject_Flags;

extern NVObject *NVObject_Create(NV_scene_t *scene, const char *name, NV_collider_type col_type, uint64_t layer, uint64_t mask, vec2 position,
                                     vec2 size, unsigned flags);
extern void NVObject_Destroy(NVObject *obj);

extern void NVObject_AssignOnUpdateFn(NVObject *obj, NVObjectUpdateFn fn);
extern void NVObject_AssignOnRenderFn(NVObject *obj, NVObjectRenderFn fn);

extern void NVObject_Move(NVObject *obj, vec2 add);

extern vec2 NVObject_GetPosition(const NVObject *obj);
extern void NVObject_SetPosition(NVObject *obj, vec2 to);

extern vec2 NVObject_GetSize(const NVObject *obj);
extern void NVObject_SetSize(NVObject *obj, vec2 to);

extern const char *NVObject_GetName(NVObject *obj);
extern void NVObject_SetName(NVObject *obj, const char *name);

extern NVTransform *NVObject_GetTransform(NVObject *obj);
extern NV_collider_t *NVObject_GetCollider(NVObject *obj);
extern NV_SpriteRenderer *NVObject_GetSpriteRenderer(NVObject *obj);

NOVA_HEADER_END;

#endif //__NOVA_OBJECT_H__