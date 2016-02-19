#pragma once

#include "SoftBody.h"
#include <GLFW\glfw3.h>

#include <iostream>
#include <algorithm>

typedef std::pair<int, btSoftBody::Node*> SoftBodyNode;

enum MoveMode { Average, Multi };

class Blob : public SoftBody
{

private:

	std::vector<SoftBodyNode> xAxis;
	std::vector<SoftBodyNode> zAxis;

	int halfSize;

public:

	MoveMode moveMode = MoveMode::Multi;
	float speed = 2.2f;

	Blob(btSoftBody* p_softBody)
		: SoftBody(p_softBody)
	{
		for (int i = 0; i < softbody->m_nodes.size(); i++)
		{
			xAxis.push_back(std::make_pair(i, &softbody->m_nodes[i]));
			zAxis.push_back(std::make_pair(i, &softbody->m_nodes[i]));
			//TODO - sort along arbitrary axes
		}

		halfSize = softbody->m_nodes.size() / 2;
	}

	~Blob()
	{

	}

	void Move(int key, int action)
	{
		if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT))
			AddForce(btVector3(0, 0, speed));
		if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT))
			AddForce(btVector3(0, 0, -speed));
		if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT))
			AddForce(btVector3(-speed, 0, 0));
		if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT))
			AddForce(btVector3(speed, 0, 0));
		if (key == GLFW_KEY_SPACE && (action == GLFW_PRESS || action == GLFW_REPEAT))
			AddForce(btVector3(0, speed, 0));

		//static int idx;
		//if (key == GLFW_KEY_I)
		//	idx++;
		//idx = idx % softbody->m_nodes.size();
		//if(key == GLFW_KEY_J)
		//	softbody->addForce(btVector3(0, 1000, 0), idx); //Their distribution is random
	
		/*std::sort(xAxis.begin(), xAxis.end(), NodeSortXfnc);
		for (std::pair<int, btSoftBody::Node*> n : xAxis)
			std::cout << n.second->m_x.getX() << "\n";*/
	}

	void AddForce(const btVector3 &force)
	{
		if (moveMode == MoveMode::Average)
		{
			softbody->addForce(force);
		}
		else if (moveMode == MoveMode::Multi)
		{
			if (force.dot(btVector3(0, 0, 1)) > 0)
			{
				std::sort(zAxis.begin(), zAxis.end(), NodeSortZfnc);

				for (int i = halfSize * 2 - 1; i >= halfSize; i--)
					softbody->addForce(force, zAxis[i].first);
			}
			else if (force.dot(btVector3(0, 0, -1)) > 0)
			{
				std::sort(zAxis.begin(), zAxis.end(), NodeSortZfnc);

				for (int i = 0; i < halfSize; i++)
					softbody->addForce(force, zAxis[i].first);
			}
			else if (force.dot(btVector3(1, 0, 0)) > 0)
			{
				std::sort(xAxis.begin(), xAxis.end(), NodeSortXfnc);

				for (int i = halfSize * 2 - 1; i >= halfSize; i--)
					softbody->addForce(force, xAxis[i].first);
			}
			else if (force.dot(btVector3(-1, 0, 0)) > 0)
			{
				std::sort(xAxis.begin(), xAxis.end(), NodeSortXfnc);

				for (int i = 0; i < halfSize; i++)
					softbody->addForce(force, xAxis[i].first);
			}
		}
	}

	/*void AddAnchor(btRigidBody* anchor)
	{
		softbody->appendAnchor(0, anchor, true, 1.0f);
	}*/

	static int NodeSortXfnc(const SoftBodyNode& a, const SoftBodyNode& b)
	{
		return a.second->m_x.getX() < b.second->m_x.getX();
	}

	static int NodeSortZfnc(const SoftBodyNode& a, const SoftBodyNode& b)
	{
		return a.second->m_x.getZ() < b.second->m_x.getZ();
	}
};