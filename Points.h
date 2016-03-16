#pragma once

#include <GL/glew.h>
#include <vector>
#include <glm/glm.hpp>

class Points
{
private:
	GLuint vao;
	GLuint *VBOs;

public:

	std::vector<glm::vec3> points;
	std::vector<glm::vec3> colors;

	Points(glm::vec3 p, glm::vec3 c = { 0, 0, 0 }) :
		Points(std::vector<glm::vec3>(1, p), std::vector<glm::vec3>(1, c))
	{ }

	Points(std::vector<glm::vec3>&& p, std::vector<glm::vec3>&& c) :
		points(p), colors(c)
	{
		glGenVertexArrays(1, &vao);
		VBOs = new GLuint[2];
		glGenBuffers(2, VBOs);

		glBindVertexArray(vao);

		glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
		glBufferData(
				GL_ARRAY_BUFFER,
				points.size() * sizeof(glm::vec3),
				points.data(),
				GL_STREAM_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);
		glBufferData(
				GL_ARRAY_BUFFER,
				colors.size() * sizeof(glm::vec3),
				colors.data(),
				GL_STREAM_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	~Points()
	{
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(2, VBOs);

		vao = 0;
		delete[] VBOs;
	}

	Points(const Points&) = delete;
	Points& operator=(const Points&) = delete;

	Points(Points&& other) :
		vao(0), VBOs(new GLuint[2] { 0 })
	{
		*this = std::move(other);
	}

	Points& operator=(Points&& other)
	{
		if (this != &other)
		{
			glDeleteVertexArrays(1, &vao);
			glDeleteBuffers(2, VBOs);
			delete[] VBOs;

			vao = other.vao;
			other.vao = 0;
			VBOs = other.VBOs;
			for (int i = 0; i < 2; i++)
				other.VBOs[i] = 0;
			points = std::move(other.points);
			colors = std::move(other.colors);
		}
	}

	void Update(glm::vec3 p_p, int i)
	{
		points[i] = p_p;

		glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3), &points[i]);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void Render(float size)
	{
		glBindVertexArray(vao);

		glPointSize(size);
		glDrawArrays(GL_POINTS, 0, points.size());

		glBindVertexArray(0);
	}
};
