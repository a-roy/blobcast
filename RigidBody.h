#pragma once

#include <btBulletDynamicsCommon.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Mesh.h" 
#include <glm/gtc/type_ptr.hpp>

//enum RBShape { Box };

class RigidBody
{

private:
	glm::mat4 modelMatrix;

public:

	btRigidBody* rigidbody;
	Mesh* mesh;

	glm::vec3 translation;
	glm::quat orientation;
	glm::vec3 scale;

	float mass;

	RigidBody(Mesh* p_mesh, glm::vec3 p_translation, glm::quat p_orientation, glm::vec3 p_scale, float p_mass = 0)
		: mesh(p_mesh), translation(p_translation), orientation(p_orientation), scale(p_scale), mass(p_mass)
	{	
		//http://www.bulletphysics.org/mediawiki-1.5.8/index.php/Collision_Shapes
		btCollisionShape* shape;
		shape = new btBoxShape(btVector3(convert(scale * 0.5f))); //Half extents

		btDefaultMotionState* transform = new btDefaultMotionState(btTransform(btQuaternion(convert(p_orientation)), btVector3(convert(p_translation))));
		btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(mass, transform, shape);
		rigidbody = new btRigidBody(groundRigidBodyCI);
	}

	~RigidBody()
	{
		delete rigidbody;
		delete mesh;
	}

	glm::mat4 GetModelMatrix()
	{
		return
			glm::translate(glm::mat4(1.0f), translation)
			//* glm::toMat4(orientation)
			//* modelMatrix
			* glm::scale(glm::mat4(1.0f), scale);
	}

	void Update()
	{
		if (mass > 0)
		{
			btTransform trans;
			rigidbody->getMotionState()->getWorldTransform(trans);
			//trans.getOpenGLMatrix(glm::value_ptr(modelMatrix));

			//translation = convert(&trans.getOrigin());
			//orientation = convert(trans.getRotation());
		}
	}

	void Render()
	{
		mesh->Draw();
	}
};