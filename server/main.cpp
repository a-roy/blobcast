#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <RakNet/MessageIdentifiers.h>
#include <RakNet/RakPeerInterface.h>

#include <iostream>
#include <random>

#include "GLFWProject.h"
#include "ShaderProgram.h"
#include "Text.h"
#include "AggregateInput.h"
#include "StreamWriter.h"

#include "SoftBody.h"
#include "Blob.h"
#include "RigidBody.h"
#include "Light.hpp"
#include "IOBuffer.h"
#include "Level.h"
#include "BlobDisplay.h"
#include "RenderingManager.h"

#include "config.h"

#include <imgui.h>
#include "imgui_impl_glfw.h"

#include "Points.h"
#include "Line.h"
#include "LevelEditor.h"
#include "BulletDebugDrawer.h"
#include "Profiler.h"


#include <stdio.h>
#include "tinyfiledialogs.h"

#include "Physics.h"
#include "Button.h"

bool init();
bool init_physics();
bool init_graphics();
bool init_stream();
void update();
void draw();

void drawGizmos();
void drawBulletDebug();
void gui();
void key_callback(
		GLFWwindow *window, int key, int scancode, int action, int mods);
void cursor_pos_callback(
	GLFWwindow *window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

GLFWwindow *window;
int width, height;

glm::mat4 viewMatrix;
glm::mat4 projMatrix;

RenderingManager renderManager;

StreamWriter *stream;
RakNet::RakPeerInterface *rakPeer = RakNet::RakPeerInterface::GetInstance();

BlobDisplay *blobDisplay;
Blob *blob;
Level *level;

ShaderProgram *displayShaderProgram;
ShaderProgram *debugdrawShaderProgram;

double currentFrame = glfwGetTime();
double lastFrame = currentFrame;
double deltaTime;
AggregateInput current_inputs;

bool bShowBlobCfg = false;
bool bShowGizmos = true;
bool bShowBulletDebug = true;
bool bShowImguiDemo = false;
bool bShowCameraSettings = true;

bool bStepPhysics = false;

LevelEditor *levelEditor;

double xcursor, ycursor;
bool bGui = true;

Camera *activeCam;
FlyCam* flyCam;
BlobCam* blobCam;

BulletDebugDrawer_DeprecatedOpenGL bulletDebugDrawer;

double frameCounterTime = 0.0f;
std::map<std::string, Measurement> Profiler::measurements;

#pragma warning(disable:4996) /* allows usage of strncpy, strcpy, strcat, sprintf, fopen */
char const *jsonExtension = ".json";

std::vector<Button*> buttons;

int main(int argc, char *argv[])
{
	window = GLFWProject::Init("Blobserver", RENDER_WIDTH, RENDER_HEIGHT);
	if (!window)
		return 1;

	// Setup ImGui binding
	ImGui_ImplGlfw_Init(window, true);

	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	glfwGetFramebufferSize(window, &width, &height);
	glfwSwapInterval(1);

	if (!init())
		return 1;

	levelEditor = new LevelEditor(Physics::dynamicsWorld, level);

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	while (!glfwWindowShouldClose(window))
	{
		Profiler::Start("Frame");
		update();

		Profiler::Start("Rendering");
		draw();
		Profiler::Finish("Rendering", false);

		Profiler::Start("Streaming");
		if (stream->IsOpen())
			stream->WriteFrame();
		Profiler::Finish("Streaming");

		if(bShowGizmos)
			drawGizmos();
		if (bShowBulletDebug)
			drawBulletDebug();
		if(bGui)
			gui();

		Profiler::Start("Rendering");
		glfwSwapBuffers(window);
		Profiler::Finish("Rendering", true);

		glfwPollEvents();
		Profiler::Finish("Frame");
	}
	if (stream->IsOpen())
	{
		rakPeer->Shutdown(0);
		RakNet::RakPeerInterface::DestroyInstance(rakPeer);
	}
	delete stream;

	Physics::Cleanup();

	delete blob;
	delete levelEditor;
	delete flyCam;
	delete blobCam;

	ImGui_ImplGlfw_Shutdown();
	glfwTerminate();
	return 0;
}

#pragma warning(default:4996)

bool init()
{
	return init_physics() && init_graphics() && init_stream();
}

bool init_physics()
{
	Physics::Init();
	
	blob = new Blob(Physics::softBodyWorldInfo, 
		btVector3(0, 100, 0), 3.0f, 512);
	btSoftBody *btblob = blob->softbody;

	level = Level::Deserialize(LevelDir "test_level.json");
	for(RigidBody* r : level->Objects)
		Physics::dynamicsWorld->addRigidBody(r->rigidbody);
	Physics::dynamicsWorld->addSoftBody(blob->softbody);
	Physics::dynamicsWorld->setDebugDrawer(&bulletDebugDrawer);

	return true;
}

bool init_graphics()
{
	if (!renderManager.init())
		return false;

	blobDisplay = new BlobDisplay(width, height, 128);

	displayShaderProgram = new ShaderProgram({
			ShaderDir "Display.vert",
			ShaderDir "Display.frag" });

	debugdrawShaderProgram = new ShaderProgram({
			ShaderDir "Gizmo.vert",
			ShaderDir "Gizmo.frag" });

	flyCam = new FlyCam(glm::vec3(0.f, 3.0f, 2.f), 2.0f, 3.0f * (glm::half_pi<float>() / 60.0f));
	blobCam = new BlobCam();

	activeCam = flyCam;

	return true;
}

bool init_stream()
{
	stream = new StreamWriter(width, height);

	RakNet::SocketDescriptor sd(REMOTE_GAME_PORT, 0);
	rakPeer->Startup(100, &sd, 1);
	rakPeer->SetMaximumIncomingConnections(100);

	return true;
}

void update()
{
	current_inputs = AggregateInput();
	while (stream->IsOpen() && rakPeer->GetReceiveBufferSize() > 0)
	{
		RakNet::Packet *p = rakPeer->Receive();
		unsigned char packet_type = p->data[0];
		if (packet_type == ID_USER_PACKET_ENUM)
		{
			BlobInput i = (BlobInput)p->data[1];
			current_inputs += i;
		}
	}

	blob->AddForces(current_inputs);

	currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	frameCounterTime += deltaTime;
	if (frameCounterTime >= 1.0f)
	{
		for (auto itr = Profiler::measurements.begin();
		itr != Profiler::measurements.end(); itr++)
			if(itr->second.avg)
				itr->second.SetAvg();
		frameCounterTime = 0;
	}

	Profiler::Start("Physics");
	if(bStepPhysics)
	{
		//For trigger objects
		/*mBody->setCollisionFlags(mBody->getCollisionFlags() |
		btCollisionObject::CF_NO_CONTACT_RESPONSE));*/

		std::cout << blob->softbody->m_rcontacts.size(); 
		if (buttons.size() > 0)
		{
			if (Physics::BroadphaseTest(blob->softbody, 
				buttons[0]->button->rigidbody))
				std::cout << "broadphase";
			if (Physics::NarrowphaseTest(blob->softbody, 
				buttons[0]->button->rigidbody))
				std::cout << "narrowphase" << std::endl;
			//rigidbody->getUserPointer to do things

			Physics::ContactResultCallback callback;
			Physics::dynamicsWorld->contactTest(buttons[0]->button->rigidbody,
				callback);
			Physics::dynamicsWorld->contactPairTest(blob->softbody,
				buttons[0]->button->rigidbody,
				callback);
			//btCollisionShape* shape = blob->softbody->getCollisionShape();
		}

		for (RigidBody *r : level->Objects)
			r->Update();
		for (Button *b : buttons)
			b->Update();
		Physics::dynamicsWorld->stepSimulation(deltaTime, 10);
	}
	Profiler::Finish("Physics");

	blobCam->Target = convert(blob->GetCentroid());
	activeCam->Update();

	blob->Update();
}

void draw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderManager.depthPass(blob, level);
	renderManager.dynamicCubeMapPass(blob, level);

	glViewport(0, 0, width, height);

	viewMatrix = activeCam->GetMatrix();
	projMatrix = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 400.f);

	renderManager.geometryPass(level, viewMatrix, projMatrix);
	renderManager.SSAOPass(projMatrix);
	renderManager.blurPass();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//renderManager.debugQuadDraw();

	renderManager.drawLevel(level, activeCam->Position, viewMatrix, projMatrix);

	renderManager.drawBlob(blob, activeCam->Position, viewMatrix, projMatrix);

	viewMatrix = glm::mat4(glm::mat3(viewMatrix));
	renderManager.drawSkybox(viewMatrix, projMatrix);
	viewMatrix = activeCam->GetMatrix();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	blobDisplay->Render(*displayShaderProgram, current_inputs);
	glDisable(GL_BLEND);
}


void drawGizmos()
{
	glm::mat4 mvpMatrix = projMatrix * activeCam->GetMatrix();
	(*debugdrawShaderProgram)["uMVPMatrix"] = mvpMatrix;

	(*debugdrawShaderProgram)["uColor"] = glm::vec4(1, 0, 0, 1);
	Line x(glm::vec3(0, 0, 0), glm::vec3(1, 0, 0));
	debugdrawShaderProgram->Use([&](){
		x.Render();
	});
	(*debugdrawShaderProgram)["uColor"] = glm::vec4(0, 1, 0, 1);
	Line y(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	debugdrawShaderProgram->Use([&](){
		y.Render();
	});
	(*debugdrawShaderProgram)["uColor"] = glm::vec4(0, 0, 1, 1);
	Line z(glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));
	debugdrawShaderProgram->Use([&](){
		z.Render();
	});

	Points p(glm::vec3(0));
	(*debugdrawShaderProgram)["uColor"] = glm::vec4(0, 0, 0, 1);
	float sz = 1.0f/glm::distance(activeCam->Position, glm::vec3(0)) * 50.0f;
	debugdrawShaderProgram->Use([&](){
		p.Render(sz);
	});

	//Line ray(levelEditor->out_origin, levelEditor->out_end);
	//ray.Render();

	//blob->DrawGizmos(debugdrawShaderProgram);
}

void drawBulletDebug()
{
	bulletDebugDrawer.SetMatrices(viewMatrix, projMatrix);
	Physics::dynamicsWorld->debugDrawWorld();
}

void gui()
{
	ImGui_ImplGlfw_NewFrame();

	ImGui::SetNextWindowPos(ImVec2(width - 500, 40));
	if (ImGui::Begin("", (bool*)true, ImVec2(0, 0), 0.9f,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
	{
		if (stream->IsOpen())
			ImGui::Text("We are blobcasting live!");
		else
			ImGui::Text("Blobcast server unavailable");
		ImGui::Separator();
		ImGui::Text("Right click to turn the camera");
		ImGui::Separator();

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
			1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::Text("Physics average %.3f ms/frame | %.1f percent | %.1f FPS",
			Profiler::measurements["Physics"].result*1000,
			(Profiler::measurements["Physics"].result
				/ Profiler::measurements["Frame"].result) * 100.0f,
			1.0f / Profiler::measurements["Physics"].result);
		ImGui::Text("Streaming average %.3f ms/frame | %.1f percent | %.1f FPS",
			Profiler::measurements["Streaming"].result * 1000,
			(Profiler::measurements["Streaming"].result
				/ Profiler::measurements["Frame"].result) * 100.0f,
			1.0f / Profiler::measurements["Streaming"].result);
		ImGui::Text("Rendering average %.3f ms/frame | %.1f percent | %.1f FPS",
			Profiler::measurements["Rendering"].result * 1000,
			(Profiler::measurements["Rendering"].result
				/ Profiler::measurements["Frame"].result) * 100.0f,
			1.0f / Profiler::measurements["Rendering"].result);

		ImGui::Separator();
		ImGui::Text("Mouse Position: (%.1f,%.1f)", xcursor, ycursor);
		ImGui::Text("Camera Position: (%.1f,%.1f,%.1f)", activeCam->Position.x,
			activeCam->Position.y, activeCam->Position.z);

		ImGui::End();
	}

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Load"))
			{
				char const *lTheOpenFileName = NULL;
				lTheOpenFileName = tinyfd_openFileDialog (
					"Load level",
					"",
					0,
					NULL,
					NULL,
					0);
				if (lTheOpenFileName != NULL)
				{
					levelEditor->selection.clear();

					for (RigidBody* rb : level->Objects)
						Physics::dynamicsWorld->removeRigidBody(rb->rigidbody);
					delete level;

					level = Level::Deserialize(lTheOpenFileName);
					for (RigidBody* rb : level->Objects)
						Physics::dynamicsWorld->addRigidBody(rb->rigidbody);

					levelEditor->level = level;
				}
			}

			if (ImGui::MenuItem("Save as.."))
			{
				char const *lTheSaveFileName = NULL;

				lTheSaveFileName = tinyfd_saveFileDialog(
					"Save Level",
					"level.json",
					/*1*/0,
					/*&jsonExtension*/NULL,
					NULL);

				if(lTheSaveFileName != NULL )
					level->Serialize(lTheSaveFileName);
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("Blob Editor", NULL, bShowBlobCfg))
				bShowBlobCfg ^= 1;
			if (ImGui::MenuItem("Gizmos", NULL, bShowGizmos))
				bShowGizmos ^= 1;
			if (ImGui::MenuItem("Bullet Debug", NULL, bShowBulletDebug))
				bShowBulletDebug ^= 1;
			if (ImGui::MenuItem("ImGui Demo", NULL, bShowImguiDemo))
				bShowImguiDemo ^= 1;
			if (ImGui::MenuItem("Camera Settings", NULL, bShowCameraSettings))
				bShowCameraSettings ^= 1;

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Create"))
		{
			if (ImGui::MenuItem("Box"))
			{
				level->AddBox(glm::vec3(0), glm::quat(), glm::vec3(1),
					glm::vec4(.5f, .5f, .5f, 1.f), 1.0f);
				Physics::dynamicsWorld->addRigidBody(
					level->Objects[level->Objects.size() - 1]->rigidbody);
			}
			if (ImGui::MenuItem("Cylinder"))
			{
				level->AddCylinder(glm::vec3(0), glm::quat(), glm::vec3(1),
					glm::vec4(.5f, .5f, .5f, 1.f), 1.0f);
				Physics::dynamicsWorld->addRigidBody(
					level->Objects[level->Objects.size() - 1]->rigidbody);
			}
			if (ImGui::MenuItem("Button"))
			{
				/*level->AddButton(glm::vec3(0), glm::quat(), glm::vec3(1),
					glm::vec4(.5f, .5f, .5f, 1.f), 1.0f);*/
				Button *b = new Button(glm::vec3(0), glm::quat(), glm::vec3(1),
					glm::vec4(.5f, .5f, .5f, 1.f), 1.0f);
				level->Objects.push_back(b->button);
				buttons.push_back(b);
				Physics::dynamicsWorld->addRigidBody(
					level->Objects[level->Objects.size() - 1]->rigidbody);
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Settings"))
		{
			if(ImGui::MenuItem("Step Physics", NULL, bStepPhysics))
				bStepPhysics ^= 1;

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	if (bShowImguiDemo)
	{
		ImGui::ShowTestWindow();
	}

	if (bShowCameraSettings)
	{
		ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiSetCond_FirstUseEver);

		ImGui::Begin("Camera Settings", &bShowCameraSettings);
		static int n;
		ImGui::Combo("Type", &n, "Fly Cam\0Blob Cam\0\0");

		if (n == 0)
		{
			activeCam = flyCam;
			ImGui::SliderFloat("Move Speed [1,100]",
				&flyCam->MoveSpeed, 0.0f, 20.0f);
		}
		else
		{
			activeCam = blobCam;
			ImGui::SliderFloat("Distance [1,100]",
				&blobCam->Distance, 1.0f, 100.0f);
			ImGui::SliderFloat("Height [1,100]",
				&blobCam->Height, 1.0f, 100.0f);
		}

		ImGui::End();
	}

	if (bShowBlobCfg)
	{
		ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiSetCond_FirstUseEver);

		ImGui::Begin("Blob Edtior", &bShowBlobCfg);
		ImGui::SliderFloat("Rigid Contacts Hardness [0,1]",
			&blob->softbody->m_cfg.kCHR, 0.0f, 1.0f);
		ImGui::SliderFloat("Dynamic Friction Coefficient [0,1]",
			&blob->softbody->m_cfg.kDF, 0.0f, 1.0f);
		ImGui::InputFloat("Pressure coefficient [-inf,+inf]",
			&blob->softbody->m_cfg.kPR, 1.0f, 100.0f);
		ImGui::InputFloat("Volume conversation coefficient [0, +inf]",
			&blob->softbody->m_cfg.kVC, 1.0f, 100.0f);
		ImGui::InputFloat("Drag coefficient [0, +inf]",
			&blob->softbody->m_cfg.kDG, 1.0f, 100.0f);
		ImGui::SliderFloat("Damping coefficient [0,1]",
			&blob->softbody->m_cfg.kDP, 0.0f, 1.0f);
		ImGui::InputFloat("Lift coefficient [0,+inf]",
			&blob->softbody->m_cfg.kLF, 1.0f, 100.0f);
		ImGui::SliderFloat("Pose matching coefficient [0,1]",
			&blob->softbody->m_cfg.kMT, 0.0f, 1.0f);

		ImGui::Separator();

		ImGui::InputFloat("Movement force", &blob->speed, 0.1f, 100.0f);

		static float vec3[3] = { 0.f, 0.f, 0.f };
		if(ImGui::InputFloat3("", vec3))
		ImGui::SameLine();
		if (ImGui::SmallButton("Set Position"))
			blob->softbody->translate(
				btVector3(vec3[0], vec3[1], vec3[2]) - blob->GetCentroid());

		ImGui::End();
	}

	glm::mat4 mvpMatrix = projMatrix * activeCam->GetMatrix();
	(*debugdrawShaderProgram)["uMVPMatrix"] = mvpMatrix;
	debugdrawShaderProgram->Use([&](){
		levelEditor->Gui(debugdrawShaderProgram);
	});

	ImGui::Render();
	glDisable(GL_SCISSOR_TEST);
	glEnable(GL_DEPTH_TEST);

	debugdrawShaderProgram->Use([&](){
		levelEditor->DrawPath(*debugdrawShaderProgram);
	});
}

void key_callback(
		GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_G && action == GLFW_PRESS)
		bGui ^= 1;

	if (key == GLFW_KEY_DELETE && action == GLFW_PRESS)
		levelEditor->DeleteSelection();

	if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_PRESS)
		levelEditor->bCtrl = true;
	else if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_RELEASE)
		levelEditor->bCtrl = false;

	blob->Move(key, action);

	GLFWProject::WASDStrafe(activeCam, window, key, scancode, action, mods);
}

void cursor_pos_callback(GLFWwindow *window, double xpos, double ypos)
{
	GLFWProject::MouseTurn(activeCam, &xcursor, &ycursor, window, xpos, ypos);

	xcursor = xpos;
	ycursor = ypos;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL)
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		else
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		if(!ImGui::GetIO().WantCaptureMouse)
			if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL)
				levelEditor->Mouse(xcursor, height - ycursor, width, height, 
					viewMatrix, projMatrix);
	}
}
