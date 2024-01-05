
uniform sampler t0S;
uniform Texture2D <float4> t0;
#define mod(x, y) (x - y * floor(x / y))

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

cbuffer Scene : register(b0) {
    float ts packoffset(c0);
    float4 SourceSize packoffset(c1);
    float4 OutputSize packoffset(c2);
    float4 OriginalSize packoffset(c3);
};

cbuffer Push {
    float autoEmu_pal;
    float autoEmu_lace;
    float scanlineShade;
    float gamma;
    float tvGamma;
};

#pragma parameter scanline_head    "[Scanlines]"        0 0 0.001 0.001
#pragma parameter scanlineShade    "Scanline Shade"     0 0 100 1
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
    float3 color = t0.Sample(t0S, input.tex).rgb;
    float3 colorUp = t0.Sample(t0S, input.tex.xy + float2( 0.0, -1.0 / targetSize.y ) ).rgb;
    float3 colorDown = t0.Sample(t0S, input.tex.xy + float2( 0.0, 1.0 / targetSize.y ) ).rgb;
    float shade = 1.0 - (scanlineShade / 100.0);
    float _gamma = gamma / 100.0;
    if (autoEmu_tvGamma && autoEmu_pal)
        _gamma *= 2.8 / 2.2;

    int lineFactor = 0;
    if ((int(autoEmu_lace) == 0) && (scanlineShade > 0.0))
        lineFactor = int(floor(mod(input.tex.y * OutputSize.y, 2.0)));

    color.r = lerp( pow(color.r, _gamma), pow((colorUp.r + colorDown.r) / 2.0, _gamma) * shade, lineFactor );
    color.g = lerp( pow(color.g, _gamma), pow((colorUp.g + colorDown.g) / 2.0, _gamma) * shade, lineFactor );
    color.b = lerp( pow(color.b, _gamma), pow((colorUp.b + colorDown.b) / 2.0, _gamma) * shade, lineFactor );

    return float4( color, 1.0 );
}