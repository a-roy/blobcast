#version 420 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
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

layout (binding = 0) uniform sampler2DShadow shadowMap;
layout (binding = 1) uniform sampler2D aoMap;
layout (binding = 2) uniform sampler2D wallTex;

const vec2 mapSize = vec2(2048, 2048);

// Returns a random number based on a vec3 and an int.
float random(vec3 seed, int i){
	vec4 seed4 = vec4(seed,i);
	float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
	return fract(sin(dot_product) * 43758.5453);
}

vec2 CalcScreenTexCoord()
{
    return gl_FragCoord.xy / screenSize;
}

float CalcShadowFactor(vec4 LightSpacePos)
{
    vec3 ProjCoords = LightSpacePos.xyz / LightSpacePos.w;
    vec2 UVCoords;
    UVCoords.x = 0.5 * ProjCoords.x + 0.5;
    UVCoords.y = 0.5 * ProjCoords.y + 0.5;
    float z = 0.5 * ProjCoords.z + 0.5;

    float xOffset = 1.0/mapSize.x;
    float yOffset = 1.0/mapSize.y;

    float Factor = 0.0;
	float bias = 0.000001;

    for (int y = -2; y <= 2; y++) {
        for (int x = -2; x <= 2; x++) {
            vec2 Offsets = vec2(x * xOffset, y * yOffset);
            vec3 UVC = vec3(UVCoords + Offsets, z + bias);
            Factor += texture(shadowMap, UVC);
        }
    }

    return (0.5 + (Factor / 24.0));
}



void main()
{   
	// Ambient
    vec3 ambient = 0.7 * directionalLight.ambientColor;
	float occlusion = texture(aoMap, CalcScreenTexCoord()).r;
	ambient *= occlusion;
	
	// Diffuse
	vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(-directionalLight.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * directionalLight.color;
	
	// Specular
    vec3 viewDir = normalize(FragPos - viewPos);
    vec3 reflectDir = reflect(-lightDir, normal);  
    vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), 32);
    vec3 specular = spec * directionalLight.color;
	
	vec3 tex = texture(wallTex, TexCoords).rgb;
	
	float shadow = CalcShadowFactor(LightSpacePos);
    vec3 lighting = (ambient + shadow * (diffuse + specular)) * vec3(objectColor) * tex;
	
    FragColor = vec4(lighting, objectColor.a);
}