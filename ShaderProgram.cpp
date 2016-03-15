#include "ShaderProgram.h"
#include <iostream>
#include <exception>
#include <glm/gtc/type_ptr.hpp>

ShaderProgram::ShaderProgram()
{
	program = glCreateProgram();
}

ShaderProgram::ShaderProgram(std::initializer_list<std::string> paths) :
	ShaderProgram()
{
	std::vector<Shader *> shaders;
	for (std::string path : paths)
		shaders.push_back(new Shader(path));
	LinkProgram(shaders);
	for (int i = 0, n = shaders.size(); i < n; i++)
		delete shaders[i];
}

ShaderProgram::ShaderProgram(std::vector<Shader *>& shaders) :
	ShaderProgram()
{
	LinkProgram(shaders);
}

ShaderProgram::~ShaderProgram()
{
	glDeleteProgram(program);
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) :
	program(0)
{
	*this = std::move(other);
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other)
{
	if (this != &other)
	{
		glDeleteProgram(program);
		program = other.program;
		uniforms = other.uniforms;
		other.program = 0;
		other.uniforms.clear();
	}
	return *this;
}

void ShaderProgram::LinkProgram(std::vector<Shader *> &shaders)
{
	for (std::size_t i = 0; i < shaders.size(); i++)
	{
		glAttachShader(program, shaders[i]->Name);
	}
	glLinkProgram(program);

	GLint isLinked = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
	if (isLinked == GL_FALSE)
	{
		std::cerr << "Shader linking failed with this message:" << std::endl;
		GLint maxLength = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

		std::vector<GLchar> infoLog(maxLength);
		glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);
		std::cerr << &infoLog[0] << std::endl;

		throw std::exception();
	}

	for (std::size_t i = 0; i < shaders.size(); i++)
	{
		glDetachShader(program, shaders[i]->Name);
	}

	GLint num_uniforms;
	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &num_uniforms);
	GLint uniform_name_length;
	glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniform_name_length);
	GLchar *name = new GLchar[uniform_name_length];
	for (GLint i = 0; i < num_uniforms; i++)
	{
		GLint size;
		GLenum type;
		glGetActiveUniform(
				program, i, uniform_name_length, NULL, &size, &type, name);
		uniforms[name] = glGetUniformLocation(program, name);
	}
	delete[] name;
}

GLint ShaderProgram::GetUniformLocation(std::string name) const
{
	auto it = uniforms.find(name);
	if (it == uniforms.end())
	{
		return -1;
	}
	else
	{
		return it->second;
	}
}

void ShaderProgram::DrawFrame(Frame *frame, glm::mat4 mvMatrix) const
{
	Use([&](){
		GLuint uMVMatrix = glGetUniformLocation(program, "uMVMatrix");
		GLuint uNMatrix = glGetUniformLocation (program, "uNMatrix");
		frame->Draw(uMVMatrix, uNMatrix, mvMatrix);
	});
}

ShaderProgram::Uniform ShaderProgram::operator[](std::string name) const
{
	Uniform u = { this, glGetUniformLocation(program, name.c_str()) };
	return u;
}

ShaderProgram::Uniform ShaderProgram::operator[](GLint location) const
{
	Uniform u = { this, location };
	return u;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(glm::vec2 value)
{
	program->Use([&](){ glUniform2fv(Location, 1, glm::value_ptr(value)); });
	return *this;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(glm::vec3 value)
{
	program->Use([&](){ glUniform3fv(Location, 1, glm::value_ptr(value)); });
	return *this;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(glm::vec4 value)
{
	program->Use([&](){ glUniform4fv(Location, 1, glm::value_ptr(value)); });
	return *this;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(glm::mat3 value)
{
	program->Use([&](){
		glUniformMatrix3fv(Location, 1, GL_FALSE, glm::value_ptr(value));
	});
	return *this;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(glm::mat4 value)
{
	program->Use([&](){
		glUniformMatrix4fv(Location, 1, GL_FALSE, glm::value_ptr(value));
	});
	return *this;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(
		const std::vector<glm::vec2>& values)
{
	program->Use([&](){
		glUniform2fv(Location, values.size(), glm::value_ptr(values[0]));
	});
	return *this;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(
		const std::vector<glm::vec3>& values)
{
	program->Use([&](){
		glUniform3fv(Location, values.size(), glm::value_ptr(values[0]));
	});
	return *this;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(
		const std::vector<glm::vec4>& values)
{
	program->Use([&](){
		glUniform4fv(Location, values.size(), glm::value_ptr(values[0]));
	});
	return *this;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(
		const std::vector<glm::mat3>& values)
{
	program->Use([&](){
		glUniformMatrix3fv(
				Location, values.size(), GL_FALSE, glm::value_ptr(values[0]));
	});
	return *this;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(
		const std::vector<glm::mat4>& values)
{
	program->Use([&](){
		glUniformMatrix4fv(
				Location, values.size(), GL_FALSE, glm::value_ptr(values[0]));
	});
	return *this;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(GLfloat value)
{
	program->Use([&](){ glUniform1f(Location, value); });
	return *this;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(GLint value)
{
	program->Use([&](){ glUniform1i(Location, value); });
	return *this;
}
