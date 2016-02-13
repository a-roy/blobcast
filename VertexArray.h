#pragma once

#include <GL/glew.h>

class VertexArray
{
	public:
		GLuint Name;

		VertexArray();
		~VertexArray();
		void SetBufferData(
				GLuint buffer, GLenum target, size_t size, void *data,
				GLenum usage);
};
