#include "RigidBody.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

RigidBody::RigidBody(Mesh* p_mesh, glm::vec3 p_translation, 
	glm::quat p_orientation, glm::vec3 p_scale, glm::vec4 p_color, 
	float p_mass) : mesh(p_mesh), color(p_color), trueColor(p_color), 
	mass(p_mass)
{
	//http://www.bulletphysics.org/mediawiki-1.5.8/index.php/Collision_Shapes
	btCollisionShape* shape;
	shape = new btBoxShape(btVector3(1,1,1));
	shape->setLocalScaling(convert(p_scale));

	btDefaultMotionState* transform = 
		new btDefaultMotionState(btTransform(
							 btQuaternion(convert(p_orientation)), 
							 btVector3(convert(p_translation))));
	btRigidBody::btRigidBodyConstructionInfo 
		groundRigidBodyCI(mass, transform, shape);
	rigidbody = new btRigidBody(groundRigidBodyCI);

	if (mass != 0)
		rigidbody->setActivationState(DISABLE_DEACTIVATION);

	rigidbody->setUserPointer(this);
}

RigidBody::~RigidBody()
{
	delete rigidbody;
	delete mesh;
}

glm::mat4 RigidBody::GetModelMatrix()
{
	return glm::translate(glm::mat4(1), GetTranslation())
		* glm::toMat4(GetOrientation())
		* glm::scale(glm::mat4(1), GetScale());
}

void RigidBody::Render()
{
	mesh->Draw();
}
