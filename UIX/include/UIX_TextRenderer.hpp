#include "stdafx.hpp"

namespace uix
{
    struct UIX_Character
    {
        glm::uvec2 size;
        glm::ivec2 bearing;
        uint32_t offset;
        uint32_t advance;
    };
    struct TextRendererContext
    {
        std::unordered_map<char, UIX_Character> characters;

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
    };
}