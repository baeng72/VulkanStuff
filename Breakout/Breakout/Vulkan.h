
#pragma once
#include <stdexcept>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cstdint>
#include <memory>
#include <algorithm>
#include <cstdio>
#include <cassert>
#include <unordered_map>
#include <thread>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>


#include <stb_image_write.h>


#include <stb_image.h>

//#define TINYOBJLOADER_IMPLEMENTATION
//#include <tiny_obj_loader.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

//_UNORM more like GL settings, a bit darker
#define PREFERRED_FORMAT VK_FORMAT_B8G8R8A8_UNORM
#define PREFERRED_IMAGE_FORMAT VK_FORMAT_R8G8B8A8_UNORM
//#define PREFERRED_FORMAT VK_FORMAT_B8G8R8A8_SRGB
//#define PREFERRED_IMAGE_FORMAT VK_FORMAT_R8G8B8A8_SRGB




VkInstance initInstance(std::vector<const char*>& requiredExtensions, std::vector<const char*>& requiredLayers);

VkSurfaceKHR initSurface(VkInstance instance, GLFWwindow* window);

struct Queues {
	uint32_t graphicsQueueFamily;
	uint32_t presentQueueFamily;
	uint32_t computeQueueFamily;
};

VkPhysicalDevice choosePhysicalDevice(VkInstance instance, VkSurfaceKHR surface, Queues& queues);

VkDevice initDevice(VkPhysicalDevice physicalDevice, std::vector<const char*> deviceExtensions, Queues queues, VkPhysicalDeviceFeatures enabledFeatures);

VkQueue getDeviceQueue(VkDevice device, uint32_t queueFamily);

VkPresentModeKHR chooseSwapchainPresentMode(std::vector<VkPresentModeKHR>& presentModes);

VkSurfaceFormatKHR chooseSwapchainFormat(std::vector<VkSurfaceFormatKHR>& formats);

VkSurfaceTransformFlagsKHR chooseSwapchainTransform(VkSurfaceCapabilitiesKHR& surfaceCaps);

VkCompositeAlphaFlagBitsKHR chooseSwapchainComposite(VkSurfaceCapabilitiesKHR& surfaceCaps);

VkSwapchainKHR initSwapchain(VkDevice device, VkSurfaceKHR surface, uint32_t width, uint32_t height, VkSurfaceCapabilitiesKHR& surfaceCaps, VkPresentModeKHR& presentMode, VkSurfaceFormatKHR& swapchainFormat, VkExtent2D& swapchainExtent);

void getSwapchainImages(VkDevice device, VkSwapchainKHR swapchain, std::vector<VkImage>& images);

void initSwapchainImageViews(VkDevice device, std::vector<VkImage>& swapchainImages, VkFormat& swapchainFormat, std::vector<VkImageView>& swapchainImageViews);

void cleanupSwapchainImageViews(VkDevice device, std::vector<VkImageView>& imageViews);

VkSemaphore initSemaphore(VkDevice device);

VkCommandPool initCommandPool(VkDevice device, uint32_t queueFamily); 

void initCommandPools(VkDevice device, size_t size, uint32_t queueFamily, std::vector<VkCommandPool>& commandPools);

VkCommandBuffer initCommandBuffer(VkDevice device, VkCommandPool commandPool);

void initCommandBuffers(VkDevice device, std::vector<VkCommandPool>& commandPools, std::vector<VkCommandBuffer>& commandBuffers);

VkRenderPass initRenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat);

VkRenderPass initMSAARenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits numSamples);

VkRenderPass initMSAARenderPass(VkDevice device, VkFormat colorFormat, VkSampleCountFlagBits numSamples);

VkRenderPass initOffScreenRenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat);

VkRenderPass initShadowRenderPass(VkDevice device, VkFormat depthFormat);

void initFramebuffers(VkDevice device, VkRenderPass renderPass, std::vector<VkImageView>& colorAttachments, VkImageView depthImageView, uint32_t width, uint32_t height, std::vector<VkFramebuffer>& framebuffers);

void initFramebuffers(VkDevice device, VkRenderPass renderPass, VkImageView depthImageView, uint32_t width, uint32_t height, std::vector<VkFramebuffer>& framebuffers);

void initFramebuffers(VkDevice device, VkRenderPass renderPass, VkImageView depthImageView, uint32_t width, uint32_t height, uint32_t layers, std::vector<VkFramebuffer>& framebuffers);

void initMSAAFramebuffers(VkDevice device, VkRenderPass renderPass, std::vector<VkImageView>& colorAttachments, VkImageView depthImageView, VkImageView resolveAttachment, uint32_t width, uint32_t height, std::vector<VkFramebuffer>& framebuffers);

void initMSAAFramebuffers(VkDevice device, VkRenderPass renderPass, std::vector<VkImageView>& colorAttachments, VkImageView resolveAttachment, uint32_t width, uint32_t height, std::vector<VkFramebuffer>& framebuffers);

struct ShaderModule {
	VkShaderModule shaderModule;
	VkShaderStageFlagBits stage;
};

VkShaderModule initShaderModule(VkDevice device, const char* filename);

struct Buffer {
	VkBuffer	buffer{ VK_NULL_HANDLE };
	VkDeviceMemory memory{ VK_NULL_HANDLE };
	VkDeviceSize size{ 0 };

};

uint32_t findMemoryType(uint32_t typeFilter, VkPhysicalDeviceMemoryProperties memoryProperties, VkMemoryPropertyFlags properties);

//MSAA support
VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDeviceProperties deviceProperties);

VkFence initFence(VkDevice device, VkFenceCreateFlags flags = 0);


void initBuffer(VkDevice device, VkPhysicalDeviceMemoryProperties& memoryProperties, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, Buffer& buffer);

void* mapBuffer(VkDevice device, Buffer& buffer);

void unmapBuffer(VkDevice device, Buffer& buffer);

void CopyBufferTo(VkDevice device, VkQueue queue, VkCommandBuffer cmd, Buffer& src, Buffer& dst, VkDeviceSize size);

struct Image {
	VkImage	image{ VK_NULL_HANDLE };
	VkDeviceMemory memory{ VK_NULL_HANDLE };
	VkSampler sampler{ VK_NULL_HANDLE };
	VkImageView imageView{ VK_NULL_HANDLE };
	uint32_t width;
	uint32_t height;
	uint32_t mipLevels;
};



void initImage(VkDevice device, VkFormat format, VkFormatProperties& formatProperties, VkPhysicalDeviceMemoryProperties& memoryProperties, VkMemoryPropertyFlags memoryPropertyFlags, uint32_t width, uint32_t height, Image& image);

void initSampler(VkDevice device, Image& image);

void initMSAAImage(VkDevice device, VkFormat format, VkFormatProperties& formatProperties, VkPhysicalDeviceMemoryProperties& memoryProperties, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags, uint32_t width, uint32_t height, VkSampleCountFlagBits numSamples, Image& image);

void initColorAttachmentImage(VkDevice device, VkFormat format, VkFormatProperties& formatProperties, VkPhysicalDeviceMemoryProperties& memoryProperties, VkMemoryPropertyFlags memoryPropertyFlags, uint32_t width, uint32_t height, Image& image);

void initDepthImage(VkDevice device, VkFormat format, VkFormatProperties& formatProperties, VkPhysicalDeviceMemoryProperties& memoryProperties, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags, uint32_t width, uint32_t height, Image& image);

void initDepthImage(VkDevice device, VkFormat format, VkFormatProperties& formatProperties, VkPhysicalDeviceMemoryProperties& memoryProperties, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags, VkSampleCountFlagBits numSamples, uint32_t width, uint32_t height, Image& image);

void initTextureImage(VkDevice device, VkFormat format, VkFormatProperties& formatProperties, VkPhysicalDeviceMemoryProperties& memoryProperties, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags, uint32_t width, uint32_t height, Image& image);

void initTextureImageNoMip(VkDevice device, VkFormat format, VkFormatProperties& formatProperties, VkPhysicalDeviceMemoryProperties& memoryProperties, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags, uint32_t width, uint32_t height, Image& image);

void initCubemapTextureImage(VkDevice device, VkFormat format, VkFormatProperties& formatProperties, VkPhysicalDeviceMemoryProperties& memoryProperties, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags, uint32_t width, uint32_t height, Image& image);

void initCubemapDepthImage(VkDevice device, VkFormat format, VkFormatProperties& formatProperties, VkPhysicalDeviceMemoryProperties& memoryProperties, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags, uint32_t width, uint32_t height, Image& image);

void generateMipMaps(VkDevice device, VkQueue queue, VkCommandBuffer cmd, Image& image);

void CopyBufferToImage(VkDevice device, VkQueue queue, VkCommandBuffer cmd, Buffer& src, Image& dst, uint32_t width, uint32_t height, VkDeviceSize offset = 0, uint32_t arrayLayer = 0);

void transitionImage(VkDevice device, VkQueue queue, VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1, uint32_t layerCount = 1);

VkDescriptorSetLayout initDescriptorSetLayout(VkDevice device, std::vector<VkDescriptorSetLayoutBinding>& descriptorBindings);

VkDescriptorPool initDescriptorPool(VkDevice device, std::vector<VkDescriptorPoolSize>& descriptorPoolSizes, uint32_t maxSets);

VkDescriptorSet initDescriptorSet(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool);

void updateDescriptorSets(VkDevice device, std::vector<VkWriteDescriptorSet> descriptorWrites);

VkPipelineLayout initPipelineLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout);

VkPipeline initGraphicsPipeline(VkDevice device, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkExtent2D extent, std::vector<ShaderModule>& shaders, VkVertexInputBindingDescription& bindingDescription, std::vector<VkVertexInputAttributeDescription>& attributeDescriptions, VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT, bool depthTest = true, VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT, VkBool32 blendEnable = VK_FALSE);



VkPipeline initDepthOnlyGraphicsPipeline(VkDevice device, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkExtent2D extent, std::vector<ShaderModule>& shaders, VkVertexInputBindingDescription& bindingDescription, std::vector<VkVertexInputAttributeDescription>& attributeDescriptions, VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT);
	


VkPipeline initComputePipeline(VkDevice device, VkPipelineLayout pipelineLayout, ShaderModule& shader);

void cleanupPipeline(VkDevice device, VkPipeline pipeline);

void cleanupPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout);


void cleanupDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool);


void cleanupDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout);

void cleanupImage(VkDevice device, Image& image);

void cleanupBuffer(VkDevice device, Buffer& buffer);

void cleanupFence(VkDevice device, VkFence fence);

void cleanupShaderModule(VkDevice device, VkShaderModule shaderModule);

void cleanupFramebuffers(VkDevice device, std::vector<VkFramebuffer>& framebuffers);

void cleanupRenderPass(VkDevice device, VkRenderPass renderPass);


void cleanupCommandBuffers(VkDevice device, std::vector<VkCommandPool>& commandPools, std::vector<VkCommandBuffer>& commandBuffers);

void cleanupCommandBuffer(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer);



void cleanupCommandPools(VkDevice device, std::vector<VkCommandPool>& commandPools);

void cleanupCommandPool(VkDevice device, VkCommandPool commandPool);

void cleanupSemaphore(VkDevice device, VkSemaphore semaphore);

void cleanupSwapchain(VkDevice device, VkSwapchainKHR swapchain);

void cleanupDevice(VkDevice device);

void cleanupSurface(VkInstance instance, VkSurfaceKHR surface);

void cleanupInstance(VkInstance instance);



void saveScreenCap(VkDevice device, VkCommandBuffer cmd, VkQueue queue, VkImage srcImage, VkPhysicalDeviceMemoryProperties& memoryProperties, VkFormatProperties& formatProperties, VkFormat colorFormat, VkExtent2D extent, uint32_t index);


struct Texture {
	Image image;
	std::string type;
	std::string path;
	std::string name;
};

struct Uniform {
	Buffer buffer;
	VkShaderStageFlags stage;
};

struct Storage {
	Buffer buffer;
	VkShaderStageFlags stage;
};

struct Drawable {
	Buffer vertexBuffer;
	Buffer indexBuffer;

	std::vector<Texture> textures;
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
	VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
	VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkPipeline pipeline{ VK_NULL_HANDLE };
	uint32_t indexCount{ 0 };

};

struct DrawableInstanced : Drawable {
	Buffer instanceBuffer;
	uint32_t instanceCount{ 0 };
};

void cleanupDrawable(VkDevice device, Drawable& drawable);



struct VulkanInfo {
	VkDevice device;
	VkPhysicalDeviceMemoryProperties memoryProperties;
	VkFormatProperties formatProperties;
	VkQueue queue;
	VkCommandBuffer commandBuffer;
};

struct DrawInfo {
	PFN_vkCmdBindPipeline pvkCmdBindPipeline{ nullptr };
	PFN_vkCmdBindDescriptorSets pvkCmdBindDescriptorSets{ nullptr };
	PFN_vkCmdBindVertexBuffers pvkCmdBindVertexBuffers{ nullptr };
	PFN_vkCmdBindIndexBuffer pvkCmdBindIndexBuffer{ nullptr };
	PFN_vkCmdDrawIndexed pvkCmdDrawIndexed{ nullptr };
};

void DrawDrawable(VkCommandBuffer cmd, DrawInfo& drawInfo, Drawable& drawable);

void DrawDrawableInstanced(VkCommandBuffer cmd, DrawInfo& drawInfo, DrawableInstanced& drawable);

void cleanupDrawableInstanced(VkDevice device, DrawableInstanced& drawable);


void loadCubemapTextures(VulkanInfo& vulkanInfo, std::vector<const char*>textures, Texture& cubeMap);
