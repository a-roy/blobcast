#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}
#include <RakNet/MessageIdentifiers.h>
#include <RakNet/RakPeerInterface.h>

#include <iostream>
#include "GLFWProject.h"
#include "ShaderProgram.h"
#include "Text.h"
#include "BlobInput.h"

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
void drawBlob();
void drawPlatforms();
void drawSkybox();
void drawGizmos();
void gui();
void stream();
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

glm::mat4 modelMatrix;
glm::mat4 viewMatrix;
glm::mat4 projMatrix;
glm::mat4 lightSpaceMatrix;

GLuint cubeMapFBO;
GLuint depthMapFBO;
GLuint dynamicCubeMap;
GLuint depthMap;

Skybox skybox;

DirectionalLight dirLight;

AVFrame *avframe;
AVCodecContext *avctx;
AVFormatContext *avfmt;
SwsContext *swctx;
GLuint pbo;
RakNet::RakPeerInterface *rakPeer = RakNet::RakPeerInterface::GetInstance();

btCollisionDispatcher *dispatcher;
btBroadphaseInterface *broadphase;
btSequentialImpulseConstraintSolver *solver;
btSoftBodyRigidBodyCollisionConfiguration *collisionConfiguration;
btSoftBodySolver *softBodySolver;
btSoftRigidDynamicsWorld *dynamicsWorld;

Blob *blob;
std::vector<RigidBody*> rigidBodies;

ShaderProgram *blobShaderProgram;
ShaderProgram *platformShaderProgram;
ShaderProgram *debugdrawShaderProgram;
ShaderProgram *skyboxShaderProgram;
ShaderProgram *depthShaderProgram;

btSoftBodyWorldInfo softBodyWorldInfo;

btVector3 current_input;
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
	window = GLFWProject::Init("Stream Test", RENDER_WIDTH, RENDER_HEIGHT);
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

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	while (!glfwWindowShouldClose(window))
	{
		update();
		draw();
		stream();
		if(bGizmos)
			drawGizmos();
		if(bGui)
			gui();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	av_write_trailer(avfmt);
	avcodec_close(avctx);

	rakPeer->Shutdown(0);
	RakNet::RakPeerInterface::DestroyInstance(rakPeer);
	av_free(avfmt->pb);
	avformat_free_context(avfmt);
	avformat_network_deinit();
	//avcodec_free_context(&avctx);
	av_frame_free(&avframe);
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

	btSoftBody* btblob = btSoftBodyHelpers::CreateEllipsoid(softBodyWorldInfo, btVector3(0, 100, 0), btVector3(1, 1, 1) * 3, 512);
	
	//Experiment with blob variables
	btblob->m_materials[0]->m_kLST = 0.1;
	btblob->m_cfg.kDF = 1;
	btblob->m_cfg.kDP = 0.001;
	btblob->m_cfg.kPR = 2500;
	btblob->setTotalMass(30, true);
	
	rigidBodies.push_back(new RigidBody(Mesh::CreateCube(new VertexArray()), 
		glm::vec3(0, -10, 0), glm::quat(), glm::vec3(200.0f, 1.0f, 200.0f), 
		glm::vec4(0.85f, 0.85f, 0.85f, 1.0f)));

	rigidBodies.push_back(new RigidBody(Mesh::CreateCube(new VertexArray()), 
		glm::vec3(0, -9, 0), glm::quat(), glm::vec3(1.0f, 1.0f, 1.0f), 
		glm::vec4(1.0f, 0.1f, 0.1f, 1.0f), 3));
	rigidBodies.push_back(new RigidBody(Mesh::CreateCube(new VertexArray()), 
		glm::vec3(10, 10, 0), glm::quat(), glm::vec3(1.0f, 1.0f, 1.0f), 
		glm::vec4(0.1f, 0.1f, 1.0f, 1.0f), 3));

	for(RigidBody* r : rigidBodies)
		dynamicsWorld->addRigidBody(r->rigidbody);

	blob = new Blob(btblob); //Blob #2
	//blob->AddAnchor(anchor);
	dynamicsWorld->addSoftBody(blob->softbody);

	return true;
}

bool init_graphics()
{
	vao = new VertexArray();

	vera = new Font(FontDir "Vera.ttf", 48.f);
	text = new Text(vao, vera);
	text->SetText("Hello world");

	std::vector<Shader *> shaders;
	
	shaders.push_back(new Shader(ShaderDir "Text.vert", GL_VERTEX_SHADER));
	shaders.push_back(new Shader(ShaderDir "Text.frag", GL_FRAGMENT_SHADER));
	text_program = new ShaderProgram(shaders);
	for (std::size_t i = 0, n = shaders.size(); i < n; i++)
		delete shaders[i];
	shaders.clear();

	shaders.push_back(new Shader(ShaderDir "Skybox.vert", GL_VERTEX_SHADER));
	shaders.push_back(new Shader(ShaderDir "Skybox.frag", GL_FRAGMENT_SHADER));
	skyboxShaderProgram = new ShaderProgram(shaders);
	for (std::size_t i = 0, n = shaders.size(); i < n; i++)
		delete shaders[i];
	shaders.clear();

	shaders.push_back(new Shader(ShaderDir "DepthShader.vert", GL_VERTEX_SHADER));
	shaders.push_back(new Shader(ShaderDir "DepthShader.frag", GL_FRAGMENT_SHADER));
	depthShaderProgram = new ShaderProgram(shaders);
	for (std::size_t i = 0, n = shaders.size(); i < n; i++)
		delete shaders[i];
	shaders.clear();

	shaders.push_back(new Shader(ShaderDir "Blob.vert", GL_VERTEX_SHADER));
	shaders.push_back(new Shader(ShaderDir "Blob.frag", GL_FRAGMENT_SHADER));
	blobShaderProgram = new ShaderProgram(shaders);
	for (std::size_t i = 0, n = shaders.size(); i < n; i++)
		delete shaders[i];
	shaders.clear();

	shaders.push_back(new Shader(ShaderDir "Platform.vert", GL_VERTEX_SHADER));
	shaders.push_back(new Shader(ShaderDir "Platform.frag", GL_FRAGMENT_SHADER));
	platformShaderProgram = new ShaderProgram(shaders);
	for (std::size_t i = 0, n = shaders.size(); i < n; i++)
		delete shaders[i];
	shaders.clear();

	debugdrawShaderProgram = platformShaderProgram;

	dirLight.color = glm::vec3(1.0f, 1.0f, 1.0f);
	dirLight.direction = glm::vec3(1.0f, 4.0f, -1.0f);

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

	camera = new FlyCam(glm::vec3(0.f, 3.0f, -1.f), 10.0f / 60.0f, 3.0f * (glm::half_pi<float>() / 60.0f));

	return true;
}

bool init_frameBuffers()
{
	// FBO for CUBE MAP
	glGenFramebuffers(1, &cubeMapFBO);

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

	glBindFramebuffer(GL_FRAMEBUFFER, cubeMapFBO);

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

	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);

	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Depth map FBO error" << std::endl;
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
}

bool init_stream()
{
	glGenBuffers(1, &pbo);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
	avcodec_register_all();
	AVDictionary *opts = NULL;
	av_dict_set(&opts, "tune", "zerolatency", 0);
	av_dict_set(&opts, "preset", "ultrafast", 0);
	AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!codec)
		return false;
	avctx = avcodec_alloc_context3(codec);
	avctx->pix_fmt = AV_PIX_FMT_YUV420P;
	avctx->width = STREAM_WIDTH;
	avctx->height = STREAM_HEIGHT;
	avctx->gop_size = 0;
	avctx->time_base = { 1, 60 };
	if (avcodec_open2(avctx, codec, &opts) < 0)
		return false;

	avframe = av_frame_alloc();
	avframe->format = AV_PIX_FMT_YUV420P;
	avframe->width = STREAM_WIDTH;
	avframe->height = STREAM_HEIGHT;
	avframe->linesize[0] = STREAM_WIDTH;
	avframe->linesize[1] = STREAM_WIDTH / 2;
	avframe->linesize[2] = STREAM_WIDTH / 2;
	if (av_frame_get_buffer(avframe, 0) != 0)
		return false;

	int linesize_align[AV_NUM_DATA_POINTERS];
	avcodec_align_dimensions2(avctx, &avframe->width, &avframe->height, linesize_align);
	glBufferData(
			GL_PIXEL_PACK_BUFFER, width * height * 3, NULL, GL_STREAM_READ);

	av_register_all();
	avformat_network_init();
	avfmt = avformat_alloc_context();
	std::string filename = STREAM_ADDRESS;
	avfmt->oformat = av_guess_format("mpegts", 0, 0);
	filename.copy(avfmt->filename, filename.size(), 0);
	avfmt->bit_rate = 200*1024*1024;
	avfmt->start_time_realtime = AV_NOPTS_VALUE;
	AVStream *s = avformat_new_stream(avfmt, codec);
	s->time_base = { 1, 60 };
	if (s == NULL)
		return false;
	s->codec = avctx;
	AVIOContext *ioctx;
	if (avio_open2(&ioctx, filename.c_str(), AVIO_FLAG_WRITE, NULL, &opts) < 0)
		return false;
	avfmt->pb = ioctx;
	if (avformat_write_header(avfmt, &opts) != 0)
		return false;

	swctx = sws_getContext(
			width, height, AV_PIX_FMT_RGB24,
			STREAM_WIDTH, STREAM_HEIGHT, AV_PIX_FMT_YUV420P,
			SWS_BICUBIC, NULL, NULL, NULL);
	if (swctx == NULL)
		return false;

	rakPeer->Startup(100, &RakNet::SocketDescriptor(REMOTE_GAME_PORT, 0), 1);
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
	while (rakPeer->GetReceiveBufferSize() > 0)
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
}

void depthPass()
{
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

	glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 100.0f);
	glm::mat4 lightView = glm::lookAt(glm::vec3(-4.0f, 4.0f, -5.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	lightSpaceMatrix = lightProjection * lightView;

	depthShaderProgram->Install();
	depthShaderProgram->SetUniform("lightSpaceMat", lightSpaceMatrix);
	blob->Render();
	depthShaderProgram->Uninstall();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void drawBlob()
{
	blobShaderProgram->Install();
	blobShaderProgram->SetUniform("objectColor", glm::vec3(0.0f, 0.8f, 0.0f));
	blobShaderProgram->SetUniform("directionalLight.base.color", dirLight.color);
	blobShaderProgram->SetUniform("directionalLight.direction", dirLight.direction);
	blobShaderProgram->SetUniform("viewPos", camera->Position);

	//mvpMatrix = projMatrix * viewMatrix; //blob verts are already in world space
	blobShaderProgram->SetUniform("projection", projMatrix);
	blobShaderProgram->SetUniform("view", viewMatrix);
	blobShaderProgram->SetUniform("lightSpaceMat", lightSpaceMatrix);

	glActiveTexture(GL_TEXTURE0);
	blobShaderProgram->SetUniform("cubeMap", 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, dynamicCubeMap);

	blob->Render();
	blobShaderProgram->Uninstall();
}


void drawPlatforms()
{
	platformShaderProgram->Install();
	platformShaderProgram->SetUniform("directionalLight.base.color", dirLight.color);
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
	glDepthMask(GL_LESS);
}


void dynamicCubePass()
{
	glBindFramebuffer(GL_FRAMEBUFFER, cubeMapFBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, TEX_WIDTH, TEX_HEIGHT);
	projMatrix = glm::perspective(glm::radians(90.0f), (float)TEX_WIDTH / (float)TEX_HEIGHT, 0.1f, 1000.0f);

	glm::vec3 camPos(0.0f, 0.0f, 0.0f); // TO DO : BLOB CENTER

	viewMatrix = glm::lookAt(camPos, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, dynamicCubeMap, 0);
	drawPlatforms();
	drawSkybox();

	glClear(GL_DEPTH_BUFFER_BIT);

	viewMatrix = glm::lookAt(camPos, glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, dynamicCubeMap, 0);
	drawPlatforms();
	drawSkybox();

	glClear(GL_DEPTH_BUFFER_BIT);

	viewMatrix = glm::lookAt(camPos, glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, dynamicCubeMap, 0);
	drawPlatforms();
	drawSkybox();

	glClear(GL_DEPTH_BUFFER_BIT);

	viewMatrix = glm::lookAt(camPos, glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, dynamicCubeMap, 0);
	drawPlatforms();
	drawSkybox();

	glClear(GL_DEPTH_BUFFER_BIT);

	viewMatrix = glm::lookAt(camPos, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, dynamicCubeMap, 0);
	drawPlatforms();
	drawSkybox();

	glClear(GL_DEPTH_BUFFER_BIT);

	viewMatrix = glm::lookAt(camPos, glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, dynamicCubeMap, 0);
	drawPlatforms();
	drawSkybox();

	glBindTexture(GL_TEXTURE_CUBE_MAP, dynamicCubeMap);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

	ImGui::Begin("We are blobcasting live! (Right-click to hide GUI)");	
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
}

void stream()
{
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
	uint8_t *data =
		(uint8_t *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	uint8_t *const srcSlice[] = { data };
	int srcStride[] = { width * 3 };
	if (data != NULL)
		sws_scale(
				swctx,
				srcSlice, srcStride,
				0, height,
				avframe->data, avframe->linesize);
	avframe->pts += 1500;
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	AVPacket *avpkt = av_packet_alloc();
	int got_packet;
	if (avcodec_encode_video2(avctx, avpkt, avframe, &got_packet) < 0)
		exit(1);
	if (got_packet == 1)
		av_write_frame(avfmt, avpkt);
	av_packet_free(&avpkt);
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
