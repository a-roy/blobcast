#include "VertexArray.h"

VertexArray::VertexArray()
{
	glGenVertexArrays(1, &Name);
}

VertexArray::~VertexArray()
{
	glDeleteVertexArrays(1, &Name);
}

void VertexArray::SetBufferData(
		GLuint buffer, GLenum target, size_t size, void *data, GLenum usage)
{
	glBindVertexArray(Name);
	glBindBuffer(target, buffer);
	glBufferData(target, size, data, usage);
	glBindVertexArray(0);
}
