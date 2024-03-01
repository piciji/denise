#version 450

/*
   Author: PiCiJi
   License: Public domain
*/

layout(push_constant) uniform Push
{
    vec4 OutputSize;

    float autoEmu_pal;
    float gamma;
    float autoEmu_tvGamma;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

#pragma parameter gamma            "Gamma"              100 30 280 1
#pragma parameter autoEmu_tvGamma  "TV Gamma"           0 0 1 1
#pragma parameter autoEmu_pal      "PAL"                1 0 1 1

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


void main() {
    vec3 color = texture(Source, vTexCoord).rgb;

    float _gamma = params.gamma / 100.0;
    if (bool(params.autoEmu_tvGamma) && bool(params.autoEmu_pal))
        _gamma *= 2.8 / 2.2;

    color = clamp(color, 0.0, 1.0);
    color = pow(color, vec3(_gamma));

    FragColor = vec4( color, 1.0 );
}