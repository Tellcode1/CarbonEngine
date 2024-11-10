#include "../include/cgfxpipeline.h"
#include "../include/cshadermanagerdev.h"
#include "../include/lunaPipeline.h"

void cgfx_make_pipeline(  const cgfx_pipeline_create_info *pInfo, cgfx_abstract_descriptor_t *dst )
{
    int iter = 0;

    for (int i = 0; i < nshaders; ++i) {
        int shader_ndescriptors = pInfo->shaders[i].ndescriptors;
        for (int j = 0; j < shader_ndescriptors; j++) {
            const csm_shader_descriptor *shader_descriptor = &pInfo->shaders[i].descriptors_info[j];
            cgfx_descriptor_t *dst_descriptor = &dst->descriptors[iter + j];
            dst_descriptor->binding = shader_descriptor->binding;
            dst_descriptor->name = shader_descriptor->name;
            dst_descriptor->set = shader_descriptor->set;
            dst_descriptor->type = shader_descriptor->type;
            memset(dst_descriptor->data, 0, sizeof(dst_descriptor->data));
        }
        iter += shader_ndescriptors;
    }

    int shader_stages[ iter ];
    iter = 0;
    for (int i = 0; i < nshaders; ++i) {
        int shader_ndescriptors = pInfo->shaders[i].ndescriptors;
        for (int j = 0; j < shader_ndescriptors; j++) {
            const csm_shader_descriptor *shader_descriptor = &pInfo->shaders[i].descriptors_info[j];
            shader_stages[iter + j] = shader_descriptor->parent->stage;
        }
        iter += shader_ndescriptors;
    }

    const int total_descriptor_bindings = iter;

    dst->ndescriptors = total_descriptor_bindings;
    VkDescriptorSetLayoutBinding *bindings = calloc(total_descriptor_bindings, sizeof(VkDescriptorSetLayoutBinding));

    for (int i = 0; i < total_descriptor_bindings; i++) {
        VkDescriptorSetLayoutBinding *dstbinding = &bindings[i];
        cgfx_descriptor_t *src_descriptor = &dst->descriptors[i];
        dstbinding->binding = src_descriptor->binding;
        dstbinding->descriptorCount = 1;
        dstbinding->descriptorType = (src_descriptor->type == CGFX_DESCRIPTOR_TYPE_UNIFORM_BUFFER) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER :
                                     (src_descriptor->type == CGFX_DESCRIPTOR_TYPE_SS_BUFFER) ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER :
                                     (src_descriptor->type == CGFX_DESCRIPTOR_TYPE_IMAGE) ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER :
                                     VK_DESCRIPTOR_TYPE_MAX_ENUM;
        dstbinding->stageFlags = shader_stages[i];
    }

    VkDescriptorSetLayoutCreateInfo descriptor_layout_info = {};
    descriptor_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_layout_info.bindingCount = total_descriptor_bindings;
    descriptor_layout_info.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(device, &descriptor_layout_info, NULL, &dst->set_layout) != VK_SUCCESS) {
        printf("descrpitor create error.\n");
        free(bindings);
        return;
    }

    free(bindings);
}