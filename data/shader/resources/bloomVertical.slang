
uniform sampler SourceS;
uniform Texture2D <float4> Source;
uniform sampler BloomHorizontalS;
uniform Texture2D <float4> BloomHorizontal;
uniform sampler lutBloomS;
uniform Texture2D lutBloom;

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

cbuffer Scene {
    float4 OutputSize : packoffset(c0);
};

cbuffer Push {
    float bloom_weight;
    float bloom_glow;
    float bloom_radius;
    float bloom_variance;
};

#pragma parameter bloom_glow       "Bloom Glow"      0 0 200 1
#pragma parameter bloom_weight     "Bloom Weight"    2.55 0 3.0 0.01
#pragma parameter bloom_radius     "Bloom Radius"    4 1 6 1
#pragma parameter bloom_variance   "Bloom Variance"  2.7 1.0 12.0 0.1

#define lutBloomTexWidth 1553.0
#define lutBloomTexHeight 6.0

float bloomTex( float pos, float posY ) {
    float4 packed = lutBloom.Sample(lutBloomS, float2(pos / lutBloomTexWidth, posY / lutBloomTexHeight) ).rgba;

    int unpacked = (uint(packed.r * 255.0) & 0xff)
                   | ((uint(packed.g * 255.0) & 0xff) << 8)
                   | ((uint(packed.b * 255.0) & 0xff) << 16)
                   | ((uint(packed.a * 255.0) & 0xff) << 24);

    return (float)unpacked / 1000000.0f;
}

PSInput VS(VSInput input) {
    PSInput output;
    output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
    output.tex = input.tex;
    return output;
}

float4 PS(PSInput input) : SV_TARGET {
    if (bloom_glow == 0) {
        return float4(Source.Sample(SourceS, input.tex).rgb, 1.0);
    }

    float3 sum = float3(0.0, 0.0, 0.0);
    float _weight = 3.0 - bloom_weight;

    int n = int(bloom_radius) << 1;
    int offset = int((bloom_variance - 1.0) * 10.0) * 14;
    float offsetY = int(bloom_radius) - 1;

    do {
        sum += (Source.Sample(SourceS, input.tex + float2(0.0, -1.0 * float(n) / OutputSize.y) ).rgb
            + Source.Sample(SourceS, input.tex + float2(0.0, float(n) / OutputSize.y) ).rgb) * bloomTex(offset + n, offsetY);

        n -= 1;
    } while(n > 0);

    sum += Source.Sample(SourceS, input.tex).rgb * bloomTex(offset + n, offsetY);

    return float4(clamp(BloomHorizontal.Sample(BloomHorizontalS, input.tex).rgb + ( pow(sum.rgb, float3(_weight, _weight, _weight)) * (bloom_glow / 100.0) ), 0.0, 1.0), 1.0);
}