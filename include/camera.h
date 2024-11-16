#ifndef __C_CAMERA_H__
#define __C_CAMERA_H__

#include "stdafx.h"
#include "vkstdafx.h"
#include "math/math.h"
#include "math/vec3.h"
#include "math/mat.h"
#include "lunaGFX.h"

// If you draw a quad with this width, it'll cover the whole screen
#define CAMERA_ORTHO_W 10.0f
#define CAMERA_ORTHO_H 10.0f

typedef struct ccamera {
    // TODO: move to uniform buffer with data like current app time, delta time, etc.
    // To be honest i dont know what delta time is supposed to be
    // doing on the GPU except for particle simulations but you ought to
    // use something like push constants for that. Not my fault ;D
    mat4 perspective;
    mat4 ortho;
    mat4 view;

    vec3 position;
    vec3 front;
    vec3 up;
    vec3 right;
    f32 yaw;
    f32 pitch;

    f32 fov;
    f32 near_clip;
    f32 far_clip;
} ccamera;

static inline ccamera ccamera_init() {
    return (ccamera) {
        .perspective = {},
        .ortho = m4ortho(-CAMERA_ORTHO_W, CAMERA_ORTHO_W, -CAMERA_ORTHO_H, CAMERA_ORTHO_H, 0.1f, 100.0f),
        .view = {},
        .position = (vec3){0.0f, 0.0f, 10.0f},
        .front = (vec3){0.0f, 0.0f,  1.0f},
        .up = (vec3){0.0f, 1.0f, 0.0f},
        .right = (vec3){1.0f, 0.0f, 0.0f},
        // These angles should not be in radians because they're converted at update() time.
        .yaw = -90.0,
        .pitch = 0.0,
        .fov = 90.0f,
        .near_clip = 0.1f,
        .far_clip = 1000.0f,
    };
}

static inline mat4 cam_get_projection(ccamera *cam) {
    return cam->perspective;
}

static inline mat4 cam_get_view(ccamera *cam) {
    return cam->view;
}

static inline vec3 cam_get_up(ccamera *cam) {
    return cam->up;
}

static inline vec3 cam_get_front(ccamera *cam) {
    return cam->front;
}

static inline void cam_rotate(ccamera *cam, f32 yaw_, f32 pitch_) {
    cam->yaw += yaw_;
    cam->pitch -= pitch_;

    cam->yaw = fmodf(cam->yaw, 360.0f);

    const f32 bound = 89.9f;
    cam->pitch = cmclamp( cam->pitch, -bound, bound );
}

static inline void cam_move(ccamera *cam, const vec3 amt) {
    cam->position = v3add(cam->position, v3muls(cam->right, amt.x));
    cam->position = v3add(cam->position, v3muls(cam->up   , amt.y));
    cam->position = v3add(cam->position, v3muls(cam->front, amt.z));
}

static inline void cam_set_position(ccamera *cam, const vec3 pos) {
    cam->position = pos;
}

static inline void cam_update(ccamera *cam, struct luna_Renderer_t *rd) {
    const float yaw_rads = cmdeg2rad(cam->yaw), pitch_rads = cmdeg2rad(cam->pitch);
    const float cospitch = cosf(pitch_rads);
    vec3 new_front;
    new_front.x = cosf(yaw_rads) * cospitch;
    new_front.y = sinf(pitch_rads);
    new_front.z = sinf(yaw_rads) * cospitch;

    const vec3 world_up = (vec3){0.0f, 1.0f, 0.0f};

    cam->front = v3normalize(new_front);
    cam->right = v3normalize(v3cross(cam->front, world_up));
    cam->up = v3normalize(v3cross(cam->right, cam->front));

    cam->view = m4lookat(cam->position, v3add(cam->position, cam->front), cam->up);

    const lunaExtent2D RenderExtent = luna_Renderer_GetRenderExtent(rd);
    const f32 aspect = (f32)RenderExtent.width / (f32)RenderExtent.height;
    cam->perspective = m4perspective(cam->fov, aspect, cam->near_clip, cam->far_clip);
}

#endif//__C_CAMERA_H__