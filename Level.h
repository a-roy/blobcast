#pragma once

#include <vector>
#include <string>
#include "RigidBody.h"
#include "ParticleSystem.h"

class Level
{
	public:
		std::vector<RigidBody *> Objects;
		std::vector<ParticleSystem *> ParticleSystems;

		~Level();
		std::size_t AddBox(
				glm::vec3 position,
				glm::quat orientation,
				glm::vec3 dimensions,
				glm::vec4 color,
				float mass = 0.f);
		void Delete(std::size_t index);
		int Find(btRigidBody *r);
		void Render(GLuint uMMatrix, GLuint uColor);
		void Serialize(std::string file);
		static Level *Deserialize(std::string file);

		/*std::size_t AddParticleSystem(glm::vec3 position, Camera* camera = NULL)
		{
			ParticleSystem* ps = new ParticleSystem(camera);
			ParticleSystems.push_back(ps);
			return Objects.size() - 1;
		}*/
};
