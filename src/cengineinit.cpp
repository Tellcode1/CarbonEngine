#include "../../include/cengineinit.hpp"
#include "../../include/containers/cvector.h"
#include <cassert>

VkInstance 							 instance = VK_NULL_HANDLE;
VkDevice 							 device = VK_NULL_HANDLE;
VkPhysicalDevice 					 physDevice = VK_NULL_HANDLE;
VkSurfaceKHR 				         surface = VK_NULL_HANDLE;
struct SDL_Window* 				     window = nullptr;
VkDebugUtilsMessengerEXT 			 debugMessenger = VK_NULL_HANDLE;
cvector_t * /* cstring_t * */ 	     ctx::availableDeviceExtensions;
cvector_t * /* cstring_t * */ 	     ctx::availableInstanceExtensions;
VkPhysicalDeviceFeatures 			 ctx::availableFeatures;

VkSampleCountFlagBits                MAX_SAMPLES;
unsigned char 				         SUPPORTS_MULTISAMPLING;
f32 				                 MAX_ANISOTROPY;

cvector_t *setify(u32 i1, u32 i2, u32 i3, u32 i4) {
	cvector_t *ret = cvector_init(sizeof(u32), 4);
    u32 nums[4] = {i1, i2, i3, i4};
	for (const auto &e : nums) {
		bool already_in = false;
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

VkInstance CreateInstance(const char* title, cvector_t * /* cstring_t * */ availableExtensions) {
	if (volkInitialize() != VK_SUCCESS) {
		LOG_AND_ABORT("Volk could not initialize. You probably don't have the vulkan loader installed. I can't do anything about that.");
	}

    VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.applicationVersion = VK_API_VERSION_1_0;
	appInfo.apiVersion = VK_API_VERSION_1_0;
	appInfo.pApplicationName = title;
	appInfo.pEngineName = "Carbon";
	appInfo.engineVersion = 0;

	uint32_t SDLExtensionCount = 0;
	SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionCount, nullptr);
	cvector_t * /* const char* */ SDLExtensions = cvector_init(sizeof(const char *), SDLExtensionCount);
	SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionCount, (const char **)cvector_data(SDLExtensions));

	cvector_t *enabledExtensions = cvector_init(sizeof(const char *), array_len(RequiredInstanceExtensions));

	for (const auto& ext : RequiredInstanceExtensions) {
		cvector_push_back(enabledExtensions, &ext);
	}

	for (int i = 0; i < (int)SDLExtensionCount; i++) {
		cvector_push_back(enabledExtensions, cvector_get(SDLExtensions, i));
	}

	u32 extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	cvector_t * /* VkExtensionProperties */ extensions = cvector_init(sizeof(VkExtensionProperties), extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, (VkExtensionProperties *)cvector_data(extensions));

	for (u32 i = 0; i < extensionCount; i++) {
		const char* name = ((VkExtensionProperties *)cvector_data(extensions))[i].extensionName;
		for(const auto& want : WantedInstanceExtensions)
			if(strcmp(name, want) == 0) {
				cvector_push_back(enabledExtensions, &name);
				cstring_t *name_str = cstring_init_str(name);
				cvector_push_back(availableExtensions, &name_str);
				break;
			}
	}

	VkInstanceCreateInfo instanceCreateinfo{};
	instanceCreateinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateinfo.pApplicationInfo = &appInfo;
	instanceCreateinfo.enabledExtensionCount = cvector_size(enabledExtensions);
	instanceCreateinfo.ppEnabledExtensionNames = (const char **)cvector_data(enabledExtensions);

	#ifdef DEBUG

	bool validationLayersAvailable = false;
	uint32_t layerCount = 0;

	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	cvector_t * /* VkLayerProperties */ layerProperties = cvector_init(sizeof(VkLayerProperties), layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, (VkLayerProperties *)cvector_data(layerProperties));

	if(array_len(ValidationLayers) != 0)
	{
		for(const auto& layer : ValidationLayers)
		{
			for (uint32_t i = 0; i < layerCount; i++)
				if (strcmp(layer, ((VkLayerProperties *)cvector_data(layerProperties))[i].layerName) == 0)
					validationLayersAvailable = true;
		}

		if (!validationLayersAvailable)
		{
			LOG_ERROR("VALIDATION LAYERS COULD NOT BE LOADED\nRequested layers:\n");
			for(const char* layer : ValidationLayers)
				printf("\t%s\n", layer);

			printf("Instance provided these layers (i.e. These layers are available):\n");
			for(uint32_t i = 0; i < layerCount; i++)
				printf("\t%s\n", ((VkLayerProperties *)cvector_data(layerProperties))[i].layerName);

			printf("But instance asked for (i.e. are not available):\n");

			cvector_t * /* const char* */ missingLayers = cvector_init(sizeof(const char *), 16);
		
			for(const auto& layer : ValidationLayers)
			{
				bool layerAvailable = false;
				for (uint32_t i = 0; i < layerCount; i++)
					if (strcmp(layer, ((VkLayerProperties *)cvector_data(layerProperties))[i].layerName) == 0)
					{
						layerAvailable = true;
						break;
					}
				if(!layerAvailable) cvector_push_back(missingLayers, &layer);
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
	instanceCreateinfo.ppEnabledLayerNames = nullptr;

	#endif

    VkInstance instance;
	vkCreateInstance(&instanceCreateinfo, nullptr, &instance);

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
		createInfo.pfnUserCallback = DebugMessengerCallback;

		auto _CreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

		if (_CreateDebugUtilsMessenger) {
			if (_CreateDebugUtilsMessenger(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
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
	vkEnumeratePhysicalDevices(instance, &physDeviceCount, nullptr);
	
	if(physDeviceCount == 0) {
		LOG_AND_ABORT("Huuuhhh??? No physical devices found? Are you running this on a banana???\n");
	}

	cvector_t * /* VkPhysicalDevice */ physicalDevices = cvector_init(sizeof(VkPhysicalDevice), physDeviceCount);
	vkEnumeratePhysicalDevices(instance, &physDeviceCount, (VkPhysicalDevice *)cvector_data(physicalDevices));

	for(u32 i = 0; i < physDeviceCount; i++) {
		const auto& device = ((VkPhysicalDevice *)cvector_data(physicalDevices))[i];

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		
		bool extensionsAvailable = true;

		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		cvector_t * /* VkExtensionProperties */ availableExtensions = cvector_init(sizeof(VkExtensionProperties), extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, (VkExtensionProperties *)cvector_data(availableExtensions));

		for (const auto& extension : RequiredDeviceExtensions) {
			bool validated = false;
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

VkDevice CreateDevice() {
	u32 queueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, nullptr);
	cvector_t * /* VkQueueFamilyProperties */ queueFamilies = cvector_init(sizeof(VkQueueFamilyProperties), queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, (VkQueueFamilyProperties *)cvector_data(queueFamilies));

	// Clang loves complaining about these.
	u32 graphicsFamily = 0, presentFamily = 0, computeFamily = 0, transferFamily = 0;
	(void)graphicsFamily, (void)presentFamily, (void)computeFamily, (void)transferFamily;

	bool foundGraphicsFamily = false, foundPresentFamily = false, foundComputeFamily = false, foundTransferFamily = false;
		
	u32 i = 0;
	for (int j = 0; j < cvector_size(queueFamilies); j++) {
		const VkQueueFamilyProperties &queueFamily = ((VkQueueFamilyProperties *)cvector_data(queueFamilies))[j];
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, i, surface, &presentSupport);
	
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

	constexpr float queuePriority = 1.0f;

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
	vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, nullptr);
	cvector_t * /* VkExtensionProperties */ extensions = cvector_init(sizeof(VkExtensionProperties), extensionCount);
	vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, (VkExtensionProperties *)cvector_data(extensions));

	for(u32 i = 0; i < extensionCount; i++) {
		const char *extname = ((VkExtensionProperties *)cvector_data(extensions))[i].extensionName;
		cstring_t *extnamestr = cstring_init_str( extname );
		cvector_push_back(ctx::availableDeviceExtensions, &extnamestr);
	}

	for(const auto& wanted : WantedDeviceExtensions)
		for(u32 i = 0; i < extensionCount; i++) {
			VkExtensionProperties ext = ((VkExtensionProperties *)cvector_data(extensions))[i];
			if(strcmp(wanted, ext.extensionName) == 0) {
				cvector_push_back(enabledExtensions, &ext.extensionName);
			}
		}

	for(const auto& required : RequiredDeviceExtensions) {
		bool validated = false;
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

	VkPhysicalDeviceFeatures availableFeatures{};
	vkGetPhysicalDeviceFeatures(physDevice, &availableFeatures);

	availableFeatures = availableFeatures;

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = cvector_size(queueCreateInfos);
	deviceCreateInfo.pQueueCreateInfos = (const VkDeviceQueueCreateInfo *)cvector_data(queueCreateInfos);
	deviceCreateInfo.enabledExtensionCount = cvector_size(enabledExtensions);
	deviceCreateInfo.ppEnabledExtensionNames = (const char * const*)cvector_data(enabledExtensions);
	deviceCreateInfo.pEnabledFeatures = &WantedFeatures;

	if (vkCreateDevice(physDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
		LOG_AND_ABORT("Failed to create device");
	}

	cvector_destroy(extensions);
	cvector_destroy(enabledExtensions);
	cvector_destroy(queueFamilies);
	cvector_destroy(uniqueQueueFamilies);
	cvector_destroy(queueCreateInfos);

	return device;
}

void ctx::Initialize(const char* title, u32 windowWidth, u32 windowHeight) {
	window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	cassert(window != NULL);

	ctx::availableDeviceExtensions = cvector_init(sizeof(cstring_t *), 4);
	ctx::availableInstanceExtensions = cvector_init(sizeof(cstring_t *), 4);
	instance = CreateInstance(title, ctx::availableInstanceExtensions);
	cassert(instance != NULL);
	
	if(SDL_Vulkan_CreateSurface(window, instance, &surface) != SDL_TRUE) {
		LOG_AND_ABORT("Surface creation failed.\nSDL reports: %s", SDL_GetError());
	}
	
	physDevice = ChoosePhysicalDevice(instance, surface);

	device = CreateDevice();
	cassert(device != NULL);

	volkLoadDevice(device);

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(physDevice, &props);

	MAX_ANISOTROPY = props.limits.maxSamplerAnisotropy;
	SUPPORTS_MULTISAMPLING = true;

	const VkSampleCountFlags samples = props.limits.framebufferColorSampleCounts;
    if (samples & VK_SAMPLE_COUNT_64_BIT)
		MAX_SAMPLES = VK_SAMPLE_COUNT_64_BIT;
    else if (samples & VK_SAMPLE_COUNT_32_BIT)
		MAX_SAMPLES = VK_SAMPLE_COUNT_32_BIT;
    else if (samples & VK_SAMPLE_COUNT_16_BIT)
		MAX_SAMPLES = VK_SAMPLE_COUNT_16_BIT;
    else if (samples & VK_SAMPLE_COUNT_8_BIT)
		MAX_SAMPLES = VK_SAMPLE_COUNT_8_BIT;
    else if (samples & VK_SAMPLE_COUNT_4_BIT)
		MAX_SAMPLES = VK_SAMPLE_COUNT_4_BIT;
    else if (samples & VK_SAMPLE_COUNT_2_BIT)
		MAX_SAMPLES = VK_SAMPLE_COUNT_2_BIT;
	else {
		MAX_SAMPLES = VK_SAMPLE_COUNT_1_BIT;
		SUPPORTS_MULTISAMPLING = false;
	}
}