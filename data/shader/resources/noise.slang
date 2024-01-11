
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
    float Seed : packoffset(c0);
};

cbuffer Push {
    float lumaNoise;
    float chromaNoise;
};

float random( float2 seed ) {
    int n = int((seed.x * 40.0) + (seed.y * 6400.0));
    n = (n<<13) ^ n;
    return 1.0 - float((n * (((n * n) * 15731) + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0;
}

#pragma parameter lumaNoise     "Luma Noise"        1.5 0.0 100.0 0.1
#pragma parameter chromaNoise   "Chroma Noise"      1.5 0.0 100.0 0.1

PSInput VS(VSInput input) {
    PSInput output;
    output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
    output.tex = input.tex;
    return output;
}

float4 PS(PSInput input) : SV_TARGET {
    float _seed = Seed / 1000.0;
    float2 xy = input.tex.xy;
    float y = random(xy + float2(_seed * xy.x, _seed * xy.y)) * lumaNoise;
    float u = random(xy + float2(_seed * xy.y, _seed * xy.x)) * chromaNoise;
    float v = random(xy - float2(_seed * xy.x, _seed * xy.y)) * chromaNoise;
    return Source.Sample(SourceS, xy).xyzw + float4(y, u, v, 0.0);
}