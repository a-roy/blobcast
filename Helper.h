#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <GL/glew.h>

glm::vec3 convert(const btVector3& v);
btVector3 convert(glm::vec3 v);
btQuaternion convert(glm::quat q);
glm::quat convert(const btQuaternion& q);

GLuint loadTexture(const char* fileName, bool alpha);
GLfloat lerp(GLfloat a, GLfloat b, GLfloat f);
