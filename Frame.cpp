#include "Frame.h"
#include <glm/gtx/transform.hpp>

void Frame::Draw(GLuint uMVMatrix, GLuint uNMatrix, glm::mat4 mvMatrix) const
{
	glm::mat4 modelMatrix(GetMatrix());
	mvMatrix = mvMatrix * modelMatrix;
	glUniformMatrix4fv(uMVMatrix, 1, GL_FALSE, &mvMatrix[0][0]);

	if (uNMatrix >= 0)
	{
		glm::mat3 normalMatrix(glm::transpose(glm::inverse(mvMatrix)));
		glUniformMatrix3fv(uNMatrix, 1, GL_FALSE, &normalMatrix[0][0]);
	}

	for (std::size_t i = 0, n = Objects.size(); i < n; i++)
	{
		Objects[i]->Draw();
	}
	for (std::size_t i = 0, n = Children.size(); i < n; i++)
	{
		Children[i]->Draw(uMVMatrix, uNMatrix, mvMatrix);
	}
}

void Frame::ComputeMetrics(glm::vec3& center, float& volume) const
{
	glm::vec3 cum_center(0.f);
	float cum_volume = 0.f;
	for (std::size_t i = 0, n = Objects.size(); i < n; i++)
	{
		float min_x, min_y, min_z, max_x, max_y, max_z;
		Objects[i]->ComputeAABB(min_x, min_y, min_z, max_x, max_y, max_z);
		float object_volume =
			(max_x - min_x) * (max_y - min_y) * (max_z - min_z);
		glm::vec3 object_center(
				(max_x + min_x) * 0.5f,
				(max_y + min_y) * 0.5f,
				(max_z + min_z) * 0.5f);
		cum_center += object_center * object_volume;
		cum_volume += object_volume;
	}
	for (std::size_t i = 0, n = Children.size(); i < n; i++)
	{
		glm::vec3 child_center;
		float child_volume;
		Children[i]->ComputeMetrics(child_center, child_volume);
		cum_center += child_center * child_volume;
		cum_volume += child_volume;
	}
	center = glm::vec3(GetMatrix() * glm::vec4(cum_center / cum_volume, 1.0));
	volume = cum_volume * Scale.x * Scale.y * Scale.z;
}

void Frame::ComputeAABB(
				float& min_x, float& min_y, float& min_z,
				float& max_x, float& max_y, float& max_z) const
{
	if (Objects.empty())
	{
		min_x = 0.f;
		min_y = 0.f;
		min_z = 0.f;
		max_x = 0.f;
		max_y = 0.f;
		max_z = 0.f;
		return;
	}

	Objects[0]->ComputeAABB(min_x, min_y, min_z, max_x, max_y, max_z);
	for (std::size_t i = 1, n = Objects.size(); i < n; i++)
	{
		float min_x_i, min_y_i, min_z_i, max_x_i, max_y_i, max_z_i;
		Objects[i]->ComputeAABB(
				min_x_i, min_y_i, min_z_i, max_x_i, max_y_i, max_z_i);
		if (min_x_i < min_x) min_x = min_x_i;
		if (min_y_i < min_y) min_y = min_y_i;
		if (min_z_i < min_z) min_z = min_z_i;
		if (max_x_i > max_x) max_x = max_x_i;
		if (max_y_i > max_y) max_y = max_y_i;
		if (max_z_i > max_z) max_z = max_z_i;
	}
}

float Frame::ComputeRadius(glm::vec3 center) const
{
	float max_distance = 0.f;
	for (std::size_t i = 0, n = Objects.size(); i < n; i++)
	{
		float distance = Objects[i]->ComputeRadius(center);
		if (distance > max_distance)
			max_distance = distance;
	}
	for (std::size_t i = 0, n = Children.size(); i < n; i++)
	{
		float distance = Children[i]->ComputeRadius(center);
		if (distance > max_distance)
			max_distance = distance;
	}
	return max_distance;
}

glm::mat4 Frame::GetMatrix() const
{
	glm::mat4 modelMatrix(glm::scale(Scale));
	modelMatrix = glm::mat4_cast(Rotation) * modelMatrix;
	modelMatrix = glm::translate(Translation) * modelMatrix;
	return modelMatrix;
}
