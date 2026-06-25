#version 450

/*
   Author: PiCiJi
   License: Public domain
*/

layout(push_constant) uniform Push
{
    vec4 OriginalSize;

    float radial_distortion;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

#pragma parameter radial_distortion     "Radial Distortion"     5 0 100 1

#define borderOffset 4.0

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


vec2 radialDistortion( vec2 xy, float intensity ) {
    vec2 center = xy - vec2(0.5, 0.5);
    float dist = dot(center,center) * intensity;
    return xy + (center * (1.0 + dist) * dist);
}

void main() {
    float intensity = params.radial_distortion / 100.0;
    float _s = params.OriginalSize.x;

    float xL = borderOffset / _s;
    float xR = (_s - borderOffset) / _s;

    float Scale = 1.0f / sqrt( distance( radialDistortion( vec2(xL * intensity, 0.0), intensity ), radialDistortion( vec2(1.0 - intensity + xR * intensity, 0.0), intensity ) ) );

    vec2 xy = ((radialDistortion(vTexCoord.xy, intensity) - vec2(0.5, 0.5)) * Scale) + vec2(0.5, 0.5);

    FragColor = texture(Source, xy);
}