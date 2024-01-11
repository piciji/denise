
uniform sampler SourceS;
uniform Texture2D <float4> Source;

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
    float4 SourceSize : packoffset(c0);
};

cbuffer Push {
    float radial_distortion;
};

#pragma parameter radial_distortion     "Radial Distortion"     20 0 100 1


float2 radialDistortion( float2 xy, float intensity ) {
    float2 center = xy - float2(0.5, 0.5);
    float dist = dot(center,center) * intensity;
    return xy + (center * (1.0 + dist) * dist);
}

PSInput VS(VSInput input) {
    PSInput output;
    output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
    output.tex = input.tex;
    return output;
}

float4 PS(PSInput input) : SV_TARGET {
    float intensity = radial_distortion / 100.0;
    float offset = 4.0;
    float width = (float)(SourceSize.x);
    float xL = offset / width;
    float xR = (width - offset) / width;

    float Scale = 1.0f / sqrt( distance( radialDistortion( float2(xL * intensity, 0.0), intensity ), radialDistortion( float2(1.0 - intensity + xR * intensity, 0.0), intensity ) ) );

    float2 xy = ((radialDistortion(input.tex.xy, intensity) - float2(0.5, 0.5)) * Scale) + float2(0.5, 0.5);

    return Source.Sample(SourceS, xy);
}