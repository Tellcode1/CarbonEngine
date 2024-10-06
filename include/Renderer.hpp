#ifndef __RENDERER__
#define __RENDERER__

#include "defines.h"
#include "stdafx.h"
#include "vkstdafx.h"

#include "../external/volk/volk.h"

#include "cengine.hpp"
#include "containers/cvector.h"

#include "engine/camera.hpp"

#include <SDL2/SDL.h>

typedef bool_t cengine_bool_t;

// The camera should also own the output texture, etc.
// Basically, the camera should be the first parameter that the renderer needs.
// Something like render_begin( renderer, camera )
// Btw this struct is for testing, this isn't an actual camera.
// I'll get to that when I switch to making the game object, transforms, etc.
extern ccamera camera;

// Move ownership to camera VV
struct FrameRenderData
{
    VkImage         swapchainImage;
    VkImageView     swapchainImageView;
    VkFramebuffer   color_framebuffer;
    VkImage         depth_image;
    VkImageView     depth_image_view;
    VkDeviceMemory  shadow_image_memory; // FIXME: move to Renderer struct to allocate all at once
    VkSemaphore     imageAvailableSemaphore;
    VkSemaphore     renderingFinishedSemaphore;
    VkFence         inFlightFence;
};

typedef enum cengine_vsync_bits {
    CENGINE_VSYNC_DISABLED = 0,
    CENGINE_VSYNC_ENABLED = 1
} cengine_vsync_bits;
typedef unsigned char cengine_vsync;

typedef enum cengine_buffering_mode_bits {
    CENGINE_BUFFER_MODE_SINGLE_BUFFERED = 0,
    CENGINE_BUFFER_MODE_DOUBLE_BUFFERED = 1,
    CENGINE_BUFFER_MODE_TRIPLE_BUFFERED = 2,
} cengine_buffering_mode_bits;
typedef unsigned cengine_buffering_mode;

typedef enum cengine_sample_count_bits {
    CENGINE_SAMPLE_COUNT_MAX_SUPPORTED = 0xFFFFFFFF,
    CENGINE_SAMPLE_COUNT_NO_EXTRA_SAMPLES = 0,
    CENGINE_SAMPLE_COUNT_1_SAMPLES = 0,
    CENGINE_SAMPLE_COUNT_2_SAMPLES = 2,
    CENGINE_SAMPLE_COUNT_4_SAMPLES = 4,
    CENGINE_SAMPLE_COUNT_8_SAMPLES = 8,
    CENGINE_SAMPLE_COUNT_16_SAMPLES = 16,
    CENGINE_SAMPLE_COUNT_32_SAMPLES = 32,
} cengine_sample_count_bits;
typedef unsigned cengine_sample_count;

typedef struct cengine_extent2d {
    int width, height;
} cengine_extent2d;

struct crenderer_config
{
    cengine_sample_count   samples = CENGINE_SAMPLE_COUNT_NO_EXTRA_SAMPLES;
    cengine_bool_t         multisampling_enable = 0;
    cengine_extent2d       window_size = { 800, 600 };
    cengine_bool_t         window_resizable = 0;
    cengine_buffering_mode buffer_mode;
    cengine_vsync          vsync_enabled;
    SDL_Scancode           exit_key = SDL_SCANCODE_ESCAPE;
};

typedef struct crenderer_t crenderer_t;

#endif