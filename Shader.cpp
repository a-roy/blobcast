#include "Shader.h"
#include <fstream>
#include <iostream>

Shader::Shader(std::string path, GLenum shaderType) : ShaderType(shaderType)
{
	Name = glCreateShader(shaderType);
	LoadAndCompile(path);
}

Shader::Shader(std::string path)
{
	std::string extension = path.substr(path.length() - 4, 4);
	GLenum shaderType;
	if (extension == "vert")
		shaderType = GL_VERTEX_SHADER;
	else if (extension == "tesc")
		shaderType = GL_TESS_CONTROL_SHADER;
	else if (extension == "tese")
		shaderType = GL_TESS_EVALUATION_SHADER;
	else if (extension == "geom")
		shaderType = GL_GEOMETRY_SHADER;
	else if (extension == "frag")
		shaderType = GL_FRAGMENT_SHADER;

	Name = glCreateShader(shaderType);
	LoadAndCompile(path);
}

Shader::~Shader()
{
	glDeleteShader(Name);
}

void Shader::LoadAndCompile(std::string path)
{
	std::vector<char> buffer;
	ReadSource(path.c_str(), buffer);
	const char *src = &buffer[0];

	// Compile the shader
	glShaderSource(Name, 1, &src, NULL);
	glCompileShader(Name);
	// Check the result of the compilation
	GLint isCompiled;
	glGetShaderiv(Name, GL_COMPILE_STATUS, &isCompiled);
	if (!isCompiled)
	{
		std::cerr << "Shader compilation failed with this message:" << std::endl;
		GLint maxLength = 0;
		glGetShaderiv(Name, GL_INFO_LOG_LENGTH, &maxLength);

		std::vector<GLchar> infoLog(maxLength);
		glGetShaderInfoLog(Name, maxLength, &maxLength, &infoLog[0]);
		std::cerr << &infoLog[0] << std::endl;

		// TODO don't just terminate the application?
		exit(1);
	}
}

void Shader::ReadSource(const char *fname, std::vector<char> &buffer)
{
	std::ifstream in;
	in.open(fname, std::ios::binary);

	if (in.is_open())
	{
		// Get the number of bytes stored in this file
		in.seekg(0, std::ios::end);
		size_t length = (size_t)in.tellg();

		// Go to start of the file
		in.seekg(0, std::ios::beg);

		// Read the content of the file in a buffer
		buffer.resize(length + 1);
		in.read(&buffer[0], length);
		in.close();
		// Add a valid C - string end
		buffer[length] = '\0';
	}
	else
	{
		std::cerr << "Unable to open " << fname << std::endl;
		exit(-1);
	}
}
