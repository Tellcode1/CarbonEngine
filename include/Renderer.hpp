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

struct RendererSingleton;

struct FrameRenderData
{
    VkImage         swapchainImage;
    VkImageView     swapchainImageView;
    VkFramebuffer   framebuffer;
    VkSemaphore     imageAvailableSemaphore;
    VkSemaphore     renderingFinishedSemaphore;
    VkFence         inFlightFence;
};

struct RendererSingleton
{
    RendererSingleton() = default;
    ~RendererSingleton() = default;

    VkSwapchainKHR swapchain;

    u8 currentFrame = 0;
    u32 imageIndex = 0;
    bool resizeRequested = false;
    bool running = true;

    VkCommandPool commandPool;
    std::vector<FrameRenderData> renderData;
    VkCommandBuffer drawBuffers[MaxFramesInFlight];

    void Initialize();

    void Update();
    void ProcessEvent(SDL_Event* event);

    inline VkCommandBuffer GetDrawBuffer() { return drawBuffers[currentFrame]; };
    bool BeginRender();
    void EndRender();

    NANO_SINGLETON_FUNCTION RendererSingleton* GetSingleton() {
        static RendererSingleton global;
        return &global;
    }

    private:
    void _SignalResize();
};
static RendererSingleton* Renderer = RendererSingleton::GetSingleton();

#endif