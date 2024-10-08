#ifndef __CVK_H
#define __CVK_H

#ifdef __cplusplus
	extern "C" {
#endif

#include "../external/volk/volk.h"

#include "defines.h"
#include "vkstdafx.h"

struct csm_shader_t;

typedef void (*cvk_result_check_fn) (const VkResult result, const char *__restrict__ FILE, const char *__restrict__ FUNC, unsigned long LINE);

#define CVK_REQUIRED_PTR(ptr) if((ptr) == NULL) LOG_AND_ABORT(#ptr" :  Required parameter \""#ptr"\" specified as NULL.\n", __basename(__FILE__), __LINE__, __PRETTY_FUNCTION__)
#define CVK_NOT_EQUAL_TO(val, to) if((val) == (to)) LOG_AND_ABORT(#val" == "#to". Value \""#val"\" must not be equal to "#to".\n", __basename(__FILE__), __LINE__, __PRETTY_FUNCTION__)

#define __cvk_to_bit(n) (1 << n)

typedef enum cvk_pipeline_flags_bits
{
	CVK_PIPELINE_FLAGS_FORCE_DEPTH_CHECK 		= __cvk_to_bit(0),
	CVK_PIPELINE_FLAGS_UNFORCE_DEPTH_CHECK 		= __cvk_to_bit(1),
	CVK_PIPELINE_FLAGS_ENABLE_BLEND             = __cvk_to_bit(2),
	CVK_PIPELINE_FLAGS_FORCE_CULLING            = __cvk_to_bit(3),
	CVK_PIPELINE_FLAGS_UNFORCE_CULLING          = __cvk_to_bit(4), // Disables culling for resulting pipeline
	CVK_PIPELINE_FLAGS_FORCE_DYNAMIC_VIEWPORT   = __cvk_to_bit(6),
	CVK_PIPELINE_FLAGS_UNFORCE_DYNAMIC_VIEWPORT = __cvk_to_bit(7),
	CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING      = __cvk_to_bit(8),
	CVK_PIPELINE_FLAGS_UNFORCE_MULTISAMPLING    = __cvk_to_bit(9)
} cvk_pipeline_flags_bits;
typedef u32 cvk_pipeline_flags;

#define CVK_ResultCheck(func) __cvk_resultFunc(func, __basename(__FILE__), __PRETTY_FUNCTION__, __LINE__)

static void __cvk_defaultResultCheckFunc(const VkResult result, const char *FILE, const char *FUNC, unsigned long LINE) {
	if (result != VK_SUCCESS) LOG_ERROR("In file %s:\n        In function %s at line %lu:\n        Function returned error code %i", FILE, FUNC, LINE, result);
}

/*
	Used internally. Do NOT modify by yourselves! Use SetResultCheckFunc instead.
*/
extern cvk_result_check_fn __cvk_resultFunc;

/*
	Set the result checking function for the API. This is called every time the program requests something in the order of vkCreate* that this namespace has a hold of.
	Use NULL to deattach the function.
*/
static inline void cvk_set_result_check_fn(cvk_result_check_fn func)
{
	if (func != NULL)
		__cvk_resultFunc = func;
	else
		func = __cvk_defaultResultCheckFunc;
}

extern u32 cvk_flag_register;

/*
*	FORWARD DECLARATIONS
*/
typedef struct cvk_pipeline_create_info cvk_pipeline_create_info;
typedef struct cvk_swapchain_create_info cvk_swapchain_create_info;
typedef struct cvk_render_pass_create_info cvk_render_pass_create_info;
typedef struct cvk_pipeline_blend_state cvk_pipeline_blend_state;

typedef enum cvk_blend_preset
{
	CVK_BLEND_PRESET_NONE                  = 0,
	CVK_BLEND_PRESET_ALPHA                 = 1,
	CVK_BLEND_PRESET_ADDITIVE              = 2,
	CVK_BLEND_PRESET_MULTIPLICATIVE        = 3,
	CVK_BLEND_PRESET_PREMULTIPLIED_ALPHA   = 4,
	CVK_BLEND_PRESET_SUBTRACTIVE           = 5,
	CVK_BLEND_PRESET_SCREEN                = 6,
} cvk_blend_preset;

typedef struct cvk_pipeline_blend_state
{
	VkBlendFactor            srcColorBlendFactor;
	VkBlendFactor            dstColorBlendFactor;
	VkBlendOp                colorBlendOp;
	VkBlendFactor            srcAlphaBlendFactor;
	VkBlendFactor            dstAlphaBlendFactor;
	VkBlendOp                alphaBlendOp;
	VkColorComponentFlags    colorWriteMask;
} cvk_pipeline_blend_state;
extern cvk_pipeline_blend_state cvk_init_pipeline_blend_state(cvk_blend_preset preset);

typedef struct cvk_pipeline_create_info
{
	VkRenderPass 	 	render_pass;
	VkPipelineLayout 	pipeline_layout;
	VkExtent2D 		 	extent;
	VkFormat 		 	format;
	u64 			 	subpass;
	VkPipeline 		 	old_pipeline;
	VkPipelineCache  	cache;
	VkPrimitiveTopology topology;

	/* EXTENSIONS */

	// Ignored if flags does not contain PIPELINE_CREATE_FLAGS_ENABLE_MULTISAMPLING
	VkSampleCountFlagBits samples;

	// Ignored if flags does not contain PIPELINE_CREATE_FLAGS_ENABLE_BLEND
	const cvk_pipeline_blend_state* blend_state;

	/*
	*	Array pointers are allowed to be NULL
	*/
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
} cvk_pipeline_create_info;
#define cvk_init_pipeline_create_info() (cvk_pipeline_create_info){.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, .samples = VK_SAMPLE_COUNT_1_BIT}

typedef struct cvk_swapchain_create_info
{
	VkExtent2D extent;
	VkPresentModeKHR present_mode;
	u64 image_count;
	VkFormat format;
	VkColorSpaceKHR color_space;
	VkSwapchainKHR old_swapchain;
} cvk_swapchain_create_info;
extern cvk_swapchain_create_info cvk_init_swapchain_create_info();

typedef struct cvk_render_pass_create_info
{
	u64 subpass;
	VkFormat format;
	VkFormat depthBufferFormat;

	// Ignored if flags does not contain PIPELINE_CREATE_FLAGS_ENABLE_MULTISAMPLING
	VkSampleCountFlagBits samples;
} cvk_render_pass_create_info;
#define cvk_init_render_pass_create_info() (cvk_render_pass_create_info){ .samples = VK_SAMPLE_COUNT_1_BIT}

extern void cvk_create_graphics_pipeline(VkDevice device, cvk_pipeline_create_info const* pCreateInfo, VkPipeline* dstPipeline, u32 flags);
extern void cvk_create_depth_pipeline(VkDevice device, cvk_pipeline_create_info const* pCreateInfo, VkPipeline* dstPipeline, u32 flags);
extern void cvk_create_pipeline_layout(VkDevice device, cvk_pipeline_create_info const* pCreateInfo, VkPipelineLayout* dstLayout);
extern void cvk_create_render_pass(VkDevice device, cvk_render_pass_create_info const* pCreateInfo, VkRenderPass* dstRenderPass, u32 flags);
extern void cvk_create_depth_pass(VkDevice device, cvk_render_pass_create_info const* pCreateInfo, VkRenderPass* dstRenderPass, u32 flags);
extern void cvk_create_swapchain(VkDevice device, cvk_swapchain_create_info const* pCreateInfo, VkSwapchainKHR* dstSwapchain);

#ifdef __cplusplus
	}
#endif

#endif//__CVK_H