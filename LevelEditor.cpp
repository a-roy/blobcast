#include "LevelEditor.h"
#include "config.h"
#include "Line.h"
#include "Points.h"
#include <sstream>

void LevelEditor::Gui(ShaderProgram *shaderProgram)
{
	if (selection.size() > 0)
	{
		ImGui::SetNextWindowSize(ImVec2(300, 400),
			ImGuiSetCond_FirstUseEver);

		std::stringstream ss;
		ss << "Selection (" << selection.size() << " objects)###Selection";
		ImGui::Begin(ss.str().c_str());

		if (ImGui::CollapsingHeader("Translation"))
			Translation();
		ImGui::Spacing(); 
		
		if (ImGui::CollapsingHeader("Rotation"))
			Rotation(shaderProgram);	
		ImGui::Spacing(); 

		if (ImGui::CollapsingHeader("Scale"))
		{
			Scale();
			ImGui::Separator();
		}
		ImGui::Spacing();
		if (ImGui::Button("Clone selection"))
			CloneSelection(); ImGui::SameLine();
		if (ImGui::Button("Delete selection"))
			DeleteSelection();

		if (selection.size() >= 2)
		{
			//if (ImGui::Button("Create compound object")) //TODO
			//{
				/*btCompoundShape* compoundShape = new btCompoundShape();
				for (auto rb : selection)
				{
					btBoxShape* boxShape = new btBoxShape(btVector3(1, 1, 1));
					boxShape->setLocalScaling(convert(rb->GetScale()));
					compoundShape->addChildShape(
						rb->rigidbody->getWorldTransform(),
						boxShape);
				}*/
			//}
		}

		if (selection.size() == 1)
		{
			RigidBody *first = *selection.begin();
			float mass = first->mass;
			if (ImGui::SliderFloat("Mass", &mass, 0, 1000.0f))
			{
				//Remove from world to change mass
				dynamicsWorld->removeRigidBody(first->rigidbody);
				btVector3 inertia;
				first->rigidbody->getCollisionShape()->
					calculateLocalInertia(mass, inertia);
				first->rigidbody->setMassProps(mass, inertia);
				dynamicsWorld->addRigidBody(first->rigidbody);

				first->mass = mass;
			}
			ImGui::ColorEdit4("Color", glm::value_ptr(first->trueColor));
			if (ImGui::CollapsingHeader("Path"))
			{
				Path();
			}
		}

		ImGui::End();
	}
}

void LevelEditor::Mouse(double xcursor, double ycursor, int width, int height,
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

		if (!bCtrl)
		{
			for (auto s : selection)
				s->color = s->trueColor;
			selection.clear();
		}

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

void LevelEditor::Translation()
{
	glm::vec3 centroid = glm::vec3(0);
	for (auto rb : selection)
		centroid += rb->GetTranslation();
	centroid /= selection.size();
	float x = centroid.x;
	if (ImGui::InputFloat("X", &x, 1.0f, 5.0f))
		TranslateSelection(glm::vec3(x-centroid.x,0,0));
	float y = centroid.y;
	if (ImGui::InputFloat("Y", &y, 1.0f, 5.0f))
		TranslateSelection(glm::vec3(0,y-centroid.y,0));
	float z = centroid.z;
	if (ImGui::InputFloat("Z", &z, 1.0f, 5.0f))
		TranslateSelection(glm::vec3(0,0,z-centroid.z));
}

void LevelEditor::TranslateSelection(glm::vec3 translate)
{
	for (auto rb : selection)
	{
		rb->rigidbody->setWorldTransform(btTransform(
			rb->rigidbody->getOrientation(),
			rb->rigidbody->getWorldTransform().getOrigin()
			+ convert(translate)));
	}
}

void LevelEditor::Rotation(ShaderProgram *shaderProgram)
{
	glm::vec3 centroid = glm::vec3(0);

	if (bLocal)
	{
		for (auto rb : selection)
		{
			DrawRotationGizmo(glm::vec3(1, 0, 0), rb->GetOrientation(),
				rb->GetTranslation(), shaderProgram, glm::vec4(1, 0, 0, 1));
			DrawRotationGizmo(glm::vec3(0, 1, 0), rb->GetOrientation(),
				rb->GetTranslation(), shaderProgram, glm::vec4(0, 1, 0, 1));
			DrawRotationGizmo(glm::vec3(0, 0, 1), rb->GetOrientation(),
				rb->GetTranslation(), shaderProgram, glm::vec4(0, 0, 1, 1));
		}
	}
	else
	{
		for (auto rb : selection)
			centroid += rb->GetTranslation();
		centroid /= selection.size();
		glm::vec3 before = centroid;

		DrawRotationGizmo(glm::vec3(1, 0, 0), glm::quat(),
			centroid, shaderProgram, glm::vec4(1, 0, 0, 1));
		DrawRotationGizmo(glm::vec3(0, 1, 0), glm::quat(),
			centroid, shaderProgram, glm::vec4(0, 1, 0, 1));
		DrawRotationGizmo(glm::vec3(0, 0, 1), glm::quat(),
			centroid, shaderProgram, glm::vec4(0, 0, 1, 1));
	}

	float x = 0;
	if (ImGui::InputFloat("X", &x, 15.0f, 15.0f))
	{
		if (bLocal) LocalRotation(x, glm::vec3(1, 0, 0));
		else GlobalRotation(x, glm::vec3(1, 0, 0), centroid);
	}

	float y = 0;
	if (ImGui::InputFloat("Y", &y, 15.0f, 15.0f))
	{
		if (bLocal) LocalRotation(y, glm::vec3(0, 1, 0));
		else GlobalRotation(y, glm::vec3(0, 1, 0), centroid);
	}

	float z = 0;
	if (ImGui::InputFloat("Z", &z, 15.0f, 15.0f))
	{
		if (bLocal) LocalRotation(z, glm::vec3(0, 0, 1));
		else GlobalRotation(z, glm::vec3(0, 0, 1), centroid);
	}

	if (ImGui::Button("Reset Rotation"))
	{
		for (auto rb : selection)
		{
			rb->rigidbody->setWorldTransform(btTransform(btQuaternion(),
				rb->rigidbody->getWorldTransform().getOrigin()));
		}
	}

	static int e = 0;
	ImGui::RadioButton("Global", &e, 0); ImGui::SameLine();
	ImGui::RadioButton("Local", &e, 1); ImGui::SameLine();
	bLocal = e;
}

void LevelEditor::LocalRotation(float angle, glm::vec3 axis)
{
	for (auto rb : selection)
	{
		glm::quat orientation = glm::angleAxis(glm::radians(angle),
			glm::toMat3(rb->GetOrientation()) *axis) 
			* rb->GetOrientation();
		rb->rigidbody->setWorldTransform(
			btTransform(convert(orientation),
				rb->rigidbody->getWorldTransform().getOrigin()));
	}
}

void LevelEditor::GlobalRotation(float angle, glm::vec3 axis, 
	glm::vec3 axisPosition)
{
	for (auto rb : selection)
	{
		glm::quat orientation = glm::angleAxis(glm::radians(angle), axis)
			* rb->GetOrientation();

		glm::vec3 pos = convert(rb->rigidbody->
			getWorldTransform().getOrigin());
		pos -= axisPosition;
		pos = glm::vec3(glm::vec4(pos, 1.0f) *
			glm::toMat4(glm::angleAxis(glm::radians(-angle), axis)));
		pos += axisPosition;

		rb->rigidbody->setWorldTransform(btTransform(convert(orientation),
				convert(pos)));
	}
}

//TODO - Scale it
void LevelEditor::DrawRotationGizmo(glm::vec3 axis, glm::quat orientation,
	glm::vec3 translation, ShaderProgram *shaderProgram, glm::vec4 color)
{
	axis = glm::toMat3(orientation) * axis;
	(*shaderProgram)["uColor"] = color;
	Line axisDraw(translation - (axis * ROTATION_GIZMO_SIZE),
		translation + (axis * ROTATION_GIZMO_SIZE));
	shaderProgram->Use([&]() { axisDraw.Render(); });
}

void LevelEditor::DrawPath(const ShaderProgram& program)
{
	if (selection.size() == 1)
	{
		RigidBody *rb = *selection.begin();
		if (!rb->motion.Points.empty())
		{
			std::vector<glm::vec3>& p(rb->motion.Points);
			std::vector<glm::vec3> c(p.size(), glm::vec3(0, 0, 1));
			std::vector<glm::vec3> l;
			for (int i = 0, n = p.size(); i < n; i++)
			{
				float t = (float)i;
				for (int j = 0; j < 10; j++)
				{
					l.push_back(rb->motion.GetPosition(t + (float)j / 10.f));
				}
			}
			if (rb->motion.Loop)
				l.push_back(l.front());
			if (c.size() > 1)
			{
				c.front() = glm::vec3(0, 1, 0);
				c.back() = glm::vec3(1, 0, 0);
			}
			Points pts(p, c);
			Line path(l);
			program.Use([&](){
				pts.Render(10.f);
			});
			program["uColor"] = glm::vec3(0, 0, 1);
			program.Use([&](){
				path.Render();
			});
		}
	}
}

void LevelEditor::Scale()
{
	glm::vec3 centroid = glm::vec3(0);
	for (auto rb : selection)
		centroid += convert(rb->rigidbody->getCollisionShape()->
			getLocalScaling());
	centroid /= selection.size();
	glm::vec3 before = centroid;
	if (ImGui::DragFloat3("Scale", glm::value_ptr(centroid)))
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

void LevelEditor::Path()
{
	RigidBody *rb = *selection.begin();
	ImGui::DragFloat("Speed", &rb->motion.Speed, 0.01f, 0.0f, 1.0f);
	ImGui::Checkbox("Loop path", &rb->motion.Loop);
	ImGui::Spacing();
	bool path_changed = false;
	int x = 0;
	for (auto i = rb->motion.Points.begin(); i != rb->motion.Points.end(); ++i)
	{
		std::string pt_text = ("Point " + std::to_string(x));
		std::string rm_text = ("Remove " + std::to_string(x));
		path_changed |=
			ImGui::DragFloat3(pt_text.c_str(), glm::value_ptr(*i));
		if (ImGui::Button(rm_text.c_str()))
		{
			i = rb->motion.Points.erase(i);
			path_changed = true;
			if (i == rb->motion.Points.end())
				break;
		}
		ImGui::Spacing();
		x++;
	}
	if (ImGui::Button("+"))
	{
		rb->motion.Points.push_back(rb->GetTranslation());
		path_changed = true;
	}
	if (path_changed)
		rb->motion.Reset();
}

void LevelEditor::DeleteSelection()
{
	for (auto rb : selection)
	{
		dynamicsWorld->removeRigidBody(rb->rigidbody);
		level->Delete(level->Find(rb->rigidbody));
		delete rb;
	}
	selection.clear();
}

void LevelEditor::CloneSelection()
{
	for (auto rb : selection)
	{
		RigidBody* newRb = new RigidBody(*rb);
		level->Objects.push_back(newRb);
		dynamicsWorld->addRigidBody(newRb->rigidbody);
	}
}

//https://github.com/opengl-tutorials/ogl/blob/master/misc05_picking/misc05_picking_BulletPhysics.cpp
void LevelEditor::ScreenPosToWorldRay(
	int mouseX, int mouseY,             // Mouse position, in pixels, from bottom-left corner of the window
	int screenWidth, int screenHeight,  // Window size, in pixels
	glm::mat4 ViewMatrix,               // Camera position and orientation
	glm::mat4 ProjectionMatrix,         // Camera parameters (ratio, field of view, near and far planes)
	glm::vec3& out_origin,              // Ouput : Origin of the ray. /!\ Starts at the near plane, so if you want the ray to start at the camera's position instead, ignore this.
	glm::vec3& out_direction            // Ouput : Direction, in world space, of the ray that goes "through" the mouse.
	)
{

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
