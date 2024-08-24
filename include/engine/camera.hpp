#ifndef __C_CAMERA_HPP__
#define __C_CAMERA_HPP__

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "../stdafx.hpp"
#include "../math/math.h"
#include "../math/vec3.h"

#define test_d3toglm(v) glm::vec3(v.x, v.y, v.z)

struct ccamera {
    // TODO: move to uniform buffer with data like current app time, delta time, etc.
    mat4 projection;
    mat4 view;

    vec3 position = vec3(0.0f, 0.0f, 3.0f);
    vec3 front = vec3(0.0f, 0.0f, -1.0f);
    vec3 up = vec3(0.0f, 1.0f, 0.0f);
    vec3 right;
    float yaw = mdeg2rad(-90.0);
    float pitch = 0.0;

    const float fov;
    const float near_clip = 0.1f;
    const float far_clip = 1000.0f;

    ccamera(float FOV = mdeg2rad(45.0))
        : fov(FOV) {
        update_vectors(true);
    }

    const mat4 get_projection() const {
        return projection;
    }

    const mat4 get_view() const {
        return view;
    }

    inline const vec3 &get_up() const {
        return up;
    }

    inline const vec3 &get_front() const {
        return front;
    }

    inline void rotate(float yaw_, float pitch_) {
        yaw += mdeg2rad(yaw_);
        pitch += mdeg2rad(pitch_);

        // If you're wondering why pitch isn't subtracted, vulkan uses reverse y axis.
        // I'm too used to vulkan now :<

        yaw = fmodf(yaw, mdeg2rad(360.0f));

        const float bound = mdeg2rad(89.9f);
        pitch = mclamp( pitch, -bound, bound );
    }

    inline void move(const vec3 &amt) {
        position = v3add(position, v3mul(right, amt.x));
        position = v3add(position, v3mul(up   , amt.y));
        position = v3add(position, v3mul(front, amt.z));
    }

    inline void set_position(const vec3 &pos) {
        position = pos;
    }

// private:
    void update_vectors(bool first) {
        vec3 new_ront;
        new_ront.x = cosf(yaw) * cosf(pitch);
        new_ront.y = sinf(pitch);
        new_ront.z = sinf(yaw) * cosf(pitch);

        const vec3 world_up = vec3(0.0f, 1.0f, 0.0f);

        this->front = v3normalize(new_ront);
        this->right = v3normalize(v3cross(this->front, world_up));
        this->up = v3normalize(v3cross(this->right, this->front));

        view = glm::lookAt(test_d3toglm(position), test_d3toglm(position) + test_d3toglm(front), test_d3toglm((up)));

        if (!first) {
            const float aspect = (f32)vctx::RenderExtent.width / (f32)vctx::RenderExtent.height;
            projection = glm::perspective(fov, aspect, near_clip, far_clip);
        }
    }
};

#endif//__C_CAMERA_HPP__