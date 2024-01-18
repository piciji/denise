
uniform sampler SourceS;
uniform Texture2D <float4> Source;
uniform Texture2D lutBandwidth;
uniform sampler lutBandwidthS;

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

#pragma parameter autoEmu_subRegion     "Sub Region"   0 0 3 1
#pragma parameter firTaps               "Bandwidth"    9 1 21 2

#define lutBandwidthTexWidth 74.0
#define lutBandwidthTexHeight 88.0
#define lutBandwidthTexRoundsStorageOffset 24.0

float bandwidth( float pos, float posY ) {
    float4 packed = lutBandwidth.Sample(lutBandwidthS, float2(pos / lutBandwidthTexWidth, posY / lutBandwidthTexHeight) ).rgba;

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
    int n = 1;
    float offsetY = int(autoEmu_subRegion) * 22 + int(firTaps);

    int rounds = lutBandwidth.Sample(lutBandwidthS, float2( lutBandwidthTexRoundsStorageOffset / lutBandwidthTexWidth, offsetY / lutBandwidthTexHeight) ).r * 255.0;

    float3 yuv = Source.Sample(SourceS, input.tex).xyz * float3(
        bandwidth(0, offsetY),
        bandwidth(25, offsetY),
        bandwidth(50, offsetY)
    );

    do {
        yuv += (Source.Sample(SourceS, input.tex + float2((-1.0 * float(n)) / OutputSize.x, 0.0) ).xyz + Source.Sample(SourceS, input.tex + float2(float(n) / OutputSize.x, 0.0) ).xyz) *
            float3(
                bandwidth(0 + n, offsetY),
                bandwidth(25 + n, offsetY),
                bandwidth(50 + n, offsetY)
            );

        n += 1;
    } while(n <= rounds);

    return float4( yuv, 1.0 );
}