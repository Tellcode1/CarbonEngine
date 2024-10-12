#ifndef __C_CAMERA_HPP__
#define __C_CAMERA_HPP__

#include "stdafx.h"
#include "vkstdafx.h"
#include "math/math.h"
#include "math/vec3.hpp"
#include "math/mat.hpp"
#include "cgfx.h"

static const f32 far_clip = 1000.0f;
static const f32 near_clip = 0.1f;
static const f32 fov = cmdeg2rad(90.0f);

typedef struct ccamera {
    // TODO: move to uniform buffer with data like current app time, delta time, etc.
    // To be honest i dont know what delta time is supposed to be
    // doing on the GPU except for particle simulations but you ought to
    // use something like push constants for that. Not my fault ;D
    mat4 projection;
    mat4 view;

    vec3 position;
    vec3 front;
    vec3 up;
    vec3 right;
    f32 yaw;
    f32 pitch;
} ccamera;

inline void cam_rotate(ccamera *cam, f32 yaw_, f32 pitch_) {
    cam->yaw += yaw_;
    cam->pitch -= pitch_;

    cam->yaw = fmodf(cam->yaw, 360.0f);

    const f32 bound = 89.9f;
    cam->pitch = cmclamp( cam->pitch, -bound, bound );
}

inline void cam_move(ccamera *cam, const vec3 amt) {
    cam->position = v3add(cam->position, v3mul(cam->right, amt.x));
    cam->position = v3add(cam->position, v3mul(cam->up   , amt.y));
    cam->position = v3add(cam->position, v3mul(cam->front, amt.z));
}

inline void cam_set_position(ccamera *cam, const vec3 pos) {
    cam->position = pos;
}

// private:
static inline void cam_update(ccamera *cam, struct crenderer_t *rd) {
    vec3 new_front;
    new_front.x = cosf(cmdeg2rad(cam->yaw)) * cosf(cmdeg2rad(cam->pitch));
    new_front.y = sinf(cmdeg2rad(cam->pitch));
    new_front.z = sinf(cmdeg2rad(cam->yaw)) * cosf(cmdeg2rad(cam->pitch));

    const vec3 world_up = (vec3){0.0f, 1.0f, 0.0f};

    cam->front = v3normalize(new_front);
    cam->right = v3normalize(v3cross(cam->front, world_up));
    cam->up = v3normalize(v3cross(cam->right, cam->front));

    const vec3 center = v3add(cam->position, cam->front);
    cam->view = m4lookat(&cam->position, &center, &cam->up);

    const cg_extent2d RenderExtent = crd_get_render_extent(rd);
    const f32 aspect = (f32)RenderExtent.width / (f32)RenderExtent.height;
    cam->projection = m4perspective(fov, aspect, near_clip, far_clip);
}

inline mat4 cam_get_projection(ccamera *cam) {
    return cam->projection;
}

inline mat4 cam_get_view(ccamera *cam) {
    return cam->view;
}

inline vec3 cam_get_up(ccamera *cam) {
    return cam->up;
}

inline vec3 cam_get_front(ccamera *cam) {
    return cam->front;
}

#endif//__C_CAMERA_HPP__