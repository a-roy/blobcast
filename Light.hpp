#pragma once

#include <glm/glm.hpp>

struct DirectionalLight
{
	glm::vec3 color;
	glm::vec3 direction;
	glm::vec3 ambientColor;

	DirectionalLight()
	{
		color = glm::vec3(0.0f, 0.0f, 0.0f);
		direction = glm::vec3(0.0f, 0.0f, 0.0f);
		ambientColor = glm::vec3(0.05f, 0.4f, 0.9f);
	}
};