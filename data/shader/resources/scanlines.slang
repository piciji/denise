#version 450

/*
   Author: PiCiJi
   License: Public domain
*/

layout(push_constant) uniform Push
{
    vec4 OutputSize;

    float autoEmu_pal;
    float autoEmu_lace;
    float scanlineShade;
    float gamma;
    float autoEmu_tvGamma;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

#pragma parameter scanline_head    "[SCANLINES AND GAMMA]"  0 0 0.001 0.001
#pragma parameter scanlineShade    "Scanline Shade"         20 0 100 1
#pragma parameter gamma            "Gamma"                  100 30 280 1
#pragma parameter autoEmu_tvGamma  "TV Gamma"               0 0 1 1
#pragma parameter autoEmu_pal      "PAL"                    1 0 1 1
#pragma parameter autoEmu_lace     "Interlace"              0 0 1 1

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
    vec3 colorUp = texture(Source, vTexCoord.xy + vec2( 0.0, -1.0 / params.OutputSize.y ) ).rgb;
    vec3 colorDown = texture(Source, vTexCoord.xy + vec2( 0.0, 1.0 / params.OutputSize.y ) ).rgb;
    float shade = 1.0 - (params.scanlineShade / 100.0);
    float _gamma = params.gamma / 100.0;
    if (bool(params.autoEmu_tvGamma) && bool(params.autoEmu_pal))
        _gamma *= 2.8 / 2.2;

    int lineFactor = 0;
    if ((int(params.autoEmu_lace) == 0) && (params.scanlineShade > 0.0))
        lineFactor = int(floor(mod(vTexCoord.y * params.OutputSize.y, 2.0)));

    color = clamp(color, 0.0, 1.0);
    color.r = mix( pow(color.r, _gamma), pow((colorUp.r + colorDown.r) / 2.0, _gamma) * shade, lineFactor );
    color.g = mix( pow(color.g, _gamma), pow((colorUp.g + colorDown.g) / 2.0, _gamma) * shade, lineFactor );
    color.b = mix( pow(color.b, _gamma), pow((colorUp.b + colorDown.b) / 2.0, _gamma) * shade, lineFactor );

    color = clamp(color, 0.0, 1.0);
    FragColor = vec4( color, 1.0 );
}