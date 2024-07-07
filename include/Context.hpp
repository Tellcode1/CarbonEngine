#ifndef __CONTEXT_HPP__
#define __CONTEXT_HPP__

#include "stdafx.hpp"

const std::vector<const char*> ValidationLayers = {  
	"VK_LAYER_KHRONOS_validation",
};
const std::vector<const char*> RequiredInstanceExtensions = {
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
};

const std::vector<const char*> WantedInstanceExtensions = {
	// VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
};

constexpr static const char* WantedDeviceExtensions[] = {
	// VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
};

const std::vector<const char*> RequiredDeviceExtensions = {
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

	std::string preceder = "[VkDebugUtilsMessengerEXT] ";
	std::string succeeder = "\n";

	switch(messageSeverity)
	{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			preceder += "[WARNING] ";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			preceder += "[INFO] ";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			preceder += "[ERROR] ";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			preceder += "[VERBOSE] ";
			break;
		default:
			preceder += "[UNKNOWN SEVERITY] ";
			break;
	}

	switch(messageType)
	{
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
			preceder += "[GENERAL] ";
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
			preceder += "[VALIDATION] ";
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
			preceder += "[PERFORMANCE] ";
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
			preceder += "[DEVICE ADDRESS BINDING] ";
			break;
		default:
			preceder += "[UNKNOWN TYPE] ";
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

	static std::unordered_set<std::string_view> availableDeviceExtensions;
	static std::unordered_set<std::string_view> availableInstanceExtensions;
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
