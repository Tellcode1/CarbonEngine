#ifndef __MATH_H__
#define __MATH_H__

#include "vec2.h"

static const double cmPI = 3.1415926535897932385;
static const double cm2PI = 6.283185307179586;
static const double cmDEG2RAD_CONSTANT = 0.017453292519943295; // 2Pi / 360.0
static const double cmRAD2DEG_CONSTANT = 57.29577951308232; // 1.0 / DEG2RAD_CONSTANT -> rad / DEG2RAD_CONSTANT

#define cmlerp(a, b, t) (a + ((b - a) * t))

#define cmmax(a, b) ((a) > (b) ? (a) : (b))
#define cmmin(a, b) ((a) < (b) ? (a) : (b))

#define cmclamp(x, min, max) (((x) < (min)) ? (min) : (((x) > (max)) ? (max) : (x)))
#define cmclamp01(x) cmclamp((x), 0, 1)

// The constants aren't used because for static const variables, they can cause minor issues like not compiling at all.
#define cmdeg2rad(x) ((x) * 0.017453292519943295)
#define cmrad2deg(x) ((x) * 57.29577951308232)

typedef struct cmrect2d {
    // The CENTER of the rect
    vec2 position;
    vec2 size;
} cmrect2d;

static inline bool cmAABB(const cmrect2d *r1, const cmrect2d *r2) {
    const vec2 r1_half_size = v2muls(r1->size, 0.5f);
    const vec2 r2_half_size = v2muls(r2->size, 0.5f);

    const vec2 r1_min = { r1->position.x - r1_half_size.x, r1->position.y - r1_half_size.y };
    const vec2 r1_max = { r1->position.x + r1_half_size.x, r1->position.y + r1_half_size.y };

    const vec2 r2_min = { r2->position.x - r2_half_size.x, r2->position.y - r2_half_size.y };
    const vec2 r2_max = { r2->position.x + r2_half_size.x, r2->position.y + r2_half_size.y };

    const bool overlaps_x = r1_max.x > r2_min.x && r1_min.x < r2_max.x;
    const bool overlaps_y = r1_max.y > r2_min.y && r1_min.y < r2_max.y;

    return overlaps_x && overlaps_y;
}

#endif//__MATH_H__