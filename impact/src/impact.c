#include "../include/impact.h"
#include "../../common/math/math.h"
#include "../../common/printf.h"
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
                                          .restitution    = pInfo->restitution,
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

  const vec3 overlap = (vec3){fmin(r1_max.x - r2_min.x, r2_max.x - r1_min.x), fmin(r1_max.y - r2_min.y, r2_max.y - r1_min.y),
                              fmin(r1_max.z - r2_min.z, r2_max.z - r1_min.z)};

  return (vec3){cmmax(0, overlap.x), cmmax(0, overlap.y), cmmax(0, overlap.z)};
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

  impact_float n_velo = v3dot(v3sub(b2->velocity, b1->velocity), mtv);

  if (n_velo < 0.0) {
    impact_float total_restitution = (b1->restitution + b2->restitution) / (impact_float)2;
    impact_float impulse           = -((impact_float)1 + total_restitution) * n_velo;

    vec3 normal_impulse = v3normalize(v3muls(mtv, impulse));
    b1->velocity        = v3add(b1->velocity, normal_impulse);
    b2->velocity        = v3sub(b2->velocity, normal_impulse);

    if (v3magcheap(b1->velocity) < 0.001) {
      b1->velocity = (vec3){};
      b1->sleeping = 1;
    }
    if (v3magcheap(b2->velocity) < 0.001) {
      b2->velocity = (vec3){};
      b2->sleeping = 1;
    }
  }
}

void IWorldStep(IWorld *world, impact_float dt) {
  dt                 = 1 / 60.0;
  const vec3 gravity = v3muls(world->gravity, dt);

  for (int i = 0; i < world->nbodies; i++) {
    IBody *b1 = &world->bodies[i];

    for (int j = 0; j < world->nbodies; j++) {
      if (i == j)
        continue;
      IBody *b2 = &world->bodies[j];
      IRect *r1 = &b1->rect;
      IRect *r2 = &b2->rect;
      if (IRectsIntersect(r1, r2)) {
        IBodyCollisionReaction(b1, b2);
      }
    }
  }
  for (int i = 0; i < world->nbodies; i++) {
    IBody *body                 = &world->bodies[i];
    const float SLEEP_THRESHOLD = 0.001f;
    if (v3magcheap(body->velocity) <= SLEEP_THRESHOLD) {
      body->sleeping = 1;
      body->velocity = (vec3){};
    } else {
      body->sleeping      = 0;
      body->rect.position = v3add(body->rect.position, v3muls(body->velocity, dt));
    }
    if (body->type == IMPACT_BODY_DYNAMIC) {
      body->velocity = v3add(body->velocity, gravity);
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
