#include "../include/lunaPipeline.h"
#include "../include/cshadermanagerdev.h"
#include "../include/cgvector.h"
#include "../include/cengine.h"
#include "../include/cgfxdef.h"
#include "../include/lunaUI.h"

luna_GPU_ResultCheckFn __cvk_resultFunc = __cvk_defaultResultCheckFunc; // To not cause nullptr dereference. SetResultCheckFunc also checks for nullptr and handles it.
u32 luna_GPU_vk_flag_register = 0;

SystemPipelines g_Pipelines;

#define HAS_FLAG(flag) ((luna_GPU_vk_flag_register & flag) || (flags & flag))

VkPipeline base_pipeline = NULL;
VkPipelineCache cache = NULL;

void __BakeUnlitPipeline( luna_Renderer_t *rd ) {
	csm_shader_t *vertex, *fragment;
    csm_load_shader("Unlit/vert", &vertex);
    csm_load_shader("Unlit/frag", &fragment);

    cassert(vertex != NULL && fragment != NULL);

    const csm_shader_t *shaders[] = { vertex, fragment };
    const VkDescriptorSetLayout layouts[] = { camera.set.layout, luna_ui_ctx.set.layout };

    const lunaExtent2D RenderExtent = luna_Renderer_GetRenderExtent(rd);

	const VkVertexInputAttributeDescription attributeDescriptions[] = {
        // location; binding; format; offset;
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 }, // pos
        { 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(vec3) }, // texcoord
    };

    const VkVertexInputBindingDescription bindingDescriptions[] = {
        // binding; stride; inputRate
        { 0, sizeof(cvertex), VK_VERTEX_INPUT_RATE_VERTEX }
    };

    const VkPushConstantRange pushConstants[] = { 
        // stageFlags, offset, size
        {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(mat4) + sizeof(vec4)}
    };

	luna_GPU_PipelineCreateInfo pc = luna_GPU_InitPipelineCreateInfo();
    pc.format = SwapChainImageFormat;
    pc.subpass = 0;
    pc.render_pass = luna_Renderer_GetRenderPass(rd);

    pc.nAttributeDescriptions = array_len(attributeDescriptions);
    pc.pAttributeDescriptions = attributeDescriptions;

    pc.nPushConstants = array_len(pushConstants);
    pc.pPushConstants = pushConstants;

    pc.nBindingDescriptions = array_len(bindingDescriptions);
    pc.pBindingDescriptions = bindingDescriptions;

    pc.nShaders = array_len(shaders);
    pc.pShaders = shaders;

    pc.nDescriptorLayouts = array_len(layouts);
    pc.pDescriptorLayouts = layouts;

    pc.extent.width = RenderExtent.width;
    pc.extent.height = RenderExtent.height;
    pc.samples = Samples;
    luna_GPU_CreatePipelineLayout(&pc, &g_Pipelines.Unlit.pipeline_layout);
    pc.pipeline_layout = g_Pipelines.Unlit.pipeline_layout;
    luna_GPU_CreateGraphicsPipeline(&pc, &g_Pipelines.Unlit.pipeline, 0);
}

void __BakeCtextPipeline( luna_Renderer_t *rd ) {
	const VkVertexInputAttributeDescription attributeDescriptions[] = {
        // location; binding; format; offset;
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 }, // pos
        { 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(vec3) }, // uv
    };

    const VkVertexInputBindingDescription bindingDescriptions[] = {
        // binding; stride; inputRate
        { 0, sizeof(vec3) + sizeof(vec2), VK_VERTEX_INPUT_RATE_VERTEX }
    };

	struct push_constants {
		mat4 model;
		vec4 color;
		vec4 outline_color;
	};

    const VkPushConstantRange pushConstants[] = {
        // stageFlags, offset, size
        { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(struct push_constants) },
    };

    csm_shader_t *vertex, *fragment;
    cassert(csm_load_shader("ctext/vert", &vertex) != -1);
    cassert(csm_load_shader("ctext/frag", &fragment) != -1);

    csm_shader_t * shaders[] = { vertex, fragment };
    VkDescriptorSetLayout layouts[] = { camera.set.layout, rd->ctext->desc_set.layout };

    const luna_GPU_PipelineBlendState blend = luna_GPU_InitPipelineBlendState(CVK_BLEND_PRESET_ALPHA);

    luna_GPU_PipelineCreateInfo pc = luna_GPU_InitPipelineCreateInfo();
    pc.format = SwapChainImageFormat;
    pc.subpass = 0;
    pc.render_pass = luna_Renderer_GetRenderPass(rd);

    pc.nAttributeDescriptions = array_len(attributeDescriptions);
    pc.pAttributeDescriptions = attributeDescriptions;

    pc.nPushConstants = array_len(pushConstants);
    pc.pPushConstants = pushConstants;

    pc.nBindingDescriptions = array_len(bindingDescriptions);
    pc.pBindingDescriptions = bindingDescriptions;

    pc.nShaders = array_len(shaders);
    pc.pShaders = (const struct csm_shader_t *const *)shaders;

    pc.nDescriptorLayouts = array_len(layouts);
    pc.pDescriptorLayouts = layouts;

    const lunaExtent2D RenderExtent = luna_Renderer_GetRenderExtent(rd);
    pc.extent.width = RenderExtent.width;
    pc.extent.height = RenderExtent.height;
    pc.blend_state = &blend;
    pc.samples = Samples;

    luna_GPU_CreatePipelineLayout(&pc, &g_Pipelines.Ctext.pipeline_layout);
    pc.pipeline_layout = g_Pipelines.Ctext.pipeline_layout;
    luna_GPU_CreateGraphicsPipeline(&pc, &g_Pipelines.Ctext.pipeline, CVK_PIPELINE_FLAGS_ENABLE_BLEND);
}

void luna_VK_BakeGlobalPipelines( luna_Renderer_t *rd )
{
	__BakeUnlitPipeline(rd);
	//__BakeLitPipeline(rd); // Not yet implemented
	__BakeCtextPipeline(rd);
}

void luna_GPU_CreateGraphicsPipeline(const luna_GPU_PipelineCreateInfo *pCreateInfo, VkPipeline *dstPipeline, u32 flags)
{
	CVK_REQUIRED_PTR(device);
	CVK_REQUIRED_PTR(pCreateInfo);
	CVK_REQUIRED_PTR(dstPipeline);
	CVK_REQUIRED_PTR(pCreateInfo->render_pass);
	CVK_NOT_EQUAL_TO(pCreateInfo->nShaders, 0);
	CVK_NOT_EQUAL_TO(pCreateInfo->format, VK_FORMAT_UNDEFINED);
	CVK_NOT_EQUAL_TO(pCreateInfo->extent.width, 0);
	CVK_NOT_EQUAL_TO(pCreateInfo->extent.height, 0);

	if(HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING)) {
		// Vulkan requires samples to not be 1.
		CVK_NOT_EQUAL_TO(pCreateInfo->samples, VK_SAMPLE_COUNT_1_BIT);
	}

	VkPipelineVertexInputStateCreateInfo vertexInputState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,

		.vertexBindingDescriptionCount = pCreateInfo->nBindingDescriptions,
		.pVertexBindingDescriptions = pCreateInfo->pBindingDescriptions,
		.vertexAttributeDescriptionCount = pCreateInfo->nAttributeDescriptions,
		.pVertexAttributeDescriptions = pCreateInfo->pAttributeDescriptions,
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
	inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyState.primitiveRestartEnable = VK_FALSE;
	inputAssemblyState.topology = pCreateInfo->topology;

	VkViewport viewportState = {};
	viewportState.x = 0;
	viewportState.y = 0;
	viewportState.width = (f32)(pCreateInfo->extent.width);
	viewportState.height = (f32)(pCreateInfo->extent.height);
	viewportState.minDepth = 0.0f;
	viewportState.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = (VkOffset2D){0, 0};
	scissor.extent = pCreateInfo->extent;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.pViewports = &viewportState;
	viewportStateCreateInfo.pScissors = &scissor;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.viewportCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizerPipelineStateCreateInfo = {};
	rasterizerPipelineStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerPipelineStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizerPipelineStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizerPipelineStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	
	if(HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_CULLING) && !(flags & CVK_PIPELINE_FLAGS_UNFORCE_CULLING))
		rasterizerPipelineStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT; // No one uses other culling modes. If you do, I hate you.
	else
		rasterizerPipelineStateCreateInfo.cullMode = VK_CULL_MODE_NONE;

	rasterizerPipelineStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizerPipelineStateCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizerPipelineStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizerPipelineStateCreateInfo.depthBiasClamp = 0.0f;
	rasterizerPipelineStateCreateInfo.depthBiasSlopeFactor = 0.0f;
	rasterizerPipelineStateCreateInfo.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisamplerPipelineStageCreateInfo = {};
	multisamplerPipelineStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplerPipelineStageCreateInfo.sampleShadingEnable = VK_FALSE;
	multisamplerPipelineStageCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisamplerPipelineStageCreateInfo.alphaToOneEnable = VK_FALSE;
	multisamplerPipelineStageCreateInfo.minSampleShading = 1.0f;
	multisamplerPipelineStageCreateInfo.pSampleMask = VK_NULL_HANDLE;
	
	if(HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING) && !(flags & CVK_PIPELINE_FLAGS_UNFORCE_MULTISAMPLING))
		multisamplerPipelineStageCreateInfo.rasterizationSamples = pCreateInfo->samples;
	else
		multisamplerPipelineStageCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorblendAttachmentState = {};

	if(HAS_FLAG(CVK_PIPELINE_FLAGS_ENABLE_BLEND))
	{
		const luna_GPU_PipelineBlendState  *blendState = pCreateInfo->blend_state;

		colorblendAttachmentState.blendEnable = VK_TRUE;
		colorblendAttachmentState.srcColorBlendFactor = blendState->srcColorBlendFactor;
		colorblendAttachmentState.dstColorBlendFactor = blendState->dstColorBlendFactor;
		colorblendAttachmentState.colorBlendOp = blendState->colorBlendOp;
		colorblendAttachmentState.srcAlphaBlendFactor = blendState->srcAlphaBlendFactor;
		colorblendAttachmentState.dstAlphaBlendFactor = blendState->dstAlphaBlendFactor;
		colorblendAttachmentState.alphaBlendOp = blendState->alphaBlendOp;
		colorblendAttachmentState.colorWriteMask = blendState->colorWriteMask;
	} else
	{
		colorblendAttachmentState.blendEnable = VK_FALSE;
		colorblendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	}

	VkPipelineColorBlendStateCreateInfo colorblendState = {};
	colorblendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorblendState.attachmentCount = 1;
	colorblendState.pAttachments = &colorblendAttachmentState;
	colorblendState.logicOp = VK_LOGIC_OP_COPY;
	colorblendState.logicOpEnable = VK_FALSE;
	colorblendState.blendConstants[0] = 0.0f;
	colorblendState.blendConstants[1] = 0.0f;
	colorblendState.blendConstants[2] = 0.0f;
	colorblendState.blendConstants[3] = 0.0f;

	VkPipelineShaderStageCreateInfo *shader_infos = (VkPipelineShaderStageCreateInfo *)calloc(pCreateInfo->nShaders, sizeof(VkPipelineShaderStageCreateInfo));
	for (int i = 0; i < pCreateInfo->nShaders; i++) {
		shader_infos[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_infos[i].stage = (VkShaderStageFlagBits)pCreateInfo->pShaders[i]->stage;
		shader_infos[i].module = (VkShaderModule)pCreateInfo->pShaders[i]->shader_module;
		shader_infos[i].pName = "main";
	}

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = pCreateInfo->nShaders,
		.pStages = shader_infos,
		.pVertexInputState = &vertexInputState,
		.pInputAssemblyState = &inputAssemblyState,
		.pViewportState = &viewportStateCreateInfo,
		.pRasterizationState = &rasterizerPipelineStateCreateInfo,
		.pMultisampleState = &multisamplerPipelineStageCreateInfo,
		.pColorBlendState = &colorblendState,
		.layout = pCreateInfo->pipeline_layout,
		.renderPass = pCreateInfo->render_pass,
		.subpass = pCreateInfo->subpass,
		.basePipelineHandle = pCreateInfo->old_pipeline,
		.basePipelineIndex = 0, // ?
	};

	VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
	const VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	if (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_DYNAMIC_VIEWPORT) && !(flags & CVK_PIPELINE_FLAGS_UNFORCE_DYNAMIC_VIEWPORT)) {}
	else  {
		dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.dynamicStateCount = 2;
		dynamicStateInfo.pDynamicStates = dynamicStates;
		graphicsPipelineCreateInfo.pDynamicState = &dynamicStateInfo;
	}
	
	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	if(HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_DEPTH_CHECK) && !(flags & CVK_PIPELINE_FLAGS_UNFORCE_DEPTH_CHECK))
	{
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilState.depthBoundsTestEnable = VK_FALSE;
		depthStencilState.minDepthBounds = 0.0f;
		depthStencilState.maxDepthBounds = 1.0f;
		depthStencilState.stencilTestEnable = VK_FALSE;

		graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilState;
	}

	graphicsPipelineCreateInfo.basePipelineHandle = base_pipeline;

	// if(cacheIsNull) cacheCreator.join();
	CVK_ResultCheck(vkCreateGraphicsPipelines(device, pCreateInfo->cache, 1, &graphicsPipelineCreateInfo, VK_NULL_HANDLE, dstPipeline));

	base_pipeline = *dstPipeline;

	free(shader_infos);
}

void luna_GPU_CreateRenderPass(luna_GPU_RenderPassCreateInfo const *pCreateInfo, VkRenderPass *dstRenderPass, u32 flags)
{
	CVK_REQUIRED_PTR(device);
    CVK_REQUIRED_PTR(pCreateInfo);
    CVK_REQUIRED_PTR(dstRenderPass);
    CVK_NOT_EQUAL_TO(pCreateInfo->format, VK_FORMAT_UNDEFINED);

    if (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_DEPTH_CHECK) && !(flags & CVK_PIPELINE_FLAGS_UNFORCE_DEPTH_CHECK))
    {
        CVK_NOT_EQUAL_TO(pCreateInfo->depthBufferFormat, VK_FORMAT_UNDEFINED);
    }

    VkAttachmentDescription colorAttachmentDescription = {};
    colorAttachmentDescription.format = pCreateInfo->format;
    colorAttachmentDescription.samples = (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING) && !(flags & CVK_PIPELINE_FLAGS_UNFORCE_MULTISAMPLING)) ? pCreateInfo->samples : VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDescription.storeOp = (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING) && !(flags & CVK_PIPELINE_FLAGS_UNFORCE_MULTISAMPLING)) ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    // colorAttachmentDescription.finalLayout = (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING) && !(flags & CVK_PIPELINE_FLAGS_UNFORCE_MULTISAMPLING)) ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentReference;

    cg_vector_t /* VkAttachmentDescription */ attachments = cg_vector_init(sizeof(VkAttachmentDescription), 5);
	cg_vector_push_back(&attachments, &colorAttachmentDescription);

	VkAttachmentDescription depthAttachment = {};
	VkAttachmentReference depthAttachmentRef = {};
    if (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_DEPTH_CHECK) && !(flags & CVK_PIPELINE_FLAGS_UNFORCE_DEPTH_CHECK))
    {
		depthAttachment.format = pCreateInfo->depthBufferFormat; // Why wasn't this being used?
    	depthAttachment.samples = pCreateInfo->samples;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		// if (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING) && !(flags & CVK_PIPELINE_FLAGS_UNFORCE_MULTISAMPLING))
		// 	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		// else
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		depthAttachmentRef.attachment = cg_vector_size(&attachments);
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        subpass.pDepthStencilAttachment = &depthAttachmentRef;

		cg_vector_push_back(&attachments, &depthAttachment);
    }

	VkAttachmentReference colorAttachmentResolveRef = {};
	VkAttachmentDescription colorAttachmentResolve = {};
    if ((flags & CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING) && !(flags & CVK_PIPELINE_FLAGS_UNFORCE_MULTISAMPLING))
    {
        colorAttachmentResolve.format = pCreateInfo->format;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        colorAttachmentResolveRef.attachment = cg_vector_size(&attachments);
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		cg_vector_push_back(&attachments, &colorAttachmentResolve);

        subpass.pResolveAttachments = &colorAttachmentResolveRef;
    }

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = cg_vector_size(&attachments);
    renderPassInfo.pAttachments = (const VkAttachmentDescription *)cg_vector_data(&attachments);
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 0;
    renderPassInfo.pDependencies = NULL;

    CVK_ResultCheck(vkCreateRenderPass(device, &renderPassInfo, VK_NULL_HANDLE, dstRenderPass));
}

void luna_GPU_CreatePipelineLayout(luna_GPU_PipelineCreateInfo const *pCreateInfo, VkPipelineLayout *dstLayout)
{
	CVK_REQUIRED_PTR(device);
	CVK_REQUIRED_PTR(pCreateInfo);
	CVK_REQUIRED_PTR(dstLayout);

    // int totalLayouts = 0;
	// for (int i = 0; i < pCreateInfo->nShaders; i++) {
	// 	totalLayouts += pCreateInfo->pShaders[i]->nsetlayouts;
	// }

	// cg_vector_t *sets = cg_vector_init(sizeof(VkDescriptorSetLayout), totalLayouts);

	// for (int i = 0; i < pCreateInfo->nShaders; i++) {
	// 	const csm_shader_t *shader = pCreateInfo->pShaders[i];
	// 	for (int j = 0; j < shader->nsetlayouts; j++) {
	// 		cg_vector_push_back(sets, &shader->setlayouts[j]);
	// 	}
	// }

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = pCreateInfo->nDescriptorLayouts;
	pipelineLayoutCreateInfo.pSetLayouts = pCreateInfo->pDescriptorLayouts;
	pipelineLayoutCreateInfo.pushConstantRangeCount = pCreateInfo->nPushConstants;
	pipelineLayoutCreateInfo.pPushConstantRanges = pCreateInfo->pPushConstants;

	CVK_ResultCheck(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, VK_NULL_HANDLE, dstLayout));
}

void luna_GPU_CreateSwapchain(luna_GPU_SwapchainCreateInfo const* pCreateInfo, VkSwapchainKHR *dstSwapchain)
{
	CVK_REQUIRED_PTR(device);
	CVK_REQUIRED_PTR(pCreateInfo);
	CVK_NOT_EQUAL_TO(pCreateInfo->extent.width, 0);
	CVK_NOT_EQUAL_TO(pCreateInfo->extent.height, 0);
	CVK_NOT_EQUAL_TO(pCreateInfo->format, VK_FORMAT_UNDEFINED);
	CVK_NOT_EQUAL_TO(pCreateInfo->image_count, 0);

	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = surface;
	swapChainCreateInfo.imageExtent = pCreateInfo->extent;
	swapChainCreateInfo.minImageCount = pCreateInfo->image_count;
	swapChainCreateInfo.imageFormat = pCreateInfo->format;
	swapChainCreateInfo.imageColorSpace = pCreateInfo->color_space;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.clipped = VK_TRUE;
	swapChainCreateInfo.presentMode = pCreateInfo->present_mode;
	swapChainCreateInfo.oldSwapchain = pCreateInfo->old_swapchain;
	swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	CVK_ResultCheck(vkCreateSwapchainKHR(device, &swapChainCreateInfo, VK_NULL_HANDLE, dstSwapchain));
}

luna_GPU_PipelineBlendState luna_GPU_InitPipelineBlendState(luna_GPU_PipelineBlendPreset preset)
{
	luna_GPU_PipelineBlendState ret = {};
	ret.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	switch(preset)
	{
		case CVK_BLEND_PRESET_NONE:
			ret.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.colorBlendOp = VK_BLEND_OP_ADD;
			ret.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.alphaBlendOp = VK_BLEND_OP_ADD;
			break;
		case CVK_BLEND_PRESET_ALPHA:
			ret.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			ret.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			ret.colorBlendOp = VK_BLEND_OP_ADD;
			ret.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			ret.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.alphaBlendOp = VK_BLEND_OP_ADD;
			break;
		case CVK_BLEND_PRESET_ADDITIVE:
			ret.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
			ret.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
			ret.colorBlendOp = VK_BLEND_OP_ADD;
			ret.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			ret.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			ret.alphaBlendOp = VK_BLEND_OP_ADD;
			break;
		case CVK_BLEND_PRESET_MULTIPLICATIVE:
			ret.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
			ret.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.colorBlendOp = VK_BLEND_OP_ADD;
			ret.srcAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
			ret.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.alphaBlendOp = VK_BLEND_OP_ADD;
			break;
		case CVK_BLEND_PRESET_PREMULTIPLIED_ALPHA:
			ret.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
			ret.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			ret.colorBlendOp = VK_BLEND_OP_ADD;
			ret.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			ret.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			ret.alphaBlendOp = VK_BLEND_OP_ADD;
			break;
		case CVK_BLEND_PRESET_SUBTRACTIVE:
			ret.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
			ret.colorBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
			ret.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			ret.alphaBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
			break;
		default:
			ret.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.colorBlendOp = VK_BLEND_OP_ADD;
			ret.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.alphaBlendOp = VK_BLEND_OP_ADD;
			break;
	}
	return ret;
}
