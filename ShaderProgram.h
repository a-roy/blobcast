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
			Uniform& operator=(glm::vec3 value);
			Uniform& operator=(glm::vec4 value);
			Uniform& operator=(glm::mat3 value);
			Uniform& operator=(glm::mat4 value);
			Uniform& operator=(GLfloat value);
			Uniform& operator=(GLint value);
		};
		GLuint program;
		std::map<std::string, GLint> uniforms;

		ShaderProgram(std::initializer_list<std::string> paths);
		ShaderProgram(std::vector<Shader *>& shaders);
		~ShaderProgram();
		void LinkProgram(std::vector<Shader *> &shaders);
		GLint GetUniformLocation(std::string name) const;
		void DrawFrame(Frame *frame, glm::mat4 mvMatrix) const;
		Uniform operator[](std::string name) const;
		Uniform operator[](GLint location) const;
		template<class Fn>
		void Use(Fn func) const
		{
			glUseProgram(program);
			func();
			glUseProgram(0);
		}
};
