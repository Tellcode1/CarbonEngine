#ifndef __VEC4_H__
#define __VEC4_H__

#include <cmath>

namespace cm {

template <typename tp>
struct vec4_base {
    tp x, y, z, w;
    vec4_base(tp xi, tp yi, tp zi, tp wi) : x(xi), y(yi), z(zi), w(wi) {}
    vec4_base(tp s) : x(s), y(s), z(s), w(s) {}
    vec4_base() : x(tp()), y(tp()), z(tp()), w(tp()) {}

    inline tp &operator [](int i) {
        return (&x)[i];
    }

    inline const tp &operator [](int i) const {
        return (&x)[i];
    }

    vec4_base<tp> operator+(const vec4_base<tp> &other) const {
        return vec4_base<tp>(x + other.x, y + other.y, z + other.z, w + other.w);
    }

    vec4_base<tp> operator-(const vec4_base<tp> &other) const {
        return vec4_base<tp>(x - other.x, y - other.y, z - other.z, w - other.w);
    }

    vec4_base<tp> operator*(const vec4_base<tp> &other) const {
        return vec4_base<tp>(x * other.x, y * other.y, z * other.z, w * other.w);
    }

    vec4_base<tp> operator*(tp scalar) const {
        return vec4_base<tp>(x * scalar, y * scalar, z * scalar, w * scalar);
    }

    vec4_base<tp> operator/(tp scalar) const {
        return vec4_base<tp>(x / scalar, y / scalar, z / scalar, w / scalar);
    }

    vec4_base<tp> &operator+=(const vec4_base<tp> &other) {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    vec4_base<tp> &operator-=(const vec4_base<tp> &other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }

    vec4_base<tp> &operator*=(tp scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        w *= scalar;
        return *this;
    }

    vec4_base<tp> &operator/=(tp scalar) {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        w /= scalar;
        return *this;
    }

    bool operator==(const vec4_base<tp>& other) const {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }
};

template <typename tp>
inline vec4_base<tp> add(const vec4_base<tp> &lhs, const vec4_base<tp> &rhs) {
    return vec4_base<tp>(lhs + rhs);
}

template <typename tp>
inline vec4_base<tp> sub(const vec4_base<tp> &lhs, const vec4_base<tp> &rhs) {
    return vec4_base<tp>(lhs - rhs);
}

template <typename tp>
inline vec4_base<tp> mul(const vec4_base<tp> &v, float scalar) {
    return vec4_base<tp>(v * scalar);
}

template <typename tp>
inline vec4_base<tp> div(const vec4_base<tp> &v, float scalar) {
    return vec4_base<tp>(v / scalar);
}

template <typename tp>
inline float mag(const vec4_base<tp> &v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

template <typename tp>
inline vec4_base<tp> normalize(const vec4_base<tp> &v) {
    const float mag = mag(v);
    if (mag == 0) return vec4_base<tp>(0);
    return div(v, mag);
}

template <typename tp>
inline float dot(const vec4_base<tp> &v1, const vec4_base<tp> &v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}

typedef vec4_base<float   >  vec4;
typedef vec4_base<double  > dvec4;
typedef vec4_base<int     > ivec4;
typedef vec4_base<unsigned> uvec4;

} // namespace cm

#endif // __VEC4_H__