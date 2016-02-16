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

#include <iostream>
#include "GLFWProject.h"
#include "ShaderProgram.h"
#include "Text.h"

#include "SoftBody.h"

#define RootDir "../../"
#define ShaderDir RootDir "shaders/"
#define FontDir RootDir "fonts/"

#define MAX_PROXIES 32766

bool init();
void update();
void draw();
void stream();
void key_callback(
		GLFWwindow *window, int key, int scancode, int action, int mods);

GLFWwindow *window;
VertexArray *vao;
ShaderProgram *program;
Font *vera;
Text *text;
int width, height;

glm::mat4 modelMatrix;
glm::mat4 projMatrix;

AVFrame *avframe;
AVCodecContext *avctx;
AVFormatContext *avfmt;
SwsContext *swctx;
GLuint pbo;

btCollisionDispatcher *dispatcher;
btBroadphaseInterface *broadphase;
btSequentialImpulseConstraintSolver *solver;
btSoftBodyRigidBodyCollisionConfiguration *collisionConfiguration;
btSoftBodySolver *softBodySolver;
btSoftRigidDynamicsWorld *dynamicsWorld;

SoftBody *blob;
btRigidBody *groundRigidBody;

ShaderProgram *blobShaderProgram;

btSoftBodyWorldInfo softBodyWorldInfo;

double currentFrame = glfwGetTime();
double lastFrame = currentFrame;
double deltaTime;

int main(int argc, char *argv[])
{
	window = GLFWProject::Init("Stream Test", 800, 600);
	if (!window)
		return 1;

	glfwSetKeyCallback(window, key_callback);
	glfwGetFramebufferSize(window, &width, &height);
	glfwSwapInterval(1);

	if (!init())
		return 1;

	GLuint uMVPMatrix = program->GetUniformLocation("uMVPMatrix");
	GLuint uAtlas = program->GetUniformLocation("uAtlas");
	GLuint uTextColor = program->GetUniformLocation("uTextColor");

	program->Install();
	glUniform4f(uTextColor, 0.5f, 1.0f, 1.0f, 1.0f);
	vera->BindTexture(uAtlas);
	program->Uninstall();

	glEnable(GL_DEPTH_TEST);

	while (!glfwWindowShouldClose(window))
	{
		update();
		draw();
		stream();
		glfwPollEvents();
	}
	av_write_trailer(avfmt);
	avcodec_close(avctx);

	av_free(avfmt->pb);
	avformat_free_context(avfmt);
	avformat_network_deinit();
	//avcodec_free_context(&avctx);
	av_frame_free(&avframe);
	delete program;
	delete text;
	delete vera;

	delete dynamicsWorld;
	delete solver;
	delete collisionConfiguration;
	delete dispatcher;
	delete broadphase;

	glfwTerminate();
	return 0;
}

bool init()
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

	softBodyWorldInfo.m_sparsesdf.Initialize();

	softBodyWorldInfo.air_density = (btScalar)1.2;
	softBodyWorldInfo.water_density = 0;
	softBodyWorldInfo.water_offset = 0;
	softBodyWorldInfo.water_normal = btVector3(0, 0, 0);

	blob = new SoftBody(btSoftBodyHelpers::CreateEllipsoid(softBodyWorldInfo, btVector3(0, 0, 0), btVector3(1, 1, 1) * 3, 512));
	dynamicsWorld->addSoftBody(blob->softbody);

	btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), -4.5f);
	btDefaultMotionState* groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, -1, 0)));
	btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0, groundMotionState, groundShape, btVector3(0, 0, 0));
	groundRigidBody = new btRigidBody(groundRigidBodyCI);

	dynamicsWorld->addRigidBody(groundRigidBody);

	vao = new VertexArray();

	vera = new Font(FontDir "Vera.ttf", 48.f);
	text = new Text(vao, vera);
	text->SetText("Hello world");

	std::vector<Shader *> shaders;
	shaders.push_back(new Shader(ShaderDir "Text.vert", GL_VERTEX_SHADER));
	shaders.push_back(new Shader(ShaderDir "Text.frag", GL_FRAGMENT_SHADER));
	program = new ShaderProgram(shaders);
	for (std::size_t i = 0, n = shaders.size(); i < n; i++)
	{
		delete shaders[i];
	}
	shaders.clear();

	shaders.push_back(new Shader(ShaderDir "Blob.vert", GL_VERTEX_SHADER));
	shaders.push_back(new Shader(ShaderDir "Blob.frag", GL_FRAGMENT_SHADER));
	blobShaderProgram = new ShaderProgram(shaders);
	for (std::size_t i = 0, n = shaders.size(); i < n; i++)
	{
		delete shaders[i];
	}
	shaders.clear();

	projMatrix = glm::ortho(
			-(float)width * 0.5f, (float)width * 0.5f,
			-(float)height * 0.5f, (float)height * 0.5f);
	modelMatrix = glm::mat4(1.f);

	glGenBuffers(1, &pbo);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
	glBufferData(
			GL_PIXEL_PACK_BUFFER, width * height * 3, NULL, GL_STREAM_READ);
	avframe = av_frame_alloc();
	avframe->format = AV_PIX_FMT_YUV444P;
	avframe->width = width;
	avframe->height = height;
	avframe->linesize[0] = width;
	avframe->linesize[1] = width;
	avframe->linesize[2] = width;
	if (av_frame_get_buffer(avframe, 0) != 0)
		return false;

	avcodec_register_all();
	AVDictionary *opts = NULL;
	av_dict_set(&opts, "tune", "zerolatency", 0);
	av_dict_set(&opts, "preset", "ultrafast", 0);
	AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!codec)
		return false;
	avctx = avcodec_alloc_context3(codec);
	avctx->pix_fmt = AV_PIX_FMT_YUV444P;
	avctx->width = width;
	avctx->height = height;
	avctx->gop_size = 0;
	avctx->time_base = { 1, 60 };
	if (avcodec_open2(avctx, codec, &opts) < 0)
		return false;

	av_register_all();
	avformat_network_init();
	avfmt = avformat_alloc_context();
	std::string filename = "udp://236.0.0.1:2000";
	avfmt->oformat = av_guess_format("mpegts", 0, 0);
	filename.copy(avfmt->filename, filename.size(), 0);
	avfmt->bit_rate = 8000000;
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
			width, height, AV_PIX_FMT_YUV444P,
			0, NULL, NULL, NULL);
	if (swctx == NULL)
		return false;

	return true;
}

void update()
{
	currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	modelMatrix = glm::rotate(0.004f, glm::vec3(0, 0, 1)) * modelMatrix;

	dynamicsWorld->stepSimulation(deltaTime/**.001f*/, 10);

	blob->Update();
}

void draw()
{
	glm::mat4 mvpMatrix = projMatrix * modelMatrix;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GLuint uMVPMatrix = program->GetUniformLocation("uMVPMatrix");
	program->Install();
	glUniformMatrix4fv(uMVPMatrix, 1, GL_FALSE, &mvpMatrix[0][0]);
	text->Draw();
	program->Uninstall();

	mvpMatrix = projMatrix;

	uMVPMatrix = blobShaderProgram->GetUniformLocation("uMVPMatrix");
	blobShaderProgram->Install();
	glUniformMatrix4fv(uMVPMatrix, 1, GL_FALSE, &mvpMatrix[0][0]);
	blob->Render();

	glfwSwapBuffers(window);
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
}
