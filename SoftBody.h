#pragma once

#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftBody.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletSoftBody/btDefaultSoftBodySolver.h>
#include <BulletSoftBody/btSoftBodyHelpers.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>

#include "Helper.h"

#include "Mesh.h" //TODO - use mesh class for drawing

class SoftBody
{
	private:

	public:

		GLuint vao;
		GLuint *VBOs;
		GLuint IBO;

		btSoftBody* softbody;

		std::vector<glm::vec3> vertices;
		std::vector<unsigned int> indices;

		SoftBody(btSoftBody* p_softBody)
		{
			softbody = p_softBody;

			for (int i = 0; i<softbody->m_faces.size(); ++i)
			{
				const btSoftBody::Face&	f = softbody->m_faces[i];
				const btSoftBody::Node *node = &softbody->m_nodes[0];
				indices.push_back(f.m_n[0] - node);
				indices.push_back(f.m_n[1] - node);
				indices.push_back(f.m_n[2] - node);
			}
			for (int i = 0, n = softbody->m_nodes.size(); i < n; i++)
			{
				vertices.push_back(convert(&softbody->m_nodes[i].m_x));
			}

			glGenVertexArrays(1, &vao);
			VBOs = new GLuint[1];
			glGenBuffers(1, VBOs);
			glGenBuffers(1, &IBO);

			glBindVertexArray(vao);

			glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STREAM_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		}

		~SoftBody()
		{
			delete softbody;
			delete[] VBOs;
		}

		void Update()
		{
			vertices.clear();
			for (int i = 0, n = softbody->m_nodes.size(); i < n; i++)
			{
				vertices.push_back(convert(&softbody->m_nodes[i].m_x));
			}

			glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STREAM_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		void Render()
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			glBindVertexArray(vao);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
			glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, (void *)0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glBindVertexArray(0);

			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
};
