#pragma once
#include <vector>
#include <stdexcept>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <cassert>
#include "GameObject.h"
/// GameLevel holds all Tiles as part of a Breakout level and 
/// hosts functionality to Load/render levels from the harddisk.
class GameLevel
{
public:
    // level state
    std::vector<GameObject> Bricks;
    ResourceManager& resourceManager;
    unsigned int solidCount;
    unsigned int nonSolidCount;
    // constructor
    GameLevel(ResourceManager resourceM):resourceManager(resourceM) { }
    // loads level from file
    void Load(const char* file, unsigned int levelWidth, unsigned int levelHeight,const char*solidName,const char*nonSolidName) {
        // clear old data
        this->Bricks.clear();
        // load from file
        unsigned int tileCode;
        
        std::string line;
        std::ifstream fstream(file);
        std::vector<std::vector<unsigned int>> tileData;
        if (fstream)
        {
            while (std::getline(fstream, line)) // read each line from level file
            {
                std::istringstream sstream(line);
                std::vector<unsigned int> row;
                while (sstream >> tileCode) // read each word separated by spaces
                    row.push_back(tileCode);
                tileData.push_back(row);
            }
            if (tileData.size() > 0)
                this->init(tileData, levelWidth, levelHeight,solidName,nonSolidName);
        }
    }
    // render level
    void Draw(DrawInfo&drawInfo,VkCommandBuffer cmd,const char*solidName,const char*nonSolidName) {
        resourceManagerDrawObject(resourceManager, drawInfo, cmd, solidName);
        resourceManagerDrawObject(resourceManager, drawInfo, cmd, nonSolidName);
        
        //for (GameObject& tile : this->Bricks)
        //    if (!tile.Destroyed) {
        //        resourceManagerDrawObject(resourceManager, drawInfo, cmd, tile.obj);
        //        //tile.Draw(renderer);
        //    }
    }
    // check if the level is completed (all non-solid tiles are destroyed)
    bool IsCompleted() {
        for (GameObject& tile : this->Bricks)
            if (!tile.IsSolid && !tile.Destroyed)
                return false;
        return true;
    }
private:
    // initialize level from tile data
    void init(std::vector<std::vector<unsigned int>> tileData, unsigned int levelWidth, unsigned int levelHeight,const char*solidName,const char*nonSolidName) {
        // calculate dimensions
        unsigned int height = (unsigned int)tileData.size();
        unsigned int width = (unsigned int)tileData[0].size(); // note we can index vector at [0] since this function is only called if height > 0
        float unit_width = (float)(levelWidth / width), unit_height = (float)(levelHeight / height);
        // initialize level tiles based on tileData		
        for (unsigned int y = 0; y < height; ++y)
        {
            for (unsigned int x = 0; x < width; ++x)
            {
                // check block type from level data (2D level array)
                if (tileData[y][x] == 1) // solid
                {
                    glm::vec2 pos(unit_width * x, unit_height * y);
                    glm::vec2 size(unit_width, unit_height);
                    GameObject obj(pos, size, glm::vec3(0.8f, 0.8f, 0.7f));
                    obj.IsSolid = true;
                    obj.obj = solidName;
                    this->Bricks.push_back(obj);
                    solidCount++;
                }
                else if (tileData[y][x] > 1)	// non-solid; now determine its color based on level data
                {
                    glm::vec3 color = glm::vec3(1.0f); // original: white
                    if (tileData[y][x] == 2)
                        color = glm::vec3(0.2f, 0.6f, 1.0f);
                    else if (tileData[y][x] == 3)
                        color = glm::vec3(0.0f, 0.7f, 0.0f);
                    else if (tileData[y][x] == 4)
                        color = glm::vec3(0.8f, 0.8f, 0.4f);
                    else if (tileData[y][x] == 5)
                        color = glm::vec3(1.0f, 0.5f, 0.0f);

                    glm::vec2 pos(unit_width * x, unit_height * y);
                    glm::vec2 size(unit_width, unit_height);
                    this->Bricks.push_back(GameObject(pos, size, color, glm::vec2(0.0f, 0.0f),nonSolidName));
                    nonSolidCount++;
                }
            }
        }
    }
    
};
