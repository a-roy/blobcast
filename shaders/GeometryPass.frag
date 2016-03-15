#version 330
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;

in vec3 FragPos;
in vec3 Normal;

const float NEAR = 0.1; // projection matrix's near plane
const float FAR = 1000.0f; // projection matrix's far plane

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * NEAR * FAR) / (FAR + NEAR - z * (FAR - NEAR));	
}

uniform vec4 objectColor;

void main()
{
    gPosition.xyz = FragPos; // Fragment position vector, stored in 1st gbuffer texture
	//gPosition.a = LinearizeDepth(gl_FragCoord.z);
	gNormal = normalize(Normal); // Per-fragment normal
}
