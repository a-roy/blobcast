#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <RakNet/MessageIdentifiers.h>
#include <RakNet/RakPeerInterface.h>
#include <RakNet/RakNetTypes.h>

#include <memory>
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
std::shared_ptr<VertexArray> vao;
std::unique_ptr<FloatBuffer> vbo;
std::unique_ptr<Text> input_display;
std::shared_ptr<Font> vera;
std::unique_ptr<ShaderProgram> stream_program;
std::unique_ptr<ShaderProgram> text_program;
int width, height;
std::string stream_address;
uint8_t *data;
GLuint tex;
std::unique_ptr<StreamReceiver> stream;

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
	vao = std::shared_ptr<VertexArray>(new VertexArray());
	vbo = std::unique_ptr<FloatBuffer>(new FloatBuffer(vao.get(), 2, 4));
	GLfloat *vertex_data = new GLfloat[8] { -1, -1, -1, 1, 1, -1, 1, 1 };
	vbo->SetData(vertex_data);

	glBindVertexArray(vao->Name);
	vbo->BufferData(0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	glGenTextures(1, &tex);
	vera = std::shared_ptr<Font>(new Font(FontDir "Vera.ttf", 24.f));
	input_display = std::unique_ptr<Text>(new Text(vera.get()));
	input_display->XPosition = 8;
	input_display->YPosition = 8;
	input_display->SetText("Input:");

	stream_program = std::unique_ptr<ShaderProgram>(new ShaderProgram({
			ShaderDir "Stream.vert",
			ShaderDir "Stream.frag" }));

	text_program = std::unique_ptr<ShaderProgram>(new ShaderProgram({
			ShaderDir "Text.vert",
			ShaderDir "Text.frag" }));

	stream = std::unique_ptr<StreamReceiver>(
			new StreamReceiver(stream_address.c_str(), width, height));
	data = (uint8_t *)malloc(width * height * 4);

	(*stream_program)["uImage"] = 0;

	glm::mat4 projMatrix = glm::ortho(0.f, (float)width, 0.f, (float)height);
	text_program->Use([&](){
		vera->BindTexture(text_program->GetUniformLocation("uAtlas"));
	});
	(*text_program)["uTextColor"] = glm::vec4(0.f, 0.f, 0.f, 1.f);
	(*text_program)["uMVPMatrix"] = projMatrix;

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
	glBindVertexArray(vao->Name);
	stream_program->Use([&](){
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	});
	glBindVertexArray(0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	vera->UploadTextureAtlas();
	text_program->Use([&](){
		input_display->Draw();
	});

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
