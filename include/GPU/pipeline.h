#ifndef __NOVA_PIPELINE_H__
#define __NOVA_PIPELINE_H__

#include "../../external/volk/volk.h"

#include "../../common/stdafx.h"
#include "../engine/renderer.h"
#include "vkstdafx.h"

NOVA_HEADER_START;

struct NVSM_shader_t;

typedef void (*NV_GPU_ResultCheckFn)(const VkResult result, const char *__restrict__ FILE, const char *__restrict__ FUNC, unsigned long LINE);

#define CVK_REQUIRED_PTR(ptr)                                                                                                                        \
  if ((ptr) == NULL)                                                                                                                                 \
  NV_LOG_AND_ABORT(#ptr " :  Required parameter \"" #ptr "\" specified as NULL.", NV_basename(__FILE__), __LINE__, __PRETTY_FUNCTION__)
#define CVK_NOT_EQUAL_TO(val, to)                                                                                                                    \
  if ((val) == (to))                                                                                                                                 \
  NV_LOG_AND_ABORT(#val " == " #to ". Value \"" #val "\" must not be equal to " #to ".", NV_basename(__FILE__), __LINE__, __PRETTY_FUNCTION__)

#define __cvk_to_bit(n) (1 << n)

typedef enum cvk_pipeline_flags_bits {
  CVK_PIPELINE_FLAGS_FORCE_DEPTH_CHECK        = __cvk_to_bit(0),
  CVK_PIPELINE_FLAGS_UNFORCE_DEPTH_CHECK      = ~CVK_PIPELINE_FLAGS_FORCE_DEPTH_CHECK,
  CVK_PIPELINE_FLAGS_FORCE_CULLING            = __cvk_to_bit(1),
  CVK_PIPELINE_FLAGS_UNFORCE_CULLING          = ~CVK_PIPELINE_FLAGS_FORCE_CULLING, // Disables culling for resulting pipeline
  CVK_PIPELINE_FLAGS_FORCE_DYNAMIC_VIEWPORT   = __cvk_to_bit(2),
  CVK_PIPELINE_FLAGS_UNFORCE_DYNAMIC_VIEWPORT = ~CVK_PIPELINE_FLAGS_FORCE_DYNAMIC_VIEWPORT,
  CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING      = __cvk_to_bit(3),
  CVK_PIPELINE_FLAGS_UNFORCE_MULTISAMPLING    = ~CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING
} cvk_pipeline_flags_bits;
typedef u32 cvk_pipeline_flags;

#define CVK_ResultCheck(func) __cvk_resultFunc(func, NV_basename(__FILE__), __PRETTY_FUNCTION__, __LINE__)

static void __cvk_defaultResultCheckFunc(const VkResult result, const char *FILE, const char *FUNC, unsigned long LINE) {
  if (result != VK_SUCCESS)
    NV_LOG_ERROR("[%s:%lu] %s: func returned %i", FILE, LINE, FUNC, result);
}

/*
  Used internally. Do NOT modify by yourselves! Use SetResultCheckFunc instead.
*/
extern NV_GPU_ResultCheckFn __cvk_resultFunc;

/*
  Set the result checking function for the API. This is called every time the program requests something in the order of vkCreate* that this namespace
  has a hold of. Use NULL to deattach the function.
*/
static inline void NV_GPU_SetResultCheckFn(NV_GPU_ResultCheckFn func) {
  if (func != NULL)
    __cvk_resultFunc = func;
  else
    func = __cvk_defaultResultCheckFunc;
}

extern u32 NV_GPU_vk_flag_register;

/*
 *	FORWARD DECLARATIONS
 */
typedef struct NV_VK_Pipeline NV_VK_Pipeline;
typedef struct NV_GPU_PipelineCreateInfo NV_GPU_PipelineCreateInfo;
typedef struct NV_GPU_SwapchainCreateInfo NV_GPU_SwapchainCreateInfo;
typedef struct NV_GPU_RenderPassCreateInfo NV_GPU_RenderPassCreateInfo;
typedef struct NV_GPU_PipelineBlendState NV_GPU_PipelineBlendState;

#define NOVA_VK_MAX_SHADERS_PER_PIPELINE 8

typedef struct NV_VK_Pipeline {
  VkPipeline pipeline;
  VkPipelineLayout pipeline_layout;

  // The common descriptor set layout.
  VkDescriptorSetLayout descriptor_layout;
} NV_VK_Pipeline;

typedef struct NV_BakedPipelines {
  NV_VK_Pipeline Unlit;
  NV_VK_Pipeline Lit;
  NV_VK_Pipeline Ctext;
  NV_VK_Pipeline Line; // Draws lines. Yep.
} NV_BakedPipelines;
extern NV_BakedPipelines g_Pipelines;

typedef enum NV_GPU_PipelineBlendPreset {
  CVK_BLEND_PRESET_NONE                = 0,
  CVK_BLEND_PRESET_ALPHA               = 1,
  CVK_BLEND_PRESET_ADDITIVE            = 2,
  CVK_BLEND_PRESET_MULTIPLICATIVE      = 3,
  CVK_BLEND_PRESET_PREMULTIPLIED_ALPHA = 4,
  CVK_BLEND_PRESET_SUBTRACTIVE         = 5,
  CVK_BLEND_PRESET_SCREEN              = 6,
} NV_GPU_PipelineBlendPreset;

typedef struct NV_GPU_PipelineBlendState {
  VkBlendFactor srcColorBlendFactor;
  VkBlendFactor dstColorBlendFactor;
  VkBlendOp colorBlendOp;
  VkBlendFactor srcAlphaBlendFactor;
  VkBlendFactor dstAlphaBlendFactor;
  VkBlendOp alphaBlendOp;
  VkColorComponentFlags colorWriteMask;
} NV_GPU_PipelineBlendState;
extern NV_GPU_PipelineBlendState NV_GPU_InitPipelineBlendState(NV_GPU_PipelineBlendPreset preset);

typedef struct NV_GPU_PipelineCreateInfo {
  VkRenderPass render_pass;
  VkPipelineLayout pipeline_layout;
  VkExtent2D extent;
  NVFormat format;
  u64 subpass;
  VkPipeline old_pipeline;
  VkPipelineCache cache;
  VkPrimitiveTopology topology;

  /* EXTENSIONS */

  // Ignored if flags does not contain PIPELINE_CREATE_FLAGS_ENABLE_MULTISAMPLING
  VkSampleCountFlagBits samples;

  // Ignored if flags does not contain PIPELINE_CREATE_FLAGS_ENABLE_BLEND
  const NV_GPU_PipelineBlendState *blend_state;

  //	Array pointers are allowed to be NULL
  const VkVertexInputAttributeDescription *pAttributeDescriptions;
  const VkVertexInputBindingDescription *pBindingDescriptions;
  const VkDescriptorSetLayout *pDescriptorLayouts;
  const VkPushConstantRange *pPushConstants;
  const struct NVSM_shader_t *const *pShaders;

  int nAttributeDescriptions;
  int nBindingDescriptions;
  int nDescriptorLayouts;
  int nPushConstants;
  int nShaders;
} NV_GPU_PipelineCreateInfo;
#define NV_GPU_InitPipelineCreateInfo()                                                                                                            \
  (NV_GPU_PipelineCreateInfo) {                                                                                                                    \
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, .samples = VK_SAMPLE_COUNT_1_BIT                                                                \
  }

typedef struct NV_GPU_SwapchainCreateInfo {
  VkExtent2D extent;
  VkPresentModeKHR present_mode;
  u64 image_count;
  NVFormat format;
  VkColorSpaceKHR color_space;
  VkSwapchainKHR old_swapchain;
} NV_GPU_SwapchainCreateInfo;
extern NV_GPU_SwapchainCreateInfo NV_GPU_InitSwapchainCreateInfo();

typedef struct NV_GPU_RenderPassCreateInfo {
  u64 subpass;
  NVFormat format;
  NVFormat depthBufferFormat;

  // Ignored if flags does not contain PIPELINE_CREATE_FLAGS_ENABLE_MULTISAMPLING
  VkSampleCountFlagBits samples;
} NV_GPU_RenderPassCreateInfo;
#define NV_GPU_InitRenderPassCreateInfo()                                                                                                          \
  (NV_GPU_RenderPassCreateInfo) {                                                                                                                  \
    .samples = VK_SAMPLE_COUNT_1_BIT                                                                                                                 \
  }

extern void NV_VK_BakeGlobalPipelines(NVRenderer_t *rd);
extern void NV_VK_DestroyGlobalPipelines();

extern void NV_GPU_CreateGraphicsPipeline(NV_GPU_PipelineCreateInfo const *pCreateInfo, VkPipeline *dstPipeline, u32 flags);
extern void NV_GPU_CreateDepthPipeline(NV_GPU_PipelineCreateInfo const *pCreateInfo, VkPipeline *dstPipeline, u32 flags);
extern void NV_GPU_CreatePipelineLayout(NV_GPU_PipelineCreateInfo const *pCreateInfo, VkPipelineLayout *dstLayout);
extern void NV_GPU_CreateRenderPass(NV_GPU_RenderPassCreateInfo const *pCreateInfo, VkRenderPass *dstRenderPass, u32 flags);
extern void NV_GPU_CreateDepthPass(NV_GPU_RenderPassCreateInfo const *pCreateInfo, VkRenderPass *dstRenderPass, u32 flags);
extern void NV_GPU_CreateSwapchain(NV_GPU_SwapchainCreateInfo const *pCreateInfo, VkSwapchainKHR *dstSwapchain);

NOVA_HEADER_END;

#endif //__NOVA_PIPELINE_H__