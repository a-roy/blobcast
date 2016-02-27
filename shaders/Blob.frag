#version 420 core

in vec3 FragPos;
in vec3 Normal;
in vec4 LightSpacePos;

out vec4 fragColor;                                                                                  

struct DirectionalLight
{
	vec3 color;                                                         
    vec3 direction;
	vec3 ambientColor;
};

uniform vec3 viewPos;
uniform vec3 objectColor;
uniform DirectionalLight directionalLight;

layout (binding = 0) uniform samplerCube cubeMap;
layout (binding = 1) uniform sampler2DShadow depthMap;

const vec2 mapSize = vec2(2048, 2048);
const float FresnelPower = 7.0;

#define EPSILON 0.00001


float CalcShadowFactor(vec4 LightSpacePos)
{
    vec3 projCoords = LightSpacePos.xyz / LightSpacePos.w;
    vec2 UVCoords;
    UVCoords.x = 0.5 * projCoords.x + 0.5;
    UVCoords.y = 0.5 * projCoords.y + 0.5;
    float z = 0.5 * projCoords.z + 0.5;
	
	if(projCoords.z > 1.0)
		return 0.0;
	
	float xOffset = 1.0/mapSize.x;
	float yOffset = 1.0/mapSize.y;
	
	float factor = 0.0;
	
	for(int y = -2; y <= 2; y++){
		for(int x = -2; x <= 2; x++){
			vec2 offsets = vec2(x * xOffset, y * yOffset);
			vec3 UVC = vec3(UVCoords + offsets, z + EPSILON);
			factor += texture(depthMap, UVC);
		}
	}
	
	return (0.5 + (factor / 18.0));
}


void main()
{   
	// Ambient
	float ambientStrength = 0.1f;
    vec3 ambient = ambientStrength * directionalLight.ambientColor;
	
	// Diffuse
	float diffuseStrength = 0.6f;
	vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(directionalLight.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diffuseStrength * diff * directionalLight.color;
	
	// Specular
    float specularStrength = 0.4f;
    vec3 viewDir = normalize(FragPos - viewPos);
    vec3 reflectDir = reflect(-lightDir, normal);  
    vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), 32);
	
    vec3 specular = specularStrength * spec * directionalLight.color;
	
	float ShadowFactor = CalcShadowFactor(LightSpacePos);
    vec3 diffuseColor = (ambient + ShadowFactor * (diffuse + specular)) * objectColor;
	
	// Reflection + refraction
	float refractiveIndex = 1.15;
	
	vec3 Refract = refract(viewDir, normal, refractiveIndex);	
	vec3 Reflect = reflect(viewDir, normal);
	
	vec3 refractColor = vec3(texture(cubeMap, Refract));
	vec3 reflectColor = vec3(texture(cubeMap, Reflect));
	
	//vec3 finalColor;
	vec3 reflectionColor = mix(diffuseColor, reflectColor, 0.2);
	vec3 finalColor = mix(reflectionColor, refractColor, 0.2);
	
	fragColor = vec4(finalColor, 1.0f);

}