#include "IOBuffer.h"

IOBuffer::IOBuffer()
{
	FBO = 0;
	texture = 0;
	depth = 0;
	internalType = GL_NONE;
}

IOBuffer::~IOBuffer()
{
	if (FBO != 0)
		glDeleteFramebuffers(1, &FBO);

	if (texture != 0)
		glDeleteTextures(1, &texture);

	if (depth != 0)
		glDeleteTextures(1, &depth);
}

bool IOBuffer::Init(int Width, int Height, bool Depth, GLenum inType) {
	
	internalType = inType;

	GLenum format, type;
	
	switch (internalType) {
	case GL_RGB32F:
		format = GL_RGB;
		type = GL_FLOAT;
	case GL_R32F:
		format = GL_RED;
		type = GL_FLOAT;
	case GL_NONE:
		break;
	default:
		std::cout << "Invalid type" << std::endl;
	}
	
	// Generate FBO
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	// Generate textures
	if (internalType != GL_NONE)
	{
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);

		glTexImage2D(GL_TEXTURE_2D, 0, internalType, Width, Height, 0, format, type, NULL);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

		GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, DrawBuffers);
	}

	// Generate depth buffer
	if (Depth)
	{
		glGenTextures(1, &depth);
		glBindTexture(GL_TEXTURE_2D, depth);
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, Width, Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
	}

	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		printf("Framebuffer error, status: 0x%x\n", Status);
		return false;
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	return true;
}


bool IOBuffer::InitForCubeMap(int Width, int Height)
{
	return false;
}


void IOBuffer::BindForWriting()
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
}

void IOBuffer::BindForReading(GLenum TextureUnit)
{
	glActiveTexture(TextureUnit);

	if (internalType == GL_NONE)
	{
		glBindTexture(GL_TEXTURE_2D, depth);
	}	
	else
	{
		glBindTexture(GL_TEXTURE_2D, texture);
		
	}
		
}