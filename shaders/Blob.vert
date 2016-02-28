#version 420 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMat;

out vec3 vPosition;
out vec3 vNormal;

void main()
{
	vPosition = position;
	vNormal = normalize(normal);
}
