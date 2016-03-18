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
		other.program = 0;
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
}

GLint ShaderProgram::GetUniformLocation(std::string name) const
{
	return glGetUniformLocation(program, name.c_str());
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
	Uniform u = { this, glGetUniformLocation(program, name.c_str()), name };
	return u;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(glm::vec2 value)
{
	warn(GL_FLOAT_VEC2, "vec2");
	program->Use([&](){ glUniform2fv(Location, 1, glm::value_ptr(value)); });
	return *this;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(glm::vec3 value)
{
	warn(GL_FLOAT_VEC3, "vec3");
	program->Use([&](){ glUniform3fv(Location, 1, glm::value_ptr(value)); });
	return *this;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(glm::vec4 value)
{
	warn(GL_FLOAT_VEC4, "vec4");
	program->Use([&](){ glUniform4fv(Location, 1, glm::value_ptr(value)); });
	return *this;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(glm::mat3 value)
{
	warn(GL_FLOAT_MAT3, "mat3");
	program->Use([&](){
		glUniformMatrix3fv(Location, 1, GL_FALSE, glm::value_ptr(value));
	});
	return *this;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(glm::mat4 value)
{
	warn(GL_FLOAT_MAT4, "mat4");
	program->Use([&](){
		glUniformMatrix4fv(Location, 1, GL_FALSE, glm::value_ptr(value));
	});
	return *this;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(
		const std::vector<glm::vec2>& values)
{
	warn(GL_FLOAT_VEC2, "vec2", values.size());
	program->Use([&](){
		glUniform2fv(Location, values.size(), glm::value_ptr(values[0]));
	});
	return *this;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(
		const std::vector<glm::vec3>& values)
{
	warn(GL_FLOAT_VEC3, "vec3", values.size());
	program->Use([&](){
		glUniform3fv(Location, values.size(), glm::value_ptr(values[0]));
	});
	return *this;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(
		const std::vector<glm::vec4>& values)
{
	warn(GL_FLOAT_VEC4, "vec4", values.size());
	program->Use([&](){
		glUniform4fv(Location, values.size(), glm::value_ptr(values[0]));
	});
	return *this;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(
		const std::vector<glm::mat3>& values)
{
	warn(GL_FLOAT_MAT3, "mat3", values.size());
	program->Use([&](){
		glUniformMatrix3fv(
				Location, values.size(), GL_FALSE, glm::value_ptr(values[0]));
	});
	return *this;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(
		const std::vector<glm::mat4>& values)
{
	warn(GL_FLOAT_MAT4, "mat4", values.size());
	program->Use([&](){
		glUniformMatrix4fv(
				Location, values.size(), GL_FALSE, glm::value_ptr(values[0]));
	});
	return *this;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(GLfloat value)
{
	warn(GL_FLOAT, "float");
	program->Use([&](){ glUniform1f(Location, value); });
	return *this;
}

ShaderProgram::Uniform& ShaderProgram::Uniform::operator=(GLint value)
{
	/// \TODO type check
	program->Use([&](){ glUniform1i(Location, value); });
	return *this;
}

void ShaderProgram::Uniform::warn(GLenum sym, std::string keyword, int num)
{
	const GLchar *name = Name.c_str();
	GLuint index;
	glGetUniformIndices(program->program, 1, &name, &index);
	if (index == GL_INVALID_INDEX)
	{
		std::cerr << "Warning: Uniform variable `" << Name << "` "
			"is not an active uniform." << std::endl;
		return;
	}
	GLint size;
	GLenum type;
	glGetActiveUniform(
			program->program, index, 0, nullptr, &size, &type, nullptr);
	if (type != sym)
	{
		std::cerr << "Warning: Uniform variable `" << Name << "` "
			"does not have type `" << keyword << "`." << std::endl;
	}
	if (num > 1 && size != num)
	{
		std::cerr << "Warning: Uniform variable `" << Name << "` "
			"with size " << size << " initialized by data with size " <<
			num << "." << std::endl;
	}
}
