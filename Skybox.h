#pragma once

#include <vector>

#include <SOIL/SOIL.h>
#include <sstream>
#include <iostream>
#include <GL/glew.h>


class Skybox {
protected:
	GLuint textureID;
	GLuint VAO, VBO;

public:
	void buildCubeMesh();
	void loadCubeMap(std::vector<const GLchar*> faces);
	GLuint getID();
	void render();
};
