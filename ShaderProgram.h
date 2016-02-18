#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include "Shader.h"
#include "Frame.h"
#include <vector>
#include <string>
#include <map>

class ShaderProgram
{
	public:
		GLuint program;
		std::map<std::string, GLint> uniforms;

		ShaderProgram(std::vector<Shader *> shaders);
		~ShaderProgram();
		void LinkProgram(std::vector<Shader *> &shaders);
		GLint GetUniformLocation(std::string name) const;
		void DrawFrame(Frame *frame, glm::mat4 mvMatrix) const;
		void Install() const;
		void Uninstall() const;

		void SetUniform(std::string name, glm::vec3 value);
		void SetUniform(std::string name, glm::vec4 value);
		void SetUniform(std::string name, glm::mat3 value);
		void SetUniform(std::string name, glm::mat4 value);
		void SetUniform(std::string name, GLfloat value);
		void SetUniform(std::string name, GLint value);
};
