#version 330
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D positionMap;
uniform mat4 projection;

float radius = 1.0;
const int MAX_KERNEL_SIZE = 64;
uniform vec3 gKernel[MAX_KERNEL_SIZE];

void main()
{
    vec3 pos = texture(positionMap, TexCoord).xyz;

    float occlusion = (pos + gKernel[0]).z;

    for (int i = 0 ; i < MAX_KERNEL_SIZE ; i++) {
        vec3 sample = pos + gKernel[i]; // generate a random point
        vec4 offset = vec4(sample, 1.0); // make it a 4-vector
        offset = projection * offset; // project on the near clipping plane
        offset.xy /= offset.w; // perform perspective divide
        offset.xy = offset.xy * 0.5 + vec2(0.5); // transform to (0,1) range

        float sampleDepth = texture(positionMap, offset.xy).b;

        if (abs(pos.z - sampleDepth) < radius) {
            occlusion += step(sampleDepth, sample.z);
        }
    }

    occlusion = 1.0 - occlusion/64.0;

    FragColor = vec4(occlusion);
}