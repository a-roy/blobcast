#version 420 core
out float FragColor;
in vec2 TexCoords;

uniform sampler2D tex;

void main()
{ 
	FragColor = texture(tex, TexCoords).x;
}