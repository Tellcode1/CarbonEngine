#include "../include/lunaVK.h"
#include "../include/lunaPipeline.h"
#include "../include/lunaGFXDef.h"
#include "../include/lunaImage.h"

#include "../include/camera.h"

#include "../include/cshadermanager.h"
#include "../include/cshadermanagerdev.h"

#include "../include/lunaUI.h"

#include "../include/cgvector.h"

#include <stdio.h>
#include <stdlib.h>

#define HAS_FLAG(flag) ((luna_GPU_vk_flag_register & flag) || (flags & flag))

VkCommandPool pool = VK_NULL_HANDLE;
VkCommandBuffer buffer = VK_NULL_HANDLE;

luna_GPU_ResultCheckFn __cvk_resultFunc = __cvk_defaultResultCheckFunc; // To not cause nullptr dereference. SetResultCheckFunc also checks for nullptr and handles it.
u32 luna_GPU_vk_flag_register = 0;

SystemPipelines g_Pipelines;

VkPipeline base_pipeline = NULL;
VkPipelineCache cache = NULL;

void __BakeUnlitPipeline( luna_Renderer_t *rd ) {
    VkDescriptorSetLayoutBinding bindings[] = {
        // binding; descriptorType; descriptorCount; stageFlags; pImmutableSamplers;
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
    };

    VkDescriptorSetLayoutCreateInfo layoutinfo = {};
    layoutinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutinfo.pBindings = bindings;
    layoutinfo.bindingCount = 1;
    cassert(vkCreateDescriptorSetLayout(device, &layoutinfo, NULL, &g_Pipelines.Unlit.descriptor_layout) == VK_SUCCESS);

	csm_shader_t *vertex, *fragment;
    csm_load_shader("Unlit/vert", &vertex);
    csm_load_shader("Unlit/frag", &fragment);

    cassert(vertex != NULL && fragment != NULL);

    const csm_shader_t *shaders[] = { vertex, fragment };
    const VkDescriptorSetLayout layouts[] = { camera.sets->layout, g_Pipelines.Unlit.descriptor_layout };

    const lunaExtent2D RenderExtent = luna_Renderer_GetRenderExtent(rd);

	const VkVertexInputAttributeDescription attributeDescriptions[] = {
        // location; binding; format; offset;
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 }, // pos
        { 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(vec3) }, // texcoord
    };

    const VkVertexInputBindingDescription bindingDescriptions[] = {
        // binding; stride; inputRate
        { 0, sizeof(vec3) + sizeof(vec2), VK_VERTEX_INPUT_RATE_VERTEX }
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
    VkDescriptorSetLayout layouts[] = { camera.sets->layout, rd->ctext->desc_set->layout };

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

void __BakeDebugLinePipeline( luna_Renderer_t *rd ) {
	struct line_push_constants {
		mat4 model;
		vec4 color;
		vec2 line_begin;
        vec2 line_end;
	};

    const VkPushConstantRange pushConstants[] = {
        // stageFlags, offset, size
        { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(struct line_push_constants) },
    };

    csm_shader_t *vertex, *fragment;
    cassert(csm_load_shader("Debug/Line/vert", &vertex) != -1);
    cassert(csm_load_shader("Debug/Line/frag", &fragment) != -1);

    csm_shader_t * shaders[] = { vertex, fragment };
    VkDescriptorSetLayout layouts[] = { camera.sets->layout };

    const luna_GPU_PipelineBlendState blend = luna_GPU_InitPipelineBlendState(CVK_BLEND_PRESET_ALPHA);

    luna_GPU_PipelineCreateInfo pc = luna_GPU_InitPipelineCreateInfo();
    pc.format = SwapChainImageFormat;

    pc.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    pc.render_pass = luna_Renderer_GetRenderPass(rd);

    pc.nAttributeDescriptions = 0;
    pc.pAttributeDescriptions = NULL;

    pc.nPushConstants = array_len(pushConstants);
    pc.pPushConstants = pushConstants;

    pc.nBindingDescriptions = 0;
    pc.pBindingDescriptions = NULL;

    pc.nShaders = array_len(shaders);
    pc.pShaders = (const struct csm_shader_t *const *)shaders;

    pc.nDescriptorLayouts = array_len(layouts);
    pc.pDescriptorLayouts = layouts;

    const lunaExtent2D RenderExtent = luna_Renderer_GetRenderExtent(rd);
    pc.extent.width = RenderExtent.width;
    pc.extent.height = RenderExtent.height;
    pc.blend_state = &blend;
    pc.samples = Samples;

    luna_GPU_CreatePipelineLayout(&pc, &g_Pipelines.Line.pipeline_layout);
    pc.pipeline_layout = g_Pipelines.Line.pipeline_layout;
    luna_GPU_CreateGraphicsPipeline(&pc, &g_Pipelines.Line.pipeline, CVK_PIPELINE_FLAGS_ENABLE_BLEND);
}

void luna_VK_BakeGlobalPipelines( luna_Renderer_t *rd )
{
	__BakeUnlitPipeline(rd);
    __BakeDebugLinePipeline(rd);
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

void luna_VK_CreateBuffer( u64 size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer *dstBuffer, VkDeviceMemory *retMem, bool externallyAllocated)
{
    if(size == 0) {
        return;
    }

    VkBuffer newBuffer;
    VkDeviceMemory newMemory;

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = usageFlags;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &bufferCreateInfo, NULL, &newBuffer) != VK_SUCCESS)
    {
        printf("Renderer::failed to create staging buffer!");
    }

    VkMemoryRequirements bufferMemoryRequirements;
    vkGetBufferMemoryRequirements(device, newBuffer, &bufferMemoryRequirements);
    
    if(!externallyAllocated)
    {
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = bufferMemoryRequirements.size;
        allocInfo.memoryTypeIndex = luna_VK_GetMemType(bufferMemoryRequirements.memoryTypeBits, propertyFlags);
        if (vkAllocateMemory(device, &allocInfo, NULL, &newMemory) != VK_SUCCESS)
        {
            LOG_ERROR("failed to alloc gpu memory for buffer");
        }
        
        vkBindBufferMemory(device, newBuffer, newMemory, 0);
        *retMem = newMemory;
    }

    *dstBuffer = newBuffer;
}

void luna_VK_StageBufferTransfer(VkBuffer dst, void *data, u64 size)
{
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    VkBufferCreateInfo stagingBufferInfo = {};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = size;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &stagingBufferInfo, NULL, &stagingBuffer) != VK_SUCCESS)
    {
        printf("Failed to create staging buffer!");
    }

    VkMemoryRequirements stagingBufferMemoryRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingBufferMemoryRequirements);
    
    VkMemoryAllocateInfo stagingBufferAlloc = {};
    stagingBufferAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    stagingBufferAlloc.allocationSize = stagingBufferMemoryRequirements.size;
    stagingBufferAlloc.memoryTypeIndex = luna_VK_GetMemType(stagingBufferMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(device, &stagingBufferAlloc, NULL, &stagingBufferMemory) != VK_SUCCESS)
    {
        printf("Failed to allocate staging buffer memory!");
    }
    
    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

    void* mapped;
    vkMapMemory(device, stagingBufferMemory, 0, size, 0, &mapped);
    memcpy(mapped, data, size);
    vkUnmapMemory(device, stagingBufferMemory);

    const VkBufferCopy copy = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };
    
    VkCommandBuffer cmd = luna_VK_BeginCommandBuffer();
    vkCmdCopyBuffer(cmd, stagingBuffer, dst, 1, &copy);
    luna_VK_EndCommandBuffer(cmd, TransferQueue, 1);

    vkDestroyBuffer(device, stagingBuffer, NULL);
    vkFreeMemory(device, stagingBufferMemory, NULL);

}

u32 luna_VK_GetMemType( const u32 memoryTypeBits, const VkMemoryPropertyFlags memoryProperties)
{
    VkPhysicalDeviceMemoryProperties properties;
	vkGetPhysicalDeviceMemoryProperties(physDevice, &properties);

	for (u32 i = 0; i < properties.memoryTypeCount; i++) 
	{
    	if ((memoryTypeBits & (1 << i)) && (properties.memoryTypes[i].propertyFlags & memoryProperties) == memoryProperties)
        	return i;
	}

    return UINT32_MAX;
}

VkCommandBuffer luna_VK_BeginCommandBufferFrom( VkCommandBuffer src)
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(src, &beginInfo);

    return src;
}

VkCommandBuffer luna_VK_BeginCommandBuffer()
{
    if (!pool) {
        VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
        cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolCreateInfo.queueFamilyIndex = GraphicsFamilyIndex;
        cmdPoolCreateInfo.flags = 0;
        if(vkCreateCommandPool(device, &cmdPoolCreateInfo, NULL, &pool) != VK_SUCCESS) {
            LOG_ERROR("Failed to create command pool");
        }
    }

    VkCommandBufferAllocateInfo cmdAllocInfo = {};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;
    cmdAllocInfo.commandPool = pool;
    vkAllocateCommandBuffers(device, &cmdAllocInfo, &buffer);

    return luna_VK_BeginCommandBufferFrom(buffer);
}

VkResult luna_VK_EndCommandBuffer( VkCommandBuffer cmd, VkQueue queue, bool waitForExecution)
{
    VkResult res = vkEndCommandBuffer(cmd);
    if (res != VK_SUCCESS) return res;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    VkFence fence = VK_NULL_HANDLE;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = 0;

    if (waitForExecution) {
        res = vkCreateFence(device, &fenceInfo, NULL, &fence);
        if (res != VK_SUCCESS) return res;
    }

    res = vkQueueSubmit(queue, 1, &submitInfo, fence);
    if (res != VK_SUCCESS) return res;

    if (waitForExecution) {
        if (fence != VK_NULL_HANDLE) {
            res = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
            if (res != VK_SUCCESS) return res;
            vkDestroyFence(device, fence, NULL);
        }
        vkDeviceWaitIdle(device);
        vkFreeCommandBuffers(device, pool, 1, &cmd);
        buffer = NULL;
    }

    return VK_SUCCESS;
}

// void help::Files::LoadBinary(const char * path, cg_vector<u8>* dst)
// {
//     FILE *f = fopen(path, "rb");
//     cassert(f != NULL);

//     fseek(f, 0, SEEK_END);
//     int file_size = ftell(f);
//     rewind(f);

//     dst->resize( file_size );
//     fread(dst->data(), 1, file_size, f);

//     fclose(f);
// }

void luna_VK_LoadBinaryFile( const char* path, u8* dst, u32* dstSize) {
    FILE *f = fopen(path, "rb");
    cassert(f != NULL);

    fseek(f, 0, SEEK_END);
    int file_size = ftell(f);

    if (dst == NULL) {
        *dstSize = file_size;
        fclose(f);
        return;
    }

    rewind(f);

    fread(dst, 1, file_size, f);

    fclose(f);
}

void luna_VK_StageImageTransfer(VkImage dst, const void *data, int width, int height, int image_size)
{
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(device, dst, &mem_req);

    const VkBufferCreateInfo stagingBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = mem_req.size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    vkCreateBuffer(device, &stagingBufferInfo, NULL, &stagingBuffer);

    VkMemoryRequirements stagingBufferRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingBufferRequirements);

    const VkMemoryAllocateInfo stagingBufferAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = stagingBufferRequirements.size,
        .memoryTypeIndex = luna_VK_GetMemType(stagingBufferRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };

    vkAllocateMemory(device, &stagingBufferAllocInfo, NULL, &stagingBufferMemory);
    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

    void *stagingBufferMapped;
    cassert(vkMapMemory(device, stagingBufferMemory, 0, stagingBufferRequirements.size, 0, &stagingBufferMapped) == VK_SUCCESS);
    memcpy(stagingBufferMapped, data, image_size);
    vkUnmapMemory(device, stagingBufferMemory);

    const VkCommandBuffer cmd = luna_VK_BeginCommandBuffer(device);

    luna_VK_TransitionTextureLayout(
        cmd, dst, 1, VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        0, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
    );

    VkBufferImageCopy region = {};
    region.imageExtent = (VkExtent3D){ width, height, 1 };
    region.imageOffset =(VkOffset3D){ 0, 0, 0 };
    region.bufferOffset = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.mipLevel = 0;
    vkCmdCopyBufferToImage(cmd, stagingBuffer, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    luna_VK_TransitionTextureLayout(
            cmd, dst, 1, VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            0, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );

    luna_VK_EndCommandBuffer(cmd, TransferQueue, true);

    vkDestroyBuffer(device, stagingBuffer, NULL);
    vkFreeMemory(device, stagingBufferMemory, NULL);
}

void luna_VK_CreateTextureFromMemory(u8 *buffer, u32 width, u32 height, VkFormat format, VkImage *dst, VkDeviceMemory *dstMem)
{
    luna_VK_CreateTextureEmpty(width, height, format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, NULL, dst, dstMem);
    luna_VK_StageImageTransfer(*dst, buffer, width, height, width * height * vk_fmt_get_bpp(format));
}

void luna_VK_CreateTextureEmpty(u32 width, u32 height, VkFormat format, VkSampleCountFlagBits samples, VkImageUsageFlags usage, int *image_size, VkImage *dst, VkDeviceMemory *dstMem)
{
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = format;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = usage;
    imageCreateInfo.samples = samples;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device, &imageCreateInfo, NULL, dst) != VK_SUCCESS)
        LOG_ERROR("Failed to create image");

    VkMemoryRequirements imageMemoryRequirements;
    vkGetImageMemoryRequirements(device, *dst, &imageMemoryRequirements);

    if (image_size) {
        *image_size = imageMemoryRequirements.size;
    }

    // allow for preallocated memory.
    if (dstMem != NULL) {

        const u32 localDeviceMemoryIndex = luna_VK_GetMemType(imageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = imageMemoryRequirements.size;
        allocInfo.memoryTypeIndex = localDeviceMemoryIndex;

        vkAllocateMemory(device, &allocInfo, NULL, dstMem);
        vkBindImageMemory(device, *dst, *dstMem, 0);
    }
}

u8* luna_VK_CreateTextureFromDisk(const char *path, u32 *width, u32 *height, VkFormat *channels, VkImage *dst, VkDeviceMemory *dstMem)
{
    luna_Image tex = luna_ImageLoad(path);

    cassert(tex.data != NULL);

    *width = tex.w;
    *height = tex.h;
    *channels = tex.fmt;

    luna_VK_CreateTextureFromMemory(tex.data, tex.w, tex.h, *channels, dst, dstMem);
    return tex.data;
}

void luna_VK_TransitionTextureLayout( VkCommandBuffer cmd, VkImage image,
										 u32 mipLevels,
                                         VkImageAspectFlagBits aspect,
										 VkImageLayout oldLayout,
										 VkImageLayout newLayout,
										 VkAccessFlags srcAccessMask,
										 VkAccessFlags dstAccessMask,
										 VkPipelineStageFlags sourceStage,
										 VkPipelineStageFlags destinationStage)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspect;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;
    vkCmdPipelineBarrier(cmd, sourceStage, destinationStage, 0, 0, NULL, 0, NULL, 1, &barrier);
}

bool luna_VK_GetSupportedFormat( VkPhysicalDevice physDevice, VkSurfaceKHR surface, VkFormat *dstFormat, VkColorSpaceKHR *dstColorSpace)
{
	CVK_REQUIRED_PTR(device);
	CVK_REQUIRED_PTR(physDevice);
	CVK_REQUIRED_PTR(surface);
	CVK_REQUIRED_PTR(dstFormat);
	CVK_REQUIRED_PTR(dstColorSpace);

    u32 formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, VK_NULL_HANDLE);
	cg_vector_t /* VkSurfaceFormatKHR */ surfaceFormats = cg_vector_init(sizeof(VkSurfaceFormatKHR), formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, (VkSurfaceFormatKHR *)cg_vector_data(&surfaceFormats));

	VkSurfaceFormatKHR selectedFormat = { VK_FORMAT_MAX_ENUM, VK_COLOR_SPACE_MAX_ENUM_KHR };

	const VkSurfaceFormatKHR desired_formats[] = {
		{ VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
		{ VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }
	};

	for (u32 i = 0; i < formatCount; i++)
	{
		const VkSurfaceFormatKHR *surfaceFormat = (VkSurfaceFormatKHR *)cg_vector_get(&surfaceFormats, i);
		for (u32 j = 0; j < array_len(desired_formats); j++) {
			if (surfaceFormat->format == desired_formats[j].format 
				&&
				surfaceFormat->colorSpace == desired_formats[j].colorSpace
			   ) {
					selectedFormat.format = surfaceFormat->format;
					selectedFormat.colorSpace = surfaceFormat->colorSpace;
			   }
		}
	}

    cg_vector_destroy(&surfaceFormats);
	if (selectedFormat.format == VK_FORMAT_MAX_ENUM || selectedFormat.colorSpace == VK_COLOR_SPACE_MAX_ENUM_KHR) {
		return VK_FALSE;
	} else {
		*dstFormat = selectedFormat.format;
		*dstColorSpace = selectedFormat.colorSpace;
		
		return VK_TRUE;
	}

	return VK_FALSE;
}

u32 luna_VK_GetSurfaceImageCount(VkPhysicalDevice physDevice, VkSurfaceKHR surface)
{
	CVK_REQUIRED_PTR(physDevice);
	CVK_REQUIRED_PTR(surface);

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &surfaceCapabilities);

	u32 requestedImageCount = surfaceCapabilities.minImageCount + 1;
	if(requestedImageCount < surfaceCapabilities.maxImageCount)
		requestedImageCount = surfaceCapabilities.maxImageCount;

	return requestedImageCount;
}

// these parameters should be replaced
// properties should be replaced by usage. Like LUNA_GPU_MEMORY_USAGE_GPU, CPU_TO_GPU, GPU_TO_CPU, etc.
void luna_GPU_AllocateMemory(VkDeviceSize size, luna_GPU_MemoryUsage usage, luna_GPU_Memory *dst) {
    dst->size = size;
    dst->usage = usage;
    const VkMemoryPropertyFlags properties = (VkMemoryPropertyFlags)usage;

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = size;

    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physDevice, &mem_properties);

    for (u32 i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((properties & mem_properties.memoryTypes[i].propertyFlags) == properties) {
            alloc_info.memoryTypeIndex = i;
            break;
        }
    }

    if (vkAllocateMemory(device, &alloc_info, NULL, &dst->memory) != VK_SUCCESS || !dst->memory) {
        printf("An allocation has failed! What are we to do???\n");
    }
}

void luna_GPU_CreateBuffer(int size, int alignment, int nchilds, VkBufferUsageFlags usage, luna_GPU_Buffer *dst) {
    dst->allocation.size = size;
    dst->size = size;
    dst->alignment = alignment;

    const int aligned_sz = align_up(size, alignment);

    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = aligned_sz;
    buffer_info.usage = usage;
    for (int i = 0; i < nchilds; i++) {
        cassert(vkCreateBuffer(device, &buffer_info, NULL, &dst->buffers[i]) == VK_SUCCESS);
    }
    dst->nchilds = nchilds;
}

void luna_GPU_FreeMemory(luna_GPU_Memory *mem) {
    vkFreeMemory(device, mem->memory, NULL);
}

// I think these given an error when, say offset is too big so maybe we can check their return values?
void luna_GPU_BindBufferToMemory(luna_GPU_Memory *mem, int offset, luna_GPU_Buffer *buffer) {
    buffer->allocation.memory = mem;
    buffer->allocation.memory_offset = offset;

    const int aligned_sz = align_up(buffer->size, buffer->alignment);
    for (int i = 0; i < buffer->nchilds; i++) {
        vkBindBufferMemory(device, buffer->buffers[i], mem->memory, offset + i * aligned_sz);
    }
}
void luna_GPU_BindTextureToMemory(luna_GPU_Memory *mem, int offset, luna_GPU_Texture *tex) {
    tex->allocation.memory = mem;
    tex->allocation.memory_offset = offset;
    vkBindImageMemory(device, tex->image, mem->memory, offset);
}

void luna_GPU_DestroyBuffer(luna_GPU_Buffer *buffer) {
    for (int i = 0; i < buffer->nchilds; i++) {
        if (buffer->buffers[i]) {
            vkDestroyBuffer(device, buffer->buffers[i], NULL);
        } else {
            LOG_INFO("Attempt to destroy a buffer (child index %i) which is NULL", i);
        }
    }
}

void luna_GPU_DestroyTexture(luna_GPU_Texture *tex) {
    if (tex->image) {
        vkDestroyImage(device, tex->image, NULL);
        vkDestroyImageView(device, tex->view, NULL);
    } else {
        LOG_INFO("Attempt to destroy an image which is NULL");
    }
}

void luna_GPU_CreateSampler(const luna_GPU_SamplerCreateInfo *pInfo, luna_GPU_Sampler *sampler) {
    sampler->filter = pInfo->filter;
    sampler->mipmap_mode = pInfo->mipmap_mode;
    sampler->address_mode = pInfo->address_mode;
    sampler->max_anisotropy = pInfo->max_anisotropy;
    sampler->mip_lod_bias = pInfo->mip_lod_bias;
    sampler->min_lod = pInfo->min_lod;
    sampler->max_lod = pInfo->max_lod;

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = pInfo->filter;
    samplerInfo.minFilter = pInfo->filter;
    samplerInfo.mipmapMode = pInfo->mipmap_mode;
    samplerInfo.addressModeU = pInfo->address_mode;
    samplerInfo.addressModeV = pInfo->address_mode;
    samplerInfo.addressModeW = pInfo->address_mode;
    samplerInfo.anisotropyEnable = pInfo->max_anisotropy > 1.0f;
    samplerInfo.maxLod = pInfo->max_lod;
    samplerInfo.minLod = pInfo->min_lod;
    cassert(vkCreateSampler(device, &samplerInfo, NULL, &sampler->vksampler) == VK_SUCCESS);
}

void luna_GPU_CreateTexture(const luna_GPU_TextureCreateInfo *pInfo, luna_GPU_Texture *tex) {
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = pInfo->type;
    imageCreateInfo.extent = pInfo->extent;
    imageCreateInfo.mipLevels = pInfo->miplevels;
    imageCreateInfo.arrayLayers = pInfo->arraylayers;
    imageCreateInfo.format = pInfo->format;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = pInfo->usage;
    imageCreateInfo.samples = (VkSampleCountFlagBits)pInfo->samples;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device, &imageCreateInfo, NULL, &tex->image) != VK_SUCCESS) {
        LOG_ERROR("Failed to create image");
    }
}

// vkimage is fetched from image!
void luna_GPU_CreateTextureView(const luna_GPU_TextureViewCreateInfo *pInfo, luna_GPU_Texture *tex) {
    const VkFormat dst_format = (pInfo->format != VK_FORMAT_UNDEFINED) ? pInfo->format : tex->format;
    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = tex->image,
        .viewType = pInfo->view_type,
        .format = dst_format,
        .subresourceRange = pInfo->subresourceRange,
        .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
    };
    cassert(vkCreateImageView(device, &view_info, NULL, &tex->view) == VK_SUCCESS);
}