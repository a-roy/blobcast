#pragma once

#include <vector>
#include <string>
#include "RigidBody.h"
#include "Button.h"

class Level
{
	public:
		std::vector<RigidBody *> Objects;
		std::vector<Button *> Buttons;

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
		std::size_t Level::AddCylinder(
				glm::vec3 position,
				glm::quat orientation,
				glm::vec3 dimensions,
				glm::vec4 color,
				float mass = 0.f);
		std::size_t Level::AddButton(
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
};
