#pragma once

#include "Entity.h"
#include "Mesh.h"
#include "Physics.h"
#include <functional>

typedef std::function<void(void)> CallbackFunc;

enum CallbackType { Enter, Stay, Leave };

class Trigger : public Entity
{

public:

	//btGeneric6DofSpring2Constraint* constraint;
	
	std::vector<CallbackFunc> onEnterCallbacks;
	std::vector<CallbackFunc> onStayCallbacks;
	std::vector<CallbackFunc> onLeaveCallbacks;

	bool bTriggered = false;

	Trigger(glm::vec3 p_translation, glm::quat p_orientation, 
		glm::vec3 p_scale) :
		Entity(NULL, Shape::Cylinder,
			p_translation, p_orientation, p_scale, glm::vec4(0), 0.0f)
	{
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
	}

	Trigger(Trigger& p) :
		Trigger(p.GetTranslation(),
			p.GetOrientation(),
			p.GetScale()) {}

	~Trigger()
	{
		//delete constraint;
	}

	void RegisterCallback(CallbackFunc callback, CallbackType type)
	{
		switch (type)
		{
			case CallbackType::Enter:
				onEnterCallbacks.push_back(callback);
				break;
			case CallbackType::Stay:
				onStayCallbacks.push_back(callback);
				break;
			case CallbackType::Leave:
				onLeaveCallbacks.push_back(callback);
		}
	}

	void OnEnter()
	{
		for (auto callback : onEnterCallbacks)
			callback();
		bTriggered = true;
	}

	void OnStay()
	{
		for (auto callback : onStayCallbacks)
			callback();
	}

	void OnLeave()
	{
		for (auto callback : onLeaveCallbacks)
			callback();
		bTriggered = false;
	}

	void Update(float deltaTime)
	{

	}

	void Render() override
	{
		//Don't render triggers
	}
};
