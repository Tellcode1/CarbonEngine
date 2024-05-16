#ifndef __IMAGE_HPP___
#define __IMAGE_HPP___

#include "stdafx.hpp"

enum ImageLoadStatus : u32
{
    IMAGE_LOAD_SUCCESS        = 0,
    IMAGE_LOAD_INVALID_VALUE  = 1,
    IMAGE_LOAD_FILE_NOT_FOUND = 2,
    IMAGE_LOAD_OUT_OF_MEMORY  = 3,
    IMAGE_LOAD_UNKNOWN_ERROR  = UINT32_MAX,
};

enum ImageLoadType
{
    IMAGE_LOAD_TYPE_FROM_DISK   = 0,
    IMAGE_LOAD_TYPE_FROM_MEMORY = 1,
};

struct Image
{
    VkImage image;
    VkImageView view;
    VkDeviceMemory memory;
    u32 width, height;
};

struct ImageLoadInfo
{
    /* REQUIRED */
    VkFormat format;
    VkSampleCountFlagBits samples;

    /* If loadType == IMAGE_LOAD_FROM_DISK */
    const char* pPath;

    /* If loadType == IMAGE_LOAD_FROM_MEMORY */
    u32 width, height;
    u8* data;
};

struct ImagesSingleton
{
    ImageLoadStatus LoadImage(ImageLoadInfo* pInfo, Image* dstImage, ImageLoadType loadType = IMAGE_LOAD_TYPE_FROM_DISK);

    static ImagesSingleton* GetSingleton() {
        static ImagesSingleton global{};
        return &global;
    }    
};
static ImagesSingleton* Images = ImagesSingleton::GetSingleton();

#endif