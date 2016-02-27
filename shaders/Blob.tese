#version 420 core

layout (triangles) in;

in vec3 tcPosition[];
in vec3 tcNormal[];

out vec3 FragPos;
out vec3 Normal;
out vec4 LightSpacePos;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMat;

#define EPSILON 0.05

// Compute a curvature parameter for quadratic approximation
vec3 curve(vec3 d, vec3 n0, vec3 n1)
{
	vec3 n_avg = (n0 + n1) * 0.5;
	vec3 n_dev = (n0 - n1) * 0.5;
	float d_avg = dot(d, n_avg);
	float d_dev = dot(d, n_dev);
	float c_dev = dot(n0, n_dev);
	float c_avg = 1 - 2 * c_dev;

	// Fall back to linear interpolation for wide angles
	if (abs(abs(c_avg) - 1.0) < EPSILON)
		return vec3(0.0);

	return (d_dev) / (1 - c_dev) * n_avg + d_avg / c_dev * n_dev;
}

void main()
{
	vec3 d1 = tcPosition[1] - tcPosition[0];
	vec3 d2 = tcPosition[2] - tcPosition[1];
	vec3 d3 = tcPosition[2] - tcPosition[0];
	vec3 c1 = curve(d1, tcNormal[0], tcNormal[1]);
	vec3 c2 = curve(d2, tcNormal[1], tcNormal[2]);
	vec3 c3 = curve(d3, tcNormal[0], tcNormal[2]);

	vec3 position =
		tcPosition[0] * gl_TessCoord.x +
		tcPosition[1] * gl_TessCoord.y +
		tcPosition[2] * gl_TessCoord.z -
		c1 * gl_TessCoord.x * gl_TessCoord.y -
		c2 * gl_TessCoord.y * gl_TessCoord.z -
		c3 * gl_TessCoord.x * gl_TessCoord.z;
	vec3 normal =
		tcNormal[0] * gl_TessCoord.x +
		tcNormal[1] * gl_TessCoord.y +
		tcNormal[2] * gl_TessCoord.z -
		c1 * gl_TessCoord.x * gl_TessCoord.y -
		c2 * gl_TessCoord.y * gl_TessCoord.z -
		c3 * gl_TessCoord.x * gl_TessCoord.z;

	Normal = normalize(normal);
	gl_Position = projection * view * vec4(position, 1.0);
	FragPos = position;
	LightSpacePos = lightSpaceMat * vec4(position, 1.0);
}
