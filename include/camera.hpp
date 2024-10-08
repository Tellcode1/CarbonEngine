#ifndef __C_CAMERA_HPP__
#define __C_CAMERA_HPP__

#include "stdafx.h"
#include "vkstdafx.h"
#include "math/math.h"
#include "math/vec3.hpp"
#include "math/mat.hpp"
#include "cgfx.h"

struct ccamera {
    // TODO: move to uniform buffer with data like current app time, delta time, etc.
    // To be honest i dont know what delta time is supposed to be
    // doing on the GPU except for particle simulations but you ought to
    // use something like push constants for that. Not my fault ;D
    cm::mat4 projection;
    cm::mat4 view;

    cm::vec3 position = cm::vec3(0.0f, 0.0f, 3.0f);
    cm::vec3 front = cm::vec3(0.0f, 0.0f,  1.0f);
    cm::vec3 up = cm::vec3(0.0f, 1.0f, 0.0f);
    cm::vec3 right;
    f32 yaw = cmdeg2rad(-90.0);
    f32 pitch = 0.0;

    const f32 fov;
    const f32 near_clip = 0.1f;
    const f32 far_clip = 1000.0f;

    ccamera(f32 FOV = cmdeg2rad(90.0))
        : fov(FOV) {}

    const cm::mat4 get_projection() const {
        return projection;
    }

    const cm::mat4 get_view() const {
        return view;
    }

    inline const cm::vec3 &get_up() const {
        return up;
    }

    inline const cm::vec3 &get_front() const {
        return front;
    }

    inline void rotate(f32 yaw_, f32 pitch_) {
        yaw += yaw_;
        pitch -= pitch_;

        yaw = fmodf(yaw, 360.0f);

        const f32 bound = 89.9f;
        pitch = cmclamp( pitch, -bound, bound );
    }

    inline void move(const cm::vec3 &amt) {
        position += cm::mul(right, amt.x);
        position += cm::mul(up   , amt.y);
        position += cm::mul(front, amt.z);
    }

    inline void set_position(const cm::vec3 &pos) {
        position = pos;
    }

// private:
    void update(struct crenderer_t *rd) {
        cm::vec3 new_front;
        new_front.x = cosf(cmdeg2rad(yaw)) * cosf(cmdeg2rad(pitch));
        new_front.y = sinf(cmdeg2rad(pitch));
        new_front.z = sinf(cmdeg2rad(yaw)) * cosf(cmdeg2rad(pitch));

        const cm::vec3 world_up = cm::vec3(0.0f, 1.0f, 0.0f);

        this->front = cm::normalize(new_front);
        this->right = cm::normalize(cm::cross(this->front, world_up));
        this->up = cm::normalize(cm::cross(this->right, this->front));

        view = cm::lookat(position, cm::add(position, front), up);

        const struct cengine_extent2d RenderExtent = crenderer_get_render_extent(rd);
        const f32 aspect = (f32)RenderExtent.width / (f32)RenderExtent.height;
        projection = cm::perspective(fov, aspect, near_clip, far_clip);
    }
};

#endif//__C_CAMERA_HPP__