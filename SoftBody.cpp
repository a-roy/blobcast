#include "SoftBody.h"
#include "Helper.h"

SoftBody::SoftBody(btSoftBody* p_softBody)
{
	softbody = p_softBody;

	for (int i = 0; i<softbody->m_faces.size(); ++i)
	{
		const btSoftBody::Face&	f = softbody->m_faces[i];
		vertices.push_back(convert(&f.m_n[0]->m_x));
		vertices.push_back(convert(&f.m_n[1]->m_x));
		vertices.push_back(convert(&f.m_n[2]->m_x));
	}

	glGenVertexArrays(1, &vao);
	VBOs = new GLuint[1];
	glGenBuffers(1, VBOs);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STREAM_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

SoftBody::~SoftBody()
{
	delete softbody;
}

void SoftBody::Update()
{
	vertices.clear();
	for (int i = 0; i<softbody->m_faces.size(); ++i)
	{
		const btSoftBody::Face&	f = softbody->m_faces[i];
		vertices.push_back(convert(&f.m_n[0]->m_x));
		vertices.push_back(convert(&f.m_n[1]->m_x));
		vertices.push_back(convert(&f.m_n[2]->m_x));
	}

	glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void SoftBody::Render()
{
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
	glBindVertexArray(0);
}