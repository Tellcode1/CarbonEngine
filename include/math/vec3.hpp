#ifndef __VEC3_HPP__
#define __VEC3_HPP__

#include <cmath>

namespace cm {

template <typename tp>
struct vec3_base {
    tp x, y, z;

    vec3_base(tp xi, tp yi, tp zi) : x(xi), y(yi), z(zi) {}
    vec3_base(tp s) : x(s), y(s), z(s) {}
    vec3_base() : x(tp()), y(tp()), z(tp()) {}

    inline tp &operator [](int i) {
        return (&x)[i];
    }

    inline const tp &operator [](int i) const {
        return (&x)[i];
    }

    vec3_base<tp> operator+(const vec3_base<tp>& other) const {
        return vec3_base<tp>(x + other.x, y + other.y, z + other.z);
    }

    vec3_base<tp> operator-(const vec3_base<tp>& other) const {
        return vec3_base<tp>(x - other.x, y - other.y, z - other.z);
    }

    vec3_base<tp> operator*(const vec3_base<tp>& other) const {
        return vec3_base<tp>(x * other.x, y * other.y, z * other.z);
    }

    vec3_base<tp> operator*(tp scalar) const {
        return vec3_base<tp>(x * scalar, y * scalar, z * scalar);
    }

    vec3_base<tp> operator/(tp scalar) const {
        return vec3_base<tp>(x / scalar, y / scalar, z / scalar);
    }

    vec3_base<tp>& operator+=(const vec3_base<tp>& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    vec3_base<tp>& operator-=(const vec3_base<tp>& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    vec3_base<tp>& operator*=(tp scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    vec3_base<tp>& operator/=(tp scalar) {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

    bool operator==(const vec3_base<tp>& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

template <typename tp>
inline vec3_base<tp> add(const vec3_base<tp>& lhs, const vec3_base<tp>& rhs) {
    return lhs + rhs;
}

template <typename tp>
inline vec3_base<tp> sub(const vec3_base<tp>& lhs, const vec3_base<tp>& rhs) {
    return lhs - rhs;
}

template <typename tp>
inline vec3_base<tp> mul(const vec3_base<tp>& v, tp scalar) {
    return v * scalar;
}

template <typename tp>
inline vec3_base<tp> div(const vec3_base<tp>& v, tp scalar) {
    return v / scalar;
}

template <typename tp>
inline tp mag(const vec3_base<tp>& v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

template <typename tp>
inline vec3_base<tp> normalize(const vec3_base<tp>& v) {
    tp magnitude = mag(v);
    if (magnitude == 0) return vec3_base<tp>(0);
    return div(v, magnitude);
}

template <typename tp>
inline tp dot(const vec3_base<tp>& v1, const vec3_base<tp>& v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

template <typename tp>
inline vec3_base<tp> cross(const vec3_base<tp>& v1, const vec3_base<tp>& v2) {
    return vec3_base<tp>(
        v1.y * v2.z - v2.y * v1.z,
        v1.z * v2.x - v2.z * v1.x,
        v1.x * v2.y - v2.x * v1.y
    );
}

typedef vec3_base<float   >  vec3;
typedef vec3_base<double  > dvec3;
typedef vec3_base<int     > ivec3;
typedef vec3_base<unsigned> uvec3;

} // namespace cm

#endif // __VEC3_HPP__