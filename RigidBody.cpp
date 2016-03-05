#include "RigidBody.h"
#include "Helper.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

RigidBody::RigidBody(Mesh* p_mesh, glm::vec3 p_translation, glm::quat p_orientation, glm::vec3 p_scale, glm::vec4 p_color, float p_mass)
	: mesh(p_mesh), translation(p_translation), orientation(p_orientation), scale(p_scale), color(p_color), mass(p_mass)
{
	scale *= 0.5f;

	//http://www.bulletphysics.org/mediawiki-1.5.8/index.php/Collision_Shapes
	btCollisionShape* shape;
	shape = new btBoxShape(btVector3(convert(scale)));

	btDefaultMotionState* transform = new btDefaultMotionState(btTransform(btQuaternion(convert(p_orientation)), btVector3(convert(p_translation))));
	btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(mass, transform, shape);
	rigidbody = new btRigidBody(groundRigidBodyCI);

	rigidbody->setUserPointer(this);
}

RigidBody::~RigidBody()
{
	delete rigidbody;
	delete mesh;
}

glm::mat4 RigidBody::GetModelMatrix()
{
	return glm::translate(glm::mat4(1), translation)
		* glm::toMat4(orientation)
		* glm::scale(glm::mat4(1), scale);
}

void RigidBody::Update()
{
	if (mass > 0)
	{
		btTransform trans;
		rigidbody->getMotionState()->getWorldTransform(trans);
		//trans.getOpenGLMatrix(glm::value_ptr(modelMatrix));
		translation = convert(&trans.getOrigin());
		orientation = convert(&trans.getRotation());
	}
}

void RigidBody::Render()
{
	mesh->Draw();
}
