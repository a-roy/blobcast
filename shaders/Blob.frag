#version 420 core

in vec3 FragPos;
in vec3 Normal;
in vec4 lightSpacePos;

out vec4 fragColor;

struct BaseLight
{
	vec3 color;
	float ambientIntensity;
	float diffuseIntensity;                                                 
};                                                                                  

struct DirectionalLight
{
	BaseLight base;                                                          
    vec3 direction;
};

uniform DirectionalLight directionalLight;
uniform vec3 viewPos;
uniform vec3 objectColor;

layout (binding = 0) uniform samplerCube cubeMap;


void main()
{   
	// Ambient
	float ambientStrength = 0.1f;
    vec3 ambient = ambientStrength * directionalLight.base.color;
	
	// Diffuse
	vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(directionalLight.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * directionalLight.base.color;
	
	// Specular
    float specularStrength = 0.5f;
    vec3 viewDir = normalize(FragPos - viewPos);
    vec3 reflectDir = reflect(-lightDir, normal);  
    vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), 32);
	
    vec3 specular = specularStrength * spec * directionalLight.base.color;
	
	vec3 R = reflect(viewDir, normal);
	vec3 reflection = texture(cubeMap, R).rgb;
	vec3 refraction = texture(cubeMap, refract(viewDir, normal, 1.1)).rgb;
	
	//float ShadowFactor = CalcShadowFactor(lightSpacePos);
    //vec3 diffuseColor = (ambient + ShadowFactor * (diffuse + specular)) * objectColor;
	vec3 diffuseColor = (ambient + diffuse + specular) * objectColor;
	
	vec3 solidColor = mix(diffuseColor, reflection, 0.2);
	
	vec3 finalColor = mix(solidColor, refraction, 0.2);
	
	fragColor = vec4(finalColor, 1.0f);

}