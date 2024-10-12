#ifndef __C_CAMERA_HPP__
#define __C_CAMERA_HPP__

#include "stdafx.h"
#include "vkstdafx.h"
#include "math/math.h"
#include "math/vec3.h"
#include "math/mat.h"
#include "cgfx.h"

struct ccamera {
    // TODO: move to uniform buffer with data like current app time, delta time, etc.
    // To be honest i dont know what delta time is supposed to be
    // doing on the GPU except for particle simulations but you ought to
    // use something like push constants for that. Not my fault ;D
    mat4 projection;
    mat4 view;

    vec3 position = (vec3){0.0f, 0.0f, 3.0f};
    vec3 front = (vec3){0.0f, 0.0f,  1.0f};
    vec3 up = (vec3){0.0f, 1.0f, 0.0f};
    vec3 right;
    f32 yaw = cmdeg2rad(-90.0);
    f32 pitch = 0.0;

    const f32 fov;
    const f32 near_clip = 0.1f;
    const f32 far_clip = 1000.0f;

    ccamera(f32 FOV = cmdeg2rad(90.0))
        : fov(FOV) {}

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

    inline void rotate(f32 yaw_, f32 pitch_) {
        yaw += yaw_;
        pitch -= pitch_;

        yaw = fmodf(yaw, 360.0f);

        const f32 bound = 89.9f;
        pitch = cmclamp( pitch, -bound, bound );
    }

    inline void move(const vec3 &amt) {
        position = v3add(position, v3muls(right, amt.x));
        position = v3add(position, v3muls(up   , amt.y));
        position = v3add(position, v3muls(front, amt.z));
    }

    inline void set_position(const vec3 &pos) {
        position = pos;
    }

// private:
    void update(struct crenderer_t *rd) {
        vec3 new_front;
        new_front.x = cosf(cmdeg2rad(yaw)) * cosf(cmdeg2rad(pitch));
        new_front.y = sinf(cmdeg2rad(pitch));
        new_front.z = sinf(cmdeg2rad(yaw)) * cosf(cmdeg2rad(pitch));

        const vec3 world_up = (vec3){0.0f, 1.0f, 0.0f};

        this->front = v3normalize(new_front);
        this->right = v3normalize(v3cross(front, world_up));
        this->up = v3normalize(v3cross(right, front));

        view = m4lookat(position, v3add(position, front), up);

        const cg_extent2d RenderExtent = crd_get_render_extent(rd);
        const f32 aspect = (f32)RenderExtent.width / (f32)RenderExtent.height;
        projection = m4perspective(fov, aspect, near_clip, far_clip);
    }
};

#endif//__C_CAMERA_HPP__