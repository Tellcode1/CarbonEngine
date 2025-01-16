#ifndef __IMPACT_H__
#define __IMPACT_H__

#ifndef impact_float
#define impact_float double
#endif

#include "../../common/math/vec3.h"
#include <stdbool.h>

typedef struct IRect IRect;
typedef struct IBody IBody;
typedef struct IWorld IWorld;

typedef struct IWorldCreateInfo IWorldCreateInfo;
typedef struct IBodyCreateInfo IBodyCreateInfo;

typedef enum IBodyType {
  // A body that cannot be moved once created, and is not affected by gravity or other objects
  // very cheap and efficient
  // eg. A rock in the terrain, it won't move, and the player will not move it
  IMPACT_BODY_STATIC = 0,

  // A body that can be moved both by the user (through code) and by the physics system
  // The most expensive and slow
  // eg. The player or an enemy
  IMPACT_BODY_DYNAMIC = 1,

  // A body that can be moved through code, but is not interactable by the physics system
  // It won't be affected by gravity, but will still move if you try pushing it into a wall
  // Not very expensive
  // This is generally for something like a trigger
  IMPACT_BODY_KINEMATIC = 2,

  // A very fast body, which is not affected by gravity, or code
  // It's velocity remains constant once created and can not be changed
  // Generally cheap but can drastically get expensive when a lot of them are on screen if bullet raycasting is not enabled
  // For, obviously, bullets and other high speed, unchanging objects
  IMPACT_BODY_BULLET = 4
} IBodyType;

struct IRect {
  vec3 position;
  vec3 half_size;
};

// IBody should be seperated by the type of body
struct IBody {
  IRect rect;
  vec3 velocity;

  impact_float mass;
  impact_float friction;
  impact_float restitution;

  IBodyType type;
  int index;
  bool enabled;
  bool sleeping;
};

struct IWorld {
  vec3 gravity;
  int nbodies;
  IBody bodies[256];
};

struct IWorldCreateInfo {
  vec3 gravity;
};

struct IBodyCreateInfo {
  vec3 position;
  vec3 half_size;
  IBodyType type;
  impact_float restitution;
  bool start_disabled;
};

extern IWorld IWorldCreate(const IWorldCreateInfo *pInfo);
extern void IWorldDestroy(IWorld *world);

extern IBody *IBodyCreate(IWorld *world, const IBodyCreateInfo *pInfo);
extern void IBodyDestroy(IBody *body);

extern void IBodyMove(IBody *body, vec3 to);

extern vec3 IBodyGetPosition(const IBody *body);
extern vec3 IBodyGetHalfSize(const IBody *body);

extern void IBodySetPosition(IBody *body, vec3 position);
extern void IBodySetHalfSize(IBody *body, vec3 size);

extern void IBodySetLinearVelocity(IBody *body, vec3 vel);
extern vec3 IBodyGetLinearVelocity(const IBody *body);

extern void IWorldStep(IWorld *world, impact_float dt);

#endif //__IMPACT_H__