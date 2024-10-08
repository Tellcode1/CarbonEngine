#include "../../include/vkcb/Context.hpp"
#include <set>

VkInstance 							 Context::instance;
VkDevice 							 Context::device;
VkPhysicalDevice 					 Context::physDevice;
VkSurfaceKHR 				         Context::surface;
SDL_Window* 				         Context::window;
VkDebugUtilsMessengerEXT 			 Context::debugMessenger;
cvector<cstring_view> 			 Context::availableDeviceExtensions;
cvector<cstring_view> 			 Context::availableInstanceExtensions;
VkPhysicalDeviceFeatures 			 Context::availableFeatures;

VkSampleCountFlagBits                device_info::MAX_SAMPLES;
bool 				                 device_info::SUPPORTS_MULTISAMPLING;
f32 				                 device_info::MAX_ANISOTROPY;

VkInstance CreateInstance(const char* title, cvector<cstring_view>& availableExtensions) {
    VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.applicationVersion = VK_API_VERSION_1_0;
	appInfo.apiVersion = VK_API_VERSION_1_0;
	appInfo.pApplicationName = title;
	appInfo.pEngineName = "Carbon Engine";
	appInfo.engineVersion = 0;

	uint32_t SDLExtensionCount = 0;
	SDL_Vulkan_GetInstanceExtensions(ctx::window, &SDLExtensionCount, nullptr);
	const char** SDLExtensions = new const char *[SDLExtensionCount];
	SDL_Vulkan_GetInstanceExtensions(ctx::window, &SDLExtensionCount, SDLExtensions);

	cvector<const char*> enabledExtensions;

	for (const auto& ext : RequiredInstanceExtensions)
		enabledExtensions.push_back(ext);

	// I honestly don't know why the debugger doesn't like it when I copy it over in a loop, sorry.
	const u32 prevSize = RequiredInstanceExtensions.size();
	enabledExtensions.resize(enabledExtensions.size() + SDLExtensionCount);
	memcpy(enabledExtensions.data() + prevSize, SDLExtensions, SDLExtensionCount * sizeof(const char*));

	u32 extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	VkExtensionProperties *extensions = new VkExtensionProperties[extensionCount];
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions);

	for (u32 i = 0; i < extensionCount; i++) {
		const char* name = extensions[i].extensionName;
		for(const auto& want : WantedInstanceExtensions)
			if(strcmp(name, want) == 0) {
				enabledExtensions.push_back(name);
				availableExtensions.push_back((unicode *)name);
				break;
			}
	}

	VkInstanceCreateInfo instanceCreateinfo{};
	instanceCreateinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateinfo.pApplicationInfo = &appInfo;
	instanceCreateinfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
	instanceCreateinfo.ppEnabledExtensionNames = enabledExtensions.data();

	#ifdef DEBUG

	bool validationLayersAvailable = false;
	uint32_t layerCount = 0;

	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	VkLayerProperties *layerProperties = new VkLayerProperties[layerCount];
	vkEnumerateInstanceLayerProperties(&layerCount, layerProperties);

	if(ValidationLayers.size() != 0)
	{
		for(const auto& layer : ValidationLayers)
		{
			for (uint32_t i = 0; i < layerCount; i++)
				if (strcmp(layer, layerProperties[i].layerName) == 0)
					validationLayersAvailable = true;
		}

		if (!validationLayersAvailable)
		{
			LOG_ERROR("VALIDTATION LAYERS COULD NOT BE LOADED\nRequested layers:\n");
			for(const char* layer : ValidationLayers)
				printf("\t%s\n", layer);

			printf("Instance provided these layers (i.e. These layers are available):\n");
			for(uint32_t i = 0; i < layerCount; i++)
				printf("\t%s\n", layerProperties[i].layerName);

			printf("But instance asked for (i.e. are not available):\n");

			cvector<const char*> missingLayers;
		
			for(const auto& layer : ValidationLayers)
			{
				bool layerAvailable = false;
				for (uint32_t i = 0; i < layerCount; i++)
					if (strcmp(layer, layerProperties[i].layerName) == 0)
					{
						layerAvailable = true;
						break;
					}
				if(!layerAvailable) missingLayers.push_back(layer);
			}
			for(const auto& layer : missingLayers)
				printf("\t%s\n", layer);
			
			abort();
		}

		instanceCreateinfo.enabledLayerCount = ValidationLayers.size();
		instanceCreateinfo.ppEnabledLayerNames = ValidationLayers.data();
	}

	delete[] layerProperties;
	delete[] extensions;
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
			if (_CreateDebugUtilsMessenger(instance, &createInfo, nullptr, &ctx::debugMessenger) != VK_SUCCESS)
				LOG_ERROR("Debug messenger failed to initialize");
			else
				LOG_DEBUG("Debug messenger successfully set up.");
		}
		else
			LOG_ERROR("vkCreateDebugUtilsMessengerEXT proc address not found");
	}
	#endif

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
		"(%s) %s Vulkan API Version %d.%d",
		deviceTypeStr, properties.deviceName,
		properties.apiVersion >> 22, (properties.apiVersion >> 12) & 0x3FF
	);
}

VkPhysicalDevice ChoosePhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {

	uint32_t physDeviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &physDeviceCount, nullptr);
	
	if(physDeviceCount == 0) {
		LOG_AND_ABORT("No physical devices found\n");
	}

	VkPhysicalDevice *physicalDevices = new VkPhysicalDevice[physDeviceCount];
	vkEnumeratePhysicalDevices(instance, &physDeviceCount, physicalDevices);

	for(u32 i = 0; i < physDeviceCount; i++) {
		const auto& device = physicalDevices[i];

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		
		bool extensionsAvailable = true;

		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		VkExtensionProperties *availableExtensions = new VkExtensionProperties[extensionCount];
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions);

		for (const auto& extension : RequiredDeviceExtensions) {
			bool validated = false;
			for(u32 j = 0; j < extensionCount; j++) {
				if (strcmp(extension, availableExtensions[j].extensionName) == 0)
					validated = true;
			}
			if (!validated) {
				LOG_ERROR("Failed to validate extension with name: %s\n", extension);
				extensionsAvailable = false;
			}
		}

		if (extensionsAvailable && formatCount > 0 && presentModeCount > 0) {
			VkPhysicalDevice selected = device; // Need to do this because the physical devices array will be deleted.

			delete[] availableExtensions;
			delete[] physicalDevices;

			LOG_INFO("Found device!");
			PrintDeviceInfo(selected);
			return selected;
		}

		delete[] availableExtensions;
	}

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(physicalDevices[0], &properties);

	LOG_ERROR("Could not find an appropriate device. Falling back to device 0: %s\n", properties.deviceName);

	VkPhysicalDevice fallback = physicalDevices[0];
	PrintDeviceInfo(fallback);
	delete[] physicalDevices;
	return fallback;
}

void Context::Initialize(const char* title, u32 windowWidth, u32 windowHeight) {
	ctx::window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	if(ctx::window == nullptr) {
		std::cout << ANSI_FORMAT_RED << "Window creation failed.\nSDL reports: " << ANSI_FORMAT_RESET << SDL_GetError() << "\n";
		exit(-1);
	}

	ctx::instance = CreateInstance(title, ctx::availableInstanceExtensions);
	assert(ctx::instance != nullptr);
	
	if(SDL_Vulkan_CreateSurface(ctx::window, ctx::instance, &ctx::surface) != SDL_TRUE) {
		std::cout << ANSI_FORMAT_RED << "Surface creation failed.\nSDL reports: " << ANSI_FORMAT_RESET << SDL_GetError() << "\n";
		exit(-1);
	}
	
	ctx::physDevice = ChoosePhysicalDevice(ctx::instance, ctx::surface);

	u32 queueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(ctx::physDevice, &queueCount, nullptr);
	VkQueueFamilyProperties queueFamilies[queueCount];
	vkGetPhysicalDeviceQueueFamilyProperties(ctx::physDevice, &queueCount, queueFamilies);

	// Clang loves complaining about these.
	u32 graphicsFamily = 0, presentFamily = 0, computeFamily = 0, transferFamily = 0;
	(void)graphicsFamily, (void)presentFamily, (void)computeFamily, (void)transferFamily;

	bool foundGraphicsFamily = false, foundPresentFamily = false, foundComputeFamily = false, foundTransferFamily = false;
		
	u32 i = 0;
	for (const auto& queueFamily : queueFamilies) {
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(ctx::physDevice, i, ctx::surface, &presentSupport);
	
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
	
	const std::set<u32> uniqueQueueFamilies = {graphicsFamily, presentFamily, computeFamily, transferFamily};
	VkDeviceQueueCreateInfo *queueCreateInfos = new VkDeviceQueueCreateInfo[uniqueQueueFamilies.size()];

	constexpr float queuePriority = 1.0f;

	u32 j = 0;
	for (const auto& queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueInfo = {};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = queueFamily;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos[j] = queueInfo;
		j++;
	}

	const char** enabledExtensions = new const char*[array_len(WantedDeviceExtensions) + RequiredDeviceExtensions.size()];

	u32 extensionCount;
	vkEnumerateDeviceExtensionProperties(ctx::physDevice, nullptr, &extensionCount, nullptr);
	VkExtensionProperties *extensions = new VkExtensionProperties[extensionCount];
	vkEnumerateDeviceExtensionProperties(ctx::physDevice, nullptr, &extensionCount, extensions);

	for(u32 i = 0; i < extensionCount; i++)
		ctx::availableDeviceExtensions.push_back((unicode *)extensions[i].extensionName);

	u32 enabledIterator = 0;

	for(const auto& wanted : WantedDeviceExtensions)
		for(u32 i = 0; i < extensionCount; i++)
			if(strcmp(wanted, extensions[i].extensionName) == 0) {
				const char* extName = extensions[i].extensionName;
				enabledExtensions[enabledIterator] = extName;
				enabledIterator++;
			}

	for(const auto& required : RequiredDeviceExtensions) {
		bool validated = false;
		for(u32 i = 0; i < extensionCount; i++)
			if(strcmp(required, extensions[i].extensionName) == 0) {
				enabledExtensions[enabledIterator] = extensions[i].extensionName;
				enabledIterator++;
				validated = true;
			}

		if(!validated)
			LOG_ERROR("Failed to validate required extension with name %s", required);
	}

	VkPhysicalDeviceFeatures availableFeatures{};
	vkGetPhysicalDeviceFeatures(ctx::physDevice, &availableFeatures);

	ctx::availableFeatures = availableFeatures;

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = uniqueQueueFamilies.size();
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
	deviceCreateInfo.enabledExtensionCount = enabledIterator;
	deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions;
	deviceCreateInfo.pEnabledFeatures = &WantedFeatures;

	if (vkCreateDevice(ctx::physDevice, &deviceCreateInfo, nullptr, &ctx::device) != VK_SUCCESS) {
		LOG_AND_ABORT("Failed to create device");
	}

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(ctx::physDevice, &props);

	device_info::MAX_ANISOTROPY = props.limits.maxSamplerAnisotropy;
	device_info::SUPPORTS_MULTISAMPLING = true;

	const VkSampleCountFlags samples = props.limits.framebufferColorSampleCounts;
    if (samples & VK_SAMPLE_COUNT_64_BIT)
		device_info::MAX_SAMPLES = VK_SAMPLE_COUNT_64_BIT;
    else if (samples & VK_SAMPLE_COUNT_32_BIT)
		device_info::MAX_SAMPLES = VK_SAMPLE_COUNT_32_BIT;
    else if (samples & VK_SAMPLE_COUNT_16_BIT)
		device_info::MAX_SAMPLES = VK_SAMPLE_COUNT_16_BIT;
    else if (samples & VK_SAMPLE_COUNT_8_BIT)
		device_info::MAX_SAMPLES = VK_SAMPLE_COUNT_8_BIT;
    else if (samples & VK_SAMPLE_COUNT_4_BIT)
		device_info::MAX_SAMPLES = VK_SAMPLE_COUNT_4_BIT;
    else if (samples & VK_SAMPLE_COUNT_2_BIT)
		device_info::MAX_SAMPLES = VK_SAMPLE_COUNT_2_BIT;
	else {
		device_info::MAX_SAMPLES = VK_SAMPLE_COUNT_1_BIT;
		device_info::SUPPORTS_MULTISAMPLING = false;
	}

	delete[] queueCreateInfos;
	delete[] enabledExtensions;
}