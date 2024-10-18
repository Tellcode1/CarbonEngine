#ifndef __CENGINE_INIT_H
#define __CENGINE_INIT_H

#ifdef __cplusplus
	extern "C" {
#endif

typedef struct ctx ctx;

#include "stdafx.h"
#include "vkstdafx.h"
#include "cengine.h"
#include "cgvector.h"
#include "cgstring.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

void ctx_initialize(const char* window_title, u32 window_width, u32 window_height);

static SDL_UNUSED const char* ValidationLayers[] = {  
	"VK_LAYER_KHRONOS_validation",
};

static SDL_UNUSED const char* RequiredInstanceExtensions[] = {
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
};

static SDL_UNUSED const char* WantedInstanceExtensions[] = {
	// VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
};

static SDL_UNUSED const char* WantedDeviceExtensions[] = {
	// VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
};

static SDL_UNUSED const char* RequiredDeviceExtensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

// ! FIND A WAY TO CHECK IF FEATURE IS AVAILABLE

const static VkPhysicalDeviceFeatures WantedFeatures = {
	.samplerAnisotropy = VK_TRUE,
};

static __attribute_maybe_unused__ VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

	printf("vkdebug: %s\n", pCallbackData->pMessage);

    return VK_FALSE;
}

#ifdef __cplusplus
	}
#endif

#endif//__CENGINE_INIT_HPP