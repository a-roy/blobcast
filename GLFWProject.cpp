#include "GLFWProject.h"
#include <glm/glm.hpp>

GLFWwindow *GLFWProject::Init(const char *title, int width, int height)
{
	if (!glfwInit())
		return NULL;

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	GLFWwindow *window;
	if (width * height == 0)
	{
		GLFWmonitor *monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode *mode = glfwGetVideoMode(monitor);
		window = glfwCreateWindow(mode->width, mode->height, title, monitor, NULL);
	}
	else
	{
		window = glfwCreateWindow(width, height, title, NULL, NULL);
	}
	if (!window)
	{
		glfwTerminate();
		return NULL;
	}

	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		glfwTerminate();
		return NULL;
	}

	return window;
}

bool GLFWProject::WASDStrafe(
		Camera *camera,
		GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_W)
	{
		if      (action == GLFW_PRESS)   camera->MoveForward = true;
		else if (action == GLFW_RELEASE) camera->MoveForward = false;
		else return false;
	}
	else if (key == GLFW_KEY_S)
	{
		if      (action == GLFW_PRESS)   camera->MoveBackward = true;
		else if (action == GLFW_RELEASE) camera->MoveBackward = false;
		else return false;
	}
	else if (key == GLFW_KEY_A)
	{
		if      (action == GLFW_PRESS)   camera->StrafeLeft = true;
		else if (action == GLFW_RELEASE) camera->StrafeLeft = false;
		else return false;
	}
	else if (key == GLFW_KEY_D)
	{
		if      (action == GLFW_PRESS)   camera->StrafeRight = true;
		else if (action == GLFW_RELEASE) camera->StrafeRight = false;
		else return false;
	}
	else return false;
	return true;
}

bool GLFWProject::MouseTurn(
		Camera *camera, double *xcursor, double *ycursor,
		GLFWwindow *window, double xpos, double ypos)
{
	if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
		camera->Turn((float)(xpos - *xcursor), (float)(ypos - *ycursor));
	else
		return false;
	return true;
}

bool GLFWProject::ClickDisablesCursor(
		double *xcursor, double *ycursor,
		GLFWwindow *window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			glfwGetCursorPos(window, xcursor, ycursor);
		}
		else
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}
	else return false;
	return true;
}
