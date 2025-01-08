#ifndef __LUNA_CAMERA_H__
#define __LUNA_CAMERA_H__

#include "../math/mat.h"
#include "../math/vec2.h"
#include "../math/vec3.h"
#include "../math/vec4.h"

#include "lunaGFX.h"
#include "../GPU/buffer.h"
#include "../GPU/descriptors.h"
#include "../engine/lunaInput.h"

typedef struct lunaCamera lunaCamera;

// If you draw a quad with this width, it'll cover the whole screen
// oh, and this should technically be doubled when you're rendering quads as they generally take full size
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

typedef struct lunaCamera {
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
    float yaw;
    float pitch;

    float fov;
    float near_clip;
    float far_clip;

    luna_GPU_Buffer ub;
    luna_GPU_Memory mem;
    luna_DescriptorSet *sets;
    camera_uniform_buffer *mem_mapped;

    // luna_GPU_Texture *render_texture;
    // VkFramebuffer framebuffer;
    // VkRenderPass render_pass;
} lunaCamera;

static inline void lunaCamera_Destroy(lunaCamera *cam) {
    // luna_DescriptorSetDestroy(cam->sets);
    luna_GPU_DestroyBuffer(&cam->ub);
    luna_GPU_FreeMemory(&cam->mem);
}

static inline lunaCamera lunaCamera_Init() {
    lunaCamera camera = {
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

    luna_GPU_CreateBuffer(ub_size * CAMERA_FAKE_BUFFER_COUNT, ub_align, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &camera.ub);
    luna_GPU_AllocateMemory(ub_size * CAMERA_FAKE_BUFFER_COUNT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &camera.mem);
    luna_GPU_BindBufferToMemory(&camera.mem, 0, &camera.ub);

    VkDescriptorSetLayoutBinding bindings[] = {
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT, NULL }
    };
    luna_AllocateDescriptorSet(&g_pool, bindings, 1, &camera.sets);

    VkDescriptorBufferInfo bufferinfo = {
        .buffer = camera.ub.buffer,
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

static inline mat4 lunaCamera_GetProjection(lunaCamera *cam) {
    return cam->perspective;
}

static inline mat4 lunaCamera_GetView(lunaCamera *cam) {
    return cam->view;
}

static inline vec3 lunaCamera_GetUp(lunaCamera *cam) {
    return cam->up;
}

static inline vec3 lunaCamera_GetFront(lunaCamera *cam) {
    return cam->front;
}

static inline void lunaCamera_Rotate(lunaCamera *cam, float yaw_, float pitch_) {
    cam->yaw += yaw_;
    cam->pitch -= pitch_;

    cam->yaw = fmodf(cam->yaw, 360.0f);

    const float bound = 89.9f;
    cam->pitch = cmclamp( cam->pitch, -bound, bound );
}

static inline void cam_move(lunaCamera *cam, const vec3 amt) {
    cam->actual_pos = v3add(cam->actual_pos, v3muls(cam->right, amt.x));
    cam->actual_pos = v3add(cam->actual_pos, v3muls(cam->up   , amt.y));
    cam->actual_pos = v3add(cam->actual_pos, v3muls(cam->front, amt.z));
}

static inline void cam_set_position(lunaCamera *cam, const vec3 pos) {
    cam->actual_pos = pos;
}

static inline void lunaCamera_Update(lunaCamera *cam, struct lunaRenderer_t *rd) {
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

    const lunaExtent2D RenderExtent = lunaRenderer_GetRenderExtent(rd);
    const float aspect = (float)RenderExtent.width / (float)RenderExtent.height;
    cam->perspective = m4perspective(cam->fov, aspect, cam->near_clip, cam->far_clip);

    camera_uniform_buffer ub = {};
    ub.perspective = cam->perspective;
    ub.ortho = cam->ortho;
    ub.view = cam->view;
    cam->mem_mapped[ lunaRenderer_GetFrame(rd) ] = ub;
}

static inline vec2 lunaCamera_GetMouseGlobalPosition(const lunaCamera *cam) {
    return v2add(
        v2mulv(lunaInput_GetMousePosition(), (vec2){ CAMERA_ORTHO_W, CAMERA_ORTHO_H}),
        (vec2){cam->position.x, cam->position.y}
    );
}

#endif//__LUNA_CAMERA_H__