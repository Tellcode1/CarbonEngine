#include "../include/crenderdevice.h"
#include "../include/defines.h"

#include "../include/containers/cvector.h"
#include "../include/containers/cstring.h"

#include "../external/volk/volk.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include "crenderdevice.h"

typedef VkBool32 (*crenderdevice_debug_messenger) (
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
);

typedef struct crdevice_t {

    // these shouldn't be here.
    // move them to like a device info static struct.
    // from here
    VkFormat SwapChainImageFormat;
    VkColorSpaceKHR SwapChainColorSpace;
    u32 SwapChainImageCount;
    VkSampleCountFlagBits Samples;

    u32 GraphicsFamilyIndex;
    u32 PresentFamilyIndex;
    u32 ComputeFamilyIndex;
    u32 TransferQueueIndex;
    u32 GraphicsAndComputeFamilyIndex;

    VkQueue GraphicsQueue;
    VkQueue GraphicsAndComputeQueue;
    VkQueue PresentQueue;
    VkQueue ComputeQueue;
    VkQueue TransferQueue;

    VkSampleCountFlagBits MAX_SAMPLES;
    unsigned char SUPPORTS_MULTISAMPLING;
    f32 MAX_ANISOTROPY;
    //to here

    VkInstance 							 instance;
    VkDevice 							 vkdevice;
    VkPhysicalDevice 					 physDevice;
    VkSurfaceKHR 				         surface;
    struct SDL_Window* 				     window;
    VkDebugUtilsMessengerEXT 			 debugMessenger;
    crenderdevice_debug_messenger        debug_messenger_fn;

    cvector_t * /* cstring_t * */ availableDeviceExtensions;
    cvector_t * /* cstring_t * */ availableInstanceExtensions;
    VkPhysicalDeviceFeatures availableFeatures;
} crdevice_t;

static const char* ValidationLayers[] = {  
	"VK_LAYER_KHRONOS_validation",
};

static const char* RequiredInstanceExtensions[] = {
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
};

static const char* WantedInstanceExtensions[] = {

};

static const char* WantedDeviceExtensions[] = {

};

static const char* RequiredDeviceExtensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

cvector_t *setify(u32 i1, u32 i2, u32 i3, u32 i4) {
	cvector_t *ret = cvector_init(sizeof(u32), 4);
    const u32 nums[4] = {i1, i2, i3, i4};
	for (int j = 0; j < array_len(nums); j++) {
		bool already_in = false;
        const u32 e = nums[j];
		for (int i = 0; i < cvector_size(ret); i++) {
			if (e == *(u32 *)cvector_get(ret, i))
				already_in = true;

		}
		if (!already_in) {
			cvector_push_back(ret, &e);
		}
	}
	return ret;
}

VkInstance CreateInstance(crdevice_t *device, const char* title) {
	if (volkInitialize() != VK_SUCCESS) {
		LOG_AND_ABORT("Volk could not initialize. You probably don't have the vulkan loader installed. I can't do anything about that.");
	}

    VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.applicationVersion = VK_API_VERSION_1_0;
	appInfo.apiVersion = VK_API_VERSION_1_0;
	appInfo.pApplicationName = title;
	appInfo.pEngineName = "Carbon";
	appInfo.engineVersion = 0;

	uint32_t SDLExtensionCount = 0;
	SDL_Vulkan_GetInstanceExtensions(device->window, &SDLExtensionCount, NULL);
	cvector_t * /* const char* */ SDLExtensions = cvector_init(sizeof(const char *), SDLExtensionCount);
	SDL_Vulkan_GetInstanceExtensions(device->window, &SDLExtensionCount, (const char **)cvector_data(SDLExtensions));

	cvector_t *enabledExtensions = cvector_init(sizeof(const char *), array_len(RequiredInstanceExtensions));

	for (int i = 0; i < array_len(RequiredInstanceExtensions); i++) {
		cvector_push_back(enabledExtensions, RequiredInstanceExtensions[i]);
	}

	for (int i = 0; i < SDLExtensionCount; i++) {
		cvector_push_back(enabledExtensions, cvector_get(SDLExtensions, i));
	}

	u32 extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
	cvector_t * /* VkExtensionProperties */ extensions = cvector_init(sizeof(VkExtensionProperties), extensionCount);
	vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, (VkExtensionProperties *)cvector_data(extensions));

	for (u32 i = 0; i < extensionCount; i++) {
		const char* name = ((VkExtensionProperties *)cvector_data(extensions))[i].extensionName;
		for(u32 j = 0; j < array_len(WantedInstanceExtensions); j++) {
            const char *want = WantedDeviceExtensions[j];
			if(strcmp(name, want) == 0) {
				cvector_push_back(enabledExtensions, &name);
				cstring_t *name_str = cstring_init_str(name);
				cvector_push_back(device->availableDeviceExtensions, &name_str);
				break;
			}
        }
	}

	VkInstanceCreateInfo instanceCreateinfo = {};
	instanceCreateinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateinfo.pApplicationInfo = &appInfo;
	instanceCreateinfo.enabledExtensionCount = cvector_size(enabledExtensions);
	instanceCreateinfo.ppEnabledExtensionNames = (const char **)cvector_data(enabledExtensions);

	#ifdef DEBUG

	bool validationLayersAvailable = false;
	uint32_t layerCount = 0;

	vkEnumerateInstanceLayerProperties(&layerCount, NULL);
	cvector_t * /* VkLayerProperties */ layerProperties = cvector_init(sizeof(VkLayerProperties), layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, (VkLayerProperties *)cvector_data(layerProperties));

	if(array_len(ValidationLayers) != 0)
	{
		for(u32 j = 0; j < array_len(ValidationLayers); j++)
		{
			for (uint32_t i = 0; i < layerCount; i++)
				if (strcmp(ValidationLayers[j], ((VkLayerProperties *)cvector_data(layerProperties))[i].layerName) == 0)
					validationLayersAvailable = true;
		}

		if (!validationLayersAvailable)
		{
			LOG_ERROR("VALIDATION LAYERS COULD NOT BE LOADED\nRequested layers:\n");
			for(u32 j = 0; j < array_len(ValidationLayers); j++)
				printf("\t%s\n", ValidationLayers[j]);

			printf("Instance provided these layers (i.e. These layers are available):\n");
			for(uint32_t i = 0; i < layerCount; i++)
				printf("\t%s\n", ((VkLayerProperties *)cvector_data(layerProperties))[i].layerName);

			printf("But instance asked for (i.e. are not available):\n");

			cvector_t * /* const char* */ missingLayers = cvector_init(sizeof(const char *), 16);
		
			for(u32 j = 0; j < array_len(ValidationLayers); j++)
			{
				bool layerAvailable = false;
				for (uint32_t i = 0; i < layerCount; i++)
					if (strcmp(ValidationLayers[j], ((VkLayerProperties *)cvector_data(layerProperties))[i].layerName) == 0)
					{
						layerAvailable = true;
						break;
					}
				if(!layerAvailable) cvector_push_back(missingLayers, &ValidationLayers[j]);
			}
			for(int i = 0; i < cvector_size(missingLayers); i++)
				printf("\t%s\n", (const char *)cvector_get(missingLayers, i));
			
			cvector_destroy(missingLayers);
			
			abort();
		}

		instanceCreateinfo.enabledLayerCount = array_len(ValidationLayers);
		instanceCreateinfo.ppEnabledLayerNames = ValidationLayers;
	}

	#else

	instanceCreateinfo.enabledLayerCount = 0;
	instanceCreateinfo.ppEnabledLayerNames = NULL;

	#endif

    VkInstance instance;
	vkCreateInstance(&instanceCreateinfo, NULL, &instance);

	#ifdef DEBUG
	if (validationLayersAvailable)
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
									VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
									VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
								VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
								VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = device->debug_messenger_fn;

		auto _CreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

		if (_CreateDebugUtilsMessenger) {
			if (_CreateDebugUtilsMessenger(instance, &createInfo, NULL, &device->debugMessenger) != VK_SUCCESS)
				LOG_ERROR("Debug messenger failed to initialize");
			else
				LOG_DEBUG("Debug messenger successfully set up.");
		}
		else
			LOG_ERROR("vkCreateDebugUtilsMessengerEXT proc address not found");
	}
	#endif

	cvector_destroy(SDLExtensions);
	cvector_destroy(extensions);
	cvector_destroy(enabledExtensions);
	cvector_destroy(layerProperties);

	volkLoadInstance(instance);
    return instance;
}

void PrintDeviceInfo(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties properties;	
	vkGetPhysicalDeviceProperties(device, &properties);

	const char* deviceTypeStr;
	switch(properties.deviceType) {
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			deviceTypeStr = "Discrete";
			break;
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			deviceTypeStr = "Integrated";
			break;
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			deviceTypeStr = "Software/CPU";
			break;
		default:
			deviceTypeStr = "Unknown";
			break;
	}

	// I think it looks cleaner this way
	LOG_INFO(
		"(%s) %s VK API Version %d.%d",
		deviceTypeStr, properties.deviceName,
		properties.apiVersion >> 22, (properties.apiVersion >> 12) & 0x3FF
	);
}

VkPhysicalDevice ChoosePhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {

	uint32_t physDeviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &physDeviceCount, NULL);
	
	if(physDeviceCount == 0) {
		LOG_AND_ABORT("Huuuhhh??? No physical devices found? Are you running this on a banana???\n");
	}

	cvector_t * /* VkPhysicalDevice */ physicalDevices = cvector_init(sizeof(VkPhysicalDevice), physDeviceCount);
	vkEnumeratePhysicalDevices(instance, &physDeviceCount, (VkPhysicalDevice *)cvector_data(physicalDevices));

	for(u32 i = 0; i < physDeviceCount; i++) {
		const VkPhysicalDevice device = ((VkPhysicalDevice *)cvector_data(physicalDevices))[i];

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, NULL);

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, NULL);
		
		bool extensionsAvailable = true;

		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);
		cvector_t * /* VkExtensionProperties */ availableExtensions = cvector_init(sizeof(VkExtensionProperties), extensionCount);
		vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, (VkExtensionProperties *)cvector_data(availableExtensions));

		for (u32 k = 0; k < array_len(RequiredDeviceExtensions); k++) {
			bool validated = false;
            const char *extension = RequiredDeviceExtensions[k];
			for(u32 j = 0; j < extensionCount; j++) {
				if (strcmp(extension, ((VkExtensionProperties *)cvector_data(availableExtensions))[j].extensionName) == 0)
					validated = true;
			}
			if (!validated) {
				LOG_ERROR("Failed to validate extension with name: %s\n", extension);
				extensionsAvailable = false;
			}
		}

		cvector_destroy(availableExtensions);

		if (extensionsAvailable && formatCount > 0 && presentModeCount > 0) {
			LOG_INFO("Found device!");
			PrintDeviceInfo(device);
			return device;
		}
	}

	VkPhysicalDevice fallback = ((VkPhysicalDevice *)cvector_data(physicalDevices))[0];

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(fallback, &properties);

	LOG_ERROR("Could not find an appropriate device. Falling back to device 0: %s\n", properties.deviceName);

	PrintDeviceInfo(fallback);

	cvector_destroy(physicalDevices);

	return fallback;
}

VkDevice CreateDevice(const crdevice_t *device) {
	u32 queueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device->physDevice, &queueCount, NULL);
	cvector_t * /* VkQueueFamilyProperties */ queueFamilies = cvector_init(sizeof(VkQueueFamilyProperties), queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device->physDevice, &queueCount, (VkQueueFamilyProperties *)cvector_data(queueFamilies));

	// Clang loves complaining about these.
	u32 graphicsFamily = 0, presentFamily = 0, computeFamily = 0, transferFamily = 0;
	(void)graphicsFamily, (void)presentFamily, (void)computeFamily, (void)transferFamily;

	bool foundGraphicsFamily = false, foundPresentFamily = false, foundComputeFamily = false, foundTransferFamily = false;
		
	u32 i = 0;
	for (int j = 0; j < cvector_size(queueFamilies); j++) {
		const VkQueueFamilyProperties queueFamily = ((VkQueueFamilyProperties *)cvector_data(queueFamilies))[j];
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device->physDevice, i, device->surface, &presentSupport);
	
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			graphicsFamily = i;
			foundGraphicsFamily = true;
		}
		if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
			computeFamily = i;
			foundComputeFamily = true;
		}
		if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
			transferFamily = i;
			foundTransferFamily = true;
		}
		if (presentSupport) {
			presentFamily = i;
			foundPresentFamily = true;
		}
		if (foundGraphicsFamily && foundComputeFamily && foundPresentFamily && foundTransferFamily)
			break;
	
		i++;
	}
	
	cvector_t * /* u32 */ uniqueQueueFamilies = setify(graphicsFamily, presentFamily, computeFamily, transferFamily);
	cvector_t * /* VkDeviceQueueCreateInfo */ queueCreateInfos = cvector_init(sizeof(VkDeviceQueueCreateInfo), cvector_size(uniqueQueueFamilies));

	const float queuePriority = 1.0f;

	for (int i = 0; i < cvector_size(uniqueQueueFamilies); i++)
	{
		VkDeviceQueueCreateInfo queueInfo = {};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = ((u32 *)cvector_data(uniqueQueueFamilies))[i];
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &queuePriority;
		cvector_push_back(queueCreateInfos, &queueInfo);
	}

	cvector_t * /* const char* */ enabledExtensions = cvector_init(sizeof(const char *), array_len(WantedDeviceExtensions) + array_len(RequiredDeviceExtensions));

	u32 extensionCount;
	vkEnumerateDeviceExtensionProperties(device->physDevice, NULL, &extensionCount, NULL);
	cvector_t * /* VkExtensionProperties */ extensions = cvector_init(sizeof(VkExtensionProperties), extensionCount);
	vkEnumerateDeviceExtensionProperties(device->physDevice, NULL, &extensionCount, (VkExtensionProperties *)cvector_data(extensions));

	for(u32 i = 0; i < extensionCount; i++) {
		const char *extname = ((VkExtensionProperties *)cvector_data(extensions))[i].extensionName;
		cstring_t *extnamestr = cstring_init_str( extname );
		cvector_push_back(device->availableDeviceExtensions, &extnamestr);
	}

	for(u32 j = 0; j < array_len(WantedDeviceExtensions); j++) {
        const char *wanted = WantedDeviceExtensions[j];
		for(u32 i = 0; i < extensionCount; i++) {
			VkExtensionProperties ext = ((VkExtensionProperties *)cvector_data(extensions))[i];
			if(strcmp(wanted, ext.extensionName) == 0) {
				cvector_push_back(enabledExtensions, &ext.extensionName);
			}
		}
    }

	for(u32 j = 0; j < array_len(RequiredDeviceExtensions); j++) {
		bool validated = false;
        const char *required = RequiredDeviceExtensions[j];
		for(u32 i = 0; i < extensionCount; i++) {
			const char* extName = ((VkExtensionProperties *)cvector_data(extensions))[i].extensionName;
			if(strcmp(required, extName) == 0) {
				cvector_push_back(enabledExtensions, &extName);
				validated = true;
			}
		}

		if(!validated)
			LOG_ERROR("Failed to validate required extension with name %s", required);
	}

	VkPhysicalDeviceFeatures availableFeatures = {};
	vkGetPhysicalDeviceFeatures(device->physDevice, &availableFeatures);

	availableFeatures = availableFeatures;

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = cvector_size(queueCreateInfos);
	deviceCreateInfo.pQueueCreateInfos = (const VkDeviceQueueCreateInfo *)cvector_data(queueCreateInfos);
	deviceCreateInfo.enabledExtensionCount = cvector_size(enabledExtensions);
	deviceCreateInfo.ppEnabledExtensionNames = (const char * const*)cvector_data(enabledExtensions);
	deviceCreateInfo.pEnabledFeatures = (VkPhysicalDeviceFeatures *)&(VkPhysicalDeviceFeatures){};
    VkDevice temp = NULL;
	if (vkCreateDevice(device->physDevice, &deviceCreateInfo, NULL, &temp) != VK_SUCCESS) {
		LOG_AND_ABORT("Failed to create device");
	}

	cvector_destroy(extensions);
	cvector_destroy(enabledExtensions);
	cvector_destroy(queueFamilies);
	cvector_destroy(uniqueQueueFamilies);
	cvector_destroy(queueCreateInfos);

	return temp;
}

crdevice_t *crdevice_init(SDL_Window *window)
{
    crdevice_t *device = malloc(sizeof(crdevice_t));
    memset(device, 0, sizeof(struct crdevice_t));

    device->window = window;
    device->instance = CreateInstance( device, SDL_GetWindowTitle(window) );
    device->physDevice = ChoosePhysicalDevice(device->instance, device->surface);
    device->vkdevice = CreateDevice(device);

    volkLoadDevice(device->vkdevice);
}

struct VkDevice_T *crdevice_get_vkdevice(const crdevice_t *rdevice)
{
    return rdevice->vkdevice;
}

struct VkPhysicalDevice_T *crdevice_get_physdevice(const crdevice_t *rdevice)
{
    return rdevice->physDevice;
}
