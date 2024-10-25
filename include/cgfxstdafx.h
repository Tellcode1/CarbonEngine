#ifndef __CGFXSTDAFX_H
#define __CGFXSTDAFX_H

#ifdef __cplusplus
    extern "C" {
#endif

#include "../external/volk/volk.h"

#ifdef bool_t
    typedef bool_t cg_bool_t;
#else
    typedef unsigned char cg_bool_t;
#endif

typedef enum cengine_vsync_bits {
    CG_VSYNC_DISABLED = 0,
    CG_VSYNC_ENABLED = 1
} cengine_vsync_bits;
typedef cg_bool_t cg_vsync;

typedef enum cg_buffering_mode_bits {
    CG_BUFFER_MODE_SINGLE_BUFFERED = 0,
    CG_BUFFER_MODE_DOUBLE_BUFFERED = 1,
    CG_BUFFER_MODE_TRIPLE_BUFFERED = 2,
} cg_buffering_mode_bits;
typedef unsigned cg_buffering_mode;

typedef enum cg_sample_count_bits {
    CG_SAMPLE_COUNT_MAX_SUPPORTED = 0xFFFFFFFF,
    CG_SAMPLE_COUNT_NO_EXTRA_SAMPLES = 1,
    CG_SAMPLE_COUNT_1_SAMPLES = 1,
    CG_SAMPLE_COUNT_2_SAMPLES = 2,
    CG_SAMPLE_COUNT_4_SAMPLES = 4,
    CG_SAMPLE_COUNT_8_SAMPLES = 8,
    CG_SAMPLE_COUNT_16_SAMPLES = 16,
    CG_SAMPLE_COUNT_32_SAMPLES = 32,
} cg_sample_count_bits;
typedef unsigned cg_sample_count;

typedef struct cg_extent2d {
    int width, height;
} cg_extent2d;

#include <vulkan/vulkan.h>

static inline int vk_fmt_get_bpp(VkFormat format) {
    switch (format) {
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8_SINT:
        case VK_FORMAT_R8_SRGB:
            return 1;

        case VK_FORMAT_R16_UNORM:
        case VK_FORMAT_R16_SNORM:
        case VK_FORMAT_R16_UINT:
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16_SFLOAT:
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8_SINT:
        case VK_FORMAT_R8G8_SRGB:
            return 2;

        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SNORM:
        case VK_FORMAT_R8G8B8_UINT:
        case VK_FORMAT_R8G8B8_SINT:
        case VK_FORMAT_R8G8B8_SRGB:
            return 3;

        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32_SFLOAT:
        case VK_FORMAT_R16G16_UNORM:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_R16G16_UINT:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16_SFLOAT:
            return 4;

        case VK_FORMAT_R16G16B16A16_UNORM:
        case VK_FORMAT_R16G16B16A16_SNORM:
        case VK_FORMAT_R16G16B16A16_UINT:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32_SFLOAT:
            return 8;

        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32_SFLOAT:
            return 12;

        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return 16;

        default:
            // warning: compressed formats don't have a set bpp so don't even think about it
            return 0;
    }
}


#ifdef __cplusplus
    }
#endif

#endif//__CGFXTEXTURE_H