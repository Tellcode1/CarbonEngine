#ifndef __VK_STDAFX_H__
#define __VK_STDAFX_H__

#include "../../common/stdafx.h"

#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__linux) || defined(__unix)
#define VK_USE_PLATFORM_XCB_KHR
#else
#error implement
#endif

#include "../../external/volk/volk.h"
#include "format.h"

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

#endif //__VK_STDAFX_H__