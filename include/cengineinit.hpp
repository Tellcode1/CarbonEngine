#ifndef __CENGINE_INIT_HPP
#define __CENGINE_INIT_HPP

struct ctx;

#include "stdafx.h"
#include "vkstdafx.h"
#include "../external/volk/volk.h"
#include "containers/cvector.hpp"
#include "containers/cstring.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

struct ctx {
	static cvector<cstring_view> availableDeviceExtensions;
	static cvector<cstring_view> availableInstanceExtensions;
	static VkPhysicalDeviceFeatures availableFeatures;
	
	static void Initialize(const char* title, u32 windowWidth, u32 windowHeight);
};

const cvector<const char*> ValidationLayers = {  
	"VK_LAYER_KHRONOS_validation",
};

const cvector<const char*> RequiredInstanceExtensions = {
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
};

const cvector<const char*> WantedInstanceExtensions = {
	// VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
};

constexpr static const char* WantedDeviceExtensions[] = {
	// VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
};

const cvector<const char*> RequiredDeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

// ! FIND A WAY TO CHECK IF FEATURE IS AVAILABLE

constexpr static VkPhysicalDeviceFeatures WantedFeatures = {
	.samplerAnisotropy = VK_TRUE,
};

static __attribute_maybe_unused__ VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

	crawstring preceder = "[VkDBG]";
	crawstring succeeder = "\n";

	switch(messageSeverity)
	{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			preceder += "[WARN]";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			preceder += "[INFO]";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			preceder += "[ERR]";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			preceder += "[VERB]";
			break;
		default:
			preceder += "[UNKNOWN SEVERITY]";
			break;
	}

	switch(messageType)
	{
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
			preceder += "[GEN] ";
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
			preceder += "[VALTION] ";
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
			preceder += "[PERF] ";
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
			preceder += "[ADDR BIND] ";
			break;
		default:
			preceder += "[UNKNOWN TYPE] ";
			break;
	}

	printf("%s%s%s", preceder.c_str(), pCallbackData->pMessage, succeeder.c_str());

    return VK_FALSE;
}

#endif//__CENGINE_INIT_HPP