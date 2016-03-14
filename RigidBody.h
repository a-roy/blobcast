#pragma once

#include <btBulletDynamicsCommon.h>
#include "Mesh.h" 
#include <glm/gtc/type_ptr.hpp>
#include "Helper.h"

#include <glm/gtc/matrix_transform.hpp> 

//enum RBShape { Box }; //TODO - add more shapes support (as needed)

class RigidBody
{

private:

	Mesh* mesh;
	
public:

	btRigidBody* rigidbody;
	glm::vec4 color;
	glm::vec4 trueColor;
	float mass;

	RigidBody(Mesh* p_mesh, glm::vec3 p_translation, glm::quat p_orientation, 
		glm::vec3 p_scale, glm::vec4 p_color, float p_mass = 0);
	RigidBody(RigidBody& rb);
	~RigidBody();

	glm::mat4 GetModelMatrix();

	void Render();

	glm::quat GetOrientation() 
	{
		return convert(rigidbody->getOrientation());
	}
	glm::vec3 GetTranslation() 
	{
		return convert(rigidbody->getWorldTransform().getOrigin());
	}
	glm::vec3 GetScale()
	{
		return convert(rigidbody->getCollisionShape()->getLocalScaling());
	}

	//void SetMass(btSoftRigidDynamicsWorld* dynamicsWorld)
	//{
	//	//Remove from world to change mass
	//	dynamicsWorld->removeRigidBody(rigidbody);
	//	btVector3 inertia;
	//	first->rigidbody->getCollisionShape()->calculateLocalInertia(mass, inertia);
	//	first->rigidbody->setMassProps(mass, inertia);
	//	dynamicsWorld->addRigidBody(first->rigidbody);

	//	first->mass = mass;
	//}
};
