#ifndef __LUNA_FORMAT_H__
#define __LUNA_FORMAT_H__

typedef enum lunaFormat {
    LUNA_FORMAT_UNDEFINED = 0,

    // These are UNORM's by the way.
    // Because that's how images are normally handled in computers.
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

typedef enum VkFormat VkFormat;

extern VkFormat luna_lunaFormatToVKFormat(lunaFormat format);

extern lunaFormat luna_VKFormatTolunaFormat(VkFormat format);

// dst is a pointer to a const char *
// like:
// const char *str; luna_FormatToString(LUNA_FORMAT_R8, &str);
extern void luna_FormatToString(lunaFormat format, const char **dst);

extern bool luna_FormatHasColorChannel(lunaFormat fmt);

// Returns false even for stencil/depth and undefined format
extern bool luna_FormatHasAlphaChannel(lunaFormat fmt);

extern bool luna_FormatHasDepthChannel(lunaFormat fmt);

extern bool luna_FormatHasStencilChannel(lunaFormat fmt);

// returns -1 on invalid format (or compressed formats)
// returns the size of depth component (in bytes) even in combined depth stencil formats
extern int luna_FormatGetBytesPerChannel(lunaFormat fmt);

extern int luna_FormatGetBytesPerPixel(lunaFormat fmt);

// 0 on error or undefined format.
// oh and I haven't implemented compressed formats yet..
// also, stencil channels are also counted
extern int luna_FormatGetNumChannels(lunaFormat fmt);

#endif//__LUNA_FORMAT_H__