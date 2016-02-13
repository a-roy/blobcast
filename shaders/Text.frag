#version 330 core

in vec2 vTexCoords;

out vec4 color;

uniform sampler2D uAtlas;
uniform vec4 uTextColor = vec4(1, 1, 1, 1);

void main()
{
	float a = texture2D(uAtlas, vTexCoords).r;
	if (a == 0.0)
		discard;
	color = vec4(uTextColor.rgb, uTextColor.a * a);
}
