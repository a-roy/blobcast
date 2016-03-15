#pragma once

#include <GL/glew.h>
#include <vector>
#include <string>

class Shader
{
	public:
		GLuint Name;
		GLenum ShaderType;
		std::string Path;

		Shader(std::string path, GLenum shaderType);
		Shader(std::string path);
		~Shader();
		Shader(const Shader& other);
		Shader& operator=(const Shader& other);
		Shader(Shader&& other);
		Shader& operator=(Shader&& other);
		void Load(std::string path);
		void Compile();
		static void ReadSource(const char *fname, std::vector<char> &buffer);
};
