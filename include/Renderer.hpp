#ifndef __RENDERER__
#define __RENDERER__

#include "stdafx.hpp"

constexpr static SDL_Scancode EXIT_KEY = SDL_SCANCODE_ESCAPE;
constexpr static u8 MaxFramesInFlight = 2;
constexpr static VkExtent2D DefaultExtent = { 800, 600 }; // Starting window size

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

    static boost::signals2::signal<void(void)> OnWindowResized;
};

using vctx = VulkanContextSingleton;

struct FrameRenderData
{
    VkImage         swapchainImage;
    VkImageView     swapchainImageView;
    VkFramebuffer   framebuffer;
    VkSemaphore     imageAvailableSemaphore;
    VkSemaphore     renderingFinishedSemaphore;
    VkFence         inFlightFence;
};

struct Renderer
{
    Renderer() = default;
    ~Renderer() = default;

    static VkSwapchainKHR swapchain;
    static VkCommandPool commandPool;

    static u8 currentFrame;
    static u32 imageIndex;
    static bool resizeRequested;
    static bool running;

    static std::vector<FrameRenderData> renderData;
    static VkCommandBuffer drawBuffers[MaxFramesInFlight];

    static void Initialize();
    static void ProcessEvent(SDL_Event* event);

    static inline VkCommandBuffer GetDrawBuffer() { return drawBuffers[currentFrame]; };
    static bool BeginRender();
    static void EndRender();

    private:
    static void _SignalResize();
};

#endif