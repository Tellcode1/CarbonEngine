#ifndef __LUNA_GFX_STDAFX_H
#define __LUNA_GFX_STDAFX_H

#ifdef __cplusplus
    extern "C" {
#endif

#include "../../external/volk/volk.h"

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

#ifdef __cplusplus
    }
#endif

#endif//__LUNA_GFX_STDAFX_H