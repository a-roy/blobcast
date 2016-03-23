#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include "Shader.h"
#include "Frame.h"
#include <initializer_list>
#include <vector>
#include <string>
#include <map>

class ShaderProgram
{
	public:
		struct Uniform
		{
			const ShaderProgram *program;
			GLint Location;
			std::string Name;
			Uniform& operator=(glm::vec2 value);
			Uniform& operator=(glm::vec3 value);
			Uniform& operator=(glm::vec4 value);
			Uniform& operator=(glm::mat3 value);
			Uniform& operator=(glm::mat4 value);
			Uniform& operator=(const std::vector<glm::vec2>& values);
			Uniform& operator=(const std::vector<glm::vec3>& values);
			Uniform& operator=(const std::vector<glm::vec4>& values);
			Uniform& operator=(const std::vector<glm::mat3>& values);
			Uniform& operator=(const std::vector<glm::mat4>& values);
			Uniform& operator=(GLfloat value);
			Uniform& operator=(GLint value);
			void warn(GLenum sym, std::string keyword, int num = 1);
		};
		GLuint program;

		ShaderProgram();
		ShaderProgram(std::initializer_list<std::string> paths);
		ShaderProgram(std::vector<Shader *>& shaders);
		~ShaderProgram();
		ShaderProgram(const ShaderProgram&) = delete;
		ShaderProgram& operator=(const ShaderProgram&) = delete;
		ShaderProgram(ShaderProgram&& other);
		ShaderProgram& operator=(ShaderProgram&& other);
		void LinkProgram(std::vector<Shader *> &shaders);
		GLint GetUniformLocation(std::string name) const;
		void DrawFrame(Frame *frame, glm::mat4 mvMatrix) const;
		Uniform operator[](std::string name) const;
		template<class Fn>
		void Use(Fn func) const
		{
			GLint name;
			glGetIntegerv(GL_CURRENT_PROGRAM, &name);
			glUseProgram(program);
			func();
			glUseProgram(name);
		}
};
