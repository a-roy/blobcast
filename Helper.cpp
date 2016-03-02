#include "Helper.h"

glm::vec3 convert(const btVector3& v)
{
	return glm::vec3(v.getX(), v.getY(), v.getZ());
}

btVector3 convert(glm::vec3 v)
{
	return btVector3(v.x, v.y, v.z);
}

btQuaternion convert(glm::quat q)
{
	return btQuaternion(q.x, q.y, q.z, q.w);
}

glm::quat convert(const btQuaternion& q)
{
	return glm::quat(q.getW(), q.getX(), q.getY(), q.getZ());
}
