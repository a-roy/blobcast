#include "Profiler.h"

double Profiler::frameCounterTime = 0.0f;
std::map<std::string, Measurement> Profiler::measurements;

void Profiler::Start(std::string measurementName)
{
	measurements[measurementName].ts = glfwGetTime();
}

void Profiler::Finish(std::string measurementName, 
	bool frameAdvance, bool averaging)
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

		if (frameAdvance)
			measurements[measurementName].count++;
	}

	measurements[measurementName].avg = averaging;
}

void Profiler::Update(double deltaTime)
{
	frameCounterTime += deltaTime;
	if (frameCounterTime >= 1.0f)
	{
		for (auto itr = Profiler::measurements.begin();
		itr != Profiler::measurements.end(); itr++)
			if (itr->second.avg)
				itr->second.SetAvg();
		frameCounterTime = 0;
	}
}

void Profiler::Gui(std::string measurementName)
{
	//const char* s = measurementName.c_str;
	ImGui::Text("%s average %.3f ms/frame | %.1f percent | %.1f FPS",
		measurementName.c_str(),
		Profiler::measurements[measurementName].result * 1000,
		(Profiler::measurements[measurementName].result
			/ Profiler::measurements["Frame"].result) * 100.0f,
		1.0f / Profiler::measurements[measurementName].result);
}