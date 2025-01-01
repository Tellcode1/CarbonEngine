#ifndef __STDAFX_H__
#define __STDAFX_H__

#include "defines.h"

#if defined(_WIN32)
    // #define VK_USE_PLATFORM_WIN32_KHR
    #error "piss your balls"
#elif defined(__linux) || defined(__unix)
    #define VK_USE_PLATFORM_XCB_KHR
#else
    #error implement
#endif

#include "../external/volk/volk.h"
#include "lunaFormat.h"

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