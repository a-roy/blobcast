#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

glm::vec3 convert(btVector3* v)
{
	return glm::vec3(v->getX(), v->getY(), v->getZ());
}

btVector3 convert(glm::vec3 v)
{
	return btVector3(v.x, v.y, v.z);
}

btQuaternion convert(glm::quat q)
{
	return btQuaternion(q.x, q.y, q.z, q.w);
}

