#ifndef __BOOTSTRAP__
#define __BOOTSTRAP__

#include "stdafx.hpp"

namespace bootstrap {
    // vkb::Instance CreateInstance();
    // vkb::PhysicalDevice GetSelectedDevice(const vkb::Instance& instance, const VkSurfaceKHR& surface);
    // VkSurfaceKHR CreateSurface(SDL_Window* window, const vkb::Instance& instance);
    // struct Instance {
    //     VkInstance instance;
    //     VkDebugUtilsMessengerEXT debugMessenger;

    //     inline operator VkInstance() {
    //         return instance;
    //     }
    //     inline VkInstance* operator &() {
    //         return &instance;
    //     }
    // };

    // VkInstance CreateInstance(const char* windowTitle);
    VkPhysicalDevice GetSelectedPhysicalDevice(const VkInstance& instance, const VkSurfaceKHR& surface);
    VkSurfaceKHR CreateSurface(SDL_Window* window, const VkInstance& instance);
    VkDevice CreateDevice();

    enum QueueType {
        GRAPHICS = VK_QUEUE_GRAPHICS_BIT,
        COMPUTE = VK_QUEUE_COMPUTE_BIT,
        TRANSFER = VK_QUEUE_TRANSFER_BIT,
    };

    u32 GetQueueIndex(VkPhysicalDevice physDevice, QueueType type);
    VkCommandPool CreateCommandPool(VkDevice device, VkPhysicalDevice physDevice, const QueueType queueType);

    struct Image {
        VkImage image;
        VkImageView view;
        VkSampler sampler;
        VkDeviceMemory memory;
    };

    struct DescriptorSet {
        VkDescriptorPool pool;
        VkDescriptorSet descSet;
        VkDescriptorSetLayout layout;

        inline operator VkDescriptorSet() const {
            return descSet;
        }
    };

    Image CreateImage(VkDevice device, VkPhysicalDevice physicalDevice, u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
    void TransitionImageLayout(VkDevice device, VkCommandPool pool, VkQueue queue, Image& img, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void TransitionImageLayout(VkDevice device, VkCommandPool pool, VkQueue queue, VkImage img, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    uint32_t GetMemoryType(const VkPhysicalDevice& physicalDevice, const uint32_t MemoryTypeBits, const VkMemoryPropertyFlags MemoryProperties);
    VkCommandBuffer BeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
    void EndSingleTimeCommands(VkDevice device, VkCommandBuffer buffer, VkQueue queue, VkCommandPool commandPool);
    void CopyBufferToImage(VkBuffer buffer, VkDevice device, VkCommandPool pool, VkQueue queue, VkImage image, uint32_t width, uint32_t height);
	VkBuffer CreateBuffer(VkPhysicalDevice& physicalDevice, VkDevice& device, VkDeviceMemory& bufferMemory, VkDeviceSize size, const VkBufferUsageFlags usageFlags, const VkMemoryPropertyFlags propertyFlags);
    void StageBufferTransfer(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer dstBuffer, void* data, size_t size);
    VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
}

#endif