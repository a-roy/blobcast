#pragma once

#include <btBulletDynamicsCommon.h>

#include "Mesh.h" //TODO - use mesh class for drawing

class RigidBody
{

private:

public:

	btRigidBody* rigidbody;

	RigidBody(btRigidBody* p_rigidBody)
	{
		rigidbody = p_rigidBody;
	}

	~RigidBody()
	{
		delete rigidbody;
	}

	void Render()
	{
		//TODO
	}
};