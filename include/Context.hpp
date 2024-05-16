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
	VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
};

constexpr static const char* WantedDeviceExtensions[] = {
	VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
};

const std::vector<const char*> RequiredDeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

namespace EnabledFeatures
{
	static bool NullDescriptor = false;
	// static bool MultiDrawIndirect = false;
	static bool MultiDrawIndirect = true;
};

#define LoadInstanceFunction(func) (reinterpret_cast<PFN_##func>(vkGetInstanceProcAddr(Context->instance, #func)))
#define LoadDeviceFunction(func) (reinterpret_cast<PFN_##func>(vkGetDeviceProcAddr(Context->device, #func)))

// ! FIND A WAY TO CHECK IF FEATURE IS AVAILABLE

constexpr static VkPhysicalDeviceFeatures WantedFeatures = {
	.robustBufferAccess = VK_TRUE,
	.multiDrawIndirect = VK_TRUE,
};

struct VulkanContextSingleton;
static VulkanContextSingleton CreateVulkanContext(const char* title, u32 windowWidth = 800, u32 windowHeight = 600);
static VkInstance CreateInstance(const char* title, std::unordered_set<std::string_view>& availableExtensions);
static VkPhysicalDevice ChoosePhysicalDevice(VkInstance instance, VkSurfaceKHR surface);

static bool __compare_VkPhysicalDeviceFeatures(VkPhysicalDeviceFeatures want, VkPhysicalDeviceFeatures have);
static inline bool IsExtensionAvailable(std::string name);

// struct AvailableExtensionArray
// {
// 	const char** availables;
// 	u32 num;

// 	inline bool operator[](const char* extName) {
// 		return std::hash<const char*>()(extName);
// 	}
// };

struct VulkanContextSingleton {
    VkInstance instance;
    VkDevice device;
    VkPhysicalDevice physDevice;
    VkSurfaceKHR surface;
    SDL_Window* window;

	std::unordered_set<std::string_view> availableDeviceExtensions;
	std::unordered_set<std::string_view> availableInstanceExtensions;
	VkPhysicalDeviceFeatures availableFeatures;

    NANO_SINGLETON_FUNCTION VulkanContextSingleton* GetSingleton() {
        static VulkanContextSingleton global{};
        return &global;
    }

	inline void Initialize(const char* title, u32 windowWidth, u32 windowHeight) {
		*GetSingleton() = CreateVulkanContext(title, windowWidth, windowHeight);
	}
};
static VulkanContextSingleton* Context = VulkanContextSingleton::GetSingleton();
static VulkanContextSingleton* ctx = Context; // ctx is much easier to use

static VkInstance& instance = ctx->instance;
static VkDevice& device = ctx->device;
static VkPhysicalDevice& physDevice = ctx->physDevice;
static VkSurfaceKHR& surface = ctx->surface;
static SDL_Window* window = ctx->window;

/* FUNCTION DEFINITIONS*/

static VulkanContextSingleton CreateVulkanContext(const char* title, u32 windowWidth, u32 windowHeight)
{
	EnabledFeatures::NullDescriptor = false;
	// ! REPLACE
	EnabledFeatures::MultiDrawIndirect = true;

    VulkanContextSingleton retval{};

    retval.window = SDL_CreateWindow(title, 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    assert(retval.window != nullptr);

    retval.instance = CreateInstance(title, retval.availableInstanceExtensions);
    assert(retval.instance != nullptr);
    
    SDL_Vulkan_CreateSurface(retval.window, retval.instance, nullptr, &retval.surface);
    assert(retval.surface != nullptr && SDL_GetError());
    
    retval.physDevice = ChoosePhysicalDevice(retval.instance, retval.surface);

	u32 queueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(retval.physDevice, &queueCount, nullptr);
	VkQueueFamilyProperties queueFamilies[queueCount];
	vkGetPhysicalDeviceQueueFamilyProperties(retval.physDevice, &queueCount, queueFamilies);

    // Clang loves complaining about these.
	__attribute__((unused)) u32 graphicsFamily, presentFamily, computeFamily;
	__attribute__((unused)) bool foundGraphicsFamily = false, foundPresentFamily = false, foundComputeFamily = false;
		
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
		if (presentSupport) {
			presentFamily = i;
			foundPresentFamily = true;
		}
		if (foundGraphicsFamily && foundComputeFamily && foundPresentFamily)
			break;
	
    	i++;
	}
    
    const std::set<u32> uniqueQueueFamilies = {graphicsFamily, presentFamily, computeFamily};
	VkDeviceQueueCreateInfo queueCreateInfos[uniqueQueueFamilies.size()];

	constexpr float queuePriority = 1.0f;

	u32 j = 0;
	for (const auto& queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueInfo{};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = queueFamily;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos[j] = queueInfo;
		j++;
	}

	const char* enabledExtensions[std::size(WantedDeviceExtensions) + RequiredDeviceExtensions.size()];

	u32 extensionCount;
	vkEnumerateDeviceExtensionProperties(retval.physDevice, nullptr, &extensionCount, nullptr);
	VkExtensionProperties extensions[extensionCount];
	vkEnumerateDeviceExtensionProperties(retval.physDevice, nullptr, &extensionCount, extensions);

	for(const auto& ext : extensions)
		retval.availableDeviceExtensions.insert(ext.extensionName);

	u32 enabledIterator = 0;

	for(const auto& wanted : WantedDeviceExtensions)
		for(const auto& available : extensions)
			if(strcmp(wanted, available.extensionName) == 0) {
				const char* extName = available.extensionName;
				std::cout << extName << '\n';
				enabledExtensions[enabledIterator] = available.extensionName;
				enabledIterator++;
			}

	for(const auto& required : RequiredDeviceExtensions) {
		bool validated = false;
		for(const auto& available : extensions)
			if(strcmp(required, available.extensionName) == 0) {
				enabledExtensions[enabledIterator] = available.extensionName;
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

	for(auto& queueCreate : queueCreateInfos) {
		queueCreate.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	}

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = uniqueQueueFamilies.size();
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
	deviceCreateInfo.enabledExtensionCount = enabledIterator;
	deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions;
	deviceCreateInfo.pEnabledFeatures = &WantedFeatures;
	// const void*& currentpNext = deviceCreateInfo.pNext;
	
	// Have to do it the hard way because ctx has not yet been initialized :<
	if(
		std::find(retval.availableDeviceExtensions.begin(),
				  retval.availableDeviceExtensions.end(),
				  VK_EXT_ROBUSTNESS_2_EXTENSION_NAME
		) != retval.availableDeviceExtensions.end()
		&&
	   	std::find(retval.availableInstanceExtensions.begin(),
				  retval.availableInstanceExtensions.end(),
				  VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
		) != retval.availableInstanceExtensions.end()
	  )
	{
		std::cout << "I want to die\n";
		VkPhysicalDeviceRobustness2FeaturesEXT robustness{};
		robustness.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;

		const auto& vkGetPhysicalDeviceFeatures2KHRLoaded = 
			reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2KHR>(vkGetInstanceProcAddr(retval.instance, "vkGetPhysicalDeviceFeatures2KHR"));
		assert(vkGetPhysicalDeviceFeatures2KHRLoaded != VK_NULL_HANDLE);

		VkPhysicalDeviceFeatures2KHR features2{};
		features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
		features2.pNext = &robustness;
		vkGetPhysicalDeviceFeatures2KHRLoaded(retval.physDevice, &features2);

		if(robustness.nullDescriptor) {
			VkPhysicalDeviceRobustness2FeaturesEXT enabledRobustness{};
			enabledRobustness.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
			enabledRobustness.nullDescriptor = VK_TRUE;

			deviceCreateInfo.pNext = &enabledRobustness;
			EnabledFeatures::NullDescriptor = true;
			std::cout << "Null descriptor enabled!\n";
		}
	}
	else
		std::cout << "no null desrciptor sad :<\n";

	if (vkCreateDevice(retval.physDevice, &deviceCreateInfo, nullptr, &retval.device) != VK_SUCCESS) {
		printf("Failed to create device\n");
		abort();
	}

    return retval;
}

static VkInstance CreateInstance(const char* title, std::unordered_set<std::string_view>& availableExtensions) {
    VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.applicationVersion = VK_API_VERSION_1_0;
	appInfo.apiVersion = VK_API_VERSION_1_0;
	appInfo.engineVersion = 0;
	appInfo.pApplicationName = title;
	appInfo.pEngineName = "No Engine";

	uint32_t SDLExtensionCount = 0;
	const char* const* SDLExtensions = SDL_Vulkan_GetInstanceExtensions(&SDLExtensionCount);
	std::vector<const char*> enabledExtensions;
	
	for (const auto& ext : RequiredInstanceExtensions)
		enabledExtensions.push_back(ext);

	for (uint32_t i = 0; i < SDLExtensionCount; i++) 
		enabledExtensions.push_back(SDLExtensions[i]);

	u32 extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	VkExtensionProperties extensions[extensionCount];
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
	VkLayerProperties layerProperties[layerCount];
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
			printf("VALIDTATION LAYERS COULD NOT BE LOADED\nRequested layers:\n");
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

    VkInstance instance;
	vkCreateInstance(&instanceCreateinfo, nullptr, &instance);

    return instance;
}

static VkPhysicalDevice ChoosePhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {

    VkPhysicalDevice chosenDevice = VK_NULL_HANDLE;
	uint32_t physDeviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &physDeviceCount, nullptr);
	
	if(physDeviceCount == 0) {
		printf("No physical devices found\n");
		abort();
	}

	VkPhysicalDevice physicalDevices[physDeviceCount];
	vkEnumeratePhysicalDevices(instance, &physDeviceCount, physicalDevices);

	for(const auto& device : physicalDevices) {
		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		VkSurfaceFormatKHR formats[formatCount];
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, formats);

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		VkPresentModeKHR presentModes[presentModeCount];
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, presentModes);

		uint32_t queueCount = 0;

		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, nullptr);
		VkQueueFamilyProperties queueFamilies[queueCount];
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, queueFamilies);
		
		bool extensionsAvailable = true;

		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		VkExtensionProperties availableExtensions[extensionCount];
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions);

		for (const auto& extension : RequiredDeviceExtensions) {
			bool validated = false;
			for(const auto& availableExt : availableExtensions) {
				if (strcmp(extension, availableExt.extensionName) == 0)
					validated = true;
			}
			if (!validated) {
				printf("Failed to validate extension with name: %s\n", extension);
				extensionsAvailable = false;
			}
		}
		
		bool foundGraphicsFamily = false, foundPresentFamily = false, foundComputeFamily = false;
		
		uint32_t i = 0;
		for (const auto& queueFamily : queueFamilies) {
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				foundGraphicsFamily = true;
			if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
				foundComputeFamily = true;
			if (presentSupport)
				foundPresentFamily = true;
			if (foundGraphicsFamily && foundComputeFamily && foundPresentFamily)
				break;

			i++;
		}

		if (((foundGraphicsFamily && foundComputeFamily && foundPresentFamily)) && extensionsAvailable && (formatCount > 0 && presentModeCount > 0)) { // Not sure how the last checks are supposed to be helpful.
			chosenDevice = device;
			break;
		}
	}

	if(chosenDevice == VK_NULL_HANDLE) {
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevices[0], &properties);

		printf("Could not find an appropriate device. Falling back to device 0: %s\n", properties.deviceName);
		chosenDevice = physicalDevices[0];
	}

	return chosenDevice;
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
