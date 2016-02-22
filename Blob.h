#pragma once

#include "SoftBody.h"
#include <GLFW\glfw3.h>

#include <iostream>
#include <algorithm>
#include <sstream>

#include "Line.h"
#include "Helper.h"
#include "ShaderProgram.h"

typedef std::pair<int, btSoftBody::Node*> SoftBodyNode;

enum MovementMode { averaging, stretch };

class Blob : public SoftBody
{

private:

public:

	btVector3 forward;
	float speed = 2.2f;

	float invSize;

	MovementMode movementMode = MovementMode::stretch;

	Blob(btSoftBody* p_softBody)
		: SoftBody(p_softBody)
	{
		forward = btVector3(0, 0, 1);

		invSize = 1.0f / softbody->m_nodes.size();
	}

	~Blob()
	{

	}

	void Move(int key, int action)
	{
		if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT))
			AddForce(btVector3(0, 0, 1));
		if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT))
			AddForce(btVector3(0, 0, -1));
		if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT))
			AddForce(btVector3(1, 0, 0));
		if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT))
			AddForce(btVector3(-1, 0, 0));
		if (key == GLFW_KEY_SPACE && (action == GLFW_PRESS || action == GLFW_REPEAT))
			AddForce(btVector3(0, 1, 0));
	}

	void AddForce(const btVector3 &force)
	{
		if(force.length() > 0.0f)
			softbody->addForce(force * speed);
	}

	void AddForces(float magFwd, float magBack, float magLeft, float magRight)
	{
		btVector3 right = forward.cross(btVector3(0, 1, 0));

		btVector3 L = forward.rotate(btVector3(0, 1, 0), -glm::quarter_pi<float>()); //assuming that forward is always on XZ plane
		btVector3 R = forward.rotate(btVector3(0, 1, 0), glm::quarter_pi<float>());

		for (int i = 0; i < softbody->m_nodes.size(); i++)
		{
			btVector3 blobSpaceNode = softbody->m_nodes[i].m_x - GetCentroid();

			if (blobSpaceNode.dot(L) > 0)
				if (blobSpaceNode.dot(R) > 0)
					if(magLeft > 0)
						softbody->addForce(-right * magLeft * speed, i);
				else
					if(magFwd > 0)
						softbody->addForce(forward * magFwd * speed, i);
			else
				if (blobSpaceNode.dot(R) > 0)
					if(magBack > 0)
						softbody->addForce(-forward * magBack * speed, i);
				else
					if(magRight > 0)
						softbody->addForce(right * magRight * speed, i);
		}
	}

	btVector3 GetCentroid()
	{
		btVector3 centroid = btVector3(0, 0, 0);
		for (int i = 0; i < softbody->m_nodes.size(); i++)
			centroid += softbody->m_nodes[i].m_x;
		centroid *= invSize;

		return centroid;
	}

	void DrawGizmos(ShaderProgram* shaderProgram)
	{		
		glm::vec3 L = convert(&forward.rotate(btVector3(0, 1, 0), -glm::quarter_pi<float>()));
		glm::vec3 R = convert(&forward.rotate(btVector3(0, 1, 0), glm::quarter_pi<float>()));
		glm::vec3 BL = -R;
		glm::vec3 BR = -L;

		glm::vec3 c = convert(&GetCentroid());

		Line l(c, c + L * 10.0f);
		Line r(c, c + R * 10.0f);
		Line bl(c, c + BL * 10.0f);
		Line br(c, c + BR * 10.0f);
		Line fwd(c, c + convert(&forward) * 10.0f);

		l.Render();
		r.Render();
		bl.Render();
		br.Render();

		shaderProgram->SetUniform("uColor", glm::vec4(1, 0, 0, 1));
		fwd.Render();
	}
};