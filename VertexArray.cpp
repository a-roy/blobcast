#include "VertexArray.h"
#include <utility>

VertexArray::VertexArray()
{
	glGenVertexArrays(1, &Name);
}

VertexArray::~VertexArray()
{
	glDeleteVertexArrays(1, &Name);
}

VertexArray::VertexArray(VertexArray&& other) :
	Name(0)
{
	*this = std::move(other);
}

VertexArray& VertexArray::operator=(VertexArray&& other)
{
	if (this != &other)
	{
		glDeleteVertexArrays(1, &Name);
		Name = other.Name;
		other.Name = 0;
	}
	return *this;
}

void VertexArray::SetBufferData(
		GLuint buffer, GLenum target, size_t size, void *data, GLenum usage)
{
	Bind([&](){
		glBindBuffer(target, buffer);
		glBufferData(target, size, data, usage);
	});
}
