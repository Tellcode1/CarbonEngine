#include "DynamicDescriptors.hpp"

// DynamicTextureDescriptor::DynamicTextureDescriptor(Renderer* rd, const std::vector<Image>& images)
// {
                             
// }

// TextureArray::TextureArray(Renderer* rd, u32 count)
// {

// }

// template <typename T>
// VertexBuffer<T>::VertexBuffer(Interface *ctx, const T& vertices)
// {
//     VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

//     VkBufferCreateInfo bufferInfo{};
//     bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//     bufferInfo.size = bufferSize;
//     bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
//     bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

//     const auto& device = ctx->device;

//     vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);

//     VkMemoryRequirements memRequirements;
//     vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

//     VkMemoryAllocateInfo allocInfo{};
//     allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//     allocInfo.allocationSize = memRequirements.size;
//     allocInfo.memoryTypeIndex = bootstrap::GetMemoryType(ctx->physDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

//     vkAllocateMemory(device, &allocInfo, nullptr, &memory);
//     vkBindBufferMemory(device, buffer, memory, 0);

//     vkMapMemory(ctx->device, memory, 0, bufferSize, 0, &mappedMemory);
//     Update(vertices);
// }

IndexBuffer::IndexBuffer(VulkanContext *ctx, const std::vector<u16> &indices)
{
    const VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    const VkDevice device = ctx->device;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

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
    Update(indices);
}

DynamicDescriptor::DynamicDescriptor()
{
    // VkDescriptorPoolSize poolSizes[] = {

    // };
    // VkDescriptorPoolCreateInfo poolCreateInfo{};
    
    // vkCreateDescriptorPool();
}
