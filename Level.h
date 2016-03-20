#pragma once

#include <vector>
#include <string>
#include "Entity.h"
#include "Button.h"
#include "ParticleSystem.h"
#include "Profiler.h"
#include "Platform.h"

class Level
{
	public:
		std::vector<Entity*> Objects;
		//std::vector<Button *> Buttons;
		std::vector<ParticleSystem *> ParticleSystems;

		Level() = default;
		~Level();
		Level(const Level& other);
		Level& operator=(const Level& other);
		Level(Level&& other);
		Level& operator=(Level&& other);
		std::size_t AddBox(
				glm::vec3 position,
				glm::quat orientation,
				glm::vec3 dimensions,
				glm::vec4 color,
				float mass = 0.f);
		std::size_t AddCylinder(
				glm::vec3 position,
				glm::quat orientation,
				glm::vec3 dimensions,
				glm::vec4 color,
				float mass = 0.f);
		std::size_t AddButton(
				glm::vec3 position,
				glm::quat orientation,
				glm::vec3 dimensions,
				glm::vec4 color,
				float mass = 0.f);
		void Delete(std::size_t index);
		void Clear();
		int Find(btRigidBody *r);
		void Render(GLuint uMMatrix, GLuint uColor);
		void Serialize(std::string file);
		static Level *Deserialize(std::string file);

		std::size_t AddParticleSystem(glm::vec3 position)
		{
			ParticleSystem* ps = new ParticleSystem(position);
			ParticleSystems.push_back(ps);
			return Objects.size() - 1;
		}

		void RenderParticles(GLuint uSize)
		{
			Profiler::Start("Particles");
			for (auto ps : ParticleSystems)
			{
				glUniform1f(uSize, ps->pointSize);
				ps->Render();
			}
			Profiler::Finish("Particles", true);
		}
};
