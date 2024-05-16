#ifndef __NANO_TEXT_RENDERER_HPP__
#define __NANO_TEXT_RENDERER_HPP__

#include "stdafx.hpp"
#include "Bootstrap.hpp"
#include "pro.hpp"
#include <glm/packing.hpp>

constexpr u8 MaxFontCount = 8;

struct Character {
    /* packed */
    glm::uint size;
    glm::uint bearing;
    f32 advance;
};

enum NanoTextAlignmentHorizontal : u8
{
    NANO_TEXT_HORIZONTAL_ALIGN_LEFT = 0,
    NANO_TEXT_HORIZONTAL_ALIGN_CENTER = 1,
    NANO_TEXT_HORIZONTAL_ALIGN_RIGHT = 2,

    TEXT_HORIZONTAL_ALIGN_LEFT   = NANO_TEXT_HORIZONTAL_ALIGN_LEFT,
    TEXT_HORIZONTAL_ALIGN_CENTER = NANO_TEXT_HORIZONTAL_ALIGN_CENTER,
    TEXT_HORIZONTAL_ALIGN_RIGHT  = NANO_TEXT_HORIZONTAL_ALIGN_RIGHT,
};

enum NanoTextAlignmentVertical : u8
{
    NANO_TEXT_VERTICAL_ALIGN_TOP = 3,
    NANO_TEXT_VERTICAL_ALIGN_CENTER = 4,
    NANO_TEXT_VERTICAL_ALIGN_BOTTOM = 5,
    
    TEXT_VERTICAL_ALIGN_TOP    = NANO_TEXT_VERTICAL_ALIGN_TOP,
    TEXT_VERTICAL_ALIGN_CENTER = NANO_TEXT_VERTICAL_ALIGN_CENTER,
    TEXT_VERTICAL_ALIGN_BOTTOM = NANO_TEXT_VERTICAL_ALIGN_BOTTOM,
};

enum NanoTextUpdateFrequency
{
    // NANO_TEXT_UPDATE_STATIC = 0,
    // NANO_TEXT_UPDATE_DYNAMIC = 2,
    NANO_TEXT_UPDATE_ONCE = 0,
    NANO_TEXT_UPDATE_SOMETIMES = 1,
    NANO_TEXT_UPDATE_DYNAMIC = 2,

    TEXT_UPDATE_ONCE = NANO_TEXT_UPDATE_ONCE,
    TEXT_UPDATE_SOMETIMES = NANO_TEXT_UPDATE_SOMETIMES,
    TEXT_UPDATE_DYNAMIC = NANO_TEXT_UPDATE_DYNAMIC,
};

enum NanoTextRenderMode
{
    NANO_TEXT_RENDER_MODE_RASTERIZED = 0,
    NANO_TEXT_RENDER_MODE_BITMAP = NANO_TEXT_RENDER_MODE_RASTERIZED,

    NANO_TEXT_RENDER_MODE_SDF = 1,
    NANO_TEXT_RENDER_MODE_SIGNED_DISTANCE_FIELD = NANO_TEXT_RENDER_MODE_SDF,
};

struct NanoFont
{
    VkImage fontTexture;
    VkImageView fontTextureView;
    VkDeviceMemory fontTextureMemory;

    u32 bmpWidth = 0;
    u32 bmpHeight = 0;
    u8 fontIndex;
    f32 lineHeight = 0.0f;

    vec2 texturePositions[ 128 * 4 ];
    Character characters[ 128 ];
};

struct TextRenderInfo {
    float scale;
    float x;
    float y;
    std::string line;
    NanoTextAlignmentHorizontal horizontal;
    NanoTextAlignmentVertical vertical;
};

struct TextRendererSingleton
{
    TextRendererSingleton() = default;
    ~TextRendererSingleton() = default;
    
    void Initialize();

    void GetTextSize(const NanoFont* font, const std::string_view& pText, f32* width, f32* height, f32 scale);
    
    void BeginRender();
    void Render(TextRenderInfo& info, const NanoFont* font);
    void EndRender(VkCommandBuffer cmd, const mat2& matrix = mat2(1.0f));

    void LoadFont(const char* path, u32 pixelSizes, NanoFont* dstFont);
    void Destroy(NanoFont* obj);

    void CreatePipeline(VkRenderPass renderPass);
    void RecreatePipeline();

    glm::mat2 projection;

    u32 charsDrawn = 0;
    u8 currFontIndex = 0;

    struct Draw
    {
        u32 indexCount;
        u32 vertexOffset;
        float scale;
        const NanoFont* font;
    };

    std::vector<Draw> drawList;
    std::vector<vec4> vertices;
    std::vector<u16> indices;

    VkBuffer unifiedBuffer = VK_NULL_HANDLE;
    VkDeviceMemory mem = VK_NULL_HANDLE;
    u32 allocatedMemorySize = 0;
    u32 alignment = 0;
    u32 ibOffset = 0;

    static TextRendererSingleton* GetSingleton() {
        static TextRendererSingleton global{};
        return &global;
    }

    VkFence fence;
    bool dispatchedCompute = false;

    // void LoadFontAsync(const char* path, u32 pixelSizes, std::function<void(NanoFont)> callback, NanoFont* dstFont = nullptr);
    VkCommandBuffer computeCmdBuffers[MaxFramesInFlight];
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
    VkCommandPool m_cmdPool = VK_NULL_HANDLE;
    VkCommandBuffer m_cmdBuffer = VK_NULL_HANDLE;

    private:
    void DispatchCompute();
    void RenderLine(const f32 x, f32* y, const std::string_view line, const NanoTextAlignmentHorizontal horizontal, const f32 scale, const NanoFont* font);

    // ! Not currently async, implement.

    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;;
    VkPipeline m_computePipeline = VK_NULL_HANDLE;;
    VkPipelineLayout m_computePipelineLayout = VK_NULL_HANDLE;;

    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    
    VkDescriptorSet m_computeDescriptorSet = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_computeDescriptorSetLayout = VK_NULL_HANDLE;

    VkImage errorImage = VK_NULL_HANDLE;
    VkDeviceMemory errorImageMemory = VK_NULL_HANDLE;
    VkImageView errorImageView = VK_NULL_HANDLE;
};

static TextRendererSingleton* TextRenderer = TextRendererSingleton::GetSingleton();

#endif