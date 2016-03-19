#pragma once

#include "Entity.h"
#include "Mesh.h"
#include "Physics.h"
#include <functional>

typedef std::function<void(void)> CallbackFunc;


class Button : public Entity
{

public:

	btGeneric6DofSpring2Constraint* constraint;
	std::vector<CallbackFunc> callbacks;

	Button(glm::vec3 p_translation, glm::quat p_orientation, 
		glm::vec3 p_scale, glm::vec4 p_color, float p_mass = 0) :
		Entity(Mesh::CreateCylinderWithNormals(), Shape::Cylinder,
			p_translation, p_orientation, p_scale, p_color, p_mass)
	{
		//http://bulletphysics.org/mediawiki-1.5.8/index.php/Constraints
		constraint = new btGeneric6DofSpring2Constraint(*rigidbody, 
			btTransform::getIdentity());
		constraint->setLinearLowerLimit(btVector3(0., 0., 0.));
		constraint->setLinearUpperLimit(btVector3(0., 0., 0.));
		constraint->setAngularLowerLimit(btVector3(0., 0., 0.));
		constraint->setAngularUpperLimit(btVector3(0., 0., 0.));
		
		//constraint->enableSpring(0, true);
		//constraint->setStiffness(0, 100);
		//constraint->getTranslationalLimitMotor()->m_enableMotor[0] = true;
		//constraint->getTranslationalLimitMotor()->m_targetVelocity[0] = -5.0f
		//constraint->setEquilibriumPoint(0, 0);

		Physics::dynamicsWorld->addConstraint(constraint);

		//rigidbody->setUserPointer(this);
	}

	Button(Button& p) :
		Button(p.GetTranslation(),
			p.GetOrientation(),
			p.GetScale(),
			p.trueColor,
			p.mass) {}

	~Button()
	{
		delete constraint;
	}

	void RegisterCallback(CallbackFunc callback)
	{
		callbacks.push_back(callback);
	}

	void DoCallbacks()
	{
		for (auto callback : callbacks)
			callback();
	}

	void Update(float deltaTime)
	{

	}
};
