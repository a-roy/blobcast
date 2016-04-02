#include "LevelEditor.h"
#include "config.h"
#include "Line.h"
#include "Points.h"
#include <sstream>
#include "Timer.h"

void LevelEditor::MainMenuBar()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File")) 
		{
			if (ImGui::MenuItem("Load")) 
			{
				char const *lTheOpenFileName = NULL;
				lTheOpenFileName = tinyfd_openFileDialog(
					"Load level",
					"",
					0,
					NULL,
					NULL,
					0);
				if (lTheOpenFileName != NULL) {
					selection.clear();
					for (GameObject* ent : Level::currentLevel->Objects)
						Physics::dynamicsWorld->removeRigidBody(ent->rigidbody);
					delete Level::currentLevel;
					for (int i = 
						Physics::dynamicsWorld->getNumConstraints() - 1; 
						i >= 0; i--)
						Physics::dynamicsWorld->removeConstraint(
							Physics::dynamicsWorld->getConstraint(i));
					Level::currentLevel = Level::Deserialize(lTheOpenFileName);
					/*for (GameObject* ent : Level::currentLevel->Objects)
						Physics::dynamicsWorld->addRigidBody(ent->rigidbody);*/
				}
			}

			if (ImGui::MenuItem("Save as..")) {
				char const *lTheSaveFileName = NULL;

				lTheSaveFileName = tinyfd_saveFileDialog(
					"Save Level",
					"level.json",
					/*1*/0,
					/*&jsonExtension*/NULL,
					NULL);

				if (lTheSaveFileName != NULL)
					Level::currentLevel->Serialize(lTheSaveFileName);
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View")) {
			if (ImGui::MenuItem("Blob Editor", NULL, bShowBlobCfg))
				bShowBlobCfg ^= 1;
			if (ImGui::MenuItem("Bullet Debug", NULL, 
				Physics::bShowBulletDebug))
				Physics::bShowBulletDebug ^= 1;
			if (ImGui::MenuItem("ImGui Demo", NULL, bShowImguiDemo))
				bShowImguiDemo ^= 1;
			if (ImGui::MenuItem("Camera Settings", NULL, bShowCameraSettings))
				bShowCameraSettings ^= 1;

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Create"))
		{
			Level* level = Level::currentLevel;
			if (ImGui::MenuItem("GameObject"))
			{
				level->AddGameObject(glm::vec3(0), glm::quat(), glm::vec3(1),
					glm::vec4(.5f, .5f, .5f, 1.f), 4, "box", 1.0f);
				selection.clear();
				selection.insert(level->Objects[level->Objects.size() - 1]);
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Settings"))
		{
			if (ImGui::MenuItem("Step Physics", NULL,
				Physics::bStepPhysics))
				Physics::bStepPhysics ^= 1;
			if (ImGui::Button("Step Once"))
				Physics::dynamicsWorld->stepSimulation(Timer::deltaTime, 10);

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
}

void LevelEditor::SelectionWindow(ShaderProgram *shaderProgram)
{
	if (selection.size() > 0)
	{
		ImGui::SetNextWindowSize(ImVec2(300, 400),
			ImGuiSetCond_FirstUseEver);

		std::stringstream ss;
		ss << "Selection (" << selection.size() << " objects)###Selection";
		ImGui::Begin(ss.str().c_str());

		if (ImGui::Button("Put Blob Above"))
		{
			GameObject *first = *selection.begin();
			Physics::blob->softbody->translate(
				convert(first->GetTranslation() + glm::vec3(0,5,0)));
		}

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
			GameObject *first = *selection.begin();

			ImGui::Text("ID: %i", first->ID);

			int tex = first->textureID;
			ImGui::InputInt("Texture ID", &tex, 1, 1);
			first->textureID = tex;

			bool collidable = first->GetCollidable();
			if (ImGui::Checkbox("Collidable", &collidable))
				first->SetCollidable(collidable);
			ImGui::Checkbox("Drawable", &first->drawable);
			Shape shapeType = first->shapeType;
			if(ImGui::Combo("Shape", (int*)&shapeType, 
				"Box\0Cylinder\0"));
			{
				first->SetShape(shapeType);
				if (shapeType == Box)
					first->SetMesh(
						Level::currentLevel->Meshes[(size_t)Box]);
				else
					first->SetMesh(
						Level::currentLevel->Meshes[(size_t)Cylinder]);
			}
				
			float mass = first->mass;
			if (ImGui::InputFloat("Mass", &mass, 1.0f, 10.0f))
			{
				//Remove from world to change mass
				Physics::dynamicsWorld->removeRigidBody(first->rigidbody);
				btVector3 inertia;
				first->rigidbody->getCollisionShape()->
					calculateLocalInertia(mass, inertia);
				first->rigidbody->setMassProps(mass, inertia);
				Physics::dynamicsWorld->addRigidBody(first->rigidbody);

				first->mass = mass;
			}
			ImGui::ColorEdit4("Color", glm::value_ptr(first->trueColor));
			
			if (first->mass) {
				if (ImGui::CollapsingHeader("Path")) {
					Path();
				}
			}
			
			if (ImGui::CollapsingHeader("Trigger")) {
				if (ImGui::Button("Set Path Link")) {
					bSetLink = true;
				}
				if (bSetLink) {
					ImGui::SameLine();
					ImGui::Text("Click on the path object now");
				}
				if (!first->trigger.bDeadly)
				{
					if (ImGui::Button("Set Deadly")) {
						first->trigger.bDeadly = true;
						first->trigger.RegisterCallback(Physics::CreateBlob, Enter);
					}
				}
				else 
				{
					ImGui::Text("Deadly!");
				}
				for (int id : first->trigger.connectionIDs) {
					std::stringstream ss;
					ss << "Platform - " << id;
					if (ImGui::Button(ss.str().c_str()))
					{
						ClearSelection();
						GameObject* e = Level::currentLevel->Find(id);
						selection.insert(e);
						e->color =
							glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
					}
				}
			}
		}

		ImGui::End();
	}
}

void LevelEditor::Mouse(double xcursor, double ycursor, int width, int height,
	glm::mat4 viewMatrix, glm::mat4 projectionMatrix)
{
	glm::vec3 out_origin;
	glm::vec3 out_direction;

	ScreenPosToWorldRay((int)xcursor, (int)ycursor, width, height,
		viewMatrix, projectionMatrix, out_origin, out_direction);

	glm::vec3 out_end = out_origin + out_direction * 1000.0f;

	btCollisionWorld::ClosestRayResultCallback
		RayCallback(btVector3(out_origin.x, out_origin.y, out_origin.z),
			btVector3(out_end.x, out_end.y, out_end.z));

	Physics::dynamicsWorld->rayTest(
		btVector3(out_origin.x, out_origin.y, out_origin.z),
		btVector3(out_end.x, out_end.y, out_end.z), RayCallback);

	if (RayCallback.hasHit())
	{
		if (typeid(*RayCallback.m_collisionObject) != typeid(btRigidBody))
			return;
		NewSelection((GameObject*)RayCallback.m_collisionObject->
			getUserPointer());
	}
	else
	{
		ClearSelection();
		bSetLink = false;
	}
}

void LevelEditor::NewSelection(GameObject* newSelection)
{
	if (bSetLink)
	{
		if (!newSelection->motion.Points.empty())
			(*selection.begin())->trigger.LinkToPlatform(newSelection);
		bSetLink = false;
	}
	else
	{
		if (!bCtrl)
			ClearSelection();
		newSelection->color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
		selection.insert(newSelection);
	}
}

void LevelEditor::ClearSelection()
{
	for (auto s : selection)
		s->color = s->trueColor;
	selection.clear();
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

void LevelEditor::DrawRotationGizmo(glm::vec3 axis, glm::quat orientation,
	glm::vec3 translation, ShaderProgram *shaderProgram, glm::vec4 color)
{
	axis = glm::toMat3(orientation) * axis;
	Line axisDraw(translation - (axis * ROTATION_GIZMO_SIZE),
		translation + (axis * ROTATION_GIZMO_SIZE),
		glm::vec3(color));
	shaderProgram->Use([&]() { axisDraw.Render(); });
}

void LevelEditor::DrawPath(const ShaderProgram& program)
{
	if (selection.size() == 1)
	{
		GameObject* ent = *selection.begin();
		if (!ent->motion.Points.empty())
		{
			std::vector<glm::vec3>& p(ent->motion.Points);
			std::vector<glm::vec3> c(p.size(), glm::vec3(0, 0, 1));
			std::vector<glm::vec3> l;
			l.push_back(p.front());
			for (int i = 0, n = (ent->motion.Loop ? p.size() : p.size() - 1);
					i < n; i++)
			{
				float t = (float)i;
				for (int j = 1; j <= 10; j++)
				{
					l.push_back(ent->motion.GetPosition(t + (float)j / 10.f));
				}
			}
			if (c.size() > 1)
			{
				c.front() = glm::vec3(0, 1, 0);
				c.back() = glm::vec3(1, 0, 0);
			}
			Points pts(p, c);
			Line path(l, glm::vec3(0, 0, 1));
			program.Use([&](){
				pts.Render(10.f);
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

	float x = centroid.x;
	if (ImGui::InputFloat("X", &x, 0.5f, 2.0f))
		ScaleSelection(glm::vec3(x - centroid.x, 0, 0));
	float y = centroid.y;
	if (ImGui::InputFloat("Y", &y, 0.5f, 2.0f))
		ScaleSelection(glm::vec3(0, y - centroid.y, 0));
	float z = centroid.z;
	if (ImGui::InputFloat("Z", &z, 0.5f, 2.0f))
		ScaleSelection(glm::vec3(0, 0, z - centroid.z));
}

void LevelEditor::ScaleSelection(glm::vec3 relScale)
{
	for (auto rb : selection)
	{
		btVector3 newScale = rb->rigidbody->getCollisionShape()->
			getLocalScaling() + convert(relScale);
		if (newScale.getX() <= 0 
			|| newScale.getY() <= 0 || newScale.getZ() <= 0)
			continue;
		rb->rigidbody->getCollisionShape()->setLocalScaling(
				newScale);
		Physics::dynamicsWorld->updateSingleAabb(rb->rigidbody);
	}
}

void LevelEditor::Path()
{
	GameObject *ent = *selection.begin();
	ImGui::DragFloat("Speed", &ent->motion.Speed, 0.01f, 0.0f, 1.0f);
	ImGui::Checkbox("Loop path", &ent->motion.Loop);
	ImGui::Checkbox("Enabled", &ent->motion.Enabled);
	static int e = ent->motion.Curved;
	ImGui::RadioButton("Straight", &e, 0);
	ImGui::SameLine();
	ImGui::RadioButton("Curved", &e, 1);
	ent->motion.Curved = e;
	ImGui::Spacing();
	bool path_changed = false;
	int x = 0;
	for (auto i = ent->motion.Points.begin(); i != ent->motion.Points.end(); ++i)
	{
		std::string pt_text = ("P" + std::to_string(x));
		std::string rm_text = ("Rm " + std::to_string(x));
		std::string ins_text = ("Ins " + std::to_string(x));
		path_changed |=
			ImGui::DragFloat3(pt_text.c_str(), glm::value_ptr(*i));
		ImGui::SameLine();
		if (ImGui::Button(rm_text.c_str()))
		{
			i = ent->motion.Points.erase(i);
			path_changed = true;
			if (i == ent->motion.Points.end())
				break;
		}
		ImGui::SameLine();
		if (ImGui::Button(ins_text.c_str()))
		{
			i = ent->motion.Points.insert(i, ent->GetTranslation());
			path_changed = true;
		}
		ImGui::Spacing();
		x++;
	}
	if (ImGui::Button("Add new point"))
	{
		ent->motion.Points.push_back(ent->GetTranslation());
		path_changed = true;
	}
	if (path_changed)
		ent->motion.Reset();
}

void LevelEditor::DeleteSelection()
{
	for (auto rb : selection)
	{
		Physics::dynamicsWorld->removeRigidBody(rb->rigidbody);
		Level::currentLevel->Delete(Level::currentLevel->Find(rb->rigidbody));
		delete rb;
	}
	selection.clear();
}

void LevelEditor::CloneSelection()
{
	for (GameObject* rb : selection)
	{
		GameObject* newEnt = new GameObject(*rb);
		Level::currentLevel->Objects.push_back(newEnt);
		//Physics::dynamicsWorld->addRigidBody(newEnt->rigidbody);
	}
}
