#ifndef __RENDERER__
#define __RENDERER__

#include "stdafx.hpp"
#include "pro.hpp"
#include "Bootstrap.hpp"
#include "AssetLoader.hpp"
#include "TextRenderer.hpp"

static constexpr bool VSyncEnabled = false;
static constexpr u8 MaxFramesInFlight = VSyncEnabled ? 1 : 3;

struct VulkanContext;
struct Texture;
// struct Texture;

struct VulkanContext {
    VkInstance instance;
    VkPhysicalDevice physDevice;
    VkDevice device;
    VkSurfaceKHR surface;
    SDL_Window* window;
};

// struct Character
// {
//     bootstrap::Image image;
//     glm::ivec2 size;
//     glm::ivec2 bearing;
//     u32 advance;
// };

extern VulkanContext CreateVulkanContext(const char* title, u32 windowWidth = 800, u32 windowHeight = 600);

struct Renderer
{
    VulkanContext* ctx;

    VkExtent2D renderArea{ 800, 600 };
    VkQueue graphicsQueue;
    
    pro::Pipeline pipeline;
    VkSwapchainKHR swapchain;
    
    VkFormat format;
    VkColorSpaceKHR colorSpace;
    u32 imageCount;

    u32 graphicsFamily;
    u32 presentFamily;
    u32 computeFamily;

    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> framebuffers;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> drawBuffers{ MaxFramesInFlight };

    VkSemaphore imageAvailableSemaphore[MaxFramesInFlight];
    VkSemaphore renderingFinishedSemaphore[MaxFramesInFlight];
    VkFence InFlightFence[MaxFramesInFlight];

    u8 currentFrame = 0;
    u32 imageIndex = 0;
    bool framebufferResized = false;

    VkDescriptorPool descPool{};
    VkDescriptorSet descSet[MaxFramesInFlight];
    VkDescriptorSetLayout descSetLayouts[MaxFramesInFlight];

    std::vector<bootstrap::Image> images{};
    bootstrap::Image LoadImage(const char* path);
    bootstrap::Image LoadImage(VkFormat format, u64 width, u64 height, void* data);

    void Initialize(VulkanContext* ctx);
    Renderer() = default;
    ~Renderer() = default;

    void ResizeRenderWindow(const VkExtent2D newExtent, const bool forceWindowResize = false);

    inline VkCommandBuffer GetDrawBuffer() { return drawBuffers[currentFrame]; };
    bool BeginRender();
    void EndRender();

    static Renderer* GetSingleton() {
        static Renderer global;
        return &global;
    }
};

static Renderer* RD = Renderer::GetSingleton();
// extern bool RendererInit(Interface const* interface);

struct Texture {
    VkImage image;
    VkImageView view;
    VkDeviceMemory imageMemory;

    Texture(Renderer* rd);
};

__attribute__((unused))
static VkSampler CreateSampler(VkDevice device) {
    VkSamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.anisotropyEnable = VK_FALSE;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = 0.0f;

    VkSampler sampler;
	vkCreateSampler(device, &samplerCreateInfo, nullptr, &sampler);

    return sampler;
}

#endif