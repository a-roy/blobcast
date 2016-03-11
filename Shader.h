#pragma once

#include <GL/glew.h>
#include <vector>
#include <string>

class Shader
{
	public:
		GLuint Name;
		GLenum ShaderType;

		Shader(std::string path, GLenum shaderType);
		Shader(std::string path);
		~Shader();
		void LoadAndCompile(std::string path);
		static void ReadSource(const char *fname, std::vector<char> &buffer);
};
