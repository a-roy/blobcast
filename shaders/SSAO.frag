#version 420 core
out float FragColor;
in vec2 TexCoords;

layout (binding = 0) uniform sampler2D gPosition;
layout (binding = 1) uniform sampler2D gNormal;
layout (binding = 2) uniform sampler2D texNoise;

uniform vec3 samples[64];
uniform vec2 screenSize;
uniform mat4 projection;

int kernelSize = 64;
float radius = 2.0;

// tile noise texture over screen based on screen dimensions divided by noise size
// TO DO : pass screen dimensions in as uniform
vec2 noiseScale = vec2(screenSize.x/4.0f, screenSize.y/4.0f);

void main()
{
    vec3 fragPos = texture(gPosition, TexCoords).xyz;
	vec3 normal = texture(gNormal, TexCoords).rgb;
	vec3 randomVec = texture(texNoise, TexCoords * noiseScale).xyz;
	
	// Create TBN change of basis matrix : from tangent to view-space
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);
	
	// Iterate over the sample kernel and calculate occlusion factor
	float occlusion = 0.0;
	for(int i = 0; i < kernelSize; i++)
	{
		// Get sample position
		vec3 samplePos = TBN * samples[i]; // From tangent to view-space
		samplePos = fragPos + samplePos * radius;
		
		// Project sample position (to sample texture) (to get position on screen/texture)
		vec4 offset = vec4(samplePos, 1.0);
		offset = projection * offset; // from view to clip-space
		offset.xy /= offset.w; // perspective divide
		offset.xy = offset.xy * 0.5 + 0.5; // transform to range (0, 1)
		
		float sampleDepth = texture(gPosition, offset.xy).z;
		
		float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
		occlusion += (sampleDepth >= samplePos.z ? 1.0 : 0.0) * rangeCheck; 
	}
	
	occlusion = 1.0 - (occlusion / kernelSize);
	
	FragColor = pow(occlusion, 6.0);
}

