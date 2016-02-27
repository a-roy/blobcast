#version 420 core

layout (vertices = 3) out;

in vec3 vPosition[];
in vec3 vNormal[];

out vec3 tcPosition[];
out vec3 tcNormal[];

uniform float blobDistance;

#define ID gl_InvocationID

void main()
{
	tcPosition[ID] = vPosition[ID];
	tcNormal[ID] = vNormal[ID];
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
