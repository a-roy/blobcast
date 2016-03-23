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
	std::vector<glm::vec3> colors;

public:

	Line(glm::vec3 from, glm::vec3 to, glm::vec3 color = { 0, 0, 0 }) :
		Line(std::vector<glm::vec3>({{ from, to }}), color)
	{ }

	Line(const std::vector<glm::vec3>& verts, glm::vec3 color = { 0, 0, 0 }) :
		vertices(verts), colors(verts.size(), color)
	{
		glGenVertexArrays(1, &vao);
		VBOs = new GLuint[2];
		glGenBuffers(2, VBOs);

		glBindVertexArray(vao);

		glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
		glBufferData(
				GL_ARRAY_BUFFER,
				vertices.size() * sizeof(glm::vec3),
				&vertices[0],
				GL_STREAM_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);
		glBufferData(
				GL_ARRAY_BUFFER,
				colors.size() * sizeof(glm::vec3),
				&colors[0],
				GL_STREAM_DRAW);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	~Line()
	{
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(2, VBOs);
		delete[] VBOs;
	}

	Line(const Line&) = delete;
	Line& operator=(const Line&) = delete;

	Line(Line&& other) :
		vao(0), VBOs(new GLuint[2] { 0 })
	{
		*this = std::move(other);
	}

	Line& operator=(Line&& other)
	{
		if (this != &other)
		{
			glDeleteVertexArrays(1, &vao);
			glDeleteBuffers(2, VBOs);

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
