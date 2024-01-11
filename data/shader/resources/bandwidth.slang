
uniform sampler SourceS;
uniform Texture2D <float4> Source;
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

cbuffer Scene {
    float4 OutputSize : packoffset(c0);
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
    int offset = int(firTaps) * 75;
    int offsetY = int(autoEmu_subRegion) * int(firTaps);

    int rounds = bandwidth.Sample(bandwidthS, float2(offset + 24, offsetY) ).x
    float3 yuv = t0.Sample(t0S, input.tex).xyz * float3(bandwidth.Sample(bandwidthS, float2(offset, offsetY)).x,
            bandwidth.Sample(bandwidthS, float2(offset + 25, offsetY)).x,bandwidth.Sample(bandwidthS, float2(offset + 50, offsetY)).x);

    int n = 1;
    do {
        yuv += (t0.Sample(t0S, input.tex + float2(-1.0 * float(n)) / OutputSize.x, 0.0) ).xyz + t0.Sample(t0S, input.tex + float2(float(n) / OutputSize.x, 0.0) ).xyz) *
            float3(bandwidth.Sample(bandwidthS, float2(offset + n, offsetY)).x,
            bandwidth.Sample(bandwidthS, float2(offset + n + 25, offsetY)).x,
            bandwidth.Sample(bandwidthS, float2(offset + n + 50, offsetY)).x);

        n += 1;
    } while(n <= rounds);

    return float4( yuv, 1.0 );
}