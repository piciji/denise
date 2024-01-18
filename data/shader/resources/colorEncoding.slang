
uniform sampler SourceS;
uniform Texture2D <float4> Source;

#define mod(x, y) (x - y * floor(x / y))
#define PI 3.14159265358979323846

struct UBO {
    float4x4 projection;
};
uniform UBO ubo;

cbuffer Scene {
    float4 OutputSize : packoffset(c0);
};

cbuffer Push {
    float noEncoding;
    float phaseErrorPAL;
    float phaseErrorNTSC;
    float autoEmu_cropTop;
    float autoEmu_lace;
    float autoEmu_pal;
    float autoEmu_lumaChroma;
};

struct VSInput {
    float2 pos : TEXCOORD0;
    float2 tex : TEXCOORD1;
};

struct PSInput {
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
};

#pragma parameter Encoding_head         "[YUV/YIC ENCODING]" 0 0 0.001 0.001
#pragma parameter noEncoding            "Disable Encoding"   0 0 1 1
#pragma parameter phaseErrorPAL         "Phase Error PAL"    22.5 -45 45 0.5
#pragma parameter phaseErrorNTSC        "Phase Error NTSC"   0 -45 45 0.5
#pragma parameter autoEmu_lumaChroma    "Luma Chroma Input"  0 0 1 1
#pragma parameter autoEmu_cropTop       "Crop Top"           0 0 100 1
#pragma parameter autoEmu_pal           "PAL"                1 0 1 1
#pragma parameter autoEmu_lace          "Interlace"          0 0 1 1

// lace : e, e, o, o, e, e, o, o, ...
// non lace: e, o, e, o, e, o, ...

PSInput VS(VSInput input) {
    PSInput output;
    output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
    output.tex = input.tex;
    return output;
}

float4 PS(PSInput input) : SV_TARGET {
    float4 color = Source.Sample(SourceS, input.tex);
    float3 lumaChroma;
    float flags = color.w;

    if (autoEmu_lumaChroma) {
        // YUV / YIC input
        lumaChroma = color.xyz;
    } else {
        // RGB input
        if (autoEmu_pal) {
            lumaChroma = mul( float3x3(0.299,0.587,0.114,-0.147407,-0.289391,0.436798,0.614777,-0.514799,-0.099978), color.rgb);
        } else {
            lumaChroma = mul( float3x3(0.23485876230514607, 0.6335007388077467, 0.13164049888710716, 0.4409594767911895, -0.27984362502847304, -0.16111585176271648, 0.14630060102591497, -0.5594814826856017, 0.4131808816596867), color.rgb);
        }
        flags = 0;
    }

    if (noEncoding) {
        color = float4(lumaChroma.xyz, flags);
    } else {
        if (autoEmu_pal) {
            float _phaseError = phaseErrorPAL * PI / 180.0;
            float rotU = cos(_phaseError);
            float rotV = sin(_phaseError);
            int oddLine = (int(autoEmu_cropTop) >> int(autoEmu_lace)) & 1;

            float3 yuvEven = float3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV, lumaChroma.z * rotU + lumaChroma.y * rotV);
            float3 yuvOdd = float3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV * -1, lumaChroma.z * rotU + lumaChroma.y * rotV * -1);

            int oddLineFrame = int(floor(mod(floor(input.tex.y * OutputSize.y / float( 1 << int(autoEmu_lace) )), 2.0)));
            color = float4( lerp(yuvOdd, yuvEven, (oddLineFrame ^ int(oddLine)) ), flags );
        } else {
            float _phaseError = phaseErrorNTSC * PI / 180.0;
            float rotU = cos(_phaseError);
            float rotV = sin(_phaseError);

            float3 yuvEven = float3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV, lumaChroma.z * rotU + lumaChroma.y * rotV);
            color = float4(yuvEven, flags);
        }
    }

    return color;
}