#pragma once

#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>

#include <imgui.h>
#include "imgui_impl_glfw.h"

#include "glm\glm.hpp"

#include <sstream>
#include <iostream>

#include "RigidBody.h"

#include <typeinfo>
#include <set>

#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "ShaderProgram.h"



class LevelEditor
{

private:
	btSoftRigidDynamicsWorld *dynamicsWorld;

public:

	//Level *level;

	glm::vec3 out_origin = glm::vec3(0);
	glm::vec3 out_end = glm::vec3(0);

	std::set<RigidBody*> selection;

	LevelEditor(btSoftRigidDynamicsWorld *p_dynamicsWorld)
	{
		dynamicsWorld = p_dynamicsWorld;
	}

	~LevelEditor()
	{

	}

	void Gui(ShaderProgram *shaderProgram)
	{
		if (selection.size() > 0)
		{
			ImGui::SetNextWindowSize(ImVec2(300, 300), 
				ImGuiSetCond_FirstUseEver);

			ImGui::Begin("Selection");
			
			//Translation
			glm::vec3 centroid = glm::vec3(0);
			for (auto rb : selection)
				centroid += rb->GetTranslation();
			centroid /= selection.size();
			glm::vec3 before = centroid;
			if (ImGui::DragFloat3("Position", glm::value_ptr(centroid)))
			{
				btVector3 relTrans = convert(centroid - before);
				for (auto rb : selection)
				{
					rb->rigidbody->setWorldTransform(btTransform(
						rb->rigidbody->getOrientation(), 
						rb->rigidbody->getWorldTransform().getOrigin() 
						+ relTrans));
				}
			}
			                                                                             
			//Rotation
			RigidBody* first = *selection.begin();
			glm::vec3 euler = 
				glm::degrees(glm::eulerAngles(first->GetOrientation()));        
			float x = euler.x;
			float y = euler.y;
			float z = euler.z;
			bool set = false;
			btQuaternion qtn;
			if (ImGui::InputFloat("RotationX", &x, 15.0f, 15.0f))
			{
				euler = glm::radians(glm::vec3(x, euler.y, euler.z));
				set = true;
			}
			if (ImGui::InputFloat("RotationY", &y, 15.0f, 15.0f))
			{
				euler = glm::radians(glm::vec3(euler.x, y, euler.z));	
				set = true;
			}
			if (ImGui::InputFloat("RotationZ", &z, 15.0f, 15.0f))
			{
				euler = glm::radians(glm::vec3(euler.x, euler.y, z));
				set = true;
			}
			if (set)
			{
				qtn = btQuaternion(euler.y, euler.x, euler.z);
				for (auto rb : selection)
				{
					rb->rigidbody->setWorldTransform(btTransform(qtn,
						rb->rigidbody->getWorldTransform().getOrigin()));
					//rb->Update();
				}
			}

			btVector3 scale = 
				first->rigidbody->getCollisionShape()->getLocalScaling();
			glm::vec3 s = convert(scale);
			if (ImGui::DragFloat3("Scale", glm::value_ptr(s)))
			{
				first->rigidbody->
					getCollisionShape()->setLocalScaling(convert(s));
				//first->Update();
				//btVector3 relTrans = convert(centroid - before);
				//for (auto rb : selection)
				//{
					
					//rb->Update();
				//}
			}


			//Scale
			/*glm::vec3 centroid = glm::vec3(0);
			for (auto rb : selection)
				centroid += rb->translation;
			centroid /= selection.size();
			glm::vec3 before = centroid;*/
			//glm::vec3 scale = glm::vec3(0);

			//first->rigidbody->getCollisionShape()->

			//	btCollisionShape::setLocalScaling().
			//		//btCollisionWorld::updateSingleAABB(rigidbody)
			//if (ImGui::DragFloat3("Scale", glm::value_ptr(centroid)))
			//{
			//	btVector3 relTrans = convert(centroid - before);
			//	for (auto rb : selection)
			//	{
			//		rb->rigidbody->setWorldTransform(btTransform(
			//			rb->rigidbody->getOrientation(),
			//			rb->rigidbody->getWorldTransform().getOrigin()
			//			+ relTrans));
			//		rb->Update();
			//	}
			//}*/

			ImGui::End();
		}
	}
                                                                                
	void Mouse(double xcursor, double ycursor, int width, int height,
		glm::mat4 viewMatrix, glm::mat4 projectionMatrix)
	{
		//glm::vec3 out_origin;
		glm::vec3 out_direction;                                                

		ScreenPosToWorldRay((int)xcursor, (int)ycursor, width, height, 
			viewMatrix, projectionMatrix, out_origin, out_direction);

		/*glm::vec3*/ out_end = out_origin + out_direction * 1000.0f;

		btCollisionWorld::ClosestRayResultCallback 
			RayCallback(btVector3(out_origin.x, out_origin.y, out_origin.z), 
						btVector3(out_end.x, out_end.y, out_end.z));
		
		dynamicsWorld->rayTest(
			btVector3(out_origin.x, out_origin.y, out_origin.z), 
			btVector3(out_end.x, out_end.y, out_end.z), RayCallback);
		
		if (RayCallback.hasHit())
		{
			//TODO - Use Bullet collision masks
			if (typeid(*RayCallback.m_collisionObject) != typeid(btRigidBody))
				return; 
			//if(RayCallback.m_collisionObject->ma)

			RigidBody* newSelection = (RigidBody*)
				RayCallback.m_collisionObject->getUserPointer();
			newSelection->color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
			selection.insert(newSelection);
		}
		else
		{
			for (auto s : selection)
				s->color = s->trueColor;
			selection.clear();
		}
	}

	//https://github.com/opengl-tutorials/ogl/blob/master/misc05_picking/misc05_picking_BulletPhysics.cpp
	static void ScreenPosToWorldRay(
		int mouseX, int mouseY,             // Mouse position, in pixels, from bottom-left corner of the window
		int screenWidth, int screenHeight,  // Window size, in pixels
		glm::mat4 ViewMatrix,               // Camera position and orientation
		glm::mat4 ProjectionMatrix,         // Camera parameters (ratio, field of view, near and far planes)
		glm::vec3& out_origin,              // Ouput : Origin of the ray. /!\ Starts at the near plane, so if you want the ray to start at the camera's position instead, ignore this.
		glm::vec3& out_direction            // Ouput : Direction, in world space, of the ray that goes "through" the mouse.
		) {

		// The ray Start and End positions, in Normalized Device Coordinates (Have you read Tutorial 4 ?)
		glm::vec4 lRayStart_NDC(
			((float)mouseX / (float)screenWidth - 0.5f) * 2.0f, // [0,1024] -> [-1,1]
			((float)mouseY / (float)screenHeight - 0.5f) * 2.0f, // [0, 768] -> [-1,1]
			-1.0, // The near plane maps to Z=-1 in Normalized Device Coordinates
			1.0f
			);
		glm::vec4 lRayEnd_NDC(
			((float)mouseX / (float)screenWidth - 0.5f) * 2.0f,
			((float)mouseY / (float)screenHeight - 0.5f) * 2.0f,
			0.0,
			1.0f
			);


		// The Projection matrix goes from Camera Space to NDC.
		// So inverse(ProjectionMatrix) goes from NDC to Camera Space.
		glm::mat4 InverseProjectionMatrix = glm::inverse(ProjectionMatrix);

		// The View Matrix goes from World Space to Camera Space.
		// So inverse(ViewMatrix) goes from Camera Space to World Space.
		glm::mat4 InverseViewMatrix = glm::inverse(ViewMatrix);

		glm::vec4 lRayStart_camera = InverseProjectionMatrix * lRayStart_NDC;    lRayStart_camera /= lRayStart_camera.w;
		glm::vec4 lRayStart_world = InverseViewMatrix       * lRayStart_camera; lRayStart_world /= lRayStart_world.w;
		glm::vec4 lRayEnd_camera = InverseProjectionMatrix * lRayEnd_NDC;      lRayEnd_camera /= lRayEnd_camera.w;
		glm::vec4 lRayEnd_world = InverseViewMatrix       * lRayEnd_camera;   lRayEnd_world /= lRayEnd_world.w;


		// Faster way (just one inverse)
		//glm::mat4 M = glm::inverse(ProjectionMatrix * ViewMatrix);
		//glm::vec4 lRayStart_world = M * lRayStart_NDC; lRayStart_world/=lRayStart_world.w;
		//glm::vec4 lRayEnd_world   = M * lRayEnd_NDC  ; lRayEnd_world  /=lRayEnd_world.w;


		glm::vec3 lRayDir_world(lRayEnd_world - lRayStart_world);
		lRayDir_world = glm::normalize(lRayDir_world);


		out_origin = glm::vec3(lRayStart_world);
		out_direction = glm::normalize(lRayDir_world);
	}

private:
	void Translation()
	{
		glm::vec3 centroid = glm::vec3(0);
		for (auto rb : selection)
			centroid += rb->GetTranslation();
		centroid /= selection.size();
		glm::vec3 before = centroid;
		if (ImGui::DragFloat3("Position", glm::value_ptr(centroid)))
		{
			btVector3 relTrans = convert(centroid - before);
			for (auto rb : selection)
			{
				rb->rigidbody->setWorldTransform(btTransform(
					rb->rigidbody->getOrientation(),
					rb->rigidbody->getWorldTransform().getOrigin()
					+ relTrans));
			}

			dynamicsWorld->updateAabbs();
		}
	}

	void Rotation(ShaderProgram *shaderProgram)
	{
		for (auto rb : selection)
		{
			glm::vec3 xAxis(1.0f, 0.0f, 0.0f);
			xAxis = glm::toMat3(rb->GetOrientation()) * xAxis;
			shaderProgram->SetUniform("uColor", glm::vec4(1, 0, 0, 1));
			Line xAxisDraw(rb->GetTranslation() - (xAxis * ROTATION_GIZMO_SIZE),
				rb->GetTranslation() + (xAxis * ROTATION_GIZMO_SIZE));
			xAxisDraw.Render();

			glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
			yAxis = glm::toMat3(rb->GetOrientation()) * yAxis;
			shaderProgram->SetUniform("uColor", glm::vec4(0, 1, 0, 1));
			Line yAxisDraw(rb->GetTranslation() - (yAxis * ROTATION_GIZMO_SIZE),
				rb->GetTranslation() + (yAxis * ROTATION_GIZMO_SIZE));
			yAxisDraw.Render();

			glm::vec3 zAxis(0.0f, 0.0f, 1.0f);
			zAxis = glm::toMat3(rb->GetOrientation()) * zAxis;
			shaderProgram->SetUniform("uColor", glm::vec4(0, 0, 1, 1));
			Line zAxisDraw(rb->GetTranslation() - (zAxis * ROTATION_GIZMO_SIZE),
				rb->GetTranslation() + (zAxis * ROTATION_GIZMO_SIZE));
			zAxisDraw.Render();
		}

		float x = 0;
		if (ImGui::InputFloat("RotationX", &x, 15.0f, 15.0f))
		{
			for (auto rb : selection)
			{
				glm::quat qtn = glm::angleAxis(glm::radians(x),
					glm::toMat3(rb->GetOrientation()) 
					* glm::vec3(1.0f, 0.0f, 0.0f))
					* rb->GetOrientation();
				rb->rigidbody->setWorldTransform(btTransform(convert(qtn),
					rb->rigidbody->getWorldTransform().getOrigin()));
			}
		}
		
		float y = 0;
		if (ImGui::InputFloat("RotationY", &y, 15.0f, 15.0f))
		{
			for (auto rb : selection)
			{
				glm::quat qtn = glm::angleAxis(glm::radians(y),
					glm::toMat3(rb->GetOrientation())
					* glm::vec3(0.0f, 1.0f, 0.0f))
					* rb->GetOrientation();
				rb->rigidbody->setWorldTransform(btTransform(convert(qtn),
					rb->rigidbody->getWorldTransform().getOrigin()));
			}
		}

		float z = 0;
		if (ImGui::InputFloat("RotationZ", &z, 15.0f, 15.0f))
		{
			for (auto rb : selection)
			{
				glm::quat qtn = glm::angleAxis(glm::radians(z),
					glm::toMat3(rb->GetOrientation()) *
					glm::vec3(0.0f, 0.0f, 1.0f))
					* rb->GetOrientation();
				rb->rigidbody->setWorldTransform(btTransform(convert(qtn),
					rb->rigidbody->getWorldTransform().getOrigin()));
			}
		}
	}

	void Scale()
	{
		glm::vec3 centroid = glm::vec3(0);
		for (auto rb : selection)
			centroid += convert(rb->rigidbody->getCollisionShape()->
				getLocalScaling());
		centroid /= selection.size();
		glm::vec3 before = centroid;
		if (ImGui::InputFloat3("Scale", glm::value_ptr(centroid)))
		{
			btVector3 relScale = convert(centroid - before);
			for (auto rb : selection)
			{
				rb->rigidbody->
					getCollisionShape()->setLocalScaling(
						rb->rigidbody->getCollisionShape()->
						getLocalScaling()
						+ relScale);
				dynamicsWorld->updateSingleAabb(rb->rigidbody);
			}
		}
	}
};
