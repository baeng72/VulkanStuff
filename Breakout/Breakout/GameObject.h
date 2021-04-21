#pragma once
#include <glm/glm.hpp>
#include "ResourceManager.h"

struct GameObject
{

    // object state
    glm::vec2   Position, Size, Velocity;
    glm::vec3   Color;
    float       Rotation;
    bool        IsSolid;
    bool        Destroyed;
    const char* obj;
    // render state
    
    // constructor(s)
    GameObject()
        : Position(0.0f, 0.0f), Size(1.0f, 1.0f), Velocity(0.0f), Color(1.0f), Rotation(0.0f), IsSolid(false), Destroyed(false),obj(nullptr) { }
    GameObject(glm::vec2 pos, glm::vec2 size, glm::vec3 color = glm::vec3(1.0f), glm::vec2 velocity = glm::vec2(0.0f, 0.0f),const char*object=nullptr)
        : Position(pos), Size(size), Velocity(velocity), Color(color), Rotation(0.0f),  IsSolid(false), Destroyed(false),obj(object) { }    
};

struct BallObject : public GameObject {
    //ball state
    float Radius;
    bool Stuck;
    BallObject():GameObject(),Radius(12.5f),Stuck(true) {

    }
    BallObject(glm::vec2 pos, float radius, glm::vec2 velocity,const char*object):GameObject(pos,glm::vec2(radius*2.0f,radius*2.0f),glm::vec3(1.0f),velocity,object),Radius(radius),Stuck(true) {

    }
    glm::vec2 Move(float dt, unsigned int windowWidth) {
        if (!this->Stuck) {
            //move the ball
            this->Position += this->Velocity * dt;
            //check if outside the window bounds; if so, reverse velocity and restore at correct postion
            if (this->Position.x <= 0.0f) {
                this->Velocity.x = -this->Velocity.x;
                this->Position.x = 0.0f;
            }
            else if (this->Position.x + this->Size.x >= windowWidth) {
                this->Velocity.x = -this->Velocity.x;
                this->Position.x = windowWidth - this->Size.x;
            }
            if (this->Position.y <= 0.0f) {
                this->Velocity.y = -this->Velocity.y;
                this->Position.y = 0.0f;
            }
        }
        return this->Position;
    }
    void Reset(glm::vec2 position, glm::vec2 velocity) {
        this->Position = position;
        this->Velocity = velocity;
        this->Stuck = true;
    }
};


