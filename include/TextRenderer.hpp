#ifndef __GAMER_HPP__
#define __GAMER_HPP__

#include "stdafx.hpp"
#include "Bootstrap.hpp"
#include "pro.hpp"

struct Character {
    glm::uvec2 size;
    glm::ivec2 bearing;
    u32 offset;
    f32 advance;
};

enum TextRendererAlignmentHorizontal : u32
{
    TEXT_HORIZONTAL_ALIGN_LEFT = 0,
    TEXT_HORIZONTAL_ALIGN_CENTER = 1,
    TEXT_HORIZONTAL_ALIGN_RIGHT = 2
};

enum TextRendererAlignmentVertical : u32
{
    TEXT_VERTICAL_ALIGN_TOP = 3,
    TEXT_VERTICAL_ALIGN_CENTER = 4,
    TEXT_VERTICAL_ALIGN_BOTTOM = 5
};

struct TextRenderer
{
    u64 lastFrameHash;
    u32 indexCount = 0;

    TextRenderer();

    struct TextRenderInfo {
        float scale;
        float x;
        float y;
        std::string line;
        TextRendererAlignmentHorizontal horizontal;
        TextRendererAlignmentVertical vertical;
    };
    std::vector<TextRenderInfo> drawList;

    void Init(VkDevice device, VkPhysicalDevice physDevice, VkQueue queue, VkCommandPool commandPool, VkRenderPass renderPass, const char* fontPath);
    void GetTextSize(const std::string& pText, f32* width, f32* height, f32 scale);
    
    void AddToTextRenderQueue(std::string& line, const f32 scale, const f32 x, f32 y, TextRendererAlignmentHorizontal horizontal = TEXT_HORIZONTAL_ALIGN_LEFT, TextRendererAlignmentVertical vertical = TEXT_VERTICAL_ALIGN_CENTER);
    void AddToTextRenderQueue(TextRenderInfo& info) { drawList.push_back(info); }
    void Render(VkCommandBuffer cmd);

    void CreatePipeline(VkRenderPass renderPass);

    VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }

    VkBuffer unifiedBuffer;
    VkDeviceMemory mem;
    u32 allocatedMemorySize = 0;
    u32 alignment = 0;

    u32 ibOffset;
    u32 bmpWidth = 0;
    u32 bmpHeight = 0;
    f32 lineHeight = 0.0f;

    TextRenderer& operator=(TextRenderer&&) = default;
    TextRenderer(TextRenderer&&) = default;

    static TextRenderer* GetSingleton() {
        static TextRenderer global;
        return &global;
    }

    private:
    VkDevice m_device;
    VkPhysicalDevice m_physDevice;
    VkQueue m_queue;

    VkCommandBuffer m_cmdBuffer;
    VkCommandPool m_cmdPool;

    Character Characters[128];
    
    VkImage FontTexture;
    VkImageView FontTextureView;
    VkSampler FontTextureSampler;

    // Font texture memory is kept different from buffer memory because it frequently changes
    VkDeviceMemory FontTextureMemory;

    VkPipeline m_pipeline;
    VkPipelineLayout m_pipelineLayout;

    VkDescriptorPool m_descriptorPool;
    VkDescriptorSet m_descriptorSet;
    VkDescriptorSetLayout m_descriptorSetLayout;};

static TextRenderer* textRD = TextRenderer::GetSingleton();

#endif