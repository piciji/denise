#version 450

/*
   Author: PiCiJi
   License: Public domain
*/

#define PI 3.14159265358979323846

layout(push_constant) uniform Push
{
    vec4 OutputSize;

    float noEncoding;
    float phaseErrorPAL;
    float phaseErrorNTSC;
    float autoEmu_cropTop;
    float autoEmu_lace;
    float autoEmu_pal;
    float autoEmu_lumaChroma;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

#pragma parameter Encoding_head         "[YUV/YIC ENCODING]" 0 0 0.001 0.001
#pragma parameter noEncoding            "Disable Encoding"   0 0 1 1
#pragma parameter phaseErrorPAL         "Phase Error PAL"    22.5 -45 45 0.5
#pragma parameter phaseErrorNTSC        "Phase Error NTSC"   0 -45 45 0.5
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

// lace : e, e, o, o, e, e, o, o, ...
// non lace: e, o, e, o, e, o, ...

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main() {
    vec4 color = texture(Source, vTexCoord);
    vec3 lumaChroma;
    float flags = color.w;

    if (bool(params.autoEmu_lumaChroma)) {
        // YUV / YIC input
        lumaChroma = color.xyz;
    } else {
        // RGB input
        if (bool(params.autoEmu_pal)) {
            lumaChroma = vec3(color.rgb * mat3(0.299,0.587,0.114,-0.147407,-0.289391,0.436798,0.614777,-0.514799,-0.099978));
        } else {
            lumaChroma = vec3(color.rgb * mat3(0.23485876230514607, 0.6335007388077467, 0.13164049888710716, 0.4409594767911895, -0.27984362502847304, -0.16111585176271648, 0.14630060102591497, -0.5594814826856017, 0.4131808816596867));
        }
        flags = 0;
    }

    if (bool(params.noEncoding)) {
        color = vec4(lumaChroma.xyz, flags);
    } else {
        if (bool(params.autoEmu_pal)) {
            float _phaseError = params.phaseErrorPAL * PI / 180.0;
            float rotU = cos(_phaseError);
            float rotV = sin(_phaseError);
            int oddLine = ((int(params.autoEmu_cropTop) >> int(params.autoEmu_lace)) & 1) ^ (int(params.autoEmu_lumaChroma) ^ 1);

            vec3 yuvEven = vec3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV, lumaChroma.z * rotU + lumaChroma.y * rotV);
            vec3 yuvOdd = vec3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV * -1, lumaChroma.z * rotU + lumaChroma.y * rotV * -1);

            int oddLineFrame = int(floor(mod(floor(vTexCoord.y * params.OutputSize.y / float( 1 << int(params.autoEmu_lace) )), 2.0)));
            color = vec4( mix(yuvOdd, yuvEven, (oddLineFrame ^ int(oddLine)) ), flags );
        } else {
            float _phaseError = params.phaseErrorNTSC * PI / 180.0;
            float rotU = cos(_phaseError);
            float rotV = sin(_phaseError);

            vec3 yuvEven = vec3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV, lumaChroma.z * rotU + lumaChroma.y * rotV);
            color = vec4(yuvEven, flags);
        }
    }

    FragColor = color;
}