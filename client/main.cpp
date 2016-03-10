#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <RakNet/MessageIdentifiers.h>
#include <RakNet/RakPeerInterface.h>
#include <RakNet/RakNetTypes.h>

#include <thread>
#include <iostream>
#include <sstream>
#include "GLFWProject.h"
#include "ShaderProgram.h"
#include "Buffer.h"
#include "Text.h"
#include "BlobInput.h"
#include "HostData.h"
#include "StreamReceiver.h"

#include "config.h"

bool connect();
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
std::string stream_address;
uint8_t *data;
GLuint tex;
StreamReceiver *stream;

RakNet::RakPeerInterface *rakPeer = RakNet::RakPeerInterface::GetInstance();
RakNet::SystemAddress hostAddress = RakNet::UNASSIGNED_SYSTEM_ADDRESS;
BlobInput current_input;

int main(int argc, char *argv[])
{
	if (!connect())
		return 1;
	window = GLFWProject::Init("Blobclient", CLIENT_WIDTH, CLIENT_HEIGHT);
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
	rakPeer->Shutdown(100);
	RakNet::RakPeerInterface::DestroyInstance(rakPeer);
	delete stream_program;
	delete text_program;
	delete input_display;
	delete vera;
	delete vbo;
	delete vao;

	glfwTerminate();
	return 0;
}

bool connect()
{
	RakNet::SocketDescriptor sd;
	RakNet::StartupResult rakStart = rakPeer->Startup(1, &sd, 1);
	if (rakStart != RakNet::RAKNET_STARTED)
		return false;

	while (hostAddress == RakNet::UNASSIGNED_SYSTEM_ADDRESS)
	{
		bool connected = false;
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
				connected = true;
#ifdef RTMP_STREAM
				std::ostringstream ss;
				ss << STREAM_PROTOCOL << sender.ToString(false)
					<< RTMP_PATH;
				stream_address = ss.str();
#else // RTMP_STREAM
				stream_address = STREAM_PATH;
#endif // RTMP_STREAM
				break;
			}
		}
		if (!connected)
		{
			rakPeer->Ping("255.255.255.255", REMOTE_GAME_PORT, true);
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	return true;
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

	stream_program = new ShaderProgram({
			ShaderDir "Stream.vert",
			ShaderDir "Stream.frag" });

	text_program = new ShaderProgram({
			ShaderDir "Text.vert",
			ShaderDir "Text.frag" });

	stream = new StreamReceiver(stream_address.c_str(), width, height);
	data = (uint8_t *)malloc(width * height * 4);

	GLuint uImage = stream_program->GetUniformLocation("uImage");
	stream_program->Install();
	glUniform1i(uImage, 0);
	stream_program->Uninstall();

	glm::mat4 projMatrix = glm::ortho(0.f, (float)width, 0.f, (float)height);
	GLuint uMVPMatrix = text_program->GetUniformLocation("uMVPMatrix");
	GLuint uTextColor = text_program->GetUniformLocation("uTextColor");
	GLuint uAtlas = text_program->GetUniformLocation("uAtlas");
	text_program->Install();
	vera->BindTexture(uAtlas);
	glUniform4f(uTextColor, 0.f, 0.f, 0.f, 1.f);
	glUniformMatrix4fv(uMVPMatrix, 1, GL_FALSE, &projMatrix[0][0]);
	text_program->Uninstall();

	return true;
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
		bool connected = false;
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
				connected = true;
				std::ostringstream ss;
				ss << "rtmp://" << sender.ToString(false) << "/live/test";
				break;
			}
		}
		if (!connected)
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

	stream->ReceiveFrame(data);
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
