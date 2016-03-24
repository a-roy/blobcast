#include "GameObject.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

int GameObject::nextID = 0;

GameObject::GameObject(Mesh* p_mesh, Shape p_shapeType,
	glm::vec3 p_translation, glm::quat p_orientation, glm::vec3 p_scale,
	glm::vec4 p_color, GLuint p_texID, float p_mass, bool p_collidable) 
	: mesh(p_mesh), color(p_color), trueColor(p_color), textureID(p_texID),
	mass(p_mass), collidable(p_collidable)
{
	//http://www.bulletphysics.org/mediawiki-1.5.8/index.php/Collision_Shapes
	btCollisionShape* shape;
	shapeType = p_shapeType;

	ID = nextID++;
	
	if(p_shapeType == Shape::Box)
		shape = new btBoxShape(btVector3(1,1,1));
	else 
		shape = new btCylinderShape(btVector3(1, 1, 1));

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

	//rigidbody->setRestitution(0.0);
	collisionFlagsDefault = rigidbody->getCollisionFlags();
	SetCollidable(collidable);

	const char* name = shape->getName();

	if(mass != 0)
		rigidbody->setActivationState(DISABLE_DEACTIVATION);
	//rigidbody->setMassProps(mass, inertia);

	rigidbody->setUserPointer(this);
}

GameObject::~GameObject()
{
	delete rigidbody;
	delete mesh;
}

glm::mat4 GameObject::GetModelMatrix()
{
	return glm::translate(glm::mat4(1), GetTranslation())
		* glm::toMat4(GetOrientation())
		* glm::scale(glm::mat4(1), GetScale());
}

void GameObject::Render()
{
	mesh->Draw();
}

void GameObject::Update(float deltaTime)
{
	if (!motion.Points.empty())
	{
		if (motion.Enabled)
			motion.Step();

		rigidbody->applyCentralForce(convert(
			Physics::InverseDynamics(GetTranslation(),
				motion.GetPosition(),
				convert(rigidbody->getLinearVelocity()),
				mass, deltaTime)));
	}
}
