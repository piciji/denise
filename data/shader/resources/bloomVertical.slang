#version 450

/*
   Author: PiCiJi
   License: Public domain
   lutBloom (Gaussian Blur) texture generation in Denise source: program/video/luts.cpp
*/

layout(push_constant) uniform Push
{
    vec4 OutputSize;

    float bloom_weight;
    float bloom_glow;
    float bloom_radius;
    float bloom_variance;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

#pragma parameter bloom_glow       "Bloom Glow"      0 0 200 1
#pragma parameter bloom_weight     "Bloom Weight"    2.55 0 3.0 0.01
#pragma parameter bloom_radius     "Bloom Radius"    4 1 6 1
#pragma parameter bloom_variance   "Bloom Variance"  2.7 1.0 12.0 0.1

#define lutBloomTexWidth 1553.0
#define lutBloomTexHeight 6.0

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main() {
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D GammaOrShades;
layout(set = 0, binding = 4) uniform sampler2D lutBloom;


float bloomTex( float pos, float posY ) {
    vec4 packed = texture(lutBloom, vec2(pos / lutBloomTexWidth, posY / lutBloomTexHeight) ).rgba;

    int unpacked = int((uint(packed.r * 255.0) & 0xff)
                   | ((uint(packed.g * 255.0) & 0xff) << 8)
                   | ((uint(packed.b * 255.0) & 0xff) << 16)
                   | ((uint(packed.a * 255.0) & 0xff) << 24));

    return float(unpacked) / 1000000.0f;
}

void main() {
    if (params.bloom_glow == 0) {
        FragColor = vec4(texture(Source, vTexCoord).rgb, 1.0);
        return;
    }

    vec3 sum = vec3(0.0, 0.0, 0.0);
    float _weight = 3.0 - params.bloom_weight;

    int n = int(params.bloom_radius) << 1;
    int offset = int((params.bloom_variance - 1.0) * 10.0) * 14;
    float offsetY = int(params.bloom_radius) - 1;

    do {
        sum += (texture(Source, vTexCoord + vec2(0.0, -1.0 * float(n) / params.OutputSize.y) ).rgb
            + texture(Source, vTexCoord + vec2(0.0, float(n) / params.OutputSize.y) ).rgb) * bloomTex(offset + n, offsetY);

        n -= 1;
    } while(n > 0);

    sum += texture(Source, vTexCoord).rgb * bloomTex(offset + n, offsetY);

    FragColor = vec4(clamp(texture(GammaOrShades, vTexCoord ).rgb + ( pow(sum.rgb, vec3(_weight, _weight, _weight)) * (params.bloom_glow / 100.0) ), 0.0, 1.0), 1.0);
}