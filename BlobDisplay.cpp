#include "BlobDisplay.h"
#include <glm/gtc/matrix_transform.hpp>

BlobDisplay::BlobDisplay(int viewportWidth, int viewportHeight, int displaySize)
{
	VAO = new VertexArray();
	VBO = new FloatBuffer(VAO, 2, 4);
	GLfloat *vertex_data = new GLfloat[8] { -1, -1, -1, 1, 1, -1, 1, 1 };
	VBO->SetData(vertex_data);
	glBindVertexArray(VAO->Name);
	VBO->BufferData(0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
	MVPMatrix = glm::ortho(
			-1.25f, ((float)viewportWidth / (float)displaySize) - 1.25f,
			-1.25f, ((float)viewportHeight / (float)displaySize) - 1.25f);
}

BlobDisplay::~BlobDisplay()
{
	delete VAO;
	delete VBO;
}

void BlobDisplay::Render(const ShaderProgram& program, AggregateInput& inputs)
{
	float total = (float)inputs.TotalCount;
	if (total == 0.0f)
		total = 1.0f;
	program["uFMagnitude"] = (float)inputs.FCount / total;
	program["uBMagnitude"] = (float)inputs.BCount / total;
	program["uRMagnitude"] = (float)inputs.RCount / total;
	program["uLMagnitude"] = (float)inputs.LCount / total;
	program["uFRMagnitude"] = (float)inputs.FRCount / total;
	program["uFLMagnitude"] = (float)inputs.FLCount / total;
	program["uBRMagnitude"] = (float)inputs.BRCount / total;
	program["uBLMagnitude"] = (float)inputs.BLCount / total;
	program["uMVPMatrix"] = MVPMatrix;
	program["uInnerRadius"] = InnerRadius;
	program["uOuterRadius"] = OuterRadius;
	program.Use([&](){
		glBindVertexArray(VAO->Name);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
	});
}
