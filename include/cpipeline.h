#ifndef __CGFXPIPELINE_H
#define __CGFXPIPELINE_H

#include <stdlib.h>

#define CPIPELINE_MAX_NAME_SIZE 128

typedef struct cpipeline_t {
    const char name[ CPIPELINE_MAX_NAME_SIZE ];
    struct VkPipeline_T *pipeline;
    struct VkPipelineLayout_T *layout;
    struct csm_shader_t **shaders;
    int nshaders;
} cpipeline_t;

static cpipeline_t *_cpipeline_map = NULL;
static int _ncpipelines = 0;

typedef enum cformat {
    CFMT_UNKNOWN = 0, // so the user knows they didn't pick a format

    // 8 bit txeture formats
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
} cformat;

typedef struct cpipeline_attribute_description {
    int location, binding, offset;
    cformat fmt;
} cpipeline_attribute_description;

typedef struct cpipeline_description {
    cpipeline_attribute_description *pAttributes;
    int nAttributes;
} cpipeline_description;

#endif//__CGFXPIPELINE_H