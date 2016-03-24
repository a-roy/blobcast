#pragma once

#include <map>
#include <GLFW/glfw3.h>
#include <imgui.h>

struct Measurement
{
	double ts = 0.0f;
	double result = 0.0f;
	
	double deltas = 0.0f;
	int count = 0;
	bool avg = false;

	void SetAvg()
	{
		result = deltas / count;
		deltas = 0.0f;
		count = 0;	
	}
};

class Profiler
{

public:

	static std::map<std::string, Measurement> measurements;
	static double frameCounterTime;

	static void Start(std::string measurementName);
	static void Finish(std::string measurementName, bool frameAdvance = true,
		bool averaging = true);

	static void Update(double deltaTime);
	static void Gui(std::string measurementName);
};
