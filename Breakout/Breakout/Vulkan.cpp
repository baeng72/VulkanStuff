#include "Vulkan.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
	VkResult res = vkCreateInstance(&instanceCI, nullptr, &instance);
	assert(res == VK_SUCCESS);
	assert(instance != VK_NULL_HANDLE);

	return instance;
}

VkSurfaceKHR initSurface(VkInstance instance, GLFWwindow* window) {
	VkSurfaceKHR surface{ VK_NULL_HANDLE };

	glfwCreateWindowSurface(instance, window, nullptr, &surface);
	assert(surface != VK_NULL_HANDLE);

	return surface;
}

VkPhysicalDevice choosePhysicalDevice(VkInstance instance, VkSurfaceKHR surface, Queues& queues) {
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
	assert(physicalDevice != VK_NULL_HANDLE);
	uint32_t graphicsQueueFamily = UINT32_MAX;
	uint32_t presentQueueFamily = UINT32_MAX;
	uint32_t computeQueueFamily = UINT32_MAX;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());


	for (uint32_t i = 0; i < queueFamilyCount; i++) {
		VkBool32 supportsPresent = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresent);
		VkQueueFamilyProperties& queueProps = queueFamilyProperties[i];
		if (graphicsQueueFamily == UINT32_MAX && queueProps.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			graphicsQueueFamily = i;
		if (presentQueueFamily == UINT32_MAX && supportsPresent)
			presentQueueFamily = i;
		if (computeQueueFamily == UINT32_MAX && queueProps.queueFlags & VK_QUEUE_COMPUTE_BIT)
			computeQueueFamily = i;
		if (graphicsQueueFamily != UINT32_MAX && presentQueueFamily != UINT32_MAX && computeQueueFamily != UINT32_MAX)
			break;
	}
	assert(graphicsQueueFamily != UINT32_MAX && presentQueueFamily != UINT32_MAX && computeQueueFamily != UINT32_MAX);
	assert(computeQueueFamily == graphicsQueueFamily && graphicsQueueFamily == presentQueueFamily);//support one queue for now	
	queues.graphicsQueueFamily = graphicsQueueFamily;
	queues.presentQueueFamily = presentQueueFamily;
	queues.computeQueueFamily = computeQueueFamily;
	return physicalDevice;
}

VkDevice initDevice(VkPhysicalDevice physicalDevice, std::vector<const char*> deviceExtensions, Queues queues, VkPhysicalDeviceFeatures enabledFeatures) {
	VkDevice device{ VK_NULL_HANDLE };
	std::vector<float> queuePriorities;
	std::vector<VkDeviceQueueCreateInfo> queueCIs;

	if (queues.computeQueueFamily == queues.graphicsQueueFamily && queues.graphicsQueueFamily == queues.presentQueueFamily) {
		queuePriorities.push_back(1.0f);
		queueCIs.push_back({ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,nullptr,0,queues.graphicsQueueFamily,1,queuePriorities.data() });
	}
	else {
		//shouldn't get here for now
	}

	VkDeviceCreateInfo deviceCI{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCI.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	deviceCI.ppEnabledExtensionNames = deviceExtensions.data();
	deviceCI.pEnabledFeatures = &enabledFeatures;
	deviceCI.queueCreateInfoCount = static_cast<uint32_t>(queueCIs.size());
	deviceCI.pQueueCreateInfos = queueCIs.data();
	VkResult res = vkCreateDevice(physicalDevice, &deviceCI, nullptr, &device);
	assert(res == VK_SUCCESS);
	return device;
}

VkQueue getDeviceQueue(VkDevice device, uint32_t queueFamily) {
	VkQueue queue{ VK_NULL_HANDLE };
	vkGetDeviceQueue(device, queueFamily, 0, &queue);
	assert(queue);
	return queue;
}


VkPresentModeKHR chooseSwapchainPresentMode(std::vector<VkPresentModeKHR>& presentModes) {
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (size_t i = 0; i < presentModes.size(); i++) {
		if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = presentModes[i];
			break;
		}
		if ((presentMode != VK_PRESENT_MODE_MAILBOX_KHR) && (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)) {
			presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
		}
	}
	return presentMode;
}

VkSurfaceFormatKHR chooseSwapchainFormat(std::vector<VkSurfaceFormatKHR>& formats) {
	VkSurfaceFormatKHR format;
	if (formats.size() > 0)
		format = formats[0];

	for (auto&& surfaceFormat : formats) {
		if (surfaceFormat.format == PREFERRED_FORMAT) {
			format = surfaceFormat;
			break;
		}
	}
	return format;
}

VkSurfaceTransformFlagsKHR chooseSwapchainTransform(VkSurfaceCapabilitiesKHR& surfaceCaps) {

	VkSurfaceTransformFlagsKHR transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	if (!(surfaceCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR))
		transform = surfaceCaps.currentTransform;
	return transform;
}

VkCompositeAlphaFlagBitsKHR chooseSwapchainComposite(VkSurfaceCapabilitiesKHR& surfaceCaps) {
	VkCompositeAlphaFlagBitsKHR compositeFlags = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
	};
	for (auto& compositeAlphaFlag : compositeAlphaFlags) {
		if (surfaceCaps.supportedCompositeAlpha & compositeAlphaFlag) {
			compositeFlags = compositeAlphaFlag;
			break;
		};
	}
	return compositeFlags;
}

VkSwapchainKHR initSwapchain(VkDevice device, VkSurfaceKHR surface, uint32_t width, uint32_t height, VkSurfaceCapabilitiesKHR& surfaceCaps, VkPresentModeKHR& presentMode, VkSurfaceFormatKHR& swapchainFormat, VkExtent2D& swapchainExtent) {
	VkSwapchainKHR swapchain{ VK_NULL_HANDLE };

	VkSurfaceTransformFlagsKHR preTransform = chooseSwapchainTransform(surfaceCaps);
	VkCompositeAlphaFlagBitsKHR compositeAlpha = chooseSwapchainComposite(surfaceCaps);

	if (surfaceCaps.currentExtent.width == (uint32_t)-1) {
		swapchainExtent.width = width;
		swapchainExtent.height = height;
	}
	else {
		swapchainExtent = surfaceCaps.currentExtent;
	}

	uint32_t desiredNumberOfSwapchainImages = surfaceCaps.minImageCount + 1;
	if ((surfaceCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfaceCaps.maxImageCount))
	{
		desiredNumberOfSwapchainImages = surfaceCaps.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchainCI = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };

	swapchainCI.surface = surface;
	swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
	swapchainCI.imageFormat = swapchainFormat.format;
	swapchainCI.imageColorSpace = swapchainFormat.colorSpace;
	swapchainCI.imageExtent = { swapchainExtent.width, swapchainExtent.height };
	swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
	swapchainCI.imageArrayLayers = 1;
	swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCI.queueFamilyIndexCount = 0;
	swapchainCI.pQueueFamilyIndices = nullptr;
	swapchainCI.presentMode = presentMode;
	swapchainCI.oldSwapchain = VK_NULL_HANDLE;
	swapchainCI.clipped = VK_TRUE;
	swapchainCI.compositeAlpha = compositeAlpha;
	if (surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
		swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	// Enable transfer destination on swap chain images if supported
	if (surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
		swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}
	VkResult res = vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain);
	assert(res == VK_SUCCESS);
	return swapchain;
}

void getSwapchainImages(VkDevice device, VkSwapchainKHR swapchain, std::vector<VkImage>& images) {
	uint32_t imageCount = 0;
	VkResult res = vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
	assert(res == VK_SUCCESS);
	assert(imageCount > 0);
	images.resize(imageCount);
	res = vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data());
	assert(res == VK_SUCCESS);
}

void initSwapchainImageViews(VkDevice device, std::vector<VkImage>& swapchainImages, VkFormat& swapchainFormat, std::vector<VkImageView>& swapchainImageViews) {
	swapchainImageViews.resize(swapchainImages.size());
	for (size_t i = 0; i < swapchainImages.size(); i++) {
		VkImageViewCreateInfo viewCI{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		viewCI.format = swapchainFormat;
		viewCI.components = { VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY };
		viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;//attachment/view with be color
		viewCI.subresourceRange.baseMipLevel = 0;
		viewCI.subresourceRange.levelCount = 1;
		viewCI.subresourceRange.baseArrayLayer = 0;
		viewCI.subresourceRange.layerCount = 1;
		viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCI.image = swapchainImages[i];
		VkResult res = vkCreateImageView(device, &viewCI, nullptr, &swapchainImageViews[i]);
		assert(res == VK_SUCCESS);
	}
}

void cleanupSwapchainImageViews(VkDevice device, std::vector<VkImageView>& imageViews) {
	for (auto& imageView : imageViews) {
		vkDestroyImageView(device, imageView, nullptr);
	}
}

VkSemaphore initSemaphore(VkDevice device) {
	VkSemaphore semaphore{ VK_NULL_HANDLE };
	VkSemaphoreCreateInfo semaphoreCI{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkResult res = vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore);
	assert(res == VK_SUCCESS);
	return semaphore;
}

VkCommandPool initCommandPool(VkDevice device, uint32_t queueFamily) {
	VkCommandPool commandPool{ VK_NULL_HANDLE };
	VkCommandPoolCreateInfo cmdPoolCI{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	cmdPoolCI.queueFamilyIndex = queueFamily;
	cmdPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VkResult res = vkCreateCommandPool(device, &cmdPoolCI, nullptr, &commandPool);
	assert(res == VK_SUCCESS);
	return commandPool;
}

void initCommandPools(VkDevice device, size_t size, uint32_t queueFamily, std::vector<VkCommandPool>& commandPools) {
	commandPools.resize(size, VK_NULL_HANDLE);
	for (size_t i = 0; i < size; i++) {
		VkCommandPoolCreateInfo cmdPoolCI{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		cmdPoolCI.queueFamilyIndex = queueFamily;
		cmdPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VkResult res = vkCreateCommandPool(device, &cmdPoolCI, nullptr, &commandPools[i]);
		assert(res == VK_SUCCESS);
	}
}

VkCommandBuffer initCommandBuffer(VkDevice device, VkCommandPool commandPool) {
	VkCommandBuffer commandBuffer{ VK_NULL_HANDLE };
	VkCommandBufferAllocateInfo cmdBufAI{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	cmdBufAI.commandPool = commandPool;
	cmdBufAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufAI.commandBufferCount = 1;
	VkResult res = vkAllocateCommandBuffers(device, &cmdBufAI, &commandBuffer);
	assert(res == VK_SUCCESS);


	return commandBuffer;
}

void initCommandBuffers(VkDevice device, std::vector<VkCommandPool>& commandPools, std::vector<VkCommandBuffer>& commandBuffers) {
	commandBuffers.resize(commandPools.size(), VK_NULL_HANDLE);
	for (size_t i = 0; i < commandPools.size(); i++) {
		VkCommandBufferAllocateInfo cmdBufAI{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		cmdBufAI.commandPool = commandPools[i];
		cmdBufAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAI.commandBufferCount = 1;
		VkResult res = vkAllocateCommandBuffers(device, &cmdBufAI, &commandBuffers[i]);
		assert(res == VK_SUCCESS);
	}
}



VkRenderPass initRenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat) {
	VkRenderPass renderPass{ VK_NULL_HANDLE };
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = colorFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorRef = { 0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderCI = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderCI.attachmentCount = 2;
	renderCI.pAttachments = attachments;
	renderCI.subpassCount = 1;
	renderCI.pSubpasses = &subpass;
	renderCI.dependencyCount = 1;
	renderCI.pDependencies = &dependency;

	VkResult res = vkCreateRenderPass(device, &renderCI, nullptr, &renderPass);
	assert(res == VK_SUCCESS);

	return renderPass;
}


VkRenderPass initMSAARenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits numSamples) {
	VkRenderPass renderPass{ VK_NULL_HANDLE };
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = colorFormat;
	colorAttachment.samples = numSamples;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorRef = { 0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = depthFormat;
	depthAttachment.samples = numSamples;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachmentResolve{};
	colorAttachmentResolve.format = colorFormat;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentResolveRef{};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.pResolveAttachments = &colorAttachmentResolveRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment,colorAttachmentResolve };
	VkRenderPassCreateInfo renderCI = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderCI.attachmentCount = 3;
	renderCI.pAttachments = attachments;
	renderCI.subpassCount = 1;
	renderCI.pSubpasses = &subpass;
	renderCI.dependencyCount = 1;
	renderCI.pDependencies = &dependency;

	VkResult res = vkCreateRenderPass(device, &renderCI, nullptr, &renderPass);
	assert(res == VK_SUCCESS);

	return renderPass;
}

VkRenderPass initMSAARenderPass(VkDevice device, VkFormat colorFormat, VkSampleCountFlagBits numSamples) {
	VkRenderPass renderPass{ VK_NULL_HANDLE };
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = colorFormat;
	colorAttachment.samples = numSamples;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorRef = { 0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };



	VkAttachmentDescription colorAttachmentResolve{};
	colorAttachmentResolve.format = colorFormat;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentResolveRef{};
	colorAttachmentResolveRef.attachment = 1;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;
	subpass.pDepthStencilAttachment = nullptr;
	subpass.pResolveAttachments = &colorAttachmentResolveRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkAttachmentDescription attachments[] = { colorAttachment, colorAttachmentResolve };
	VkRenderPassCreateInfo renderCI = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderCI.attachmentCount = 2;
	renderCI.pAttachments = attachments;
	renderCI.subpassCount = 1;
	renderCI.pSubpasses = &subpass;
	renderCI.dependencyCount = 1;
	renderCI.pDependencies = &dependency;

	VkResult res = vkCreateRenderPass(device, &renderCI, nullptr, &renderPass);
	assert(res == VK_SUCCESS);

	return renderPass;
}



VkRenderPass initOffScreenRenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat) {
	VkRenderPass renderPass{ VK_NULL_HANDLE };
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = colorFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkAttachmentReference colorRef = { 0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderCI = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderCI.attachmentCount = 2;
	renderCI.pAttachments = attachments;
	renderCI.subpassCount = 1;
	renderCI.pSubpasses = &subpass;
	renderCI.dependencyCount = 1;
	renderCI.pDependencies = &dependency;

	VkResult res = vkCreateRenderPass(device, &renderCI, nullptr, &renderPass);
	assert(res == VK_SUCCESS);

	return renderPass;
}

VkRenderPass initShadowRenderPass(VkDevice device, VkFormat depthFormat) {
	VkRenderPass renderPass{ VK_NULL_HANDLE };


	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;// VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; we'll transion at render pass

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 0;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 0;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependencies[2] = { 0 };
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;


	VkAttachmentDescription attachments[] = { depthAttachment };
	VkRenderPassCreateInfo renderCI = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderCI.attachmentCount = 1;
	renderCI.pAttachments = attachments;
	renderCI.subpassCount = 1;
	renderCI.pSubpasses = &subpass;
	renderCI.dependencyCount = 2;
	renderCI.pDependencies = dependencies;

	VkResult res = vkCreateRenderPass(device, &renderCI, nullptr, &renderPass);
	assert(res == VK_SUCCESS);

	return renderPass;
}


void initFramebuffers(VkDevice device, VkRenderPass renderPass, std::vector<VkImageView>& colorAttachments, VkImageView depthImageView, uint32_t width, uint32_t height, std::vector<VkFramebuffer>& framebuffers) {
	framebuffers.resize(colorAttachments.size(), VK_NULL_HANDLE);
	for (size_t i = 0; i < colorAttachments.size(); i++) {
		std::vector<VkImageView> attachments;
		attachments.push_back(colorAttachments[i]);
		attachments.push_back(depthImageView);


		VkFramebufferCreateInfo fbCI{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		fbCI.renderPass = renderPass;
		fbCI.attachmentCount = static_cast<uint32_t>(attachments.size());
		fbCI.pAttachments = attachments.data();
		fbCI.width = width;
		fbCI.height = height;
		fbCI.layers = 1;
		VkResult res = vkCreateFramebuffer(device, &fbCI, nullptr, &framebuffers[i]);
		assert(res == VK_SUCCESS);
	}
}


void initFramebuffers(VkDevice device, VkRenderPass renderPass, VkImageView depthImageView, uint32_t width, uint32_t height, std::vector<VkFramebuffer>& framebuffers) {
	framebuffers.resize(1, VK_NULL_HANDLE);

	std::vector<VkImageView> attachments;

	attachments.push_back(depthImageView);


	VkFramebufferCreateInfo fbCI{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	fbCI.renderPass = renderPass;
	fbCI.attachmentCount = static_cast<uint32_t>(attachments.size());
	fbCI.pAttachments = attachments.data();
	fbCI.width = width;
	fbCI.height = height;
	fbCI.layers = 1;
	VkResult res = vkCreateFramebuffer(device, &fbCI, nullptr, &framebuffers[0]);
	assert(res == VK_SUCCESS);

}

void initFramebuffers(VkDevice device, VkRenderPass renderPass, VkImageView depthImageView, uint32_t width, uint32_t height, uint32_t layers, std::vector<VkFramebuffer>& framebuffers) {
	framebuffers.resize(1, VK_NULL_HANDLE);

	std::vector<VkImageView> attachments;

	attachments.push_back(depthImageView);


	VkFramebufferCreateInfo fbCI{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	fbCI.renderPass = renderPass;
	fbCI.attachmentCount = static_cast<uint32_t>(attachments.size());
	fbCI.pAttachments = attachments.data();
	fbCI.width = width;
	fbCI.height = height;
	fbCI.layers = layers;
	VkResult res = vkCreateFramebuffer(device, &fbCI, nullptr, &framebuffers[0]);
	assert(res == VK_SUCCESS);

}

void initMSAAFramebuffers(VkDevice device, VkRenderPass renderPass, std::vector<VkImageView>& colorAttachments, VkImageView depthImageView, VkImageView resolveAttachment, uint32_t width, uint32_t height, std::vector<VkFramebuffer>& framebuffers) {
	framebuffers.resize(colorAttachments.size(), VK_NULL_HANDLE);
	for (size_t i = 0; i < colorAttachments.size(); i++) {
		std::vector<VkImageView> attachments;
		attachments.push_back(resolveAttachment);
		attachments.push_back(depthImageView);
		attachments.push_back(colorAttachments[i]);

		VkFramebufferCreateInfo fbCI{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		fbCI.renderPass = renderPass;
		fbCI.attachmentCount = static_cast<uint32_t>(attachments.size());
		fbCI.pAttachments = attachments.data();
		fbCI.width = width;
		fbCI.height = height;
		fbCI.layers = 1;
		VkResult res = vkCreateFramebuffer(device, &fbCI, nullptr, &framebuffers[i]);
		assert(res == VK_SUCCESS);
	}
}

void initMSAAFramebuffers(VkDevice device, VkRenderPass renderPass, std::vector<VkImageView>& colorAttachments, VkImageView resolveAttachment, uint32_t width, uint32_t height, std::vector<VkFramebuffer>& framebuffers) {
	framebuffers.resize(colorAttachments.size(), VK_NULL_HANDLE);
	for (size_t i = 0; i < colorAttachments.size(); i++) {
		std::vector<VkImageView> attachments;
		attachments.push_back(resolveAttachment);
		attachments.push_back(colorAttachments[i]);




		VkFramebufferCreateInfo fbCI{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		fbCI.renderPass = renderPass;
		fbCI.attachmentCount = static_cast<uint32_t>(attachments.size());
		fbCI.pAttachments = attachments.data();
		fbCI.width = width;
		fbCI.height = height;
		fbCI.layers = 1;
		VkResult res = vkCreateFramebuffer(device, &fbCI, nullptr, &framebuffers[i]);
		assert(res == VK_SUCCESS);
	}
}

VkShaderModule initShaderModule(VkDevice device, const char* filename) {
	VkShaderModule shaderModule{ VK_NULL_HANDLE };
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.codeSize = buffer.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());
	VkResult res = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
	assert(res == VK_SUCCESS);
	return shaderModule;
}

uint32_t findMemoryType(uint32_t typeFilter, VkPhysicalDeviceMemoryProperties memoryProperties, VkMemoryPropertyFlags properties) {
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	assert(0);
	return 0;
}

VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDeviceProperties deviceProperties) {



	VkSampleCountFlags counts = deviceProperties.limits.framebufferColorSampleCounts & deviceProperties.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}


VkFence initFence(VkDevice device, VkFenceCreateFlags flags) {
	VkFenceCreateInfo fenceCI{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceCI.flags = flags;
	VkFence fence{ VK_NULL_HANDLE };
	VkResult res = vkCreateFence(device, &fenceCI, nullptr, &fence);
	assert(res == VK_SUCCESS);
	return fence;
}

void initBuffer(VkDevice device, VkPhysicalDeviceMemoryProperties& memoryProperties, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, Buffer& buffer) {
	VkBufferCreateInfo bufferCI{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferCI.size = size;
	bufferCI.usage = usageFlags;
	bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VkResult res = vkCreateBuffer(device, &bufferCI, nullptr, &buffer.buffer);
	assert(res == VK_SUCCESS);
	VkMemoryRequirements memReqs{};
	vkGetBufferMemoryRequirements(device, buffer.buffer, &memReqs);
	VkMemoryAllocateInfo memAllocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, memoryProperties, memoryPropertyFlags);
	res = vkAllocateMemory(device, &memAllocInfo, nullptr, &buffer.memory);
	assert(res == VK_SUCCESS);
	res = vkBindBufferMemory(device, buffer.buffer, buffer.memory, 0);
	assert(res == VK_SUCCESS);
	buffer.size = size;
}

void* mapBuffer(VkDevice device, Buffer& buffer) {
	void* pData{ nullptr };
	VkResult res = vkMapMemory(device, buffer.memory, 0, buffer.size, 0, &pData);
	assert(res == VK_SUCCESS);
	return pData;
}

void unmapBuffer(VkDevice device, Buffer& buffer) {
	vkUnmapMemory(device, buffer.memory);
}

void CopyBufferTo(VkDevice device, VkQueue queue, VkCommandBuffer cmd, Buffer& src, Buffer& dst, VkDeviceSize size) {
	VkBufferCopy copyRegion = {};
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

	VkResult res = vkBeginCommandBuffer(cmd, &beginInfo);
	assert(res == VK_SUCCESS);

	copyRegion.size = size;
	vkCmdCopyBuffer(cmd, src.buffer, dst.buffer, 1, &copyRegion);

	res = vkEndCommandBuffer(cmd);
	assert(res == VK_SUCCESS);

	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;

	VkFence fence = initFence(device);


	res = vkQueueSubmit(queue, 1, &submitInfo, fence);
	assert(res == VK_SUCCESS);

	res = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
	assert(res == VK_SUCCESS);


	vkDestroyFence(device, fence, nullptr);
}

void initImage(VkDevice device, VkFormat format, VkFormatProperties& formatProperties, VkPhysicalDeviceMemoryProperties& memoryProperties, VkMemoryPropertyFlags memoryPropertyFlags, uint32_t width, uint32_t height, Image& image) {
	VkImageCreateInfo imageCI{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.format = format;
	imageCI.extent = { width,height,1 };
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 1;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	VkResult res = vkCreateImage(device, &imageCI, nullptr, &image.image);
	assert(res == VK_SUCCESS);

	VkMemoryRequirements memReqs{};
	vkGetImageMemoryRequirements(device, image.image, &memReqs);
	VkMemoryAllocateInfo memAllocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, memoryProperties, memoryPropertyFlags);
	res = vkAllocateMemory(device, &memAllocInfo, nullptr, &image.memory);
	assert(res == VK_SUCCESS);
	res = vkBindImageMemory(device, image.image, image.memory, 0);
	assert(res == VK_SUCCESS);

	VkImageViewCreateInfo imageViewCI{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCI.format = format;
	imageViewCI.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	imageViewCI.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1 };
	imageViewCI.image = image.image;
	res = vkCreateImageView(device, &imageViewCI, nullptr, &image.imageView);
	assert(res == VK_SUCCESS);

	VkSamplerCreateInfo samplerCI{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerCI.magFilter = samplerCI.minFilter = VK_FILTER_LINEAR;
	samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCI.addressModeU = samplerCI.addressModeV = samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerCI.mipLodBias = 0.0f;
	samplerCI.maxAnisotropy = 1.0f;
	samplerCI.compareOp = VK_COMPARE_OP_NEVER;
	samplerCI.minLod = samplerCI.maxLod = 0.0f;
	samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	res = vkCreateSampler(device, &samplerCI, nullptr, &image.sampler);
	assert(res == VK_SUCCESS);
	image.width = width;
	image.height = height;
}

void initSampler(VkDevice device, Image& image) {
	VkSamplerCreateInfo samplerCI{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerCI.magFilter = samplerCI.minFilter = VK_FILTER_LINEAR;
	samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCI.addressModeU = samplerCI.addressModeV = samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCI.mipLodBias = 0.0f;
	samplerCI.maxAnisotropy = 1.0f;
	samplerCI.compareOp = VK_COMPARE_OP_NEVER;
	samplerCI.minLod = 0.0f;
	samplerCI.maxLod = 1.0f;
	samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VkResult res = vkCreateSampler(device, &samplerCI, nullptr, &image.sampler);
	assert(res == VK_SUCCESS);
}

void initMSAAImage(VkDevice device, VkFormat format, VkFormatProperties& formatProperties, VkPhysicalDeviceMemoryProperties& memoryProperties, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags, uint32_t width, uint32_t height, VkSampleCountFlagBits numSamples, Image& image) {

	VkImageCreateInfo imageCI{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.format = format;
	imageCI.extent = { width,height,1 };
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 1;
	imageCI.samples = numSamples;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCI.usage = usage;

	VkResult res = vkCreateImage(device, &imageCI, nullptr, &image.image);
	assert(res == VK_SUCCESS);

	VkMemoryRequirements memReqs{};
	vkGetImageMemoryRequirements(device, image.image, &memReqs);
	VkMemoryAllocateInfo memAllocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, memoryProperties, memoryPropertyFlags);
	res = vkAllocateMemory(device, &memAllocInfo, nullptr, &image.memory);
	assert(res == VK_SUCCESS);
	res = vkBindImageMemory(device, image.image, image.memory, 0);
	assert(res == VK_SUCCESS);

	VkImageViewCreateInfo imageViewCI{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCI.format = format;
	imageViewCI.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	imageViewCI.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1 };
	imageViewCI.image = image.image;
	imageViewCI.subresourceRange.levelCount = 1;


	res = vkCreateImageView(device, &imageViewCI, nullptr, &image.imageView);
	assert(res == VK_SUCCESS);

	image.width = width;
	image.height = height;
	image.mipLevels = 1;
}

void initColorAttachmentImage(VkDevice device, VkFormat format, VkFormatProperties& formatProperties, VkPhysicalDeviceMemoryProperties& memoryProperties, VkMemoryPropertyFlags memoryPropertyFlags, uint32_t width, uint32_t height, Image& image) {
	VkImageCreateInfo imageCI{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.format = format;
	imageCI.extent = { width,height,1 };
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 1;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	VkResult res = vkCreateImage(device, &imageCI, nullptr, &image.image);
	assert(res == VK_SUCCESS);

	VkMemoryRequirements memReqs{};
	vkGetImageMemoryRequirements(device, image.image, &memReqs);
	VkMemoryAllocateInfo memAllocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, memoryProperties, memoryPropertyFlags);
	res = vkAllocateMemory(device, &memAllocInfo, nullptr, &image.memory);
	assert(res == VK_SUCCESS);
	res = vkBindImageMemory(device, image.image, image.memory, 0);
	assert(res == VK_SUCCESS);

	VkImageViewCreateInfo imageViewCI{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCI.format = format;
	imageViewCI.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	imageViewCI.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1 };
	imageViewCI.image = image.image;
	res = vkCreateImageView(device, &imageViewCI, nullptr, &image.imageView);
	assert(res == VK_SUCCESS);

	VkSamplerCreateInfo samplerCI{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerCI.magFilter = samplerCI.minFilter = VK_FILTER_LINEAR;
	samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCI.addressModeU = samplerCI.addressModeV = samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerCI.mipLodBias = 0.0f;
	samplerCI.maxAnisotropy = 1.0f;
	samplerCI.compareOp = VK_COMPARE_OP_NEVER;
	samplerCI.minLod = samplerCI.maxLod = 0.0f;
	samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	res = vkCreateSampler(device, &samplerCI, nullptr, &image.sampler);
	assert(res == VK_SUCCESS);
	image.width = width;
	image.height = height;
}

void initDepthImage(VkDevice device, VkFormat format, VkFormatProperties& formatProperties, VkPhysicalDeviceMemoryProperties& memoryProperties, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags, uint32_t width, uint32_t height, Image& image) {

	VkImageCreateInfo imageCI{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.format = format;
	imageCI.extent = { (uint32_t)width,(uint32_t)height,1 };
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 1;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCI.usage = usage;
	VkResult res = vkCreateImage(device, &imageCI, nullptr, &image.image);
	assert(res == VK_SUCCESS);

	VkMemoryRequirements memReqs{};
	vkGetImageMemoryRequirements(device, image.image, &memReqs);
	VkMemoryAllocateInfo memAllocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, memoryProperties, memoryPropertyFlags);
	res = vkAllocateMemory(device, &memAllocInfo, nullptr, &image.memory);
	assert(res == VK_SUCCESS);
	res = vkBindImageMemory(device, image.image, image.memory, 0);
	assert(res == VK_SUCCESS);

	VkImageViewCreateInfo imageViewCI{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCI.format = format;
	//imageViewCI.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	imageViewCI.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT,0,1,0,1 };
	imageViewCI.image = image.image;
	imageViewCI.subresourceRange.levelCount = 1;

	res = vkCreateImageView(device, &imageViewCI, nullptr, &image.imageView);
	assert(res == VK_SUCCESS);



	image.width = width;
	image.height = height;
	image.mipLevels = 1;
}

void initDepthImage(VkDevice device, VkFormat format, VkFormatProperties& formatProperties, VkPhysicalDeviceMemoryProperties& memoryProperties, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags, VkSampleCountFlagBits numSamples, uint32_t width, uint32_t height, Image& image) {

	VkImageCreateInfo imageCI{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.format = format;
	imageCI.extent = { (uint32_t)width,(uint32_t)height,1 };
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 1;
	imageCI.samples = numSamples;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCI.usage = usage;
	VkResult res = vkCreateImage(device, &imageCI, nullptr, &image.image);
	assert(res == VK_SUCCESS);

	VkMemoryRequirements memReqs{};
	vkGetImageMemoryRequirements(device, image.image, &memReqs);
	VkMemoryAllocateInfo memAllocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, memoryProperties, memoryPropertyFlags);
	res = vkAllocateMemory(device, &memAllocInfo, nullptr, &image.memory);
	assert(res == VK_SUCCESS);
	res = vkBindImageMemory(device, image.image, image.memory, 0);
	assert(res == VK_SUCCESS);

	VkImageViewCreateInfo imageViewCI{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCI.format = format;
	//imageViewCI.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	imageViewCI.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT,0,1,0,1 };
	imageViewCI.image = image.image;
	imageViewCI.subresourceRange.levelCount = 1;

	res = vkCreateImageView(device, &imageViewCI, nullptr, &image.imageView);
	assert(res == VK_SUCCESS);



	image.width = width;
	image.height = height;
	image.mipLevels = 1;
}

void initTextureImage(VkDevice device, VkFormat format, VkFormatProperties& formatProperties, VkPhysicalDeviceMemoryProperties& memoryProperties, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags, uint32_t width, uint32_t height, Image& image) {

	uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
	VkImageCreateInfo imageCI{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.format = format;
	imageCI.extent = { (uint32_t)width,(uint32_t)height,1 };
	imageCI.mipLevels = mipLevels;
	imageCI.arrayLayers = 1;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCI.usage = usage;
	VkResult res = vkCreateImage(device, &imageCI, nullptr, &image.image);
	assert(res == VK_SUCCESS);

	VkMemoryRequirements memReqs{};
	vkGetImageMemoryRequirements(device, image.image, &memReqs);
	VkMemoryAllocateInfo memAllocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, memoryProperties, memoryPropertyFlags);
	res = vkAllocateMemory(device, &memAllocInfo, nullptr, &image.memory);
	assert(res == VK_SUCCESS);
	res = vkBindImageMemory(device, image.image, image.memory, 0);
	assert(res == VK_SUCCESS);

	VkImageViewCreateInfo imageViewCI{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCI.format = format;
	//imageViewCI.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	imageViewCI.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1 };
	if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	imageViewCI.image = image.image;
	imageViewCI.subresourceRange.levelCount = mipLevels;

	res = vkCreateImageView(device, &imageViewCI, nullptr, &image.imageView);
	assert(res == VK_SUCCESS);


	VkSamplerCreateInfo samplerCI{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerCI.magFilter = samplerCI.minFilter = VK_FILTER_LINEAR;
	samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCI.addressModeU = samplerCI.addressModeV = samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCI.mipLodBias = 0.0f;
	samplerCI.anisotropyEnable = VK_TRUE;
	samplerCI.maxAnisotropy = 16.0f;
	samplerCI.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerCI.minLod = 0.0f;
	samplerCI.maxLod = (float)mipLevels;
	samplerCI.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	res = vkCreateSampler(device, &samplerCI, nullptr, &image.sampler);
	assert(res == VK_SUCCESS);
	image.width = width;
	image.height = height;
	image.mipLevels = mipLevels;
}

void initTextureImageNoMip(VkDevice device, VkFormat format, VkFormatProperties& formatProperties, VkPhysicalDeviceMemoryProperties& memoryProperties, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags, uint32_t width, uint32_t height, Image& image) {

	uint32_t mipLevels = 1;
	VkImageCreateInfo imageCI{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.format = format;
	imageCI.extent = { (uint32_t)width,(uint32_t)height,1 };
	imageCI.mipLevels = mipLevels;
	imageCI.arrayLayers = 1;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCI.usage = usage;
	VkResult res = vkCreateImage(device, &imageCI, nullptr, &image.image);
	assert(res == VK_SUCCESS);

	VkMemoryRequirements memReqs{};
	vkGetImageMemoryRequirements(device, image.image, &memReqs);
	VkMemoryAllocateInfo memAllocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, memoryProperties, memoryPropertyFlags);
	res = vkAllocateMemory(device, &memAllocInfo, nullptr, &image.memory);
	assert(res == VK_SUCCESS);
	res = vkBindImageMemory(device, image.image, image.memory, 0);
	assert(res == VK_SUCCESS);

	VkImageViewCreateInfo imageViewCI{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCI.format = format;
	//imageViewCI.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	imageViewCI.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1 };
	if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	imageViewCI.image = image.image;
	imageViewCI.subresourceRange.levelCount = mipLevels;

	res = vkCreateImageView(device, &imageViewCI, nullptr, &image.imageView);
	assert(res == VK_SUCCESS);


	VkSamplerCreateInfo samplerCI{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerCI.magFilter = samplerCI.minFilter = VK_FILTER_LINEAR;
	samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCI.addressModeU = samplerCI.addressModeV = samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCI.mipLodBias = 0.0f;
	samplerCI.anisotropyEnable = VK_TRUE;
	samplerCI.maxAnisotropy = 16.0f;
	samplerCI.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerCI.minLod = 0.0f;
	samplerCI.maxLod = (float)mipLevels;
	samplerCI.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	res = vkCreateSampler(device, &samplerCI, nullptr, &image.sampler);
	assert(res == VK_SUCCESS);
	image.width = width;
	image.height = height;
	image.mipLevels = mipLevels;


}

void initCubemapTextureImage(VkDevice device, VkFormat format, VkFormatProperties& formatProperties, VkPhysicalDeviceMemoryProperties& memoryProperties, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags, uint32_t width, uint32_t height, Image& image) {

	//uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
	VkImageCreateInfo imageCI{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.format = format;
	imageCI.extent = { (uint32_t)width,(uint32_t)height,1 };
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 6;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCI.usage = usage;
	imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	VkResult res = vkCreateImage(device, &imageCI, nullptr, &image.image);
	assert(res == VK_SUCCESS);

	VkMemoryRequirements memReqs{};
	vkGetImageMemoryRequirements(device, image.image, &memReqs);
	VkMemoryAllocateInfo memAllocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, memoryProperties, memoryPropertyFlags);
	res = vkAllocateMemory(device, &memAllocInfo, nullptr, &image.memory);
	assert(res == VK_SUCCESS);
	res = vkBindImageMemory(device, image.image, image.memory, 0);
	assert(res == VK_SUCCESS);

	VkImageViewCreateInfo imageViewCI{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	imageViewCI.format = format;
	imageViewCI.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	imageViewCI.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1 };
	imageViewCI.subresourceRange.layerCount = 6;
	imageViewCI.image = image.image;
	imageViewCI.subresourceRange.levelCount = 1;// mipLevels;

	res = vkCreateImageView(device, &imageViewCI, nullptr, &image.imageView);
	assert(res == VK_SUCCESS);


	VkSamplerCreateInfo samplerCI{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerCI.magFilter = samplerCI.minFilter = VK_FILTER_LINEAR;
	samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCI.addressModeU = samplerCI.addressModeV = samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCI.mipLodBias = 0.0f;
	samplerCI.anisotropyEnable = VK_TRUE;
	samplerCI.maxAnisotropy = 16.0f;
	samplerCI.compareOp = VK_COMPARE_OP_NEVER;
	samplerCI.minLod = 0.0f;
	samplerCI.maxLod = (float)1;// mipLevels;
	samplerCI.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	res = vkCreateSampler(device, &samplerCI, nullptr, &image.sampler);
	assert(res == VK_SUCCESS);
	image.width = width;
	image.height = height;
	image.mipLevels = 1;// mipLevels;
}


void initCubemapDepthImage(VkDevice device, VkFormat format, VkFormatProperties& formatProperties, VkPhysicalDeviceMemoryProperties& memoryProperties, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags, uint32_t width, uint32_t height, Image& image) {

	//uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
	VkImageCreateInfo imageCI{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.format = format;
	imageCI.extent = { (uint32_t)width,(uint32_t)height,1 };
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 6;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCI.usage = usage;
	imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	VkResult res = vkCreateImage(device, &imageCI, nullptr, &image.image);
	assert(res == VK_SUCCESS);

	VkMemoryRequirements memReqs{};
	vkGetImageMemoryRequirements(device, image.image, &memReqs);
	VkMemoryAllocateInfo memAllocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, memoryProperties, memoryPropertyFlags);
	res = vkAllocateMemory(device, &memAllocInfo, nullptr, &image.memory);
	assert(res == VK_SUCCESS);
	res = vkBindImageMemory(device, image.image, image.memory, 0);
	assert(res == VK_SUCCESS);

	VkImageViewCreateInfo imageViewCI{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	imageViewCI.format = format;
	imageViewCI.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	imageViewCI.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT,0,1,0,1 };
	imageViewCI.subresourceRange.layerCount = 6;
	imageViewCI.image = image.image;
	imageViewCI.subresourceRange.levelCount = 1;// mipLevels;

	res = vkCreateImageView(device, &imageViewCI, nullptr, &image.imageView);
	assert(res == VK_SUCCESS);


	VkSamplerCreateInfo samplerCI{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerCI.magFilter = samplerCI.minFilter = VK_FILTER_NEAREST;
	samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCI.addressModeU = samplerCI.addressModeV = samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCI.mipLodBias = 0.0f;
	samplerCI.anisotropyEnable = VK_TRUE;
	samplerCI.maxAnisotropy = 16.0f;
	samplerCI.compareOp = VK_COMPARE_OP_NEVER;
	samplerCI.minLod = 0.0f;
	samplerCI.maxLod = (float)1;// mipLevels;
	samplerCI.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	res = vkCreateSampler(device, &samplerCI, nullptr, &image.sampler);
	assert(res == VK_SUCCESS);
	image.width = width;
	image.height = height;
	image.mipLevels = 1;// mipLevels;


}

void generateMipMaps(VkDevice device, VkQueue queue, VkCommandBuffer cmd, Image& image) {
	//create mip maps
	uint32_t mipLevels = image.mipLevels;
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

	VkResult res = vkBeginCommandBuffer(cmd, &beginInfo);
	assert(res == VK_SUCCESS);
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image.image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = image.width;
	int32_t mipHeight = image.height;

	for (uint32_t i = 1; i < mipLevels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(cmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(cmd,
			image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);


		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}
	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmd,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	res = vkEndCommandBuffer(cmd);
	assert(res == VK_SUCCESS);

	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;

	VkFence fence = initFence(device);


	res = vkQueueSubmit(queue, 1, &submitInfo, fence);
	assert(res == VK_SUCCESS);

	res = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
	assert(res == VK_SUCCESS);

	vkDestroyFence(device, fence, nullptr);

}

void CopyBufferToImage(VkDevice device, VkQueue queue, VkCommandBuffer cmd, Buffer& src, Image& dst, uint32_t width, uint32_t height, VkDeviceSize offset, uint32_t arrayLayer) {
	VkBufferCopy copyRegion = {};
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

	VkResult res = vkBeginCommandBuffer(cmd, &beginInfo);
	assert(res == VK_SUCCESS);

	VkBufferImageCopy region{};
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent = { width,height,1 };
	region.bufferOffset = offset;
	region.imageSubresource.baseArrayLayer = arrayLayer;
	vkCmdCopyBufferToImage(cmd,
		src.buffer,
		dst.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region);
	res = vkEndCommandBuffer(cmd);
	assert(res == VK_SUCCESS);

	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;

	VkFence fence = initFence(device);


	res = vkQueueSubmit(queue, 1, &submitInfo, fence);
	assert(res == VK_SUCCESS);

	res = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
	assert(res == VK_SUCCESS);


	vkDestroyFence(device, fence, nullptr);
}

void transitionImage(VkDevice device, VkQueue queue, VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t layerCount) {
	VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = layerCount;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {

	}
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VkResult res = vkBeginCommandBuffer(cmd, &beginInfo);
	assert(res == VK_SUCCESS);
	vkCmdPipelineBarrier(cmd, sourceStage, destinationStage,
		0, 0, nullptr, 0, nullptr, 1, &barrier);
	res = vkEndCommandBuffer(cmd);
	assert(res == VK_SUCCESS);

	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;

	VkFence fence = initFence(device);


	res = vkQueueSubmit(queue, 1, &submitInfo, fence);
	assert(res == VK_SUCCESS);

	res = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
	assert(res == VK_SUCCESS);


	vkDestroyFence(device, fence, nullptr);
}

VkDescriptorSetLayout initDescriptorSetLayout(VkDevice device, std::vector<VkDescriptorSetLayoutBinding>& descriptorBindings) {
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
	VkDescriptorSetLayoutCreateInfo descLayoutCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	descLayoutCI.bindingCount = static_cast<uint32_t>(descriptorBindings.size());
	descLayoutCI.pBindings = descriptorBindings.data();
	VkResult res = vkCreateDescriptorSetLayout(device, &descLayoutCI, nullptr, &descriptorSetLayout);
	assert(res == VK_SUCCESS);
	return descriptorSetLayout;
}

VkDescriptorPool initDescriptorPool(VkDevice device, std::vector<VkDescriptorPoolSize>& descriptorPoolSizes, uint32_t maxSets) {
	VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
	VkDescriptorPoolCreateInfo descPoolCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	descPoolCI.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
	descPoolCI.pPoolSizes = descriptorPoolSizes.data();
	descPoolCI.maxSets = maxSets;
	VkResult res = vkCreateDescriptorPool(device, &descPoolCI, nullptr, &descriptorPool);
	assert(res == VK_SUCCESS);
	return descriptorPool;
}

VkDescriptorSet initDescriptorSet(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool) {
	VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
	VkDescriptorSetAllocateInfo descAI{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	descAI.descriptorPool = descriptorPool;
	descAI.descriptorSetCount = 1;
	descAI.pSetLayouts = &descriptorSetLayout;

	VkResult res = vkAllocateDescriptorSets(device, &descAI, &descriptorSet);
	assert(res == VK_SUCCESS);
	return descriptorSet;
}

void updateDescriptorSets(VkDevice device, std::vector<VkWriteDescriptorSet> descriptorWrites) {
	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

VkPipelineLayout initPipelineLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout) {
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkPipelineLayoutCreateInfo layoutCI{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	layoutCI.pSetLayouts = descriptorSetLayout == VK_NULL_HANDLE ? nullptr : &descriptorSetLayout;
	layoutCI.setLayoutCount = descriptorSetLayout == VK_NULL_HANDLE ? 0 : 1;
	VkResult res = vkCreatePipelineLayout(device, &layoutCI, nullptr, &pipelineLayout);
	assert(res == VK_SUCCESS);
	return pipelineLayout;
}

VkPipeline initGraphicsPipeline(VkDevice device, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkExtent2D extent, std::vector<ShaderModule>& shaders, VkVertexInputBindingDescription& bindingDescription, std::vector<VkVertexInputAttributeDescription>& attributeDescriptions, VkCullModeFlags cullMode, bool depthTest, VkSampleCountFlagBits numSamples, VkBool32 blendEnable) {
	VkPipeline pipeline{ VK_NULL_HANDLE };

	//we're working with triangles;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Specify rasterization state. 
	VkPipelineRasterizationStateCreateInfo raster{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	raster.polygonMode = VK_POLYGON_MODE_FILL;
	raster.cullMode = cullMode;// VK_CULL_MODE_BACK_BIT;// VK_CULL_MODE_NONE;
	raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; //VK_FRONT_FACE_CLOCKWISE;// 
	raster.lineWidth = 1.0f;

	//all colors, no blending
	VkPipelineColorBlendAttachmentState blendAttachment{};
	blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	blendAttachment.blendEnable = blendEnable;
	//blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
	//blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	//blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	//blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;// ONE_MINUS_DST_COLOR;
	//blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	//blendAttachment.alphaBlendOp = VK_BLEND_OP_SUBTRACT;
	blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;


	VkPipelineColorBlendStateCreateInfo blend{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	blend.attachmentCount = 1;
	blend.logicOpEnable = VK_FALSE;
	blend.logicOp = VK_LOGIC_OP_COPY;
	blend.pAttachments = &blendAttachment;

	//viewport & scissor box
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)extent.width;
	viewport.height = (float)extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = extent;
	VkPipelineViewportStateCreateInfo viewportCI{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportCI.viewportCount = 1;
	viewportCI.pViewports = &viewport;
	viewportCI.scissorCount = 1;
	viewportCI.pScissors = &scissor;

	VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

	depthStencil.depthTestEnable = depthTest ? VK_TRUE : VK_FALSE;
	depthStencil.depthWriteEnable = depthTest ? VK_TRUE : VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;
	/*depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER;*/

	VkPipelineMultisampleStateCreateInfo multisamplingCI{};
	multisamplingCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingCI.sampleShadingEnable = VK_FALSE;
	multisamplingCI.rasterizationSamples = numSamples;
	if (numSamples != VK_SAMPLE_COUNT_1_BIT)
		multisamplingCI.minSampleShading = 0.5f; // Optional
	else
		multisamplingCI.minSampleShading = 1.0f; // Optional
	multisamplingCI.pSampleMask = nullptr; // Optional
	multisamplingCI.alphaToCoverageEnable = VK_FALSE; // Optional
	multisamplingCI.alphaToOneEnable = VK_FALSE; // Optional

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	for (auto& shaderModule : shaders) {
		VkPipelineShaderStageCreateInfo shaderCI{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		shaderCI.stage = shaderModule.stage;
		shaderCI.module = shaderModule.shaderModule;
		shaderCI.pName = "main";
		shaderStages.push_back(shaderCI);
	}

	VkPipelineVertexInputStateCreateInfo vertexCI{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexCI.vertexBindingDescriptionCount = 1;
	vertexCI.pVertexBindingDescriptions = &bindingDescription;
	vertexCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexCI.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkGraphicsPipelineCreateInfo pipe{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipe.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipe.pStages = shaderStages.data();
	pipe.pVertexInputState = &vertexCI;
	pipe.pInputAssemblyState = &inputAssembly;
	pipe.pRasterizationState = &raster;
	pipe.pColorBlendState = &blend;
	pipe.pMultisampleState = &multisamplingCI;
	pipe.pViewportState = &viewportCI;
	pipe.pDepthStencilState = &depthStencil;
	pipe.pDynamicState = nullptr;

	pipe.renderPass = renderPass;
	pipe.layout = pipelineLayout;

	VkResult res = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipe, nullptr, &pipeline);
	assert(res == VK_SUCCESS);

	return pipeline;

}


VkPipeline initDepthOnlyGraphicsPipeline(VkDevice device, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkExtent2D extent, std::vector<ShaderModule>& shaders, VkVertexInputBindingDescription& bindingDescription, std::vector<VkVertexInputAttributeDescription>& attributeDescriptions, VkCullModeFlags cullMode) {
	VkPipeline pipeline{ VK_NULL_HANDLE };

	//we're working with triangles;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Specify rasterization state. 
	VkPipelineRasterizationStateCreateInfo raster{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	raster.polygonMode = VK_POLYGON_MODE_FILL;
	raster.cullMode = cullMode;// VK_CULL_MODE_BACK_BIT;// VK_CULL_MODE_NONE;
	raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; //VK_FRONT_FACE_CLOCKWISE;//  
	raster.lineWidth = 1.0f;
	raster.depthBiasEnable = VK_TRUE;
	raster.depthBiasConstantFactor = 0.01f;
	raster.depthBiasSlopeFactor = 0.01f;

	//all colors, no blending
	//VkPipelineColorBlendAttachmentState blendAttachment{};
	//blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo blend{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	blend.attachmentCount = 0;
	blend.logicOpEnable = VK_FALSE;
	blend.logicOp = VK_LOGIC_OP_COPY;
	//blend.pAttachments = &blendAttachment;

	//viewport & scissor box
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)extent.width;
	viewport.height = (float)extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = extent;
	VkPipelineViewportStateCreateInfo viewportCI{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportCI.viewportCount = 1;
	viewportCI.pViewports = &viewport;
	viewportCI.scissorCount = 1;
	viewportCI.pScissors = &scissor;

	VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;// _OR_EQUAL;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;
	/*depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER;*/

	VkPipelineMultisampleStateCreateInfo multisamplingCI{};
	multisamplingCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingCI.sampleShadingEnable = VK_FALSE;
	multisamplingCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisamplingCI.pSampleMask = nullptr; // Optional
	multisamplingCI.alphaToCoverageEnable = VK_FALSE; // Optional
	multisamplingCI.alphaToOneEnable = VK_FALSE; // Optional

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	for (auto& shaderModule : shaders) {
		VkPipelineShaderStageCreateInfo shaderCI{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		shaderCI.stage = shaderModule.stage;
		shaderCI.module = shaderModule.shaderModule;
		shaderCI.pName = "main";
		shaderStages.push_back(shaderCI);
	}

	VkPipelineVertexInputStateCreateInfo vertexCI{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexCI.vertexBindingDescriptionCount = 1;
	vertexCI.pVertexBindingDescriptions = &bindingDescription;
	vertexCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexCI.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkGraphicsPipelineCreateInfo pipe{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipe.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipe.pStages = shaderStages.data();
	pipe.pVertexInputState = &vertexCI;
	pipe.pInputAssemblyState = &inputAssembly;
	pipe.pRasterizationState = &raster;
	pipe.pColorBlendState = &blend;
	pipe.pMultisampleState = &multisamplingCI;
	pipe.pViewportState = &viewportCI;
	pipe.pDepthStencilState = &depthStencil;
	pipe.pDynamicState = nullptr;

	pipe.renderPass = renderPass;
	pipe.layout = pipelineLayout;

	VkResult res = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipe, nullptr, &pipeline);
	assert(res == VK_SUCCESS);

	return pipeline;

}


VkPipeline initComputePipeline(VkDevice device, VkPipelineLayout pipelineLayout, ShaderModule& shader) {
	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkPipelineShaderStageCreateInfo shaderCI{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	shaderCI.module = shader.shaderModule;
	shaderCI.pName = "main";
	shaderCI.stage = shader.stage;
	VkComputePipelineCreateInfo computeCI{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	computeCI.stage = shaderCI;
	computeCI.layout = pipelineLayout;
	VkResult res = vkCreateComputePipelines(device, nullptr, 1, &computeCI, nullptr, &pipeline);
	assert(res == VK_SUCCESS);

	return pipeline;
}

void cleanupPipeline(VkDevice device, VkPipeline pipeline) {
	vkDestroyPipeline(device, pipeline, nullptr);
}

void cleanupPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout) {
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

void cleanupDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool) {
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}


void cleanupDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout) {
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}


void cleanupImage(VkDevice device, Image& image) {
	if (image.sampler != VK_NULL_HANDLE)
		vkDestroySampler(device, image.sampler, nullptr);
	if (image.imageView != VK_NULL_HANDLE)
		vkDestroyImageView(device, image.imageView, nullptr);
	if (image.memory != VK_NULL_HANDLE)
		vkFreeMemory(device, image.memory, nullptr);
	if (image.image != VK_NULL_HANDLE)
		vkDestroyImage(device, image.image, nullptr);
}

void cleanupBuffer(VkDevice device, Buffer& buffer) {
	vkFreeMemory(device, buffer.memory, nullptr);
	vkDestroyBuffer(device, buffer.buffer, nullptr);

}

void cleanupFence(VkDevice device, VkFence fence) {
	vkDestroyFence(device, fence, nullptr);
}
void cleanupShaderModule(VkDevice device, VkShaderModule shaderModule) {
	vkDestroyShaderModule(device, shaderModule, nullptr);
}

void cleanupFramebuffers(VkDevice device, std::vector<VkFramebuffer>& framebuffers) {
	for (auto& framebuffer : framebuffers) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}
}

void cleanupRenderPass(VkDevice device, VkRenderPass renderPass) {
	vkDestroyRenderPass(device, renderPass, nullptr);
}



void cleanupCommandBuffers(VkDevice device, std::vector<VkCommandPool>& commandPools, std::vector<VkCommandBuffer>& commandBuffers) {
	for (size_t i = 0; i < commandBuffers.size(); i++) {
		vkFreeCommandBuffers(device, commandPools[i], 1, &commandBuffers[i]);
	}
}

void cleanupCommandBuffer(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer) {
	std::vector<VkCommandPool> commandPools{ commandPool };
	std::vector<VkCommandBuffer> commandBuffers{ commandBuffer };
	cleanupCommandBuffers(device, commandPools, commandBuffers);
}



void cleanupCommandPools(VkDevice device, std::vector<VkCommandPool>& commandPools) {
	for (auto& commandPool : commandPools) {
		vkDestroyCommandPool(device, commandPool, nullptr);
	}
}

void cleanupCommandPool(VkDevice device, VkCommandPool commandPool) {
	std::vector<VkCommandPool> commandPools{ commandPool };
	cleanupCommandPools(device, commandPools);
}

void cleanupSemaphore(VkDevice device, VkSemaphore semaphore) {
	vkDestroySemaphore(device, semaphore, nullptr);
}
void cleanupSwapchain(VkDevice device, VkSwapchainKHR swapchain) {
	vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void cleanupDevice(VkDevice device) {
	vkDestroyDevice(device, nullptr);
}

void cleanupSurface(VkInstance instance, VkSurfaceKHR surface) {
	vkDestroySurfaceKHR(instance, surface, nullptr);
}

void cleanupInstance(VkInstance instance) {
	vkDestroyInstance(instance, nullptr);
}



void saveScreenCap(VkDevice device, VkCommandBuffer cmd, VkQueue queue, VkImage srcImage, VkPhysicalDeviceMemoryProperties& memoryProperties, VkFormatProperties& formatProperties, VkFormat colorFormat, VkExtent2D extent, uint32_t index) {
	//cribbed from Sascha Willems code.
	
	bool supportsBlit = (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) && (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);
	Image dstImage;
	VkImageCreateInfo imageCI{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.format = colorFormat;
	imageCI.extent = { extent.width,extent.height,1 };
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 1;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.tiling = VK_IMAGE_TILING_LINEAR;
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	VkResult res = vkCreateImage(device, &imageCI, nullptr, &dstImage.image);
	assert(res == VK_SUCCESS);

	VkMemoryRequirements memReqs{};
	vkGetImageMemoryRequirements(device, dstImage.image, &memReqs);
	VkMemoryAllocateInfo memAllocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, memoryProperties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	res = vkAllocateMemory(device, &memAllocInfo, nullptr, &dstImage.memory);
	assert(res == VK_SUCCESS);
	res = vkBindImageMemory(device, dstImage.image, dstImage.memory, 0);
	assert(res == VK_SUCCESS);





	transitionImage(device, queue, cmd, dstImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	transitionImage(device, queue, cmd, srcImage, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	res = vkBeginCommandBuffer(cmd, &beginInfo);
	assert(res == VK_SUCCESS);



	if (supportsBlit) {
		VkOffset3D blitSize;
		blitSize.x = extent.width;
		blitSize.y = extent.height;
		blitSize.z = 1;

		VkImageBlit imageBlitRegion{};
		imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.srcSubresource.layerCount = 1;
		imageBlitRegion.srcOffsets[1] = blitSize;
		imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.dstSubresource.layerCount = 1;
		imageBlitRegion.dstOffsets[1];

		vkCmdBlitImage(cmd, srcImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			dstImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageBlitRegion,
			VK_FILTER_NEAREST);
	}
	else {
		VkImageCopy imageCopyRegion{};

		imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.srcSubresource.layerCount = 1;
		imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.dstSubresource.layerCount = 1;
		imageCopyRegion.extent.width = extent.width;
		imageCopyRegion.extent.height = extent.height;
		imageCopyRegion.extent.depth = 1;

		vkCmdCopyImage(cmd,
			srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			dstImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageCopyRegion);
	}

	res = vkEndCommandBuffer(cmd);
	assert(res == VK_SUCCESS);

	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;

	VkFence fence = initFence(device);


	res = vkQueueSubmit(queue, 1, &submitInfo, fence);
	assert(res == VK_SUCCESS);

	res = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
	assert(res == VK_SUCCESS);


	vkDestroyFence(device, fence, nullptr);


	transitionImage(device, queue, cmd, dstImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

	transitionImage(device, queue, cmd, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	VkImageSubresource subResource{ VK_IMAGE_ASPECT_COLOR_BIT,0,0 };
	VkSubresourceLayout subResourceLayout;
	vkGetImageSubresourceLayout(device, dstImage.image, &subResource, &subResourceLayout);

	bool colorSwizzle = false;
	if (!supportsBlit)
	{
		std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
		colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), colorFormat) != formatsBGR.end());
	}

	uint8_t* data{ nullptr };
	vkMapMemory(device, dstImage.memory, 0, VK_WHOLE_SIZE, 0, (void**)&data);
	data += subResourceLayout.offset;

	std::string filename = std::to_string(index) + ".jpg";
	if (colorSwizzle) {
		uint32_t* ppixel = (uint32_t*)data;
		//must be a better way to do this
		for (uint32_t i = 0; i < extent.height; i++) {
			for (uint32_t j = 0; j < extent.width; j++) {

				uint32_t pix = ppixel[i * extent.width + j];
				uint8_t a = (pix & 0xFF000000) >> 24;
				uint8_t r = (pix & 0x00FF0000) >> 16;
				uint8_t g = (pix & 0x0000FF00) >> 8;
				uint8_t b = (pix & 0x000000FF);
				uint32_t newPix = (a << 24) | (b << 16) | (g << 8) | r;
				ppixel[i * extent.width + j] = newPix;

			}
		}
	}
	stbi_write_jpg(filename.c_str(), extent.width, extent.height, 4, data, 100);

	vkUnmapMemory(device, dstImage.memory);

	cleanupImage(device, dstImage);

	
}


void cleanupDrawable(VkDevice device, Drawable& drawable) {
	if (drawable.pipeline != VK_NULL_HANDLE)
		vkDestroyPipeline(device, drawable.pipeline, nullptr);
	if (drawable.pipelineLayout != VK_NULL_HANDLE)
		vkDestroyPipelineLayout(device, drawable.pipelineLayout, nullptr);

	if (drawable.descriptorPool != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(device, drawable.descriptorPool, nullptr);
	if (drawable.descriptorSetLayout != VK_NULL_HANDLE)
		vkDestroyDescriptorSetLayout(device, drawable.descriptorSetLayout, nullptr);


	cleanupBuffer(device, drawable.vertexBuffer);
	cleanupBuffer(device, drawable.indexBuffer);
	for (auto& tex : drawable.textures) {
		cleanupImage(device, tex.image);
	}

}

VkDeviceSize drawoffsets[1] = { 0 };



void DrawDrawable(VkCommandBuffer cmd, DrawInfo& drawInfo, Drawable& drawable) {
	drawInfo.pvkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, drawable.pipeline);
	drawInfo.pvkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, drawable.pipelineLayout, 0, 1, &drawable.descriptorSet, 0, 0);
	drawInfo.pvkCmdBindVertexBuffers(cmd, 0, 1, &drawable.vertexBuffer.buffer, drawoffsets);
	drawInfo.pvkCmdBindIndexBuffer(cmd, drawable.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
	drawInfo.pvkCmdDrawIndexed(cmd, drawable.indexCount, 1, 0, 0, 0);
}

void DrawDrawableInstanced(VkCommandBuffer cmd, DrawInfo& drawInfo, DrawableInstanced& drawable) {
	drawInfo.pvkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, drawable.pipeline);
	drawInfo.pvkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, drawable.pipelineLayout, 0, 1, &drawable.descriptorSet, 0, 0);
	drawInfo.pvkCmdBindVertexBuffers(cmd, 0, 1, &drawable.vertexBuffer.buffer, drawoffsets);
	drawInfo.pvkCmdBindIndexBuffer(cmd, drawable.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
	drawInfo.pvkCmdDrawIndexed(cmd, drawable.indexCount, drawable.instanceCount, 0, 0, 0);
}

void cleanupDrawableInstanced(VkDevice device, DrawableInstanced& drawable) {
	cleanupDrawable(device, drawable);
	cleanupBuffer(device, drawable.instanceBuffer);
}


void loadCubemapTextures(VulkanInfo& vulkanInfo, std::vector<const char*>textures, Texture& cubeMap) {
	int width = 0;
	int height = 0;
	std::vector<int> widthArray;
	std::vector<int> heightArray;
	std::vector<stbi_uc*> pixelArray;
	VkDeviceSize cubeSize = 0;
	for (auto& tex : textures) {
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(tex, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		width = texWidth;
		height = texHeight;
		assert(pixels);
		VkDeviceSize imageSize = (uint64_t)texWidth * (uint64_t)texHeight * 4;
		cubeSize += imageSize;
		widthArray.push_back(texWidth);
		heightArray.push_back(texHeight);
		pixelArray.push_back(pixels);
	}

	initCubemapTextureImage(vulkanInfo.device, PREFERRED_IMAGE_FORMAT, vulkanInfo.formatProperties, vulkanInfo.memoryProperties, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, (uint32_t)width, (uint32_t)height, cubeMap.image);
	transitionImage(vulkanInfo.device, vulkanInfo.queue, vulkanInfo.commandBuffer, cubeMap.image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cubeMap.image.mipLevels, 6);

	Buffer stagingBuffer;
	initBuffer(vulkanInfo.device, vulkanInfo.memoryProperties, cubeSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);
	void* ptr = mapBuffer(vulkanInfo.device, stagingBuffer);
	stbi_uc* pPix = (stbi_uc*)ptr;
	VkDeviceSize offset = 0;
	for (int i = 0; i < pixelArray.size(); i++) {
		int width = widthArray[i];
		int height = heightArray[i];
		VkDeviceSize imageSize = (uint64_t)width * (uint64_t)height * 4;

		memcpy(&pPix[offset], pixelArray[i], imageSize);
		offset += imageSize;
		stbi_image_free(pixelArray[i]);
	}
	offset = 0;
	for (int i = 0; i < pixelArray.size(); i++) {
		int width = widthArray[i];
		int height = heightArray[i];
		VkDeviceSize imageSize = (uint64_t)width * (uint64_t)height * 4;
		CopyBufferToImage(vulkanInfo.device, vulkanInfo.queue, vulkanInfo.commandBuffer, stagingBuffer, cubeMap.image, width, height, offset, (uint32_t)i);
		offset += imageSize;

	}
	transitionImage(vulkanInfo.device, vulkanInfo.queue, vulkanInfo.commandBuffer, cubeMap.image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cubeMap.image.mipLevels, 6);
	//generateMipMaps(vulkanInfo.device, vulkanInfo.queue, vulkanInfo.commandBuffer, cubeMap.image); //do we need mip maps? LOD is always the same

	cleanupBuffer(vulkanInfo.device, stagingBuffer);

}


