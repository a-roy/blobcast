#pragma once
#include <GL/glew.h>
#include <iostream>

class IOBuffer
{
public:
	IOBuffer();
	~IOBuffer();

	bool Init(int Width, int Height, bool Depth, GLenum InternalType);
	bool InitForCubeMap(int Width, int Height);
	void BindForWriting();
	void BindForReading(GLenum TextureUnit);

private:
	GLuint FBO;
	GLuint texture;
	GLuint depth;
	GLenum internalType;
};