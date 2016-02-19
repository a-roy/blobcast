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
#include <RakNet/RakNetTypes.h>

#include <iostream>
#include <sstream>
#include "GLFWProject.h"
#include "ShaderProgram.h"
#include "Buffer.h"
#include "Text.h"
#include "BlobInput.h"
#include "HostData.h"

#include "config.h"

bool init();
void update();
void draw();
void key_callback(
		GLFWwindow *window, int key, int scancode, int action, int mods);

GLFWwindow *window;
VertexArray *vao;
FloatBuffer *vbo;
Text *input_display;
Font *vera;
ShaderProgram *stream_program;
ShaderProgram *text_program;
int width, height;
uint8_t *data;
GLuint tex;

AVFormatContext *avfmt = NULL;
AVCodecContext *avctx = NULL;
AVFrame *avframe = NULL;
SwsContext *swctx = NULL;

RakNet::RakPeerInterface *rakPeer = RakNet::RakPeerInterface::GetInstance();
RakNet::SystemAddress hostAddress = RakNet::UNASSIGNED_SYSTEM_ADDRESS;
BlobInput current_input;

int main(int argc, char *argv[])
{
	window = GLFWProject::Init("Client Test", 1600, 900);
	if (!window)
		return 1;

	glfwSetKeyCallback(window, key_callback);
	glfwGetFramebufferSize(window, &width, &height);

	if (!init())
		return 1;

	while (!glfwWindowShouldClose(window))
	{
		update();
		draw();
		glfwPollEvents();
	}
	free(data);
	rakPeer->Shutdown(0);
	RakNet::RakPeerInterface::DestroyInstance(rakPeer);
	avcodec_close(avctx);
	avcodec_free_context(&avctx);
	avformat_network_deinit();
	av_frame_free(&avframe);
	delete stream_program;
	delete text_program;
	delete input_display;
	delete vera;
	delete vbo;
	delete vao;

	glfwTerminate();
	return 0;
}

bool init()
{
	vao = new VertexArray();
	vbo = new FloatBuffer(vao, 2, 4);
	GLfloat *vertex_data = new GLfloat[8] { -1, -1, -1, 1, 1, -1, 1, 1 };
	vbo->SetData(vertex_data);

	glGenTextures(1, &tex);
	vera = new Font(FontDir "Vera.ttf", 24.f);
	input_display = new Text(vao, vera);
	input_display->XPosition = 8;
	input_display->YPosition = 8;
	input_display->SetText("Inputs:");

	std::vector<Shader *> shaders;
	shaders.push_back(new Shader(ShaderDir "Stream.vert", GL_VERTEX_SHADER));
	shaders.push_back(new Shader(ShaderDir "Stream.frag", GL_FRAGMENT_SHADER));
	stream_program = new ShaderProgram(shaders);
	for (std::size_t i = 0, n = shaders.size(); i < n; i++)
	{
		delete shaders[i];
	}
	shaders.clear();

	shaders.push_back(new Shader(ShaderDir "Text.vert", GL_VERTEX_SHADER));
	shaders.push_back(new Shader(ShaderDir "Text.frag", GL_FRAGMENT_SHADER));
	text_program = new ShaderProgram(shaders);
	for (std::size_t i = 0, n = shaders.size(); i < n; i++)
	{
		delete shaders[i];
	}
	shaders.clear();

	AVDictionary *opts = NULL;
	av_dict_set(&opts, "tune", "zerolatency", 0);
	av_dict_set(&opts, "preset", "ultrafast", 0);
	av_register_all();
	avformat_network_init();
	if (avformat_open_input(
				&avfmt,
				"udp://236.0.0.1:2000?fifo_size=1000000&overrun_nonfatal=1",
				NULL,
				&opts
				) < 0)
		return 1;
	AVCodec *codec = avcodec_find_decoder(avfmt->streams[0]->codec->codec_id);
	if (codec == NULL)
		return 1;

	avctx = avcodec_alloc_context3(codec);
	if (avcodec_open2(avctx, NULL, &opts) < 0)
		return 1;

	swctx = sws_getContext(
			width, height, AV_PIX_FMT_YUV420P,
			width, height, AV_PIX_FMT_BGRA,
			0, NULL, NULL, NULL);

	avframe = av_frame_alloc();
	data = (uint8_t *)malloc(width * height * 4);

	GLuint uImage = stream_program->GetUniformLocation("uImage");
	stream_program->Install();
	glUniform1i(uImage, 0);
	stream_program->Uninstall();

	glm::mat4 projMatrix = glm::ortho(0.f, (float)width, 0.f, (float)height);
	GLuint uMVPMatrix = text_program->GetUniformLocation("uMVPMatrix");
	GLuint uAtlas = text_program->GetUniformLocation("uAtlas");
	text_program->Install();
	vera->BindTexture(uAtlas);
	glUniformMatrix4fv(uMVPMatrix, 1, GL_FALSE, &projMatrix[0][0]);
	text_program->Uninstall();

	rakPeer->Startup(1, &RakNet::SocketDescriptor(), 1);
}

void update()
{
	std::ostringstream ss;
	ss << "Input: ";
	if (current_input & Forward)
		ss << "F";
	if (current_input & Backward)
		ss << "B";
	if (current_input & Right)
		ss << "R";
	if (current_input & Left)
		ss << "L";
	if (current_input & Jump)
		ss << "J";

	input_display->SetText(ss.str());
}

void draw()
{
	if (hostAddress == RakNet::UNASSIGNED_SYSTEM_ADDRESS)
	{
		while (rakPeer->GetReceiveBufferSize() > 0)
		{
			RakNet::Packet *p = rakPeer->Receive();
			unsigned char packet_type = p->data[0];
			RakNet::SystemAddress sender = p->systemAddress;
			rakPeer->DeallocatePacket(p);
			if (packet_type == ID_UNCONNECTED_PONG)
			{
				hostAddress = sender;
				const char *host = sender.ToString();
				rakPeer->Connect(host, REMOTE_GAME_PORT, NULL, 0);
				break;
			}
		}
		rakPeer->Ping("255.255.255.255", REMOTE_GAME_PORT, true);
	}
	else
	{
		RakNet::ConnectionState connection =
			rakPeer->GetConnectionState(hostAddress);
		if (connection != RakNet::IS_CONNECTED)
		{
			return;
		}
	}

	char send_data[2];
	send_data[0] = ID_USER_PACKET_ENUM;
	send_data[1] = current_input;
	rakPeer->Send(
			send_data, 2,
			IMMEDIATE_PRIORITY, RELIABLE, 0,
			hostAddress, false);

	AVPacket *pkt = av_packet_alloc();
	av_init_packet(pkt);
	avformat_flush(avfmt);
	if (av_read_frame(avfmt, pkt) < 0)
		return;
	int got_picture;
	if (avcodec_decode_video2(avctx, avframe, &got_picture, pkt) < 0)
		return;
	av_packet_unref(pkt);
	av_packet_free(&pkt);
	if (got_picture == 0)
		return;

	int linesize_align[AV_NUM_DATA_POINTERS];
	avcodec_align_dimensions2(
			avctx, &avframe->width, &avframe->height, linesize_align);

	uint8_t *const dstSlice[] = { data };
	int dstStride[] = { width * 4 };
	if (data != NULL)
		sws_scale(
				swctx,
				avframe->data, avframe->linesize,
				0, height,
				dstSlice, dstStride);
	av_frame_unref(avframe);
	glClear(GL_COLOR_BUFFER_BIT);
	glActiveTexture(GL_TEXTURE0 + tex - 1);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGBA,
			width, height, 0,
			GL_BGRA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	stream_program->Install();
	glEnableVertexAttribArray(0);
	vbo->BufferData(0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisableVertexAttribArray(0);
	stream_program->Uninstall();

	text_program->Install();
	input_display->Draw();
	text_program->Uninstall();

	glfwSwapBuffers(window);
}

void key_callback(
		GLFWwindow *window, int key, int scancode, int action, int mods)
{
	BlobInput changed_input = NoInput;
	if (key == GLFW_KEY_W)
		changed_input = Forward;
	else if (key == GLFW_KEY_S)
		changed_input = Backward;
	else if (key == GLFW_KEY_D)
		changed_input = Right;
	else if (key == GLFW_KEY_A)
		changed_input = Left;
	else if (key == GLFW_KEY_SPACE)
		changed_input = Jump;

	if (changed_input != 0)
	{
		if (action == GLFW_PRESS)
			current_input = (BlobInput)(current_input | changed_input);
		else if (action == GLFW_RELEASE)
			current_input = (BlobInput)(current_input & ~changed_input);
	}
}
