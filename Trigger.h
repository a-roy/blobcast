#pragma once

#include "Mesh.h"
#include "Physics.h"
#include <functional>
#include <algorithm>

typedef std::function<void(void)> CallbackFunc;

enum CallbackType { Enter, Stay, Leave };

class GameObject;

class Trigger //: public Component
{

public:

	//btGeneric6DofSpring2Constraint* constraint;

	bool bEnabled = false;
	
	std::vector<CallbackFunc> onEnterCallbacks;
	std::vector<CallbackFunc> onStayCallbacks;
	std::vector<CallbackFunc> onLeaveCallbacks;

	std::vector<int> connectionIDs;

	bool bTriggered = false;
	bool bDeadly = false;

	Trigger(){}
	~Trigger(){}

	void RegisterCallback(CallbackFunc callback, CallbackType type);

	void OnEnter();
	void OnStay();
	void OnLeave();

	void LinkToPlatform(GameObject* platform,
		GameObject* button);
};
