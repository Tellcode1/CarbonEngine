#ifndef __CVK_H
#define __CVK_H

#ifdef __cplusplus
	extern "C" {
#endif

#include "../external/volk/volk.h"

#include "defines.h"
#include "lunaGFX.h"
#include "vkstdafx.h"

struct csm_shader_t;

typedef void (*luna_GPU_ResultCheckFn) (const VkResult result, const char *__restrict__ FILE, const char *__restrict__ FUNC, unsigned long LINE);

#define CVK_REQUIRED_PTR(ptr) if((ptr) == NULL) LOG_AND_ABORT(#ptr" :  Required parameter \""#ptr"\" specified as NULL.", __basename(__FILE__), __LINE__, __PRETTY_FUNCTION__)
#define CVK_NOT_EQUAL_TO(val, to) if((val) == (to)) LOG_AND_ABORT(#val" == "#to". Value \""#val"\" must not be equal to "#to".", __basename(__FILE__), __LINE__, __PRETTY_FUNCTION__)

#define __cvk_to_bit(n) (1 << n)

typedef enum cvk_pipeline_flags_bits
{
	CVK_PIPELINE_FLAGS_FORCE_DEPTH_CHECK 		= __cvk_to_bit(0),
	CVK_PIPELINE_FLAGS_UNFORCE_DEPTH_CHECK 		= ~CVK_PIPELINE_FLAGS_FORCE_DEPTH_CHECK,
	CVK_PIPELINE_FLAGS_FORCE_CULLING            = __cvk_to_bit(1),
	CVK_PIPELINE_FLAGS_UNFORCE_CULLING          = ~CVK_PIPELINE_FLAGS_FORCE_CULLING, // Disables culling for resulting pipeline
	CVK_PIPELINE_FLAGS_FORCE_DYNAMIC_VIEWPORT   = __cvk_to_bit(2),
	CVK_PIPELINE_FLAGS_UNFORCE_DYNAMIC_VIEWPORT = ~CVK_PIPELINE_FLAGS_FORCE_DYNAMIC_VIEWPORT,
	CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING      = __cvk_to_bit(3),
	CVK_PIPELINE_FLAGS_UNFORCE_MULTISAMPLING    = ~CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING
} cvk_pipeline_flags_bits;
typedef u32 cvk_pipeline_flags;

#define CVK_ResultCheck(func) __cvk_resultFunc(func, __basename(__FILE__), __PRETTY_FUNCTION__, __LINE__)

static void __cvk_defaultResultCheckFunc(const VkResult result, const char *FILE, const char *FUNC, unsigned long LINE) {
	if (result != VK_SUCCESS) LOG_ERROR("[%s:%lu] %s: func returned %i", FILE, LINE, FUNC, result);
}

/*
	Used internally. Do NOT modify by yourselves! Use SetResultCheckFunc instead.
*/
extern luna_GPU_ResultCheckFn __cvk_resultFunc;

/*
	Set the result checking function for the API. This is called every time the program requests something in the order of vkCreate* that this namespace has a hold of.
	Use NULL to deattach the function.
*/
static inline void luna_GPU_SetResultCheckFn(luna_GPU_ResultCheckFn func)
{
	if (func != NULL)
		__cvk_resultFunc = func;
	else
		func = __cvk_defaultResultCheckFunc;
}

extern u32 luna_GPU_vk_flag_register;

/*
*	FORWARD DECLARATIONS
*/
typedef struct luna_VK_Pipeline luna_VK_Pipeline;
typedef struct luna_GPU_PipelineCreateInfo luna_GPU_PipelineCreateInfo;
typedef struct luna_GPU_SwapchainCreateInfo luna_GPU_SwapchainCreateInfo;
typedef struct luna_GPU_RenderPassCreateInfo luna_GPU_RenderPassCreateInfo;
typedef struct luna_GPU_PipelineBlendState luna_GPU_PipelineBlendState;

#define LUNA_VK_MAX_SHADERS_PER_PIPELINE 8

typedef struct luna_VK_Pipeline {
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;

	// The common descriptor set layout.
    VkDescriptorSetLayout descriptor_layout;
} luna_VK_Pipeline;

typedef struct luna_BakedPipelines {
	luna_VK_Pipeline Unlit;
	luna_VK_Pipeline Lit;
	luna_VK_Pipeline Ctext;
	luna_VK_Pipeline Line; // Draws lines. Yep.
} luna_BakedPipelines;
extern luna_BakedPipelines g_Pipelines;

typedef enum luna_GPU_PipelineBlendPreset
{
	CVK_BLEND_PRESET_NONE                  = 0,
	CVK_BLEND_PRESET_ALPHA                 = 1,
	CVK_BLEND_PRESET_ADDITIVE              = 2,
	CVK_BLEND_PRESET_MULTIPLICATIVE        = 3,
	CVK_BLEND_PRESET_PREMULTIPLIED_ALPHA   = 4,
	CVK_BLEND_PRESET_SUBTRACTIVE           = 5,
	CVK_BLEND_PRESET_SCREEN                = 6,
} luna_GPU_PipelineBlendPreset;

typedef struct luna_GPU_PipelineBlendState
{
	VkBlendFactor            srcColorBlendFactor;
	VkBlendFactor            dstColorBlendFactor;
	VkBlendOp                colorBlendOp;
	VkBlendFactor            srcAlphaBlendFactor;
	VkBlendFactor            dstAlphaBlendFactor;
	VkBlendOp                alphaBlendOp;
	VkColorComponentFlags    colorWriteMask;
} luna_GPU_PipelineBlendState;
extern luna_GPU_PipelineBlendState luna_GPU_InitPipelineBlendState(luna_GPU_PipelineBlendPreset preset);

typedef struct luna_GPU_PipelineCreateInfo
{
	VkRenderPass 	 	render_pass;
	VkPipelineLayout 	pipeline_layout;
	VkExtent2D 		 	extent;
	lunaFormat 		 	format;
	u64 			 	subpass;
	VkPipeline 		 	old_pipeline;
	VkPipelineCache  	cache;
	VkPrimitiveTopology topology;

	/* EXTENSIONS */

	// Ignored if flags does not contain PIPELINE_CREATE_FLAGS_ENABLE_MULTISAMPLING
	VkSampleCountFlagBits samples;

	// Ignored if flags does not contain PIPELINE_CREATE_FLAGS_ENABLE_BLEND
	const luna_GPU_PipelineBlendState* blend_state;

	//	Array pointers are allowed to be NULL
	const VkVertexInputAttributeDescription *pAttributeDescriptions;
	const VkVertexInputBindingDescription *pBindingDescriptions;
	const VkDescriptorSetLayout *pDescriptorLayouts;
	const VkPushConstantRange *pPushConstants;
	const struct csm_shader_t * const *pShaders;

	int nAttributeDescriptions;
	int nBindingDescriptions;
	int nDescriptorLayouts;
	int nPushConstants;
	int nShaders;
} luna_GPU_PipelineCreateInfo;
#define luna_GPU_InitPipelineCreateInfo() (luna_GPU_PipelineCreateInfo){.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, .samples = VK_SAMPLE_COUNT_1_BIT}

typedef struct luna_GPU_SwapchainCreateInfo
{
	VkExtent2D extent;
	VkPresentModeKHR present_mode;
	u64 image_count;
	lunaFormat format;
	VkColorSpaceKHR color_space;
	VkSwapchainKHR old_swapchain;
} luna_GPU_SwapchainCreateInfo;
extern luna_GPU_SwapchainCreateInfo luna_GPU_InitSwapchainCreateInfo();

typedef struct luna_GPU_RenderPassCreateInfo
{
	u64 subpass;
	lunaFormat format;
	lunaFormat depthBufferFormat;

	// Ignored if flags does not contain PIPELINE_CREATE_FLAGS_ENABLE_MULTISAMPLING
	VkSampleCountFlagBits samples;
} luna_GPU_RenderPassCreateInfo;
#define luna_GPU_InitRenderPassCreateInfo() (luna_GPU_RenderPassCreateInfo){ .samples = VK_SAMPLE_COUNT_1_BIT}

extern void luna_VK_BakeGlobalPipelines( luna_Renderer_t *rd );

extern void luna_GPU_CreateGraphicsPipeline(luna_GPU_PipelineCreateInfo const* pCreateInfo, VkPipeline* dstPipeline, u32 flags);
extern void luna_GPU_CreateDepthPipeline(luna_GPU_PipelineCreateInfo const* pCreateInfo, VkPipeline* dstPipeline, u32 flags);
extern void luna_GPU_CreatePipelineLayout(luna_GPU_PipelineCreateInfo const* pCreateInfo, VkPipelineLayout* dstLayout);
extern void luna_GPU_CreateRenderPass(luna_GPU_RenderPassCreateInfo const* pCreateInfo, VkRenderPass* dstRenderPass, u32 flags);
extern void luna_GPU_CreateDepthPass(luna_GPU_RenderPassCreateInfo const* pCreateInfo, VkRenderPass* dstRenderPass, u32 flags);
extern void luna_GPU_CreateSwapchain(luna_GPU_SwapchainCreateInfo const* pCreateInfo, VkSwapchainKHR* dstSwapchain);

#ifdef __cplusplus
	}
#endif

#endif//__CVK_H