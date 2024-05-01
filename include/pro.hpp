#ifndef __PRO__
#define __PRO__

#include "stdafx.hpp"

typedef void (*ResultCheckFunc) (const VkResult);
constexpr u64 tobit(u64 num) { return 1 << num; }

__attribute__((unused)) static void __print_and_abort(const char *msg, const char *file, unsigned int line, const char *function) noexcept(true) {
	printf("%s:%u: %s: %s\n", file, line, function, msg);
	printf("Fatal error. Aborting.\n");
	abort();
}

#ifndef PRO_DISABLE_ERROR_CHECKING
	#define REQUIRED_PTR(ptr) if(ptr == nullptr) __print_and_abort(#ptr" :  Required parameter '"#ptr"' specified as nullptr.", __FILE__, __LINE__, __PRETTY_FUNCTION__)
	#define NOT_EQUAL_TO(val, to) if(val == to) __print_and_abort(#val" == "#to". Value "#val" must not be equal to "#to, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#else
	#define REQUIRED_PTR(ptr)
	#define NOT_EQUAL_TO(val, to)
#endif

#ifndef PRO_ARRAY_TYPE
	#include <safearray.hpp>
	#define PRO_ARRAY_TYPE std::vector
#endif


typedef u32 ProFlags;

typedef enum PipelineCreateFlagBits
{
	PIPELINE_CREATE_FLAGS_ENABLE_DEPTH_CHECK = tobit(0),
	PIPELINE_CREATE_FLAGS_ENABLE_BLEND = tobit(1),
	PIPELINE_CREATE_FLAGS_ENABLE_CULLING = tobit(2),

	// IMPLEMENT
	PIPELINE_CREATE_FLAGS_ENABLE_MULTISAMPLING = tobit(3),
	PIPELINE_CREATE_FLAGS_DYNAMIC_VIEWPORT = tobit(4),
	PIPELINE_CREATE_FLAGS_DYNAMIC_SCISSOR = tobit(5),
} PipelineCreateFlagBits;

typedef ProFlags PipelineCreateFlags;

namespace pro
{
	inline void __defaultResultCheckFunc(const VkResult _) { return; }
	
	/*
		Used internally. Do NOT modify by yourselves! Use SetResultCheckFunc instead.
	*/
	static ResultCheckFunc __resultFunc = __defaultResultCheckFunc; // To not cause nullptr dereference. SetResultCheckFunc also checks for nullptr and handles it.
	inline void ResultCheck(VkResult result) { __resultFunc(result); }

	/*
		Set the result checking function for the API. This is called every time the program requests something in the order of vkCreate* that this namespace has a hold of.
		Use nullptr to deattach the function.
	*/
	inline void SetResultCheckFunc(ResultCheckFunc func)
	{
		__resultFunc = (func != nullptr) ? func : __defaultResultCheckFunc;
	}

	/*
	*	FORWARD DECLARATIONS
	*/
	struct PipelineCreateInfo;
	struct SwapchainCreateInfo;
	struct FramebufferCreateInfo;
	struct BufferCreateInfo;
	struct RenderPassCreateInfo;
	struct Pipeline;
	struct Buffer;

	#ifdef LEGACY
	struct PipelineLayoutCreateInfo;
	#endif

	struct PipelineCreateInfo
	{
		VkRenderPass renderPass = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkExtent2D extent = {0, 0};
		VkFormat format = VK_FORMAT_UNDEFINED;
		u64 subpass = 0;
		VkPipeline oldPipeline = VK_NULL_HANDLE;
		VkPipelineCache cache = VK_NULL_HANDLE;
		
		// Ignored if flags does not contain PIPELINE_CREATE_FLAGS_ENABLE_DEPTH_CHECK
		// VkFormat depthBufferFormat;
		// Ignored if flags does not contain PIPELINE_CREATE_FLAGS_ENABLE_MULTISAMPLING
		VkSampleCountFlagBits samples;

		/*
		*	Array pointers are allowed to be nullptr
		*/
		const PRO_ARRAY_TYPE<VkVertexInputAttributeDescription>* pAttributeDescriptions = nullptr;
		const PRO_ARRAY_TYPE<VkVertexInputBindingDescription>* pBindingDescriptions = nullptr;
		const PRO_ARRAY_TYPE<VkDescriptorSetLayout>* pDescriptorLayouts = nullptr;
		const PRO_ARRAY_TYPE<VkPushConstantRange>* pPushConstants = nullptr;
		const PRO_ARRAY_TYPE<VkPipelineShaderStageCreateInfo>* pShaderCreateInfos = nullptr;

		PipelineCreateInfo() = default;
		~PipelineCreateInfo() = default;
	};

	#ifdef LEGACY
	struct PipelineLayoutCreateInfo
	{
		const SafeArray<VkVertexInputAttributeDescription>* pAttributeDescriptions = VK_NULL_HANDLE;
		const SafeArray<VkVertexInputBindingDescription>* pBindingDescription = VK_NULL_HANDLE;
		const SafeArray<VkDescriptorSetLayout>* pDescriptorLayouts = VK_NULL_HANDLE;
		const SafeArray<VkPushConstantRange>* pPushConstants = VK_NULL_HANDLE;

		PipelineLayoutCreateInfo() = default;
		~PipelineLayoutCreateInfo() = default;
	};

	struct SwapchainCreateInfo
	{
		VkSurfaceKHR surface;
		VkFormat format = VK_FORMAT_UNDEFINED;
		VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_MAX_ENUM_KHR;
		VkExtent2D extent = {0, 0};
		uint32_t imageCount = 0;
		VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
		VkSurfaceTransformFlagBitsKHR preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		VkSwapchainKHR pOldSwapchain = VK_NULL_HANDLE;

		SwapchainCreateInfo() = default;
		~SwapchainCreateInfo() = default;
	};
	#endif

	struct SwapchainCreateInfo
	{
		VkPhysicalDevice physDevice = VK_NULL_HANDLE;
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkExtent2D extent = {0, 0};
		VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
		u64 requestedImageCount = 0;
		VkFormat format = VK_FORMAT_UNDEFINED;
		VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_MAX_ENUM_KHR;
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

	void CreateEntirePipeline(VkDevice device, PipelineCreateInfo* pCreateInfo, Pipeline* dstPipeline, PipelineCreateFlags flags);

	void CreateGraphicsPipeline(VkDevice device, PipelineCreateInfo const* pCreateInfo, VkPipeline* dstPipeline, u32 flags);
	void CreateGraphicsPipeline(VkDevice device, PipelineCreateInfo const* pCreateInfo, Pipeline* dstPipeline, u32 flags);
	void CreatePipelineLayout(VkDevice device, PipelineCreateInfo const* pCreateInfo, Pipeline* dstPipeline);
	void CreateRenderPass(VkDevice device, RenderPassCreateInfo const* pCreateInfo, VkRenderPass* dstRenderPass, u32 flags);
	void CreateRenderPass(VkDevice device, PipelineCreateInfo const* pCreateInfo, Pipeline* dstPipeline, u32 flags);

	void CreateSwapChain(VkDevice device, SwapchainCreateInfo const* pCreateInfo, VkSwapchainKHR* dstSwapchain);

	// HELPER FUNCTIONS
	bool GetSupportedFormat(VkDevice device, VkPhysicalDevice physDevice, VkSurfaceKHR surface, VkFormat* dstFormat, VkColorSpaceKHR* dstColorSpace);
	u32 GetImageCount(VkPhysicalDevice physDevice, VkSurfaceKHR surface);
	u32 GetMemoryType(VkPhysicalDevice physDevice, const u32 memoryTypeBits, const VkMemoryPropertyFlags properties);
}

#endif