#version 420 core

layout (vertices = 3) out;

in vec3 vPosition[];
in vec3 vNormal[];

out vec3 tcPosition[];
out vec3 tcNormal[];
out vec3 tcCurve[];

uniform float blobDistance;

#define ID gl_InvocationID
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
	tcPosition[ID] = vPosition[ID];
	tcNormal[ID] = vNormal[ID];
	uint adj_ID = (ID + 1) % 3;
	tcCurve[ID] = curve(
		vPosition[adj_ID] - vPosition[ID],
		vNormal[ID],
		vNormal[adj_ID]);
	if (ID == 0)
	{
		int tessLevel = 1;
		tessLevel += int(blobDistance < 80.0);
		tessLevel += int(blobDistance < 10.0);
		gl_TessLevelInner[0] = tessLevel;
		gl_TessLevelOuter[0] = tessLevel;
		gl_TessLevelOuter[1] = tessLevel;
		gl_TessLevelOuter[2] = tessLevel;
	}
}
