#ifndef __CVK_HPP__
#define __CVK_HPP__

#include "../external/volk/volk.h"

#include "defines.h"
#include "vkstdafx.h"

struct csm_shader_t;

typedef void (*cvk_result_check_fn) (const VkResult result, const char *__restrict__ FILE, const char *__restrict__ FUNC, unsigned long LINE);

#define CVK_REQUIRED_PTR(ptr) if(ptr == nullptr) LOG_AND_ABORT(#ptr" :  Required parameter \""#ptr"\" specified as nullptr.\n", __basename(__FILE__), __LINE__, __PRETTY_FUNCTION__)
#define CVK_NOT_EQUAL_TO(val, to) if(val == to) LOG_AND_ABORT(#val" == "#to". Value \""#val"\" must not be equal to "#to".\n", __basename(__FILE__), __LINE__, __PRETTY_FUNCTION__)

#define __cvk_to_bit(n) (1 << n)

enum cvk_pipeline_flags_bits
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
};
typedef u32 cvk_pipeline_flags;

#define CVK_ResultCheck(func) __cvk_resultFunc(func, __basename(__FILE__), __PRETTY_FUNCTION__, __LINE__)

inline void __cvk_defaultResultCheckFunc(const VkResult result, const char *FILE, const char *FUNC, unsigned long LINE) {
	(result != VK_SUCCESS) ? LOG_ERROR("In file %s:\n        In function %s at line %lu:\n        Function returned error code %i", FILE, FUNC, LINE, result) : void(0);
}

/*
	Used internally. Do NOT modify by yourselves! Use SetResultCheckFunc instead.
*/
extern cvk_result_check_fn __cvk_resultFunc;

/*
	Set the result checking function for the API. This is called every time the program requests something in the order of vkCreate* that this namespace has a hold of.
	Use nullptr to deattach the function.
*/
inline void cvk_set_result_check_fn(cvk_result_check_fn func)
{
	if (func != nullptr)
		__cvk_resultFunc = func;
	else
		func = __cvk_defaultResultCheckFunc;
}

extern u32 cvk_flag_register;

/*
*	FORWARD DECLARATIONS
*/
struct cvk_pipeline_create_info;
struct cvk_swapchain_create_info;
struct cvk_render_pass_create_info;
struct cvk_pipeline_blend_state;

enum cvk_blend_preset
{
	CVK_BLEND_PRESET_NONE                  = 0,
	CVK_BLEND_PRESET_ALPHA                 = 1,
	CVK_BLEND_PRESET_ADDITIVE              = 2,
	CVK_BLEND_PRESET_MULTIPLICATIVE        = 3,
	CVK_BLEND_PRESET_PREMULTIPLIED_ALPHA   = 4,
	CVK_BLEND_PRESET_SUBTRACTIVE           = 5,
	CVK_BLEND_PRESET_SCREEN                = 6,
};

struct cvk_pipeline_blend_state
{
	VkBlendFactor            srcColorBlendFactor;
	VkBlendFactor            dstColorBlendFactor;
	VkBlendOp                colorBlendOp;
	VkBlendFactor            srcAlphaBlendFactor;
	VkBlendFactor            dstAlphaBlendFactor;
	VkBlendOp                alphaBlendOp;
	VkColorComponentFlags    colorWriteMask;

	cvk_pipeline_blend_state(cvk_blend_preset ePreset);
	cvk_pipeline_blend_state() = default;
	~cvk_pipeline_blend_state() = default;
};

struct cvk_pipeline_create_info
{
	VkRenderPass 	 	render_pass = VK_NULL_HANDLE;
	VkPipelineLayout 	pipeline_layout = VK_NULL_HANDLE;
	VkExtent2D 		 	extent = {0, 0};
	VkFormat 		 	format = VK_FORMAT_UNDEFINED;
	u64 			 	subpass = 0;
	VkPipeline 		 	old_pipeline = VK_NULL_HANDLE;
	VkPipelineCache  	cache = VK_NULL_HANDLE;
	VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	/* EXTENSIONS */

	// Ignored if flags does not contain PIPELINE_CREATE_FLAGS_ENABLE_MULTISAMPLING
	VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;

	// Ignored if flags does not contain PIPELINE_CREATE_FLAGS_ENABLE_BLEND
	const cvk_pipeline_blend_state* blend_state = nullptr;

	/*
	*	Array pointers are allowed to be nullptr
	*/
	const VkVertexInputAttributeDescription *pAttributeDescriptions;
	const VkVertexInputBindingDescription *pBindingDescriptions;
	const VkDescriptorSetLayout *pDescriptorLayouts;
	const VkPushConstantRange *pPushConstants;
	const csm_shader_t * const *pShaders;

	int nAttributeDescriptions;
	int nBindingDescriptions;
	int nDescriptorLayouts;
	int nPushConstants;
	int nShaders;

	cvk_pipeline_create_info() = default;
	~cvk_pipeline_create_info() = default;
};

struct cvk_swapchain_create_info
{
	VkExtent2D extent = {0, 0};
	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
	u64 image_count = 0;
	VkFormat format = VK_FORMAT_UNDEFINED;
	VkColorSpaceKHR color_space = VK_COLOR_SPACE_MAX_ENUM_KHR;
	VkSwapchainKHR old_swapchain = VK_NULL_HANDLE;

	cvk_swapchain_create_info() = default;
	~cvk_swapchain_create_info() = default;
};

struct cvk_render_pass_create_info
{
	u64 subpass = 0;
	VkFormat format = VK_FORMAT_UNDEFINED;
	VkFormat depthBufferFormat = VK_FORMAT_UNDEFINED;

	// Ignored if flags does not contain PIPELINE_CREATE_FLAGS_ENABLE_MULTISAMPLING
	VkSampleCountFlagBits samples;

	cvk_render_pass_create_info() = default;
	~cvk_render_pass_create_info() = default;
};

void cvk_create_graphics_pipeline(VkDevice device, cvk_pipeline_create_info const* pCreateInfo, VkPipeline* dstPipeline, u32 flags);
void cvk_create_depth_pipeline(VkDevice device, cvk_pipeline_create_info const* pCreateInfo, VkPipeline* dstPipeline, u32 flags);
void cvk_create_pipeline_layout(VkDevice device, cvk_pipeline_create_info const* pCreateInfo, VkPipelineLayout* dstLayout);
void cvk_create_render_pass(VkDevice device, cvk_render_pass_create_info const* pCreateInfo, VkRenderPass* dstRenderPass, u32 flags);
void cvk_create_depth_pass(VkDevice device, cvk_render_pass_create_info const* pCreateInfo, VkRenderPass* dstRenderPass, u32 flags);
void cvk_create_swapchain(VkDevice device, cvk_swapchain_create_info const* pCreateInfo, VkSwapchainKHR* dstSwapchain);

#endif//__CVK_HPP__