
uniform sampler t0S;
uniform Texture2D <float4> t0;

cbuffer Scene : register(b0) {
    float ts packoffset(c0);
    float4 SourceSize packoffset(c1);
    float4 OutputSize packoffset(c2);
    float4 OriginalSize packoffset(c3);
};

cbuffer Push {
    float lumaFall;
    float lumaRise;
};

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

#pragma parameter lumaLatency_head  "[LUMA LATENCY]"    0 0 0.001 0.001
#pragma parameter lumaRise          "Luma Rise"         2.0 1.0 4.0 0.1
#pragma parameter lumaFall          "Luma Fall"         1.2 1.0 4.0 0.1

PSInput VS(VSInput input) {
    PSInput output;
    output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
    output.tex = input.tex;
    return output;
}

float4 PS(PSInput input) : SV_TARGET {
    float4 color = t0.Sample(t0S, input.tex);
    float2 xy = input.tex.xy;
    float ySrc = t0.Sample(t0S, xy + float2( -3.0 / float(SourceSize.x), 0.0)).x;
    float y = ySrc;
    float yTarget;
    float yDiff = 0.0;
    int yDirection;
    int yChanged;

    float _lumaRise = lumaRise;
    float _lumaFall = lumaFall;

    if (_lumaRise > 0.0) _lumaRise = 1.0 / _lumaRise;
    if (_lumaFall > 0.0) _lumaFall = 1.0 / _lumaFall;

    yTarget = t0.Sample(t0S, xy + float2( -2.0 / float(SourceSize.x), 0.0)).x;
    // check for a change of luma between 2 adjacent pixel
    yChanged = ySrc == yTarget ? 0 : 1;
    ySrc = yTarget;
    yDiff = yChanged == 1 ? (yTarget - y) : yDiff;
    yDirection = int( sign( yTarget - y ) );

    // a direction of 0 means no change, a direction of 1 means 'rise' only
    // a direction of -1 means 'fall' only
    y = yDirection == 1 ? min(y + (yDiff * _lumaRise), yTarget) :
        (yDirection == -1 ? max(y + (yDiff * _lumaFall), yTarget) : y);

    yTarget = t0.Sample(t0S, xy + float2( -1.0 / float(SourceSize.x), 0.0)).x;
    // check for a change of luma between 2 adjacent pixel
    yChanged = ySrc == yTarget ? 0 : 1;
    ySrc = yTarget;
    yDiff = yChanged == 1 ? (yTarget - y) : yDiff;
    yDirection = int( sign( yTarget - y ) );

    // a direction of 0 means no change, a direction of 1 means 'rise' only
    // a direction of -1 means 'fall' only
    y = yDirection == 1 ? min(y + (yDiff * _lumaRise), yTarget) :
        (yDirection == -1 ? max(y + (yDiff * _lumaFall), yTarget) : y);

    yTarget = t0.Sample(t0S, xy).x;
    // check for a change of luma between 2 adjacent pixel
    yChanged = ySrc == yTarget ? 0 : 1;
    ySrc = yTarget;
    yDiff = yChanged == 1 ? (yTarget - y) : yDiff;
    yDirection = int( sign( yTarget - y ) );

    // a direction of 0 means no change, a direction of 1 means 'rise' only
    // a direction of -1 means 'fall' only
    y = yDirection == 1 ? min(y + (yDiff * _lumaRise), yTarget) :
        (yDirection == -1 ? max(y + (yDiff * _lumaFall), yTarget) : y);

    return float4(y, color.yzw);
}
