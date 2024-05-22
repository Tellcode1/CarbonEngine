#ifndef __RENDERER__
#define __RENDERER__

#include "stdafx.hpp"

struct WindowSizeType {
    u32 width, height;
    u32& x = width, y = height;

    constexpr inline operator VkExtent2D() {
        return VkExtent2D{ width, height };
    }

    constexpr inline operator glm::uvec2() {
        return glm::uvec2{ width, height };
    }

    WindowSizeType() = default;
    ~WindowSizeType() = default;
    constexpr WindowSizeType(const WindowSizeType& rhs) : width(rhs.width), height(rhs.height) { }
    constexpr WindowSizeType(u32 w, u32 h) : width(w), height(h) {}

    constexpr WindowSizeType& operator =(WindowSizeType&& rhs) {
        width = rhs.width;
        height = rhs.height;
        return *this;
    }
};

constexpr static SDL_Scancode EXIT_KEY = SDL_SCANCODE_ESCAPE;
constexpr static u8 MaxFramesInFlight = 2;
constexpr static WindowSizeType DefaultExtent = { 800, 600 };

// struct FrameRenderData
// {
//     VkSemaphore imageAvailableSemaphore;
//     VkSemaphore renderingFinishedSemaphore;
//     VkFence InFlightFence;
//     VkDescriptorSet descriptorSet;
//     VkDescriptorSetLayout descriptorSetLayout;
// };

struct GraphicsSingleton
{
    VkFormat SwapChainImageFormat;
    VkColorSpaceKHR SwapChainColorSpace;
    u32 SwapChainImageCount;

    VkRenderPass GlobalRenderPass;

    WindowSizeType RenderArea = DefaultExtent;

    u32 GraphicsFamilyIndex;
    u32 PresentFamilyIndex;
    u32 ComputeFamilyIndex;
    u32 GraphicsAndComputeFamilyIndex;

    VkQueue GraphicsQueue;
    VkQueue GraphicsAndComputeQueue;
    VkQueue& PresentQueue = GraphicsQueue;
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
    
    pro::Pipeline pipeline;
    VkSwapchainKHR swapchain;
    
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> framebuffers;

    VkCommandPool commandPool;
    VkDescriptorPool descPool;

    u8 currentFrame = 0;
    u32 imageIndex = 0;
    bool framebufferResized = false;

    VkSemaphore imageAvailableSemaphore[MaxFramesInFlight];
    VkSemaphore renderingFinishedSemaphore[MaxFramesInFlight];
    VkFence InFlightFence[MaxFramesInFlight];
    VkDescriptorSet descSet[MaxFramesInFlight];
    VkDescriptorSetLayout descSetLayouts[MaxFramesInFlight];
    VkCommandBuffer drawBuffers[MaxFramesInFlight];

    VkShaderModule LoadShaderModule(const char* path);
    void ResizeRenderWindow(const VkExtent2D newExtent, const bool forceWindowResize = false);

    void Initialize();
    void ProcessEvent(SDL_Event* event);

    inline VkCommandBuffer GetDrawBuffer() { return drawBuffers[currentFrame]; };
    bool BeginRender();
    void EndRender();

    NANO_SINGLETON_FUNCTION RendererSingleton* GetSingleton() {
        static RendererSingleton global;
        return &global;
    }
};
static RendererSingleton* Renderer = RendererSingleton::GetSingleton();

#endif