#ifndef __STDAFX_HPP__
#define __STDAFX_HPP__

#include "defines.h"

#if defined(CB_OS_WINDOWS)
    #define VK_USE_PLATFORM_WIN32_KHR
#elif defined(CB_OS_LINUX)
    #define VK_USE_PLATFORM_XCB_KHR
#elif defined(CB_OS_MAC)
    #define VK_USE_PLATFORM_MACOS_MVK
#elif defined(CB_OS_ANDROID)
    #error implement
#endif

#include "../external/volk/volk.h"

extern VkFormat SwapChainImageFormat;
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

extern VkInstance 							 instance;
extern VkDevice 							 device;
extern VkPhysicalDevice 					 physDevice;
extern VkSurfaceKHR 				         surface;
extern struct SDL_Window* 				     window;
extern VkDebugUtilsMessengerEXT 			 debugMessenger;

extern VkSampleCountFlagBits MAX_SAMPLES;
extern unsigned char SUPPORTS_MULTISAMPLING;
extern f32 MAX_ANISOTROPY;

#endif //__STDAFX_HPP__