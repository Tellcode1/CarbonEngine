#ifndef __NV_COLLIDER_H__
#define __NV_COLLIDER_H__

#include "../../common/math/vec2.h"

NOVA_HEADER_START;

typedef struct NV_collider_t NV_collider_t;
typedef struct NV_scene_t NV_scene_t;

typedef enum NV_collider_type { NOVA_COLLIDER_TYPE_STATIC = 0, NOVA_COLLIDER_TYPE_DYNAMIC = 1, NOVA_COLLIDER_TYPE_KINEMATIC = 2 } NV_collider_type;

typedef enum NV_collider_shape {
  NOVA_COLLIDER_SHAPE_RECT    = 0,
  NOVA_COLLIDER_SHAPE_CIRCLE  = 1, // only x of size is used
  NOVA_COLLIDER_SHAPE_CAPSULE = 2, // x of size is radius and y is height.
} NV_collider_shape;

typedef struct NV_collider_ray_hit {
  const NV_collider_t *host;
  NV_collider_t *other;
  vec2 point_of_contact;
  bool hit;
} NV_collider_ray_hit;

// mask defines the layers that the collider can collide with
// both layer and mask must be bitmasks
extern NV_collider_t *NV_collider_Init(NV_scene_t *scene, vec2 position, vec2 size, NV_collider_type type, NV_collider_shape shape, uint64_t layer,
                                       uint64_t mask, bool start_enabled);
extern void NV_collider_Destroy(NV_collider_t *col);

extern vec2 NV_collider_GetPosition(const NV_collider_t *col);
extern void NV_collider_SetPosition(NV_collider_t *col, vec2 to);

vec2 NV_collider_GetSize(const NV_collider_t *col);
void NV_collider_SetSize(NV_collider_t *col, vec2 to);

extern NV_collider_ray_hit NV_collider_cast_ray(const NV_collider_t *col, vec2 orig, vec2 dir, uint32_t layer, uint32_t mask);

// You need to update the colliders through luneScene_Update();

NOVA_HEADER_END;

#endif //__NOVA_COLLIDER_H__