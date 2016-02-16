#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

glm::vec3 convertBtVector3(btVector3* btv3)
{
	return glm::vec3(btv3->getX(), btv3->getY(), btv3->getZ());
}