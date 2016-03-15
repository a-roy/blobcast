#pragma once

#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>

#include <imgui.h>
#include "imgui_impl_glfw.h"

#include "glm/glm.hpp"

#include <sstream>
#include <iostream>

#include "RigidBody.h"

#include <typeinfo>
#include <set>

#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "ShaderProgram.h"

#include "Level.h"

class LevelEditor
{

private:
	btSoftRigidDynamicsWorld *dynamicsWorld;
	
public:

	Level* level;

	glm::vec3 out_origin = glm::vec3(0);
	glm::vec3 out_end = glm::vec3(0);
	std::set<RigidBody*> selection;
	bool bLocal = true;
	bool bCtrl = false;

	LevelEditor(btSoftRigidDynamicsWorld *p_dynamicsWorld,
		Level *p_level) :
		level(p_level), dynamicsWorld(p_dynamicsWorld){}
	~LevelEditor(){}

	void Gui(ShaderProgram *shaderProgram);                                                                             
	void Mouse(double xcursor, double ycursor, int width, int height,
		glm::mat4 viewMatrix, glm::mat4 projectionMatrix);
	static void ScreenPosToWorldRay(
		int mouseX, int mouseY,             
		int screenWidth, int screenHeight, 
		glm::mat4 ViewMatrix,               
		glm::mat4 ProjectionMatrix,         
		glm::vec3& out_origin,              
		glm::vec3& out_direction            
		);

	void DeleteSelection(); 
	void CloneSelection();

private:
	void Translation();
	void Rotation(ShaderProgram *shaderProgram);
	void LocalRotation(float angle, glm::vec3 axis);
	void GlobalRotation(float angle, glm::vec3 axis, glm::vec3 axisPosition);
	void Scale();

	void DrawRotationGizmo(glm::vec3 axis, glm::quat orientation,
		glm::vec3 translation, ShaderProgram *shaderProgram, glm::vec4 color);
};
