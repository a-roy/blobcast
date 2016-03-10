#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <RakNet/MessageIdentifiers.h>
#include <RakNet/RakPeerInterface.h>

#include <iostream>
#include "GLFWProject.h"
#include "ShaderProgram.h"
#include "Text.h"
#include "BlobInput.h"
#include "StreamWriter.h"

#include "SoftBody.h"
#include "Blob.h"
#include "RigidBody.h"
#include "Light.hpp"
#include "Skybox.h"
#include "Level.h"

#include "config.h"

#include <imgui.h>
#include "imgui_impl_glfw.h"

#include "Point.h"
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
VertexArray *vao;
ShaderProgram *text_program;
Font *vera;
Text *text;
int width, height;

FloatBuffer *display_vbo;

glm::mat4 modelMatrix;
glm::mat4 viewMatrix;
glm::mat4 projMatrix;
glm::mat4 lightSpaceMatrix;

GLuint cubeMapFBO;
GLuint depthMapFBO;
GLuint gBuffer;
GLuint dynamicCubeMap;
GLuint depthMap;

Skybox skybox;

DirectionalLight dirLight;

StreamWriter *stream;
RakNet::RakPeerInterface *rakPeer = RakNet::RakPeerInterface::GetInstance();

btCollisionDispatcher *dispatcher;
btBroadphaseInterface *broadphase;
btSequentialImpulseConstraintSolver *solver;
btSoftBodyRigidBodyCollisionConfiguration *collisionConfiguration;
btSoftBodySolver *softBodySolver;
btSoftRigidDynamicsWorld *dynamicsWorld;

Blob *blob;
std::vector<RigidBody*> rigidBodies;
Level *level;

ShaderProgram *displayShaderProgram;
ShaderProgram *blobShaderProgram;
ShaderProgram *platformShaderProgram;
ShaderProgram *debugdrawShaderProgram;
ShaderProgram *skyboxShaderProgram;
ShaderProgram *depthShaderProgram;

btSoftBodyWorldInfo softBodyWorldInfo;

double currentFrame = glfwGetTime();
double lastFrame = currentFrame;
double deltaTime;

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

	levelEditor = new LevelEditor(dynamicsWorld);

	GLuint uMVPMatrix = text_program->GetUniformLocation("uMVPMatrix");
	GLuint uAtlas = text_program->GetUniformLocation("uAtlas");
	GLuint uTextColor = text_program->GetUniformLocation("uTextColor");

	text_program->Install();
	glUniform4f(uTextColor, 0.5f, 1.0f, 1.0f, 1.0f);
	vera->BindTexture(uAtlas);
	text_program->Uninstall();

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

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
	delete text_program;
	delete text;
	delete vera;

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

	blob = new Blob(softBodyWorldInfo, btVector3(0, 100, 0), 3.0f, 160);
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
	vao = new VertexArray();
	display_vbo = new FloatBuffer(vao, 2, 4);
	GLfloat *vertex_data = new GLfloat[8] { -1, -1, -1, 1, 1, -1, 1, 1 };
	display_vbo->SetData(vertex_data);

	vera = new Font(FontDir "Vera.ttf", 48.f);
	text = new Text(vao, vera);
	text->SetText("Hello world");

	text_program = new ShaderProgram(2,
			ShaderDir "Text.vert",
			ShaderDir "Text.frag");

	displayShaderProgram = new ShaderProgram(2,
			ShaderDir "Display.vert",
			ShaderDir "Display.frag");

	skyboxShaderProgram = new ShaderProgram(2,
			ShaderDir "Skybox.vert",
			ShaderDir "Skybox.frag");

	depthShaderProgram = new ShaderProgram(2,
			ShaderDir "DepthShader.vert",
			ShaderDir "DepthShader.frag");

	blobShaderProgram = new ShaderProgram(4,
			ShaderDir "Blob.vert",
			ShaderDir "Blob.tesc",
			ShaderDir "Blob.tese",
			ShaderDir "Blob.frag");

	platformShaderProgram = new ShaderProgram(2,
			ShaderDir "Platform.vert",
			ShaderDir "Platform.frag");

	debugdrawShaderProgram = new ShaderProgram(2,
			ShaderDir "Gizmo.vert",
			ShaderDir "Gizmo.frag");

	dirLight.color = glm::vec3(1.0f, 1.0f, 1.0f);
	dirLight.direction = glm::vec3(-5.0f, 5.0f, -5.0f);

	skybox.buildCubeMesh();
	std::vector<const GLchar*> faces;
	faces.push_back(TextureDir "skybox/posx.jpg");
	faces.push_back(TextureDir "skybox/negx.jpg");
	faces.push_back(TextureDir "skybox/posy.jpg");
	faces.push_back(TextureDir "skybox/negy.jpg");
	faces.push_back(TextureDir "skybox/posz.jpg");
	faces.push_back(TextureDir "skybox/negz.jpg");
	skybox.loadCubeMap(faces);

	//projMatrix = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 300.f);

	modelMatrix = glm::mat4(1.f);

	flyCam = new FlyCam(glm::vec3(0.f, 3.0f, 2.f), 2.0f, 3.0f * (glm::half_pi<float>() / 60.0f));
	blobCam = new BlobCam();

	activeCam = flyCam;

	return true;
}

// TO DO : move buffer creation out of main
bool init_frameBuffers()
{
	// FBO for CUBE MAP
	glGenFramebuffers(1, &cubeMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, cubeMapFBO);

	glGenTextures(1, &dynamicCubeMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, dynamicCubeMap);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	for (int i = 0; i<6; i++) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
			TEX_WIDTH, TEX_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	}

	GLuint depthRenderBuffer;
	glGenRenderbuffers(1, &depthRenderBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, TEX_WIDTH, TEX_WIDTH);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderBuffer);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Cube map FBO error" << std::endl;
		return false;
	}	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// FBO for DEPTH MAP
	glGenFramebuffers(1, &depthMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);

	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Depth map FBO error" << std::endl;
		return false;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// FBO for GEOMETRY PASS
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	GLuint gPositionDepth, gNormal, gAlbedo;
	// Position + lin depth color buffer
	glGenTextures(1, &gPositionDepth);
	glBindTexture(GL_TEXTURE_2D, gPositionDepth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, RENDER_WIDTH, RENDER_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPositionDepth, 0);
	// Normal color buffer
	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, RENDER_WIDTH, RENDER_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
	// Albedo color buffer
	glGenTextures(1, &gAlbedo);
	glBindTexture(GL_TEXTURE_2D, gAlbedo);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, RENDER_WIDTH, RENDER_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);

	GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	GLuint rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, RENDER_WIDTH, RENDER_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Geometry frame buffer not complete" << std::endl;
		return false;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
	btVector3 cum_input(0, 0, 0);
	float num_inputs = 0.f;
	int forward_count = 0;
	int backward_count = 0;
	int right_count = 0;
	int left_count = 0;
	int jump_count = 0;
	while (stream->IsOpen() && rakPeer->GetReceiveBufferSize() > 0)
	{
		RakNet::Packet *p = rakPeer->Receive();
		unsigned char packet_type = p->data[0];
		if (packet_type == ID_USER_PACKET_ENUM)
		{
			BlobInput i = (BlobInput)p->data[1];
			if (i & Forward)
				forward_count++;
			if (i & Backward)
				backward_count++;
			if (i & Right)
				right_count++;
			if (i & Left)
				left_count++;
			if (i & Jump)
				jump_count++;
			num_inputs += 1.f;
		}
	}
	if (num_inputs > 0.f)
		cum_input /= num_inputs;

	if (blob->movementMode == MovementMode::averaging)
	{
		blob->AddForce(btVector3(left_count - right_count, jump_count,
			forward_count - backward_count) / cum_input);
	}
	else if (blob->movementMode == MovementMode::stretch)
	{
		blob->AddForces(forward_count / num_inputs, backward_count / num_inputs,
			left_count / num_inputs, right_count / num_inputs);
		blob->AddForce(btVector3(0, 1, 0) * jump_count / num_inputs);
	}
	if (num_inputs > 0.f)
	{
		displayShaderProgram->Install();
		displayShaderProgram->SetUniform("uForward", forward_count / num_inputs);
		displayShaderProgram->SetUniform("uBackward", backward_count / num_inputs);
		displayShaderProgram->SetUniform("uRight", right_count / num_inputs);
		displayShaderProgram->SetUniform("uLeft", left_count / num_inputs);
		displayShaderProgram->Uninstall();
	}

	modelMatrix = glm::rotate(0.004f, glm::vec3(0, 0, 1)) * modelMatrix;

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
		dynamicsWorld->stepSimulation(deltaTime, 10);
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

	glViewport(0, 0, RENDER_WIDTH, RENDER_HEIGHT);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 mvpMatrix;
	viewMatrix = activeCam->GetMatrix();
	projMatrix = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 300.f);

	//text_program->Install();
	//mvpMatrix = projMatrix * viewMatrix * modelMatrix;
	//text_program->SetUniform("uMVPMatrix", mvpMatrix);
	//text->Draw();
	//text_program->Uninstall();

	drawBlob();

	drawPlatforms();

	viewMatrix = glm::mat4(glm::mat3(viewMatrix));
	drawSkybox();
	viewMatrix = activeCam->GetMatrix();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glm::mat4 displayMVP = glm::ortho(
			-1.25f, ((float)width  / 128.f) - 1.25f,
			-1.25f, ((float)height / 128.f) - 1.25f);
	displayShaderProgram->Install();
	displayShaderProgram->SetUniform("uMVPMatrix", displayMVP);
	displayShaderProgram->SetUniform("uInnerRadius", 0.7f);
	displayShaderProgram->SetUniform("uOuterRadius", 0.9f);
	glEnableVertexAttribArray(0);
	display_vbo->BufferData(0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisableVertexAttribArray(0);
	displayShaderProgram->Uninstall();
	glDisable(GL_BLEND);
}

void depthPass()
{
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

	glm::mat4 lightProjection = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, -100.0f, 100.0f);
	glm::mat4 lightView = glm::lookAt(dirLight.direction, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	lightSpaceMatrix = lightProjection * lightView;

	depthShaderProgram->Install();
	depthShaderProgram->SetUniform("lightSpaceMat", lightSpaceMatrix);
	depthShaderProgram->SetUniform("model", glm::mat4());
	blob->Render();
	GLuint uMMatrix = depthShaderProgram->GetUniformLocation("model");
	glFrontFace(GL_CW);
	level->Render(uMMatrix, -1);
	for (int i = 0; i < rigidBodies.size(); i++) {
		depthShaderProgram->SetUniform("model", rigidBodies[i]->GetModelMatrix());
		rigidBodies[i]->Render();
	}
	glFrontFace(GL_CCW);
	depthShaderProgram->Uninstall();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_CULL_FACE);
}

void drawBlob()
{
	blobShaderProgram->Install();
	blobShaderProgram->SetUniform("objectColor", glm::vec3(0.0f, 1.0f, 0.0f));
	blobShaderProgram->SetUniform("directionalLight.color", dirLight.color);
	blobShaderProgram->SetUniform("directionalLight.ambientColor", dirLight.ambientColor);
	blobShaderProgram->SetUniform("directionalLight.direction", dirLight.direction);
	blobShaderProgram->SetUniform("viewPos", activeCam->Position);
	blobShaderProgram->SetUniform("blobDistance",
			glm::distance(convert(blob->GetCentroid()), activeCam->Position));

	//mvpMatrix = projMatrix * viewMatrix; //blob verts are already in world space
	blobShaderProgram->SetUniform("projection", projMatrix);
	blobShaderProgram->SetUniform("view", viewMatrix);
	blobShaderProgram->SetUniform("lightSpaceMat", lightSpaceMatrix);

	glActiveTexture(GL_TEXTURE0);
	blobShaderProgram->SetUniform("cubeMap", 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, dynamicCubeMap);

	glActiveTexture(GL_TEXTURE1);
	blobShaderProgram->SetUniform("depthMap", 1);
	glBindTexture(GL_TEXTURE_2D, depthMap);

	blob->RenderPatches();
	blobShaderProgram->Uninstall();
}


void drawPlatforms()
{
	platformShaderProgram->Install();
	platformShaderProgram->SetUniform("directionalLight.color", dirLight.color);
	platformShaderProgram->SetUniform("directionalLight.ambientColor", dirLight.ambientColor);
	platformShaderProgram->SetUniform("directionalLight.direction", dirLight.direction);
	platformShaderProgram->SetUniform("viewPos", activeCam->Position);

	platformShaderProgram->SetUniform("projection", projMatrix);
	platformShaderProgram->SetUniform("view", viewMatrix);
	platformShaderProgram->SetUniform("lightSpaceMat", lightSpaceMatrix);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthMap);


	GLuint uMMatrix = platformShaderProgram->GetUniformLocation("model");
	GLuint uColor = platformShaderProgram->GetUniformLocation("objectColor");
	level->Render(uMMatrix, uColor);
	for (RigidBody* r : rigidBodies) {
		platformShaderProgram->SetUniform("model", r->GetModelMatrix());
		platformShaderProgram->SetUniform("objectColor", r->color);
		r->Render();
	}

	platformShaderProgram->Uninstall();
}

void drawSkybox()
{
	glDepthFunc(GL_LEQUAL);
	skyboxShaderProgram->Install();
	skyboxShaderProgram->SetUniform("view", viewMatrix);
	skyboxShaderProgram->SetUniform("projection", projMatrix);

	glActiveTexture(GL_TEXTURE0);
	skyboxShaderProgram->SetUniform("skybox", 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.getID());
	skybox.render();

	skyboxShaderProgram->Uninstall();
	glDepthFunc(GL_LESS);
}

void dynamicCubePass()
{
	glBindFramebuffer(GL_FRAMEBUFFER, cubeMapFBO);
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

	glBindTexture(GL_TEXTURE_CUBE_MAP, dynamicCubeMap);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void drawCubeFace(
		glm::vec3 position, glm::vec3 direction, glm::vec3 up, GLenum face)
{
	viewMatrix = glm::lookAt(position, position + direction, up);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, face, dynamicCubeMap, 0);

	drawPlatforms();
	viewMatrix = glm::mat4(glm::mat3(viewMatrix));
	drawSkybox();
}


void drawGizmos()
{
	debugdrawShaderProgram->Install();

	glm::mat4 mvpMatrix = projMatrix * activeCam->GetMatrix();
	debugdrawShaderProgram->SetUniform("uMVPMatrix", mvpMatrix);

	debugdrawShaderProgram->SetUniform("uColor", glm::vec4(1, 0, 0, 1));
	Line x(glm::vec3(0, 0, 0), glm::vec3(1, 0, 0));
	x.Render();
	debugdrawShaderProgram->SetUniform("uColor", glm::vec4(0, 1, 0, 1));
	Line y(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	y.Render();
	debugdrawShaderProgram->SetUniform("uColor", glm::vec4(0, 0, 1, 1));
	Line z(glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));
	z.Render();

	Point p(glm::vec3(0));
	debugdrawShaderProgram->SetUniform("uColor", glm::vec4(0, 0, 0, 1));
	p.Render(1.0f/glm::distance(activeCam->Position, glm::vec3(0)) * 50.0f);

	//Line ray(levelEditor->out_origin, levelEditor->out_end);
	//ray.Render();

	//blob->DrawGizmos(debugdrawShaderProgram);
	debugdrawShaderProgram->Uninstall();
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
				//TODO - don't save with selection color!
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

	debugdrawShaderProgram->Install();
	glm::mat4 mvpMatrix = projMatrix * activeCam->GetMatrix();
	debugdrawShaderProgram->SetUniform("uMVPMatrix", mvpMatrix);
	levelEditor->Gui(debugdrawShaderProgram);
	debugdrawShaderProgram->Uninstall();

	ImGui::Render();
	glDisable(GL_SCISSOR_TEST);
	glEnable(GL_DEPTH_TEST);
}

void key_callback(
		GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	
	if (key == GLFW_KEY_G && action == GLFW_PRESS)
		bGui ^= 1;

	if (key == GLFW_KEY_DELETE && action == GLFW_PRESS)
	{
		for (auto rb : levelEditor->selection)
		{
			dynamicsWorld->removeRigidBody(rb->rigidbody);
			level->Delete(level->Find(rb->rigidbody));
			delete rb;
		}
		levelEditor->selection.clear();
	}

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
				levelEditor->Mouse(xcursor, height - ycursor, width, height, viewMatrix, projMatrix);
	}
}
