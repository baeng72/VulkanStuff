#pragma once
#include <string>
#include <map>
#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "ResourceManager.h"


struct TextManager {
	struct Character {
		glm::ivec2 Size;
		glm::ivec2 Bearing;
		unsigned int offset;
		unsigned int Advance;
	};
	const char* pName;
	std::map<char, Character> Characters;
	std::map<glm::vec2, std::tuple<float, std::string>> Strings;
	float invBmpWidth;


};

void textManagerInit(TextManager& textManager, ResourceManager& resourceManager, VkRenderPass renderPass, VkExtent2D extent, VkSampleCountFlagBits samples, const char* fontPath, uint32_t width, uint32_t height, const char* name);
void textManagerUpdate(TextManager& textManager, ResourceManager& resourceManager, glm::vec2 pos, float scale, const char* pText);
void textManagerCleanup(TextManager& textManager);



