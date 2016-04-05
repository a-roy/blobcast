#pragma once

#include <btBulletDynamicsCommon.h>
#include "Mesh.h" 
#include "Path.h"
#include <glm/gtc/type_ptr.hpp>
#include "Helper.h"
#include "Trigger.h"

#include <glm/gtc/matrix_transform.hpp> 

enum Shape { /*None,*/ Box, Cylinder, SHAPE_NUMITEMS };

class GameObject
{

private:

	Mesh* mesh;
	
	bool collidable;
	int collisionFlagsDefault;

	float mass;

public:

	static int nextID;

	btRigidBody* rigidbody;
	
	bool drawable = true;
	glm::vec4 color;
	glm::vec4 trueColor;
	
	Shape shapeType;
	int ID;
	GLuint textureID;

	//Components
	Path motion;
	Trigger trigger;

	GameObject(Mesh* p_mesh, Shape p_shapeType, glm::vec3 p_translation,
		glm::quat p_orientation, glm::vec3 p_scale, glm::vec4 p_color, 
		GLuint p_texID, float p_mass = 0, bool bCollidable = true, 
		Path p_motion = Path());
	~GameObject();

	//Cloning doesn't copy trigger, for now
	GameObject(GameObject& p) :
		GameObject(p.mesh,
			p.shapeType,
			p.GetTranslation(),
			p.GetOrientation(),
			p.GetScale(),
			p.trueColor,
			p.textureID,
			p.mass,
			p.collidable,
			p.motion) {}

	glm::mat4 GetModelMatrix();

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

	bool GetCollidable() { return collidable; }
	void SetCollidable(bool set)
	{
		collidable = set;
		if (collidable)
			rigidbody->setCollisionFlags(
				rigidbody->getCollisionFlags() 
				&~ rigidbody->CF_NO_CONTACT_RESPONSE);
		else
			rigidbody->setCollisionFlags(
				rigidbody->getCollisionFlags()
				| rigidbody->CF_NO_CONTACT_RESPONSE);
	}

	void SetShape(Shape p_shapeType);
	void SetMesh(Mesh* p_mesh) { mesh = p_mesh; }

private:
	void SetShape(glm::vec3 translation, glm::quat orientation,
		glm::vec3 scale, Shape p_shapeType);

public:
	void Render();
	void Update(float deltaTime);

	float GetMass() { return mass; }

	void SetMass(float p_mass)
	{
		Physics::dynamicsWorld->removeRigidBody(rigidbody);
		btVector3 inertia;
		rigidbody->getCollisionShape()->
			calculateLocalInertia(p_mass, inertia);
		rigidbody->setMassProps(p_mass, inertia);
		Physics::dynamicsWorld->addRigidBody(rigidbody);
		mass = p_mass;
	}
};



