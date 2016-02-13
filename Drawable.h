#pragma once

#include <glm/glm.hpp>

class Drawable
{
	public:
		virtual void Draw() const = 0;
		virtual void ComputeAABB(
				float& min_x, float& min_y, float& min_z,
				float& max_x, float& max_y, float& max_z) const = 0;
		virtual float ComputeRadius(glm::vec3 center) const = 0;
};
