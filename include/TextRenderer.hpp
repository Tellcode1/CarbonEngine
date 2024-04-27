#ifndef __GAMER_HPP__
#define __GAMER_HPP__

#include "stdafx.hpp"
#include "Bootstrap.hpp"
#include "pro.hpp"

// enum TextRendererUpdateStateBits
// {
//     TEXT_RENDERER_ALL_GOOD = 1,
//     TEXT_RENDERER_NEED_STAGING = 2,
//     TEXT_RENDERER_NEED_RECALCULATION = 4,
// };

struct Character {
    glm::uvec2 size;
    glm::ivec2 bearing;
    u32 offset;
    f32 advance;
};

struct TextRendererInitInfo
{
    VkDevice device;
    VkPhysicalDevice physDevice;
    VkQueue queue;
    VkCommandPool commandPool;
    const char* fontPath;

    VkFormat format = VK_FORMAT_B8G8R8A8_SRGB;
    VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
};

enum TextRendererAlignmentHorizontal
{
    TEXT_HORIZONTAL_ALIGN_LEFT = 0,
    TEXT_HORIZONTAL_ALIGN_CENTER = 1,
    TEXT_HORIZONTAL_ALIGN_RIGHT = 2
};

enum TextRendererAlignmentVertical
{
    TEXT_VERTICAL_ALIGN_TOP = 0,
    TEXT_VERTICAL_ALIGN_CENTER = 1,
    TEXT_VERTICAL_ALIGN_BOTTOM = 2
};

struct TextRenderer
{
    size_t prevFrameHash;

    TextRenderer();

    void Init(VkDevice device, VkPhysicalDevice physDevice, VkQueue queue, VkCommandPool commandPool, VkSemaphore imgSemaphore, const char* fontPath);
    void Render(const char* pText, VkCommandBuffer cmd, float scale, float x, float y, TextRendererAlignmentHorizontal horizontal = TEXT_HORIZONTAL_ALIGN_LEFT, TextRendererAlignmentVertical vertical = TEXT_VERTICAL_ALIGN_CENTER);
    void GetTextSize(const char* pText, u32* width, u32* height, u32* maxWidth);

    void CreatePipeline();

    // std::map<char, Character> Characters;
    Character Characters[128];
    
    VkImage FontTexture;
    VkImageView FontTextureView;
    VkSampler FontTextureSampler;

    // Font texture memory is kept different from buffer memory because it frequently changes
    VkDeviceMemory FontTextureMemory;
    
    VkBuffer unifiedBuffer;
    VkDeviceMemory mem;

    u32 ibOffset;

    u32 bmpWidth = 0;
    u32 bmpHeight = 0;
    float lineHeight = 0.0f;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderingFinishedSemaphore;
    VkFence InFlightFence;
    VkPipelineLayout m_pipelineLayout;

    private:
    VkSampleCountFlagBits m_samples;

    VkDevice m_device;
    VkPhysicalDevice m_physDevice;
    VkQueue m_queue;

    VkCommandBuffer m_cmdBuffer;
    VkCommandPool m_cmdPool;

    VkPipeline m_pipeline;
    VkRenderPass m_renderPass;

    VkDescriptorPool m_descriptorPool;
    VkDescriptorSet m_descriptorSet;
    VkDescriptorSetLayout m_descriptorSetLayout;
};

#endif