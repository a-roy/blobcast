#include "Camera.h"
#include <glm/gtx/transform.hpp>

void EulerCamera::Move()
{
	glm::vec3 forward(glm::sin(Yaw), 0.f, -glm::cos(Yaw));
	glm::vec3 right  (glm::cos(Yaw), 0.f,  glm::sin(Yaw));
	if (MoveForward)  Position += MoveSpeed * forward;
	if (MoveBackward) Position -= MoveSpeed * forward;
	if (StrafeLeft)   Position -= MoveSpeed * right;
	if (StrafeRight)  Position += MoveSpeed * right;
}

void EulerCamera::Turn(float x, float y)
{
	Yaw += x * TurnSpeed;
	Pitch -= y * TurnSpeed;
	/*if (Pitch > glm::half_pi<float>())
		Pitch = glm::half_pi<float>();
	else if (Pitch < -glm::half_pi<float>())
		Pitch = -glm::half_pi<float>();*/
}

glm::mat4 EulerCamera::GetMatrix() const
{
	glm::mat4 translate(glm::translate(-Position));
	glm::mat4 yaw  (glm::rotate(-Yaw,   glm::vec3(0.f, -1.f,  0.f)));
	glm::mat4 pitch(glm::rotate(-Pitch, glm::vec3(1.f,  0.f,  0.f)));
	return pitch * yaw * translate;
}

void QuaternionCamera::Move()
{
	glm::vec3 forward(
			glm::mat3_cast(glm::inverse(Orientation)) *
			glm::vec3(0.f, 0.f, -1.f));
	glm::vec3 right(
			glm::mat3_cast(glm::inverse(Orientation)) *
			glm::vec3(1.f, 0.f,  0.f));

	if (MoveForward)  Position += MoveSpeed * forward;
	if (MoveBackward) Position -= MoveSpeed * forward;
	if (StrafeLeft)   Position -= MoveSpeed * right;
	if (StrafeRight)  Position += MoveSpeed * right;
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

void FlyCam::Move()
{
	glm::vec3 right = glm::normalize(glm::cross(Forward, Up));
	if (MoveForward)  Position += MoveSpeed * Forward;
	if (MoveBackward) Position -= MoveSpeed * Forward;
	if (StrafeLeft)   Position -= MoveSpeed * right;
	if (StrafeRight)  Position += MoveSpeed * right;
}

void FlyCam::Turn(float x, float y)
{
	EulerCamera::Turn(x, y);

	this->Forward = glm::normalize(glm::vec3(
		cos(glm::radians(Yaw)) * cos(glm::radians(Pitch)),
		sin(glm::radians(Pitch)),
		sin(glm::radians(Yaw)) * cos(glm::radians(Pitch))));
	glm::vec3 right = glm::normalize(glm::cross(Forward, glm::vec3(0, 1.0f, 0)));
	this->Up = glm::normalize(glm::cross(right, Forward));
}

glm::mat4 FlyCam::GetMatrix() const
{
	return glm::lookAt(Position, Position + Forward, Up);
}

