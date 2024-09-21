#ifndef __MAT_HPP__
#define __MAT_HPP__

#include "vec4.hpp"
#include "vec3.hpp"

namespace cm {

template <typename tp>
struct mat4_base {
    vec4_base<tp> rows[4];

    mat4_base(
        vec4_base<tp> r0,
        vec4_base<tp> r1,
        vec4_base<tp> r2,
        vec4_base<tp> r3
    ) : rows{r0, r1, r2, r3} {}

    mat4_base(tp diag) 
        : rows{
            vec4_base<tp>(diag, 0, 0, 0),
            vec4_base<tp>(0, diag, 0, 0),
            vec4_base<tp>(0, 0, diag, 0),
            vec4_base<tp>(0, 0, 0, diag)
        } {}

    mat4_base(tp diag1, tp diag2, tp diag3, tp diag4) 
    : rows{
        vec4_base<tp>(diag1, 0, 0, 0),
        vec4_base<tp>(0, diag2, 0, 0),
        vec4_base<tp>(0, 0, diag3, 0),
        vec4_base<tp>(0, 0, 0, diag4)
    } {}

    mat4_base() 
        : rows{
            vec4_base<tp>(0),
            vec4_base<tp>(0),
            vec4_base<tp>(0),
            vec4_base<tp>(0)
        } {}

    vec4_base<tp>& operator[](int i) {
        return rows[i];
    }

    const vec4_base<tp>& operator[](int i) const {
        return rows[i];
    }

    mat4_base<tp> operator+(const mat4_base<tp>& other) const {
        return mat4_base<tp>(
            rows[0] + other.rows[0],
            rows[1] + other.rows[1],
            rows[2] + other.rows[2],
            rows[3] + other.rows[3]
        );
    }

    mat4_base<tp> operator-(const mat4_base<tp>& other) const {
        return mat4_base<tp>(
            rows[0] - other.rows[0],
            rows[1] - other.rows[1],
            rows[2] - other.rows[2],
            rows[3] - other.rows[3]
        );
    }

    mat4_base<tp> operator*(tp scalar) const {
        return mat4_base<tp>(
            rows[0] * scalar,
            rows[1] * scalar,
            rows[2] * scalar,
            rows[3] * scalar
        );
    }

    mat4_base<tp> operator*(const mat4_base<tp>& other) const {
        return mat4_base<tp>(
            rows[0] * other[0][0] + rows[1] * other[0][1] + rows[2] * other[0][2] + rows[3] * other[0][3],
            rows[0] * other[1][0] + rows[1] * other[1][1] + rows[2] * other[1][2] + rows[3] * other[1][3],
            rows[0] * other[2][0] + rows[1] * other[2][1] + rows[2] * other[2][2] + rows[3] * other[2][3],
            rows[0] * other[3][0] + rows[1] * other[3][1] + rows[2] * other[3][2] + rows[3] * other[3][3]
        );
    }

    vec4_base<tp> operator*(const vec4_base<tp>& vec) const {
        return vec4_base<tp>(
            rows[0].x * vec.x + rows[0].y * vec.y + rows[0].z * vec.z + rows[0].w * vec.w,
            rows[1].x * vec.x + rows[1].y * vec.y + rows[1].z * vec.z + rows[1].w * vec.w,
            rows[2].x * vec.x + rows[2].y * vec.y + rows[2].z * vec.z + rows[2].w * vec.w,
            rows[3].x * vec.x + rows[3].y * vec.y + rows[3].z * vec.z + rows[3].w * vec.w
        );
    }

    mat4_base<tp>& operator+=(const mat4_base<tp>& other) {
        rows[0] += other.rows[0];
        rows[1] += other.rows[1];
        rows[2] += other.rows[2];
        rows[3] += other.rows[3];
        return *this;
    }

    mat4_base<tp>& operator-=(const mat4_base<tp>& other) {
        rows[0] -= other.rows[0];
        rows[1] -= other.rows[1];
        rows[2] -= other.rows[2];
        rows[3] -= other.rows[3];
        return *this;
    }

    mat4_base<tp>& operator*=(tp scalar) {
        rows[0] *= scalar;
        rows[1] *= scalar;
        rows[2] *= scalar;
        rows[3] *= scalar;
        return *this;
    }
};

template <typename tp>
inline mat4_base<tp> add(const mat4_base<tp>& lhs, const mat4_base<tp>& rhs) {
    return lhs + rhs;
}

template <typename tp>
inline mat4_base<tp> sub(const mat4_base<tp>& lhs, const mat4_base<tp>& rhs) {
    return lhs - rhs;
}

template <typename tp>
inline mat4_base<tp> mul(const mat4_base<tp>& mat, tp scalar) {
    return mat * scalar;
}

template <typename tp>
inline mat4_base<tp> mul(tp scalar, const mat4_base<tp>& mat) {
    return mat * scalar;
}

template <typename tp>
inline mat4_base<tp> mul(const mat4_base<tp>& lhs, const mat4_base<tp>& rhs) {
    return lhs * rhs;
}

template <typename tp>
inline vec4_base<tp> mul(const mat4_base<tp>& mat, const vec4_base<tp>& vec) {
    return mat * vec;
}

template <typename tp>
inline mat4_base<tp> perspective(tp fovradians, tp aspect_ratio, tp z_near, tp z_far) {
    const tp halftan = tan(fovradians / static_cast<tp>(2));

    mat4_base<tp> result(tp(0));
    result[0][0] = tp(1) / (aspect_ratio * halftan);
    result[1][1] = -(tp(1) / (halftan)); // y flip
    result[2][2] = - (z_far + z_near) / (z_far - z_near);
    result[2][3] = - tp(1);
    result[3][2] = - (tp(2) * z_far * z_near) / (z_far - z_near);
    return result;
}

template <typename tp>
mat4_base<tp> lookat(vec3_base<tp> const& eye, vec3_base<tp> const& center, vec3_base<tp> const& up) {
        const vec3_base<tp> f(normalize(center - eye));
		const vec3_base<tp> s(normalize(cross(f, up)));
		const vec3_base<tp> u(cross(s, f));

		mat4_base<tp> ret(1);
		ret[0][0] = s.x;
		ret[1][0] = s.y;
		ret[2][0] = s.z;
		ret[0][1] = u.x;
		ret[1][1] = u.y;
		ret[2][1] = u.z;
		ret[0][2] = -f.x;
		ret[1][2] = -f.y;
		ret[2][2] = -f.z;
		ret[3][0] = -dot(s, eye);
		ret[3][1] = -dot(u, eye);
		ret[3][2] = dot(f, eye);
		return ret;
}

template <typename tp>
mat4_base<tp> scale(const mat4_base<tp> &mat, const vec3_base<tp> &amount) {
    mat4_base<tp> matrix = mat4_base<tp>(tp(1));
    matrix[0] = mat[0] * amount[0];
    matrix[1] = mat[1] * amount[1];
    matrix[2] = mat[2] * amount[2];
    matrix[3] = mat[3];
    return matrix;
}

template <typename tp>
mat4_base<tp> translate(const mat4_base<tp> &mat, const vec3_base<tp> &amount) {
    mat4_base<tp> matrix = mat4_base<tp>(tp(1));
    matrix[3][0] = amount.x;
    matrix[3][1] = amount.y;
    matrix[3][2] = amount.z;
    return mat * matrix;
}

template <typename tp>
mat4_base<tp> rotate(const mat4_base<tp> &mat, tp angle_radians, const vec3_base<tp> &amount) {
    tp c = cos(angle_radians);
    tp s = sin(angle_radians);

    vec3_base<tp> axis(normalize(amount));
    vec3_base<tp> temp(axis * (tp(1) - c));

    mat4_base<tp> rotation;
    rotation[0][0] = c + temp[0] * axis[0];
    rotation[0][1] = temp[0] * axis[1] + s * axis[2];
    rotation[0][2] = temp[0] * axis[2] - s * axis[1];

    rotation[1][0] = temp[1] * axis[0] - s * axis[2];
    rotation[1][1] = c + temp[1] * axis[1];
    rotation[1][2] = temp[1] * axis[2] + s * axis[0];

    rotation[2][0] = temp[2] * axis[0] + s * axis[1];
    rotation[2][1] = temp[2] * axis[1] - s * axis[0];
    rotation[2][2] = c + temp[2] * axis[2];

    mat4_base<tp> result;
    result[0] = mat[0] * rotation[0][0] + mat[1] * rotation[0][1] + mat[2] * rotation[0][2];
    result[1] = mat[0] * rotation[1][0] + mat[1] * rotation[1][1] + mat[2] * rotation[1][2];
    result[2] = mat[0] * rotation[2][0] + mat[1] * rotation[2][1] + mat[2] * rotation[2][2];
    result[3] = mat[3];
    return result;
}

template <typename tp>
mat4_base<tp> rotate(const mat4_base<tp> &mat, const vec3_base<tp> &amount) {
    mat4_base<tp> m1 = cm::rotate(mat, amount.x, cm::vec3(1.0f, 0.0f, 0.0f));
    mat4_base<tp> m2 = cm::rotate(mat, amount.y, cm::vec3(0.0f, 1.0f, 0.0f));
    mat4_base<tp> m3 = cm::rotate(mat, amount.z, cm::vec3(0.0f, 0.0f, 1.0f));
    return m3 * m2 * m1;
}

typedef mat4_base<float>    mat4;
typedef mat4_base<double>   dmat4;
typedef mat4_base<int>      imat4;
typedef mat4_base<unsigned> umat4;

} // namespace cm

#endif//__MAT_HPP__