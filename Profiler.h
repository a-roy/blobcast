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

	static void Start(std::string measurementName)
	{
		measurements[measurementName].ts = glfwGetTime();
	}

	static void Finish(std::string measurementName, bool frameAdvance = true,
		bool averaging = true)
	{
		if (!averaging)
		{
			measurements[measurementName].result = glfwGetTime() -
				measurements[measurementName].ts;
		}
		else
		{
			measurements[measurementName].deltas += glfwGetTime() -
				measurements[measurementName].ts;
			
			if(frameAdvance)
				measurements[measurementName].count++;
		}

		measurements[measurementName].avg = averaging;
	}

	static void Gui(std::string measurementName)
	{
		//const char* s = measurementName.c_str;
		ImGui::Text("%s average %.3f ms/frame | %.1f percent | %.1f FPS",
			measurementName.c_str(),
			Profiler::measurements[measurementName].result * 1000,
			(Profiler::measurements[measurementName].result
				/ Profiler::measurements["Frame"].result) * 100.0f,
			1.0f / Profiler::measurements[measurementName].result);
	}
};
