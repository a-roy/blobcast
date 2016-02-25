#pragma once

#include <glm/glm.hpp>

struct BaseLight
{
	glm::vec3 color;
	float ambientIntensity;
	float diffuseIntensity;

	BaseLight()
	{
		color = glm::vec3(0.0f, 0.0f, 0.0f);
		ambientIntensity = 0.0f;
		diffuseIntensity = 0.0f;
	}
};

struct DirectionalLight : public BaseLight
{
	glm::vec3 direction;

	DirectionalLight()
	{
		direction = glm::vec3(0.0f, 0.0f, 0.0f);
	}
};