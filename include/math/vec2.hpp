#ifndef __VEC2_HPP__
#define __VEC2_HPP__

#include <cmath>

namespace cm {

template <typename tp>
struct vec2_base {
    tp x, y;

    vec2_base(tp xi, tp yi) : x(xi), y(yi) {}
    vec2_base(tp s) : x(s), y(s) {}
    vec2_base() : x(tp(0)), y(tp(0)) {}

    inline tp &operator [](int i) {
        return (&x)[i];
    }

    inline const tp &operator [](int i) const {
        return (&x)[i];
    }

    vec2_base<tp> operator+(const vec2_base<tp> &other) const {
        return vec2_base<tp>(x + other.x, y + other.y);
    }

    vec2_base<tp> operator-(const vec2_base<tp> &other) const {
        return vec2_base<tp>(x - other.x, y - other.y);
    }

    vec2_base<tp> operator*(const vec2_base<tp> &other) const {
        return vec2_base<tp>(x * other.x, y * other.y);
    }

    vec2_base<tp> operator*(tp scalar) const {
        return vec2_base<tp>(x * scalar, y * scalar);
    }

    vec2_base<tp> operator/(tp scalar) const {
        return vec2_base<tp>(x / scalar, y / scalar);
    }

    vec2_base<tp> &operator+=(const vec2_base<tp> &other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    vec2_base<tp> &operator-=(const vec2_base<tp> &other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    vec2_base<tp> &operator*=(tp scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    vec2_base<tp> &operator/=(tp scalar) {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    bool operator==(const vec2_base<tp>& other) const {
        return x == other.x && y == other.y;
    }
};

template <typename tp>
inline vec2_base<tp> add(const vec2_base<tp> &lhs, const vec2_base<tp> &rhs) {
    return lhs + rhs;
}

template <typename tp>
inline vec2_base<tp> sub(const vec2_base<tp> &lhs, const vec2_base<tp> &rhs) {
    return lhs - rhs;
}

template <typename tp>
inline vec2_base<tp> mul(const vec2_base<tp> &v, tp scalar) {
    return v * scalar;
}

template <typename tp>
inline vec2_base<tp> div(const vec2_base<tp> &v, tp scalar) {
    return v / scalar;
}

template <typename tp>
inline tp mag(const vec2_base<tp> &v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

template <typename tp>
inline vec2_base<tp> normalize(const vec2_base<tp> &v) {
    tp magnitude = mag(v);
    if (magnitude == 0) return vec2_base<tp>(0);
    return div(v, magnitude);
}

template <typename tp>
inline tp dot(const vec2_base<tp> &v1, const vec2_base<tp> &v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

typedef vec2_base<float   > vec2;
typedef vec2_base<double  > dvec2;
typedef vec2_base<int     > ivec2;
typedef vec2_base<unsigned> uvec2;

} // namespace cm

#endif//__VEC2_HPP__