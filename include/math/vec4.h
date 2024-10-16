#ifndef __CMATH_VEC4_H
#define __CMATH_VEC4_H

#ifdef __cplusplus
    extern "C" {
#endif

#include <math.h>

typedef unsigned char v4bool_t;
typedef struct vec4 {
    float x,y,z,w;
} vec4;

#define v4ptr(expr) (&(vec4){expr})

inline vec4 v4add(const vec4 v1, const vec4 v2) {
    return (vec4){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w};
}

inline vec4 v4sub(const vec4 v1, const vec4 v2) {
    return (vec4){v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w};
}

inline vec4 v4mulv(const vec4 v1, const vec4 v2) {
    return (vec4){v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w};
}

inline vec4 v4muls(const vec4 v1, const float s) {
    return (vec4){v1.x * s, v1.y * s, v1.z * s, v1.w * s};
}

inline vec4 v4divs(const vec4 v1, const float s) {
    return (vec4){v1.x / s, v1.y / s, v1.z / s, v1.w / s};
}

inline float v4mag(const vec4 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

inline vec4 v4normalize(const vec4 v) {
    float magnitude = v4mag(v);
    if (magnitude == 0) return (vec4){};
    return v4divs(v, magnitude);
}

inline float v4dot(const vec4 v1, const vec4 v2) {
    return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w);
}

inline v4bool_t v4areeq(const vec4 v1, const vec4 v2) {
    return (v4bool_t)( v1.x == v2.x && v1.y == v2.y && v1.z == v2.z && v1.w == v2.w  );
}

#ifdef __cplusplus
    }
#endif

#endif // __CMATH_VEC4_H