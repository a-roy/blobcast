#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D tex;

void main()
{ 
	float value =  texture(tex, TexCoords).r;
    color = vec4(value);
}

