#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;

out vec3 vPosition;
out vec3 vNormal;

uniform mat4 uMVPMatrix;

void main()
{
	vPosition = aPosition;
	vNormal = normalize(aNormal);
	gl_Position = uMVPMatrix * vec4(aPosition, 1);
}
