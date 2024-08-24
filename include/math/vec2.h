#ifndef __VEC2_H__
#define __VEC2_H__

#include <math.h>

typedef struct vec2 {
    float x, y;
    vec2(float xi, float yi) : x(xi), y(yi) {}
    vec2(float s) : x(s), y(s) {}
    vec2() : x(0.0f), y(0.0f) {}
} vec2;

typedef struct ivec2 {
    int x, y;
    ivec2(int xi, int yi) : x(xi), y(yi) {}
    ivec2(int s) : x(s), y(s) {}
    ivec2() : x(0), y(0) {}
} ivec2;

typedef struct uvec2 {
    unsigned int x, y;
    uvec2(unsigned int xi, unsigned int yi) : x(xi), y(yi) {}
    uvec2(unsigned int s) : x(s), y(s) {}
    uvec2() : x(0), y(0) {}
} uvec2;

typedef struct dvec2 {
    double x, y;
    dvec2(double xi, double yi) : x(xi), y(yi) {}
    dvec2(double s) : x(s), y(s) {}
    dvec2() : x(0.0), y(0.0) {}
} dvec2;

#define v2_decl_funcs(tp, tp_name) \
inline tp tp_name##add(const tp &lhs, const tp &rhs) {\
    return tp( lhs.x + rhs.x, lhs.y + rhs.y );\
}\
\
inline tp tp_name##sub(const tp &lhs, const tp &rhs) {\
    return tp( lhs.x - rhs.x, lhs.y - rhs.y );\
}\
\
inline tp tp_name##mul(const tp &v, float scalar) {\
    return tp( v.x * scalar, v.y * scalar );\
}\
\
inline tp tp_name##div(const tp &v, float scalar) {\
    return tp( v.x / scalar, v.y / scalar );\
}\
\
inline float v2mag(const tp &v) {\
    return sqrtf( v.x * v.x + v.y * v.y );\
}\
\
inline tp v2normalize(const tp &v) {\
    const float mag = v2mag(v);\
    if (mag == 0.0f) return tp(0.0f);\
    return tp_name##div(v, mag);\
}\

v2_decl_funcs(vec2, v2);
v2_decl_funcs(dvec2, d2);
v2_decl_funcs(ivec2, i2);
v2_decl_funcs(uvec2, u2);

#endif//__VEC2_H__