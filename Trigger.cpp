#include "Trigger.h"
#include "GameObject.h"

void Trigger::RegisterCallback(CallbackFunc callback, CallbackType type)
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

	bEnabled = true;
}

void Trigger::OnEnter()
{
	for (auto callback : onEnterCallbacks)
		callback();
	bTriggered = true;
}

void Trigger::OnStay()
{
	for (auto callback : onStayCallbacks)
		callback();
}

void Trigger::OnLeave()
{
	for (auto callback : onLeaveCallbacks)
		callback();
	bTriggered = false;
}

void Trigger::LinkToPlatform(GameObject* platform)
{
	RegisterCallback(
		[platform]() { platform->motion.Enabled = true; },
		CallbackType::Enter
		);
	RegisterCallback(
		[platform]() { platform->motion.Enabled = false; },
		CallbackType::Leave
		);

	auto ids = connectionIDs;
	if (!std::binary_search(ids.begin(), ids.end(),
		platform->ID))
		connectionIDs.push_back(platform->ID);
}