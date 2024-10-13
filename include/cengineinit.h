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

void ctx_initialize(const char* title, u32 windowWidth, u32 windowHeight, struct cg_device_t *device);

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

	cg_string_t *preceder = cg_string_init_str("[VkDBG]");
	cg_string_t *succeeder = cg_string_init_str("\n");

	switch(messageSeverity)
	{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			cg_string_append(preceder, "[WARN]");
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			cg_string_append(preceder, "[INFO]");
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			cg_string_append(preceder, "[ERR]");
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			cg_string_append(preceder, "[VERB]");
			break;
		default:
			cg_string_append(preceder, "[UNKNOWN SEVERITY]");
			break;
	}

	switch(messageType)
	{
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
			cg_string_append(preceder, "[GEN] ");
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
			cg_string_append(preceder, "[VALTION] ");
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
			cg_string_append(preceder, "[PERF] ");
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
			cg_string_append(preceder, "[ADDR BIND] ");
			break;
		default:
			cg_string_append(preceder, "[UNKNOWN TYPE] ");
			break;
	}

	printf("%s%s%s", cg_string_data(preceder), pCallbackData->pMessage, cg_string_data(succeeder));

	cg_string_destroy(preceder);
	cg_string_destroy(succeeder);
    return VK_FALSE;
}

#ifdef __cplusplus
	}
#endif

#endif//__CENGINE_INIT_HPP