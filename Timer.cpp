#include "Timer.h"

double Timer::currentFrame = 0;
double Timer::lastFrame = Timer::currentFrame;
double Timer::deltaTime;

void Timer::Update(double glfwTime)
{
	currentFrame = glfwTime;
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
}