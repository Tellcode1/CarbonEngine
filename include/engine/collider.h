#ifndef __NV_COLLIDER_H__
#define __NV_COLLIDER_H__

#include "../../common/math/vec2.h"

NOVA_HEADER_START;

typedef struct NVCollider NVCollider;
typedef struct NVScene NVScene;

typedef enum NVCollider_Type { NOVA_COLLIDER_TYPE_STATIC = 0, NOVA_COLLIDER_TYPE_DYNAMIC = 1, NOVA_COLLIDER_TYPE_KINEMATIC = 2 } NVCollider_Type;

typedef enum NVCollider_Shape {
  NOVA_COLLIDER_SHAPE_RECT    = 0,
  NOVA_COLLIDER_SHAPE_CIRCLE  = 1, // only x of size is used
  NOVA_COLLIDER_SHAPE_CAPSULE = 2, // x of size is radius and y is height.
} NVCollider_Shape;

typedef struct NVCollider_RayHit {
  const NVCollider *host;
  NVCollider *other;
  vec2 point_of_contact;
  bool hit;
} NVCollider_RayHit;

// mask defines the layers that the collider can collide with
// both layer and mask must be bitmasks
extern NVCollider *NVCollider_Init(NVScene *scene, vec2 position, vec2 size, NVCollider_Type type, NVCollider_Shape shape, uint64_t layer,
                                       uint64_t mask, bool start_enabled);
extern void NVCollider_Destroy(NVCollider *col);

extern vec2 NVCollider_GetPosition(const NVCollider *col);
extern void NVCollider_SetPosition(NVCollider *col, vec2 to);

vec2 NVCollider_GetSize(const NVCollider *col);
void NVCollider_SetSize(NVCollider *col, vec2 to);

extern NVCollider_RayHit NVCollider_RayCast(const NVCollider *col, vec2 orig, vec2 dir, uint32_t layer, uint32_t mask);

// You need to update the colliders through luneScene_Update();

NOVA_HEADER_END;

#endif //__NOVA_COLLIDER_H__