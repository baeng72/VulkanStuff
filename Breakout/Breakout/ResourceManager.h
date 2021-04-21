#pragma once
#include <map>
#include "Vulkan.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <tiny_obj_loader.h>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec2 uv;

	bool operator==(const Vertex& other)const {
		return pos == other.pos && norm == other.norm && uv == other.uv;
	}
	static VkVertexInputBindingDescription& getInputBindingDescription() {
		static VkVertexInputBindingDescription bindingDescription = { 0,sizeof(Vertex),VK_VERTEX_INPUT_RATE_VERTEX };
		return bindingDescription;
	}
	static std::vector<VkVertexInputAttributeDescription>& getInputAttributeDescription() {
		static std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {
			{0,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,pos)},
			{1,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,norm)},
			{2,0,VK_FORMAT_R32G32_SFLOAT,offsetof(Vertex,uv)},
		};
		return attributeDescriptions;
	}
};
namespace std {

	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex)const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.norm) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.uv) << 1);

		}
	};
}

struct TextureDesc {
	Image image;
	VkShaderStageFlags stage;
	VkDescriptorImageInfo imageInfo{};
};

struct BufferDesc {
	Buffer buffer;
	VkShaderStageFlags stage;
	void* ptr{ nullptr };
	VkDescriptorBufferInfo bufferInfo{};
};

struct Shader {
	ShaderModule vertShader;
	ShaderModule fragShader;
};

struct DrawObject {
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
	VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
	VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkPipeline pipeline{ VK_NULL_HANDLE };
	uint32_t instanceCount{ 0 };
	uint32_t indexCount{ 0 };
};

struct ResourceManager {
	VulkanInfo vulkanInfo;
	std::map<std::string, TextureDesc> Textures;
	std::map<std::string, BufferDesc> UniformBuffers;
	std::map<std::string, Shader> Shaders;
	std::map<std::string, Buffer> VertexBuffers;
	std::map<std::string, Buffer> IndexBuffers;
	std::map<std::string, uint32_t> IndexCounts;
	std::map<std::string, uint32_t> InstanceCounts;
	std::map<std::string, BufferDesc> StorageBuffers;
	std::map<std::string, DrawObject> DrawObjects;
};

void resourceManagerConstruct(ResourceManager& resourceManager, VulkanInfo& vulkanInfo);

void resourceManagerDestroy(ResourceManager& resourceManager);


void resourceManagerLoadTexture(ResourceManager& resourceManager, const char* path, const char* name, VkShaderStageFlags shaderStages, bool enableLod);

void resourceManagerLoadTextureFromMemory(ResourceManager& resourceManager, uint8_t* pImageData, VkFormat format, uint32_t imageWidth, uint32_t imageHeight, const char* name, VkShaderStageFlags shaderStages, bool enableLod);

void resourceManagerLoadObjFile(ResourceManager& resourceManager, const char* path, const char* name);

void resourceManagerLoadObjFromMemory(ResourceManager& resourceManager, float* vertices, uint32_t vertSize, uint32_t* indices, uint32_t indSize, const char* name);

void resourceManagerInitVertexBuffer(ResourceManager& resourceManager, Vertex* vertices, uint32_t vertCount, const char* name);

void resourceManagerInitIndexBuffer(ResourceManager& resourceManager, uint32_t* indices, uint32_t indexCount, const char* name);

void resourceManagerInitInstanceBuffer(ResourceManager& resourceManager, VkDeviceSize instanceSize, uint32_t instanceCount, VkShaderStageFlags stage, const char* name);

void resourceManagerInitUniformBuffer(ResourceManager& resourceManager, VkDeviceSize uniformSize, VkShaderStageFlags stage, const char* name);



void resourceManagerInitShaders(ResourceManager& resourceManager, const char* vertPath, const char* fragPath, const char* name);

std::vector<VkDescriptorSetLayoutBinding> resourceManagerGetLayoutSetBindings(ResourceManager& resourceManager, const char* name);

std::vector<VkDescriptorPoolSize> resourceManagerGetDescriptorPoolSizes(ResourceManager& resourceManager, const char* name);

std::vector<VkWriteDescriptorSet> resourceManagerGetDescriptorWrites(ResourceManager& resourceManager, VkDescriptorSet descriptorSet, const char* name);


std::vector<ShaderModule> resourceManagerGetShaderModules(ResourceManager& resourceManager, const char* name);

void* resourceManagerGetUniformPtr(ResourceManager& resourceManager, const char* name);

void* resourceManagerGetStoragePtr(ResourceManager& resourceManager, const char* name);

VkBuffer resourceManagerGetVertexBuffer(ResourceManager& resourceManager, const char* name);

VkBuffer resourceManagerGetIndexBuffer(ResourceManager& resourceManager, const char* name);

uint32_t resourceManagerGetIndexCount(ResourceManager& resourceManager, const char* name);

uint32_t resourceManagerGetInstanceCount(ResourceManager& resourceManager, const char* name);

struct ResourceObjectInfo {
	const char* diffuseTexture{ nullptr };	
	bool enableLod{ false };
	const char* objPath{ nullptr };		
	VkDeviceSize instanceSize{ 0 };
	uint32_t instanceCount{ 0 };
	VkDeviceSize uboSize{ 0 };
	const char* vertShaderPath{ nullptr };
	const char* fragShaderPath{ nullptr };
};





void resourceManagerInitObject(ResourceManager& resourceManager, const ResourceObjectInfo& info, VkRenderPass renderPass, VkExtent2D extent, VkSampleCountFlagBits maxSamples, VkBool32 blendEnable, const char* name);


DrawObject& resourceManagerGetDrawObject(ResourceManager& resourceManager, const char* name);

void resourceManagerDrawObject(ResourceManager& resourceManager, DrawInfo& drawInfo, VkCommandBuffer cmd, const char* name);


void resourceManagerDrawObject(ResourceManager& resourceManager, DrawInfo& drawInfo, VkCommandBuffer cmd, const char* name, uint32_t instanceCount);