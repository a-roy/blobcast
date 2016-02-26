#pragma once

#include <GL/glew.h>
#include "VertexArray.h"
#include "Buffer.h"
#include "Drawable.h"
#include <vector>

class Mesh : public Drawable
{
	public:
		VertexArray *VAO;
		std::vector<FloatBuffer *> VBOs;
		ElementBuffer *IBO;

		int NumVerts;
		int NumTris;

		Mesh(VertexArray *vao, int vertexBuffers, int numVerts, int numFaces);
		Mesh(
				VertexArray *vao,
				std::vector<GLfloat> vbo,
				std::vector<GLfloat> nbo,
				std::vector<GLuint> ibo);
		~Mesh();
		void Draw() const;
		void SetVertexData(
				GLuint attribute, GLfloat *data, int itemSize);
		void SetIndexData(unsigned int *iboData);
		void ComputeAABB(
				float& min_x, float& min_y, float& min_z,
				float& max_x, float& max_y, float& max_z) const;
		float ComputeRadius(glm::vec3 center) const;
		static Mesh *CreateCube(VertexArray *vao);
		static Mesh *CreateTriplePlane(VertexArray *vao);
		static Mesh *CreateCubeWithNormals(VertexArray *vao);
};
