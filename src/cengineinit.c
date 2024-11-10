#include "../../include/cengineinit.h"
#include "../../include/cengine.h"
#include "../../include/cgvector.h"

VkSampleCountFlagBits                MAX_SAMPLES;
unsigned char 				         SUPPORTS_MULTISAMPLING;
f32 				                 MAX_ANISOTROPY;

cg_vector_t setify(u32 i1, u32 i2, u32 i3, u32 i4) {
	cg_vector_t ret = cg_vector_init(sizeof(u32), 4);
    u32 nums[4] = {i1, i2, i3, i4};
	for (int j = 0; j < array_len(nums); j++) {
		const u32 e = nums[j];
		bool already_in = false;
		for (int i = 0; i < cg_vector_size(&ret); i++) {
			if (e == *(u32 *)cg_vector_get(&ret, i))
				already_in = true;

		}
		if (!already_in) {
			cg_vector_push_back(&ret, &e);
		}
	}
	return ret;
}

VkInstance CreateInstance(const char* title) {
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
	SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionCount, NULL);
	cg_vector_t /* const char* */ SDLExtensions = cg_vector_init(sizeof(const char *), SDLExtensionCount);
	SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionCount, (const char **)cg_vector_data(&SDLExtensions));

	cg_vector_t enabledExtensions = cg_vector_init(sizeof(const char *), array_len(RequiredInstanceExtensions));

	for (int i = 0; i < array_len(RequiredInstanceExtensions); i++) {
		const char *ext = RequiredInstanceExtensions[i];
		cg_vector_push_back(&enabledExtensions, &ext);
	}

	for (int i = 0; i < (int)SDLExtensionCount; i++) {
		cg_vector_push_back(&enabledExtensions, cg_vector_get(&SDLExtensions, i));
	}

	u32 extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
	cg_vector_t /* VkExtensionProperties */ extensions = cg_vector_init(sizeof(VkExtensionProperties), extensionCount);
	vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, (VkExtensionProperties *)cg_vector_data(&extensions));

	for (u32 i = 0; i < extensionCount; i++) {
		const char* name = ((VkExtensionProperties *)cg_vector_data(&extensions))[i].extensionName;
		for(int j = 0; j < array_len(WantedInstanceExtensions); j++) {
			const char * want = WantedInstanceExtensions[j];
			if(strcmp(name, want) == 0) {
				cg_vector_push_back(&enabledExtensions, &name);
				break;
			}
		}
	}

	VkInstanceCreateInfo instanceCreateinfo = {};
	instanceCreateinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateinfo.pApplicationInfo = &appInfo;
	instanceCreateinfo.enabledExtensionCount = cg_vector_size(&enabledExtensions);
	instanceCreateinfo.ppEnabledExtensionNames = (const char **)cg_vector_data(&enabledExtensions);

	#ifdef DEBUG

	bool validationLayersAvailable = false;
	uint32_t layerCount = 0;

	vkEnumerateInstanceLayerProperties(&layerCount, NULL);
	cg_vector_t /* VkLayerProperties */ layerProperties = cg_vector_init(sizeof(VkLayerProperties), layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, (VkLayerProperties *)cg_vector_data(&layerProperties));

	if(array_len(ValidationLayers) != 0) {
		for(int j = 0; j < array_len(ValidationLayers); j++) {
			for (uint32_t i = 0; i < layerCount; i++) {
				if (strcmp(ValidationLayers[j], ((VkLayerProperties *)cg_vector_data(&layerProperties))[i].layerName) == 0) {
					validationLayersAvailable = true;
				}
			}
		}

		if (!validationLayersAvailable)
		{
			LOG_ERROR("VALIDATION LAYERS COULD NOT BE LOADED\nRequested layers:\n");
			for(int i = 0; i < array_len(ValidationLayers); i++) {
				printf("\t%s\n", ValidationLayers[i]);
			}

			printf("Instance provided these layers (i.e. These layers are available):\n");
			for(uint32_t i = 0; i < layerCount; i++)
				printf("\t%s\n", ((VkLayerProperties *)cg_vector_data(&layerProperties))[i].layerName);

			printf("But instance asked for (i.e. are not available):\n");

			cg_vector_t /* const char* */ missingLayers = cg_vector_init(sizeof(const char *), 16);
		
			for(int i = 0; i < array_len(ValidationLayers); i++)
			{
				const char *layer = ValidationLayers[i];
				bool layerAvailable = false;
				for (uint32_t i = 0; i < layerCount; i++) {
					if (strcmp(layer, ((VkLayerProperties *)cg_vector_data(&layerProperties))[i].layerName) == 0)
					{
						layerAvailable = true;
						break;
					}
				}
				if(!layerAvailable) cg_vector_push_back(&missingLayers, &layer);
			}
			for(int i = 0; i < cg_vector_size(&missingLayers); i++)
				printf("\t%s\n", (const char *)cg_vector_get(&missingLayers, i));
			
			cg_vector_destroy(&missingLayers);
			
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
		createInfo.pfnUserCallback = DebugMessengerCallback;

		PFN_vkCreateDebugUtilsMessengerEXT _CreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

		if (_CreateDebugUtilsMessenger) {
			if (_CreateDebugUtilsMessenger(instance, &createInfo, NULL, &debugMessenger) != VK_SUCCESS)
				LOG_ERROR("Debug messenger failed to initialize");
			else
				LOG_DEBUG("Debug messenger successfully set up.");
		}
		else
			LOG_ERROR("vkCreateDebugUtilsMessengerEXT proc address not found");
	}
	#endif

	cg_vector_destroy(&SDLExtensions);
	cg_vector_destroy(&extensions);
	cg_vector_destroy(&enabledExtensions);
	cg_vector_destroy(&layerProperties);

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

	cg_vector_t /* VkPhysicalDevice */ physicalDevices = cg_vector_init(sizeof(VkPhysicalDevice), physDeviceCount);
	vkEnumeratePhysicalDevices(instance, &physDeviceCount, (VkPhysicalDevice *)cg_vector_data(&physicalDevices));

	for(u32 i = 0; i < physDeviceCount; i++) {
		const VkPhysicalDevice device = ((VkPhysicalDevice *)cg_vector_data(&physicalDevices))[i];

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, NULL);

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, NULL);
		
		bool extensionsAvailable = true;

		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);
		cg_vector_t /* VkExtensionProperties */ availableExtensions = cg_vector_init(sizeof(VkExtensionProperties), extensionCount);
		vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, (VkExtensionProperties *)cg_vector_data(&availableExtensions));

		for (int i = 0; i < array_len(RequiredDeviceExtensions); i++) {
			const char *extension = RequiredDeviceExtensions[i];
			bool validated = false;
			for(u32 j = 0; j < extensionCount; j++) {
				if (strcmp(extension, ((VkExtensionProperties *)cg_vector_data(&availableExtensions))[j].extensionName) == 0)
					validated = true;
			}
			if (!validated) {
				LOG_ERROR("Failed to validate extension with name: %s\n", extension);
				extensionsAvailable = false;
			}
		}

		cg_vector_destroy(&availableExtensions);

		if (extensionsAvailable && formatCount > 0 && presentModeCount > 0) {
			PrintDeviceInfo(device);
			return device;
		}
	}

	VkPhysicalDevice fallback = ((VkPhysicalDevice *)cg_vector_data(&physicalDevices))[0];

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(fallback, &properties);

	LOG_ERROR("No device found. Falling back to device \"%s\".\n", properties.deviceName);

	PrintDeviceInfo(fallback);

	cg_vector_destroy(&physicalDevices);

	return fallback;
}

VkDevice CreateDevice() {
	u32 queueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, NULL);
	cg_vector_t /* VkQueueFamilyProperties */ queueFamilies = cg_vector_init(sizeof(VkQueueFamilyProperties), queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, (VkQueueFamilyProperties *)cg_vector_data(&queueFamilies));

	// Clang loves complaining about these.
	u32 graphicsFamily = 0, presentFamily = 0, computeFamily = 0, transferFamily = 0;
	(void)graphicsFamily, (void)presentFamily, (void)computeFamily, (void)transferFamily;

	bool foundGraphicsFamily = false, foundPresentFamily = false, foundComputeFamily = false, foundTransferFamily = false;
		
	u32 i = 0;
	for (int j = 0; j < cg_vector_size(&queueFamilies); j++) {
		const VkQueueFamilyProperties queueFamily = ((VkQueueFamilyProperties *)cg_vector_data(&queueFamilies))[j];
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
	
	cg_vector_t /* u32 */ uniqueQueueFamilies = setify(graphicsFamily, presentFamily, computeFamily, transferFamily);
	cg_vector_t /* VkDeviceQueueCreateInfo */ queueCreateInfos = cg_vector_init(sizeof(VkDeviceQueueCreateInfo), cg_vector_size(&uniqueQueueFamilies));

	const float queuePriority = 1.0f;

	for (int i = 0; i < cg_vector_size(&uniqueQueueFamilies); i++)
	{
		VkDeviceQueueCreateInfo queueInfo = {};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = ((u32 *)cg_vector_data(&uniqueQueueFamilies))[i];
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &queuePriority;
		cg_vector_push_back(&queueCreateInfos, &queueInfo);
	}

	cg_vector_t /* const char* */ enabledExtensions = cg_vector_init(sizeof(const char *), array_len(WantedDeviceExtensions) + array_len(RequiredDeviceExtensions));

	u32 extensionCount;
	vkEnumerateDeviceExtensionProperties(physDevice, NULL, &extensionCount, NULL);
	cg_vector_t /* VkExtensionProperties */ extensions = cg_vector_init(sizeof(VkExtensionProperties), extensionCount);
	vkEnumerateDeviceExtensionProperties(physDevice, NULL, &extensionCount, (VkExtensionProperties *)cg_vector_data(&extensions));

	for (int i = 0; i < array_len(WantedDeviceExtensions); i++) {
		const char *wanted = WantedDeviceExtensions[i];
		for(u32 i = 0; i < extensionCount; i++) {
			VkExtensionProperties ext = ((VkExtensionProperties *)cg_vector_data(&extensions))[i];
			if(strcmp(wanted, ext.extensionName) == 0) {
				cg_vector_push_back(&enabledExtensions, &ext.extensionName);
			}
		}
	}

	for (int i = 0; i < array_len(RequiredDeviceExtensions); i++) {
		const char *required = RequiredDeviceExtensions[i];
		bool validated = false;
		for(u32 i = 0; i < extensionCount; i++) {
			const char* extName = ((VkExtensionProperties *)cg_vector_data(&extensions))[i].extensionName;
			if(strcmp(required, extName) == 0) {
				cg_vector_push_back(&enabledExtensions, &extName);
				validated = true;
			}
		}

		if(!validated)
			LOG_ERROR("Failed to validate required extension with name %s", required);
	}

	VkPhysicalDeviceFeatures availableFeatures = {};
	vkGetPhysicalDeviceFeatures(physDevice, &availableFeatures);

	availableFeatures = availableFeatures;

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = cg_vector_size(&queueCreateInfos);
	deviceCreateInfo.pQueueCreateInfos = (const VkDeviceQueueCreateInfo *)cg_vector_data(&queueCreateInfos);
	deviceCreateInfo.enabledExtensionCount = cg_vector_size(&enabledExtensions);
	deviceCreateInfo.ppEnabledExtensionNames = (const char * const*)cg_vector_data(&enabledExtensions);
	deviceCreateInfo.pEnabledFeatures = &WantedFeatures;

	if (vkCreateDevice(physDevice, &deviceCreateInfo, NULL, &device) != VK_SUCCESS) {
		LOG_AND_ABORT("Failed to create device");
	}

	cg_vector_destroy(&extensions);
	cg_vector_destroy(&enabledExtensions);
	cg_vector_destroy(&queueFamilies);
	cg_vector_destroy(&uniqueQueueFamilies);
	cg_vector_destroy(&queueCreateInfos);

	return device;
}

void ctx_initialize(const char* title, u32 windowWidth, u32 windowHeight) {
	window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	cassert(window != NULL);

	instance = CreateInstance(title);
	cassert(instance != NULL);

	if(SDL_Vulkan_CreateSurface(window, instance, &surface) != SDL_TRUE) {
		LOG_AND_ABORT("Surface creation failed.\nSDL reports: %s", SDL_GetError());
	}

	physDevice = ChoosePhysicalDevice(instance, surface);

	device = CreateDevice(device);
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