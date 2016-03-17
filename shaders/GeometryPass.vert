#version 330
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	vec4 viewPos = view * model * vec4(position, 1.0f);
	FragPos = viewPos.xyz;
	gl_Position = projection * viewPos;
	
	mat3 normalMatrix = transpose(inverse(mat3(view * model)));
	Normal = normalMatrix * normal;
}