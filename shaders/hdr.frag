#version 420 core

out vec4 color;
in vec2 TexCoords;

uniform sampler2D hdrBuffer;
uniform float exposure;
uniform bool hdr;

void main()
{
	const float gamma = 2.2;
	vec3 hdrColor = texture(hdrBuffer, TexCoords).rgb;
	// Exposure tone mapping
	vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
	// Gamma correction
	mapped = pow(mapped, vec3(1.0 / gamma));
	
	color = vec4(mapped, 1.0);
}