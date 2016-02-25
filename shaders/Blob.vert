#version 420 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMat;

out vec3 FragPos;
out vec3 Normal;
out vec4 lightSpacePos;

void main()
{
	gl_Position = projection * view * vec4(position, 1.0);
	FragPos = position;
	Normal = normalize(normal);
	lightSpacePos = lightSpaceMat * vec4(position, 1.0);
}