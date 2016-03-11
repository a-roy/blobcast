#version 330
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D colorMap;

float offsets[4] = float[](-1.5, -0.5, 0.5, 1.5);

void main()
{
	vec3 color = vec3(0.0, 0.0, 0.0);
	
	for(int i=0; i<4; i++){
		for(int j=0; j<4; j++){
			vec2 tex = TexCoord;
			tex.x = TexCoord.x + offsets[j] / textureSize(colorMap, 0).x;
			tex.y = TexCoord.y + offsets[i] / textureSize(colorMap, 0).y;
			color += vec3(texture(colorMap, tex));
		}
	}
	
	color /= 16.0;
	
	FragColor = vec4(color, 1.0);
}