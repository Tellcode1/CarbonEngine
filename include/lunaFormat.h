#ifndef __LUNA_FORMAT_H__
#define __LUNA_FORMAT_H__

#include "vkstdafx.h"

typedef enum lunaFormat {
    LUNA_FORMAT_UNDEFINED = 0,

    LUNA_FORMAT_R8,
    LUNA_FORMAT_RG8,
    LUNA_FORMAT_RGB8,
    LUNA_FORMAT_RGBA8,

    LUNA_FORMAT_BGR8,
    LUNA_FORMAT_BGRA8,

    LUNA_FORMAT_RGB16,
    LUNA_FORMAT_RGBA16,
    LUNA_FORMAT_RG32,
    LUNA_FORMAT_RGB32,
    LUNA_FORMAT_RGBA32,
    
    LUNA_FORMAT_R8_SINT,
    LUNA_FORMAT_RG8_SINT,
    LUNA_FORMAT_RGB8_SINT,
    LUNA_FORMAT_RGBA8_SINT,

    LUNA_FORMAT_R8_UINT,
    LUNA_FORMAT_RG8_UINT,
    LUNA_FORMAT_RGB8_UINT,
    LUNA_FORMAT_RGBA8_UINT,

    LUNA_FORMAT_R8_SRGB,
    LUNA_FORMAT_RG8_SRGB,
    LUNA_FORMAT_RGB8_SRGB,
    LUNA_FORMAT_RGBA8_SRGB,

    LUNA_FORMAT_BGR8_SRGB,
    LUNA_FORMAT_BGRA8_SRGB,

    LUNA_FORMAT_D16,
    LUNA_FORMAT_D24,
    LUNA_FORMAT_D32,
    LUNA_FORMAT_D24_S8,
    LUNA_FORMAT_D32_S8,

    LUNA_FORMAT_BC1,
    LUNA_FORMAT_BC3,
    LUNA_FORMAT_BC7,
} lunaFormat;

static inline VkFormat luna_lunaFormatToVKFormat(lunaFormat format) {
    switch (format) {
        case LUNA_FORMAT_R8:        return VK_FORMAT_R8_UNORM;
        case LUNA_FORMAT_RG8:       return VK_FORMAT_R8G8_UNORM;
        case LUNA_FORMAT_RGB8:      return VK_FORMAT_R8G8B8_UNORM;
        case LUNA_FORMAT_RGBA8:     return VK_FORMAT_R8G8B8A8_UNORM;

        case LUNA_FORMAT_BGR8:      return VK_FORMAT_B8G8R8_UNORM;
        case LUNA_FORMAT_BGRA8:     return VK_FORMAT_B8G8R8A8_UNORM;

        case LUNA_FORMAT_RGB16:     return VK_FORMAT_R16G16B16_UNORM;
        case LUNA_FORMAT_RGBA16:    return VK_FORMAT_R16G16B16A16_UNORM;
        case LUNA_FORMAT_RG32:      return VK_FORMAT_R32G32_SFLOAT;
        case LUNA_FORMAT_RGB32:     return VK_FORMAT_R32G32B32_SFLOAT;
        case LUNA_FORMAT_RGBA32:    return VK_FORMAT_R32G32B32A32_SFLOAT;
        
        case LUNA_FORMAT_R8_SINT:   return VK_FORMAT_R8_SINT;
        case LUNA_FORMAT_RG8_SINT:  return VK_FORMAT_R8G8_SINT;
        case LUNA_FORMAT_RGB8_SINT: return VK_FORMAT_R8G8B8_SINT;
        case LUNA_FORMAT_RGBA8_SINT:return VK_FORMAT_R8G8B8A8_SINT;

        case LUNA_FORMAT_R8_UINT:   return VK_FORMAT_R8_UINT;
        case LUNA_FORMAT_RG8_UINT:  return VK_FORMAT_R8G8_UINT;
        case LUNA_FORMAT_RGB8_UINT: return VK_FORMAT_R8G8B8_UINT;
        case LUNA_FORMAT_RGBA8_UINT:return VK_FORMAT_R8G8B8A8_UINT;

        case LUNA_FORMAT_R8_SRGB:  return VK_FORMAT_R8_SRGB;
        case LUNA_FORMAT_RG8_SRGB:  return VK_FORMAT_R8G8_SRGB;
        case LUNA_FORMAT_RGB8_SRGB:  return VK_FORMAT_R8G8B8_SRGB;
        case LUNA_FORMAT_RGBA8_SRGB:  return VK_FORMAT_R8G8B8A8_SRGB;

        case LUNA_FORMAT_BGR8_SRGB:  return VK_FORMAT_B8G8R8_SRGB;
        case LUNA_FORMAT_BGRA8_SRGB:  return VK_FORMAT_B8G8R8A8_SRGB;

        case LUNA_FORMAT_D16:       return VK_FORMAT_D16_UNORM;
        case LUNA_FORMAT_D24:       return VK_FORMAT_D24_UNORM_S8_UINT;
        case LUNA_FORMAT_D32:       return VK_FORMAT_D32_SFLOAT;
        case LUNA_FORMAT_D24_S8:    return VK_FORMAT_D24_UNORM_S8_UINT;
        case LUNA_FORMAT_D32_S8:    return VK_FORMAT_D32_SFLOAT_S8_UINT;

        case LUNA_FORMAT_BC1:       return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
        case LUNA_FORMAT_BC3:       return VK_FORMAT_BC3_UNORM_BLOCK;
        case LUNA_FORMAT_BC7:       return VK_FORMAT_BC7_UNORM_BLOCK;

        default:                    return VK_FORMAT_UNDEFINED;
    }
}

static inline lunaFormat luna_VKFormatTolunaFormat(VkFormat format) {
    switch (format) {
        case VK_FORMAT_R8_UNORM:        return LUNA_FORMAT_R8;
        case VK_FORMAT_R8G8_UNORM:       return LUNA_FORMAT_RG8;
        case VK_FORMAT_R8G8B8_UNORM:      return LUNA_FORMAT_RGB8;
        case VK_FORMAT_R8G8B8A8_UNORM:     return LUNA_FORMAT_RGBA8;

        case VK_FORMAT_B8G8R8_UNORM:      return LUNA_FORMAT_BGR8;
        case VK_FORMAT_B8G8R8A8_UNORM:     return LUNA_FORMAT_BGRA8;

        case VK_FORMAT_R16G16B16_UNORM:     return LUNA_FORMAT_RGB16;
        case VK_FORMAT_R16G16B16A16_UNORM:    return LUNA_FORMAT_RGBA16;
        case VK_FORMAT_R32G32_SFLOAT:      return LUNA_FORMAT_RG32;
        case VK_FORMAT_R32G32B32_SFLOAT:     return LUNA_FORMAT_RGB32;
        case VK_FORMAT_R32G32B32A32_SFLOAT:    return LUNA_FORMAT_RGBA32;
        
        case VK_FORMAT_R8_SINT:   return LUNA_FORMAT_R8_SINT;
        case VK_FORMAT_R8G8_SINT:  return LUNA_FORMAT_RG8_SINT;
        case VK_FORMAT_R8G8B8_SINT: return LUNA_FORMAT_RGB8_SINT;
        case VK_FORMAT_R8G8B8A8_SINT:return LUNA_FORMAT_RGBA8_SINT;

        case VK_FORMAT_R8_SRGB:  return LUNA_FORMAT_R8_SRGB;
        case VK_FORMAT_R8G8_SRGB:  return LUNA_FORMAT_RG8_SRGB;
        case VK_FORMAT_R8G8B8_SRGB:  return LUNA_FORMAT_RGB8_SRGB;
        case VK_FORMAT_R8G8B8A8_SRGB:  return LUNA_FORMAT_RGBA8_SRGB;

        case VK_FORMAT_B8G8R8_SRGB:  return LUNA_FORMAT_BGR8_SRGB;
        case VK_FORMAT_B8G8R8A8_SRGB:  return LUNA_FORMAT_BGRA8_SRGB;

        case VK_FORMAT_R8_UINT:   return LUNA_FORMAT_R8_UINT;
        case VK_FORMAT_R8G8_UINT:  return LUNA_FORMAT_RG8_UINT;
        case VK_FORMAT_R8G8B8_UINT: return LUNA_FORMAT_RGB8_UINT;
        case VK_FORMAT_R8G8B8A8_UINT:return LUNA_FORMAT_RGBA8_UINT;

        case VK_FORMAT_D16_UNORM:       return LUNA_FORMAT_D16;
        case VK_FORMAT_D32_SFLOAT:       return LUNA_FORMAT_D32;
        case VK_FORMAT_D24_UNORM_S8_UINT:    return LUNA_FORMAT_D24_S8;
        case VK_FORMAT_D32_SFLOAT_S8_UINT:    return LUNA_FORMAT_D32_S8;

        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:       return LUNA_FORMAT_BC1;
        case VK_FORMAT_BC3_UNORM_BLOCK:       return LUNA_FORMAT_BC3;
        case VK_FORMAT_BC7_UNORM_BLOCK:       return LUNA_FORMAT_BC7;

        default:                    return LUNA_FORMAT_UNDEFINED;
    }
}

static inline void luna_FormatToString(lunaFormat format, const char **dst) {
    switch(format) {
        case LUNA_FORMAT_UNDEFINED: *dst = "LUNA_FORMAT_UNDEFINED"; return;
        case LUNA_FORMAT_R8: *dst = "LUNA_FORMAT_R8"; return;
        case LUNA_FORMAT_RG8: *dst = "LUNA_FORMAT_RG8"; return;
        case LUNA_FORMAT_RGB8: *dst = "LUNA_FORMAT_RGB8"; return;
        case LUNA_FORMAT_RGBA8: *dst = "LUNA_FORMAT_RGBA8"; return;
        case LUNA_FORMAT_BGR8: *dst = "LUNA_FORMAT_BGR8"; return;
        case LUNA_FORMAT_BGRA8: *dst = "LUNA_FORMAT_BGRA8"; return;
        case LUNA_FORMAT_RGB16: *dst = "LUNA_FORMAT_RGB16"; return;
        case LUNA_FORMAT_RGBA16: *dst = "LUNA_FORMAT_RGBA16"; return;
        case LUNA_FORMAT_RG32: *dst = "LUNA_FORMAT_RG32"; return;
        case LUNA_FORMAT_RGB32: *dst = "LUNA_FORMAT_RGB32"; return;
        case LUNA_FORMAT_RGBA32: *dst = "LUNA_FORMAT_RGBA32"; return;
        case LUNA_FORMAT_R8_SINT: *dst = "LUNA_FORMAT_R8_SINT"; return;
        case LUNA_FORMAT_RG8_SINT: *dst = "LUNA_FORMAT_RG8_SINT"; return;
        case LUNA_FORMAT_RGB8_SINT: *dst = "LUNA_FORMAT_RGB8_SINT"; return;
        case LUNA_FORMAT_RGBA8_SINT: *dst = "LUNA_FORMAT_RGBA8_SINT"; return;
        case LUNA_FORMAT_R8_UINT: *dst = "LUNA_FORMAT_R8_UINT"; return;
        case LUNA_FORMAT_RG8_UINT: *dst = "LUNA_FORMAT_RG8_UINT"; return;
        case LUNA_FORMAT_RGB8_UINT: *dst = "LUNA_FORMAT_RGB8_UINT"; return;
        case LUNA_FORMAT_RGBA8_UINT: *dst = "LUNA_FORMAT_RGBA8_UINT"; return;
        case LUNA_FORMAT_R8_SRGB: *dst = "LUNA_FORMAT_R8_SRGB"; return;
        case LUNA_FORMAT_RG8_SRGB: *dst = "LUNA_FORMAT_RG8_SRGB"; return;
        case LUNA_FORMAT_RGB8_SRGB: *dst = "LUNA_FORMAT_RGB8_SRGB"; return;
        case LUNA_FORMAT_RGBA8_SRGB: *dst = "LUNA_FORMAT_RGBA8_SRGB"; return;
        case LUNA_FORMAT_BGR8_SRGB: *dst = "LUNA_FORMAT_BGR8_SRGB"; return;
        case LUNA_FORMAT_BGRA8_SRGB: *dst = "LUNA_FORMAT_BGRA8_SRGB"; return;
        case LUNA_FORMAT_D16: *dst = "LUNA_FORMAT_D16"; return;
        case LUNA_FORMAT_D24: *dst = "LUNA_FORMAT_D24"; return;
        case LUNA_FORMAT_D32: *dst = "LUNA_FORMAT_D32"; return;
        case LUNA_FORMAT_D24_S8: *dst = "LUNA_FORMAT_D24_S8"; return;
        case LUNA_FORMAT_D32_S8: *dst = "LUNA_FORMAT_D32_S8"; return;
        case LUNA_FORMAT_BC1: *dst = "LUNA_FORMAT_BC1"; return;
        case LUNA_FORMAT_BC3: *dst = "LUNA_FORMAT_BC3"; return;
        case LUNA_FORMAT_BC7: *dst = "LUNA_FORMAT_BC7"; return;
    }
}

#endif//__LUNA_FORMAT_H__