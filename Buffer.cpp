#include "Buffer.h"

FloatBuffer::FloatBuffer(
		VertexArray *vao, unsigned int itemSize, unsigned int numItems) :
	Buffer<GLfloat>(vao, GL_ARRAY_BUFFER, GL_STATIC_DRAW, itemSize, numItems)
{ }

void FloatBuffer::BufferData(GLuint attribute, size_t stride)
{
	Buffer<GLfloat>::BufferData(attribute);
	glVertexAttribPointer(
			attribute,
			ItemSize,
			GL_FLOAT,
			GL_FALSE,
			stride,
			nullptr);
}

ElementBuffer::ElementBuffer(VertexArray *vao, int numItems) :
	Buffer<unsigned int>(
			vao, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW, 3, numItems)
{ }
