#include "TextManager.h"

namespace std {

	/*template<> struct hash<glm::ivec2> {
		size_t operator()(glm::ivec2 const& vec)const {
			return (hash<glm::vec2>()(vec));

		}
	};*/

	bool operator<(const glm::ivec2& lhs, const glm::ivec2& rhs)
	{
		return lhs.x < rhs.x ||
			lhs.x == rhs.x && (lhs.y < rhs.y);
	}
}

struct TextUBOP {
	glm::mat4 projection;
	glm::vec3 color;
};



void textManagerInit(TextManager& textManager, ResourceManager& resourceManager, VkRenderPass renderPass, VkExtent2D extent, VkSampleCountFlagBits maxSamples, const char* fontPath, uint32_t width, uint32_t height, const char* name) {
	FT_Library ft;
	FT_Error res = FT_Init_FreeType(&ft);
	assert(res == 0);
	FT_Face face;
	res = FT_New_Face(ft, fontPath, 0, &face);
	assert(res == 0);
	FT_Set_Pixel_Sizes(face, 0, 48);
	unsigned int bmpWidth = 0;
	unsigned int bmpHeight = 0;
	std::vector<uint8_t> pixels;

	std::map<char, std::vector<uint8_t>> data;
	for (unsigned char c = 0; c < 128; c++)
	{

		res = FT_Load_Char(face, c, FT_LOAD_RENDER);
		assert(res == 0);


		bmpHeight = max(bmpHeight, face->glyph->bitmap.rows);

		unsigned int pitch = face->glyph->bitmap.pitch;
		TextManager::Character character = {
			glm::ivec2(face->glyph->bitmap.width,face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left,face->glyph->bitmap_top),
			bmpWidth,
			static_cast<unsigned int>(face->glyph->advance.x)
		};

		textManager.Characters.insert(std::pair<char, TextManager::Character>(c, character));
		if (face->glyph->bitmap.width > 0) {
			void* ptr = face->glyph->bitmap.buffer;

			std::vector<uint8_t> charData(face->glyph->bitmap.width * face->glyph->bitmap.rows);

			for (unsigned int i = 0; i < face->glyph->bitmap.rows; i++) {
				for (unsigned int j = 0; j < face->glyph->bitmap.width; j++) {
					uint8_t byte = face->glyph->bitmap.buffer[i * pitch + j];
					charData[i * pitch + j] = byte;
				}
			}
			data.insert(std::pair<char, std::vector<uint8_t>>(c, charData));
		}
		bmpWidth += face->glyph->bitmap.width;
	}
	textManager.invBmpWidth = 1 / (float)bmpWidth;
	//allocate texture
	uint8_t* buffer = new uint8_t[bmpHeight * bmpWidth];
	memset(buffer, 0, bmpHeight * bmpWidth);

	uint32_t xpos = 0;
	for (unsigned char c = 0; c < 128; c++)
	{


		TextManager::Character& character = textManager.Characters[c];

		std::vector<uint8_t> charData = data[c];
		uint32_t width = character.Size.x;
		uint32_t height = character.Size.y;
		for (uint32_t i = 0; i < height; i++) {
			for (uint32_t j = 0; j < width; j++) {
				uint8_t byte = charData[i * width + j];
				buffer[i * bmpWidth + xpos + j] = byte;
			}
		}
		xpos += width;
	}
	resourceManagerInitUniformBuffer(resourceManager, sizeof(TextUBOP), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, name);
	glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, -1.0f, 1.0f);
	projection[1][1] *= -1;
	TextUBOP* ptrUBO = reinterpret_cast<TextUBOP*>(resourceManagerGetUniformPtr(resourceManager, name));
	ptrUBO->projection = projection;
	ptrUBO->color = glm::vec3(1.0f);
	resourceManagerLoadTextureFromMemory(resourceManager, buffer, VK_FORMAT_R8_UNORM, bmpWidth, bmpHeight, name, VK_SHADER_STAGE_FRAGMENT_BIT, false);
	resourceManagerInitShaders(resourceManager, "Shaders/text.vert.spv", "Shaders/text.frag.spv", name);
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
	drawObject.pipeline = initGraphicsPipeline(resourceManager.vulkanInfo.device, renderPass, drawObject.pipelineLayout, extent, shaders, inputBindingDescription, inputAttributeDescription, VK_CULL_MODE_BACK_BIT, true, maxSamples, true);
	drawObject.instanceCount = resourceManagerGetInstanceCount(resourceManager, name);
	drawObject.indexCount = resourceManagerGetIndexCount(resourceManager, name);
	drawObject.instanceCount = 1;
	resourceManager.InstanceCounts[name] = 1;
	resourceManager.DrawObjects[name] = drawObject;
	delete[] buffer;
	textManager.pName = name;//store name, so we don't need to pass it in future

}

void textManagerUpdate(TextManager& textManager, ResourceManager& resourceManager, glm::vec2 pos, float scale, const char* pText) {
	//check if we're adding or removing
	if (textManager.Strings.find(pos) != textManager.Strings.end()) {
		//already exists, if pText is nullptr, we're removing
		if (pText == nullptr) {
			textManager.Strings.erase(pos);
			return;
		}
		//we're updating existing
		std::string text = pText;
		std::tuple<float,std::string> val = std::pair<float, std::string>(scale, text);
		textManager.Strings[pos] = val;
	}
	else {
		if (pText != nullptr) {
			std::tuple<float, std::string> val = std::pair<float, std::string>(scale, pText);
			textManager.Strings.insert(std::pair<glm::vec2, std::tuple<float, std::string>>(pos, val));
		}
	}
	//loop through all strings
	VkDeviceSize vertSize = 0;
	VkDeviceSize indSize = 0;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	uint32_t indexoffset = 0;
	for (auto& string : textManager.Strings) {
		std::tuple<float, std::string> val = string.second;
		float currScale = std::get<0>(val);
		const char* pText = std::get<1>(val).c_str();
		size_t len = strlen(pText);
		vertSize += len * sizeof(Vertex) * 4;//4 verts per quad
		indSize += len * sizeof(uint32_t) * 6;//6 indices per quad
		glm::vec2 pos = string.first;
		float x = pos.x;
		float y = pos.y;
		for (size_t i = 0; i < len; i++) {
			char c = pText[i];
			TextManager::Character& character = textManager.Characters[c];
			//build quad for character
			float xpos = x + character.Bearing.x * currScale;
			float ypos = y - (character.Size.y - character.Bearing.y) * currScale;

			float w = (float)character.Size.x * currScale;
			float h = (float)character.Size.y * currScale;
			float u0 = (float)character.offset * textManager.invBmpWidth;
			float u1 = (float)(character.offset + character.Size.x) * textManager.invBmpWidth;
			Vertex topleft = { {xpos,ypos,0.0f},{0.0f,0.0f,0.0f },{u0,1.0f} };
			Vertex topright = { { xpos + w,ypos,0.0f},{0.0f,0.0f,0.0f},{u1,1.0f} };
			Vertex bottomleft = { { xpos,ypos - h,0.0f},{0.0f,0.0f,0.0f},{u0,0.0f} };
			Vertex bottomright = { {xpos + w,ypos - h,0.0f},{0.0f,0.0f,0.0f},{u1,0.0f} };
			x += (character.Advance >> 6) * currScale;
			vertices.push_back(topleft);
			vertices.push_back(topright);
			vertices.push_back(bottomleft);
			vertices.push_back(bottomright);
			indices.push_back(indexoffset + 0);
			indices.push_back(indexoffset + 1);
			indices.push_back(indexoffset + 2);
			indices.push_back(indexoffset + 1);
			indices.push_back(indexoffset + 3);
			indices.push_back(indexoffset + 2);
			indexoffset += 4;
		}
	}
	//size_t len = strlen(pText);
	//VkDeviceSize vertSize = len * sizeof(Vertex)*4;//4 verts per quad
	//VkDeviceSize indSize = len * sizeof(uint32_t) * 6;// 6 indices per quad
	//std::vector<Vertex> vertices;
	//std::vector<uint32_t> indices;
	//unsigned int xoffset = 0;
	//unsigned int indexoffset = 0;
	//float x = pos.x;
	//float y = pos.y;
	//for (size_t i = 0; i < len; i++) {
	//	char c =  pText[i];
	//	TextManager::Character& character = textManager.Characters[c];
	//	//build quad for character
	//	float xpos = x + character.Bearing.x * scale;
	//	float ypos = y - (character.Size.y - character.Bearing.y) * scale;

	//	float w = character.Size.x * scale;
	//	float h = character.Size.y * scale;
	//	float u0 = (float)character.offset * textManager.invBmpWidth;
	//	float u1 = (float)(character.offset + w) * textManager.invBmpWidth;
	//	Vertex topleft = { {xpos,ypos,0.0f},{0.0f,0.0f,0.0f },{u0,1.0f} };
	//	Vertex topright = { { xpos+w,ypos,0.0f},{0.0f,0.0f,0.0f},{u1,1.0f} };
	//	Vertex bottomleft = { { xpos,ypos-h,0.0f},{0.0f,0.0f,0.0f},{u0,0.0f} };
	//	Vertex bottomright = { {xpos+w,ypos-h,0.0f},{0.0f,0.0f,0.0f},{u1,0.0f} };
	//	x += (character.Advance >> 6) * scale;
	//	vertices.push_back(topleft);
	//	vertices.push_back(topright);
	//	vertices.push_back(bottomleft);
	//	vertices.push_back(bottomright);
	//	indices.push_back(indexoffset+0);
	//	indices.push_back(indexoffset+1);
	//	indices.push_back(indexoffset+2);
	//	indices.push_back(indexoffset+1);
	//	indices.push_back(indexoffset+3);
	//	indices.push_back(indexoffset+2);
	//	indexoffset += 4;

	//}
	if(vertSize>0)
		resourceManagerInitVertexBuffer(resourceManager, vertices.data(), (uint32_t)vertices.size(), textManager.pName);
	if(indSize>0)
		resourceManagerInitIndexBuffer(resourceManager, indices.data(), (uint32_t)indices.size(), textManager.pName);
}

