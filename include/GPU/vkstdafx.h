#ifndef __VK_STDAFX_H__
#define __VK_STDAFX_H__

#include "../../common/stdafx.h"
#include "../GPU/format.h"

NOVA_HEADER_START;

#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__linux) || defined(__unix)
#define VK_USE_PLATFORM_XCB_KHR
#else
#error implement
#endif

#define NVVK_FORWARD_DECLARE(s)                                                                                                                        \
  typedef struct s##_T s##_T;                                                                                                                        \
  typedef s##_T *s;

NVVK_FORWARD_DECLARE(VkInstance);
NVVK_FORWARD_DECLARE(VkDevice);
NVVK_FORWARD_DECLARE(VkPhysicalDevice);
NVVK_FORWARD_DECLARE(VkSurfaceKHR);
NVVK_FORWARD_DECLARE(VkDebugUtilsMessengerEXT);
NVVK_FORWARD_DECLARE(VkQueue);

extern VkInstance instance;
extern VkDevice device;
extern VkPhysicalDevice physDevice;
extern VkSurfaceKHR surface;
extern struct SDL_Window *window;
extern VkDebugUtilsMessengerEXT debugMessenger;

extern NVFormat SwapChainImageFormat;
extern u32 SwapChainColorSpace;
extern u32 SwapChainImageCount;
extern u32 Samples;

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

extern u32 MAX_SAMPLES;
extern unsigned char SUPPORTS_MULTISAMPLING;
extern float MAX_ANISOTROPY;

NOVA_HEADER_END;

#endif //__VK_STDAFX_H__