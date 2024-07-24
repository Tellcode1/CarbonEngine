#ifndef __VECTOR2_HPP__
#define __VECTOR2_HPP__

#include "defines.h"
#include <math.h>

struct vec2
{
    public:
    f32 x, y;

    CARBON_FORCE_INLINE vec2() : x(0.0f), y(0.0f) {}
    CARBON_FORCE_INLINE vec2(f32 scalar) : x(scalar), y(scalar) {}
    CARBON_FORCE_INLINE vec2(f32 _x, f32 _y) : x(_x), y(_y) {}

    CARBON_FORCE_INLINE f32 magnitude() const {
        return x * x + y * y;
    }

    CARBON_FORCE_INLINE vec2 normalized() const {
        const f32 mag = magnitude();
        return this->divide(mag);
    }

    CARBON_FORCE_INLINE vec2 divide(f32 scalar) const {
        return vec2(
            x / scalar,
            y / scalar
        );
    }

    CARBON_FORCE_INLINE vec2 divide(const vec2 &other) const {
        return vec2(
            x / other.x,
            y / other.y
        );
    }

    CARBON_FORCE_INLINE vec2 mul(f32 scalar) const {
        return vec2(
            x * scalar,
            y * scalar
        );
    }

    CARBON_FORCE_INLINE vec2 mul(const vec2 &other) const {
        return vec2(
            x * other.x,
            y * other.y
        );
    }

    CARBON_FORCE_INLINE vec2 add(f32 scalar) const {
        return vec2(
            x + scalar,
            y + scalar
        );
    }

    CARBON_FORCE_INLINE vec2 add(const vec2 &other) const {
        return vec2(
            x + other.x,
            y + other.y
        );
    }
};

#endif//__VECTOR2_HPP__