#pragma once
#include <vector>
#include <SOIL/SOIL.h>
#include <sstream>
#include <iostream>
#include <GL/glew.h>
#include <glm\glm.hpp>


class Skybox
{
public:
	Skybox();
	void buildCubeMesh();
	bool loadCubeMap(std::vector<const GLchar*> faces);
	GLuint getID();
	void render();
	glm::mat4 modelMat;

protected:
	GLuint textureID;
	GLuint VAO, VBO;
};
