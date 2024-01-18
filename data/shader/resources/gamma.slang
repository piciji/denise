
uniform sampler SourceS;
uniform Texture2D <float4> Source;

struct VSInput {
    float2 pos : TEXCOORD0;
    float2 tex : TEXCOORD1;
};

struct PSInput {
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
};

struct UBO {
    float4x4 projection;
};
uniform UBO ubo;

cbuffer Scene {
    float4 OutputSize : packoffset(c0);
};

cbuffer Push {
    float autoEmu_pal;
    float autoEmu_lace;
    float gamma;
    float autoEmu_tvGamma;
};

#pragma parameter gamma            "Gamma"              100 30 280 1
#pragma parameter autoEmu_tvGamma  "TV Gamma"           1 0 1 1
#pragma parameter autoEmu_pal      "PAL"                1 0 1 1
#pragma parameter autoEmu_lace     "Interlace"          0 0 1 1

PSInput VS(VSInput input) {
    PSInput output;
    output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
    output.tex = input.tex;
    return output;
}

float4 PS(PSInput input) : SV_TARGET {
    float3 color = Source.Sample(SourceS, input.tex).rgb;

    float _gamma = gamma / 100.0;
    if (autoEmu_tvGamma && autoEmu_pal)
        _gamma *= 2.8 / 2.2;

    color = clamp(color, 0.0, 1.0);
    color = pow(color, _gamma);

    return float4( color, 1.0 );
}