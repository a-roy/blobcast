#pragma once

#include <GL/glew.h>

class VertexArray
{
	public:
		GLuint Name;

		VertexArray();
		~VertexArray();
		VertexArray(const VertexArray&) = delete;
		VertexArray& operator=(const VertexArray&) = delete;
		VertexArray(VertexArray&& other);
		VertexArray& operator=(VertexArray&& other);
		void SetBufferData(
				GLuint buffer, GLenum target, size_t size, void *data,
				GLenum usage);
};
