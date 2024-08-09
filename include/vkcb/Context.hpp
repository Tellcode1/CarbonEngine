#ifndef __CONTEXT_HPP__
#define __CONTEXT_HPP__

#include "vkcbstdafx.hpp"
#include "containers/cvector.hpp"
#include "containers/cstring.hpp"

#include <vector>

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
	.multiDrawIndirect = VK_TRUE,
	.samplerAnisotropy = VK_TRUE,
};

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

	cstring preceder = U"[VkDBG]";
	cstring succeeder = U"\n";

	switch(messageSeverity)
	{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			preceder += U"[WARN]";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			preceder += U"[INFO]";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			preceder += U"[ERR]";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			preceder += U"[VERB]";
			break;
		default:
			preceder += U"[UNKNOWN SEVERITY]";
			break;
	}

	switch(messageType)
	{
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
			preceder += U"[GEN] ";
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
			preceder += U"[VALTION] ";
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
			preceder += U"[PERF] ";
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
			preceder += U"[ADDR BIND] ";
			break;
		default:
			preceder += U"[UNKNOWN TYPE] ";
			break;
	}

	printf("%s%s%s", preceder.c_str(), pCallbackData->pMessage, succeeder.c_str());

    return VK_FALSE;
}

struct Context {
    static VkInstance instance;
    static VkDevice device;
    static VkPhysicalDevice physDevice;
    static VkSurfaceKHR surface;
    static SDL_Window* window;

	static VkDebugUtilsMessengerEXT debugMessenger;

	static cvector<cstring_view> availableDeviceExtensions;
	static cvector<cstring_view> availableInstanceExtensions;
	static VkPhysicalDeviceFeatures availableFeatures;

	static void Initialize(const char* title, u32 windowWidth, u32 windowHeight);
};
using ctx = Context; // ctx is much easier to use

static VkInstance& instance = ctx::instance;
static VkDevice& device = ctx::device;
static VkPhysicalDevice& physDevice = ctx::physDevice;
static VkSurfaceKHR& surface = ctx::surface;
static SDL_Window*& window = ctx::window;

#endif
