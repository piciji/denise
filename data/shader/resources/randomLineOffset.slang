
uniform sampler t0S;
uniform Texture2D <float4> t0;

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
    float lineFactor;
};

#pragma parameter randomLineOffset      "Random Line Offset"    0 0 10.0 0.01

float random( float2 seed ) {
    int n = int((seed.x * 40.0) + (seed.y * 6400.0));
    n = (n<<13) ^ n;
    return 1.0 - float((n * (((n * n) * 15731) + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0;
}

PSInput VS(VSInput input) {
    PSInput output;
    output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
    output.tex = input.tex;
    return output;
}

float4 PS(PSInput input) : SV_TARGET {
    float time = ts / 1000000.0;
    float lineFactor = pow(randomLineOffset, 2.0) / 10000.0;
    float2 xy = input.tex.xy;
    float offset = random(float2(time * xy.y, xy.y + (time * xy.y))) * lineFactor;
    float x0 = xy.x + offset;
    float x1 = x0 + targetSize.z;
    float4 tex0 = t0.Sample(t0S, float2( frac(x0), xy.y )).xyzw;
    float4 tex1 = t0.Sample(t0S, float2( frac(x1), xy.y )).xyzw;
    return lerp(tex0, tex1, frac(x0 * OutputSize.x));
}