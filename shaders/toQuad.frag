#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D tex;

void main()
{ 
	vec3 value = texture(tex, TexCoords).rgb;
    color = vec4(value, 1.0);
}

