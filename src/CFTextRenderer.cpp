#include "CFTextRenderer.hpp"
#include "Renderer.hpp"
#include "epichelperlib.hpp"

VkPipeline ctext::pipeline = VK_NULL_HANDLE;
VkPipelineLayout ctext::pipelineLayout = VK_NULL_HANDLE;
VkShaderModule ctext::vertex = VK_NULL_HANDLE;
VkShaderModule ctext::fragment = VK_NULL_HANDLE;

typedef char32_t unicode;

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
                (x + start.x * scale),
                (y + start.y * scale)
            );

            vec2 verticesB = vec2(
                (x + end.x * scale),
                (y + end.y * scale)
            );

            vertices.push_back(vec4(verticesA, texturePositionsA));
            vertices.push_back(vec4(verticesB, texturePositionsB));
        }
    }
}

void ctext::Render(cf::CFont font, VkCommandBuffer cmd, std::u32string text, float x, float y, float scale)
{
    // const unicode *epic = U"epic";

    // std::cout << std::endl;
    // f32 penX = x;
    // for (u32 i = 0; i < 4; i++) {
    //     GenerateGlyphVertices(font, font->GetGlyph(epic[i]), amog, penX, y, scale);
    //     penX += font->GetGlyph(epic[i]).geometry.getAdvance() * scale;
    // }
    cmd = Renderer->GetDrawBuffer();

    constexpr VkDeviceSize offsets = 0;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdDraw(cmd, 3, 1, 0, 0);
}

void ctext::Init()
{
    u32 VertexShaderBinarySize, FragmentShaderBinarySize;
    help::Files::LoadBinary("./Shaders/vert.text.spv", nullptr, &VertexShaderBinarySize);
    std::vector<u8> VertexShaderBinary(VertexShaderBinarySize);
    help::Files::LoadBinary("./Shaders/vert.text.spv", VertexShaderBinary.data(), &VertexShaderBinarySize);

    help::Files::LoadBinary("./Shaders/frag.text.spv", nullptr, &FragmentShaderBinarySize);
    std::vector<u8> FragmentShaderBinary(FragmentShaderBinarySize);
    help::Files::LoadBinary("./Shaders/frag.text.spv", FragmentShaderBinary.data(), &FragmentShaderBinarySize);

    VkShaderModuleCreateInfo vertexShaderModuleInfo{};
    vertexShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertexShaderModuleInfo.codeSize = VertexShaderBinarySize;
    vertexShaderModuleInfo.pCode = reinterpret_cast<const u32*>(VertexShaderBinary.data());
    if (vkCreateShaderModule(device, &vertexShaderModuleInfo, nullptr, &vertex) != VK_SUCCESS) {
        LOG_ERROR("TextRenderer::Failed to create vertex shader module!");
    }

    VkShaderModuleCreateInfo fragmentShaderModuleInfo{};
    fragmentShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragmentShaderModuleInfo.codeSize = FragmentShaderBinarySize;
    fragmentShaderModuleInfo.pCode = reinterpret_cast<const u32*>(FragmentShaderBinary.data());
    if (vkCreateShaderModule(device, &fragmentShaderModuleInfo, nullptr, &fragment) != VK_SUCCESS) {
        LOG_ERROR("TextRenderer::Failed to create fragment shader module!");
    }

    const std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {
        // location; binding; format; offset;
    };

    const std::vector<VkVertexInputBindingDescription> bindingDescriptions = {
        // binding; stride; inputRate
    };

    VkPipelineShaderStageCreateInfo vertexShaderInfo{};
    vertexShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderInfo.module = vertex;
    vertexShaderInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentShaderInfo{};
    fragmentShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderInfo.module = fragment;
    fragmentShaderInfo.pName = "main";

    // const pro::PipelineBlendState blendState(pro::BlendPreset::PRO_BLEND_PRESET_ALPHA);

    const std::vector<VkPipelineShaderStageCreateInfo> shaders = { vertexShaderInfo, fragmentShaderInfo };

    const pro::PipelineBlendState blendState(pro::BlendPreset::PRO_BLEND_PRESET_ALPHA);

    pro::PipelineCreateInfo pc{};
    pc.format = Graphics->SwapChainImageFormat;
    pc.subpass = 0;
    pc.renderPass = Graphics->GlobalRenderPass;
    pc.pipelineLayout = pipelineLayout;
    pc.pAttributeDescriptions = &attributeDescriptions;
    pc.pBindingDescriptions = &bindingDescriptions;
    pc.pDescriptorLayouts = nullptr;
    pc.pShaderCreateInfos = &shaders;
    pc.pPushConstants = nullptr;
    pc.pBlendState = &blendState;
    pc.extent.width = Graphics->RenderExtent.width;
    pc.extent.height = Graphics->RenderExtent.height;
    pc.pShaderCreateInfos = &shaders;
    pro::CreatePipelineLayout(device, &pc, &pipelineLayout);
    pc.pipelineLayout = pipelineLayout;
    pro::CreateGraphicsPipeline(device, &pc, &pipeline, PIPELINE_CREATE_FLAGS_ENABLE_BLEND);
}
