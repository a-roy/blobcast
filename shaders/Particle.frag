#version 400

uniform sampler2D tex;
out vec4 fragColor;
in vec4 color;

void main()
{
	fragColor = texture(tex, gl_PointCoord);// * color;
}
