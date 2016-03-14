#version 330
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 FragPos;
in vec3 Normal;

uniform vec4 objectColor;

void main()
{
    gPosition = FragPos; // Fragment position vector, stored in 1st gbuffer texture
	gNormal = normalize(Normal); // Per-fragment normal
	gAlbedoSpec.rgb = objectColor.rgb; // Diffuse per-fragment color
	gAlbedoSpec.a = 0.2; // Specular intensity
	
}