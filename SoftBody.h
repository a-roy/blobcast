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

		btSoftBody* softbody;

		std::vector<glm::vec3> vertices;

		SoftBody(btSoftBody* p_softBody)
		{
			softbody = p_softBody;

			softbody->m_materials[0]->m_kLST = 0.1;
			softbody->m_cfg.kDF = 1;
			softbody->m_cfg.kDP = 0.001;
			softbody->m_cfg.kPR = 2500;
			softbody->setTotalMass(30, true);
	
			for (int i = 0; i<softbody->m_faces.size(); ++i)
			{
				const btSoftBody::Face&	f = softbody->m_faces[i];
				vertices.push_back(convertBtVector3(&f.m_n[0]->m_x));
				vertices.push_back(convertBtVector3(&f.m_n[1]->m_x));
				vertices.push_back(convertBtVector3(&f.m_n[2]->m_x));
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

		~SoftBody()
		{
			delete softbody;
		}

		void Update()
		{
			vertices.clear();
			for (int i = 0; i<softbody->m_faces.size(); ++i)
			{
				const btSoftBody::Face&	f = softbody->m_faces[i];
				vertices.push_back(convertBtVector3(&f.m_n[0]->m_x));
				vertices.push_back(convertBtVector3(&f.m_n[1]->m_x));
				vertices.push_back(convertBtVector3(&f.m_n[2]->m_x));
			}

			glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STREAM_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		void Render()
		{
			glPolygonMode(GL_FRONT, GL_LINE);

			glBindVertexArray(vao);
			glDrawArrays(GL_TRIANGLES, 0, vertices.size());
			glBindVertexArray(0);

			glPolygonMode(GL_FRONT, GL_FILL);
		}
};