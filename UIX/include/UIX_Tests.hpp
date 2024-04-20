#ifndef __UIX_TESTS_HPP__
#define __UIX_TESTS_HPP__

#include "stdafx.hpp"

constexpr u8 MAX_FRAMES_IN_FLIGHT = 2;

namespace uix
{
    struct UIXContext
    {
        VkInstance                      Instance;
        VkPhysicalDevice                physicalDevice;
        VkDevice                        device;
        u32                             queueFamily;
    };

    struct UIXRenderer
    {
        UIXContext*                     ctx;
        VkQueue                         queue;
        VkRenderPass                    renderPass;
        VkDescriptorPool                descriptorPool;
        
    };
}

#endif