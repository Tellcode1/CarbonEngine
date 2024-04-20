#include "pro.hpp"

typedef unsigned  u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef float f32;
typedef double f64;

// arrptr == pointer to array
// These defines serve the core purpose of not causing nullptr dereferences because of dereferencing arrays.
#define FALLBACK_SIZE(arrptr) (arrptr != VK_NULL_HANDLE) ? arrptr->size() : 0
#define FALLBACK_PTR(arrptr) (arrptr != VK_NULL_HANDLE) ? arrptr->data() : VK_NULL_HANDLE

void pro::CreateEntirePipeline(const VkDevice device, PipelineCreateInfo* pCreateInfo, Pipeline* dstPipeline, u32 flags)
{
	// Error checking is already done by the individual functions so no need to do it here.

	CreateRenderPass(device, pCreateInfo, dstPipeline, flags);
	pCreateInfo->renderPass = dstPipeline->renderPass;

	CreatePipelineLayout(device, pCreateInfo, dstPipeline);
	pCreateInfo->pipelineLayout = dstPipeline->layout;

	CreateGraphicsPipeline(device, pCreateInfo, dstPipeline, flags);
}

void pro::CreateGraphicsPipeline(VkDevice device, PipelineCreateInfo const* pCreateInfo, Pipeline* dstPipeline, u32 flags)
{
	REQUIRED_PTR(device);
	REQUIRED_PTR(pCreateInfo);
	REQUIRED_PTR(dstPipeline);
	REQUIRED_PTR(pCreateInfo->pShaderCreateInfos);
	REQUIRED_PTR(pCreateInfo->renderPass);
	NOT_EQUAL_TO(pCreateInfo->format, VK_FORMAT_UNDEFINED);
	NOT_EQUAL_TO(pCreateInfo->extent.width, 0);
	NOT_EQUAL_TO(pCreateInfo->extent.height, 0);

	// const bool cacheIsNull = pCreateInfo->cache == VK_NULL_HANDLE;

	// VkPipelineCache cache = pCreateInfo->cache;
	// std::thread cacheCreator;

	// if(cacheIsNull) {
	// 	cacheCreator = std::thread(
	// 		[&device, pCreateInfo, &cache]() 
	// 		{
	// 			VkPipelineCacheCreateInfo cacheCreateInfo{
	// 				VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO
	// 			};
	// 			ResultCheck(vkCreatePipelineCache(device, &cacheCreateInfo, VK_NULL_HANDLE, &cache));
	// 		}
	// 	);
	// }

	VkPipelineVertexInputStateCreateInfo vertexInputState{};
	vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;	
	vertexInputState.vertexAttributeDescriptionCount = FALLBACK_SIZE(pCreateInfo->pAttributeDescriptions);
	vertexInputState.vertexBindingDescriptionCount = FALLBACK_SIZE(pCreateInfo->pBindingDescription);
	vertexInputState.pVertexAttributeDescriptions = FALLBACK_PTR(pCreateInfo->pAttributeDescriptions);
	vertexInputState.pVertexBindingDescriptions = FALLBACK_PTR(pCreateInfo->pBindingDescription);

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
	inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyState.primitiveRestartEnable = VK_FALSE;
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkViewport viewportState{};
	viewportState.x = 0;
	viewportState.y = 0;
	viewportState.width = static_cast<f32>(pCreateInfo->extent.width);
	viewportState.height = static_cast<f32>(pCreateInfo->extent.height);
	viewportState.minDepth = 0.0f;
	viewportState.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = pCreateInfo->extent;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.pViewports = &viewportState;
	viewportStateCreateInfo.pScissors = &scissor;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.viewportCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizerPipelineStateCreateInfo{};
	rasterizerPipelineStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerPipelineStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizerPipelineStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizerPipelineStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	
	if(flags & PIPELINE_CREATE_FLAGS_ENABLE_CULLING)
		rasterizerPipelineStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	else
		rasterizerPipelineStateCreateInfo.cullMode = VK_CULL_MODE_NONE;

	rasterizerPipelineStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizerPipelineStateCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizerPipelineStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizerPipelineStateCreateInfo.depthBiasClamp = 0.0f;
	rasterizerPipelineStateCreateInfo.depthBiasSlopeFactor = 0.0f;
	rasterizerPipelineStateCreateInfo.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisamplerPipelineStageCreateInfo{};
	multisamplerPipelineStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplerPipelineStageCreateInfo.sampleShadingEnable = VK_FALSE;
	multisamplerPipelineStageCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisamplerPipelineStageCreateInfo.alphaToOneEnable = VK_FALSE;
	multisamplerPipelineStageCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisamplerPipelineStageCreateInfo.minSampleShading = 1.0f;
	multisamplerPipelineStageCreateInfo.pSampleMask = VK_NULL_HANDLE;

	VkPipelineColorBlendAttachmentState colorblendAttachmentState{};

	if(flags & PIPELINE_CREATE_FLAGS_ENABLE_BLEND)
	{
		colorblendAttachmentState.blendEnable = VK_TRUE;
		colorblendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		
		colorblendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorblendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorblendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		colorblendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		colorblendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		colorblendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	} else
	{
		colorblendAttachmentState.blendEnable = VK_FALSE;
	}

	VkPipelineColorBlendStateCreateInfo colorblendState{};
	colorblendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorblendState.attachmentCount = 1;
	colorblendState.pAttachments = &colorblendAttachmentState;
	colorblendState.logicOp = VK_LOGIC_OP_COPY;
	colorblendState.logicOpEnable = VK_FALSE;
	colorblendState.blendConstants[0] = 0.0f;
	colorblendState.blendConstants[1] = 0.0f;
	colorblendState.blendConstants[2] = 0.0f;
	colorblendState.blendConstants[3] = 0.0f;

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.stageCount = static_cast<uint32_t>(pCreateInfo->pShaderCreateInfos->size());
	graphicsPipelineCreateInfo.pStages = pCreateInfo->pShaderCreateInfos->data();
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizerPipelineStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisamplerPipelineStageCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colorblendState;
	graphicsPipelineCreateInfo.layout = pCreateInfo->pipelineLayout;
	graphicsPipelineCreateInfo.renderPass = pCreateInfo->renderPass;
	graphicsPipelineCreateInfo.subpass = pCreateInfo->subpass;
	graphicsPipelineCreateInfo.basePipelineHandle = pCreateInfo->oldPipeline;

	if(flags & PIPELINE_CREATE_FLAGS_ENABLE_DEPTH_CHECK)
	{
		VkPipelineDepthStencilStateCreateInfo depthStencilState{};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencilState.depthBoundsTestEnable = VK_FALSE;
		depthStencilState.minDepthBounds = 0.0f;
		depthStencilState.maxDepthBounds = 1.0f;
		depthStencilState.stencilTestEnable = VK_FALSE;

		graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilState;
	}

	// if(cacheIsNull) cacheCreator.join();
	// ResultCheck(vkCreateGraphicsPipelines(device, cache, 1, &graphicsPipelineCreateInfo, VK_NULL_HANDLE, &dstPipeline->pipeline));
	ResultCheck(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, VK_NULL_HANDLE, &dstPipeline->pipeline));
}

void pro::CreateRenderPass(VkDevice device, PipelineCreateInfo const *pCreateInfo, Pipeline *dstPipeline, u32 flags)
{
	REQUIRED_PTR(device);
	REQUIRED_PTR(pCreateInfo);
	REQUIRED_PTR(dstPipeline);
	NOT_EQUAL_TO(pCreateInfo->format, VK_FORMAT_UNDEFINED);

	if(flags & PIPELINE_CREATE_FLAGS_ENABLE_DEPTH_CHECK)
	{
		NOT_EQUAL_TO(pCreateInfo->depthBufferFormat, VK_FORMAT_UNDEFINED);
	}

    VkAttachmentDescription colorAttachmentDescription{};
	colorAttachmentDescription.format = pCreateInfo->format;
	colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentReference{};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = pCreateInfo->subpass;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;

	subpass.pColorAttachments = &colorAttachmentReference;

	std::vector<VkAttachmentDescription> attachments { colorAttachmentDescription };
	
	if(flags & PIPELINE_CREATE_FLAGS_ENABLE_DEPTH_CHECK)
	{
		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = pCreateInfo->depthBufferFormat;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		
		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		dependency.srcStageMask |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		attachments.push_back(depthAttachment);
	}
	
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	ResultCheck(vkCreateRenderPass(device, &renderPassInfo, VK_NULL_HANDLE, &dstPipeline->renderPass));
}

void pro::CreatePipelineLayout(VkDevice device, PipelineCreateInfo const *pCreateInfo, Pipeline *dstPipeline)
{
	REQUIRED_PTR(device);
	REQUIRED_PTR(pCreateInfo);
	REQUIRED_PTR(dstPipeline);

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = FALLBACK_SIZE(pCreateInfo->pDescriptorLayouts);
	pipelineLayoutCreateInfo.pSetLayouts = FALLBACK_PTR(pCreateInfo->pDescriptorLayouts);
	pipelineLayoutCreateInfo.pushConstantRangeCount = FALLBACK_SIZE(pCreateInfo->pPushConstants);
	pipelineLayoutCreateInfo.pPushConstantRanges = FALLBACK_PTR(pCreateInfo->pPushConstants);

	ResultCheck(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, VK_NULL_HANDLE, &dstPipeline->layout));
}

#ifdef LEGACY
void pro::CreateSwapChain(const VkDevice device, const SwapchainCreateInfo *pCreateInfo, VkSwapchainKHR *dstSwapchain)
{
	REQUIRED_PTR(device);
	REQUIRED_PTR(pCreateInfo);
	REQUIRED_PTR(pCreateInfo->surface);
	NOT_EQUAL_TO(pCreateInfo->format, VK_FORMAT_UNDEFINED);

	VkSwapchainCreateInfoKHR swapChainCreateInfo{};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = pCreateInfo->surface;
	swapChainCreateInfo.imageExtent = pCreateInfo->extent;
	swapChainCreateInfo.minImageCount = pCreateInfo->imageCount;
	swapChainCreateInfo.imageFormat = pCreateInfo->format;
	swapChainCreateInfo.imageColorSpace = pCreateInfo->colorSpace;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainCreateInfo.preTransform = pCreateInfo->preTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.clipped = VK_TRUE;
	swapChainCreateInfo.presentMode = pCreateInfo->presentMode;
	swapChainCreateInfo.oldSwapchain = pCreateInfo->pOldSwapchain;

	ResultCheck(vkCreateSwapchainKHR(device, &swapChainCreateInfo, VK_NULL_HANDLE, dstSwapchain));
}

#else

void pro::CreateSwapChain(VkDevice device, SwapchainCreateInfo const* pCreateInfo, VkSwapchainKHR *dstSwapchain)
{
	REQUIRED_PTR(device);
	REQUIRED_PTR(pCreateInfo);
	REQUIRED_PTR(pCreateInfo->physDevice);
	REQUIRED_PTR(pCreateInfo->surface);
	NOT_EQUAL_TO(pCreateInfo->extent.width, 0);
	NOT_EQUAL_TO(pCreateInfo->extent.height, 0);
	NOT_EQUAL_TO(pCreateInfo->format, VK_FORMAT_UNDEFINED);
	NOT_EQUAL_TO(pCreateInfo->presentMode, VK_PRESENT_MODE_MAX_ENUM_KHR);
	NOT_EQUAL_TO(pCreateInfo->requestedImageCount, 0);

	VkSwapchainCreateInfoKHR swapChainCreateInfo{};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = pCreateInfo->surface;
	swapChainCreateInfo.imageExtent = pCreateInfo->extent;
	swapChainCreateInfo.minImageCount = pCreateInfo->requestedImageCount;
	swapChainCreateInfo.imageFormat = pCreateInfo->format;
	swapChainCreateInfo.imageColorSpace = pCreateInfo->colorSpace;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.clipped = VK_TRUE;
	swapChainCreateInfo.presentMode = pCreateInfo->presentMode;
	swapChainCreateInfo.oldSwapchain = pCreateInfo->pOldSwapchain;

	if (vkCreateSwapchainKHR(device, &swapChainCreateInfo, VK_NULL_HANDLE, dstSwapchain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swapchain");
	}
}

#endif

void pro::CreateFramebuffer(const VkDevice device, const FramebufferCreateInfo* pCreateInfo, VkFramebuffer* dstFramebuffer)
{
	REQUIRED_PTR(pCreateInfo);
	REQUIRED_PTR(pCreateInfo->pRenderPass);
	REQUIRED_PTR(pCreateInfo->pAttachments);

	VkFramebufferCreateInfo swapchainFrameBufferCreateInfo{};
	swapchainFrameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	swapchainFrameBufferCreateInfo.renderPass = pCreateInfo->pRenderPass;
	swapchainFrameBufferCreateInfo.attachmentCount = pCreateInfo->pAttachments->size();
	swapchainFrameBufferCreateInfo.pAttachments = *pCreateInfo->pAttachments->data();
	swapchainFrameBufferCreateInfo.width = pCreateInfo->extent.width;
	swapchainFrameBufferCreateInfo.height = pCreateInfo->extent.height;
	swapchainFrameBufferCreateInfo.layers = 1;

	ResultCheck(vkCreateFramebuffer(device, &swapchainFrameBufferCreateInfo, VK_NULL_HANDLE, dstFramebuffer));
}

bool pro::GetSupportedFormat(VkDevice device, VkPhysicalDevice physDevice, VkSurfaceKHR surface, VkFormat *dstFormat, VkColorSpaceKHR *dstColorSpace)
{
	REQUIRED_PTR(device);
	REQUIRED_PTR(physDevice);
	REQUIRED_PTR(surface);
	REQUIRED_PTR(dstFormat);
	REQUIRED_PTR(dstColorSpace);

    u32 formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, VK_NULL_HANDLE);
	VkSurfaceFormatKHR surfaceFormats[formatCount];
	vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, surfaceFormats);

	VkSurfaceFormatKHR selectedFormat;
	std::unordered_map<VkFormat, VkColorSpaceKHR> desiredFormats = {
		{ VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
		{ VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }
	};

	for (const auto& surfaceFormat : surfaceFormats)
	{
		const auto formatIter = desiredFormats.find(surfaceFormat.format);
		if (formatIter != desiredFormats.end() &&
			formatIter->second == surfaceFormat.colorSpace)
		{
			selectedFormat = surfaceFormat;
			desiredFormats.erase(formatIter);
			break;
		}
	}

	if (selectedFormat.format == VK_FORMAT_UNDEFINED) {
		return VK_FALSE;
	} else {
		*dstFormat = selectedFormat.format;
		*dstColorSpace = selectedFormat.colorSpace;
		
		return VK_TRUE;
	}
}

u32 pro::GetImageCount(VkPhysicalDevice physDevice, VkSurfaceKHR surface)
{
	REQUIRED_PTR(physDevice);
	REQUIRED_PTR(surface);

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &surfaceCapabilities);

	uint32_t requestedImageCount = surfaceCapabilities.minImageCount + 1;
	if(requestedImageCount < surfaceCapabilities.maxImageCount)
	{
		requestedImageCount = surfaceCapabilities.maxImageCount;
	}

	return requestedImageCount;
}

u32 pro::GetMemoryType(VkPhysicalDevice physDevice, const u32 memoryTypeBits, const VkMemoryPropertyFlags properties)
{
	REQUIRED_PTR(physDevice);

    VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physDevice, &memoryProperties);

	for (u32 i = 0; i < memoryProperties.memoryTypeCount; i++) 
	{
    	if ((memoryTypeBits & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
        	return i;
    	}
	}

	return -1;
}

void pro::CreateBuffer(VkDevice device, BufferCreateInfo const *pCreateInfo, Buffer *dstBuffer)
{
	REQUIRED_PTR(device);
	REQUIRED_PTR(pCreateInfo);
	REQUIRED_PTR(dstBuffer);

	VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = pCreateInfo->size;
    bufferInfo.usage = pCreateInfo->usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    ResultCheck(vkCreateBuffer(device, &bufferInfo, VK_NULL_HANDLE, &dstBuffer->buffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, dstBuffer->buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = pCreateInfo->memoryType;
    ResultCheck(vkAllocateMemory(device, &allocInfo, VK_NULL_HANDLE, &dstBuffer->memory));

    ResultCheck(vkBindBufferMemory(device, dstBuffer->buffer, dstBuffer->memory, pCreateInfo->offset));
}
