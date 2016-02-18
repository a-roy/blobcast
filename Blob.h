#pragma once

#include "SoftBody.h"
#include <GLFW\glfw3.h>

class Blob : public SoftBody
{
private:

public:

	Blob(btSoftBody* p_softBody)
		: SoftBody(p_softBody)
	{

	}

	~Blob()
	{

	}

	void Move(int key, int action)
	{
		if(key == GLFW_KEY_UP && action == GLFW_PRESS)
			softbody->addForce(btVector3(0, 0, 10));
		if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
			softbody->addForce(btVector3(0, 0, -10));
		if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
			softbody->addForce(btVector3(-10, 0, 0));
		if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
			softbody->addForce(btVector3(10, 0, 0));
		if (key == GLFW_KEY_SPACE)
			softbody->addForce(btVector3(0, 10, 0));
	}

	void AddAnchor(btRigidBody* anchor)
	{
		softbody->appendAnchor(0, anchor, true, 1.0f);
	}
};