#version 420 core
out float FragColor;

layout (binding = 0) uniform sampler2D gPosition;
layout (binding = 1) uniform sampler2D gNormal;

uniform vec2 screenSize;

float pi = 3.1415926535897932384626433832795;
float pi2 = 6.2831853071795864769252867665590;

float sampleRadius = 1.5;
float shadowScalar = 0.5;
float depthThreshold = 0.00025;
float shadowContrast = 0.5;
float aoBias = 0.002;
float epsilon = 0.01;
int numSamples = 16;
int numSpiralTurns = 8;

vec2 focalLength = vec2(1.0 / tan(45.0 * 0.5) * screenSize.y / screenSize.x, 1.0 / tan(45.0 * 0.5));

vec3 GetViewPos(vec2 uv)
{
	return texture(gPosition, uv).xyz;
}

vec2 Hammersley(uint i, uint N)
{
  return vec2( float(i) / float(N), float(bitfieldReverse(i)) * 2.3283064365386963e-10 );
}

float RandAngle()
{
  uint x = uint(gl_FragCoord.x);
  uint y = uint(gl_FragCoord.y);
  return (30u * x ^ y + 10u * x * y);
}

vec2 TapLocation(int sampleNumber, float spinAngle, out float ssR)
{
	float alpha = float(sampleNumber + 0.5) * (1.0 / numSamples);
	float angle = alpha * (numSpiralTurns * 6.28) + spinAngle;
	ssR = alpha;
	return vec2(cos(angle), sin(angle));
}

void main()
{
	float occlusion = 0.0;
	vec2 aoTexCoord = gl_FragCoord.xy/screenSize;

	vec3 P = GetViewPos(aoTexCoord);
	vec3 N = texture(gNormal, aoTexCoord).xyz;
	
	float uvDiskRadius = 0.5 * focalLength.y * sampleRadius / P.z;
	
	vec2 pixelPosC = gl_FragCoord.xy;
	pixelPosC.y = screenSize.y - pixelPosC.y;
	
	float randPatternRotAngle = (3 * int(pixelPosC.x)^int(pixelPosC.y) + int(pixelPosC.x)*int(pixelPosC.y)) *10.0;
	float aoAlpha = 2.0 * pi / numSamples + randPatternRotAngle;
	
	for(int i = 0; i < numSamples; ++i)
	{
		float ssR;
		vec2 aoUnitOffset = TapLocation(i, randPatternRotAngle, ssR);
		ssR *= uvDiskRadius;
		
		vec2 aoTexS = aoTexCoord + ssR * aoUnitOffset;
		vec3 Pi = GetViewPos(aoTexS);
		vec3 V = Pi - P;
		float Vv = dot(V ,V);
		float Vn = dot(V, N);
		float aoF = max(sampleRadius * sampleRadius - Vv, 0.0);
		occlusion += aoF * aoF * aoF * max((Vn - aoBias)/(epsilon + Vv), 0.0);
	}
	
	float aoTemp = sampleRadius * sampleRadius * sampleRadius;
	occlusion /= aoTemp * aoTemp;
	occlusion = max(0.0, 1.0 - occlusion * (1.0/numSamples));

	FragColor = pow(occlusion, 8.0);
}

// void main()
// {
	// float occlusion = 0.0;
	// vec2 aoTexCoord = gl_FragCoord.xy / screenSize;
	// vec3 P = GetViewPos(aoTexCoord);
	// vec3 N = texture(gNormal, aoTexCoord).xyz;
	// float perspectiveRadius = sampleRadius / P.z;
	
	// // Main sample loop
	// for(int i = 0; i < numSamples; ++i)
	// {
		// // Generate sample position
		// vec2 E = Hammersley(i, numSamples) * vec2(pi, pi2);
		// E.y += RandAngle(); // Apply random angle rotation
		// vec2 sE = vec2(cos(E.y), sin(E.y)) * perspectiveRadius * cos(E.x);
		// vec2 samplePos = gl_FragCoord.xy / screenSize + sE;
		
		// // Create alchemy helper variables
		// vec3 Pi = GetViewPos(samplePos);
		// vec3 V = Pi - P;
		// float sqrLen = dot(V, V);
		// float Heaveside = step(sqrt(sqrLen), sampleRadius);
		// float dD = depthThreshold * P.z;
		
		// // For arithmetically removing edge-bleeding error
		// // introduced by clamping the AO map.
		// float edgeError = step(0.0, samplePos.x) * step(0.0, 1.0 - samplePos.x) *
						  // step(0.0, samplePos.y) * step(0.0, 1.0 - samplePos.y);
						  
		// // Summation of occlusion factor
		// occlusion += (max(0.0, dot(N, V) + dD) * Heaveside * edgeError) / (sqrLen + epsilon);
	// }
	
	// occlusion *= (2 * shadowScalar) / numSamples;
	// occlusion = max(0.0, 1.0 - pow(occlusion, shadowContrast));
	// FragColor = occlusion;
// }
