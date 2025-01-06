#ifndef __STDAFX_H__
#define __STDAFX_H__

#include "../stdafx.h"

#if defined(_WIN32)
    #define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__linux) || defined(__unix)
    #define VK_USE_PLATFORM_XCB_KHR
#else
    #error implement
#endif

#include "../../external/volk/volk.h"
#include "format.h"

static inline void SetObjectName(VkDevice device, uint64_t objectHandle, VkObjectType objectType, const char* name) {
    VkDebugUtilsObjectNameInfoEXT nameInfo = {};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.objectType = objectType;
    nameInfo.objectHandle = objectHandle;
    nameInfo.pObjectName = name;

    PFN_vkSetDebugUtilsObjectNameEXT func = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");
    if (func) {
        func(device, &nameInfo);
    } else {
        LOG_ERROR("no vkSetDebugUtilsObjectNameEXT");
    }
}


extern VkInstance instance;
extern VkDevice device;
extern VkPhysicalDevice physDevice;
extern VkSurfaceKHR surface;
extern struct SDL_Window *window;
extern VkDebugUtilsMessengerEXT debugMessenger;

extern lunaFormat SwapChainImageFormat;
extern VkColorSpaceKHR SwapChainColorSpace;
extern u32 SwapChainImageCount;
extern VkSampleCountFlagBits Samples;

extern u32 GraphicsFamilyIndex;
extern u32 PresentFamilyIndex;
extern u32 ComputeFamilyIndex;
extern u32 TransferQueueIndex;
extern u32 GraphicsAndComputeFamilyIndex;

extern VkQueue GraphicsQueue;
extern VkQueue GraphicsAndComputeQueue;
extern VkQueue PresentQueue;
extern VkQueue ComputeQueue;
extern VkQueue TransferQueue;

extern VkSampleCountFlagBits MAX_SAMPLES;
extern unsigned char SUPPORTS_MULTISAMPLING;
extern float MAX_ANISOTROPY;

#endif //__STDAFX_H__