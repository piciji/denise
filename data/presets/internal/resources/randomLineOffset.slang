#version 450

/*
   Author: PiCiJi
   License: Public domain
*/

layout(push_constant) uniform Push
{
    vec4 OutputSize;
    uint FrameCount;

    float randomLineOffset;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

#pragma parameter randomLineOffset      "Random Line Offset"    0 0 10.0 0.01

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
    if (params.randomLineOffset > 0) {
        float lineFactor = pow(params.randomLineOffset, 2.0) / 10000.0;
        vec2 xy = vTexCoord.xy;
        float _seed = float(params.FrameCount) * (20.0 / 100.0);

        float offset = random(vec2(_seed * xy.y, xy.y + (_seed * xy.y))) * lineFactor;
        float x0 = xy.x + offset;
        float x1 = x0 + params.OutputSize.z;
        vec4 tex0 = texture(Source, vec2( fract(x0), xy.y )).xyzw;
        vec4 tex1 = texture(Source, vec2( fract(x1), xy.y )).xyzw;
        FragColor = mix(tex0, tex1, fract(x0 * params.OutputSize.x));
    } else {
        FragColor = texture(Source, vTexCoord.xy).xyzw;
    }
}