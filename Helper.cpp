#include "Helper.h"
#include <SOIL\SOIL.h>
#include <iostream>

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

GLuint loadTexture(const char* fileName)
{
	int width, height;
	unsigned char* image;
	image = SOIL_load_image(fileName, &width, &height, 0, SOIL_LOAD_RGB);
	if (image == NULL) {
		std::cout << "Unable to open texture" << std::endl;
	}

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
		width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	SOIL_free_image_data(image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

	glBindTexture(GL_TEXTURE_2D, 0);
	return textureID;
}