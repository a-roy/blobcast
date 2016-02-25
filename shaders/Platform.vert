#version 420 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMat;

out vec3 fragPos;
out vec3 Normal;
out vec2 fragTexCoord;
out vec4 lightSpacePos;

void main()
{
	gl_Position = projection * view * model * vec4(position, 1.0f);
	fragPos = vec3(model * vec4(position, 1.0f));
	Normal = mat3(transpose(inverse(model))) * normal;
	lightSpacePos = lightSpaceMat * model * vec4(position, 1.0);
	fragTexCoord = texCoord;
}