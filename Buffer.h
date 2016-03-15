#pragma once

#include <GL/glew.h>
#include "VertexArray.h"
#include <cstring>

template <class T>
class Buffer
{
	public:
		GLuint Name;
		VertexArray *VAO;
		GLenum Target;
		GLenum Usage;
		unsigned int ItemSize;
		unsigned int NumItems;
		T *Data;

		Buffer(
				VertexArray *vao,
				GLenum target,
				GLenum usage,
				unsigned int itemSize,
				unsigned int numItems) :
			VAO(vao),
			Target(target),
			Usage(usage),
			ItemSize(itemSize),
			NumItems(numItems),
			Data(NULL)
		{
			glGenBuffers(1, &Name);
		}

		~Buffer()
		{
			glDeleteBuffers(1, &Name);
			if (managed_data)
				delete[] Data;
		}

		virtual void SetData(T *data, bool copy = true)
		{
			if (managed_data)
			{
				managed_data = false;
				delete[] Data;
				Data = nullptr;
			}
			if (copy && data != nullptr)
			{
				Data = new T[NumItems * ItemSize];
				memcpy(Data, data, sizeof(T) * ItemSize * NumItems);
				managed_data = true;
			}
			VAO->SetBufferData(
					Name,
					Target,
					sizeof(T) * ItemSize * NumItems,
					Data,
					Usage);
		}

		virtual void BufferData(GLuint attribute, size_t stride = 0)
		{
			glBindBuffer(Target, Name);
		}

	private:
		bool managed_data = false;
};

class FloatBuffer : public Buffer<GLfloat>
{
	public:
		FloatBuffer(
				VertexArray *vao,
				unsigned int itemSize,
				unsigned int numItems);

		void BufferData(GLuint attribute, size_t stride = 0);
};

class ElementBuffer : public Buffer<unsigned int>
{
	public:
		ElementBuffer(VertexArray *vao, int numItems);
};
