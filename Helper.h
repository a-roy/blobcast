#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <GL\glew.h>

glm::vec3 convert(btVector3* v);
btVector3 convert(glm::vec3 v);
btQuaternion convert(glm::quat q);
glm::quat convert(btQuaternion* q);
GLfloat lerp(GLfloat a, GLfloat b, GLfloat f);

