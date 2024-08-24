#ifndef __VEC3_H__
#define __VEC3_H__

#include <math.h>

typedef struct vec3 {
    float x, y, z;
    vec3(float xi, float yi, float zi) : x(xi), y(yi), z(zi) {}
    vec3(float s) : x(s), y(s), z(s) {}
    vec3() : x(0.0f), y(0.0f), z(0.0f) {}
} vec3;

typedef struct ivec3 {
    int x, y, z;
    ivec3(int xi, int yi, int zi) : x(xi), y(yi), z(zi) {}
    ivec3(int s) : x(s), y(s), z(s) {}
    ivec3() : x(0), y(0), z(0) {}
} ivec3;

typedef struct uvec3 {
    unsigned x, y, z;
    uvec3(unsigned xi, unsigned yi, unsigned zi) : x(xi), y(yi), z(zi) {}
    uvec3(unsigned s) : x(s), y(s), z(s) {}
    uvec3() : x(0), y(0), z(0) {}
} uvec3;

typedef struct dvec3 {
    double x, y, z;
    dvec3(double xi, double yi, double zi) : x(xi), y(yi), z(zi) {}
    dvec3(double s) : x(s), y(s), z(s) {}
    dvec3() : x(0.0), y(0.0), z(0.0) {}
} dvec3;

#define v3_decl_funcs(tp, tp_name)\
inline tp tp_name##add(const tp &lhs, const tp &rhs) {\
    return tp(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);\
}\
\
inline tp tp_name##sub(const tp &lhs, const tp &rhs) {\
    return tp(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);\
}\
\
inline tp tp_name##mul(const tp &v, float scalar) {\
    return tp(v.x * scalar, v.y * scalar, v.z * scalar);\
}\
\
inline tp tp_name##div(const tp &v, float scalar) {\
    return tp(v.x / scalar, v.y / scalar, v.z / scalar);\
}\
\
inline float tp_name##mag(const tp &v) {\
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);\
}\
\
inline tp tp_name##normalize(const tp &v) {\
    const float mag = tp_name##mag(v);\
    if (mag == 0.0f) return tp(0.0f);\
    return tp_name##div(v, mag);\
}\
\
inline tp tp_name##cross(const tp &v1, const tp &v2) {\
    return tp(\
        v1.y * v2.z - v2.y * v1.z,\
        v1.z * v2.x - v2.z * v1.x,\
        v1.x * v2.y - v2.x * v1.y \
    );\
}

v3_decl_funcs(vec3, v3);
v3_decl_funcs(dvec3, d3);
v3_decl_funcs(ivec3, i3);
v3_decl_funcs(uvec3, u3);

#endif // __VEC3_H__