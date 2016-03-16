#version 330 core

in vec3 vColor;

out vec4 color;

uniform vec4 uColor;

void main()
{
	color = uColor + vec4(vColor, 1);
}
