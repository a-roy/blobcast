#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Camera.h"
#include "Frame.h"

class GLFWProject
{
	public:
		static GLFWwindow *Init(
				const char *title, int width = 0, int height = 0);
		static bool WASDStrafe(
				Camera *camera,
				GLFWwindow *window,
				int key, int scancode, int action, int mods);
		static bool MouseTurn(
				Camera *camera, double *xcursor, double *ycursor,
				GLFWwindow *window, double xpos, double ypos);
		static bool ClickDisablesCursor(
				double *xcursor, double *ycursor,
				GLFWwindow *window, int button, int action, int mods);
};
