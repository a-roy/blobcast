#version 330 core

layout (location = 0) in vec2 aPosition;
layout (location = 1) in vec2 aTexCoords;

out vec2 vTexCoords;

uniform mat4 uMVPMatrix;

void main()
{
	gl_Position = uMVPMatrix * vec4(aPosition, 0, 1);
	vTexCoords = aTexCoords;
}
