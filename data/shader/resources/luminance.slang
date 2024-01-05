
uniform sampler t0S;
uniform Texture2D <float4> t0;

struct UBO {
    float4x4 projection;
};
uniform UBO ubo;

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
    float luminance;
    float luminance_center;
};

#pragma parameter luminance             "Luminance"                 100 1 500 1
#pragma parameter luminance_center      "Light From Center"         100 1 300 1

PSInput VS(VSInput input) {
    PSInput output;
    output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
    output.tex = input.tex;
    return output;
}

float4 PS(PSInput input) : SV_TARGET {
    float3 color = t0.Sample(t0S, input.tex).xyz;
    float2 lightVector = (input.tex - float2(0.5, 0.5)) * (100.0 / luminance_center);
    color *= exp(-dot(lightVector, lightVector)) * (100.0 / luminance);

    return float4( color, 1.0 );
}