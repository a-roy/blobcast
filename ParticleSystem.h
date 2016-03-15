#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>

#include <vector>
#include <queue>

#include <GL/glew.h>
#include <SOIL/SOIL.h>

#include "Helper.h"
#include "config.h"

struct Environment
{
	float gravity;
	glm::vec3 wind;
	float fluidDensity;
};

struct BufferData
{
	glm::vec3 position;
	glm::vec4 colour;
};

//bool operator<(BufferData& bd1, BufferData& bd2)
//{
	// reverse order
	//return this->cameraDistance > that.cameraDistance;

	/*return glm::length2(bd1.position - 
	Camera::Instance->viewProperties.position)
> glm::length2(bd2.position - Camera::Instance->viewProperties.position);*/
//}

struct Particle
{
	bool active = true;
	glm::vec3 position;
	glm::vec3 velocity = glm::vec3(0);
	glm::vec3 acceleration = glm::vec3(0);
	glm::vec4 color;
	float age = 0;

	Particle(glm::vec3 startPos)
	{
		position = startPos;
	}

	void Reset()
	{
		position = glm::vec3(0);
		velocity = glm::vec3(0);
		acceleration = glm::vec3(0);
		age = 0;
	}
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
	
	Emitter()
	{
	}

	void Emit(Particle* particle)
	{
		glm::linearRand(0.0f, 0.0f);
		particle->position = glm::vec3(
			centre.x + glm::linearRand(-emitArea.x*0.5f, emitArea.x*0.5f),
			centre.y + glm::linearRand(-emitArea.y*0.5f, emitArea.y*0.5f),
			centre.z + glm::linearRand(-emitArea.z*0.5f, emitArea.z*0.5f));
		glm::vec2 xz = glm::circularRand(glm::linearRand(velXZMin, velXZMax));
		particle->velocity = glm::vec3(xz.x, glm::linearRand(velYMin, velYMax), 
			xz.y);
	}
};

class ParticleSystem
{
private:

	std::vector<Particle*> particles;
	std::queue<Particle*> inactiveParticles;

	GLuint vao;
	GLuint vbo;

	GLuint posBuffer;
	GLuint colBuffer;

	int maxSize;

	GLuint textureID;

public:

	int liveParticles;
	float simulationSpeed;

	bool gravity;
	bool wind;
	bool drag;

	Environment env;
	bool bCollisions;
	glm::vec3 plane, normal;
	float coefficientOfRestitution; //Collision stuff
	float radius, surfaceArea, dragCoefficient; //Drag stuff
	float mass;

	Emitter emitter;

	glm::vec4 startColour;
	glm::vec4 endColour;
	float particleLife;
	bool bRecyclePlane;
	bool bRecycleAge;
	float pointSize;

	//Camera* camera;
	//bool bZSort;

	ParticleSystem(/*Camera* p_camera,*/ int size = 1000)
	{
		//camera = p_camera;
		textureID = loadTexture(TextureDir "particle.png");

		startColour = glm::vec4(1, 0, 0, 1);
		endColour = glm::vec4(1, 0, 0, 1);
		particleLife = 5;
		pointSize = 5.0f;
		maxSize = size;
		liveParticles = 0;

		plane = glm::vec3(0, 0, 0);
		normal = glm::vec3(0, 1, 0);
		coefficientOfRestitution = 0.5f;

		simulationSpeed = 0.4f;
		gravity = true;
		drag = true;
		wind = false;

		env.gravity = 9.81f;
		env.wind = glm::vec3(100.0f, 0, 0);
		env.fluidDensity = 1.225f; //air
		dragCoefficient = 0.47f; //sphere  
		radius = 0.05f;
		surfaceArea = 3.14159265 * glm::pow(radius, 2.0f); //sphere
		mass = 1;

		for (int i = 0; i < size; i++)
		{
			Particle* p = new Particle(emitter.centre);

			p->active = false;

			inactiveParticles.push(p);
			particles.push_back(p);
		}

		bCollisions = true;

		bRecycleAge = true;
		bRecyclePlane = true;

		//bZSort = true;
	}

	~ParticleSystem()
	{
		for (auto it = particles.begin(); it != particles.end(); it++)
		{
			delete *it;
		}

		while (inactiveParticles.size() > 0)
		{
			Particle* p = inactiveParticles.front();
			inactiveParticles.pop();
			delete p;
		}
	}

	void Generate()
	{
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, this->maxSize * sizeof(BufferData), 
			nullptr, GL_STREAM_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BufferData), 
			(GLvoid*)0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(BufferData), 
			(GLvoid*)offsetof(BufferData, colour));

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void Update(double deltaTime)
	{
		std::vector<BufferData> data;

		float timestep = deltaTime;

		for (int i = 0; i < emitter.emitRate; i++)
		{
			if (inactiveParticles.size() > 0)
			{
				Particle* p = inactiveParticles.front();
				emitter.Emit(p);
				inactiveParticles.front()->active = true;
				inactiveParticles.pop();
			}
		}

		for (int i = 0; i < particles.size(); i++)
		{
			if (particles[i]->active)
			{
				Euler(particles[i], timestep * simulationSpeed);
				
				if (glm::dot(particles[i]->position - plane, normal) < 0.01f
					&& glm::dot(normal, particles[i]->velocity) < 0.01f)
				{
					if (bCollisions)
					{
						particles[i]->position +=
							-glm::dot(particles[i]->position - plane, normal)
							* normal; //post processing method
						particles[i]->velocity +=
							(1 + coefficientOfRestitution)
							* -glm::dot(particles[i]->velocity, normal)
							* normal;
					}
					else
					{
						if (bRecyclePlane)
						{
							inactiveParticles.push(particles[i]);
							particles[i]->active = false;
							particles[i]->Reset();
						}
					}
				}
				
				particles[i]->age += timestep * simulationSpeed;
				particles[i]->color = glm::mix(startColour, endColour, 
					particles[i]->age / particleLife);
				if (particles[i]->age > particleLife && bRecycleAge)
				{
					inactiveParticles.push(particles[i]);
					particles[i]->active = false;
					particles[i]->Reset();
				}

				BufferData bd;
				bd.position = particles[i]->position;
				bd.colour = particles[i]->color;
				data.push_back(bd);
			}
		}

		liveParticles = data.size();

		//if (bZSort)
			//std::sort(data.begin(), data.end());

		if (liveParticles > 0)
		{
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, maxSize*sizeof(data[0]), 
				NULL, GL_STREAM_DRAW); // Buffer orphaning
			glBufferSubData(GL_ARRAY_BUFFER, 0, 
				liveParticles*sizeof(data[0]), &data[0]);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
	}

	void Render()
	{
		glBindVertexArray(vao);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glDrawArrays(GL_POINTS, 0, maxSize - inactiveParticles.size());
		glBindVertexArray(0);
	}

	void Euler(Particle* p, float timeStep)
	{
		p->position += timeStep * p->velocity;
		p->velocity += timeStep * p->acceleration;
		p->acceleration = fNet(p->position, p->velocity) / mass;
	}

	glm::vec3 fNet(glm::vec3 pos, glm::vec3 vel)
	{
		glm::vec3 fNet;

		if (gravity)
			fNet += Gravity(pos, vel);
		if (drag)
			if (wind)
				fNet += Drag(pos, vel, true);
			else
				fNet += Drag(pos, vel, false);

		return fNet;
	}

	glm::vec3 Gravity(glm::vec3 pos, glm::vec3 vel)
	{
		return mass * env.gravity * glm::vec3(0, -1, 0);
	}

	glm::vec3 Drag(glm::vec3 pos, glm::vec3 vel, bool wind)
	{
		if (wind)
			return 0.5f * env.fluidDensity * surfaceArea * dragCoefficient
			* (vel - env.wind).length() * -(vel - env.wind);
		else
			return 0.5f * env.fluidDensity * surfaceArea * dragCoefficient
			* vel.length() * -vel;
	}
};