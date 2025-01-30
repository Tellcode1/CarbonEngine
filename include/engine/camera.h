#ifndef __NV_CAMERA_H__
#define __NV_CAMERA_H__

#include "../../common/math/mat.h"
#include "../../common/math/vec2.h"
#include "../../common/math/vec3.h"

#include "../GPU/buffer.h"
#include "../engine/input.h"
#include "renderer.h"

NOVA_HEADER_START;

typedef struct NVCamera NVCamera;
typedef struct NV_DescriptorSet NV_DescriptorSet;

#define CAMERA_FAKE_BUFFER_COUNT 3

#define align_up(sz, align)      ((sz + align - 1) & ~(align - 1))

typedef struct camera_uniform_buffer {
  mat4 perspective;
  mat4 ortho;
  mat4 view;
} camera_uniform_buffer;

struct NVCamera {
  // If you draw a quad with this width, it'll cover the whole screen
  // oh, and this should technically be HALVED when you're rendering quads as they generally take HALF size
  // that's just to say this is the FULL width along each direction.
  vec2 ortho_size;

  // TODO: move to uniform buffer with data like current app time, delta time, etc.
  // To be honest i dont know what delta time is supposed to be
  // doing on the GPU except for particle simulations but you ought to
  // use something like push constants for that. Not my fault ;D
  mat4 perspective;
  mat4 ortho;
  mat4 view;

  // This reduces "choppiness" created by moving the camera if the camera has moved after transferring to the uniform buffer
  // position is the position occupied by the camera when it was sent to the uniform buffer
  // actual pos is the real time position of the camera.
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

  NV_GPU_Buffer ub;
  NV_GPU_Memory *mem;
  NV_DescriptorSet *sets;
  camera_uniform_buffer *mem_mapped;

  // NV_GPU_Texture *render_texture;
  // VkFramebuffer framebuffer;
  // VkRenderPass render_pass;
};

extern void NVCamera_Destroy(NVCamera *cam);
extern NVCamera NVCamera_Init();
extern mat4 NVCamera_GetProjection(NVCamera *cam);
extern mat4 NVCamera_GetView(NVCamera *cam);
extern vec3 NVCamera_GetUp(NVCamera *cam);
extern vec3 NVCamera_GetFront(NVCamera *cam);
extern void NVCamera_Rotate(NVCamera *cam, float yaw_, float pitch_);
extern void NVCamera_Move(NVCamera *cam, const vec3 amt);
extern void NVCamera_SetPosition(NVCamera *cam, const vec3 pos);
extern void NVCamera_Update(NVCamera *cam, struct NVRenderer_t *rd);
extern vec2 NVCamera_GetMouseGlobalPosition(const NVCamera *cam);

NOVA_HEADER_END;

#endif //__NOVA_CAMERA_H__