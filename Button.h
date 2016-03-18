#pragma once

#include "RigidBody.h"
#include "Mesh.h"
#include "Physics.h"

//http://bulletphysics.org/mediawiki-1.5.8/index.php/Constraints
class Button //: RigidBody .. perhaps then is a 'has a' relationship
{

public:

	RigidBody* button;
	btGeneric6DofSpring2Constraint* constraint;

	Button(glm::vec3 p_translation, glm::quat p_orientation, 
		glm::vec3 p_scale, glm::vec4 p_color, float p_mass = 0)
	{
		button = new RigidBody(Mesh::CreateCylinderWithNormals(),
			Shape::Cylinder, p_translation, p_orientation, p_scale,
			p_color, p_mass);

		constraint = new btGeneric6DofSpring2Constraint(*button->rigidbody, 
			btTransform::getIdentity());
		constraint->setLinearLowerLimit(btVector3(0., -1., 0.));
		constraint->setLinearUpperLimit(btVector3(0., 1., 0.));
		constraint->setAngularLowerLimit(btVector3(0., 0., 0.));
		constraint->setAngularUpperLimit(btVector3(0., 0., 0.));
		

		constraint->enableSpring(0, true);
		constraint->setStiffness(0, 100);

		//constraint->getTranslationalLimitMotor()->m_enableMotor[0] = true;
		//constraint->getTranslationalLimitMotor()->m_targetVelocity[0] = -5.0f;

		//constraint->setEquilibriumPoint(0, 0);

		//button->rigidbody->setGravity(btVector3(0, 10, 0));
		//button->rigidbody->set


		Physics::dynamicsWorld->addConstraint(constraint);
	}

	~Button()
	{
		delete button;
	}

	void Update()
	{
		
	}
};
