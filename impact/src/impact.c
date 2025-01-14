#include "../include/impact.h"
#include "../../common/stdafx.h"
#include <tgmath.h>

#ifndef RESTRICT
#define RESTRICT __restrict
#endif

IBody *IBodyCreate(IWorld *world, const IBodyCreateInfo *pInfo) {
  cassert(fabs(pInfo->position.x) < 1000.0 && fabs(pInfo->position.y) < 1000.0);
  cassert(fabs(pInfo->half_size.x) < 1000.0 && fabs(pInfo->half_size.y) < 1000.0);

  world->bodies[world->nbodies] = (IBody){.rect.position  = pInfo->position,
                                          .rect.half_size = pInfo->half_size,
                                          .index          = world->nbodies,
                                          .type           = pInfo->type,
                                          .mass           = 1.0f,
                                          .enabled        = !pInfo->start_disabled};
  world->nbodies++;

  return &world->bodies[world->nbodies - 1];
}

void IBodyDestroy(IBody *body) {}

static inline bool IRectsIntersect(const IRect *RESTRICT r1, const IRect *RESTRICT r2) {
  const vec3 r1_min = v3sub(r1->position, r1->half_size);
  const vec3 r1_max = v3add(r1->position, r1->half_size);

  const vec3 r2_min = v3sub(r2->position, r2->half_size);
  const vec3 r2_max = v3add(r2->position, r2->half_size);

  const bool overlaps_x = r1_max.x > r2_min.x && r1_min.x < r2_max.x;
  const bool overlaps_y = r1_max.y > r2_min.y && r1_min.y < r2_max.y;
  const bool overlaps_z = r1_max.z > r2_min.z && r1_min.z < r2_max.z;

  return overlaps_x && overlaps_y && overlaps_z;
}

static inline vec3 IRectsGetOverlap(const IRect *RESTRICT r1, const IRect *RESTRICT r2) {
  const vec3 r1_min = v3sub(r1->position, r1->half_size);
  const vec3 r1_max = v3add(r1->position, r1->half_size);

  const vec3 r2_min = v3sub(r2->position, r2->half_size);
  const vec3 r2_max = v3add(r2->position, r2->half_size);

  const impact_float overlap_x = fmin(r1_max.x - r2_min.x, r2_max.x - r1_min.x);
  const impact_float overlap_y = fmin(r1_max.y - r2_min.y, r2_max.y - r1_min.y);
  const impact_float overlap_z = fmin(r1_max.z - r2_min.z, r2_max.z - r1_min.z);

  return (vec3){overlap_x, overlap_y, overlap_z};
}

vec3 IRectsGetMTV(const IRect *RESTRICT r1, const IRect *RESTRICT r2) {
  vec3 overlap = IRectsGetOverlap(r1, r2);

  impact_float overlap_x = fabs(overlap.x);
  impact_float overlap_y = fabs(overlap.y);
  impact_float overlap_z = fabs(overlap.z);

  if (overlap_x < overlap_y && overlap_x < overlap_z) {
    return (vec3){overlap_x, 0, 0};
  } else if (overlap_y < overlap_z) {
    return (vec3){0, overlap_y, 0};
  } else {
    return (vec3){0, 0, overlap_z};
  }
}

static inline void IBodyCollisionReaction(IBody *RESTRICT b1, IBody *RESTRICT b2) {
  IRect *r1 = &b1->rect;
  IRect *r2 = &b2->rect;

  vec3 mtv = IRectsGetMTV(r1, r2);

  r1->position = v3add(r1->position, mtv);
  r2->position = v3sub(r2->position, mtv);

  impact_float normal_velocity = v3dot(v3sub(b2->velocity, b1->velocity), mtv);

  if (normal_velocity > 0.0) {
    static const impact_float restitution = 0.0f;
    impact_float impulse                  = -((impact_float)1.0f + restitution) * normal_velocity;

    vec3 normal_impulse = v3muls(mtv, impulse);
    b1->velocity        = v3add(b1->velocity, normal_impulse);
    b2->velocity        = v3sub(b2->velocity, normal_impulse);

    // clang-format off
    const impact_float IMPACT_DAMPING_FACTOR = 1.0 - 0.95;
    b1->velocity = v3muls(b1->velocity, IMPACT_DAMPING_FACTOR);
    b2->velocity = v3muls(b2->velocity, IMPACT_DAMPING_FACTOR);

    if (v3magcheap(b1->velocity) < 0.001) {
      b1->velocity = (vec3){};
      b1->sleeping = 0;
    }
    if (v3magcheap(b2->velocity) < 0.001) {
      b2->velocity = (vec3){};
      b2->sleeping = 0;
    }

    const impact_float CORRECTION_FACTOR = 0.1f;
    b1->rect.position   = v3add(b1->rect.position, v3muls(mtv, CORRECTION_FACTOR));
    b2->rect.position   = v3sub(b2->rect.position, v3muls(mtv, CORRECTION_FACTOR));
    // clang-format on
  }
}

void IWorldStep(IWorld *world, impact_float dt) {
  for (int i = 0; i < world->nbodies; i++) {
    IBody *body = &world->bodies[i];
    if (body->type == IMPACT_BODY_DYNAMIC) {
      body->velocity = v3add(body->velocity, v3muls(world->gravity, dt));
    }

    const float SLEEP_THRESHOLD = 0.001f;
    if (v3magcheap(body->velocity) <= SLEEP_THRESHOLD) {
      body->sleeping = 1;
      body->velocity = (vec3){};
    } else {
      body->sleeping      = 0;
      body->rect.position = v3add(body->rect.position, v3muls(body->velocity, dt));
    }
  }

  for (int i = 0; i < world->nbodies; i++) {
    IBody *b1 = &world->bodies[i];

    for (int j = i + 1; j < world->nbodies; j++) {
      IBody *b2 = &world->bodies[j];
      IRect *r1 = &b1->rect;
      IRect *r2 = &b2->rect;
      if (IRectsIntersect(r1, r2)) {
        IBodyCollisionReaction(b1, b2);
      }
    }
  }
}

void IBodyMove(IBody *body, vec3 to) {
  body->rect.position = v3add(body->rect.position, to);
}

vec3 IBodyGetPosition(const IBody *body) {
  return body->rect.position;
}

vec3 IBodyGetHalfSize(const IBody *body) {
  return body->rect.half_size;
}

void IBodySetPosition(IBody *body, vec3 position) {
  body->rect.position = position;
}

void IBodySetHalfSize(IBody *body, vec3 size) {
  body->rect.half_size = size;
}

void IBodySetLinearVelocity(IBody *body, vec3 vel) {
  body->velocity = vel;
}

vec3 IBodyGetLinearVelocity(const IBody *body) {
  return body->velocity;
}
