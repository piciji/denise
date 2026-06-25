#version 450

/*
   Author: PiCiJi
   License: Public domain
*/

layout(push_constant) uniform Push
{
    uint FrameCount;

    float lumaNoise;
    float chromaNoise;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

#pragma parameter lumaNoise     "Luma Noise"        0 0 100.0 0.1
#pragma parameter chromaNoise   "Chroma Noise"      0 0 100.0 0.1

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

float random( vec2 seed ) {
    int n = int((seed.x * 40.0) + (seed.y * 6400.0));
    n = (n<<13) ^ n;
    return 1.0 - float((n * (((n * n) * 15731) + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0;
}

void main() {
    vec2 xy = vTexCoord.xy;
    float _seed = float(params.FrameCount) * (20.0 / 100.0);

    float y = random(xy + vec2(_seed * xy.x, _seed * xy.y)) * (params.lumaNoise / 100.0);
    float u = random(xy + vec2(_seed * xy.y, _seed * xy.x)) * (params.chromaNoise / 100.0);
    float v = random(xy - vec2(_seed * xy.x, _seed * xy.y)) * (params.chromaNoise / 100.0);
    FragColor = texture(Source, xy).xyzw + vec4(y, u, v, 0.0);
}