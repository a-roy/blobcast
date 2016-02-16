#include <GL/glew.h>
#include <GLFW/glfw3.h>
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
#include "Buffer.h"

#define RootDir "../../"
#define ShaderDir RootDir "shaders/"

bool init();
void draw();

GLFWwindow *window;
VertexArray *vao;
FloatBuffer *vbo;
ShaderProgram *program;
int width, height;
uint8_t *data;
GLuint tex;

AVFormatContext *avfmt = NULL;
AVCodecContext *avctx = NULL;
AVFrame *avframe = NULL;
SwsContext *swctx = NULL;

int main(int argc, char *argv[])
{
	window = GLFWProject::Init("Client Test", 1024, 576);
	if (!window)
		return 1;

	glfwGetFramebufferSize(window, &width, &height);

	if (!init())
		return 1;

	while (!glfwWindowShouldClose(window))
	{
		draw();
		glfwPollEvents();
	}
	free(data);
	avcodec_close(avctx);
	avcodec_free_context(&avctx);
	avformat_network_deinit();
	av_frame_free(&avframe);
	delete program;
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
	std::vector<Shader *> shaders;
	shaders.push_back(new Shader(ShaderDir "Stream.vert", GL_VERTEX_SHADER));
	shaders.push_back(new Shader(ShaderDir "Stream.frag", GL_FRAGMENT_SHADER));
	program = new ShaderProgram(shaders);
	for (std::size_t i = 0, n = shaders.size(); i < n; i++)
	{
		delete shaders[i];
	}
	shaders.clear();

	AVDictionary *opts = NULL;
	av_register_all();
	avformat_network_init();
	if (avformat_open_input(&avfmt, "udp://236.0.0.1:2000", NULL, &opts) < 0)
		return 1;
	AVCodec *codec = avcodec_find_decoder(avfmt->streams[0]->codec->codec_id);
	if (codec == NULL)
		return 1;

	avctx = avcodec_alloc_context3(codec);
	if (avcodec_open2(avctx, NULL, &opts) < 0)
		return 1;

	swctx = sws_getContext(
			width, height, AV_PIX_FMT_YUV444P,
			width, height, AV_PIX_FMT_BGRA,
			0, NULL, NULL, NULL);

	avframe = av_frame_alloc();
	data = (uint8_t *)malloc(width * height * 4);
	glGenTextures(1, &tex);

	GLuint uImage = program->GetUniformLocation("uImage");
	program->Install();
	glUniform1i(uImage, 0);
	program->Uninstall();
}

void draw()
{
	AVPacket *pkt = av_packet_alloc();
	av_init_packet(pkt);
	if (av_read_frame(avfmt, pkt) < 0)
		exit(1);
	int got_picture;
	if (avcodec_decode_video2(avctx, avframe, &got_picture, pkt) < 0)
		exit(1);
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
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGBA,
			width, height, 0,
			GL_BGRA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	program->Install();
	glEnableVertexAttribArray(0);
	vbo->BufferData(0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisableVertexAttribArray(0);
	program->Uninstall();

	glfwSwapBuffers(window);
}
