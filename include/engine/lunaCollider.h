#ifndef __LUNA_COLLIDER_H__
#define __LUNA_COLLIDER_H__

#include "../math/vec2.h"

typedef struct lunaCollider lunaCollider;
typedef struct lunaScene lunaScene;

typedef enum lunaCollider_Type {
    LUNA_COLLIDER_TYPE_STATIC = 0,
    LUNA_COLLIDER_TYPE_DYNAMIC = 1,
    LUNA_COLLIDER_TYPE_KINEMATIC = 2
} lunaCollider_Type;

typedef enum lunaCollider_Shape {
    LUNA_COLLIDER_SHAPE_RECT = 0,
    LUNA_COLLIDER_SHAPE_CIRCLE = 1, // only x of size is used
    LUNA_COLLIDER_SHAPE_CAPSULE = 2, // x of size is radius and y is height.
} lunaCollider_Shape;

typedef struct lunaCollider_RayHit {
    const lunaCollider *host;
    lunaCollider *other;
    vec2 point_of_contact;
    bool hit;
} lunaCollider_RayHit;

extern lunaCollider *lunaCollider_Init(lunaScene *scene, vec2 position, vec2 size, lunaCollider_Type type, lunaCollider_Shape shape, bool start_enabled);
extern void lunaCollider_Destroy(lunaCollider *col);

extern vec2 lunaCollider_GetPosition(const lunaCollider *col);
extern void lunaCollider_SetPosition(lunaCollider *col, vec2 to);

vec2 lunaCollider_GetSize(const lunaCollider *col);
void lunaCollider_SetSize(lunaCollider *col, vec2 to);

extern lunaCollider_RayHit lunaCollider_RayCast(const lunaCollider *col, vec2 orig, vec2 dir);

// You need to update the colliders through luneScene_Update();

#endif//__LUNA_COLLIDER_H__