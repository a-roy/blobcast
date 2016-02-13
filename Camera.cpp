#include "Camera.h"
#include <glm/gtx/transform.hpp>

void EulerCamera::Move()
{
	glm::vec3 forward(glm::sin(Yaw), 0.f, -glm::cos(Yaw));
	glm::vec3 right  (glm::cos(Yaw), 0.f,  glm::sin(Yaw));
	if (WalkForward)  Position += MoveSpeed * forward;
	if (WalkBackward) Position -= MoveSpeed * forward;
	if (StrafeLeft)   Position -= MoveSpeed * right;
	if (StrafeRight)  Position += MoveSpeed * right;
	if (RollLeft)
	{
		Roll -= RollSpeed;
		if (Roll < -glm::half_pi<float>())
			Roll = -glm::half_pi<float>();
	}
	if (RollRight)
	{
		Roll += RollSpeed;
		if (Roll > glm::half_pi<float>())
			Roll = glm::half_pi<float>();
	}
}

void EulerCamera::Turn(float x, float y)
{
	Yaw += x * TurnSpeed;
	Pitch -= y * TurnSpeed;
	if (Pitch > glm::half_pi<float>())
		Pitch = glm::half_pi<float>();
	else if (Pitch < -glm::half_pi<float>())
		Pitch = -glm::half_pi<float>();
}

glm::mat4 EulerCamera::GetMatrix() const
{
	glm::mat4 translate(glm::translate(-Position));
	glm::mat4 yaw  (glm::rotate(-Yaw,   glm::vec3(0.f, -1.f,  0.f)));
	glm::mat4 roll (glm::rotate(-Roll,  glm::vec3(0.f,  0.f, -1.f)));
	glm::mat4 pitch(glm::rotate(-Pitch, glm::vec3(1.f,  0.f,  0.f)));
	return pitch * roll * yaw * translate;
}

void QuaternionCamera::Move()
{
	glm::vec3 forward(
			glm::mat3_cast(glm::inverse(Orientation)) *
			glm::vec3(0.f, 0.f, -1.f));
	glm::vec3 right(
			glm::mat3_cast(glm::inverse(Orientation)) *
			glm::vec3(1.f, 0.f,  0.f));
	if (WalkForward)  Position += MoveSpeed * forward;
	if (WalkBackward) Position -= MoveSpeed * forward;
	if (StrafeLeft)   Position -= MoveSpeed * right;
	if (StrafeRight)  Position += MoveSpeed * right;
	if (RollLeft)
		Orientation =
			glm::angleAxis(-RollSpeed, glm::vec3(0.f, 0.f, 1.f)) * Orientation;
	if (RollRight)
		Orientation =
			glm::angleAxis( RollSpeed, glm::vec3(0.f, 0.f, 1.f)) * Orientation;
}

void QuaternionCamera::Turn(float x, float y)
{
	Orientation =
		glm::angleAxis(x * TurnSpeed, glm::vec3(0.f, 1.f, 0.f)) * Orientation;
	Orientation =
		glm::angleAxis(y * TurnSpeed, glm::vec3(1.f, 0.f, 0.f)) * Orientation;
}

glm::mat4 QuaternionCamera::GetMatrix() const
{
	glm::mat4 translate(glm::translate(-Position));
	glm::mat4 rotate(glm::mat4_cast(Orientation));
	return rotate * translate;
}
