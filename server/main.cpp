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
#include "Skybox.h"
#include "IOBuffer.h"
#include "Level.h"
#include "BlobDisplay.h"

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

bool init();
bool init_physics();
bool init_graphics();
bool init_frameBuffers();
bool init_stream();
void update();
void draw();
void depthPass();
void dynamicCubePass();
void geometryPass();
void SSAOPass();
void blurPass();
void drawCubeFace(
		glm::vec3 position, glm::vec3 direction, glm::vec3 up, GLenum face);
void drawBlob();
void drawPlatforms();
void drawSkybox();
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

glm::mat4 modelMatrix;
glm::mat4 viewMatrix;
glm::mat4 projMatrix;
glm::mat4 lightSpaceMatrix;

IOBuffer cubeMapBuffer;
IOBuffer gBuffer;
IOBuffer aoBuffer;
IOBuffer blurBuffer;
IOBuffer depthBuffer;

GLuint noiseTexture;

Skybox skybox;
Mesh *quad;
std::vector<glm::vec3> ssaoKernel;

DirectionalLight dirLight;

StreamWriter *stream;
RakNet::RakPeerInterface *rakPeer = RakNet::RakPeerInterface::GetInstance();

btCollisionDispatcher *dispatcher;
btBroadphaseInterface *broadphase;
btSequentialImpulseConstraintSolver *solver;
btSoftBodyRigidBodyCollisionConfiguration *collisionConfiguration;
btSoftBodySolver *softBodySolver;
btSoftRigidDynamicsWorld *dynamicsWorld;

BlobDisplay *blobDisplay;
Blob *blob;
Level *level;

ShaderProgram *displayShaderProgram;
ShaderProgram *blobShaderProgram;
ShaderProgram *platformShaderProgram;
ShaderProgram *debugdrawShaderProgram;
ShaderProgram *skyboxShaderProgram;
ShaderProgram *depthShaderProgram;
ShaderProgram *geomPassShaderProgram;
ShaderProgram *SSAOShaderProgram;
ShaderProgram *blurShaderProgram;
ShaderProgram *quadShaderProgram;
ShaderProgram *lightingShaderProgram;

btSoftBodyWorldInfo softBodyWorldInfo;

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

	levelEditor = new LevelEditor(dynamicsWorld, level);

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

	delete dynamicsWorld;
	delete solver;
	delete collisionConfiguration;
	delete dispatcher;
	delete broadphase;

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
	return init_physics() && init_graphics() && init_frameBuffers() && init_stream();
}

bool init_physics()
{
	broadphase = new btDbvtBroadphase();
	btVector3 worldAabbMin(-1000, -1000, -1000);
	btVector3 worldAabbMax(1000, 1000, 1000);
	broadphase = new btAxisSweep3(worldAabbMin, worldAabbMax, MAX_PROXIES);

	collisionConfiguration = new btSoftBodyRigidBodyCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfiguration);
	solver = new btSequentialImpulseConstraintSolver();
	softBodySolver = new btDefaultSoftBodySolver();
	dynamicsWorld = new btSoftRigidDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration, softBodySolver);

	dynamicsWorld->setGravity(btVector3(0, -10, 0));

	softBodyWorldInfo.m_broadphase = broadphase;
	softBodyWorldInfo.m_dispatcher = dispatcher;
	softBodyWorldInfo.m_gravity.setValue(0, -10, 0);
	softBodyWorldInfo.air_density = (btScalar)1.2;
	softBodyWorldInfo.water_density = 0;
	softBodyWorldInfo.water_offset = 0;
	softBodyWorldInfo.water_normal = btVector3(0, 0, 0);
	softBodyWorldInfo.m_sparsesdf.Initialize();

	blob = new Blob(softBodyWorldInfo, btVector3(0, 100, 0), 3.0f, 512);
	btSoftBody *btblob = blob->softbody;

	level = Level::Deserialize(LevelDir "test_level.json");
	for(RigidBody* r : level->Objects)
		dynamicsWorld->addRigidBody(r->rigidbody);
	dynamicsWorld->addSoftBody(blob->softbody);
	dynamicsWorld->setDebugDrawer(&bulletDebugDrawer);

	return true;
}

bool init_graphics()
{
	blobDisplay = new BlobDisplay(width, height, 128);

	displayShaderProgram = new ShaderProgram({
			ShaderDir "Display.vert",
			ShaderDir "Display.frag" });

	skyboxShaderProgram = new ShaderProgram({
			ShaderDir "Skybox.vert",
			ShaderDir "Skybox.frag" });

	depthShaderProgram = new ShaderProgram({
			ShaderDir "DepthShader.vert",
			ShaderDir "DepthShader.frag" });

	blobShaderProgram = new ShaderProgram({
			ShaderDir "Blob.vert",
			ShaderDir "Blob.tesc",
			ShaderDir "Blob.tese",
			ShaderDir "Blob.frag" });
	debugdrawShaderProgram = blobShaderProgram;

	platformShaderProgram = new ShaderProgram({
			ShaderDir "Platform.vert",
			ShaderDir "Platform.frag" });

	debugdrawShaderProgram = new ShaderProgram({
			ShaderDir "Gizmo.vert",
			ShaderDir "Gizmo.frag" });

	quadShaderProgram = new ShaderProgram({
			ShaderDir "toQuad.vert",
			ShaderDir "toQuad.frag" });

	geomPassShaderProgram = new ShaderProgram({
			ShaderDir "GeometryPass.vert",
			ShaderDir "GeometryPass.frag" });

	lightingShaderProgram = new ShaderProgram({
			ShaderDir "LightingPass.vert",
			ShaderDir "LightingPass.frag" });

	SSAOShaderProgram = new ShaderProgram({
			ShaderDir "SSAO.vert",
			ShaderDir "SSAO.frag" });

	blurShaderProgram = new ShaderProgram({
			ShaderDir "SSAO.vert",
			ShaderDir "Blur.frag" });

	dirLight.color = glm::vec3(1.0f, 1.0f, 1.0f);
	dirLight.direction = glm::vec3(-0.2f, -0.5f, -0.2f);

	quad = Mesh::CreateQuad();

	skybox.buildCubeMesh();
	std::vector<const GLchar*> faces;
	faces.push_back(TextureDir "sunny_skybox/negx.png");
	faces.push_back(TextureDir "sunny_skybox/posx.png");
	faces.push_back(TextureDir "sunny_skybox/posy.png");
	faces.push_back(TextureDir "sunny_skybox/negy.png");
	faces.push_back(TextureDir "sunny_skybox/posz.png");
	faces.push_back(TextureDir "sunny_skybox/negz.png");
	skybox.loadCubeMap(faces);
	skybox.modelMat = glm::rotate(skybox.modelMat, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
	std::default_random_engine generator;
	for (GLuint i = 0; i < 64; ++i)
	{
		glm::vec3 sample(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0, 
			randomFloats(generator) 
		);

		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		GLfloat scale = GLfloat(i) / 64.0;

		// Scale samples s.t. they're more aligned to center of kernel
		scale = lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		ssaoKernel.push_back(sample);
	}

	// Noise texture
	std::vector<glm::vec3> ssaoNoise;
	for (GLuint i = 0; i < 16; i++)
	{
		// rotate around z-axis (in tangent space)
		glm::vec3 noise(
			randomFloats(generator) * 2.0 - 1.0, 
			randomFloats(generator) * 2.0 - 1.0, 
			0.0f ); 
		ssaoNoise.push_back(noise);
	}
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	modelMatrix = glm::mat4(1.f);

	flyCam = new FlyCam(glm::vec3(0.f, 3.0f, 2.f), 2.0f, 3.0f * (glm::half_pi<float>() / 60.0f));
	blobCam = new BlobCam();

	activeCam = flyCam;

	return true;
}

bool init_frameBuffers()
{
	if (!gBuffer.Init(width, height, true, GL_RGB32F))
		return false;

	if (!aoBuffer.Init(width, height, false, GL_RED))
		return false;

	if (!blurBuffer.Init(width, height, false, GL_RED))
		return false;

	if (!cubeMapBuffer.Init(TEX_WIDTH, TEX_HEIGHT, true, GL_RGB))
		return false;

	if (!depthBuffer.Init(SHADOW_WIDTH, SHADOW_HEIGHT, false, GL_DEPTH_COMPONENT))
		return false;

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
		for (RigidBody *r : level->Objects)
			r->Update();
		dynamicsWorld->stepSimulation(deltaTime, 10);
	}
	Profiler::Finish("Physics");

	blobCam->Target = convert(blob->GetCentroid());
	activeCam->Update();

	blob->Update();
}

void draw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	depthPass();
	dynamicCubePass();

	glViewport(0, 0, width, height);

	viewMatrix = activeCam->GetMatrix();
	projMatrix = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 400.f);

	geometryPass();
	SSAOPass();
	blurPass();

	// For debugging
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
	//quadShaderProgram->Use([&]() {
	//	quad->Draw();
	//});

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	(*platformShaderProgram)["directionalLight.color"] = dirLight.color;
	(*platformShaderProgram)["directionalLight.ambientColor"] = dirLight.ambientColor;
	(*platformShaderProgram)["directionalLight.direction"] = dirLight.direction;
	(*platformShaderProgram)["viewPos"] = activeCam->Position;
	(*platformShaderProgram)["screenSize"] = glm::vec2(RENDER_WIDTH, RENDER_HEIGHT);

	(*platformShaderProgram)["projection"] = projMatrix;
	(*platformShaderProgram)["view"] = viewMatrix;
	(*platformShaderProgram)["lightSpaceMat"] = lightSpaceMatrix;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthBuffer.texture0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, blurBuffer.texture0);

	GLuint uMMatrix = platformShaderProgram->GetUniformLocation("model");
	GLuint uColor = platformShaderProgram->GetUniformLocation("objectColor");
	platformShaderProgram->Use([&]() {
		level->Render(uMMatrix, uColor);
	});

	drawBlob();

	viewMatrix = glm::mat4(glm::mat3(viewMatrix));
	drawSkybox();
	viewMatrix = activeCam->GetMatrix();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	blobDisplay->Render(*displayShaderProgram, current_inputs);
	glDisable(GL_BLEND);
}

void depthPass()
{
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthBuffer.FBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

	glm::mat4 lightProjection = glm::ortho(-500.0f, 500.0f, -500.0f, 500.0f, -500.0f, 500.0f);
	glm::mat4 lightView = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), dirLight.direction, glm::vec3(0.0f, 1.0f, 0.0f));
	lightSpaceMatrix = lightProjection * lightView;

	(*depthShaderProgram)["lightSpaceMat"] = lightSpaceMatrix;
	(*depthShaderProgram)["model"] = glm::mat4();
	depthShaderProgram->Use([&](){
		blob->Render();
		GLuint uMMatrix = depthShaderProgram->GetUniformLocation("model");
		level->Render(uMMatrix, -1);
	});

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_CULL_FACE);
}


void geometryPass()
{
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.FBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	(*geomPassShaderProgram)["projection"] = projMatrix;
	(*geomPassShaderProgram)["view"] = viewMatrix;
	GLuint uMMatrix = geomPassShaderProgram->GetUniformLocation("model");
	GLuint uColor = geomPassShaderProgram->GetUniformLocation("objectColor");
	geomPassShaderProgram->Use([&](){
		level->Render(uMMatrix, uColor);
	});

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSAOPass()
{
	glBindFramebuffer(GL_FRAMEBUFFER, aoBuffer.FBO);	
	glClear(GL_COLOR_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gBuffer.texture0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gBuffer.texture1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	
	(*SSAOShaderProgram)["screenSize"] = glm::vec2(width, height);
	(*SSAOShaderProgram)["projection"] = projMatrix;
	SSAOShaderProgram->Use([&](){
		for (GLuint i = 0; i < 64; ++i)
			glUniform3fv(glGetUniformLocation(SSAOShaderProgram->program, ("samples[" + std::to_string(i) + "]").c_str()), 1, &ssaoKernel[i][0]);
	});
	
	SSAOShaderProgram->Use([&](){
		quad->Draw();
	});

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void blurPass()
{
	glBindFramebuffer(GL_FRAMEBUFFER, blurBuffer.FBO);
	glClear(GL_COLOR_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, aoBuffer.texture0);

	blurShaderProgram->Use([&](){
		quad->Draw();
	});
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void drawBlob()
{
	(*blobShaderProgram)["objectColor"] = glm::vec3(0.0f, 1.0f, 0.0f);
	(*blobShaderProgram)["directionalLight.color"] = dirLight.color;
	(*blobShaderProgram)["directionalLight.ambientColor"] = dirLight.ambientColor;
	(*blobShaderProgram)["directionalLight.direction"] = dirLight.direction;
	(*blobShaderProgram)["viewPos"] = activeCam->Position;
	(*blobShaderProgram)["blobDistance"] =
		glm::distance(convert(blob->GetCentroid()), activeCam->Position);

	(*blobShaderProgram)["projection"] = projMatrix;
	(*blobShaderProgram)["view"] = viewMatrix;
	(*blobShaderProgram)["lightSpaceMat"] = lightSpaceMatrix;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapBuffer.texture0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthBuffer.texture0);

	blobShaderProgram->Use([&](){
		blob->RenderPatches();
	});
}


void drawPlatforms()
{
	(*platformShaderProgram)["directionalLight.color"] = dirLight.color;
	(*platformShaderProgram)["directionalLight.ambientColor"] = dirLight.ambientColor;
	(*platformShaderProgram)["directionalLight.direction"] = dirLight.direction;
	(*platformShaderProgram)["viewPos"] = activeCam->Position;

	(*platformShaderProgram)["projection"] = projMatrix;
	(*platformShaderProgram)["view"] = viewMatrix;
	(*platformShaderProgram)["lightSpaceMat"] = lightSpaceMatrix;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthBuffer.texture0);

	GLuint uMMatrix = platformShaderProgram->GetUniformLocation("model");
	GLuint uColor = platformShaderProgram->GetUniformLocation("objectColor");
	platformShaderProgram->Use([&](){
		level->Render(uMMatrix, uColor);
	});
}

void drawSkybox()
{
	glDepthFunc(GL_LEQUAL);
	(*skyboxShaderProgram)["model"] = skybox.modelMat;
	(*skyboxShaderProgram)["view"] = viewMatrix;
	(*skyboxShaderProgram)["projection"] = projMatrix;

	glActiveTexture(GL_TEXTURE0);
	//(*skyboxShaderProgram)["skybox"] = 0;
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.getID());
	skyboxShaderProgram->Use([&](){ skybox.render(); });

	glDepthFunc(GL_LESS);
}

void dynamicCubePass()
{
	glBindFramebuffer(GL_FRAMEBUFFER, cubeMapBuffer.FBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, TEX_WIDTH, TEX_HEIGHT);
	projMatrix = glm::perspective(glm::radians(90.0f), (float)TEX_WIDTH / (float)TEX_HEIGHT, 0.1f, 1000.0f);

	glm::vec3 position = convert(blob->GetCentroid());
	drawCubeFace(
			position,
			glm::vec3(1.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, -1.0f, 0.0f),
			GL_TEXTURE_CUBE_MAP_POSITIVE_X);

	glClear(GL_DEPTH_BUFFER_BIT);

	drawCubeFace(position,
			glm::vec3(-1.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, -1.0f, 0.0f),
			GL_TEXTURE_CUBE_MAP_NEGATIVE_X);

	glClear(GL_DEPTH_BUFFER_BIT);

	drawCubeFace(position,
			glm::vec3(0.0f, 0.0f, 1.0f),
			glm::vec3(0.0f, -1.0f, 0.0f),
			GL_TEXTURE_CUBE_MAP_POSITIVE_Z);

	glClear(GL_DEPTH_BUFFER_BIT);

	drawCubeFace(position,
			glm::vec3(0.0f, 0.0f, -1.0f),
			glm::vec3(0.0f, -1.0f, 0.0f),
			GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);

	glClear(GL_DEPTH_BUFFER_BIT);

	drawCubeFace(position,
			glm::vec3(0.0f, 1.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 1.0f),
			GL_TEXTURE_CUBE_MAP_POSITIVE_Y);

	glClear(GL_DEPTH_BUFFER_BIT);

	drawCubeFace(position,
			glm::vec3(0.0f, -1.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, -1.0f),
			GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);

	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapBuffer.texture0);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void drawCubeFace(glm::vec3 position, glm::vec3 direction, glm::vec3 up, GLenum face)
{
	viewMatrix = glm::lookAt(position, position + direction, up);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, face, cubeMapBuffer.texture0, 0);

	drawPlatforms();
	viewMatrix = glm::mat4(glm::mat3(viewMatrix));
	drawSkybox();
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
	dynamicsWorld->debugDrawWorld();
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
						dynamicsWorld->removeRigidBody(rb->rigidbody);
					delete level;

					level = Level::Deserialize(lTheOpenFileName);
					for (RigidBody* rb : level->Objects)
						dynamicsWorld->addRigidBody(rb->rigidbody);

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
			if (ImGui::MenuItem("Platform"))
			{
				level->AddBox(glm::vec3(0), glm::quat(), glm::vec3(1),
					glm::vec4(.5f, .5f, .5f, 1.f));
				dynamicsWorld->addRigidBody(
					level->Objects[level->Objects.size()-1]->rigidbody);
			}
			if (ImGui::MenuItem("Physics Box"))
			{
				level->AddBox(glm::vec3(0), glm::quat(), glm::vec3(1),
					glm::vec4(.5f, .5f, .5f, 1.f), 1.0f);
				dynamicsWorld->addRigidBody(
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
