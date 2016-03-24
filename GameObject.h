#pragma once

#include <btBulletDynamicsCommon.h>
#include "Mesh.h" 
#include "Path.h"
#include <glm/gtc/type_ptr.hpp>
#include "Helper.h"
#include "Trigger.h"

#include <glm/gtc/matrix_transform.hpp> 

enum Shape { None, Box, Cylinder, SHAPE_NUMITEMS };

class GameObject
{

private:

	Mesh* mesh;
	
	bool collidable;
	int collisionFlagsDefault;

public:

	static int nextID;

	btRigidBody* rigidbody;
	
	glm::vec4 color;
	glm::vec4 trueColor;
	float mass;
	Shape shapeType;
	int ID;
	GLuint textureID;

	//Components
	Path motion;
	Trigger trigger;

	GameObject(Mesh* p_mesh, Shape p_shapeType, glm::vec3 p_translation,
		glm::quat p_orientation, glm::vec3 p_scale, glm::vec4 p_color, 
		GLuint p_texID, float p_mass = 0, bool bCollidable = true);
	~GameObject();

	//Cloning doesn't copy path or trigger, for now
	GameObject(GameObject& p) :
		GameObject(Mesh::CreateCubeWithNormals(),
			p.shapeType,
			p.GetTranslation(),
			p.GetOrientation(),
			p.GetScale(),
			p.trueColor,
			p.textureID,
			p.mass,
			p.collidable) {}

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

	void SetCollidable(bool set)
	{
		collidable = set;
		if (collidable)
			rigidbody->setCollisionFlags(collisionFlagsDefault);
		else
			rigidbody->setCollisionFlags(collisionFlagsDefault
				| rigidbody->CF_NO_CONTACT_RESPONSE);
	}

	bool GetCollidable()
	{
		return collidable;
	}

	void Render();
	void Update(float deltaTime);

#pragma region constraint stuff
	//http://bulletphysics.org/mediawiki-1.5.8/index.php/Constraints
	/*constraint = new btGeneric6DofSpring2Constraint(*rigidbody,
	btTransform::getIdentity());
	constraint->setLinearLowerLimit(btVector3(0., 0., 0.));
	constraint->setLinearUpperLimit(btVector3(0., 0., 0.));
	constraint->setAngularLowerLimit(btVector3(0., 0., 0.));
	constraint->setAngularUpperLimit(btVector3(0., 0., 0.));*/
	//constraint->enableSpring(0, true);
	//constraint->setStiffness(0, 100);
	//constraint->getTranslationalLimitMotor()->m_enableMotor[0] = true;
	//constraint->getTranslationalLimitMotor()->m_targetVelocity[0] = -5.0f
	//constraint->setEquilibriumPoint(0, 0);
	//Physics::dynamicsWorld->addConstraint(constraint);	
#pragma endregion
};



