#include "ParticleSystem.h"

Camera* BufferData::cam = NULL;

bool operator<(BufferData& bd1, BufferData& bd2)
{
	return glm::distance(bd1.position, BufferData::cam->Position) >
		glm::distance(bd2.position, BufferData::cam->Position);
}

Particle::Particle(glm::vec3 startPos)
{
	position = startPos;
}

void Particle::Reset()
{
	position = glm::vec3(0);
	velocity = glm::vec3(0);
	acceleration = glm::vec3(0);
	age = 0;
}

void Emitter::Emit(Particle* particle)
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

ParticleSystem::ParticleSystem(glm::vec3 position, const char* file, int size)
{
	textureID = loadTexture(file, true);
	maxSize = size;

	for (int i = 0; i < size; i++)
	{
		Particle* p = new Particle(emitter.centre);
		p->active = false;
		inactiveParticles.push(p);
		particles.push_back(p);
	}

	Generate();
}

ParticleSystem::~ParticleSystem()
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

void ParticleSystem::Generate()
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

#include <iostream>
void ParticleSystem::Update(double deltaTime)
{
	std::vector<BufferData> data;

	static double timer = 0;
	timer += deltaTime;

	while (timer >= 1.0f / emitter.emitRate)
	{
		timer -= (1.0f / emitter.emitRate);

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
			Euler(particles[i], deltaTime * simulationSpeed);

			if (glm::dot(particles[i]->position - plane, normal) < 0.01f
				&& glm::dot(normal, particles[i]->velocity) < 0.01f)
			{
				if (bRecyclePlane)
				{
					inactiveParticles.push(particles[i]);
					particles[i]->active = false;
					particles[i]->Reset();
					continue;
				}

				if (bResponse)
				{
					particles[i]->position +=
						-glm::dot(particles[i]->position - plane, normal)
						* normal; //post processing method
					particles[i]->velocity +=
						(1 + coefficientOfRestitution)
						* -glm::dot(particles[i]->velocity, normal)
						* normal;
				}
			}

			particles[i]->age += deltaTime * simulationSpeed;
			particles[i]->color = glm::mix(startColour, endColour,
				particles[i]->age / particleLife);
			if (particles[i]->age > particleLife && bRecycleAge)
			{
				inactiveParticles.push(particles[i]);
				particles[i]->active = false;
				particles[i]->Reset();
				continue;
			}

			BufferData bd;
			bd.position = particles[i]->position;
			bd.colour = particles[i]->color;
			data.push_back(bd);
		}
	}

	liveParticles = data.size();

	if(PARTICLE_ZSORT && BufferData::cam != NULL)
		std::sort(data.begin(), data.end());

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

void ParticleSystem::Euler(Particle* p, float timeStep)
{
	p->position += timeStep * p->velocity;
	p->velocity += timeStep * p->acceleration;
	p->acceleration = FNet(p->position, p->velocity) * invMass;
}

glm::vec3 ParticleSystem::FNet(glm::vec3 pos, glm::vec3 vel)
{
	glm::vec3 fNet;

	if (bGravity)
		fNet += Gravity();
	if (bDrag)
		fNet += Drag(vel);

	return fNet;
}

glm::vec3 ParticleSystem::Gravity()
{
	return mass * gravity;
}

glm::vec3 ParticleSystem::Drag(glm::vec3 vel)
{
	return dragCoefficient * -vel;
}

void ParticleSystem::Render()
{
	glBindVertexArray(vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glDrawArrays(GL_POINTS, 0, maxSize - inactiveParticles.size());
	glBindVertexArray(0);
}