#include "Path.h"

void Path::Reset()
{
	position = 0.0f;
}

bool Path::Step(float dt)
{
	if (Loop)
	{
		float path_length = (float)Points.size();
		position += Speed * dt;
		if (position > path_length)
			position = 0.0f;
	}
	else
	{
		float path_length = (float)(Points.size() - 1);
		if (position == path_length)
			return false;
		position += Speed * dt;
		if (position > path_length)
			position = path_length;
	}
	return true;
}

glm::vec3 Path::GetPosition()
{
	return GetPosition(position);
}

glm::vec3 Path::GetPosition(float time)
{
	int i0 = (int)time;
	int i1 = i0 + 1;
	if (i1 == Points.size())
		i1 = 0;
	glm::vec3 p0 = Points[i0];
	glm::vec3 p1 = Points[i1];
	glm::vec3 m0 = CatmullRomTangent(i0);
	glm::vec3 m1 = CatmullRomTangent(i1);
	float t = glm::fract(time);
	float t2 = t * t;
	float t3 = t2 * t;
	glm::vec3 pt =
		(1 - 3 * t2 + 2 * t3) * p0 +
		(3 * t2 - 2 * t3) * p1 +
		(t - 2 * t2 + t3) * m0 +
		(-t2 + t3) * m1;
	return pt;
}

glm::vec3 Path::CatmullRomTangent(int p)
{
	glm::vec3 prev =
		(p > 0) ? Points[p - 1] :
		(Loop ? Points.back() : Points[0]);
	glm::vec3 next =
		(p < Points.size() - 1) ? Points[p + 1] :
		(Loop ? Points.front() : Points.back());
	return next - prev;
}
