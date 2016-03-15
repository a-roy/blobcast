#version 330

uniform mat4 uView;
uniform mat4 uProj;
uniform float uSize;

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec4 vertex_color;

out vec4 color;

void main()
{
	vec4 Vertex = vec4(vertex_position.x, vertex_position.y, vertex_position.z,
	1.0);
	vec4 eyePos = uView * Vertex;
    gl_Position = uProj * eyePos;

	float dist = length(eyePos.xyz);
    float att = inversesqrt(0.1f*dist);
    gl_PointSize = uSize * att;

	color = vertex_color;
}
