#pragma once

#include "VertexArray.h"
#include "Buffer.h"
#include "ShaderProgram.h"
#include "AggregateInput.h"
#include <glm/glm.hpp>

class BlobDisplay
{
	public:
		VertexArray VAO;
		FloatBuffer VBO;
		glm::mat4 MVPMatrix;
		float InnerRadius = 0.7f;
		float OuterRadius = 0.9f;

		BlobDisplay(int viewportWidth, int viewportHeight, int displaySize);
		void Render(const ShaderProgram& program, AggregateInput& inputs);
};
