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

	btVector3 inertia; 
	shape->calculateLocalInertia(mass, inertia);
	btRigidBody::btRigidBodyConstructionInfo 
		groundRigidBodyCI(mass, transform, shape, inertia);
	rigidbody = new btRigidBody(groundRigidBodyCI);

	//if(mass != 0)
		//rigidbody->setActivationState(DISABLE_DEACTIVATION);
	//rigidbody->setMassProps(mass, inertia);

	rigidbody->setUserPointer(this);
}

RigidBody::RigidBody(RigidBody& rb) :
	RigidBody(Mesh::CreateCubeWithNormals(),
		rb.GetTranslation(),
		rb.GetOrientation(),
		rb.GetScale(),
		rb.trueColor,
		rb.mass){}

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

void RigidBody::Update()
{
	if (!path_points.empty() && path_speed != 0.0f &&
			path_position < (float)(path_points.size() - 1))
	{
		glm::vec3 p0(path_points  [(int)path_position]);
		glm::vec3 m0(path_tangents[(int)path_position]);
		glm::vec3 p1(path_points  [(int)(path_position + 1.0f)]);
		glm::vec3 m1(path_tangents[(int)(path_position + 1.0f)]);
		float t = path_position - glm::floor(path_position);
		float t2 = t * t;
		float t3 = t2 * t;
		glm::vec3 pt =
			(1 - 3 * t2 + 2 * t3) * p0 +
			(3 * t2 - 2 * t3) * p1 +
			(t - 2 * t2 + t3) * m0 +
			(-t2 + t3) * m1;
		rigidbody->translate(convert(pt - GetTranslation()));
		path_position += path_speed;
	}
}
