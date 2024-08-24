#ifndef __VEC4_H__
#define __VEC4_H__

#include <math.h>

typedef struct vec4 {
    float x, y, z, w;
    vec4(float xi, float yi, float zi, float wi) : x(xi), y(yi), z(zi), w(wi) {}
    vec4(float s) : x(s), y(s), z(s), w(s) {}
    vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
} vec4;

typedef struct ivec4 {
    int x, y, z, w;
    ivec4(int xi, int yi, int zi, float wi) : x(xi), y(yi), z(zi), w(wi) {}
    ivec4(int s) : x(s), y(s), z(s), w(s) {}
    ivec4() : x(0), y(0), z(0), w(0) {}
} ivec4;

typedef struct uvec4 {
    unsigned x, y, z, w;
    uvec4(unsigned xi, unsigned yi, unsigned zi, float wi) : x(xi), y(yi), z(zi), w(wi) {}
    uvec4(unsigned s) : x(s), y(s), z(s), w(s) {}
    uvec4() : x(0), y(0), z(0), w(0) {}
} uvec4;

typedef struct dvec4 {
    double x, y, z, w;
    dvec4(double xi, double yi, double zi, float wi) : x(xi), y(yi), z(zi), w(wi) {}
    dvec4(double s) : x(s), y(s), z(s), w(s) {}
    dvec4() : x(0.0), y(0.0), z(0.0), w(0.0) {}
} dvec4;

#define v4_decl_funcs(tp, tp_name)\
inline tp tp_name##add(const tp &lhs, const tp &rhs) {\
    return tp(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);\
}\
\
inline tp tp_name##sub(const tp &lhs, const tp &rhs) {\
    return tp(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w);\
}\
\
inline tp tp_name##mul(const tp &v, float scalar) {\
    return tp(v.x * scalar, v.y * scalar, v.z * scalar, v.w * scalar);\
}\
\
inline tp tp_name##div(const tp &v, float scalar) {\
    return tp(v.x / scalar, v.y / scalar, v.z / scalar, v.w / scalar);\
}\
\
inline float tp_name##mag(const tp &v) {\
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);\
}\
\
inline tp tp_name##normalize(const tp &v) {\
    const float mag = tp_name##mag(v);\
    if (mag == 0.0f) return tp(0.0f);\
    return tp_name##div(v, mag);\
}

v4_decl_funcs(vec4, v4);
v4_decl_funcs(dvec4, d4);
v4_decl_funcs(ivec4, i4);
v4_decl_funcs(uvec4, u4);

#endif // __VEC4_H__