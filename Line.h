#pragma once

#include <GL/glew.h>
#include <vector>
#include <glm/glm.hpp>

class Line
{
private:
	GLuint vao;
	GLuint *VBOs;
	std::vector<glm::vec3> vertices;

public:

	Line(glm::vec3 from, glm::vec3 to) :
		Line({{ from, to }})
	{ }

	Line(const std::vector<glm::vec3>& verts) :
		vertices(verts)
	{
		glGenVertexArrays(1, &vao);
		VBOs = new GLuint[1];
		glGenBuffers(1, VBOs);

		glBindVertexArray(vao);

		glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
		glBufferData(
				GL_ARRAY_BUFFER,
				vertices.size() * sizeof(glm::vec3),
				&vertices[0],
				GL_STREAM_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	~Line()
	{
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, VBOs);
		delete[] VBOs;
	}

	Line(const Line&) = delete;
	Line& operator=(const Line&) = delete;

	Line(Line&& other) :
		vao(0), VBOs(new GLuint[1] { 0 })
	{
		*this = std::move(other);
	}

	Line& operator=(Line&& other)
	{
		if (this != &other)
		{
			glDeleteVertexArrays(1, &vao);
			glDeleteBuffers(1, VBOs);

			vao = other.vao;
			other.vao = 0;
			VBOs[0] = other.VBOs[0];
			other.VBOs[0] = 0;
			vertices = std::move(other.vertices);
		}
		return *this;
	}

	/*void Update(glm::vec3 from, glm::vec3 to)
	{
		vertices.clear();
		vertices.push_back(from);
		vertices.push_back(to);

		glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STREAM_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}*/

	void Render()
	{
		glBindVertexArray(vao);
		glDrawArrays(GL_LINE_STRIP, 0, vertices.size());
		glBindVertexArray(0);
	}
};
