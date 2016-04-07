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

	bool bTriggered = false;

	std::vector<CallbackFunc> onEnterCallbacks;
	std::vector<CallbackFunc> onStayCallbacks;
	std::vector<CallbackFunc> onLeaveCallbacks;

	bool bEnabled = false; //When component won't need this
	
	//For specific behaviours
	std::vector<int> connectionIDs;
	bool bDeadly = false;
	bool bLoopy = false;

	Trigger(){}
	~Trigger(){}

	void RegisterCallback(CallbackFunc callback, CallbackType type);

	void OnEnter();
	void OnStay();
	void OnLeave();

	//For specific behaviour
	void LinkToPlatform(GameObject* platform,
		GameObject* button, bool loading = false);
};
