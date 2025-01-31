#ifndef __NV_camera_H__
#define __NV_camera_H__

#include "../../common/math/mat.h"
#include "../../common/math/vec2.h"
#include "../../common/math/vec3.h"

#include "../GPU/buffer.h"
#include "../engine/input.h"
#include "renderer.h"

NOVA_HEADER_START;

typedef struct NV_camera_t NV_camera_t;
typedef struct NV_descriptor_set NV_descriptor_set;

#define CAMERA_FAKE_BUFFER_COUNT 3

#define align_up(sz, align)      ((sz + align - 1) & ~(align - 1))

typedef struct camera_uniform_buffer {
  mat4 perspective;
  mat4 ortho;
  mat4 view;
} camera_uniform_buffer;

struct NV_camera_t {
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
  NV_descriptor_set *sets;
  camera_uniform_buffer *mem_mapped;

  // NV_GPU_Texture *render_texture;
  // VkFramebuffer framebuffer;
  // VkRenderPass render_pass;
};

extern void NV_camera_destroy(NV_camera_t *cam);
extern NV_camera_t NV_camera_init();
extern mat4 NV_camera_get_projection(NV_camera_t *cam);
extern mat4 NV_camera_get_view(NV_camera_t *cam);
extern vec3 NV_camera_get_up_vector(NV_camera_t *cam);
extern vec3 NV_camera_get_front_vector(NV_camera_t *cam);
extern void NV_camera_rotate(NV_camera_t *cam, float yaw_, float pitch_);
extern void NV_camera_move(NV_camera_t *cam, const vec3 amt);
extern void NV_camera_set_position(NV_camera_t *cam, const vec3 pos);
extern void NV_camera_update(NV_camera_t *cam, struct NV_renderer_t *rd);
extern vec2 NV_camera_get_global_mouse_position(const NV_camera_t *cam);

NOVA_HEADER_END;

#endif //__NOVA_CAMERA_H__