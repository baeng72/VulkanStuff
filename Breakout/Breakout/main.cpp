#include "Vulkan.h"

#include "ResourceManager.h"
#include "Game.h"


const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 720;


float lastX = (float)SCREEN_WIDTH / 2.0f;
float lastY = (float)SCREEN_HEIGHT / 2.0f;
float deltaTime = 0.0f;//time between current and last frame
float lastFrame = 0.0f;


bool firstMouse = true;

bool spinCube = true;
bool spinCubePressed = false;





//struct GameObject : public Drawable{
//	glm::vec2 Position;
//	glm::vec2 Size;
//	glm::vec2 Velocity;
//	glm::vec3 Color;
//	float Rotation;
//	bool IsSolid;
//	bool Destroyed;	
//};
//
//void initGameObject(GameObject&gameObject,glm::vec2 pos, glm::vec2 size, glm::vec3 color = glm::vec3(1.0f), glm::vec2 velocity = glm::vec2(0.0f, 0.0f)) {
//	gameObject.Position = pos;
//	gameObject.Size = size;	
//	gameObject.Color = color;
//	gameObject.Velocity = velocity;
//	gameObject.IsSolid = gameObject.Destroyed = false;
//}
//
//
//struct GameLevel{
//	std::vector<GameObject> Bricks;//maybe separate Drawables & GameObject position stuff to work with cache
//};
//
//void initGameLevel(const char* file, unsigned int levelWidth, unsigned int levelHeight,GameLevel&level) {
//	level.Bricks.clear();
//	unsigned int tileCode;
//	std::string line;
//	std::ifstream fstream(file);
//	std::vector<std::vector<unsigned int>> tileData;
//	if (fstream) {
//		while (std::getline(fstream, line))//read each line from level file
//		{
//			std::istringstream sstream(line);
//			std::vector<unsigned int> row;
//			while (sstream >> tileCode)//read each word separated by spaces
//				row.push_back(tileCode);
//			tileData.push_back(row);
//		}
//		if (tileData.size() > 0) {
//			//calculate dimensions
//			size_t height = tileData.size();
//			size_t width = tileData[0].size();
//			float unit_width = levelWidth / static_cast<float>(width);
//			float unit_height = levelHeight / static_cast<float>(height);
//			//initialize level tiles based on tileData
//			for (unsigned int y = 0; y < height; ++y) {
//				for (unsigned int x = 0; x < width; ++x) {
//					//check block type from level data (2D level array)
//					if (tileData[x][y] == 1)//solid
//					{
//						glm::vec2 pos(unit_width * x, unit_height * y);
//						glm::vec2 size(unit_width, unit_height);
//						GameObject gameObject;
//						initGameObject(gameObject, pos, size, glm::vec3(0.8f, 0.8f, 0.7f));
//					}
//				}
//			}
//		}
//
//	}
//}
//
//
//
//


GLFWwindow* initWindow(uint32_t width, uint32_t height) {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
	assert(window != nullptr);
	
	return window;
}




void cleanupWindow(GLFWwindow* window) {
	glfwDestroyWindow(window);
	glfwTerminate();
}


void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	Game* pGame = reinterpret_cast<Game*>(glfwGetWindowUserPointer(window));
	// when a user presses the escape key, we set the WindowShouldClose property to true, closing the application
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			pGame->Keys[key] = true;
		else if (action == GLFW_RELEASE) {
			pGame->Keys[key] = false;
			pGame->KeysProcessed[key] = false;
		}

	}
}


int main() {

	GLFWwindow* window = initWindow(SCREEN_WIDTH, SCREEN_HEIGHT);
	
	

	uint32_t extCount = 0;
	auto ext = glfwGetRequiredInstanceExtensions(&extCount);
	std::vector<const char*> requiredExtensions(ext, ext + extCount);
#ifdef NDEBUG
	std::vector<const char*> requiredLayers{ "VK_LAYER_LUNARG_monitor" };
#else
	std::vector<const char*> requiredLayers{ "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };
#endif

	VkInstance instance = initInstance(requiredExtensions, requiredLayers);
	VkSurfaceKHR surface = initSurface(instance, window);

	Queues queues;
	VkPhysicalDevice physicalDevice = choosePhysicalDevice(instance, surface, queues);
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
	VkSurfaceCapabilitiesKHR surfaceCaps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps);
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data());
	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

	VkSampleCountFlagBits maxSamples = getMaxUsableSampleCount(deviceProperties);


	std::vector<const char*> deviceExtensions{ "VK_KHR_swapchain" };
	VkPhysicalDeviceFeatures enabledFeatures{};
	if (deviceFeatures.samplerAnisotropy)
		enabledFeatures.samplerAnisotropy = VK_TRUE;
	if (deviceFeatures.sampleRateShading)
		enabledFeatures.sampleRateShading = VK_TRUE;
	if (deviceFeatures.logicOp)
		enabledFeatures.logicOp = VK_TRUE;
	//ENABLE Geometry shader
	//if (deviceFeatures.geometryShader)
	//	enabledFeatures.geometryShader = VK_TRUE;
	VkDevice device = initDevice(physicalDevice, deviceExtensions, queues, enabledFeatures);

	VkQueue graphicsQueue = getDeviceQueue(device, queues.graphicsQueueFamily);
	VkQueue presentQueue = getDeviceQueue(device, queues.presentQueueFamily);
	VkQueue computeQueue = getDeviceQueue(device, queues.computeQueueFamily);

	VkPresentModeKHR presentMode = chooseSwapchainPresentMode(presentModes);
	VkSurfaceFormatKHR swapchainFormat = chooseSwapchainFormat(surfaceFormats);
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, swapchainFormat.format, &formatProperties);
	VkExtent2D swapchainExtent{};
	VkSwapchainKHR swapchain = initSwapchain(device, surface, SCREEN_WIDTH, SCREEN_HEIGHT, surfaceCaps, presentMode, swapchainFormat, swapchainExtent);
	std::vector<VkImage> swapchainImages;
	getSwapchainImages(device, swapchain, swapchainImages);
	std::vector<VkImageView> swapchainImageViews;
	initSwapchainImageViews(device, swapchainImages, swapchainFormat.format, swapchainImageViews);

	VkSemaphore presentComplete = initSemaphore(device);
	VkSemaphore renderComplete = initSemaphore(device);

	VkCommandPool commandPool = initCommandPool(device, queues.graphicsQueueFamily);
	VkCommandBuffer commandBuffer = initCommandBuffer(device, commandPool);

	std::vector<VkCommandPool> commandPools;
	initCommandPools(device, swapchainImages.size(), queues.graphicsQueueFamily, commandPools);

	std::vector<VkCommandBuffer> commandBuffers;
	initCommandBuffers(device, commandPools, commandBuffers);


	VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
#define SHADOW_WIDTH 1024
#define SHADOW_HEIGHT 1024
	//Image shadowImage;
	//initTextureImageNoMip(device, depthFormat, formatProperties, memoryProperties, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,  SHADOW_WIDTH, SHADOW_HEIGHT, shadowImage);
	//initDepthImage(device, depthFormat, formatProperties, memoryProperties, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, SHADOW_WIDTH, SHADOW_HEIGHT, shadowImage);
	//initSampler(device, shadowImage);

	//CubeMap for shadow map
	//initCubemapDepthImage(device, depthFormat, formatProperties, memoryProperties, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, SHADOW_WIDTH, SHADOW_HEIGHT, shadowImage);

	//VkRenderPass shadowRenderPass = initShadowRenderPass(device, depthFormat);
	//std::vector<VkFramebuffer> shadowFramebuffers;
	//initFramebuffers(device, shadowRenderPass, shadowImage.imageView, SHADOW_WIDTH, SHADOW_HEIGHT, 6, shadowFramebuffers);//cubemap has 6 layers?
	//VkFramebuffer shadowFramebuffer = shadowFramebuffers[0];





	VkRenderPass renderPass = initMSAARenderPass(device, swapchainFormat.format,  maxSamples);

	//Image depthImage;
	//initDepthImage(device, depthFormat, formatProperties, memoryProperties, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, maxSamples, WIDTH, HEIGHT, depthImage);

	Image msaaImage;
	initMSAAImage(device, swapchainFormat.format, formatProperties, memoryProperties, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, SCREEN_WIDTH, SCREEN_HEIGHT, maxSamples, msaaImage);

	std::vector<VkFramebuffer> framebuffers;
	//initMSAAFramebuffers(device, renderPass, swapchainImageViews, depthImage.imageView, msaaImage.imageView, WIDTH, HEIGHT, framebuffers);
	initMSAAFramebuffers(device, renderPass, swapchainImageViews,  msaaImage.imageView, SCREEN_WIDTH, SCREEN_HEIGHT, framebuffers);

	VulkanInfo vulkanInfo{ device,memoryProperties,formatProperties,graphicsQueue,commandBuffer };

	ResourceManager resourceManager;
	resourceManagerConstruct(resourceManager, vulkanInfo);
	Game game(resourceManager, renderPass, swapchainExtent, maxSamples, SCREEN_WIDTH, SCREEN_HEIGHT);
	game.Init();
	//key handler
	glfwSetWindowUserPointer(window, &game);//store game for callback
	//glfwSetFramebufferSizeCallback(window, frambebuffer_size_callback);
	glfwSetKeyCallback(window, keyCallback);


	//GameObject sprite;
	////load texture, probably largest piece of data
	//Texture faceTexture;
	//int texWidth, texHeight, texChannels;
	//stbi_uc* texPixels = stbi_load("Textures/awesomeface.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	//assert(texPixels);
	//VkDeviceSize faceTextureSize = (uint64_t)texWidth * (uint64_t)texHeight * 4;
	//initTextureImage(vulkanInfo.device, PREFERRED_IMAGE_FORMAT, vulkanInfo.formatProperties, vulkanInfo.memoryProperties, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, (uint32_t)texWidth, (uint32_t)texHeight, faceTexture.image);
	//transitionImage(vulkanInfo.device, vulkanInfo.queue, vulkanInfo.commandBuffer, faceTexture.image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, faceTexture.image.mipLevels);
	//sprite.textures.push_back(faceTexture);
	///*Texture redLeatherNormal;
	//stbi_uc* normalPixels = stbi_load("Textures/Red_leather_normal.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	//VkDeviceSize brickNormalSize = (uint64_t)texWidth * (uint64_t)texHeight * 4;
	//initTextureImage(vulkanInfo.device, PREFERRED_IMAGE_FORMAT, vulkanInfo.formatProperties, vulkanInfo.memoryProperties, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, (uint32_t)texWidth, (uint32_t)texHeight, redLeatherNormal.image);
	//transitionImage(vulkanInfo.device, vulkanInfo.queue, vulkanInfo.commandBuffer, redLeatherNormal.image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, redLeatherNormal.image.mipLevels);*/

	//float spriteVertices[] = {
	//	// pos      // tex
	//	0.0f, 1.0f, 0.0f, 1.0f,
	//	1.0f, 0.0f, 1.0f, 0.0f,
	//	0.0f, 0.0f, 0.0f, 0.0f,
	//	1.0f, 1.0f, 1.0f, 1.0f,
	//};
	//uint32_t spriteIndices[] = {
	//	0,1,2,0,3,1
	//};
	//VkDeviceSize spriteVertexSize = sizeof(spriteVertices);
	//VkDeviceSize spriteIndexSize = sizeof(spriteIndices);
	//sprite.indexCount = sizeof(spriteIndices) / sizeof(spriteIndices[0]);


	///*ObjModel::ObjModelData objModelData;
	//ObjModel::loadObjFile("Models/Monkey.obj", nullptr, objModelData, false);
	//VkDeviceSize cubeVertexSize = objModelData.vertices.size() * sizeof(ObjModel::Vertex);
	//VkDeviceSize cubeIndexSize = objModelData.indices.size() * sizeof(uint32_t);
	//uint32_t cubeIndexCount = (uint32_t)objModelData.indices.size();*/
	////Buffer spriteVertexBuffer;
	////Buffer spriteIndexBuffer;


	////use one staging buffer, work out max size needed
	//VkDeviceSize maxSize = std::max(faceTextureSize, std::max(spriteVertexSize, spriteIndexSize));

	//Buffer stagingBuffer;
	//initBuffer(device, memoryProperties, maxSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);
	//void* ptr = mapBuffer(device, stagingBuffer);
	////copy image data using staging buffer
	//memcpy(ptr, texPixels, faceTextureSize);
	//stbi_image_free(texPixels);
	//CopyBufferToImage(vulkanInfo.device, vulkanInfo.queue, vulkanInfo.commandBuffer, stagingBuffer, faceTexture.image, texWidth, texHeight);

	//generateMipMaps(vulkanInfo.device, vulkanInfo.queue, vulkanInfo.commandBuffer, faceTexture.image);

	///*memcpy(ptr, normalPixels, brickNormalSize);
	//stbi_image_free(normalPixels);
	//CopyBufferToImage(device, graphicsQueue, commandBuffer, stagingBuffer, redLeatherNormal.image, texWidth, texHeight);
	//generateMipMaps(device, graphicsQueue, commandBuffer, redLeatherNormal.image);*/

	////cube vertices & indices
	//initBuffer(device, memoryProperties, spriteVertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sprite.vertexBuffer);
	//memcpy(ptr, spriteVertices, spriteVertexSize);
	//CopyBufferTo(device, graphicsQueue, commandBuffer, stagingBuffer, sprite.vertexBuffer, spriteVertexSize);
	//initBuffer(device, memoryProperties, spriteIndexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sprite.indexBuffer);
	//memcpy(ptr, spriteIndices, spriteIndexSize);
	//CopyBufferTo(device, graphicsQueue, commandBuffer, stagingBuffer, sprite.indexBuffer, spriteIndexSize);



	//unmapBuffer(device, stagingBuffer);
	//cleanupBuffer(device, stagingBuffer);

	//struct UBOMVP {
	//	glm::mat4 model;
	//	
	//	glm::mat4 projection;
	//	alignas(16) glm::vec3 color;
	//}uboMVP;
	//uboMVP.model = glm::mat4(1.0f);
	////uboMVP.invmodel = glm::transpose(glm::inverse(uboMVP.model));

	//Buffer mvpUniformBuffer;
	//initBuffer(device, memoryProperties, sizeof(UBOMVP), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mvpUniformBuffer);
	//void* uboMVPPtr = mapBuffer(device, mvpUniformBuffer);

	///*struct UBOLIGHT {
	//	alignas(16) glm::vec3 lightPos;
	//	alignas(16) glm::vec3 viewPos;
	//}uboLight;




	//Buffer lightUniformBuffer;
	//initBuffer(device, memoryProperties, sizeof(UBOLIGHT), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, lightUniformBuffer);
	//void* uboLightPtr = mapBuffer(device, lightUniformBuffer);*/






	//VkVertexInputBindingDescription bindingDescription = { 0,sizeof(float)*4,VK_VERTEX_INPUT_RATE_VERTEX };
	//std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {
	//	{0,0,VK_FORMAT_R32G32_SFLOAT,0},
	//	{1,0,VK_FORMAT_R32G32_SFLOAT,2*sizeof(float)},
	//	/*{1,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(ObjModel::Vertex,norm)},
	//	{2,0,VK_FORMAT_R32G32_SFLOAT,offsetof(ObjModel::Vertex,uv)},*/
	//};
	//std::vector<VkDescriptorSetLayoutBinding> layoutSetBindings;
	//layoutSetBindings = { { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT , nullptr },
	//	{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT , nullptr },
	//	/*{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT , nullptr },
	//	{ 3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT , nullptr },*/
	//};

	//sprite.descriptorSetLayout = initDescriptorSetLayout(vulkanInfo.device, layoutSetBindings);
	//std::vector<VkDescriptorPoolSize> poolSizes;
	//poolSizes = { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,6 },
	//	{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,6} };
	//sprite.descriptorPool = initDescriptorPool(vulkanInfo.device, poolSizes, 7);
	//sprite.descriptorSet = initDescriptorSet(vulkanInfo.device, sprite.descriptorSetLayout, sprite.descriptorPool);
	//std::vector<VkWriteDescriptorSet> descriptorWrites;
	//VkDescriptorBufferInfo uboBufferMVP{};
	//uboBufferMVP.buffer = mvpUniformBuffer.buffer;
	//uboBufferMVP.range = sizeof(UBOMVP);
	//VkDescriptorImageInfo texImageInfo{};
	//texImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	//texImageInfo.imageView = faceTexture.image.imageView;
	//texImageInfo.sampler = faceTexture.image.sampler;
	///*VkDescriptorImageInfo normImageInfo{};
	//normImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	//normImageInfo.imageView = redLeatherNormal.image.imageView;
	//normImageInfo.sampler = redLeatherNormal.image.sampler;
	//VkDescriptorBufferInfo uboLightInfo{};
	//uboLightInfo.buffer = lightUniformBuffer.buffer;
	//uboLightInfo.range = sizeof(UBOLIGHT);*/
	//descriptorWrites = {
	//	{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,nullptr,sprite.descriptorSet,0,0,1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,nullptr,&uboBufferMVP,nullptr },
	//	{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,nullptr,sprite.descriptorSet,1,0,1,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,&texImageInfo,nullptr,nullptr },		
	//};
	//updateDescriptorSets(device, descriptorWrites);



	//sprite.pipelineLayout = initPipelineLayout(device, sprite.descriptorSetLayout);
	//VkShaderModule vertShader = initShaderModule(device, "Shaders/sprite.vert.spv");

	//VkShaderModule fragShader = initShaderModule(device, "Shaders/sprite.frag.spv");
	//std::vector<ShaderModule> shaders = { {vertShader,VK_SHADER_STAGE_VERTEX_BIT},{fragShader,VK_SHADER_STAGE_FRAGMENT_BIT} };

	//sprite.pipeline = initGraphicsPipeline(device, renderPass, sprite.pipelineLayout, swapchainExtent, shaders, bindingDescription, attributeDescriptions, VK_CULL_MODE_BACK_BIT, true, maxSamples);
	//cleanupShaderModule(device, fragShader);
	//cleanupShaderModule(device, vertShader);








	PFN_vkAcquireNextImageKHR pvkAcquireNextImage = (PFN_vkAcquireNextImageKHR)vkGetDeviceProcAddr(device, "vkAcquireNextImageKHR");
	assert(pvkAcquireNextImage);
	PFN_vkQueuePresentKHR pvkQueuePresent = (PFN_vkQueuePresentKHR)vkGetDeviceProcAddr(device, "vkQueuePresentKHR");
	assert(pvkQueuePresent);
	PFN_vkQueueSubmit pvkQueueSubmit = (PFN_vkQueueSubmit)vkGetDeviceProcAddr(device, "vkQueueSubmit");
	assert(pvkQueueSubmit);
	PFN_vkBeginCommandBuffer pvkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)vkGetDeviceProcAddr(device, "vkBeginCommandBuffer");
	assert(pvkBeginCommandBuffer);
	PFN_vkCmdBeginRenderPass pvkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)vkGetDeviceProcAddr(device, "vkCmdBeginRenderPass");
	assert(pvkCmdBeginRenderPass);
	PFN_vkCmdBindPipeline pvkCmdBindPipeline = (PFN_vkCmdBindPipeline)vkGetDeviceProcAddr(device, "vkCmdBindPipeline");
	assert(pvkCmdBindPipeline);
	PFN_vkCmdBindVertexBuffers pvkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)vkGetDeviceProcAddr(device, "vkCmdBindVertexBuffers");
	assert(pvkCmdBindVertexBuffers);
	PFN_vkCmdBindIndexBuffer pvkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)vkGetDeviceProcAddr(device, "vkCmdBindIndexBuffer");
	assert(pvkCmdBindIndexBuffer);
	PFN_vkCmdDrawIndexed pvkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)vkGetDeviceProcAddr(device, "vkCmdDrawIndexed");
	assert(pvkCmdDrawIndexed);
	PFN_vkCmdEndRenderPass pvkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)vkGetDeviceProcAddr(device, "vkCmdEndRenderPass");
	assert(pvkCmdEndRenderPass);
	PFN_vkEndCommandBuffer pvkEndCommandBuffer = (PFN_vkEndCommandBuffer)vkGetDeviceProcAddr(device, "vkEndCommandBuffer");
	assert(pvkEndCommandBuffer);
	PFN_vkQueueWaitIdle pvkQueueWaitIdle = (PFN_vkQueueWaitIdle)vkGetDeviceProcAddr(device, "vkQueueWaitIdle");
	assert(pvkQueueWaitIdle);
	PFN_vkCmdBindDescriptorSets pvkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)vkGetDeviceProcAddr(device, "vkCmdBindDescriptorSets");
	assert(pvkCmdBindDescriptorSets);
	PFN_vkCmdDispatch pvkCmdDispatch = (PFN_vkCmdDispatch)vkGetDeviceProcAddr(device, "vkCmdDispatch");
	assert(pvkCmdDispatch);
	PFN_vkCmdDraw pvkCmdDraw = (PFN_vkCmdDraw)vkGetDeviceProcAddr(device, "vkCmdDraw");
	assert(pvkCmdDraw);

	//main loop
	uint32_t index = 0;
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VkCommandBuffer cmd{ VK_NULL_HANDLE };
	VkClearValue clearValues[2] = { {0.0f, 0.0f, 0.0f, 0.0f},{1.0f,0.0f } };
	//VkClearValue shadowClearValues[1] = { {1.0f,0.0f} };
	//VkRenderPassBeginInfo shadowRenderPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	//shadowRenderPassBeginInfo.renderPass = shadowRenderPass;
	//shadowRenderPassBeginInfo.framebuffer = shadowFramebuffer;
	//shadowRenderPassBeginInfo.renderArea = { 0,0,SHADOW_WIDTH,SHADOW_HEIGHT };
	//shadowRenderPassBeginInfo.pClearValues = shadowClearValues;
	//shadowRenderPassBeginInfo.clearValueCount = 1;
	VkRenderPassBeginInfo renderPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.renderArea = { 0,0,SCREEN_WIDTH,SCREEN_HEIGHT };
	renderPassBeginInfo.clearValueCount = sizeof(clearValues) / sizeof(clearValues[0]);
	renderPassBeginInfo.pClearValues = clearValues;
	VkDeviceSize offsets[1] = { 0 };
	VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo		submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.pWaitDstStageMask = &submitPipelineStages;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &presentComplete;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &renderComplete;
	submitInfo.commandBufferCount = 1;

	VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &index;
	presentInfo.pWaitSemaphores = &renderComplete;
	presentInfo.waitSemaphoreCount = 1;
	VkResult res;
	uint32_t frameCount = 0;

	/*glm::vec2 spritePosition = glm::vec2(200.0f, 200.0f);
	glm::vec2 spriteSize = glm::vec2(300.0f, 400.0f);
	glm::vec3 spriteColor = glm::vec3(0.0f, 1.0f, 0.0f);

	
	initGameObject(sprite, spritePosition, spriteSize, spriteColor);*/

	/*std::vector<VkFence> fences(framebuffers.size());
	for (uint32_t i = 0; i < framebuffers.size(); i++) {
		fences[i] = initFence(device,VK_FENCE_CREATE_SIGNALED_BIT);
	}*/


	DrawInfo drawInfo;
	drawInfo.pvkCmdBindDescriptorSets = pvkCmdBindDescriptorSets;
	drawInfo.pvkCmdBindPipeline = pvkCmdBindPipeline;
	drawInfo.pvkCmdBindVertexBuffers = pvkCmdBindVertexBuffers;
	drawInfo.pvkCmdBindIndexBuffer = pvkCmdBindIndexBuffer;
	drawInfo.pvkCmdDrawIndexed = pvkCmdDrawIndexed;

	//int key;
	glm::vec3 lightPos(0.0f, 1.0f, 3.0f);
	while (!glfwWindowShouldClose(window)) {
		float time = (float)glfwGetTime();
		//processInput(window);

		if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
			/*std::thread t1(saveScreenCap, device, graphicsQueue, swapchainImages[index], std::ref(memoryProperties), std::ref(formatProperties), swapchainFormat.format, swapchainExtent, frameCount);
			t1.detach();*/
			saveScreenCap(device, commandBuffers[index], graphicsQueue, swapchainImages[index], memoryProperties, formatProperties, swapchainFormat.format, swapchainExtent, frameCount);
		}

		/*key = glfwGetKey(window, GLFW_KEY_SPACE);

		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spinCubePressed)
		{
			spinCube = !spinCube;
			spinCubePressed = true;
		}
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
		{
			spinCubePressed = false;
		}*/

		glfwPollEvents();
		float currentFrame = time;
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		game.ProcessInput(deltaTime);


		/*lightPos.z = sin(currentFrame * 0.5f) * 6.0f;
		uboLight.viewPos = camera.Position;
		uboLight.lightPos = lightPos;
		memcpy(uboLightPtr, &uboLight, sizeof(UBOLIGHT));*/

		//glm::mat4 view = camera.GetViewMatrix();
		//glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
		//glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(WIDTH),0.0f,(float)HEIGHT, 0.0f, 1.0f);
		////projection[1][1] *= -1;//fix for gl to vulkan
		//glm::mat4 model = glm::mat4(1.0f);		
		//model = glm::translate(model, glm::vec3(spritePosition, 0.0f));//first translate (transformations are scale first, then rotation, then final translation
		//model = glm::translate(model, glm::vec3(0.5f * spriteSize.x, 0.5f * spriteSize.y, 0.0f));//move origin of rotation to center of quad
		//model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		//model = glm::translate(model, glm::vec3(-0.5f * spriteSize.x, -0.5f * spriteSize.y, 0.0f));//move origin back
		//model = glm::scale(model, glm::vec3(spriteSize, 1.0f));//last scale
		////if (spinCube)
		////	model = glm::rotate(model, glm::radians(15.0f) * time, glm::vec3(0, 1, 0));
		//uboMVP.projection = projection;
		////uboMVP.view = view;
		//uboMVP.model = model;
		//uboMVP.color = spriteColor;
		////uboMVP.invmodel = glm::transpose(glm::inverse(model));
		//memcpy(uboMVPPtr, &uboMVP, sizeof(UBOMVP));




		game.Update(deltaTime);




		res = pvkAcquireNextImage(device, swapchain, UINT64_MAX, presentComplete, nullptr, &index);
		assert(res == VK_SUCCESS);

		//vkWaitForFences(device, 1, &fences[index], VK_TRUE, UINT64_MAX);
		//vkResetFences(device, 1, &fences[index]);

		cmd = commandBuffers[index];


		pvkBeginCommandBuffer(cmd, &beginInfo);

		//pvkCmdBeginRenderPass(cmd, &shadowRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		////draw to screen here



		//////draw cubes
		////draw cube map
		//pvkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, cubeMapShadowPipeline);
		//pvkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, cubeMapShadowPipelineLayout, 0, 1, &cubeMapShadowDescriptorSet, 0, 0);
		//pvkCmdBindVertexBuffers(cmd, 0, 1, &cubeVertexBuffer.buffer, offsets);
		//pvkCmdBindIndexBuffer(cmd, cubeIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
		//pvkCmdDrawIndexed(cmd, cubeIndexCount, 1, 0, 0, 0);

		//pvkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, objectShadowPipeline);
		//pvkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, objectShadowPipelineLayout, 0, 1, &objectShadowDescriptorSet, 0, 0);
		//pvkCmdBindVertexBuffers(cmd, 0, 1, &cubeVertexBuffer.buffer, offsets);
		//pvkCmdBindIndexBuffer(cmd, cubeIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
		//pvkCmdDrawIndexed(cmd, cubeIndexCount, NUM_CUBES, 0, 0, 0);


		//pvkCmdEndRenderPass(cmd);

		renderPassBeginInfo.framebuffer = framebuffers[index];
		pvkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


		////draw cubes
		//DrawDrawable(cmd, drawInfo, sprite);
		game.Render(drawInfo, cmd);
		/*pvkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, spritePipeline);
		pvkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, spritePipelineLayout, 0, 1, &spriteDescriptorSet, 0, 0);
		pvkCmdBindVertexBuffers(cmd, 0, 1, &spriteVertexBuffer.buffer, offsets);
		pvkCmdBindIndexBuffer(cmd, spriteIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
		pvkCmdDrawIndexed(cmd, spriteIndexCount, 1, 0, 0, 0);*/



		pvkCmdEndRenderPass(cmd);


		res = pvkEndCommandBuffer(cmd);
		assert(res == VK_SUCCESS);

		submitInfo.pCommandBuffers = &cmd;
		res = pvkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		assert(res == VK_SUCCESS);

		res = pvkQueuePresent(presentQueue, &presentInfo);
		assert(res == VK_SUCCESS);
		pvkQueueWaitIdle(presentQueue);
		frameCount++;
	}
	//vkWaitForFences(device, fences.size(), fences.data(), VK_TRUE, UINT64_MAX);
	vkDeviceWaitIdle(device);

	game.Clear();
	/*for (uint32_t i = 0; i < framebuffers.size(); i++) {
		cleanupFence(device, fences[i]);
	}*/

	/*cleanupPipeline(device, spritePipeline);
	cleanupPipelineLayout(device, spritePipelineLayout);
	cleanupDescriptorPool(device, spriteDescriptorPool);
	cleanupDescriptorSetLayout(device, spriteDescriptorSetLayout);*/






	cleanupFramebuffers(device, framebuffers);
	cleanupImage(device, msaaImage);
	//cleanupImage(device, depthImage);
	cleanupRenderPass(device, renderPass);

	


	cleanupCommandBuffers(device, commandPools, commandBuffers);
	cleanupCommandPools(device, commandPools);

	cleanupCommandBuffer(device, commandPool, commandBuffer);
	cleanupCommandPool(device, commandPool);
	cleanupSemaphore(device, renderComplete);
	cleanupSemaphore(device, presentComplete);

	cleanupSwapchainImageViews(device, swapchainImageViews);
	cleanupSwapchain(device, swapchain);
	cleanupDevice(device);
	cleanupSurface(instance, surface);
	cleanupInstance(instance);
	cleanupWindow(window);

	return EXIT_SUCCESS;
}