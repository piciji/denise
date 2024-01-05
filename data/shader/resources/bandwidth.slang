
uniform sampler t0S;
uniform Texture2D <float4> t0;
uniform Texture2D <float> bandwidth;
uniform sampler bandwidthS;

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
    float autoEmu_subRegion;
    float firTaps;
};

#pragma parameter autoEmu_subRegion     "Sub Region"    0 0 3 1
#pragma parameter firTaps               "Bandwidth"    9 0 21 1

PSInput VS(VSInput input) {
    PSInput output;
    output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
    output.tex = input.tex;
    return output;
}

float4 PS(PSInput input) : SV_TARGET {
    int offset = int(autoEmu_subRegion) * 22 * 75 + int(firTaps) * 75;
    int rounds = bandwidth.Sample(bandwidthS, offset + 24).x
    float3 yuv = t0.Sample(t0S, input.tex).xyz * float3(bandwidth.Sample(bandwidthS, offset).x,bandwidth.Sample(bandwidthS, offset + 25).x,bandwidth.Sample(bandwidthS, offset + 50).x);

    int n = 1;
    do {
        yuv += (t0.Sample(t0S, input.tex + float2(-1.0 * float(n)) / OutputSize.x, 0.0) ).xyz + t0.Sample(t0S, input.tex + float2(float(n) / OutputSize.x, 0.0) ).xyz) *
            float3(bandwidth.Sample(bandwidthS, offset + n).x,bandwidth.Sample(bandwidthS, offset + n + 25).x,bandwidth.Sample(bandwidthS, offset + n + 50).x);

        n += 1;
    } while(n <= rounds);

    return float4( yuv, 1.0 );
}