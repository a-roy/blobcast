#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Camera
{
	public:
		float MoveSpeed;
		float TurnSpeed;
		float RollSpeed;
		bool WalkForward = false;
		bool WalkBackward = false;
		bool StrafeLeft = false;
		bool StrafeRight = false;
		bool RollLeft = false;
		bool RollRight = false;
		glm::vec3 Position = glm::vec3(0.f, 0.f, -1.f);

		Camera(float moveSpeed, float turnSpeed, float rollSpeed) :
			MoveSpeed(moveSpeed), TurnSpeed(turnSpeed), RollSpeed(rollSpeed) { }
		virtual void Move() = 0;
		virtual void Turn(float x, float y) = 0;
		virtual glm::mat4 GetMatrix() const = 0;
};

class EulerCamera : public Camera
{
	public:
		float Yaw = 0.f;
		float Roll = 0.f;
		float Pitch = 0.f;

		EulerCamera(float moveSpeed, float turnSpeed, float rollSpeed) :
			Camera(moveSpeed, turnSpeed, rollSpeed) { }
		virtual void Move();
		virtual void Turn(float x, float y);
		virtual glm::mat4 EulerCamera::GetMatrix() const;
};

class QuaternionCamera : public Camera
{
	public:
		glm::quat Orientation;

		QuaternionCamera(float moveSpeed, float turnSpeed, float rollSpeed) :
			Camera(moveSpeed, turnSpeed, rollSpeed) { }
		virtual void Move();
		virtual void Turn(float x, float y);
		virtual glm::mat4 GetMatrix() const;
};
