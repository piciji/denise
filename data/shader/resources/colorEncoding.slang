
uniform sampler t0S;
uniform Texture2D <float4> t0;
uniform sampler phaseShiftS;
uniform Texture2D <float> phaseShift;

#define mod(x, y) (x - y * floor(x / y))

struct UBO {
    float4x4 projection;
};
uniform UBO ubo;

cbuffer Scene : register(b0) {
    float ts packoffset(c0);
    float4 SourceSize packoffset(c1);
    float4 OutputSize packoffset(c2);
    float4 OriginalSize packoffset(c3);
};

cbuffer Push {
    float noEncoding;
    float phaseError;
    float autoEmu_cropTop;
    float autoEmu_lace;
    float autoEmu_pal;
};

struct VSInput {
    float2 pos : TEXCOORD0;
    float2 tex : TEXCOORD1;
};

struct PSInput {
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
};

#pragma parameter Encoding_head    "[YUV/YIC Encoding]" 0 0 0.001 0.001
#pragma parameter noEncoding       "Disable Encoding"   0 0 1 1
#pragma parameter phaseError       "Phase Error"        0 -45 45 0.5
#pragma parameter autoEmu_cropTop  "Crop Top"           0 0 100 1
#pragma parameter autoEmu_pal      "PAL"                1 0 1 1
#pragma parameter autoEmu_lace     "Interlace"          0 0 1 1

// lace : e, e, o, o, e, e, o, o, ...
// non lace: e, o, e, o, e, o, ...

PSInput VS(VSInput input) {
    PSInput output;
    output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
    output.tex = input.tex;
    return output;
}

float4 PS(PSInput input) : SV_TARGET {
    float4 color = t0.Sample(t0S, input.tex);
    float3 lumaChroma;
    int flags = int(256.0 * color.w);

    if (flags & 0x80) {
        // RGB input
        if (autoEmu_pal) {
            lumaChroma = mul( float3x3(0.299,0.587,0.114,-0.147407,-0.289391,0.436798,0.614777,-0.514799,-0.099978), color.rgb);
        } else {
            float3 lumaChroma = mul( float3x3(0.23485876230514607, 0.6335007388077467, 0.13164049888710716, 0.4409594767911895, -0.27984362502847304, -0.16111585176271648, 0.14630060102591497, -0.5594814826856017, 0.4131808816596867), color.rgb);
        }
    } else {
        // YUV / YIC input
        lumaChroma = color.xyz;
        flags = color.w;
    }

    if (noEncoding) {
        color = float4(lumaChroma.xyz, flags);
    } else {
        int offset = (phaseError + 45.0) * 2.0;
        float rotU = phaseShift.Sample(phaseShiftS, offset);
        float rotV = phaseShift.Sample(phaseShiftS, offset + 181);
        int oddLine = (int(autoEmu_cropTop) >> int(autoEmu_lace)) & 1;

        float3 yuvEven = float3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV, lumaChroma.z * rotU + lumaChroma.y * rotV);
        float3 yuvOdd = float3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV * -1, lumaChroma.z * rotU + lumaChroma.y * rotV * -1);

        if (autoEmu_pal) {
            int oddLineFrame = int(floor(mod(floor(input.tex.y * OutputSize.y / float( 1 << int(autoEmu_lace) )), 2.0)));
            color = float4( lerp(yuvOdd, yuvEven, (oddLineFrame ^ int(oddLine)) ), flags );
        } else
            color = float4(yuvEven, flags);
    }

    return color;
}