#include <vector>
#include <iostream>
#include <cassert>
#include <vulkan/vulkan.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl << std::endl;

	return VK_FALSE;
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	createInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}


VkInstance initInstance(std::vector<const char*>& requiredExtensions, std::vector<const char*>& requiredLayers) {
	VkInstance instance{ VK_NULL_HANDLE };
	if (std::find(requiredExtensions.begin(), requiredExtensions.end(), "VK_KHR_surface") == requiredExtensions.end())
		requiredExtensions.push_back("VK_KHR_surface");
	if (std::find(requiredExtensions.begin(), requiredExtensions.end(), "VK_KHR_win32_surface") == requiredExtensions.end())
		requiredExtensions.push_back("VK_KHR_win32_surface");
	VkInstanceCreateInfo instanceCI{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	instanceCI.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	instanceCI.ppEnabledExtensionNames = requiredExtensions.data();
	instanceCI.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size());
	instanceCI.ppEnabledLayerNames = requiredLayers.data();
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	
	

	populateDebugMessengerCreateInfo(debugCreateInfo);
	instanceCI.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	
	VkResult res = vkCreateInstance(&instanceCI, nullptr, &instance);
	assert(res == VK_SUCCESS);
	assert(instance != VK_NULL_HANDLE);


	return instance;
}

void cleanupInstance(VkInstance instance) {
	vkDestroyInstance(instance, nullptr);
}


VkPhysicalDevice choosePhysicalDevice(VkInstance instance) {
	VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };
	uint32_t physicalDeviceCount = 0;
	VkResult res;
	res = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
	assert(res == VK_SUCCESS);
	assert(physicalDeviceCount > 0);
	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	res = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());
	assert(res == VK_SUCCESS);


	for (size_t i = 0; i < physicalDevices.size(); i++) {
		VkPhysicalDevice phys = physicalDevices[i];
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(phys, &physicalDeviceProperties);

		if (physicalDeviceProperties.deviceType & VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			physicalDevice = physicalDevices[i];
			break;
		}
	}
	



	return physicalDevice;
}





void CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		assert(func(instance, pCreateInfo, pAllocator, pDebugMessenger)==VK_SUCCESS);
	}
	else {
		assert(0);
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void setupDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, pDebugMessenger);
}


int main() {
	
	std::cout << "Initializing instance...";
	std::vector<const char*>requiredExtensions{ VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
	std::vector<const char*>requiredLayers = { "VK_LAYER_KHRONOS_validation" };//enable validation
	VkInstance instance = initInstance(requiredExtensions, requiredLayers);
	std::cout << "Done." << std::endl;
	std::cout << std::endl << "Setting up debug callback...";
	VkDebugUtilsMessengerEXT debugMessenger;
	setupDebugMessenger(instance, &debugMessenger);
	std::cout << "Done." << std::endl;
	std::cout << std::endl << "Choosing physical device...";
	VkPhysicalDevice physicalDevice = choosePhysicalDevice(instance);
	std::cout << "Done." << std::endl ;

	std::cout << std::endl << "Getting device extension properties...";
	uint32_t count = 0;
	VkResult res = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, nullptr);
	if (res == VK_SUCCESS) {
		std::vector<VkExtensionProperties> extensionProps(count);
		res = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, extensionProps.data());
		if (res == VK_SUCCESS) {
			for (auto& prop : extensionProps) {
				std::cout << "Extension: " << prop.extensionName << std::endl;
			}
		}
		else {
			std::cout << "error getting extension property data.";
		}
	}
	else {
		std::cout << "error getting device extension property count." << std::endl;
	}
	std::cout << "Done." << std::endl;
	DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	cleanupInstance(instance);
	return 0;
}