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
			Uniform& operator=(std::vector<glm::vec3> values);
			Uniform& operator=(std::vector<glm::vec4> values);
			Uniform& operator=(std::vector<glm::mat3> values);
			Uniform& operator=(std::vector<glm::mat4> values);
			Uniform& operator=(GLfloat value);
			Uniform& operator=(GLint value);
		};
		GLuint program;
		std::map<std::string, GLint> uniforms;

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
		Uniform operator[](GLint location) const;
		template<class Fn>
		void Use(Fn func) const
		{
			glUseProgram(program);
			func();
			glUseProgram(0);
		}
};
