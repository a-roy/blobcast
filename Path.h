#pragma once

#include <glm/glm.hpp>
#include <vector>

class Path
{
	typedef std::vector<glm::vec3>::iterator iterator;

	public:
		std::vector<glm::vec3> Points;
		float Speed = 0.0f;
		bool Loop = true;

		Path() = default;
		void Reset();
		void Step(float dt = 1.0f);
		glm::vec3 GetPosition();
		glm::vec3 GetPoint(int p);

	private:
		float position = 0.0f;
		glm::vec3 CatmullRomTangent(int p);
};
