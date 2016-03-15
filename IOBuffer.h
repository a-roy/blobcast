#pragma once
#include <GL/glew.h>
#include <iostream>

class IOBuffer
{
public:
	IOBuffer();
	~IOBuffer();

	bool Init(int Width, int Height, bool Depth, GLenum InternalType);

	GLuint FBO;
	GLuint texture0, texture1;
	
private:
	GLuint depthRenderBuffer;
	GLenum internalType;
};