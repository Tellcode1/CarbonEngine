#ifndef __GAMER_HPP__
#define __GAMER_HPP__

#include "stdafx.hpp"
#include "Bootstrap.hpp"
#include "pro.hpp"

struct Character {
    glm::uvec2 size;
    glm::ivec2 bearing;
    uint32_t offset;
    uint32_t advance;
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

struct TextRenderer
{
    size_t prevFrameHash;

    TextRenderer();

    void Init(VkDevice device, VkPhysicalDevice physDevice, VkQueue queue, VkCommandPool commandPool, const char* fontPath);
    void Render(const char* pText, VkCommandBuffer cmd, VkPipelineLayout test, glm::vec2& pos);
    void GetTextSize(const char* pText, float* width, float* height);

    void CreatePipeline();

    std::unordered_map<char, Character> Characters;
    
    VkImage FontTexture;
    VkDeviceMemory FontTextureMemory;
    VkImageView FontTextureView;
    VkSampler FontTextureSampler;
    VkBuffer unifiedBuffer;
    VkDeviceMemory mem;
    u32 ibOffset;

    u32 bmpWidth = 0;
    u32 bmpHeight = 0;

    private:
    VkDevice m_device;
    VkPhysicalDevice m_physDevice;
    VkQueue m_queue;

    VkPipeline m_pipeline;
    VkPipelineLayout m_layout;
    VkRenderPass m_renderPass;

    VkDescriptorPool m_descriptorPool;
    VkDescriptorSet m_descriptorSet;
    VkDescriptorSetLayout m_descriptorSetLayout;
};

#endif