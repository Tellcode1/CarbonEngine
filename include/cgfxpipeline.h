#ifndef __CGFX_DESCRIPTORS_H
#define __CGFX_DESCRIPTORS_H

#include "lunaGFXstdafx.h"
#include "cshadermanagerdev.h"
#include "lunaGFX.h"

#define CGFX_MAX_SHADERS_PER_PIPELINE (10)
#define CGFX_MAX_DESCRIPTORS_PER_SHADER (10)

typedef struct cgfx_render_texture {
    VkImage image;
    VkImageView image_view;
    VkDeviceMemory memory;
    int memory_offset;
    int width, height;
} cgfx_render_texture;

typedef struct cgfxcamera {
    cgfx_render_texture render_texture;
    VkFramebuffer framebuffer;
    VkRenderPass pass;
    VkFormat fmt;
    lunaExtent2D extent;
} cgfxcamera;

typedef struct cgfx_descriptor {
    VkDescriptorType type;
    int binding, array_index;
    const char *name;
} cgfx_descriptor;

typedef struct cgfx_descriptor_set {
    VkDescriptorSet set;
    VkDescriptorSetLayout layout;
    cgfx_descriptor *descriptors;
    int ndescriptors;
} cgfx_descriptor_set;

// should be like cgfx_update_descriptors("pootis", CGFX_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uniform_buffer);
typedef struct cgfx_descriptor_t {
    const char *name;
    VkDescriptorType type;
    int binding;
    int set;
    uchar data[ sizeof(void *) ]; // enough to hold one handle
} cgfx_descriptor_t;

typedef struct cgfx_pipeline_t {
    const char *name;
    VkFormat format;
    cgfx_descriptor_t descriptors[ CGFX_MAX_DESCRIPTORS_PER_SHADER ];
    csm_shader_t shaders[ CGFX_MAX_SHADERS_PER_PIPELINE ];
    int nshaders, ndescriptors;
} cgfx_pipeline_t;

typedef struct cgfx_abstract_descriptor_t {
    int ndescriptors;
    VkDescriptorSetLayout set_layout;
    cgfx_descriptor_t descriptors[ CGFX_MAX_DESCRIPTORS_PER_SHADER ];
} cgfx_abstract_descriptor_t;

typedef struct cgfx_pipeline_create_info {
    VkVertexInputAttributeDescription *attribute_descriptions;
    int nattribute_descriptions;
    VkVertexInputBindingDescription *binding_descriptions;
    int nbinding_descriptions;
    csm_shader_t *shaders;
    int nshaders;
} cgfx_pipeline_create_info;

void cgfx_make_pipeline(  const cgfx_pipeline_create_info *pInfo, cgfx_abstract_descriptor_t *dst );

static inline void cgfx_descriptor_update(const char *varname, void *value, cgfx_abstract_descriptor_t *descriptor) {
    for (int i = 0; i < descriptor->ndescriptors; i++) {
        if (strcmp(descriptor->descriptors[i].name, varname) == 0) {
            *((long *)descriptor->descriptors[i].data) = *(long *)value;
        }
    }
}

#endif//__CGFX_DESCRIPTORS_H