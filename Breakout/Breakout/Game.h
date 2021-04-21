#pragma once
#include <irrKlang.h>
using namespace irrklang;

#include "ResourceManager.h"
#include "GameLevel.h"
#include "ParticleGenerator.h"
#include "TextManager.h"
enum GameState {
	GAME_ACTIVE,
	GAME_MENU,
	GAME_WIN
};

//Initial size of the player paddle
const glm::vec2 PLAYER_SIZE(100.0f, 20.0f);
//Initial velocity of the player paddle
const float PLAYER_VELOCITY(500.0f);

const glm::vec2 INITIAL_BALL_VELOCITY(100.0f, -350.0f);
//Radius of ball
const float BALL_RADIUS = 25.0f;// 12.5f;

const unsigned int NR_PARTICLES = 500;

// Represents the four possible (collision) directions
enum Direction {
	UP,
	RIGHT,
	DOWN,
	LEFT
};
// Defines a Collision typedef that represents collision data
typedef std::tuple<bool, Direction, glm::vec2> Collision; // <collision?, what direction?, difference vector center - closest point>

class Game
{
	ISoundEngine* SoundEngine{ nullptr };
	TextManager textManager;
	struct UBOP {
		
		glm::mat4 projection;

	};
	UBOP* ptrSolidMP{ nullptr };
	UBOP* ptrNonSolidMP{ nullptr };
	UBOP* ptrBackgroundMP{ nullptr };
	UBOP* ptrBallMP{ nullptr };
	UBOP* ptrPlayerMP{ nullptr };
	UBOP* ptrParticleMP{ nullptr };
	struct InstanceData {
		glm::mat4 model;
		alignas(16) glm::vec3 color;
	};
	unsigned int particlePause = 0;
	InstanceData* ptrSolidInstance{ nullptr };
	InstanceData* ptrNonSolidInstance{ nullptr };
	InstanceData* ptrBackgroundInstance{ nullptr };
	InstanceData* ptrBallInstance{ nullptr };
	InstanceData* ptrPlayerInstance{ nullptr };
	struct ParticleInstanceData {
		glm::vec2 offset;
		alignas(16) glm::vec4 color;
	};
	ParticleInstanceData* ptrParticleInstance{ nullptr };
	uint32_t instanceCountSolid{ 0 };
	uint32_t instanceCountNonSolid{ 0 };
	uint32_t instanceCountParticle{ 0 };
	std::vector<InstanceData> LevelBricks;//cache translations and rotations, as they don't change
public:
	GameState State;
	bool	Keys[1024];
	bool	KeysProcessed[1024];
	unsigned int Width;
	unsigned int Height;
	ResourceManager& resourceManager;
	std::vector<GameLevel> Levels;
	unsigned int Level;
	BallObject *Ball;
	GameObject* Player;
	ParticleGenerator* Particles;
	unsigned int Lives;
	unsigned int Score{ 0 };
	Game(ResourceManager&resourceMan,VkRenderPass renderPass, VkExtent2D extent, VkSampleCountFlagBits numSamples, unsigned int width, unsigned int height) :resourceManager(resourceMan),State(GAME_ACTIVE), Keys(), Width(width), Height(height) {
		SoundEngine = createIrrKlangDevice();
		
		assert(SoundEngine);
		textManagerInit(textManager, resourceManager, renderPass, extent, numSamples, "Fonts/AVENGEANCE HEROIC AVENGER.ttf", width, height, "Font");
		GameLevel one(resourceMan);
		one.Load("Levels/one.lvl", Width, Height / 2, "Solid", "NonSolid");
		GameLevel two(resourceMan);
		two.Load("Levels/two.lvl", Width, Height / 2, "Solid", "NonSolid");
		GameLevel three(resourceMan);
		three.Load("Levels/three.lvl", Width, Height / 2, "Solid", "NonSolid");
		GameLevel four(resourceMan);
		four.Load("Levels/four.lvl", Width, Height / 2, "Solid", "NonSolid");
		Levels.push_back(one);
		Levels.push_back(two);
		Levels.push_back(three);
		Levels.push_back(four);
		Level = 0;
		uint32_t maxSolid = 0;
		uint32_t maxNonSolid = 0;
		for (auto& level : Levels) {
			uint32_t levMaxSolid = 0;
			uint32_t levMaxNonSolid = 0;
			for (auto& gameObj : level.Bricks) {
				if (gameObj.obj == "Solid") {
					levMaxSolid++;
				}
				else if (gameObj.obj == "NonSolid") {
					levMaxNonSolid++;
				}
			}
			maxSolid = std::max(maxSolid, levMaxSolid);
			maxNonSolid = std::max(maxNonSolid, levMaxNonSolid);
		}

		ResourceObjectInfo spriteInfo;
		//spriteInfo.diffuseTexture = "Textures/taco_alpha.png";
		spriteInfo.diffuseTexture = "Textures/block_solid.png";
		spriteInfo.enableLod = false;
		spriteInfo.fragShaderPath = "Shaders/sprite.frag.spv";
		spriteInfo.vertShaderPath = "Shaders/sprite.vert.spv";
		spriteInfo.objPath = "Models/plane.obj";

		spriteInfo.uboSize = sizeof(UBOP);
		spriteInfo.instanceSize = sizeof(InstanceData);
		spriteInfo.instanceCount = maxSolid;
		resourceManagerInitObject(resourceManager, spriteInfo, renderPass, extent, numSamples, VK_FALSE, "Solid");
		ptrSolidMP = reinterpret_cast<UBOP*>(resourceManagerGetUniformPtr(resourceManager, "Solid"));
		ptrSolidInstance = reinterpret_cast<InstanceData*>(resourceManagerGetStoragePtr(resourceManager, "Solid"));

		//spriteInfo.diffuseTexture = "Textures/natie_alpha.png";
		spriteInfo.diffuseTexture = "Textures/block.png";
		spriteInfo.fragShaderPath = "Shaders/sprite.frag.spv";
		spriteInfo.vertShaderPath = "Shaders/sprite.vert.spv";
		spriteInfo.objPath = "Models/plane.obj";
		spriteInfo.uboSize = sizeof(UBOP);
		spriteInfo.instanceSize = sizeof(InstanceData);
		spriteInfo.instanceCount = maxNonSolid;
		resourceManagerInitObject(resourceManager, spriteInfo, renderPass, extent, numSamples, VK_FALSE, "NonSolid");
		ptrNonSolidMP = reinterpret_cast<UBOP*>(resourceManagerGetUniformPtr(resourceManager, "NonSolid"));
		ptrNonSolidInstance = reinterpret_cast<InstanceData*>(resourceManagerGetStoragePtr(resourceManager, "NonSolid"));

		spriteInfo.diffuseTexture = "Textures/background.jpg";
		spriteInfo.fragShaderPath = "Shaders/sprite.frag.spv";
		spriteInfo.vertShaderPath = "Shaders/sprite.vert.spv";
		spriteInfo.objPath = "Models/plane.obj";
		spriteInfo.uboSize = sizeof(UBOP);
		spriteInfo.instanceSize = sizeof(InstanceData);
		spriteInfo.instanceCount = 1;//just 1 for now
		resourceManagerInitObject(resourceManager, spriteInfo, renderPass, extent, numSamples, VK_FALSE, "Background");
		ptrBackgroundMP = reinterpret_cast<UBOP*>(resourceManagerGetUniformPtr(resourceManager, "Background"));
		ptrBackgroundInstance = reinterpret_cast<InstanceData*>(resourceManagerGetStoragePtr(resourceManager, "Background"));

		//spriteInfo.diffuseTexture = "Textures/jamie_alpha.png";
		spriteInfo.diffuseTexture = "Textures/awesomeface.png";
		spriteInfo.fragShaderPath = "Shaders/sprite.frag.spv";
		spriteInfo.vertShaderPath = "Shaders/sprite.vert.spv";
		spriteInfo.objPath = "Models/plane.obj";
		spriteInfo.uboSize = sizeof(UBOP);
		spriteInfo.instanceSize = sizeof(InstanceData);
		spriteInfo.instanceCount = 1;//just 1 for now
		resourceManagerInitObject(resourceManager, spriteInfo, renderPass, extent, numSamples, VK_TRUE, "Ball");
		ptrBallMP = reinterpret_cast<UBOP*>(resourceManagerGetUniformPtr(resourceManager, "Ball"));
		ptrBallInstance = reinterpret_cast<InstanceData*>(resourceManagerGetStoragePtr(resourceManager, "Ball"));

		spriteInfo.diffuseTexture = "Textures/paddle.png";
		spriteInfo.fragShaderPath = "Shaders/sprite.frag.spv";
		spriteInfo.vertShaderPath = "Shaders/sprite.vert.spv";
		spriteInfo.objPath = "Models/plane.obj";
		spriteInfo.uboSize = sizeof(UBOP);
		spriteInfo.instanceSize = sizeof(InstanceData);
		spriteInfo.instanceCount = 1;//just 1 for now
		resourceManagerInitObject(resourceManager, spriteInfo, renderPass, extent, numSamples, VK_TRUE, "Paddle");
		ptrPlayerMP = reinterpret_cast<UBOP*>(resourceManagerGetUniformPtr(resourceManager, "Paddle"));
		ptrPlayerInstance = reinterpret_cast<InstanceData*>(resourceManagerGetStoragePtr(resourceManager, "Paddle"));

		//Particles
		spriteInfo.diffuseTexture = "Textures/particle.png";
		spriteInfo.fragShaderPath = "Shaders/particle.frag.spv";
		spriteInfo.vertShaderPath = "Shaders/particle.vert.spv";
		spriteInfo.objPath = "Models/plane.obj";
		spriteInfo.uboSize = sizeof(UBOP);
		spriteInfo.instanceSize = sizeof(ParticleInstanceData);
		spriteInfo.instanceCount = NR_PARTICLES;
		resourceManagerInitObject(resourceManager, spriteInfo, renderPass, extent, numSamples, VK_TRUE, "Particle");
		ptrParticleMP = reinterpret_cast<UBOP*>(resourceManagerGetUniformPtr(resourceManager, "Particle"));
		ptrParticleInstance = reinterpret_cast<ParticleInstanceData*>(resourceManagerGetStoragePtr(resourceManager, "Particle"));




		glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(this->Width), 0.0f, -static_cast<float>(this->Height), -1.0f, 1.0f);
		projection[1][1] *= -1;

		ptrBackgroundMP->projection = projection;
		ptrSolidMP->projection = projection;
		ptrNonSolidMP->projection = projection;
		ptrBallMP->projection = projection;
		ptrPlayerMP->projection = projection;
		ptrParticleMP->projection = projection;
		glm::mat4 identity = glm::mat4(1.0f);
		ptrBackgroundInstance->model = glm::scale(identity, glm::vec3(Width, Height, 1.0f));
		ptrBackgroundInstance->color = glm::vec3(1.0f);
		for (auto& gameObj : Levels[Level].Bricks) {
			if (!gameObj.Destroyed) {
				glm::vec3 color = gameObj.Color;

				glm::mat4 model = glm::translate(identity, glm::vec3(gameObj.Position, 0.0));
				model = glm::rotate(model, glm::radians(gameObj.Rotation), glm::vec3(0.0f, 0.0f, 1.0f));
				model = glm::scale(model, glm::vec3(gameObj.Size.x, gameObj.Size.y, 1.0f));
				InstanceData instanceData;
				instanceData.model = model;
				instanceData.color = color;

				LevelBricks.push_back(instanceData);//cache transformation as it's one off for each brick
				
			}
		}

		glm::vec2 playerPos = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
		Player = new GameObject(playerPos, PLAYER_SIZE, glm::vec3(1.0f), glm::vec2(0.0f, 0.0f), "Paddle");
		assert(Player);


		glm::vec2 ballPos = playerPos + glm::vec2(PLAYER_SIZE.x / 2.0f - BALL_RADIUS, -BALL_RADIUS * 2.0f);
		Ball = new BallObject(ballPos, BALL_RADIUS, INITIAL_BALL_VELOCITY, "Ball");
		assert(Ball);

		Particles = new ParticleGenerator(resourceManager, NR_PARTICLES);
		assert(Particles);
		ResetLevel();
		ResetPlayer();
		//prepare text for menu
		
		//State = GAME_MENU;
	}
	~Game() {
		
	}
	void Init() {
		
		SoundEngine->play2D("Music/jazzyfrenchy2.mp3", true);

	}
	void ProcessInput(float dt) {
		if (State == GAME_ACTIVE) {
			float velocity = PLAYER_VELOCITY * dt;
			//move playerboard
			if (this->Keys[GLFW_KEY_A]) {
				if (Player->Position.x >= 0.0f) {
					Player->Position.x -= velocity;
					if (Ball->Stuck)
						Ball->Position.x -= velocity;
				}
			}
			if (this->Keys[GLFW_KEY_D]) {
				if (Player->Position.x <= this->Width - Player->Size.x) {
					Player->Position.x += velocity;
					if (Ball->Stuck) {
						Ball->Position.x += velocity;
					}
				}
			}
			if (this->Keys[GLFW_KEY_SPACE]) {
				Ball->Stuck = false;
				//SoundEngine->play2D("Music/Grunt.mp3", false);
			}
		}
		else if (State == GAME_MENU) {
			if (Keys[GLFW_KEY_ENTER]&&!KeysProcessed[GLFW_KEY_ENTER]) {
				State = GAME_ACTIVE;
				KeysProcessed[GLFW_KEY_ENTER] = true;
				textManagerUpdate(textManager, resourceManager, glm::vec2(425, -200), 1.0f, nullptr);
				textManagerUpdate(textManager, resourceManager, glm::vec2(375, -150), 1.0f, nullptr);
			}
			uint32_t currLevel = Level;
			if (Keys[GLFW_KEY_W]&&!KeysProcessed[GLFW_KEY_W]) {
				Level = (Level + 1) % 4;
				KeysProcessed[GLFW_KEY_W] = true;
			}
			if (Keys[GLFW_KEY_S]&&!KeysProcessed[GLFW_KEY_S]) {
				if (Level > 0)
					--Level;
				else
					Level = 3;
				KeysProcessed[GLFW_KEY_S] = true;
			}
			if (Level != currLevel) {
				
				LevelBricks.clear();
				for (auto& gameObj : Levels[Level].Bricks) {
					if (!gameObj.Destroyed) {
						glm::vec3 color = gameObj.Color;

						glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(gameObj.Position, 0.0));
						model = glm::rotate(model, glm::radians(gameObj.Rotation), glm::vec3(0.0f, 0.0f, 1.0f));
						model = glm::scale(model, glm::vec3(gameObj.Size.x, gameObj.Size.y, 1.0f));
						InstanceData instanceData;
						instanceData.model = model;
						instanceData.color = color;

						LevelBricks.push_back(instanceData);//cache transformation as it's one off for each brick

					}
				}
			}
			PrintInfo();
		}
		else if (State = GAME_WIN) {
			textManagerUpdate(textManager, resourceManager, glm::vec2(250, -300), 1.0f, nullptr);
			textManagerUpdate(textManager, resourceManager, glm::vec2(250, -200), 1.0f, nullptr);
		}
	}
	void Update(float dt) {

		

		//InstanceData* currSolid = ptrSolidInstance;
		instanceCountSolid = instanceCountNonSolid = 0;
		//InstanceData* currNonSolid = ptrNonSolidInstance;
		size_t idx = 0;
		for (auto& gameObj : Levels[Level].Bricks) {

			if (!gameObj.Destroyed) {
#define CACHE_XFORMS
#ifdef CACHE_XFORMS
				

				if (gameObj.obj == "Solid") {
					ptrSolidInstance[instanceCountSolid++] = LevelBricks[idx];
				
				}
				else if (gameObj.obj == "NonSolid") {
					ptrNonSolidInstance[instanceCountNonSolid++] = LevelBricks[idx];
				
				}

#else
				glm::vec3 color = gameObj.Color;
				glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(gameObj.Position, 0.0));
				model = glm::rotate(model, glm::radians(gameObj.Rotation), glm::vec3(0.0f, 0.0f, 1.0f));
				model = glm::scale(model, glm::vec3(gameObj.Size.x, gameObj.Size.y, 1.0f));

				if (gameObj.obj == "Solid") {
					
					ptrSolidInstance[instanceCountSolid].model = model;
					ptrSolidInstance[instanceCountSolid].color = color;
					instanceCountSolid++;
					//currSolid->model = model;
					//currSolid->color = color;
					//currSolid++;
				}
				else if (gameObj.obj == "NonSolid") {
					
					ptrNonSolidInstance[instanceCountNonSolid].model = model;
					ptrNonSolidInstance[instanceCountNonSolid].color = color;
					instanceCountNonSolid++;
					//currNonSolid->model = model;
					//currNonSolid->color = color;
					//currNonSolid++;
				}
#endif				
			}
			idx++;
		}
		
		
		//update ball object
		Ball->Move(dt, Width);
		glm::vec3 color = Ball->Color;
		glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(Ball->Position, 0.0f));
		model = glm::rotate(model, glm::radians(Ball->Rotation), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(Ball->Size.x, Ball->Size.y, 1.0f));
		if (Ball->Position.y >= this->Height) {
			--Lives;
			if (Lives == 0) {
				SoundEngine->play2D("Music/Sad-Trombone2.mp3", false);
				ResetLevel();
				State = GAME_MENU;
				textManagerUpdate(textManager, resourceManager, glm::vec2(425, -200), 1.0f, "Press ENTER to start");
				textManagerUpdate(textManager, resourceManager, glm::vec2(375, -150), 1.0f, "Press W or S to Select Level");
			}
			else {
				SoundEngine->play2D("Music/bad-answer2.mp3", false);
				textManagerUpdate(textManager, resourceManager, glm::vec2(425, -200), 1.0f,nullptr);
				textManagerUpdate(textManager, resourceManager, glm::vec2(375, -150), 1.0f, nullptr);
			}

			
			ResetPlayer();
		}
		uint32_t numParticles = !Ball->Stuck && particlePause++==10 ? 2 : 0;
		if (particlePause > 10)
			particlePause = 0;
		Particles->Update(dt, *Ball, numParticles, glm::vec2(Ball->Radius / 2.0f));
		
		
			instanceCountParticle = 0;
			for (auto& particle : Particles->particles) {
				if (particle.Life > 0.0f) {
					ptrParticleInstance[instanceCountParticle].offset = particle.Position;
					ptrParticleInstance[instanceCountParticle].color = particle.Color;
					instanceCountParticle++;
				}
			}
			
		
		ptrBallInstance->model = model;
		ptrBallInstance->color = color;

		color = Player->Color;
		model = glm::translate(glm::mat4(1.0f), glm::vec3(Player->Position, 0.0f));
		model = glm::rotate(model, glm::radians(Player->Rotation), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model,glm::vec3(Player->Size.x, Player->Size.y, 1.0f));
		ptrPlayerInstance->model = model;
		ptrPlayerInstance->color = color;
		DoCollisions();
		if (State == GAME_ACTIVE && Levels[Level].IsCompleted()) {
			ResetLevel();
			ResetPlayer();
			State = GAME_WIN;
			textManagerUpdate(textManager, resourceManager, glm::vec2(250, -300), 1.0f, "You WON!!!!");
			textManagerUpdate(textManager, resourceManager, glm::vec2(250, -200), 1.0f, "Press ENTER to retury or ESC to quit");
		}


		
	}
	void Render(DrawInfo&drawInfo,VkCommandBuffer cmd) {
		if (State == GAME_ACTIVE || State == GAME_MENU) {
			resourceManagerDrawObject(resourceManager, drawInfo, cmd, "Background");
			resourceManagerDrawObject(resourceManager, drawInfo, cmd, "Solid", instanceCountSolid);
			resourceManagerDrawObject(resourceManager, drawInfo, cmd, "NonSolid",  instanceCountNonSolid);
			resourceManagerDrawObject(resourceManager, drawInfo, cmd, "Paddle");
			resourceManagerDrawObject(resourceManager, drawInfo, cmd, "Particle", instanceCountParticle);
			resourceManagerDrawObject(resourceManager, drawInfo, cmd, "Ball");
			resourceManagerDrawObject(resourceManager, drawInfo, cmd, "Font");
			
			
		}
		else if (State == GAME_WIN) {
			resourceManagerDrawObject(resourceManager, drawInfo, cmd, "Font");
		}
		
	}

	void Clear() {
		resourceManagerDestroy(resourceManager);
		delete Particles;
		delete Ball;
		delete Player;
		SoundEngine->drop();
	}

	void ResetLevel() {
		if (this->Level == 0)
			this->Levels[0].Load("Levels/one.lvl", this->Width, this->Height / 2, "Solid", "NonSolid");
		else if (this->Level == 1)
			this->Levels[1].Load("Levels/two.lvl", this->Width, this->Height / 2, "Solid", "NonSolid");
		else if (this->Level == 2)
			this->Levels[2].Load("Levels/three.lvl", this->Width, this->Height / 2, "Solid", "NonSolid");
		else if (this->Level == 3)
			this->Levels[3].Load("Levels/four.lvl", this->Width, this->Height / 2, "Solid", "NonSolid");
		Lives = 3;
		Score = 0;
		PrintScore();
	}

	void ResetPlayer() {
		// reset player/ball stats
		Player->Size = PLAYER_SIZE;
		Player->Position = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
		Ball->Reset(Player->Position + glm::vec2(PLAYER_SIZE.x / 2.0f - BALL_RADIUS, -(BALL_RADIUS * 2.0f)), INITIAL_BALL_VELOCITY);
		
		PrintInfo();
	}

	void PrintInfo() {
		char buffer[32];
		sprintf_s(buffer, sizeof(buffer), "Level: %d", (Level+1));
		textManagerUpdate(textManager, resourceManager, glm::vec2(5.0f, -50.0f), 1.0f, buffer);
		sprintf_s(buffer, sizeof(buffer), "Lives: %d", Lives);
		textManagerUpdate(textManager, resourceManager, glm::vec2(5.0f, -5.0f), 1.0f, buffer);
		
	}
	void PrintScore() {
		char buffer[32];
		sprintf_s(buffer, sizeof(buffer), "Score: %d", Score);
		textManagerUpdate(textManager, resourceManager, glm::vec2(1000, -5.0f), 1.0f, buffer);
	}

	void DoCollisions()
	{
		for (GameObject& box : this->Levels[this->Level].Bricks)
		{
			if (!box.Destroyed)
			{
				Collision collision = CheckCollision(*Ball, box);
				if (std::get<0>(collision)) // if collision is true
				{
					// destroy block if not solid
					if (!box.IsSolid) {
						box.Destroyed = true;
						SoundEngine->play2D("Music/bleep.wav", false);
						Score++;
						PrintScore();
					}
					else {
						SoundEngine->play2D("Music/bleep.wav", false);
					}
					// collision resolution
					Direction dir = std::get<1>(collision);
					glm::vec2 diff_vector = std::get<2>(collision);
					if (dir == LEFT || dir == RIGHT) // horizontal collision
					{
						Ball->Velocity.x = -Ball->Velocity.x; // reverse horizontal velocity
						// relocate
						float penetration = Ball->Radius - std::abs(diff_vector.x);
						if (dir == LEFT)
							Ball->Position.x += penetration; // move ball to right
						else
							Ball->Position.x -= penetration; // move ball to left;
					}
					else // vertical collision
					{
						Ball->Velocity.y = -Ball->Velocity.y; // reverse vertical velocity
						// relocate
						float penetration = Ball->Radius - std::abs(diff_vector.y);
						if (dir == UP)
							Ball->Position.y -= penetration; // move ball bback up
						else
							Ball->Position.y += penetration; // move ball back down
					}
				}
			}
		}
		// check collisions for player pad (unless stuck)
		Collision result = CheckCollision(*Ball, *Player);
		if (!Ball->Stuck && std::get<0>(result))
		{
			// check where it hit the board, and change velocity based on where it hit the board
			float centerBoard = Player->Position.x + Player->Size.x / 2.0f;
			float distance = (Ball->Position.x + Ball->Radius) - centerBoard;
			float percentage = distance / (Player->Size.x / 2.0f);
			// then move accordingly
			float strength = 2.0f;
			glm::vec2 oldVelocity = Ball->Velocity;
			Ball->Velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
			//Ball->Velocity.y = -Ball->Velocity.y;
			Ball->Velocity = glm::normalize(Ball->Velocity) * glm::length(oldVelocity); // keep speed consistent over both axes (multiply by length of old velocity, so total strength is not changed)
			// fix sticky paddle
			Ball->Velocity.y = -1.0f * abs(Ball->Velocity.y);
			SoundEngine->play2D("Music/Grunt.mp3", false);
		}
	}


	Collision CheckCollision(BallObject& one, GameObject& two) // AABB - Circle collision
	{
		// get center point circle first 
		glm::vec2 center(one.Position + one.Radius);
		// calculate AABB info (center, half-extents)
		glm::vec2 aabb_half_extents(two.Size.x / 2.0f, two.Size.y / 2.0f);
		glm::vec2 aabb_center(two.Position.x + aabb_half_extents.x, two.Position.y + aabb_half_extents.y);
		// get difference vector between both centers
		glm::vec2 difference = center - aabb_center;
		glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
		// now that we know the the clamped values, add this to AABB_center and we get the value of box closest to circle
		glm::vec2 closest = aabb_center + clamped;
		// now retrieve vector between center circle and closest point AABB and check if length < radius
		difference = closest - center;

		if (glm::length(difference) < one.Radius) // not <= since in that case a collision also occurs when object one exactly touches object two, which they are at the end of each collision resolution stage.
			return std::make_tuple(true, VectorDirection(difference), difference);
		else
			return std::make_tuple(false, UP, glm::vec2(0.0f, 0.0f));
	}
	// calculates which direction a vector is facing (N,E,S or W)
	Direction VectorDirection(glm::vec2 target)
	{
		glm::vec2 compass[] = {
			glm::vec2(0.0f, 1.0f),	// up
			glm::vec2(1.0f, 0.0f),	// right
			glm::vec2(0.0f, -1.0f),	// down
			glm::vec2(-1.0f, 0.0f)	// left
		};
		float max = 0.0f;
		unsigned int best_match = -1;
		for (unsigned int i = 0; i < 4; i++)
		{
			float dot_product = glm::dot(glm::normalize(target), compass[i]);
			if (dot_product > max)
			{
				max = dot_product;
				best_match = i;
			}
		}
		return (Direction)best_match;
	}
};
