#version 330 core

layout (location = 0) in vec2 aPosition;
out vec2 vPosition;

void main()
{
	vPosition = aPosition;
	gl_Position = vec4(aPosition, 0, 1);
}
