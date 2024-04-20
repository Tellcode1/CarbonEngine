#ifndef __ASSET_LOADER__
#define __ASSET_LOADER__

#include "stdafx.hpp"

struct Image {
    std::string name;
    i32 width = 0, height = 0;
    u8* data = nullptr;
};

struct TexturePositions {
    glm::vec2 positions[4];
};

namespace Loader {
    /*
    *   Loads a standard image using stb_image.
    */
    Image LoadImage(const char* path);

    /*
    *   Parameter "path" may not be nullptr.
    */
    std::vector<char> LoadBinary(const char *path);

    /*
    *   Helper function for creating a shader module using LoadBinary().
    */
   VkShaderModule LoadShaderModule(const char* pathToShaderCode, VkDevice device);
}

// namespace Atlas {
//    /*
//    *    Loads an image from the atlas and returns its texture coordinates.
//    *    This does not return an image!
//    *    Throws std::runtime_error if it does not find the image.
//    *    Use Atlas::QueryAvailableImages to get the list of available images.
//    */
//    Atlas_OutputImage LoadTextureFromAtlas(const char* filePath = "atlas.hpp");

//    /*
//    *    Finds all the available images in the atlas and returns them.
//    */
//    std::vector<std::string> QueryAvailableImages();
// }

#endif