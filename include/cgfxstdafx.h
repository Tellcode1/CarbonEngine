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

typedef enum cg_format {
    CFMT_UNKNOWN = 0, // so the user knows they didn't pick a format

    // 8 bits per pixel texture formats
    CFMT_R8_UNORM,
    CFMT_R8_SNORM,
    CFMT_R8_UINT,
    CFMT_R8_SINT,
    CFMT_RG8_UNORM,
    CFMT_RG8_SNORM,
    CFMT_RG8_UINT,
    CFMT_RG8_SINT,
    CFMT_RGB8_UNORM,
    CFMT_RGB8_SNORM,
    CFMT_RGBA8_UNORM,
    CFMT_RGBA8_SNORM,
    CFMT_RGBA8_UINT,
    CFMT_RGBA8_SINT,
    CFMT_SRGB8_ALPHA8,

    // 16 bit formats
    CFMT_R16_UNORM,
    CFMT_R16_SNORM,
    CFMT_R16_UINT,
    CFMT_R16_SINT,
    CFMT_R16_FLOAT,
    CFMT_RG16_UNORM,
    CFMT_RG16_SNORM,
    CFMT_RG16_UINT,
    CFMT_RG16_SINT,
    CFMT_RG16_FLOAT,
    CFMT_RGB16_UNORM,
    CFMT_RGB16_SNORM,
    CFMT_RGB16_UINT,
    CFMT_RGB16_SINT,
    CFMT_RGB16_FLOAT,
    CFMT_RGBA16_UNORM,
    CFMT_RGBA16_SNORM,
    CFMT_RGBA16_UINT,
    CFMT_RGBA16_SINT,
    CFMT_RGBA16_FLOAT,

    CFMT_R32_UINT,
    CFMT_R32_SINT,
    CFMT_R32_FLOAT,
    CFMT_RG32_UINT,
    CFMT_RG32_SINT,
    CFMT_RG32_FLOAT,
    CFMT_RGB32_UINT,
    CFMT_RGB32_SINT,
    CFMT_RGB32_FLOAT,
    CFMT_RGBA32_UINT,
    CFMT_RGBA32_SINT,
    CFMT_RGBA32_FLOAT,

    // depth and setncil formats
    CFMT_D16_UNORM,
    CFMT_D32_FLOAT,
    CFMT_D24_UNORM_S8_UINT,
    CFMT_D32_FLOAT_S8_UINT,
} cg_format;

static inline VkFormat cfmt_conv_cfmt_to_vkfmt(cg_format fmt) {
    switch (fmt) {
        case CFMT_R8_UNORM: return VK_FORMAT_R8_UNORM;
        case CFMT_R8_SNORM: return VK_FORMAT_R8_SNORM;
        case CFMT_R8_UINT:  return VK_FORMAT_R8_UINT;
        case CFMT_R8_SINT:  return VK_FORMAT_R8_SINT;
        case CFMT_RG8_UNORM: return VK_FORMAT_R8G8_UNORM;
        case CFMT_RG8_SNORM: return VK_FORMAT_R8G8_SNORM;
        case CFMT_RG8_UINT:  return VK_FORMAT_R8G8_UINT;
        case CFMT_RG8_SINT:  return VK_FORMAT_R8G8_SINT;
        case CFMT_RGB8_UNORM: return VK_FORMAT_R8G8B8_UNORM;
        case CFMT_RGB8_SNORM: return VK_FORMAT_R8G8B8_SNORM;
        case CFMT_RGBA8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
        case CFMT_RGBA8_SNORM: return VK_FORMAT_R8G8B8A8_SNORM;
        case CFMT_RGBA8_UINT:  return VK_FORMAT_R8G8B8A8_UINT;
        case CFMT_RGBA8_SINT:  return VK_FORMAT_R8G8B8A8_SINT;
        case CFMT_SRGB8_ALPHA8: return VK_FORMAT_R8G8B8A8_SRGB;

        case CFMT_R16_UNORM: return VK_FORMAT_R16_UNORM;
        case CFMT_R16_SNORM: return VK_FORMAT_R16_SNORM;
        case CFMT_R16_UINT:  return VK_FORMAT_R16_UINT;
        case CFMT_R16_SINT:  return VK_FORMAT_R16_SINT;
        case CFMT_R16_FLOAT: return VK_FORMAT_R16_SFLOAT;
        case CFMT_RG16_UNORM: return VK_FORMAT_R16G16_UNORM;
        case CFMT_RG16_SNORM: return VK_FORMAT_R16G16_SNORM;
        case CFMT_RG16_UINT:  return VK_FORMAT_R16G16_UINT;
        case CFMT_RG16_SINT:  return VK_FORMAT_R16G16_SINT;
        case CFMT_RG16_FLOAT: return VK_FORMAT_R16G16_SFLOAT;
        case CFMT_RGB16_UNORM: return VK_FORMAT_R16G16B16_UNORM;
        case CFMT_RGB16_SNORM: return VK_FORMAT_R16G16B16_SNORM;
        case CFMT_RGB16_UINT:  return VK_FORMAT_R16G16B16_UINT;
        case CFMT_RGB16_SINT:  return VK_FORMAT_R16G16B16_SINT;
        case CFMT_RGB16_FLOAT: return VK_FORMAT_R16G16B16_SFLOAT;
        case CFMT_RGBA16_UNORM: return VK_FORMAT_R16G16B16A16_UNORM;
        case CFMT_RGBA16_SNORM: return VK_FORMAT_R16G16B16A16_SNORM;
        case CFMT_RGBA16_UINT:  return VK_FORMAT_R16G16B16A16_UINT;
        case CFMT_RGBA16_SINT:  return VK_FORMAT_R16G16B16A16_SINT;
        case CFMT_RGBA16_FLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;

        case CFMT_R32_UINT:  return VK_FORMAT_R32_UINT;
        case CFMT_R32_SINT:  return VK_FORMAT_R32_SINT;
        case CFMT_R32_FLOAT: return VK_FORMAT_R32_SFLOAT;
        case CFMT_RG32_UINT:  return VK_FORMAT_R32G32_UINT;
        case CFMT_RG32_SINT:  return VK_FORMAT_R32G32_SINT;
        case CFMT_RG32_FLOAT: return VK_FORMAT_R32G32_SFLOAT;
        case CFMT_RGB32_UINT: return VK_FORMAT_R32G32B32_UINT;
        case CFMT_RGB32_SINT: return VK_FORMAT_R32G32B32_SINT;
        case CFMT_RGB32_FLOAT: return VK_FORMAT_R32G32B32_SFLOAT;
        case CFMT_RGBA32_UINT: return VK_FORMAT_R32G32B32A32_UINT;
        case CFMT_RGBA32_SINT: return VK_FORMAT_R32G32B32A32_SINT;
        case CFMT_RGBA32_FLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;

        case CFMT_D16_UNORM: return VK_FORMAT_D16_UNORM;
        case CFMT_D32_FLOAT: return VK_FORMAT_D32_SFLOAT;
        case CFMT_D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT;
        case CFMT_D32_FLOAT_S8_UINT: return VK_FORMAT_D32_SFLOAT_S8_UINT;

        case CFMT_UNKNOWN:
        default:
            return VK_FORMAT_UNDEFINED; // maybe print or something idk
    }
}

static inline cg_format cfmt_conv_vkfmt_to_cfmt(VkFormat fmt) {
    switch(fmt) {
        case VK_FORMAT_R8_UNORM: return CFMT_R8_UNORM;
        case VK_FORMAT_R8_SNORM: return CFMT_R8_SNORM;
        case VK_FORMAT_R8_UINT:  return CFMT_R8_UINT;
        case VK_FORMAT_R8_SINT:  return CFMT_R8_SINT;
        case VK_FORMAT_R8G8_UNORM: return CFMT_RG8_UNORM;
        case VK_FORMAT_R8G8_SNORM: return CFMT_RG8_SNORM;
        case VK_FORMAT_R8G8_UINT:  return CFMT_RG8_UINT;
        case VK_FORMAT_R8G8_SINT:  return CFMT_RG8_SINT;
        case VK_FORMAT_R8G8B8_UNORM: return CFMT_RGB8_UNORM;
        case VK_FORMAT_R8G8B8_SNORM: return CFMT_RGB8_SNORM;
        case VK_FORMAT_R8G8B8A8_UNORM: return CFMT_RGBA8_UNORM;
        case VK_FORMAT_R8G8B8A8_SNORM: return CFMT_RGBA8_SNORM;
        case VK_FORMAT_R8G8B8A8_UINT:  return CFMT_RGBA8_UINT;
        case VK_FORMAT_R8G8B8A8_SINT:  return CFMT_RGBA8_SINT;
        case VK_FORMAT_R8G8B8A8_SRGB: return CFMT_SRGB8_ALPHA8;

        case VK_FORMAT_R16_UNORM: return CFMT_R16_UNORM;
        case VK_FORMAT_R16_SNORM: return CFMT_R16_SNORM;
        case VK_FORMAT_R16_UINT:  return CFMT_R16_UINT;
        case VK_FORMAT_R16_SINT:  return CFMT_R16_SINT;
        case VK_FORMAT_R16_SFLOAT: return CFMT_R16_FLOAT;
        case VK_FORMAT_R16G16_UNORM: return CFMT_RG16_UNORM;
        case VK_FORMAT_R16G16_SNORM: return CFMT_RG16_SNORM;
        case VK_FORMAT_R16G16_UINT:  return CFMT_RG16_UINT;
        case VK_FORMAT_R16G16_SINT:  return CFMT_RG16_SINT;
        case VK_FORMAT_R16G16_SFLOAT: return CFMT_RG16_FLOAT;
        case VK_FORMAT_R16G16B16_UNORM: return CFMT_RGB16_UNORM;
        case VK_FORMAT_R16G16B16_SNORM: return CFMT_RGB16_SNORM;
        case VK_FORMAT_R16G16B16_UINT:  return CFMT_RGB16_UINT;
        case VK_FORMAT_R16G16B16_SINT:  return CFMT_RGB16_SINT;
        case VK_FORMAT_R16G16B16_SFLOAT: return CFMT_RGB16_FLOAT;
        case VK_FORMAT_R16G16B16A16_UNORM: return CFMT_RGBA16_UNORM;
        case VK_FORMAT_R16G16B16A16_SNORM: return CFMT_RGBA16_SNORM;
        case VK_FORMAT_R16G16B16A16_UINT:  return CFMT_RGBA16_UINT;
        case VK_FORMAT_R16G16B16A16_SINT:  return CFMT_RGBA16_SINT;
        case VK_FORMAT_R16G16B16A16_SFLOAT: return CFMT_RGBA16_FLOAT;

        case VK_FORMAT_R32_UINT:  return CFMT_R32_UINT;
        case VK_FORMAT_R32_SINT:  return CFMT_R32_SINT;
        case VK_FORMAT_R32_SFLOAT: return CFMT_R32_FLOAT;
        case VK_FORMAT_R32G32_UINT:  return CFMT_RG32_UINT;
        case VK_FORMAT_R32G32_SINT:  return CFMT_RG32_SINT;
        case VK_FORMAT_R32G32_SFLOAT: return CFMT_RG32_FLOAT;
        case VK_FORMAT_R32G32B32_UINT: return CFMT_RGB32_UINT;
        case VK_FORMAT_R32G32B32_SINT: return CFMT_RGB32_SINT;
        case VK_FORMAT_R32G32B32_SFLOAT: return CFMT_RGB32_FLOAT;
        case VK_FORMAT_R32G32B32A32_UINT: return CFMT_RGBA32_UINT;
        case VK_FORMAT_R32G32B32A32_SINT: return CFMT_RGBA32_SINT;
        case VK_FORMAT_R32G32B32A32_SFLOAT: return CFMT_RGBA32_FLOAT;

        case VK_FORMAT_D16_UNORM: return CFMT_D16_UNORM;
        case VK_FORMAT_D32_SFLOAT: return CFMT_D32_FLOAT;
        case VK_FORMAT_D24_UNORM_S8_UINT: return CFMT_D24_UNORM_S8_UINT;
        case VK_FORMAT_D32_SFLOAT_S8_UINT: return CFMT_D32_FLOAT_S8_UINT;

        case VK_FORMAT_UNDEFINED:
        default:
            return CFMT_UNKNOWN; // maybe print or something idk
    }
}

static inline int cfmt_get_bytesperpixel(cg_format format) {
    switch (format) {
        case CFMT_R8_UNORM:
        case CFMT_R8_SNORM:
        case CFMT_R8_UINT:
        case CFMT_R8_SINT:
            return 1;

        case CFMT_RG8_UNORM:
        case CFMT_RG8_SNORM:
        case CFMT_RG8_UINT:
        case CFMT_RG8_SINT:
            return 2;

        case CFMT_RGB8_UNORM:
        case CFMT_RGB8_SNORM:
            return 3;

        case CFMT_RGBA8_UNORM:
        case CFMT_RGBA8_SNORM:
        case CFMT_RGBA8_UINT:
        case CFMT_RGBA8_SINT:
        case CFMT_SRGB8_ALPHA8:
            return 4;

        case CFMT_R16_UNORM:
        case CFMT_R16_SNORM:
        case CFMT_R16_UINT:
        case CFMT_R16_SINT:
        case CFMT_R16_FLOAT:
            return 2;

        case CFMT_RG16_UNORM:
        case CFMT_RG16_SNORM:
        case CFMT_RG16_UINT:
        case CFMT_RG16_SINT:
        case CFMT_RG16_FLOAT:
            return 4;

        case CFMT_RGB16_UNORM:
        case CFMT_RGB16_SNORM:
        case CFMT_RGB16_UINT:
        case CFMT_RGB16_SINT:
        case CFMT_RGB16_FLOAT:
            return 6;

        case CFMT_RGBA16_UNORM:
        case CFMT_RGBA16_SNORM:
        case CFMT_RGBA16_UINT:
        case CFMT_RGBA16_SINT:
        case CFMT_RGBA16_FLOAT:
            return 8;

        case CFMT_R32_UINT:
        case CFMT_R32_SINT:
        case CFMT_R32_FLOAT:
            return 4;

        case CFMT_RG32_UINT:
        case CFMT_RG32_SINT:
        case CFMT_RG32_FLOAT:
            return 8;

        case CFMT_RGB32_UINT:
        case CFMT_RGB32_SINT:
        case CFMT_RGB32_FLOAT:
            return 12;

        case CFMT_RGBA32_UINT:
        case CFMT_RGBA32_SINT:
        case CFMT_RGBA32_FLOAT:
            return 16;

        case CFMT_D16_UNORM:
            return 2;

        case CFMT_D32_FLOAT:
            return 4;

        case CFMT_D24_UNORM_S8_UINT:
            return 4;

        case CFMT_D32_FLOAT_S8_UINT:
            return 5;

        case CFMT_UNKNOWN:
        default:
            return 0;
    }
}

#ifdef __cplusplus
    }
#endif

#endif//__CGFXTEXTURE_H