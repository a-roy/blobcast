#version 420 core

layout (triangles) in;

in vec3 tcPosition[];
in vec3 tcNormal[];
in vec3 tcCurve[];

out vec3 FragPos;
out vec3 Normal;
out vec4 LightSpacePos;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMat;

void main()
{
	vec3 position =
		tcPosition[0] * gl_TessCoord.x +
		tcPosition[1] * gl_TessCoord.y +
		tcPosition[2] * gl_TessCoord.z -
		tcCurve[0] * gl_TessCoord.x * gl_TessCoord.y -
		tcCurve[1] * gl_TessCoord.y * gl_TessCoord.z -
		tcCurve[2] * gl_TessCoord.x * gl_TessCoord.z;
	Normal = normalize(
		tcNormal[0] * gl_TessCoord.x +
		tcNormal[1] * gl_TessCoord.y +
		tcNormal[2] * gl_TessCoord.z);

	gl_Position = projection * view * vec4(position, 1.0);
	FragPos = position;
	LightSpacePos = lightSpaceMat * vec4(position, 1.0);
}
