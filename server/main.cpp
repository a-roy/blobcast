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

#include "config.h"

#include <imgui.h>
#include "imgui_impl_glfw.h"

#include "Line.h"

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

Camera *camera;

double xcursor, ycursor;
bool bGui = true;
bool bShowBlobCfg = true;

bool bGizmos = true;

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

	GLuint uMVPMatrix = text_program->GetUniformLocation("uMVPMatrix");
	GLuint uAtlas = text_program->GetUniformLocation("uAtlas");
	GLuint uTextColor = text_program->GetUniformLocation("uTextColor");

	text_program->Install();
	glUniform4f(uTextColor, 0.5f, 1.0f, 1.0f, 1.0f);
	vera->BindTexture(uAtlas);
	text_program->Uninstall();

	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	while (!glfwWindowShouldClose(window))
	{
		update();
		draw();
		if (stream->IsOpen())
			stream->WriteFrame();
		if(bGizmos)
			drawGizmos();
		if(bGui)
			gui();

		glfwSwapBuffers(window);
		glfwPollEvents();
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

	ImGui_ImplGlfw_Shutdown();
	glfwTerminate();
	return 0;
}

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

	//Experiment with environment variables
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

	//Experiment with blob variables
	btblob->m_materials[0]->m_kLST = 0.1;
	btblob->m_cfg.kDF = 1;
	btblob->m_cfg.kDP = 0.001;
	btblob->m_cfg.kPR = 2500;
	btblob->setTotalMass(30, true);
	
	rigidBodies.push_back(new RigidBody(Mesh::CreateCubeWithNormals(new VertexArray()), 
		glm::vec3(0, -10, 0), glm::quat(), glm::vec3(50.0f, 5.0f, 50.0f), 
		glm::vec4(0.85f, 0.85f, 0.85f, 1.0f)));

	rigidBodies.push_back(new RigidBody(Mesh::CreateCubeWithNormals(new VertexArray()),
		glm::vec3(0, -9, 0), glm::quat(), glm::vec3(1.0f, 1.0f, 1.0f), 
		glm::vec4(1.0f, 0.1f, 0.1f, 1.0f), 3));
	rigidBodies.push_back(new RigidBody(Mesh::CreateCubeWithNormals(new VertexArray()),
		glm::vec3(-10, 10, 0), glm::quat(), glm::vec3(5.0f, 5.0f, 5.0f), 
		glm::vec4(0.1f, 0.1f, 1.0f, 1.0f), 3));
	rigidBodies.push_back(new RigidBody(Mesh::CreateCubeWithNormals(new VertexArray()),
		glm::vec3(5, -5, 0), glm::quat(), glm::vec3(2.0f, 2.0f, 2.0f), 
		glm::vec4(1.0f, 0.8f, 0.1f, 1.0f), 3));

	for(RigidBody* r : rigidBodies)
		dynamicsWorld->addRigidBody(r->rigidbody);

	//blob->AddAnchor(anchor);
	dynamicsWorld->addSoftBody(blob->softbody);

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

	text_program = new ShaderProgram({
			ShaderDir "Text.vert",
			ShaderDir "Text.frag" });

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

	platformShaderProgram = new ShaderProgram({
			ShaderDir "Platform.vert",
			ShaderDir "Platform.frag" });

	debugdrawShaderProgram = new ShaderProgram({
			ShaderDir "Gizmo.vert",
			ShaderDir "Gizmo.frag" });

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

	camera = new FlyCam(glm::vec3(0.f, 3.0f, 2.f), 10.0f / 60.0f, 3.0f * (glm::half_pi<float>() / 60.0f));

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

	currentFrame = glfwGetTime();

	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	modelMatrix = glm::rotate(0.004f, glm::vec3(0, 0, 1)) * modelMatrix;

	dynamicsWorld->stepSimulation(deltaTime, 10);

	camera->Update();

	blob->Update();

	for (RigidBody* r : rigidBodies)
		r->Update();
}

bool show_test_window = true;

void draw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	depthPass();
	dynamicCubePass();

	glViewport(0, 0, RENDER_WIDTH, RENDER_HEIGHT);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 mvpMatrix;
	viewMatrix = camera->GetMatrix();
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
	viewMatrix = camera->GetMatrix();

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
	glFrontFace(GL_CW);
	for (int i = 1; i < rigidBodies.size(); i++) {
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
	blobShaderProgram->SetUniform("viewPos", camera->Position);
	blobShaderProgram->SetUniform("blobDistance",
			glm::distance(convert(blob->GetCentroid()), camera->Position));

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
	platformShaderProgram->SetUniform("viewPos", camera->Position);

	platformShaderProgram->SetUniform("projection", projMatrix);
	platformShaderProgram->SetUniform("view", viewMatrix);
	platformShaderProgram->SetUniform("lightSpaceMat", lightSpaceMatrix);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthMap);


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

	glm::mat4 mvpMatrix = projMatrix * camera->GetMatrix();
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

	blob->DrawGizmos(debugdrawShaderProgram);

	debugdrawShaderProgram->Uninstall();
}

void gui()
{
	ImGui_ImplGlfw_NewFrame();

	if (stream->IsOpen())
		ImGui::Begin("We are blobcasting live! (Right-click to hide GUI)");
	else
		ImGui::Begin("Blobcast server unavailable (Right-click to hide GUI)");
	if (ImGui::Button("Show/Hide Blob Edtior")) bShowBlobCfg ^= 1;
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::Checkbox("Show Gizmos", &bGizmos);
	ImGui::End();

	if (bShowBlobCfg)
	{
#pragma region Bullet Softbody cfg variables not yet exposed
		//eAeroModel::_			aeromodel;		// Aerodynamic model (default: V_Point)
		//btScalar				kVCF;			// Velocities correction factor (Baumgarte)	
		//btScalar				kCHR;			// Rigid contacts hardness [0,1]
		//btScalar				kKHR;			// Kinetic contacts hardness [0,1]
		//btScalar				kSHR;			// Soft contacts hardness [0,1]
		//btScalar				kAHR;			// Anchors hardness [0,1]

		//btScalar				kSRHR_CL;		// Soft vs rigid hardness [0,1] (cluster only)
		//btScalar				kSKHR_CL;		// Soft vs kinetic hardness [0,1] (cluster only)
		//btScalar				kSSHR_CL;		// Soft vs soft hardness [0,1] (cluster only)
		//btScalar				kSR_SPLT_CL;	// Soft vs rigid impulse split [0,1] (cluster only)
		//btScalar				kSK_SPLT_CL;	// Soft vs rigid impulse split [0,1] (cluster only)
		//btScalar				kSS_SPLT_CL;	// Soft vs rigid impulse split [0,1] (cluster only)

		//btScalar				maxvolume;		// Maximum volume ratio for pose
		//btScalar				timescale;		// Time scale
		//int						viterations;	// Velocities solver iterations
		//int						piterations;	// Positions solver iterations
		//int						diterations;	// Drift solver iterations
		//int						citerations;	// Cluster solver iterations
		//int						collisions;		// Collisions flags
		//tVSolverArray			m_vsequence;	// Velocity solvers sequence
		//tPSolverArray			m_psequence;	// Position solvers sequence
		//tPSolverArray			m_dsequence;	// Drift solvers sequence
#pragma endregion

		ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiSetCond_FirstUseEver);
		ImGui::Begin("Blob Edtior", &bShowBlobCfg);
		ImGui::SliderFloat("Rigid Contacts Hardness [0,1]", &blob->softbody->m_cfg.kCHR, 0.0f, 1.0f);
		ImGui::SliderFloat("Dynamic Friction Coefficient [0,1]", &blob->softbody->m_cfg.kDF, 0.0f, 1.0f);
		ImGui::InputFloat("Pressure coefficient [-inf,+inf]", &blob->softbody->m_cfg.kPR, 1.0f, 100.0f);
		ImGui::InputFloat("Volume conversation coefficient [0, +inf]", &blob->softbody->m_cfg.kVC, 1.0f, 100.0f);
		ImGui::InputFloat("Drag coefficient [0, +inf]", &blob->softbody->m_cfg.kDG, 1.0f, 100.0f);
		ImGui::SliderFloat("Damping coefficient [0,1]", &blob->softbody->m_cfg.kDP, 0.0f, 1.0f);
		ImGui::InputFloat("Lift coefficient [0,+inf]", &blob->softbody->m_cfg.kLF, 1.0f, 100.0f);
		ImGui::SliderFloat("Pose matching coefficient [0,1]", &blob->softbody->m_cfg.kMT, 0.0f, 1.0f);
		ImGui::InputFloat("Movement force", &blob->speed, 0.1f, 100.0f);
		
		ImGui::End();
	}

	ImGui::Render();
	glDisable(GL_SCISSOR_TEST);
	glEnable(GL_DEPTH_TEST);
}

void key_callback(
		GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	blob->Move(key, action);

	GLFWProject::WASDStrafe(camera, window, key, scancode, action, mods);
}

void cursor_pos_callback(
	GLFWwindow *window, double xpos, double ypos)
{
	GLFWProject::MouseTurn(camera, &xcursor, &ycursor, window, xpos, ypos);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
		bGui = !bGui;
	GLFWProject::ClickDisablesCursor(&xcursor, &ycursor, window, button, action, mods);
}
