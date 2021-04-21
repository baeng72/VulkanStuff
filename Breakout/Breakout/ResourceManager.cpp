#include "ResourceManager.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

void resourceManagerConstruct(ResourceManager& resourceManager, VulkanInfo& vulkanInfo) {
	resourceManager.vulkanInfo = vulkanInfo;
}

void resourceManagerDestroy(ResourceManager& resourceManager) {
	for (auto& iter : resourceManager.Textures) {
		cleanupImage(resourceManager.vulkanInfo.device, iter.second.image);
	}
	for (auto& iter : resourceManager.UniformBuffers) {
		BufferDesc& bufferDesc = iter.second;
		if (bufferDesc.ptr != nullptr)
			unmapBuffer(resourceManager.vulkanInfo.device, bufferDesc.buffer);
		cleanupBuffer(resourceManager.vulkanInfo.device, bufferDesc.buffer);
	}
	for (auto& iter : resourceManager.StorageBuffers) {
		BufferDesc& bufferDesc = iter.second;
		if (bufferDesc.ptr != nullptr)
			unmapBuffer(resourceManager.vulkanInfo.device, bufferDesc.buffer);
		cleanupBuffer(resourceManager.vulkanInfo.device, bufferDesc.buffer);
	}
	for (auto& iter : resourceManager.Shaders) {
		cleanupShaderModule(resourceManager.vulkanInfo.device, iter.second.vertShader.shaderModule);
		cleanupShaderModule(resourceManager.vulkanInfo.device, iter.second.fragShader.shaderModule);
	}
	for (auto& iter : resourceManager.VertexBuffers) {
		cleanupBuffer(resourceManager.vulkanInfo.device, iter.second);
	}
	for (auto& iter : resourceManager.IndexBuffers) {
		cleanupBuffer(resourceManager.vulkanInfo.device, iter.second);
	}
	for (auto& iter : resourceManager.DrawObjects) {
		DrawObject& draw = iter.second;
		cleanupPipeline(resourceManager.vulkanInfo.device, draw.pipeline);
		cleanupPipelineLayout(resourceManager.vulkanInfo.device, draw.pipelineLayout);
		cleanupDescriptorPool(resourceManager.vulkanInfo.device, draw.descriptorPool);
		cleanupDescriptorSetLayout(resourceManager.vulkanInfo.device, draw.descriptorSetLayout);

	}
}

void resourceManagerLoadTexture(ResourceManager& resourceManager, const char* path, const char* name, VkShaderStageFlags shaderStages, bool enableLod) {
	Image image;
	int texWidth, texHeight, texChannels;
	stbi_uc* texPixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	assert(texPixels);
	VkDeviceSize imageSize = (uint64_t)texWidth * (uint64_t)texHeight * 4;
	enableLod ? initTextureImage(resourceManager.vulkanInfo.device, PREFERRED_IMAGE_FORMAT, resourceManager.vulkanInfo.formatProperties, resourceManager.vulkanInfo.memoryProperties, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, (uint32_t)texWidth, (uint32_t)texHeight, image) :
		initTextureImageNoMip(resourceManager.vulkanInfo.device, PREFERRED_IMAGE_FORMAT, resourceManager.vulkanInfo.formatProperties, resourceManager.vulkanInfo.memoryProperties, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, (uint32_t)texWidth, (uint32_t)texHeight, image);
	transitionImage(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.queue, resourceManager.vulkanInfo.commandBuffer, image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image.mipLevels);

	VkDeviceSize maxSize = imageSize;

	Buffer stagingBuffer;
	initBuffer(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.memoryProperties, maxSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);
	void* ptr = mapBuffer(resourceManager.vulkanInfo.device, stagingBuffer);
	//copy image data using staging buffer
	memcpy(ptr, texPixels, imageSize);
	stbi_image_free(texPixels);
	CopyBufferToImage(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.queue, resourceManager.vulkanInfo.commandBuffer, stagingBuffer, image, texWidth, texHeight);

	//even if lod not enabled, need to transition, so use this code.
	generateMipMaps(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.queue, resourceManager.vulkanInfo.commandBuffer, image);

	unmapBuffer(resourceManager.vulkanInfo.device, stagingBuffer);
	cleanupBuffer(resourceManager.vulkanInfo.device, stagingBuffer);

	TextureDesc textureDesc;
	textureDesc.image = image;
	textureDesc.stage = shaderStages;
	textureDesc.imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	textureDesc.imageInfo.imageView = image.imageView;
	textureDesc.imageInfo.sampler = image.sampler;
	resourceManager.Textures.insert(std::pair<std::string, TextureDesc>(name, textureDesc));

}

void resourceManagerLoadTextureFromMemory(ResourceManager& resourceManager, uint8_t*pImageData,VkFormat format,uint32_t imageWidth,uint32_t imageHeight, const char* name, VkShaderStageFlags shaderStages, bool enableLod) {
	Image image;
	VkDeviceSize pixsize = 4;
	if (format == VK_FORMAT_R8_UNORM)
		pixsize = 1;
	else if (format == VK_FORMAT_R16_UNORM)
		pixsize = 2;
	
	VkDeviceSize imageSize = (uint64_t)imageWidth * (uint64_t)imageHeight * pixsize;
	enableLod ? initTextureImage(resourceManager.vulkanInfo.device, format, resourceManager.vulkanInfo.formatProperties, resourceManager.vulkanInfo.memoryProperties, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, (uint32_t)imageWidth, (uint32_t)imageHeight, image) :
		initTextureImageNoMip(resourceManager.vulkanInfo.device, format, resourceManager.vulkanInfo.formatProperties, resourceManager.vulkanInfo.memoryProperties, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, (uint32_t)imageWidth, (uint32_t)imageHeight, image);
	transitionImage(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.queue, resourceManager.vulkanInfo.commandBuffer, image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image.mipLevels);

	VkDeviceSize maxSize = imageSize;

	Buffer stagingBuffer;
	initBuffer(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.memoryProperties, maxSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);
	void* ptr = mapBuffer(resourceManager.vulkanInfo.device, stagingBuffer);
	//copy image data using staging buffer
	memcpy(ptr, pImageData, imageSize);	
	CopyBufferToImage(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.queue, resourceManager.vulkanInfo.commandBuffer, stagingBuffer, image, imageWidth, imageHeight);

	//even if lod not enabled, need to transition, so use this code.
	generateMipMaps(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.queue, resourceManager.vulkanInfo.commandBuffer, image);

	unmapBuffer(resourceManager.vulkanInfo.device, stagingBuffer);
	cleanupBuffer(resourceManager.vulkanInfo.device, stagingBuffer);

	TextureDesc textureDesc;
	textureDesc.image = image;
	textureDesc.stage = shaderStages;
	textureDesc.imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	textureDesc.imageInfo.imageView = image.imageView;
	textureDesc.imageInfo.sampler = image.sampler;
	resourceManager.Textures.insert(std::pair<std::string, TextureDesc>(name, textureDesc));

}

void resourceManagerLoadObjFromMemory(ResourceManager& resourceManager, float* vertices, uint32_t vertSize, uint32_t* indices, uint32_t indSize, const char* name) {
	VkDeviceSize vertexSize = vertSize;
	VkDeviceSize indexSize = indSize;
	Buffer vertexBuffer;
	Buffer indexBuffer;
	initBuffer(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.memoryProperties, vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer);
	initBuffer(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.memoryProperties, indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer);
	VkDeviceSize maxSize = std::max(vertexSize, indexSize);
	Buffer stagingBuffer;
	initBuffer(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.memoryProperties, maxSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);
	void* ptr = mapBuffer(resourceManager.vulkanInfo.device, stagingBuffer);
	//copy vertex data
	memcpy(ptr, vertices, vertexSize);
	CopyBufferTo(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.queue, resourceManager.vulkanInfo.commandBuffer, stagingBuffer, vertexBuffer, vertexSize);
	memcpy(ptr, indices, indexSize);
	CopyBufferTo(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.queue, resourceManager.vulkanInfo.commandBuffer, stagingBuffer, indexBuffer, indexSize);


	unmapBuffer(resourceManager.vulkanInfo.device, stagingBuffer);
	cleanupBuffer(resourceManager.vulkanInfo.device, stagingBuffer);

	resourceManager.VertexBuffers[name] = vertexBuffer;
	resourceManager.IndexBuffers[name] = indexBuffer;
	resourceManager.IndexCounts[name] = (uint32_t)indSize;
}

void resourceManagerLoadObjFile(ResourceManager& resourceManager, const char* path, const char* name) {
	std::vector<Vertex> vertices;					//object vertices
	std::vector<uint32_t> indices;
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	bool res = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path, "Models");
	if (!res) {
		std::cerr << warn << " " << err << std::endl;
	}
	assert(res);
	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.norm = {
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2]
			};

			vertex.uv = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};


			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}
			indices.push_back(uniqueVertices[vertex]);
		}
	}
	VkDeviceSize vertexSize = sizeof(Vertex) * vertices.size();
	VkDeviceSize indexSize = sizeof(uint32_t) * indices.size();
	Buffer vertexBuffer;
	Buffer indexBuffer;
	initBuffer(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.memoryProperties, vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer);
	initBuffer(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.memoryProperties, indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer);
	VkDeviceSize maxSize = std::max(vertexSize, indexSize);
	Buffer stagingBuffer;
	initBuffer(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.memoryProperties, maxSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);
	void* ptr = mapBuffer(resourceManager.vulkanInfo.device, stagingBuffer);
	//copy vertex data
	memcpy(ptr, vertices.data(), vertexSize);
	CopyBufferTo(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.queue, resourceManager.vulkanInfo.commandBuffer, stagingBuffer, vertexBuffer, vertexSize);
	memcpy(ptr, indices.data(), indexSize);
	CopyBufferTo(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.queue, resourceManager.vulkanInfo.commandBuffer, stagingBuffer, indexBuffer, indexSize);


	unmapBuffer(resourceManager.vulkanInfo.device, stagingBuffer);
	cleanupBuffer(resourceManager.vulkanInfo.device, stagingBuffer);

	resourceManager.VertexBuffers[name] = vertexBuffer;
	resourceManager.IndexBuffers[name] = indexBuffer;
	resourceManager.IndexCounts[name] = (uint32_t)indices.size();

}

void resourceManagerInitVertexBuffer(ResourceManager& resourceManager, Vertex* vertices, uint32_t vertCount, const char* name) {
	VkDeviceSize vertexSize = sizeof(Vertex) * vertCount;
	Buffer vertexBuffer;
	initBuffer(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.memoryProperties, vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer);
	Buffer stagingBuffer;
	initBuffer(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.memoryProperties, vertexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);
	void* ptr = mapBuffer(resourceManager.vulkanInfo.device, stagingBuffer);
	//copy vertex data
	memcpy(ptr, vertices, vertexSize);
	CopyBufferTo(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.queue, resourceManager.vulkanInfo.commandBuffer, stagingBuffer, vertexBuffer, vertexSize);
	unmapBuffer(resourceManager.vulkanInfo.device, stagingBuffer);
	cleanupBuffer(resourceManager.vulkanInfo.device, stagingBuffer);
	if (resourceManager.VertexBuffers.find(name) != resourceManager.VertexBuffers.end()) {
		Buffer oldBuffer = resourceManager.VertexBuffers[name];
		cleanupBuffer(resourceManager.vulkanInfo.device, oldBuffer);
	}

	resourceManager.VertexBuffers[name] = vertexBuffer;
}

void resourceManagerInitIndexBuffer(ResourceManager& resourceManager, uint32_t* indices, uint32_t indexCount, const char* name) {
	VkDeviceSize indexSize = sizeof(uint32_t) * indexCount;
	Buffer indexBuffer;
	initBuffer(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.memoryProperties, indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer);
	Buffer stagingBuffer;
	initBuffer(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.memoryProperties, indexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);
	void* ptr = mapBuffer(resourceManager.vulkanInfo.device, stagingBuffer);
	memcpy(ptr, indices, indexSize);
	CopyBufferTo(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.queue, resourceManager.vulkanInfo.commandBuffer, stagingBuffer, indexBuffer, indexSize);


	unmapBuffer(resourceManager.vulkanInfo.device, stagingBuffer);
	cleanupBuffer(resourceManager.vulkanInfo.device, stagingBuffer);
	if (resourceManager.IndexBuffers.find(name) != resourceManager.IndexBuffers.end()) {
		Buffer oldBuffer = resourceManager.IndexBuffers[name];
		cleanupBuffer(resourceManager.vulkanInfo.device, oldBuffer);
	}
	resourceManager.IndexBuffers[name] = indexBuffer;
	resourceManager.IndexCounts[name] = indexCount;
}

void resourceManagerInitInstanceBuffer(ResourceManager& resourceManager, VkDeviceSize instanceSize, uint32_t instanceCount, VkShaderStageFlags stage, const char* name) {
	Buffer instanceBuffer;
	VkDeviceSize instanceBufferSize = instanceSize * instanceCount;
	initBuffer(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.memoryProperties, instanceBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, instanceBuffer);
	void* ptr = mapBuffer(resourceManager.vulkanInfo.device, instanceBuffer);
	BufferDesc bufferDesc;
	bufferDesc.buffer = instanceBuffer;
	bufferDesc.stage = stage;
	bufferDesc.bufferInfo.buffer = instanceBuffer.buffer;
	bufferDesc.bufferInfo.range = instanceBufferSize;
	bufferDesc.ptr = ptr;
	resourceManager.StorageBuffers[name] = bufferDesc;
	resourceManager.InstanceCounts[name] = instanceCount;
}

void resourceManagerInitUniformBuffer(ResourceManager& resourceManager, VkDeviceSize uniformSize, VkShaderStageFlags stage, const char* name) {
	Buffer uniformBuffer;
	VkDeviceSize instanceBufferSize = uniformSize;
	initBuffer(resourceManager.vulkanInfo.device, resourceManager.vulkanInfo.memoryProperties, instanceBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer);
	void* ptr = mapBuffer(resourceManager.vulkanInfo.device, uniformBuffer);
	BufferDesc bufferDesc;
	bufferDesc.buffer = uniformBuffer;
	bufferDesc.stage = stage;
	bufferDesc.bufferInfo.buffer = uniformBuffer.buffer;
	bufferDesc.bufferInfo.range = instanceBufferSize;
	bufferDesc.ptr = ptr;
	resourceManager.UniformBuffers[name] = bufferDesc;
}


void resourceManagerInitShaders(ResourceManager& resourceManager, const char* vertPath, const char* fragPath, const char* name) {
	Shader shader;
	shader.vertShader.shaderModule = initShaderModule(resourceManager.vulkanInfo.device, vertPath);
	shader.vertShader.stage = VK_SHADER_STAGE_VERTEX_BIT;
	shader.fragShader.shaderModule = initShaderModule(resourceManager.vulkanInfo.device, fragPath);
	shader.fragShader.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	resourceManager.Shaders[name] = shader;
}

std::vector<VkDescriptorSetLayoutBinding> resourceManagerGetLayoutSetBindings(ResourceManager& resourceManager, const char* name) {
	uint32_t binding = 0;
	std::vector<VkDescriptorSetLayoutBinding> layoutSetBindings;
	for (auto& iter : resourceManager.UniformBuffers) {
		if (iter.first == name) {
			BufferDesc& bufferDesc = iter.second;
			layoutSetBindings.push_back({ binding++,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,bufferDesc.stage,nullptr });
		}
	}
	for (auto& iter : resourceManager.StorageBuffers) {
		if (iter.first == name) {
			BufferDesc& bufferDesc = iter.second;
			layoutSetBindings.push_back({ binding++,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,bufferDesc.stage,nullptr });
		}
	}
	for (auto& iter : resourceManager.Textures) {
		if (iter.first == name) {
			TextureDesc& textureDesc = iter.second;
			layoutSetBindings.push_back({ binding++, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1, textureDesc.stage, nullptr });
		}
	}
	return layoutSetBindings;
}


std::vector<VkDescriptorPoolSize> resourceManagerGetDescriptorPoolSizes(ResourceManager& resourceManager, const char* name) {
	std::vector<VkDescriptorPoolSize> poolSizes;
	bool firstUniform = true;
	for (auto& iter : resourceManager.UniformBuffers) {
		if (iter.first == name) {

			if (firstUniform == true) {
				poolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1 });
				firstUniform = false;
			}
			else {
				poolSizes[0].descriptorCount++;
			}
		}
	}
	bool firstStorage = true;
	for (auto& iter : resourceManager.StorageBuffers) {
		if (iter.first == name) {
			if (firstStorage) {
				poolSizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1 });
				firstStorage = false;
			}
			else {
				poolSizes[poolSizes.size() - 1].descriptorCount++;
			}
		}
	}
	bool firstTexture = true;
	for (auto& iter : resourceManager.Textures) {
		if (iter.first == name) {
			if (firstTexture) {
				poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1 });
			}
			else {
				poolSizes[poolSizes.size() - 1].descriptorCount++;
			}
		}
	}
	for (auto& iter : poolSizes) {
		iter.descriptorCount *= 3;
	}

	return poolSizes;
}

std::vector<VkWriteDescriptorSet> resourceManagerGetDescriptorWrites(ResourceManager& resourceManager, VkDescriptorSet descriptorSet, const char* name) {
	uint32_t binding = 0;
	std::vector<VkWriteDescriptorSet> descriptorWrites;


	for (auto& iter : resourceManager.UniformBuffers) {
		if (iter.first == name) {
			BufferDesc& bufferDesc = iter.second;
			descriptorWrites.push_back({ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,nullptr,descriptorSet,binding++,0,1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,nullptr,&bufferDesc.bufferInfo,nullptr });
		}
	}
	for (auto& iter : resourceManager.StorageBuffers) {
		if (iter.first == name) {
			BufferDesc& bufferDesc = iter.second;
			descriptorWrites.push_back({ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,nullptr,descriptorSet,binding++,0,1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,nullptr,&bufferDesc.bufferInfo,nullptr });
		}
	}
	for (auto& iter : resourceManager.Textures) {
		if (iter.first == name) {
			TextureDesc& textureDesc = iter.second;
			descriptorWrites.push_back({ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,nullptr,descriptorSet,binding++,0,1,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,&textureDesc.imageInfo,nullptr,nullptr });
		}
	}

	return descriptorWrites;
}

std::vector<ShaderModule> resourceManagerGetShaderModules(ResourceManager& resourceManager, const char* name) {
	std::vector<ShaderModule> shaders;
	for (auto& iter : resourceManager.Shaders) {
		if (iter.first == name) {
			shaders.push_back(iter.second.vertShader);
			shaders.push_back(iter.second.fragShader);
		}
	}
	return shaders;
}

void* resourceManagerGetUniformPtr(ResourceManager& resourceManager, const char* name) {
	for (auto& iter : resourceManager.UniformBuffers) {
		if (iter.first == name) {
			return iter.second.ptr;
		}
	}
	return nullptr;
}

void* resourceManagerGetStoragePtr(ResourceManager& resourceManager, const char* name) {
	for (auto& iter : resourceManager.StorageBuffers) {
		if (iter.first == name) {
			return iter.second.ptr;
		}
	}
	return nullptr;
}

VkBuffer resourceManagerGetVertexBuffer(ResourceManager& resourceManager, const char* name) {
	for (auto& iter : resourceManager.VertexBuffers) {
		if (iter.first == name) {
			return iter.second.buffer;
		}
	}
	return VK_NULL_HANDLE;
}

VkBuffer resourceManagerGetIndexBuffer(ResourceManager& resourceManager, const char* name) {
	for (auto& iter : resourceManager.IndexBuffers) {
		if (iter.first == name) {
			return iter.second.buffer;
		}
	}
	return VK_NULL_HANDLE;
}

uint32_t resourceManagerGetIndexCount(ResourceManager& resourceManager, const char* name) {
	for (auto& iter : resourceManager.IndexCounts) {
		if (iter.first == name) {
			return iter.second;
		}
	}
	return 0;
}

uint32_t resourceManagerGetInstanceCount(ResourceManager& resourceManager, const char* name) {
	for (auto& iter : resourceManager.InstanceCounts) {
		if (iter.first == name) {
			return iter.second;
		}
	}
	return 0;
}

void resourceManagerInitObject(ResourceManager& resourceManager, const ResourceObjectInfo& info, VkRenderPass renderPass, VkExtent2D extent, VkSampleCountFlagBits maxSamples, VkBool32 blendEnable, const char* name) {
	if (info.diffuseTexture != nullptr)
		resourceManagerLoadTexture(resourceManager, info.diffuseTexture, name, VK_SHADER_STAGE_FRAGMENT_BIT, info.enableLod);
	if (info.objPath != nullptr)
		resourceManagerLoadObjFile(resourceManager, info.objPath, name);
	if (info.instanceSize > 0 && info.instanceCount > 0)
		resourceManagerInitInstanceBuffer(resourceManager, info.instanceSize, info.instanceCount, VK_SHADER_STAGE_VERTEX_BIT, name);
	if (info.uboSize > 0)
		resourceManagerInitUniformBuffer(resourceManager, info.uboSize, VK_SHADER_STAGE_VERTEX_BIT, name);
	if (info.vertShaderPath != nullptr || info.fragShaderPath != nullptr)
		resourceManagerInitShaders(resourceManager, info.vertShaderPath, info.fragShaderPath, name);

	std::vector< VkDescriptorSetLayoutBinding> bindings = resourceManagerGetLayoutSetBindings(resourceManager, name);
	DrawObject drawObject;
	drawObject.descriptorSetLayout = initDescriptorSetLayout(resourceManager.vulkanInfo.device, bindings);

	std::vector<VkDescriptorPoolSize> poolSizes = resourceManagerGetDescriptorPoolSizes(resourceManager, name);
	drawObject.descriptorPool = initDescriptorPool(resourceManager.vulkanInfo.device, poolSizes, poolSizes[0].descriptorCount + 1);
	drawObject.descriptorSet = initDescriptorSet(resourceManager.vulkanInfo.device, drawObject.descriptorSetLayout, drawObject.descriptorPool);
	std::vector<VkWriteDescriptorSet> descriptorWrites = resourceManagerGetDescriptorWrites(resourceManager, drawObject.descriptorSet, name);
	updateDescriptorSets(resourceManager.vulkanInfo.device, descriptorWrites);
	drawObject.pipelineLayout = initPipelineLayout(resourceManager.vulkanInfo.device, drawObject.descriptorSetLayout);
	std::vector<ShaderModule> shaders = resourceManagerGetShaderModules(resourceManager, name);
	auto& inputBindingDescription = Vertex::getInputBindingDescription();
	auto& inputAttributeDescription = Vertex::getInputAttributeDescription();
	drawObject.pipeline = initGraphicsPipeline(resourceManager.vulkanInfo.device, renderPass, drawObject.pipelineLayout, extent, shaders, inputBindingDescription, inputAttributeDescription, VK_CULL_MODE_BACK_BIT, true, maxSamples, blendEnable);
	drawObject.instanceCount = resourceManagerGetInstanceCount(resourceManager, name);
	drawObject.indexCount = resourceManagerGetIndexCount(resourceManager, name);
	resourceManager.DrawObjects[name] = drawObject;

}

DrawObject& resourceManagerGetDrawObject(ResourceManager& resourceManager, const char* name) {
	return resourceManager.DrawObjects[name];
}

VkDeviceSize resoffsets[1] = { 0 };
void resourceManagerDrawObject(ResourceManager& resourceManager, DrawInfo& drawInfo, VkCommandBuffer cmd, const char* name) {
	DrawObject& drawObj = resourceManager.DrawObjects[name];
	VkBuffer vertexBuffer = resourceManagerGetVertexBuffer(resourceManager, name);
	VkBuffer indexBuffer = resourceManagerGetIndexBuffer(resourceManager, name);
	uint32_t indexCount = resourceManagerGetIndexCount(resourceManager, name);
	uint32_t instanceCount = resourceManagerGetInstanceCount(resourceManager, name);
	drawInfo.pvkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, drawObj.pipeline);
	drawInfo.pvkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, drawObj.pipelineLayout, 0, 1, &drawObj.descriptorSet, 0, 0);
	drawInfo.pvkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, resoffsets);
	drawInfo.pvkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	drawInfo.pvkCmdDrawIndexed(cmd, indexCount, instanceCount, 0, 0, 0);
}


void resourceManagerDrawObject(ResourceManager& resourceManager, DrawInfo& drawInfo, VkCommandBuffer cmd, const char* name, uint32_t instanceCount) {
	DrawObject& drawObj = resourceManager.DrawObjects[name];
	VkBuffer vertexBuffer = resourceManagerGetVertexBuffer(resourceManager, name);
	VkBuffer indexBuffer = resourceManagerGetIndexBuffer(resourceManager, name);
	uint32_t indexCount = resourceManagerGetIndexCount(resourceManager, name);
	//uint32_t instanceCount = resourceManagerGetInstanceCount(resourceManager, name);
	drawInfo.pvkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, drawObj.pipeline);
	drawInfo.pvkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, drawObj.pipelineLayout, 0, 1, &drawObj.descriptorSet, 0, 0);
	drawInfo.pvkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, resoffsets);
	drawInfo.pvkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	drawInfo.pvkCmdDrawIndexed(cmd, indexCount, instanceCount, 0, 0, 0);
}