#version 420 core

in vec2 vPosition;
out vec4 fColor;

uniform float uInnerRadius;
uniform float uOuterRadius;
uniform float uForward;
uniform float uBackward;
uniform float uRight;
uniform float uLeft;

void main()
{
	float dist = length(vPosition);
	vec2 dir = normalize(vPosition);
	float extrusion =
		pow(max(dot(dir, vec2( 0,  1)), 0), 2) * uForward +
		pow(max(dot(dir, vec2( 0, -1)), 0), 2) * uBackward +
		pow(max(dot(dir, vec2( 1,  0)), 0), 2) * uRight +
		pow(max(dot(dir, vec2(-1,  0)), 0), 2) * uLeft;
	float radius = uInnerRadius + (uOuterRadius - uInnerRadius) * extrusion;
	float alphaI = 0.5 * (
		smoothstep(uInnerRadius - 0.01, uInnerRadius, dist) -
		smoothstep(uInnerRadius, uInnerRadius + 0.01, dist));
	float alphaO = 0.5 * (
		smoothstep(uOuterRadius - 0.01, uOuterRadius, dist) -
		smoothstep(uOuterRadius, uOuterRadius + 0.01, dist));
	float alpha = alphaI + alphaO +
		smoothstep(radius - 0.01, radius, dist) -
		smoothstep(radius, radius + 0.01, dist);
	if (alpha == 0.0)
		discard;
	fColor = vec4(0, 0, 0, alpha);
}
