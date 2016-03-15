#version 420 core

in vec3 FragPos;
in vec3 Normal;
in vec4 LightSpacePos;

out vec4 FragColor;                                                                            

struct DirectionalLight
{
	vec3 color;                                                       
    vec3 direction;
	vec3 ambientColor;
};

uniform DirectionalLight directionalLight;
uniform vec3 viewPos;
uniform vec2 screenSize;
uniform vec4 objectColor;

layout (binding = 0) uniform sampler2DShadow depthMap;
layout (binding = 1) uniform sampler2D aoMap;

const vec2 mapSize = vec2(2048, 2048);

#define EPSILON 0.00001

vec2 CalcScreenTexCoord()
{
    return gl_FragCoord.xy / screenSize;
}

float CalcShadowFactor(vec4 LightSpacePos)
{
    vec3 projCoords = LightSpacePos.xyz / LightSpacePos.w;
    vec2 UVCoords;
    UVCoords.x = 0.5 * projCoords.x + 0.5;
    UVCoords.y = 0.5 * projCoords.y + 0.5;
    float z = 0.5 * projCoords.z + 0.5;
	
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
	
	return (0.5 + (factor / 16.0));
}

void main()
{   
	// Ambient
    vec3 ambient = directionalLight.ambientColor;
	//ambient *= texture(aoMap, CalcScreenTexCoord()).r;
	
	// Diffuse
	vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(-directionalLight.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * directionalLight.color;
	
	// Specular
    float specularStrength = 0.5f;
    vec3 viewDir = normalize(FragPos - viewPos);
    vec3 reflectDir = reflect(-lightDir, normal);  
    vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), 32);
    vec3 specular = specularStrength * spec * directionalLight.color;
	
	float ShadowFactor = CalcShadowFactor(LightSpacePos);
    vec3 result = (ambient + ShadowFactor * (diffuse + specular)) * vec3(objectColor);
	
    FragColor = vec4(result, 1.0f);
}