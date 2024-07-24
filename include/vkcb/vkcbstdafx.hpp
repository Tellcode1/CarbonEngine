#ifndef __VKCB_STDAFX_HPP__
#define __VKCB_STDAFX_HPP__

#include <vulkan/vulkan.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "carrays/csignal.hpp"

struct VulkanContextSingleton
{
    static VkFormat SwapChainImageFormat;
    static VkColorSpaceKHR SwapChainColorSpace;
    static u32 SwapChainImageCount;

    static VkRenderPass GlobalRenderPass;

    static VkExtent2D RenderExtent;

    static u32 GraphicsFamilyIndex;
    static u32 PresentFamilyIndex;
    static u32 ComputeFamilyIndex;
    static u32 TransferQueueIndex;
    static u32 GraphicsAndComputeFamilyIndex;

    static VkQueue GraphicsQueue;
    static VkQueue GraphicsAndComputeQueue;
    static VkQueue PresentQueue;
    static VkQueue ComputeQueue;
    static VkQueue TransferQueue;

    static csignal OnWindowResized;
};

using vctx = VulkanContextSingleton;

#include "pro.hpp"
#include "Context.hpp"
#include "Renderer.hpp"

#endif //__VKCB_STDAFX_HPP__