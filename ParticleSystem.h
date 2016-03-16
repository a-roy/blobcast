#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>

#include <vector>
#include <queue>

#include <GL/glew.h>
#include <SOIL/SOIL.h>

#include "Helper.h"
#include "config.h"
#include "Camera.h"

struct BufferData
{
	static Camera* cam;
	glm::vec3 position;
	glm::vec4 colour;
};

bool operator<(BufferData& bd1, BufferData& bd2);

struct Particle
{
	bool active = true;
	glm::vec3 position;
	glm::vec3 velocity = glm::vec3(0);
	glm::vec3 acceleration = glm::vec3(0);
	glm::vec4 color;
	float age = 0;

	Particle(glm::vec3 startPos);
	void Reset();
};

struct Emitter
{
	glm::vec3 centre = glm::vec3(0.0f);
	int emitRate = 10;
	glm::vec3 emitArea = glm::vec3(0.0f);
	float velYMin = 30.0f;
	float velYMax = 50.0f;
	float velXZMin = 10.0f;
	float velXZMax = 20.0f;

	void Emit(Particle* particle);
};

class ParticleSystem
{
private:

	std::vector<Particle*> particles;
	std::queue<Particle*> inactiveParticles;

	GLuint vao;
	GLuint vbo;

	int maxSize;

	GLuint textureID;

public:

	Emitter emitter;

	int liveParticles = 0;
	float simulationSpeed = 1.0f;
	
	bool bResponse = false;
	glm::vec3 plane = glm::vec3(0, 0, 0);
	glm::vec3 normal = glm::vec3(0, 1, 0);
	float coefficientOfRestitution = 0.5f;

	bool bGravity = true;
	glm::vec3 gravity = glm::vec3(0, -9.81f, 0);
	float mass = 1.0f;
	float invMass = 1.0f / mass;

	bool bDrag = true;
	float dragCoefficient = 10.0f;
	
	glm::vec4 startColour = glm::vec4(1, 0, 0, 1);
	glm::vec4 endColour = glm::vec4(1, 0, 0, 1);
	float particleLife = 5;
	bool bRecyclePlane = true;
	bool bRecycleAge = true;

	float pointSize = 5.0f;
	bool bZSort = true;

	ParticleSystem(int size = 1000);
	~ParticleSystem();

	void Update(double deltaTime);
	void Render();

private:

	void Generate();
	void Euler(Particle* p, float timeStep);
	glm::vec3 FNet(glm::vec3 pos, glm::vec3 vel);
	
	glm::vec3 Gravity();
	glm::vec3 Drag(glm::vec3 vel);
	
};