#ifndef __DYNAMIC_DESCRIPTORS__
#define __DYNAMIC_DESCRIPTORS__

#include "stdafx.hpp"
#include "Bootstrap.hpp"
#include "Renderer.hpp"

using vec2 = glm::vec2;

// struct DynamicTextureDescriptor {
//     VkDescriptorPool pool;
//     const std::vector<VkDescriptorSet> set;
//     const std::vector<VkDescriptorSetLayout> setLayout;

//     inline operator VkDescriptorPool() const {
//         return pool;
//     }

//     explicit DynamicTextureDescriptor(Renderer* rd, const std::vector<Image>& images);
//     ~DynamicTextureDescriptor() = default;
// };

// struct TextureArray {
//     VkImage textureArrayImage = VK_NULL_HANDLE;
//     VkImageView textureArrayImageView = VK_NULL_HANDLE;
//     VkDeviceMemory textureArrayImageMemory = VK_NULL_HANDLE;
//     VkSampler textureArraySampler = VK_NULL_HANDLE;
//     u32 count = 0;

//     TextureArray(Renderer* rd, u32 count);
//     ~TextureArray() = default;
// };

template <typename T>
struct VertexBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    void* mappedMemory;
    VulkanContext* ctx;

    inline operator VkBuffer() {
        return buffer;
    }

    VertexBuffer(VulkanContext* ctx, const T& vertices);

    inline void Update(const T& vertices) {
        memcpy(mappedMemory, vertices.data(), static_cast<u64>(sizeof(vertices[0]) * vertices.size()));
    }

    ~VertexBuffer() {
        // vkUnmapMemory(ctx->device, memory);
    }
};

struct IndexBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    void* mappedMemory;
    VulkanContext* ctx;

    inline operator VkBuffer() {
        return buffer;
    }

    IndexBuffer(VulkanContext* ctx, const std::vector<u16>& indices);

    inline void Update(const std::vector<u16>& indices) {
        memcpy(mappedMemory, indices.data(), static_cast<u64>(sizeof(indices[0]) * indices.size()));
    }

    ~IndexBuffer() = default;
};

struct DynamicDescriptor {
    VulkanContext* ctx;
    VkDescriptorPool pool;
    VkDescriptorSet set[MaxFramesInFlight];
    VkDescriptorSetLayout setLayout[MaxFramesInFlight];

    DynamicDescriptor();
    ~DynamicDescriptor() = default;
};

#endif

template <typename T>
VertexBuffer<T>::VertexBuffer(VulkanContext *ctx, const T &vertices)
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    const auto& device = ctx->device;

    vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = bootstrap::GetMemoryType(ctx->physDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkAllocateMemory(device, &allocInfo, nullptr, &memory);
    vkBindBufferMemory(device, buffer, memory, 0);

    vkMapMemory(ctx->device, memory, 0, bufferSize, 0, &mappedMemory);
    Update(vertices);
}
