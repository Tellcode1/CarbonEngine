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

struct ContextSingleton;
static ContextSingleton CreateContext(const char* title, u32 windowWidth = 800, u32 windowHeight = 600);
static VkInstance CreateInstance(const char* title, std::unordered_set<std::string_view>& availableExtensions);
static VkPhysicalDevice ChoosePhysicalDevice(VkInstance instance, VkSurfaceKHR surface);

static bool __compare_VkPhysicalDeviceFeatures(VkPhysicalDeviceFeatures want, VkPhysicalDeviceFeatures have);
static inline bool IsExtensionAvailable(std::string name);

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

	std::string preceder = "Debug Messenger : \n\t";
	std::string succeeder = "\n";

	switch(messageSeverity)
	{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			preceder += "SEVERITY : WARNING\n\t";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			preceder += "SEVERITY : INFO\n\t";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			preceder += "SEVERITY : ERROR\n\t";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			preceder += "SEVERITY : VERBOSE\n\t";
			break;
		default:
			preceder += "UNKNOWN SEVERITY\n\t";
			break;
	}

	printf("%s%s%s", preceder.c_str(), pCallbackData->pMessage, succeeder.c_str());

    return VK_FALSE;
}

// struct AvailableExtensionArray
// {
// 	const char** availables;
// 	u32 num;

// 	inline bool operator[](const char* extName) {
// 		return std::hash<const char*>()(extName);
// 	}
// };

struct ContextSingleton {
    VkInstance instance;
    VkDevice device;
    VkPhysicalDevice physDevice;
    VkSurfaceKHR surface;
    SDL_Window* window;

	VkDebugUtilsMessengerEXT debugMessenger;

	std::unordered_set<std::string_view> availableDeviceExtensions;
	std::unordered_set<std::string_view> availableInstanceExtensions;
	VkPhysicalDeviceFeatures availableFeatures;

    NANO_SINGLETON_FUNCTION ContextSingleton* GetSingleton() {
        static ContextSingleton global{};
        return &global;
    }

	inline void Initialize(const char* title, u32 windowWidth, u32 windowHeight) {
		*GetSingleton() = CreateContext(title, windowWidth, windowHeight);
	}
};
static ContextSingleton* Context = ContextSingleton::GetSingleton();
static ContextSingleton* ctx = Context; // ctx is much easier to use

static VkInstance& instance = ctx->instance;
static VkDevice& device = ctx->device;
static VkPhysicalDevice& physDevice = ctx->physDevice;
static VkSurfaceKHR& surface = ctx->surface;
static SDL_Window*& window = ctx->window;

/* FUNCTION DEFINITIONS*/

static ContextSingleton CreateContext(const char* title, u32 windowWidth, u32 windowHeight)
{
    ContextSingleton retval{};

    retval.window = SDL_CreateWindow(title, windowWidth, windowHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if(retval.window == nullptr) {
		std::cout << ANSI_FORMAT_RED << "Window creation failed.\nSDL reports: " << ANSI_FORMAT_RESET << SDL_GetError() << "\n";
		exit(-1);
	}

    retval.instance = CreateInstance(title, retval.availableInstanceExtensions);
    assert(retval.instance != nullptr);
    
	if(SDL_Vulkan_CreateSurface(retval.window, retval.instance, nullptr, &retval.surface) != 0) {
		std::cout << ANSI_FORMAT_RED << "Surface creation failed.\nSDL reports: " << ANSI_FORMAT_RESET << SDL_GetError() << "\n";
		exit(-1);
	}
    
    retval.physDevice = ChoosePhysicalDevice(retval.instance, retval.surface);

	u32 queueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(retval.physDevice, &queueCount, nullptr);
	VkQueueFamilyProperties queueFamilies[queueCount];
	vkGetPhysicalDeviceQueueFamilyProperties(retval.physDevice, &queueCount, queueFamilies);

    // Clang loves complaining about these.
	u32 graphicsFamily, presentFamily, computeFamily, transferFamily;
	graphicsFamily, presentFamily, computeFamily, transferFamily;

	bool foundGraphicsFamily = false, foundPresentFamily = false, foundComputeFamily = false, foundTransferFamily = false;
		
	u32 i = 0;
	for (const auto& queueFamily : queueFamilies) {
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(retval.physDevice, i, retval.surface, &presentSupport);
	
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
	vkEnumerateDeviceExtensionProperties(retval.physDevice, nullptr, &extensionCount, nullptr);
	VkExtensionProperties *extensions = new VkExtensionProperties[extensionCount];
	vkEnumerateDeviceExtensionProperties(retval.physDevice, nullptr, &extensionCount, extensions);

	for(u32 i = 0; i < extensionCount; i++)
		retval.availableDeviceExtensions.insert(extensions[i].extensionName);

	u32 enabledIterator = 0;

	for(const auto& wanted : WantedDeviceExtensions)
		for(u32 i = 0; i < extensionCount; i++)
			if(strcmp(wanted, extensions[i].extensionName) == 0) {
				const char* extName = extensions[i].extensionName;
				enabledExtensions[enabledIterator] = extensions[i].extensionName;
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
			printf("Failed to validate required extension with name %s. \n", required);
	}

	VkPhysicalDeviceFeatures availableFeatures{};
	vkGetPhysicalDeviceFeatures(retval.physDevice, &availableFeatures);

	retval.availableFeatures = availableFeatures;

	// ! REPLACE WITH A CALL THAT ACTUALLY GIVES THE AVAILABLE FEATURES
	// * Should be like if(wanted.wantFeature) if(hasFeature.feature) enabledFeatures.feature = VK_TRUE
	assert(__compare_VkPhysicalDeviceFeatures(WantedFeatures, availableFeatures));

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = uniqueQueueFamilies.size();
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
	deviceCreateInfo.enabledExtensionCount = enabledIterator;
	deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions;
	deviceCreateInfo.pEnabledFeatures = &WantedFeatures;

	if (vkCreateDevice(retval.physDevice, &deviceCreateInfo, nullptr, &retval.device) != VK_SUCCESS) {
		printf("Failed to create device\n");
		exit(-1);
	}

	delete[] queueCreateInfos;
	delete[] enabledExtensions;

    return retval;
}

static VkInstance CreateInstance(const char* title, std::unordered_set<std::string_view>& availableExtensions) {
    VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.applicationVersion = VK_API_VERSION_1_0;
	appInfo.apiVersion = VK_API_VERSION_1_0;
	appInfo.pApplicationName = title;
	appInfo.pEngineName = "Carbon Engine";
	appInfo.engineVersion = 0;

	uint32_t SDLExtensionCount = 0;
	const char* const* SDLExtensions = SDL_Vulkan_GetInstanceExtensions(&SDLExtensionCount);
	std::vector<const char*> enabledExtensions;

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
				availableExtensions.insert(name);
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
			std::cout << ANSI_FORMAT_RED << "VALIDATION LAYERS COULD NOT BE LOADED\n" << ANSI_FORMAT_RESET << "Requested layers:\n";
			for(const char* layer : ValidationLayers)
				printf("\t%s\n", layer);

			printf("Instance provided these layers (i.e. These layers are available):\n");
			for(uint32_t i = 0; i < layerCount; i++)
				printf("\t%s\n", layerProperties[i].layerName);

			printf("But instance asked for (i.e. are not available):\n");

			std::vector<const char*> missingLayers;
		
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

	delete[] layerProperties;
	delete[] extensions;

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
			if (_CreateDebugUtilsMessenger(instance, &createInfo, nullptr, &Context->debugMessenger) != VK_SUCCESS)
				LOG_ERROR("Debug messenger failed to initialize");
			else
				printf("Debug messenger successfully set up.\n");
		}
		else if (!_CreateDebugUtilsMessenger)
			LOG_ERROR("vkCreateDebugUtilsMessengerEXT proc address not found");
	}
	#endif

    return instance;
}

static void PrintDeviceInfo(VkPhysicalDevice device)
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

	printf(
		"Selected device : (%s) %s Vulkan API Version %d.%d\n",
		deviceTypeStr, properties.deviceName,
		properties.apiVersion >> 22, (properties.apiVersion >> 12) & 0x3FF
	);
}

static VkPhysicalDevice ChoosePhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {

	uint32_t physDeviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &physDeviceCount, nullptr);
	
	if(physDeviceCount == 0) {
		printf("No physical devices found\n");
		abort();
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

			std::cout << ANSI_FORMAT_GREEN << "Found device!\n" << ANSI_FORMAT_RESET;
			PrintDeviceInfo(selected);
			return selected;
		}

		delete[] availableExtensions;
	}

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(physicalDevices[0], &properties);

	printf("Could not find an appropriate device. Falling back to device 0: %s\n", properties.deviceName);
	delete[] physicalDevices;

	VkPhysicalDevice fallback = physicalDevices[0];
	PrintDeviceInfo(fallback);
	return fallback;
}

static inline bool IsExtensionAvailable(std::string name)
{
	// for(const auto& available : ctx->availableExtensions)
	// 	if(strcmp(available.c_str(), name) == 0)
	// 		return true;
	return std::find(ctx->availableDeviceExtensions.begin(), ctx->availableDeviceExtensions.end(), name) != ctx->availableDeviceExtensions.end();
}

static bool __compare_VkPhysicalDeviceFeatures(VkPhysicalDeviceFeatures want, VkPhysicalDeviceFeatures have)
{
	bool validated = true;

	if(want.robustBufferAccess) validated = 		 			  validated && have.robustBufferAccess;
	if(want.fullDrawIndexUint32) validated = 		 			  validated && have.fullDrawIndexUint32;
	if(want.imageCubeArray) validated = 			 			  validated && have.imageCubeArray;
	if(want.independentBlend) validated = 			 			  validated && have.independentBlend;
	if(want.geometryShader) validated = 			 			  validated && have.geometryShader;
	if(want.tessellationShader) validated = 		 			  validated && have.tessellationShader;
	if(want.sampleRateShading) validated = 			 			  validated && have.sampleRateShading;
	if(want.dualSrcBlend) validated =                			  validated && have.dualSrcBlend;
	if(want.logicOp) validated = 					 			  validated && have.logicOp;
	if(want.multiDrawIndirect) validated = 			 			  validated && have.multiDrawIndirect;
	if(want.drawIndirectFirstInstance) validated =   			  validated && have.drawIndirectFirstInstance;
	if(want.depthClamp) validated = 				 			  validated && have.depthClamp;
	if(want.depthBiasClamp) validated = 			 			  validated && have.depthBiasClamp;
	if(want.fillModeNonSolid) validated =  			 			  validated && have.fillModeNonSolid;
	if(want.depthBounds) validated = 				 			  validated && have.depthBounds;
	if(want.wideLines) validated =     				 			  validated && have.wideLines;
	if(want.largePoints) validated = 				 			  validated && have.largePoints;
	if(want.alphaToOne) validated = 				 			  validated && have.alphaToOne;
	if(want.multiViewport) validated = 				 			  validated && have.multiViewport;
	if(want.samplerAnisotropy) validated = 			 			  validated && have.samplerAnisotropy;
	if(want.textureCompressionETC2) validated =      			  validated && have.textureCompressionETC2;
	if(want.textureCompressionASTC_LDR) validated =  			  validated && have.textureCompressionASTC_LDR;
	if(want.textureCompressionBC) validated =        			  validated && have.textureCompressionBC;
	if(want.occlusionQueryPrecise) validated =       			  validated && have.occlusionQueryPrecise;
	if(want.pipelineStatisticsQuery) validated =     			  validated && have.pipelineStatisticsQuery;
	if(want.vertexPipelineStoresAndAtomics) validated = 		  validated && have.vertexPipelineStoresAndAtomics;
	if(want.fragmentStoresAndAtomics) validated =    			  validated && have.fragmentStoresAndAtomics;
	if(want.shaderTessellationAndGeometryPointSize) validated =   validated && have.shaderTessellationAndGeometryPointSize;
	if(want.shaderImageGatherExtended) validated = 	 			  validated && have.shaderImageGatherExtended;
	if(want.shaderStorageImageExtendedFormats) validated = 		  validated && have.shaderStorageImageExtendedFormats;
	if(want.shaderStorageImageMultisample) validated = 			  validated && have.shaderStorageImageMultisample;
	if(want.shaderStorageImageReadWithoutFormat) validated =  	  validated && have.shaderStorageImageReadWithoutFormat;
	if(want.shaderStorageImageWriteWithoutFormat) validated =  	  validated && have.shaderStorageImageWriteWithoutFormat;
	if(want.shaderUniformBufferArrayDynamicIndexing) validated =  validated && have.shaderUniformBufferArrayDynamicIndexing;
	if(want.shaderSampledImageArrayDynamicIndexing) validated =   validated && have.shaderSampledImageArrayDynamicIndexing;
	if(want.shaderStorageBufferArrayDynamicIndexing) validated =  validated && have.shaderStorageBufferArrayDynamicIndexing;
	if(want.shaderStorageImageArrayDynamicIndexing) validated =   validated && have.shaderStorageImageArrayDynamicIndexing;
	if(want.shaderClipDistance) validated = 					  validated && have.shaderClipDistance;
	if(want.shaderCullDistance) validated = 					  validated && have.shaderCullDistance;
	if(want.shaderFloat64) validated = 							  validated && have.shaderFloat64;
	if(want.shaderInt64) validated = 							  validated && have.shaderInt64;
	if(want.shaderInt16) validated = 							  validated && have.shaderInt16;
	if(want.shaderResourceResidency) validated   = 				  validated && have.shaderResourceResidency;
	if(want.shaderResourceMinLod) validated   	 = 				  validated && have.shaderResourceMinLod;
	if(want.sparseBinding) validated = 			   				  validated && have.sparseBinding;
	if(want.sparseResidencyBuffer) validated = 					  validated && have.sparseResidencyBuffer;
	if(want.sparseResidencyImage2D) validated = 				  validated && have.sparseResidencyImage2D;
	if(want.sparseResidencyImage3D) validated = 				  validated && have.sparseResidencyImage3D;
	if(want.sparseResidency2Samples) validated = 				  validated && have.sparseResidency2Samples;
	if(want.sparseResidency4Samples) validated = 				  validated && have.sparseResidency4Samples;
	if(want.sparseResidency8Samples) validated = 				  validated && have.sparseResidency8Samples;
	if(want.sparseResidency16Samples) validated = 				  validated && have.sparseResidency16Samples;
	if(want.sparseResidencyAliased)   validated = 				  validated && have.sparseResidencyAliased;
	if(want.variableMultisampleRate)  validated = 				  validated && have.variableMultisampleRate;
	if(want.inheritedQueries) validated = 		  				  validated && have.inheritedQueries;

	return validated;
}

#endif
