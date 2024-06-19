#ifndef __RENDERER__
#define __RENDERER__

#include "stdafx.hpp"

constexpr static SDL_Scancode EXIT_KEY = SDL_SCANCODE_ESCAPE;
constexpr static u8 MaxFramesInFlight = 2;
constexpr static VkExtent2D DefaultExtent = { 800, 600 };

struct GraphicsSingleton
{
    VkFormat SwapChainImageFormat;
    VkColorSpaceKHR SwapChainColorSpace;
    u32 SwapChainImageCount;

    VkRenderPass GlobalRenderPass;

    VkExtent2D RenderExtent = DefaultExtent;

    u32 GraphicsFamilyIndex;
    u32 PresentFamilyIndex;
    u32 ComputeFamilyIndex;
    u32 TransferQueueIndex;
    u32 GraphicsAndComputeFamilyIndex;

    VkQueue GraphicsQueue;
    VkQueue GraphicsAndComputeQueue;
    VkQueue PresentQueue;
    VkQueue ComputeQueue;
    VkQueue TransferQueue;

    boost::signals2::signal<void(void)> OnWindowResized;

    NANO_SINGLETON_FUNCTION GraphicsSingleton* GetSingleton() {
        static GraphicsSingleton global;
        return &global;
    }
};

static GraphicsSingleton* Graphics = GraphicsSingleton::GetSingleton();

struct RendererSingleton
{
    RendererSingleton() = default;
    ~RendererSingleton() = default;

    VkSwapchainKHR swapchain;
    
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> framebuffers;

    u8 currentFrame = 0;
    u32 imageIndex = 0;
    bool resizeRequested = false;
    bool running = true;

    VkCommandPool commandPool;
    VkSemaphore imageAvailableSemaphore[MaxFramesInFlight];
    VkSemaphore renderingFinishedSemaphore[MaxFramesInFlight];
    VkFence InFlightFence[MaxFramesInFlight];
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
    void _SignalResize(VkExtent2D newSize);
};
static RendererSingleton* Renderer = RendererSingleton::GetSingleton();

#endif