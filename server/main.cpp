#define NOMINMAX

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
#include "Entity.h"
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
#include "Timer.h"

#include "ParticleSystem.h"

//#include <SPK.h>
//#include <SPK_GL.h>

#include "Physics.h"
#include "Trigger.h"

bool init();
bool init_physics();
bool init_graphics();
bool init_stream();
void update();
void draw();

void infoBox();
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

double Timer::currentFrame = glfwGetTime();
double Timer::lastFrame = Timer::currentFrame;
double Timer::deltaTime;
AggregateInput current_inputs;

LevelEditor *levelEditor;

double xcursor, ycursor;
bool bGui = true;

Camera *activeCam;
FlyCam* flyCam;
BlobCam* blobCam;

BulletDebugDrawer_DeprecatedOpenGL bulletDebugDrawer;

double frameCounterTime = 0.0f;
std::map<std::string, Measurement> Profiler::measurements;

#pragma warning(disable:4996)

int main(int argc, char *argv[])
{
	window = GLFWProject::Init("Blobserver", RENDER_WIDTH, RENDER_HEIGHT);
	if (!window)
		return 1;

	ImGui_ImplGlfw_Init(window, true);

	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	glfwGetFramebufferSize(window, &width, &height);
	glfwSwapInterval(1);

	if (!init())
		return 1;

	levelEditor = new LevelEditor(level);

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_POINT_SPRITE);
	glEnable(GL_PROGRAM_POINT_SIZE);
	
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

		if (Physics::bShowBulletDebug)
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
	for(Entity* r : level->Objects)
		Physics::dynamicsWorld->addRigidBody(r->rigidbody);
	Physics::dynamicsWorld->addSoftBody(blob->softbody);
	Physics::dynamicsWorld->setDebugDrawer(&bulletDebugDrawer);

	level->AddParticleSystem(glm::vec3(0));

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

	flyCam = new FlyCam(glm::vec3(0.f, 3.0f, 2.f), 2.0f, 
		3.0f * (glm::half_pi<float>() / 60.0f));
	blobCam = new BlobCam();

	activeCam = flyCam;
	BufferData::cam = flyCam;

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

	Timer::currentFrame = glfwGetTime();
	Timer::deltaTime = Timer::currentFrame - Timer::lastFrame;
	Timer::lastFrame = Timer::currentFrame;
	frameCounterTime += Timer::deltaTime;
	if (frameCounterTime >= 1.0f)
	{
		for (auto itr = Profiler::measurements.begin();
		itr != Profiler::measurements.end(); itr++)
			if(itr->second.avg)
				itr->second.SetAvg();
		frameCounterTime = 0;
	}

	Profiler::Start("Physics");
	if(Physics::bStepPhysics)
	{
		for(Entity* ent : level->Objects)
		{
			Trigger* trigger = dynamic_cast<Trigger*>(ent);

			if (trigger)
			{
				bool collides = false;
				if (Physics::BroadphaseCheck(blob->softbody,
					trigger->rigidbody))
					if (Physics::NarrowphaseCheck(blob->softbody,
						trigger->rigidbody))
						collides = true;
				
				if (collides)
					if (!trigger->bTriggered) 
						trigger->OnEnter();
					else 
						trigger->OnStay();
				else 
					if (trigger->bTriggered) 
						trigger->OnLeave();
				
			}
		}

		for (Entity *r : level->Objects)
			r->Update(Timer::deltaTime);

		Physics::dynamicsWorld->stepSimulation(Timer::deltaTime, 10);
	}
	Profiler::Finish("Physics");

	Profiler::Start("Particles");
	for (auto ps : level->ParticleSystems)
		ps->Update(Timer::deltaTime);
	Profiler::Finish("Particles", false);

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
	projMatrix = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 400.0f);

	renderManager.geometryPass(level, viewMatrix, projMatrix);
	renderManager.SSAOPass(projMatrix, activeCam->Position);
	renderManager.blurPass();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, width, height);

	//renderManager.debugQuadDraw();

	projMatrix = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 400.0f);
	renderManager.drawLevel(level, activeCam->Position, viewMatrix, projMatrix);
	renderManager.drawBlob(blob, activeCam->Position, viewMatrix, projMatrix);

	viewMatrix = glm::mat4(glm::mat3(viewMatrix));
	renderManager.drawSkybox(viewMatrix, projMatrix);
	viewMatrix = activeCam->GetMatrix();

	renderManager.drawParticles(level, viewMatrix, projMatrix);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	blobDisplay->Render(*displayShaderProgram, current_inputs);
	glDisable(GL_BLEND);
}

void drawBulletDebug()
{
	bulletDebugDrawer.SetMatrices(viewMatrix, projMatrix);
	Physics::dynamicsWorld->debugDrawWorld();
}

void gui()
{
	ImGui_ImplGlfw_NewFrame();

	infoBox();
	levelEditor->MainMenuBar();

	if (levelEditor->bShowImguiDemo)
		ImGui::ShowTestWindow();
	if (levelEditor->bShowBlobCfg)
		blob->Gui();
	
	if (levelEditor->bShowCameraSettings)
	{
		ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiSetCond_FirstUseEver);

		ImGui::Begin("Camera Settings", &levelEditor->bShowCameraSettings);
		static int n;
		ImGui::Combo("Type", &n, "Fly Cam\0Blob Cam\0\0");

		if (n == 0)
		{
			activeCam = flyCam;
			ImGui::SliderFloat("Move Speed [1,100]",
				&flyCam->MoveSpeed, 0.0f, 20.0f);
			BufferData::cam = flyCam;
		}
		else
		{
			activeCam = blobCam;
			ImGui::SliderFloat("Distance [1,100]",
				&blobCam->Distance, 1.0f, 100.0f);
			ImGui::SliderFloat("Height [1,100]",
				&blobCam->Height, 1.0f, 100.0f);
			BufferData::cam = blobCam;
		}

		ImGui::End();
	}

	glm::mat4 mvpMatrix = projMatrix * activeCam->GetMatrix();
	(*debugdrawShaderProgram)["uMVPMatrix"] = mvpMatrix;
	debugdrawShaderProgram->Use([&](){
		levelEditor->SelectionWindow(debugdrawShaderProgram);
	});

	ImGui::Render();
	glDisable(GL_SCISSOR_TEST);
	glEnable(GL_DEPTH_TEST);

	debugdrawShaderProgram->Use([&]() {
		levelEditor->DrawPath(*debugdrawShaderProgram);
	});
}

void infoBox()
{
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
		Profiler::Gui("Physics");
		Profiler::Gui("Streaming");
		Profiler::Gui("Rendering");
		Profiler::Gui("Particles");

		ImGui::Separator();
		ImGui::Text("Mouse Position: (%.1f,%.1f)", xcursor, ycursor);
		ImGui::Text("Camera Position: (%.1f,%.1f,%.1f)", activeCam->Position.x,
			activeCam->Position.y, activeCam->Position.z);

		ImGui::End();
	}
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
