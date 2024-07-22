#ifndef __RENDERER__
#define __RENDERER__

#include "cengine.hpp"

constexpr static SDL_Scancode EXIT_KEY = SDL_SCANCODE_ESCAPE;
constexpr static u8 MaxFramesInFlight = 2;
constexpr static VkExtent2D DefaultExtent = { 800, 600 }; // Starting window size

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
;
    static u32 imageIndex;

    static std::vector<FrameRenderData> renderData;
    static VkCommandBuffer drawBuffers[MaxFramesInFlight];

    static void initialize();

    static VkCommandBuffer CARBON_FORCE_INLINE GetDrawBuffer() { return drawBuffers[cengine::get_current_frame()]; }
    static bool BeginRender();
    static void EndRender();

    private:
    static void _SignalResize();
};

#endif