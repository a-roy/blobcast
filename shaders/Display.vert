#version 420 core

layout (location = 0) in vec2 aPosition;
out vec2 vPosition;

uniform mat4 uMVPMatrix;

void main()
{
	vPosition = aPosition;
	gl_Position = uMVPMatrix * vec4(aPosition, 0.0, 1.0);
}
