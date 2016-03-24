#pragma once

#include <vector>
#include <string>
#include "GameObject.h"
#include <array>
#include "ParticleSystem.h"
#include "Profiler.h"

class Level
{
	public:
		static Level* currentLevel;

		std::vector<GameObject*> Objects;
		//std::vector<Button *> Buttons;
		std::vector<ParticleSystem *> ParticleSystems;
		std::array<Mesh *, SHAPE_NUMITEMS> Meshes;

		Level();
		~Level();
		Level(const Level& other);
		Level& operator=(const Level& other);
		Level(Level&& other);
		Level& operator=(Level&& other);
		std::size_t AddGameObject(
				glm::vec3 position,
				glm::quat orientation,
				glm::vec3 dimensions,
				glm::vec4 color,
				GLuint texID,
				std::string type,
				float mass = 0.f
				);
		void Delete(std::size_t index);
		void Clear();
		int Find(btRigidBody *r);
		GameObject* Find(int id);
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
