#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Drawable.h"
#include <vector>

class Frame
{
	public:
		std::vector<Drawable *> Objects;
		std::vector<Frame *> Children;
		glm::vec3 Scale = glm::vec3(1.0f);
		glm::quat Rotation;
		glm::vec3 Translation = glm::vec3(0.0f);

		void Draw(GLuint uMVMatrix, GLuint uNMatrix, glm::mat4 mvMatrix) const;
		void ComputeMetrics(glm::vec3& center, float& volume) const;
		void ComputeAABB(
				float& min_x, float& min_y, float& min_z,
				float& max_x, float& max_y, float& max_z) const;
		float ComputeRadius(glm::vec3 center) const;
		glm::mat4 GetMatrix() const;
};
