#ifndef __PRO__
#define __PRO__

/*

	pro.hpp

	This library does NOT intend to remove the use of vulkan calls. If something does not work, call the functions yourselves.
	It exists only to help vulkan, not to replace it!

*/

#include "vkcbstdafx.hpp"

typedef void (*ResultCheckFunc) (const VkResult result, const char *FILE, const char *FUNC, size_t LINE);
typedef u32 ProFlags;

#define REQUIRED_PTR(ptr) if(ptr == nullptr) LOG_AND_ABORT(#ptr" :  Required parameter '"#ptr"' specified as nullptr.\n", get_filename(__FILE__), __LINE__, __PRETTY_FUNCTION__)
#define NOT_EQUAL_TO(val, to) if(val == to) LOG_AND_ABORT(#val" == "#to". Value "#val" must not be equal to "#to".\n", get_filename(__FILE__), __LINE__, __PRETTY_FUNCTION__)

constexpr u64 tobit(u64 num) { return 1 << num; }

enum PipelineCreateFlagBits
{
	PIPELINE_CREATE_FLAGS_ENABLE_DEPTH_CHECK = tobit(0),
	PIPELINE_CREATE_FLAGS_ENABLE_BLEND = tobit(1),
	PIPELINE_CREATE_FLAGS_ENABLE_CULLING = tobit(2),
	PIPELINE_CREATE_FLAGS_DISABLE_DYNAMIC_VIEWPORT = tobit(3),

	// IMPLEMENT
	PIPELINE_CREATE_FLAGS_ENABLE_MULTISAMPLING = tobit(4),
};
typedef ProFlags PipelineCreateFlags;

#define ResultCheck(func) pro::__resultFunc(func, get_filename(__FILE__), __PRETTY_FUNCTION__, __LINE__)

namespace pro
{
	inline void __defaultResultCheckFunc(const VkResult result, const char *FILE, const char *FUNC, size_t LINE) {
		(result != VK_SUCCESS) ? LOG_ERROR("In file %s:\n        In function %s at line %lu:\n        Function returned error code %i", FILE, FUNC, LINE, result) : void(0);
	}
	
	/*
		Used internally. Do NOT modify by yourselves! Use SetResultCheckFunc instead.
	*/
	static ResultCheckFunc __resultFunc = __defaultResultCheckFunc; // To not cause nullptr dereference. SetResultCheckFunc also checks for nullptr and handles it.

	/*
		Set the result checking function for the API. This is called every time the program requests something in the order of vkCreate* that this namespace has a hold of.
		Use nullptr to deattach the function.
	*/
	inline void SetResultCheckFunc(ResultCheckFunc func)
	{
		if (func != nullptr)
			__resultFunc = func;
		else
			func = __defaultResultCheckFunc;
	}

	/*
	*	FORWARD DECLARATIONS
	*/
	struct PipelineCreateInfo;
	struct SwapchainCreateInfo;
	struct RenderPassCreateInfo;
	struct Pipeline;
	struct PipelineBlendState;

	enum BlendPreset
	{
		PRO_BLEND_PRESET_NONE                  = 0,
		PRO_BLEND_PRESET_ALPHA                 = 1,
		PRO_BLEND_PRESET_ADDITIVE              = 2,
		PRO_BLEND_PRESET_MULTIPLICATIVE        = 3,
		PRO_BLEND_PRESET_PREMULTIPLIED_ALPHA   = 4,
		PRO_BLEND_PRESET_SUBTRACTIVE           = 5,
		PRO_BLEND_PRESET_SCREEN                = 6,
	};

	struct PipelineBlendState
	{
		VkBlendFactor            srcColorBlendFactor;
		VkBlendFactor            dstColorBlendFactor;
		VkBlendOp                colorBlendOp;
		VkBlendFactor            srcAlphaBlendFactor;
		VkBlendFactor            dstAlphaBlendFactor;
		VkBlendOp                alphaBlendOp;
		VkColorComponentFlags    colorWriteMask;

		PipelineBlendState(BlendPreset ePreset);
		PipelineBlendState() = default;
		~PipelineBlendState() = default;
	};

	struct PipelineCreateInfo
	{
		VkRenderPass 	 	renderPass = VK_NULL_HANDLE;
		VkPipelineLayout 	pipelineLayout = VK_NULL_HANDLE;
		VkExtent2D 		 	extent = {0, 0};
		VkFormat 		 	format = VK_FORMAT_UNDEFINED;
		u64 			 	subpass = 0;
		VkPipeline 		 	oldPipeline = VK_NULL_HANDLE;
		VkPipelineCache  	cache = VK_NULL_HANDLE;
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		/* EXTENSIONS */

		// Ignored if flags does not contain PIPELINE_CREATE_FLAGS_ENABLE_MULTISAMPLING
		VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;

		// Ignored if flags does not contain PIPELINE_CREATE_FLAGS_ENABLE_BLEND
		const PipelineBlendState* pBlendState = nullptr;

		/*
		*	Array pointers are allowed to be nullptr
		*/
		const cvector<VkVertexInputAttributeDescription>* pAttributeDescriptions = nullptr;
		const cvector<VkVertexInputBindingDescription>* pBindingDescriptions = nullptr;
		const cvector<VkDescriptorSetLayout>* pDescriptorLayouts = nullptr;
		const cvector<VkPushConstantRange>* pPushConstants = nullptr;
		const cvector<VkPipelineShaderStageCreateInfo>* pShaderCreateInfos = nullptr;

		PipelineCreateInfo() = default;
		~PipelineCreateInfo() = default;
	};

	struct SwapchainCreateInfo
	{
		VkPhysicalDevice physDevice = VK_NULL_HANDLE;
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkExtent2D extent = {0, 0};
		VkPresentModeKHR ePresentMode = VK_PRESENT_MODE_FIFO_KHR;
		u64 requestedImageCount = 0;
		VkFormat format = VK_FORMAT_UNDEFINED;
		VkColorSpaceKHR eColorSpace = VK_COLOR_SPACE_MAX_ENUM_KHR;
		VkSwapchainKHR pOldSwapchain = VK_NULL_HANDLE;

		SwapchainCreateInfo() = default;
		~SwapchainCreateInfo() = default;
	};

	struct RenderPassCreateInfo
	{
		u64 subpass = 0;
		VkFormat format = VK_FORMAT_UNDEFINED;
		VkFormat depthBufferFormat = VK_FORMAT_UNDEFINED;

		// Ignored if flags does not contain PIPELINE_CREATE_FLAGS_ENABLE_MULTISAMPLING
		VkSampleCountFlagBits samples;

		RenderPassCreateInfo() = default;
		~RenderPassCreateInfo() = default;
	};

	struct Pipeline
	{
		VkShaderModule vertexShader = VK_NULL_HANDLE;
		VkShaderModule fragmentShader = VK_NULL_HANDLE;
		VkShaderModule computeShader = VK_NULL_HANDLE;
		VkPipeline pipeline = VK_NULL_HANDLE;
		VkPipelineLayout layout = VK_NULL_HANDLE;
		VkRenderPass renderPass = VK_NULL_HANDLE;

		inline operator VkPipeline() { return pipeline; }

		Pipeline() = default;
		~Pipeline() = default;
	};

	void CreateGraphicsPipeline(VkDevice device, PipelineCreateInfo const* pCreateInfo, VkPipeline* dstPipeline, u32 flags);
	void CreateGraphicsPipeline(VkDevice device, PipelineCreateInfo const* pCreateInfo, Pipeline* dstPipeline, u32 flags);
	void CreatePipelineLayout(VkDevice device, PipelineCreateInfo const* pCreateInfo, VkPipelineLayout* dstLayout);
	void CreatePipelineLayout(VkDevice device, PipelineCreateInfo const* pCreateInfo, Pipeline* dstPipeline);
	void CreateRenderPass(VkDevice device, RenderPassCreateInfo const* pCreateInfo, VkRenderPass* dstRenderPass, u32 flags);
	void CreateRenderPass(VkDevice device, PipelineCreateInfo const* pCreateInfo, Pipeline* dstPipeline, u32 flags);

	void CreateSwapChain(VkDevice device, SwapchainCreateInfo const* pCreateInfo, VkSwapchainKHR* dstSwapchain);
}

#endif