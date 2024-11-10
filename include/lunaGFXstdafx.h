#ifndef __LUNA_GFX_STDAFX_H
#define __LUNA_GFX_STDAFX_H

#ifdef __cplusplus
    extern "C" {
#endif

#include "../external/volk/volk.h"

#ifndef bool
    typedef unsigned char bool;
#endif

typedef enum luna_Window_OptionBits {
    LUNA_WINDOW_OPTION_VSYNC           = 1 << 0,
    LUNA_WINDOW_OPTION_RESIZABLE       = 1 << 1,
    LUNA_WINDOW_OPTION_BORDERLESS      = 1 << 2,
    LUNA_WINDOW_OPTION_FULLSCREEN      = 1 << 3,
    LUNA_WINDOW_OPTION_MAXIMIZED       = 1 << 4,
    LUNA_WINDOW_OPTION_MINIMIZED       = 1 << 5,
    LUNA_WINDOW_OPTION_HIDDEN          = 1 << 6,
    LUNA_WINDOW_OPTION_HIGH_DPI        = 1 << 7,
    LUNA_WINDOW_OPTION_ALWAYS_ON_TOP   = 1 << 8,
} luna_Window_OptionBits;

typedef enum luna_Window_VSyncBits {
    LUNA_WINDOW_VSYNC_DISABLED = 0,
    LUNA_WINDOW_VSYNC_ENABLED = 1
} luna_Window_VSyncBits;
typedef bool luna_Window_VSync;

typedef enum luna_Window_BufferModeBits {
    CG_BUFFER_MODE_SINGLE_BUFFERED = 0,
    CG_BUFFER_MODE_DOUBLE_BUFFERED = 1,
    CG_BUFFER_MODE_TRIPLE_BUFFERED = 2,
} luna_Window_BufferModeBits;
typedef unsigned lunaBufferMode;

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
typedef unsigned lunaSampleCount;

typedef struct lunaExtent2D {
    int width, height;
} lunaExtent2D;

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

#endif//__LUNA_GFX_STDAFX_H