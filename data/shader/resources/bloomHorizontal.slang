
uniform sampler t0S;
uniform Texture2D <float4> t0;
uniform sampler t1S;
uniform Texture2D <float4> t1;
uniform sampler bloomS;
uniform Texture2D <float> bloom;

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
    float bloom_radius;
    float bloom_variance;
};

#pragma parameter bloom_head       "[BLOOM]"            0 0 0.001 0.001
#pragma parameter bloom_radius     "Bloom Radius"       4 1 6 1
#pragma parameter bloom_variance   "Bloom Variance"     2.7 1.0 12.0 0.1

PSInput VS(VSInput input) {
    PSInput output;
    output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
    output.tex = input.tex;
    return output;
}

float4 PS(PSInput input) : SV_TARGET {

    float3 sum = float3(0.0, 0.0, 0.0);

    int n = bloom_radius << 1;
    //int offset = (int(bloom_radius) - 1) * (111 * 13) + int(bloom_variance - 1.0 * 10.0) * 13;
    int offset = (111 * 13) + int(bloom_variance - 1.0 * 10.0) * 13;
    int offsetY = int(bloom_radius) - 1;

    do {
        sum += (t0.Sample(t0S, input.tex + float2(-1.0 * float(n) / OutputSize.x, 0.0) ).rgb
            + t0.Sample(t0S, input.tex + float2(float(n) / OutputSize.x, 0.0) ).rgb) * bloom.Sample(bloomS, float2(offset + n, offsetY) );

        n -= 1;
    } while(n > 0);

    sum += t0.Sample(t0S, input.tex).rgb * bloom.Sample(bloomS, float2(offset + n, offsetY) );

    return float4( sum.rgb, 1.0 );
}