#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

glm::vec3 convert(const btVector3& v);
btVector3 convert(glm::vec3 v);
btQuaternion convert(glm::quat q);
glm::quat convert(const btQuaternion& q);

