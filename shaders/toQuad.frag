#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2DArray tex;

void main()
{ 
	float value =  texture(tex, vec3(TexCoords, 0)).r;
    color = vec4(value);
}

