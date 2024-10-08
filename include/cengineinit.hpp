#ifndef __CENGINE_INIT_HPP
#define __CENGINE_INIT_HPP

struct ctx;

#include "stdafx.h"
#include "vkstdafx.h"
#include "../external/volk/volk.h"
#include "containers/cvector.h"
#include "containers/cstring.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

struct ctx {
	static cvector_t * /* cstring_t * */ availableDeviceExtensions;
	static cvector_t * /* cstring_t * */ availableInstanceExtensions;
	static VkPhysicalDeviceFeatures availableFeatures;
	
	static void Initialize(const char* title, u32 windowWidth, u32 windowHeight);
};

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

	cstring_t *preceder = cstring_init_str("[VkDBG]");
	cstring_t *succeeder = cstring_init_str("\n");

	switch(messageSeverity)
	{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			cstring_append(preceder, "[WARN]");
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			cstring_append(preceder, "[INFO]");
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			cstring_append(preceder, "[ERR]");
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			cstring_append(preceder, "[VERB]");
			break;
		default:
			cstring_append(preceder, "[UNKNOWN SEVERITY]");
			break;
	}

	switch(messageType)
	{
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
			cstring_append(preceder, "[GEN] ");
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
			cstring_append(preceder, "[VALTION] ");
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
			cstring_append(preceder, "[PERF] ");
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
			cstring_append(preceder, "[ADDR BIND] ");
			break;
		default:
			cstring_append(preceder, "[UNKNOWN TYPE] ");
			break;
	}

	printf("%s%s%s", cstring_data(preceder), pCallbackData->pMessage, cstring_data(succeeder));

	cstring_destroy(preceder);
	cstring_destroy(succeeder);
    return VK_FALSE;
}

#endif//__CENGINE_INIT_HPP