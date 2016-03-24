#pragma once
class Timer
{
public:
	static double currentFrame;
	static double lastFrame;
	static double deltaTime;

	static void Update(double glfwTime);
};