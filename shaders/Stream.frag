#version 330 core

in vec2 vPosition;
uniform sampler2D uImage;
out vec4 fColor;

void main()
{
	fColor = texture(uImage, (vPosition + 1.0) * 0.5);
}
