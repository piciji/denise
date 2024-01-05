
uniform sampler t0S;
uniform Texture2D <float4> t0;

struct UBO {
    float4x4 projection;
};
uniform UBO ubo;

#define mod(x, y) (x - y * floor(x / y))

struct VSInput {
    float2 pos : TEXCOORD0;
    float2 tex : TEXCOORD1;
};

struct PSInput {
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
};

cbuffer Scene : register(b0) {
    float ts packoffset(c0);
    float4 SourceSize packoffset(c1);
    float4 OutputSize packoffset(c2);
    float4 OriginalSize packoffset(c3);
};

cbuffer Push {
    float hanoverBars;
    float hanoverBarsAlt;
    float noEncoding;
    float autoEmu_cropTop;
    float autoEmu_lace;
    float autoEmu_pal;
};

#pragma parameter hanoverBars      "Hanover Bars"       10 -100 100 1
#pragma parameter noEncoding       "Disable Encoding"   0 0 1 1
#pragma parameter autoEmu_cropTop  "Crop Top"           0 0 100 1
#pragma parameter autoEmu_pal      "PAL"                1 0 1 1
#pragma parameter autoEmu_lace     "Interlace"          0 0 1 1

PSInput VS(VSInput input) {
    PSInput output;
    output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
    output.tex = input.tex;
    return output;
}

float4 PS(PSInput input) : SV_TARGET {
    if (autoEmu_pal) {
        if (noEncoding) {
            float3 color = mul(float3x3(1.0,0.0,1.140251,1.0,-0.39393070,-0.58080921,1.0,2.0283976,0.0), t0.Sample(t0S, input.tex.xy).xyz);
        } else {
            int oddLine = (int(autoEmu_cropTop) >> int(autoEmu_lace)) & 1;
            int lineFactor = int(floor(mod(floor(input.tex.y * OutputSize.y / float( 1 << int(autoEmu_lace) )), 2.0)));
            float _hanoverBars = (100.0 + -1.0 * abs(hanoverBars)) / 100.0;
            float _hanoverBarsAlt = (100.0 + hanoverBars) / 100.0;

            float3 yuv = (t0.Sample(t0S, input.tex.xy).xyz);
            float3 yuvLineBefore = (t0.Sample(t0S, input.tex.xy + float2(0.0, -1.0 * float( 1 << int(autoEmu_lace) ) / OutputSize.y )).xyz);

            float2 merged = float2(yuv.y + yuvLineBefore.y, yuv.z + yuvLineBefore.z) * lerp(_hanoverBars, _hanoverBarsAlt, lineFactor ^ int(oddLine)) * 0.5;
            float3 color = mul(float3x3(1.0,0.0,1.140251,1.0,-0.39393070,-0.58080921,1.0,2.0283976,0.0), float3(yuv.x, merged.x, merged.y));
        }
    } else {
        float3 color = mul(float3x3(1.0, 1.630, 0.317, 1.0, -0.378, -0.466, 1.0, -1.089, 1.677), t0.Sample(t0S, input.tex.xy).xyz);
    }

    return float4(color, 1.0);
}