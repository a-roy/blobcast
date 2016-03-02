#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Camera
{
	public:
		float MoveSpeed;
		float TurnSpeed;

		bool MoveForward = false;
		bool MoveBackward = false;
		bool StrafeLeft = false;
		bool StrafeRight = false;

		glm::vec3 Position; 

		Camera(glm::vec3 position, float moveSpeed, float turnSpeed) :
			Position(position), MoveSpeed(moveSpeed), TurnSpeed(turnSpeed) { }
		
		virtual void Move() = 0;
		virtual void Turn(float x, float y) = 0;

		virtual glm::mat4 GetMatrix() const = 0;

		virtual void Update() { Move(); }
};

class EulerCamera : public Camera
{
	public:
		float Yaw = 0.f;
		float Pitch = 0.f;

		EulerCamera(glm::vec3 position, float moveSpeed, float turnSpeed) :
			Camera(position, moveSpeed, turnSpeed) { }
		
		virtual void Move();
		virtual void Turn(float x, float y);
		virtual glm::mat4 GetMatrix() const;
};

class QuaternionCamera : public Camera
{
	public:
		glm::quat Orientation;

		QuaternionCamera(glm::vec3 position, float moveSpeed, float turnSpeed) :
			Camera(position, moveSpeed, turnSpeed) { }
		
		virtual void Move();
		virtual void Turn(float x, float y);
		virtual glm::mat4 GetMatrix() const;
};

class FlyCam : public EulerCamera
{
	public:
		glm::vec3 Forward = glm::vec3(0,0,1);
		glm::vec3 Up = glm::vec3(0,1,0);

		FlyCam(glm::vec3 position, float moveSpeed, float turnSpeed) :
			EulerCamera(position, moveSpeed, turnSpeed) { }

		virtual void Move();
		virtual void Turn(float x, float y);
		virtual glm::mat4 GetMatrix() const;
};

//TODO
class ThirdPersonCam : public Camera
{
	public:

};

//TODO
class DynamicCam : public ThirdPersonCam
{
	public:

};
