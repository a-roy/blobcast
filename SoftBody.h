#pragma once

#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftBody.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletSoftBody/btDefaultSoftBodySolver.h>
#include <BulletSoftBody/btSoftBodyHelpers.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>

#include "Helper.h"
#include <GL/glew.h>
#include <vector>

class SoftBody
{
	private:

	public:

		GLuint vao;
		GLuint *VBOs;
		GLuint IBO;

		btSoftBody* softbody;

		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
		std::vector<unsigned int> indices;

		SoftBody(btSoftBody* p_softBody);
		~SoftBody();

		void Update();
		void Render();
};
