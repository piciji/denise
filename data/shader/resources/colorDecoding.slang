#version 450

/*
   Author: PiCiJi
   License: Public domain
*/

layout(push_constant) uniform Push
{
    vec4 OutputSize;

    float hanoverBars;
    float noEncoding;
    float autoEmu_cropTop;
    float autoEmu_lace;
    float autoEmu_pal;
    float autoEmu_lumaChroma;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

#pragma parameter hanoverBars           "Hanover Bars"       10 -100 100 1
#pragma parameter noEncoding            "Disable Encoding"   0 0 1 1
#pragma parameter autoEmu_lumaChroma    "Luma Chroma Input"  0 0 1 1
#pragma parameter autoEmu_cropTop       "Crop Top"           1 0 100 1
#pragma parameter autoEmu_pal           "PAL"                1 0 1 1
#pragma parameter autoEmu_lace          "Interlace"          0 0 1 1

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
    vec3 color;

    if (bool(params.autoEmu_pal)) {
        if (bool(params.noEncoding)) {
            color = texture(Source, vTexCoord.xy).xyz * mat3(1.0,0.0,1.140251,1.0,-0.39393070,-0.58080921,1.0,2.0283976,0.0);
        } else {
            int oddLine = ((int(params.autoEmu_cropTop) >> int(params.autoEmu_lace)) & 1) ^ (int(params.autoEmu_lumaChroma) ^ 1);
            int lineFactor = int(floor(mod(floor(vTexCoord.y * params.OutputSize.y / float( 1 << int(params.autoEmu_lace) )), 2.0)));
            float _hanoverBars = (100.0 - abs(params.hanoverBars)) / 100.0;
            float _hanoverBarsAlt = 1.0;
            if (params.hanoverBars > 0)
                _hanoverBarsAlt = (100.0 + params.hanoverBars) / 100.0;

            vec3 yuv = (texture(Source, vTexCoord.xy).xyz);
            vec3 yuvLineBefore;
            float yOffset = float( 1 << int(params.autoEmu_lace) ) / params.OutputSize.y;

            if (!(bool(params.autoEmu_lumaChroma)) && (vTexCoord.y < yOffset ) ) {
                // no hidden delay line
                yuvLineBefore = texture(Source, vTexCoord.xy + vec2(0.0, yOffset) ).xyz;
            } else {
                yuvLineBefore = texture(Source, vTexCoord.xy + vec2(0.0, -1.0 * yOffset) ).xyz;
            }

            vec2 merged = vec2(yuv.y + yuvLineBefore.y, yuv.z + yuvLineBefore.z) * mix(_hanoverBars, _hanoverBarsAlt, lineFactor ^ int(oddLine)) * 0.5;
            color = vec3(yuv.x, merged.x, merged.y) * mat3(1.0,0.0,1.140251,1.0,-0.39393070,-0.58080921,1.0,2.0283976,0.0);
        }
    } else {
        color = texture(Source, vTexCoord.xy).xyz * mat3(1.0, 1.630, 0.317, 1.0, -0.378, -0.466, 1.0, -1.089, 1.677);
    }

    FragColor = vec4(color, 1.0);
}