#ifndef __CMATH_VEC2_H
#define __CMATH_VEC2_H

#ifdef __cplusplus
    extern "C" {
#endif

#include <math.h>

typedef unsigned char v2bool_t;
typedef struct vec2 {
    float x,y;
} vec2;

#define v2ptr(expr) (&(vec2){expr})

static inline vec2 v2add(const vec2 v1, const vec2 v2) {
    return (vec2){v1.x + v2.x, v1.y + v2.y};
}

static inline vec2 v2sub(const vec2 v1, const vec2 v2) {
    return (vec2){v1.x - v2.x, v1.y - v2.y};
}

static inline vec2 v2mulv(const vec2 v1, const vec2 v2) {
    return (vec2){v1.x * v2.x, v1.y * v2.y};
}

static inline vec2 v2muls(const vec2 v1, const float s) {
    return (vec2){v1.x * s, v1.y * s};
}

static inline vec2 v2divs(const vec2 v1, const float s) {
    return (vec2){v1.x / s, v1.y / s};
}

static inline float v2mag(const vec2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

static inline vec2 v2normalize(const vec2 v) {
    float magnitude = v2mag(v);
    if (magnitude == 0) return (vec2){};
    return v2divs(v, magnitude);
}

static inline float v2dot(const vec2 v1, const vec2 v2) {
    return (v1.x * v2.x + v1.y * v2.y);
}

static inline v2bool_t v2areeq(const vec2 v1, const vec2 v2) {
    return (v2bool_t)( v1.x == v2.x && v1.y == v2.y  );
}

#ifdef __cplusplus
    }
#endif

#endif // __CMATH_VEC2_H