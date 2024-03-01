#version 450

/*
   Author: PiCiJi
   License: Public domain
   lutBandwidth (FIR filter) texture generation in Denise source: program/video/luts.cpp
*/

layout(push_constant) uniform Push
{
    vec4 OutputSize;
    float autoEmu_subRegion;
    float firTaps;

} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

#pragma parameter autoEmu_subRegion     "Sub Region"   0 0 3 1
#pragma parameter firTaps               "Bandwidth"    9 1 21 2

#define lutBandwidthTexWidth 74.0
#define lutBandwidthTexHeight 88.0
#define lutBandwidthTexRoundsStorageOffset 24.0

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
layout(set = 0, binding = 3) uniform sampler2D lutBandwidth;

float bandwidth( float pos, float posY ) {
    vec4 packed = texture(lutBandwidth, vec2(pos / lutBandwidthTexWidth, posY / lutBandwidthTexHeight) ).rgba;

    int unpacked = int((uint(packed.r * 255.0) & 0xff)
        | ((uint(packed.g * 255.0) & 0xff) << 8)
        | ((uint(packed.b * 255.0) & 0xff) << 16)
        | ((uint(packed.a * 255.0) & 0xff) << 24));

    return float(unpacked) / 1000000.0f;
}

void main() {
    int n = 1;
    float offsetY = int(params.autoEmu_subRegion) * 22 + int(params.firTaps);

    int rounds = int(texture(lutBandwidth, vec2( lutBandwidthTexRoundsStorageOffset / lutBandwidthTexWidth, offsetY / lutBandwidthTexHeight) ).r * 255.0);

    vec3 yuv = texture(Source, vTexCoord).xyz * vec3(
        bandwidth(0, offsetY),
        bandwidth(25, offsetY),
        bandwidth(50, offsetY)
    );

    do {
        yuv += (texture(Source, vTexCoord + vec2((-1.0 * float(n)) / params.OutputSize.x, 0.0) ).xyz + texture(Source, vTexCoord + vec2(float(n) / params.OutputSize.x, 0.0) ).xyz) *
            vec3(
                bandwidth(0 + n, offsetY),
                bandwidth(25 + n, offsetY),
                bandwidth(50 + n, offsetY)
            );

        n += 1;
    } while(n <= rounds);

    FragColor = vec4( yuv, 1.0 );
}