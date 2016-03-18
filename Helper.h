#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <GL/glew.h>

glm::vec3 convert(const btVector3& v);
btVector3 convert(glm::vec3 v);
btQuaternion convert(glm::quat q);
glm::quat convert(const btQuaternion& q);

void ScreenPosToWorldRay(
	int mouseX, int mouseY,             
	int screenWidth, int screenHeight,  
	glm::mat4 ViewMatrix,               
	glm::mat4 ProjectionMatrix,         
	glm::vec3& out_origin,             
	glm::vec3& out_direction           
	);

GLuint loadTexture(const char* fileName, bool alpha);
GLfloat lerp(GLfloat a, GLfloat b, GLfloat f);
