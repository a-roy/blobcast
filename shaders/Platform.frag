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

layout (binding = 0) uniform sampler2DArray shadowMap;
layout (binding = 1) uniform sampler2D aoMap;
layout (binding = 2) uniform sampler2D wallTex;

const vec2 mapSize = vec2(1024, 1024);

#define EPSILON 0.00001

vec2 CalcScreenTexCoord()
{
    return gl_FragCoord.xy / screenSize;
}

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // Get closest depth value from light's perspective
    float closestDepth = texture(shadowMap, vec3(projCoords.xy, 0)).r; 
    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
    float bias = 0.00001;
    
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0).xy;
    for(int x = -2; x <= 2; ++x)
    {
        for(int y = -2; y <= 2; ++y)
        {
            float pcfDepth = texture(shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, 0)).r; 
            shadow += currentDepth + bias > pcfDepth  ? 0.0 : 1.0;        
        }    
    }
    shadow = (0.5 + (shadow / 16.0));
    
    // // Keep the shadow at 0.0 when outside the far plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 1.0;
        
    return shadow;
}

// float ShadowCalculation2(vec4 LightSpacePos)
// {
    // vec3 projCoords = LightSpacePos.xyz / LightSpacePos.w;
    // vec2 UVCoords;
    // UVCoords.x = 0.5 * projCoords.x + 0.5;
    // UVCoords.y = 0.5 * projCoords.y + 0.5;
    // float z = 0.5 * projCoords.z + 0.5;
	
	// if(projCoords.z > 1.0)
		// return 0.0;
	
	// float xOffset = 1.0/mapSize.x;
	// float yOffset = 1.0/mapSize.y;
	
	// float factor = 0.0;
	
	// for(int y = -2; y <= 2; y++){
		// for(int x = -2; x <= 2; x++){
			// vec2 offsets = vec2(x * xOffset, y * yOffset);
			// vec4 UVC = vec4(UVCoords + offsets, 0, z + EPSILON);
			// factor += texture(shadowMap, UVC);
		// }
	// }
	
	// return (0.5 + (factor / 18.0));
// }


void main()
{   
	// Ambient
    vec3 ambient = 0.5 * directionalLight.ambientColor;
	ambient *= texture(aoMap, CalcScreenTexCoord()).r;
	
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
	
	float shadow = ShadowCalculation(LightSpacePos);
    vec3 lighting = (ambient + shadow * (diffuse + specular)) * vec3(objectColor);
	
    FragColor = vec4(lighting, objectColor.a);
}