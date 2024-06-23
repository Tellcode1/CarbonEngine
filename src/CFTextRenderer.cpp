#include "CFTextRenderer.hpp"
#include "Renderer.hpp"
#include "epichelperlib.hpp"

void GenerateGlyphVertices(const cf::CFont font, const cf::CFGlyph& glyph, std::vector<vec4>& vertices, float x, float y, float scale) {
    for (const msdfgen::Contour& contour : glyph.geometry.getShape().contours) {
        for (const msdfgen::EdgeHolder& edge : contour.edges) {
            const msdfgen::Point2 start = edge->point(0.0);
            const msdfgen::Point2 end = edge->point(1.0);

            /*
                Why the FJSDKLFJKL does msdfgen use doubles instead of floats :<<<<<<
                kill me
            */
            double l_, b_, r_, t_;
            glyph.geometry.getQuadAtlasBounds(l_, b_, r_, t_);
            
            // doubles are for losers. L bozo ratio
            f32 l = l_, b = b_, r = r_, t = t_;

            vec2 texturePositionsA = vec2(
                l / font->atlasWidth, b / font->atlasHeight
            );

            vec2 texturePositionsB = vec2(
                r / font->atlasWidth, t / font->atlasHeight
            );

            vec2 verticesA = vec2(
                (x + start.x * scale) / Graphics->RenderExtent.width,
                (y + start.y * scale) / Graphics->RenderExtent.height
            );

            vec2 verticesB = vec2(
                (x + end.x * scale) / Graphics->RenderExtent.width,
                (y + end.y * scale) / Graphics->RenderExtent.height
            );

            vertices.push_back(vec4(verticesA, texturePositionsA));
            vertices.push_back(vec4(verticesB, texturePositionsB));
        }
    }
}

void ctext::Render(cf::CFont font, VkCommandBuffer cmd, std::u32string text, float x, float y, float scale)
{
	std::vector<vec4> amog;

    const char32_t *epic = U"epic";

    // std::cout << std::endl;
    f32 penX = x;
    for (u32 i = 0; i < 4; i++) {
        GenerateGlyphVertices(font, font->GetGlyph(epic[i]), amog, penX, y, scale);
        penX += font->GetGlyph(epic[i]).geometry.getAdvance() * scale;
    }

    VkBuffer among;
    VkDeviceMemory amongMemory;
    help::Buffers::CreateBuffer(
        amog.size() * sizeof(vec4),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &among,
        &amongMemory
    );

    void *mapped;
    vkMapMemory(device, amongMemory, 0, amog.size() * sizeof(vec4), 0, &mapped);
    memcpy(mapped, amog.data(), amog.size() * sizeof(vec4));
    vkUnmapMemory(device, amongMemory);

    constexpr VkDeviceSize offsets = 0;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindVertexBuffers(cmd, 0, 1, &among, &offsets);
    vkCmdDraw(cmd, amog.size(), 1, 0, 0);
}

void ctext::Init()
{
    u32 vertexSize, fragmentSize;
    help::Files::LoadBinary("./Shaders/vert.text.spv", nullptr, &vertexSize);
    u8 *vertexBinary = new u8[vertexSize];
    help::Files::LoadBinary("./Shaders/vert.text.spv", vertexBinary, &vertexSize);

    help::Files::LoadBinary("./Shaders/frag.text.spv", nullptr, &fragmentSize);
    u8 *fragmentBinary = new u8[fragmentSize];
    help::Files::LoadBinary("./Shaders/frag.text.spv", fragmentBinary, &fragmentSize);

    VkShaderModuleCreateInfo vertexShaderModuleInfo{};
    vertexShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertexShaderModuleInfo.codeSize = vertexSize;
    vertexShaderModuleInfo.pCode = reinterpret_cast<const u32*>(vertexBinary);
    if (vkCreateShaderModule(device, &vertexShaderModuleInfo, nullptr, &vertex) != VK_SUCCESS) {
        LOG_ERROR("ctext : Failed to create vertex shader module!");
    }

    VkShaderModuleCreateInfo fragmentShaderModuleInfo{};
    fragmentShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragmentShaderModuleInfo.codeSize = fragmentSize;
    fragmentShaderModuleInfo.pCode = reinterpret_cast<const u32*>(fragmentBinary);
    if (vkCreateShaderModule(device, &fragmentShaderModuleInfo, nullptr, &fragment) != VK_SUCCESS) {
        LOG_ERROR("ctext : Failed to create fragment shader module!");
    }

    delete[] vertexBinary;
    delete[] fragmentBinary;

    VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
    vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStageInfo.module = vertex;
    vertexShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
    fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderStageInfo.module = fragment;
    fragmentShaderStageInfo.pName = "main";

    VkVertexInputAttributeDescription attributeDescription{};
    attributeDescription.location = 0;
    attributeDescription.binding = 0;
    attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescription.offset = 0;

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(vec4);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    pro::PipelineBlendState blendState(pro::BlendPreset::PRO_BLEND_PRESET_ALPHA);

    std::vector<VkVertexInputAttributeDescription> attributeDescriptions = { attributeDescription };
    std::vector<VkVertexInputBindingDescription> bindingDescriptions = { bindingDescription };
    std::vector<VkPipelineShaderStageCreateInfo> shaders = { vertexShaderStageInfo, fragmentShaderStageInfo };

    pro::PipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.pShaderCreateInfos = &shaders;
    pipelineCreateInfo.extent = Graphics->RenderExtent;
    pipelineCreateInfo.format = Graphics->SwapChainImageFormat;
    pipelineCreateInfo.renderPass = Graphics->GlobalRenderPass;
    pipelineCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    pro::CreatePipelineLayout(device, &pipelineCreateInfo, &layout);

    pipelineCreateInfo.pipelineLayout = layout;

    pipelineCreateInfo.pAttributeDescriptions = &attributeDescriptions;
    pipelineCreateInfo.pBindingDescriptions = &bindingDescriptions;
    pipelineCreateInfo.pBlendState = &blendState;
    pipelineCreateInfo.pDescriptorLayouts = nullptr;
    pipelineCreateInfo.pPushConstants = nullptr;

    pro::CreateGraphicsPipeline(device, &pipelineCreateInfo, &pipeline, 0);
}
