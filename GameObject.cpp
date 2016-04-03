#include "GameObject.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include "config.h"

int GameObject::nextID = 0;

GameObject::GameObject(Mesh* p_mesh, Shape p_shapeType,
	glm::vec3 p_translation, glm::quat p_orientation, glm::vec3 p_scale,
	glm::vec4 p_color, GLuint p_texID, float p_mass, bool p_collidable,
	Path p_motion)
	: mesh(p_mesh), color(p_color), trueColor(p_color), textureID(p_texID),
	mass(p_mass)
{
	ID = nextID++;

	SetShape(p_translation, p_orientation, p_scale, p_shapeType);
	
	collisionFlagsDefault = rigidbody->getCollisionFlags();
	SetCollidable(p_collidable);

	if(mass != 0)
		rigidbody->setActivationState(DISABLE_DEACTIVATION);

	rigidbody->setFriction(RB_FRICTION);

	motion = p_motion;

	rigidbody->setUserPointer(this);
}

GameObject::~GameObject()
{
	delete rigidbody;
}

glm::mat4 GameObject::GetModelMatrix()
{
	return glm::translate(glm::mat4(1), GetTranslation())
		* glm::toMat4(GetOrientation())
		* glm::scale(glm::mat4(1), GetScale());
}

void GameObject::Render()
{
	if(drawable)
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

void GameObject::SetShape(Shape p_shapeType)
{
	glm::vec3 translation = GetTranslation();
	glm::quat orientation = GetOrientation();
	glm::vec3 scale = GetScale();

	//TODO: if not in dynamics world
		Physics::dynamicsWorld->removeRigidBody(rigidbody);
		delete rigidbody;
	
	SetShape(translation, orientation, scale,
		p_shapeType);
}

void GameObject::SetShape(glm::vec3 translation, glm::quat orientation,
	glm::vec3 scale, Shape p_shapeType)
{
	btCollisionShape* shape;
	if (p_shapeType == Shape::Box)
		shape = new btBoxShape(btVector3(1, 1, 1));
	else
		shape = new btCylinderShape(btVector3(1, 1, 1));
	shape->setLocalScaling(convert(scale));
	btDefaultMotionState* transform =
		new btDefaultMotionState(btTransform(
			btQuaternion(convert(orientation)),
			btVector3(convert(translation))));
	btVector3 inertia;
	shape->calculateLocalInertia(mass, inertia);

	btRigidBody::btRigidBodyConstructionInfo
		groundRigidBodyCI(mass, transform, shape, inertia);
	
	rigidbody = new btRigidBody(groundRigidBodyCI);
	rigidbody->setMassProps(mass, inertia);
	rigidbody->setUserPointer(this);
	Physics::dynamicsWorld->addRigidBody(rigidbody);
	
	shapeType = p_shapeType;
}
