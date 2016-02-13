#include "Mesh.h"

Mesh::Mesh(VertexArray *vao, int vertexBuffers, int numVerts, int numFaces) :
	VAO(vao), VBOs(vertexBuffers), NumVerts(numVerts), NumTris(numFaces) { }

Mesh::Mesh(
		VertexArray *vao,
		std::vector<GLfloat> vbo,
		std::vector<GLfloat> nbo,
		std::vector<GLuint> ibo) : Mesh(vao, 2, vbo.size() / 3, ibo.size() / 3)
{
	SetVertexData(0, &vbo[0], 3);
	SetVertexData(1, &nbo[0], 3);
	SetIndexData(&ibo[0]);
}

Mesh::~Mesh()
{
	for (std::size_t i = 0, n = VBOs.size(); i < n; i++)
	{
		delete VBOs[i];
	}
	delete IBO;
}

void Mesh::Draw() const
{
	for (std::size_t i = 0, n = VBOs.size(); i < n; i++)
	{
		glEnableVertexAttribArray(i);
		VBOs[i]->BufferData(i);
	}

	IBO->BufferData(-1);

	glDrawElements(
			GL_TRIANGLES,
			NumTris * 3,
			GL_UNSIGNED_INT,
			(void *)0
			);

	for (std::size_t i = 0, n = VBOs.size(); i < n; i++)
	{
		glDisableVertexAttribArray(i);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Mesh::SetVertexData(
		unsigned int attribute, GLfloat *data, int itemSize)
{
	VBOs[attribute] = new FloatBuffer(VAO, itemSize, NumVerts);
	VBOs[attribute]->SetData(data);
}

void Mesh::SetIndexData(unsigned int *iboData)
{
	IBO = new ElementBuffer(VAO, NumTris);
	IBO->SetData(iboData);
}

void Mesh::ComputeAABB(
		float& min_x, float& min_y, float& min_z,
		float& max_x, float& max_y, float& max_z) const
{
	min_x = VBOs[0]->Data[0];
	min_y = VBOs[0]->Data[1];
	min_z = VBOs[0]->Data[2];
	max_x = VBOs[0]->Data[0];
	max_y = VBOs[0]->Data[1];
	max_z = VBOs[0]->Data[2];
	for (int i = 1; i < NumVerts; i++)
	{
		float x = VBOs[0]->Data[i * 3];
		if (x < min_x) min_x = x;
		if (x > max_x) max_x = x;
		float y = VBOs[0]->Data[i * 3 + 1];
		if (y < min_y) min_y = y;
		if (y > max_y) max_y = y;
		float z = VBOs[0]->Data[i * 3 + 2];
		if (z < min_z) min_z = z;
		if (z > max_z) max_z = z;
	}
}

float Mesh::ComputeRadius(glm::vec3 center) const
{
	float max_square_distance = 0.f;
	for (int i = 0; i < NumVerts; i++)
	{
		float x = VBOs[0]->Data[i * 3] - center.x;
		float y = VBOs[0]->Data[i * 3 + 1] - center.y;
		float z = VBOs[0]->Data[i * 3 + 2] - center.z;
		float square_distance = x * x + y * y + z * z;
		if (square_distance > max_square_distance)
			max_square_distance = square_distance;
	}
	return glm::sqrt(max_square_distance);
}

Mesh *Mesh::CreateCube(VertexArray *vao)
{
	Mesh *cube = new Mesh(vao, 1, 8, 12);
	GLfloat vbo[] = {
		-1, -1, -1,   // 0
		-1, -1,  1,   // 1
		-1,  1, -1,   // 2
		-1,  1,  1,   // 3
		 1, -1, -1,   // 4
		 1, -1,  1,   // 5
		 1,  1, -1,   // 6
		 1,  1,  1 }; // 7
	cube->SetVertexData(0, vbo, 3);
	unsigned int ibo[] = {
		// front (-Z)
		0, 6, 2,
		6, 0, 4,
		// back (+Z)
		5, 3, 7,
		3, 5, 1,
		// left (-X)
		1, 2, 3,
		2, 1, 0,
		// right (+X)
		4, 7, 6,
		7, 4, 5,
		// top (+Y)
		2, 7, 3,
		7, 2, 6,
		// bottom (-Y)
		1, 4, 0,
		4, 1, 5 };
	cube->SetIndexData(ibo);

	return cube;
}

Mesh *Mesh::CreateTriplePlane(VertexArray *vao)
{
	Mesh *triple_plane = new Mesh(vao, 1, 12, 6);
	GLfloat vbo[] = {
		-1, -1,  0,
		 1, -1,  0,
		-1,  1,  0,
		 1,  1,  0,
		-1,  0, -1,
		-1,  0,  1,
		 1,  0, -1,
		 1,  0,  1,
		 0, -1, -1,
		 0,  1, -1,
		 0, -1,  1,
		 0,  1,  1 };
	triple_plane->SetVertexData(0, vbo, 3);
	unsigned int ibo[] = {
		 0,  3,  2,
		 3,  0,  1,
		 7,  4,  6,
		 4,  7,  5,
		11,  8, 10,
		 8, 11,  9 };
	triple_plane->SetIndexData(ibo);

	return triple_plane;
}
