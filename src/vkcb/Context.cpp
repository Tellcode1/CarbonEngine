#include "../../include/vkcb/Context.hpp"
#include "../../include/containers/cvector.hpp"

#include <cassert>

VkInstance 							 instance = VK_NULL_HANDLE;
VkDevice 							 device = VK_NULL_HANDLE;
VkPhysicalDevice 					 physDevice = VK_NULL_HANDLE;
VkSurfaceKHR 				         surface = VK_NULL_HANDLE;
struct SDL_Window* 				     window = nullptr;
VkDebugUtilsMessengerEXT 			 debugMessenger = VK_NULL_HANDLE;
cvector<cstring_view> 			     ctx::availableDeviceExtensions;
cvector<cstring_view> 			     ctx::availableInstanceExtensions;
VkPhysicalDeviceFeatures 			 ctx::availableFeatures;

VkSampleCountFlagBits                MAX_SAMPLES;
unsigned char 				         SUPPORTS_MULTISAMPLING;
f32 				                 MAX_ANISOTROPY;

cvector<u32> setify(const cvector<u32> &in) {
	cvector<u32> ret(in.size());
	for (const auto &e : in) {
		bool already_in = false;
		for (const auto &i : ret)
			if (e == i)
				already_in = true;
		if (!already_in)
			ret.push_back(e);
	}
	return ret;
}

VkInstance CreateInstance(const char* title, cvector<cstring_view>& availableExtensions) {
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
	cvector<const char*> SDLExtensions(SDLExtensionCount);
	SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionCount, SDLExtensions.data());

	cvector<const char*> enabledExtensions(RequiredInstanceExtensions.size());

	for (const auto& ext : RequiredInstanceExtensions)
		enabledExtensions.push_back(ext);

	// I honestly don't know why the debugger doesn't like it when I copy it over in a loop, sorry.
	const u32 prevSize = RequiredInstanceExtensions.size();
	enabledExtensions.resize(enabledExtensions.size() + SDLExtensionCount);
	memcpy(enabledExtensions.data() + prevSize, SDLExtensions.data(), SDLExtensionCount * sizeof(const char*));

	u32 extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	cvector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

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
	cvector<VkLayerProperties> layerProperties(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());

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

	cvector<VkPhysicalDevice> physicalDevices(physDeviceCount);
	vkEnumeratePhysicalDevices(instance, &physDeviceCount, physicalDevices.data());

	for(u32 i = 0; i < physDeviceCount; i++) {
		const auto& device = physicalDevices[i];

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		
		bool extensionsAvailable = true;

		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		cvector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

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
			LOG_INFO("Found device!");
			PrintDeviceInfo(device);
			return device;
		}
	}

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(physicalDevices[0], &properties);

	LOG_ERROR("Could not find an appropriate device. Falling back to device 0: %s\n", properties.deviceName);

	PrintDeviceInfo(physicalDevices[0]);
	return physicalDevices[0];
}

VkDevice CreateDevice() {
	u32 queueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, nullptr);
	cvector<VkQueueFamilyProperties> queueFamilies(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, queueFamilies.data());

	// Clang loves complaining about these.
	u32 graphicsFamily = 0, presentFamily = 0, computeFamily = 0, transferFamily = 0;
	(void)graphicsFamily, (void)presentFamily, (void)computeFamily, (void)transferFamily;

	bool foundGraphicsFamily = false, foundPresentFamily = false, foundComputeFamily = false, foundTransferFamily = false;
		
	u32 i = 0;
	for (const auto& queueFamily : queueFamilies) {
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
	
	const cvector<u32> uniqueQueueFamilies = setify({graphicsFamily, presentFamily, computeFamily, transferFamily});
	const cvector<VkDeviceQueueCreateInfo> queueCreateInfos(uniqueQueueFamilies.size());

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

	cvector<const char*> enabledExtensions(array_len(WantedDeviceExtensions) + RequiredDeviceExtensions.size());

	u32 extensionCount;
	vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, nullptr);
	cvector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, extensions.data());

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
	vkGetPhysicalDeviceFeatures(physDevice, &availableFeatures);

	availableFeatures = availableFeatures;

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = uniqueQueueFamilies.size();
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.enabledExtensionCount = enabledIterator;
	deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
	deviceCreateInfo.pEnabledFeatures = &WantedFeatures;

	if (vkCreateDevice(physDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
		LOG_AND_ABORT("Failed to create device");
	}

	return device;
}

void ctx::Initialize(const char* title, u32 windowWidth, u32 windowHeight) {
	window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	cassert(window != NULL);

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