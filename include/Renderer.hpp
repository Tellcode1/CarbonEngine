#ifndef __RENDERER__
#define __RENDERER__

#include "stdafx.hpp"

static constexpr u8 MaxFramesInFlight = 2;
static constexpr glm::vec<2, u16> DefaultExtent = { 800, 600 };

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

    glm::vec<2, u16> RenderArea = DefaultExtent;

    u32 GraphicsFamilyIndex;
    u32 PresentFamilyIndex;
    u32 ComputeFamilyIndex;
    u32 GraphicsAndComputeFamilyIndex;

    VkQueue GraphicsQueue;
    VkQueue GraphicsAndComputeQueue;
    VkQueue& PresentQueue = GraphicsQueue;
    VkQueue ComputeQueue;
    VkQueue TransferQueue;

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