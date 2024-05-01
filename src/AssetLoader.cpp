#include "AssetLoader.hpp"
#include <filesystem>

Image Loader::LoadImage(const char *path)
{
    i32 channels;
    
    Image img{};
    img.data = stbi_load(path, &img.width, &img.height, &channels, STBI_rgb_alpha);

    if(img.data == nullptr) {
        throw std::runtime_error("Failed to load image: " + std::string(path));
    }

    return img;
}

std::vector<char> Loader::LoadBinary(const char *path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file at path: " << path << std::endl;
        throw std::runtime_error("Failed to open file");
    }

    const i64 fileSize = static_cast<i64>(file.tellg());
    assert(fileSize != -1);

    std::vector<char> fileData(fileSize);
    file.seekg(0);
    file.read(fileData.data(), fileSize);
    file.close();

    return fileData;
}

VkShaderModule Loader::LoadShaderModule(const char *pathToShaderCode, VkDevice device)
{
    const std::vector<char> shaderCode = LoadBinary(pathToShaderCode);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size();
    createInfo.pCode = reinterpret_cast<const u32*>(shaderCode.data());
    
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module");
    }

    return shaderModule;
}

// int Loader::LoadCache(const char *name, std::vector<char> &cache)
// {
//     std::ifstream file(name, std::ios::ate | std::ios::binary);
    
//     if (!file.is_open())
//         return 0;
    
//     const i64 fileSize = static_cast<i64>(file.tellg());
//     assert(fileSize != -1);

//     cache.resize(fileSize);

//     file.seekg(0);
//     file.read(cache.data(), fileSize);
//     file.close();
    
//     return 1;
// }

// std::vector<std::string> Atlas::QueryAvailableImages()
// {
//     return std::vector<std::string>();
// }
