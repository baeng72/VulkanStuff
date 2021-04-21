#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "ResourceManager.h"
#include "GameObject.h"
struct Particle {
    glm::vec2   Position, Velocity;
    glm::vec4   Color;
    float       Life;
    Particle() :Position(0.0f), Velocity(0.0f), Color(1.0f), Life(0.0f) {

    }

};

class ParticleGenerator {
    //std::vector<Particle> particles;
    unsigned int amount;
    unsigned int lastUsedParticle{ 0 };
    ResourceManager& resourceManager;
    
    unsigned int firstUnusedParticle() {
        //first search from last used particle, this will usually return almost instantly
        for (unsigned int i = lastUsedParticle; i < amount; ++i) {
            if (particles[i].Life <= 0.0f) {
                lastUsedParticle = i;
                return i;
            }
        }
        //otherwise, do a linear search
        for (unsigned int i = 0; i < lastUsedParticle; ++i) {
            if (particles[i].Life <= 0.0f) {
                lastUsedParticle = i;
                return i;
            }
        }
        lastUsedParticle = 0;
        return 0;
    }
    void respawnParticle(Particle& particle, GameObject& object, glm::vec2 offset = glm::vec2(0.0f, 0.0f)) {
        float random = ((rand() % 100) - 50) / 10.0f;
        float rColor = 2.5f + ((rand() % 100) / 100.0f);
        particle.Position = object.Position + random + offset;
        particle.Color = glm::vec4(rColor, rColor, rColor, 1.0f);
        particle.Life = 1.0f;
        particle.Velocity = object.Velocity * 0.1f;
    }
    void Init() {
        //create amount default particle particles
        for (unsigned int i = 0; i < amount; ++i) {
            particles.push_back(Particle());
        }
    }
public:
    std::vector<Particle> particles;
    ParticleGenerator(ResourceManager& resourceM, unsigned int amt) :resourceManager(resourceM), amount(amt) {
        Init();
    }
    void Update(float dt, GameObject& object, unsigned int newParticles, glm::vec2 offset = glm::vec2(0.0f, 0.0f)) {
        //add new particles
        for (unsigned int i = 0; i < newParticles; ++i) {
            int unusedParticles = firstUnusedParticle();
            respawnParticle(particles[unusedParticles], object, offset);
        }
        //update all particles
        for (unsigned int i = 0; i < amount; ++i) {
            Particle& p = particles[i];
            p.Life -= dt;
            if (p.Life > 0.0f) {
                p.Position -= p.Velocity * dt;
                if(p.Color.a>0.0f)
                    p.Color.a -= dt * 0.25f;
            }
        }
    }
    void Draw(DrawInfo& drawInfo, VkCommandBuffer cmd, const char* solidName, const char* particleName) {
        resourceManagerDrawObject(resourceManager, drawInfo, cmd, particleName);
    }
};
