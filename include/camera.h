#ifndef __C_CAMERA_H__
#define __C_CAMERA_H__

#include "stdafx.h"
#include "vkstdafx.h"
#include "math/math.h"
#include "math/vec3.h"
#include "math/mat.h"
#include "lunaGPUObjects.h"
#include "lunaDescriptors.h"
#include "lunaGFX.h"

// If you draw a quad with this width, it'll cover the whole screen
// oh, and this should technically be doubled when you're rendering quads as they generally take half size
// that's just to say this is the half width along each direction. Double it and you'll get the full width needed.
#define CAMERA_ORTHO_W 10.0f
#define CAMERA_ORTHO_H 10.0f

#define CAMERA_FAKE_BUFFER_COUNT 3

#define align_up(sz, align) ((sz + align - 1) & ~(align - 1))

typedef struct camera_uniform_buffer {
    mat4 perspective;
    mat4 ortho;
    mat4 view;
} camera_uniform_buffer;

typedef struct ccamera {
    // TODO: move to uniform buffer with data like current app time, delta time, etc.
    // To be honest i dont know what delta time is supposed to be
    // doing on the GPU except for particle simulations but you ought to
    // use something like push constants for that. Not my fault ;D
    mat4 perspective;
    mat4 ortho;
    mat4 view;

    // This reduces "choppiness" created by moving the camera if the camera has moved after transferring to the uniform buffer
    // position is the position occupied by the camera when it was sent to the uniform buffer
    vec3 position;
    vec3 actual_pos;
    vec3 front;
    vec3 up;
    vec3 right;
    f32 yaw;
    f32 pitch;

    f32 fov;
    f32 near_clip;
    f32 far_clip;

    luna_GPU_Buffer ub;
    luna_GPU_Memory mem;
    luna_DescriptorSet *sets;
    camera_uniform_buffer *mem_mapped;
} ccamera;

static inline ccamera ccamera_init() {
    ccamera camera = {
        .perspective = {},
        .ortho = m4ortho(-CAMERA_ORTHO_W, CAMERA_ORTHO_W, -CAMERA_ORTHO_H, CAMERA_ORTHO_H, 0.1f, 100.0f),
        .position = (vec3){0.0f, 0.0f, 10.0f},
        .actual_pos = (vec3){0.0f, 0.0f, 10.0f},
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

    VkPhysicalDeviceProperties phys_device_properties;
    vkGetPhysicalDeviceProperties(physDevice, &phys_device_properties);

    int ub_align = phys_device_properties.limits.minUniformBufferOffsetAlignment;

    // the size of a single uniform buffer.
    int ub_size = align_up(sizeof(camera_uniform_buffer), ub_align);

    luna_GPU_CreateBuffer(ub_size * CAMERA_FAKE_BUFFER_COUNT, ub_align, 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &camera.ub);
    luna_GPU_AllocateMemory(ub_size * CAMERA_FAKE_BUFFER_COUNT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &camera.mem);
    luna_GPU_BindBufferToMemory(&camera.mem, 0, &camera.ub);

    VkDescriptorSetLayoutBinding bindings[] = {
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT, NULL }
    };
    luna_AllocateDescriptorSet(&g_pool, bindings, 1, &camera.sets);

    VkDescriptorBufferInfo bufferinfo = {
        .buffer = camera.ub.buffers[0],
        .offset = 0,
        .range = ub_size
    };
    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = camera.sets->set,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        .pBufferInfo = &bufferinfo,
    };
    luna_DescriptorSetSubmitWrite(camera.sets, &write);
    vkMapMemory(device, camera.mem.memory, 0, VK_WHOLE_SIZE, 0, (void **)&camera.mem_mapped);
    return camera;
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
    cam->actual_pos = v3add(cam->actual_pos, v3muls(cam->right, amt.x));
    cam->actual_pos = v3add(cam->actual_pos, v3muls(cam->up   , amt.y));
    cam->actual_pos = v3add(cam->actual_pos, v3muls(cam->front, amt.z));
}

static inline void cam_set_position(ccamera *cam, const vec3 pos) {
    cam->actual_pos = pos;
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

    cam->view = m4lookat(cam->actual_pos, v3add(cam->actual_pos, cam->front), cam->up);

    cam->position = cam->actual_pos;

    const lunaExtent2D RenderExtent = luna_Renderer_GetRenderExtent(rd);
    const f32 aspect = (f32)RenderExtent.width / (f32)RenderExtent.height;
    cam->perspective = m4perspective(cam->fov, aspect, cam->near_clip, cam->far_clip);

    camera_uniform_buffer ub = {};
    ub.perspective = cam->perspective;
    ub.ortho = cam->ortho;
    ub.view = cam->view;
    cam->mem_mapped[ luna_Renderer_GetFrame(rd) ] = ub;
}

#endif//__C_CAMERA_H__