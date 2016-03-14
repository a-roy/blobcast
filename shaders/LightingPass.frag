#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

layout (binding = 0) uniform sampler2D gPositionDepth;
layout (binding = 1) uniform sampler2D gNormal;
layout (binding = 2) uniform sampler2D gAlbedoSpec;
layout (binding = 3) uniform sampler2D ssao;

struct DirectionalLight
{
	vec3 color;                                                       
    vec3 direction;
	vec3 ambientColor;
};

uniform DirectionalLight directionalLight;
uniform vec3 viewPos;

void main()
{             
    // Retrieve data from gbuffer
    vec3 FragPos = texture(gPositionDepth, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;
	float AmbientOcclusion = texture(ssao, TexCoords).r;
	
	// Ambient
    vec3 ambient = vec3(AmbientOcclusion) * directionalLight.color;
	
	// Diffuse
    vec3 lightDir = normalize(-directionalLight.direction);
	float diff = max(dot(Normal, lightDir), 0.0);
    vec3 diffuse = diff * directionalLight.color;
	
	// Specular
    vec3 viewDir = normalize(viewPos-FragPos); // ViewPos is 0,0,0 
    vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(Normal, halfwayDir), 0.0), 8.0);
    vec3 specular = Specular * spec * directionalLight.color;
	
	vec3 lighting = (ambient + diffuse + specular) * Diffuse;
        
    FragColor = vec4(lighting, 1.0);
}