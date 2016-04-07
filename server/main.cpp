#define NOMINMAX

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <RakNet/MessageIdentifiers.h>
#include <RakNet/RakPeerInterface.h>

#include <memory>
#include <iostream>
#include <random>

#include "GLFWProject.h"
#include "ShaderProgram.h"
#include "Text.h"
#include "AggregateInput.h"
#include "StreamWriter.h"

#include "SoftBody.h"
#include "Blob.h"
#include "GameObject.h"
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
float death_plane_y = -100.f;

ShaderProgram *displayShaderProgram;
ShaderProgram *debugdrawShaderProgram;

AggregateInput current_inputs;

LevelEditor *levelEditor;
Level* Level::currentLevel;

double xcursor, ycursor;
bool bGui = true;

Camera *activeCam;
FlyCam* flyCam;
BlobCam* blobCam;

std::unique_ptr<Text> chat_text;
std::shared_ptr<Font> chat_font;
std::unique_ptr<ShaderProgram> text_program;

BulletDebugDrawer_DeprecatedOpenGL bulletDebugDrawer;

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

	levelEditor = new LevelEditor();

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

		if (bGui)
		{
			if (Physics::bShowBulletDebug)
				drawBulletDebug();
			gui();
		}

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
	
	Physics::CreateBlob();

	Level::currentLevel = Level::Deserialize(LevelDir "level1.json");
	/*for(GameObject* r : Level::currentLevel->Objects)
		Physics::dynamicsWorld->addRigidBody(r->rigidbody);*/
	Physics::dynamicsWorld->setDebugDrawer(&bulletDebugDrawer);

	//Level::currentLevel->AddParticleSystem(glm::vec3(0));

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

	chat_font = std::shared_ptr<Font>(new Font(FontDir "Vera.ttf", 16.f));
	chat_text = std::unique_ptr<Text>(new Text(chat_font.get()));
	chat_text->XPosition = width - 432;
	chat_text->YPosition = 32;
	chat_text->SetText(" ");

	text_program = std::unique_ptr<ShaderProgram>(new ShaderProgram({
			ShaderDir "Text.vert",
			ShaderDir "Text.frag" }));

	glm::mat4 textMatrix = glm::ortho(0.f, (float)width, 0.f, (float)height);
	(*text_program)["uAtlas"] = 0;
	(*text_program)["uTextColor"] = glm::vec4(0.f, 0.f, 0.f, 1.f);
	(*text_program)["uMVPMatrix"] = textMatrix;

	return true;
}

bool init_stream()
{
	stream = new StreamWriter(width, height, 1);

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
		else if (packet_type == ID_USER_PACKET_ENUM + 1)
		{
			std::string text = "Blobchat: ";
			text.insert(text.end(), p->data + 1, p->data + p->length);
			chat_text->SetText(text);
		}
	}

	Physics::blob->AddForces(current_inputs);

	Timer::Update(glfwGetTime());
	Profiler::Update(Timer::deltaTime);

	Profiler::Start("Physics");
	if(Physics::bStepPhysics)
	{
		for(GameObject* ent : Level::currentLevel->Objects)
		{
			if (ent->trigger.bEnabled)
			{
				bool collides = false;
				
				if (!ent->trigger.bDeadly
					&& !ent->trigger.bLoopy)
				{
					Physics::ContactResultCallback callback(&collides);
					Physics::dynamicsWorld->contactTest
						(ent->rigidbody, callback);
				}

				if (Physics::BroadphaseCheck(Physics::blob->softbody,
					ent->rigidbody))
					if (Physics::NarrowphaseCheck(Physics::blob->softbody,
						ent->rigidbody))
						collides = true;

				if (collides)
					if (!ent->trigger.bTriggered)
						ent->trigger.OnEnter();
					else
						ent->trigger.OnStay();
				else
					if (ent->trigger.bTriggered)
						ent->trigger.OnLeave();
			}

			if (ent->motion.Points.size() > 0)
				ent->rigidbody->setAngularVelocity(btVector3(0, 0, 0));
				//ent->rigidbody->setAngularFactor(btVector3(0,0,0));
		}

		for (GameObject *r : Level::currentLevel->Objects)
			r->Update(Timer::deltaTime);

		Physics::dynamicsWorld->stepSimulation(Timer::deltaTime, 10);
		if (Physics::blob->GetCentroid().getY() < death_plane_y)
			Physics::CreateBlob();
	}
	Profiler::Finish("Physics");

	Profiler::Start("Particles");
	/*for (auto ps : level->ParticleSystems)
		ps->Update(Timer::deltaTime);*/
	Profiler::Finish("Particles", false);

	blobCam->Target = convert(Physics::blob->GetCentroid());
	activeCam->Update();

	Physics::blob->Update();
}

void draw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderManager.depthPass(Physics::blob, Level::currentLevel, activeCam->Position);
	renderManager.dynamicCubeMapPass(Physics::blob, Level::currentLevel);

	glViewport(0, 0, width, height);

	viewMatrix = activeCam->GetMatrix();
	projMatrix = glm::perspective(glm::radians(60.0f), 
		(float)width / (float)height, 0.1f, 500.0f);

	renderManager.geometryPass(Level::currentLevel, viewMatrix, projMatrix);
	renderManager.SSAOPass(projMatrix, activeCam->Position);
	renderManager.blurPass();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//glViewport(0, 0, 200, 200);
	//renderManager.debugQuadDraw();
	
	glViewport(0, 0, width, height);
	
	renderManager.drawBlob(Physics::blob, activeCam->Position, viewMatrix, projMatrix);

	renderManager.drawLevel(Level::currentLevel, activeCam->Position, 
		viewMatrix, projMatrix);

	viewMatrix = glm::mat4(glm::mat3(viewMatrix));
	renderManager.drawSkybox(viewMatrix, projMatrix);
	viewMatrix = activeCam->GetMatrix();

	//renderManager.drawParticles(Level::currentLevel, viewMatrix, projMatrix);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	blobDisplay->Render(*displayShaderProgram, current_inputs);

	chat_font->UploadTextureAtlas(0);
	text_program->Use([&](){
		chat_text->Draw();
	});
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
		Physics::blob->Gui();
	
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
	/*if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);*/
	//So Barbara doesn't lose her stuff

	if (key == GLFW_KEY_G && action == GLFW_PRESS)
		bGui ^= 1;
	if (key == GLFW_KEY_F && action == GLFW_PRESS)
		Physics::dynamicsWorld->stepSimulation(Timer::deltaTime, 10);

	if (key == GLFW_KEY_DELETE && action == GLFW_PRESS)
		levelEditor->DeleteSelection();

	if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_PRESS)
		levelEditor->bCtrl = true;
	else if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_RELEASE)
		levelEditor->bCtrl = false;

	Physics::blob->Move(key, action);

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
