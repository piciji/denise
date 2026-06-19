#version 450

/*
   Author: PiCiJi
   License: Public domain
*/

#define PI 3.14159265358979323846

layout(push_constant) uniform Push
{
    vec4 OutputSize;
    vec4 SourceSize;

    float phaseErrorPAL;
    float phaseErrorNTSC;
    float AppCropTop;
    float AppCropLeft;
    float AppInterlace;
    float AppPAL;

    float vic_glitches;
    float glitch_cas;
    float glitch_ras;
    float glitch_phi;
    float glitch_aec;
    float glitch_ba;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

#pragma parameter label0                "[YUV/YIC ENCODING]"        0 0 0.001 0.001
#pragma parameter phaseErrorPAL         "  Phase Error PAL"         22.5 -45 45 0.5
#pragma parameter phaseErrorNTSC        "  Phase Error NTSC"        0 -45 45 0.5
#pragma parameter empty0                " "                         0 0 0.001 0.001
#pragma parameter Glitches_head         "[VIC-II Glitches (ignore for Amiga)]" 0 0 0.001 0.001
#pragma parameter vic_glitches          "  enable next 5 options"   0 0 1 1
#pragma parameter glitch_aec            "    AEC"                   10.5 0 25 0.1
#pragma parameter glitch_ba             "    BA"                    10.5 0 25 0.1
#pragma parameter glitch_phi            "    PHI"                   10.5 0 25 0.1
#pragma parameter glitch_ras            "    RAS"                   10.5 0 25 0.1
#pragma parameter glitch_cas            "    CAS"                   10.5 0 25 0.1
#pragma parameter empty1                " "                         0 0 0.001 0.001

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

float vicGlitches(float luma, int flags) {
    float _y;
    vec4 ySrc = texture(Source, vTexCoord.xy + vec2( -2.0 / float(params.SourceSize.x), 0.0));
    int _col = int((uint(ySrc.b * 255.0) & 255)
    | ((uint(ySrc.g * 255.0) & 255) << 8)
    | ((uint(ySrc.r * 255.0) & 255) << 16)
    | ((uint(ySrc.a * 255.0) & 255) << 24));

    ySrc.b = (float(_col & 1023) - 256.0) / 255.0;
    ySrc.g = (float((_col >> 10) & 1023) - 256.0) / 255.0;
    ySrc.r = (float((_col >> 20) & 1023) - 256.0) / 255.0;

    if (bool(params.AppPAL))
        _y = ySrc.r * 0.299 + ySrc.g * 0.587 + ySrc.b * 0.114;
    else
        _y = ySrc.r * 0.23485876230514607 + ySrc.g * 0.6335007388077467 + ySrc.b * 0.13164049888710716;

    if (_y <= 0.0)
        return luma;

    float AEC = params.glitch_aec * params.glitch_aec * 0.0001;
    float BA = params.glitch_ba * params.glitch_ba * 0.0001;
    float PHI0 = params.glitch_phi * params.glitch_phi * 0.0001;
    float CAS = params.glitch_cas * params.glitch_cas * 0.0001;
    float RAS = params.glitch_ras * params.glitch_ras * 0.0001;

    float xposF = vTexCoord.x * params.SourceSize.x;
    int xpos = int(xposF);
    xpos += int(params.AppCropLeft) + (2 << (int(params.AppPAL) ^ 1)) + 4;

    xpos &= 7;
    luma *= 1.0 - ((flags & 1) * BA);
    luma *= 1.0 - (((flags >> 1) & 1) * AEC);
    luma *= 1.0 - (((xpos >> 2) & 1) * PHI0);
    luma *= 1.0 - (((xpos >> 1) & 1) * CAS);
    luma *= 1.0 - (((((~xpos >> 1) & 1) & (~xpos & 1)) ^ 1) * RAS);

    return luma;
}

void main() {
    vec4 color = texture(Source, vTexCoord);
    vec3 lumaChroma;
    float flags = color.w;

    // 10 BIT RGB input to prevent accuracy loss
    int _col = int((uint(color.b * 255.0) & 255)
    | ((uint(color.g * 255.0) & 255) << 8)
    | ((uint(color.r * 255.0) & 255) << 16)
    | ((uint(color.a * 255.0) & 255) << 24));

    color.b = (float(_col & 1023) - 256.0) / 255.0;
    color.g = (float((_col >> 10) & 1023) - 256.0) / 255.0;
    color.r = (float((_col >> 20) & 1023) - 256.0) / 255.0;

    if (bool(params.AppPAL)) {
        float _phaseError = params.phaseErrorPAL * PI / 180.0;
        float rotU = cos(_phaseError);
        float rotV = sin(_phaseError);
        int oddLine = ((int(params.AppCropTop) >> int(params.AppInterlace)) & 1) ^ 1;

        lumaChroma = vec3(color.rgb * mat3(0.299,0.587,0.114,-0.147407,-0.289391,0.436798,0.614777,-0.514799,-0.099978));
        vec3 yuvEven = vec3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV, lumaChroma.z * rotU + lumaChroma.y * rotV);
        vec3 yuvOdd = vec3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV * -1, lumaChroma.z * rotU + lumaChroma.y * rotV * -1);

        int oddLineFrame = int(floor(mod(floor(vTexCoord.y * params.OutputSize.y / float( 1 << int(params.AppInterlace) )), 2.0)));
        color = vec4( mix(yuvOdd, yuvEven, (oddLineFrame ^ int(oddLine)) ), 1.0 );
    } else {
        float _phaseError = params.phaseErrorNTSC * PI / 180.0;
        float rotU = cos(_phaseError);
        float rotV = sin(_phaseError);

        lumaChroma = vec3(color.rgb * mat3(0.23485876230514607, 0.6335007388077467, 0.13164049888710716, 0.4409594767911895, -0.27984362502847304, -0.16111585176271648, 0.14630060102591497, -0.5594814826856017, 0.4131808816596867));
        vec3 yuvEven = vec3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV, lumaChroma.z * rotU + lumaChroma.y * rotV);
        color = vec4(yuvEven, 1.0);
    }

    if (bool(params.vic_glitches))
        color.x = vicGlitches(color.x, int( flags * 255.0 ) >> 6);

    FragColor = color;
}
