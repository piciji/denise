
uniform sampler t0S;
uniform Texture2D <float4> t0;

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
    float glitch_cas;
    float glitch_ras;
    float glitch_phi;
    float glitch_aec;
    float glitch_ba;
    float autoEmu_cropLeft;
    float autoEmu_pal;
};

struct VSInput {
    float2 pos : TEXCOORD0;
    float2 tex : TEXCOORD1;
};

struct PSInput {
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
};

// luma will be darkened when some vic lines changes their state
// AEC and BA state will be transferred in unused alpha channel for each pixel
// PHI0, CAS, RAS have the same behavior within each cycle.
// PHI0: second half cycle is darkened
// Pixel: 0, 0.5, 1, 1.5, 2, 2.5, 3, 3.5 -> first half cycle
// CAS: third and fourth pixel of each half cycle are darkened
// RAS: first pixel of each half cycle will be non darkened

#pragma parameter vicGlitches_head      "[VICII Glitches]"   0 0 0.001 0.001
#pragma parameter glitch_aec            "AEC"                10.5 0 100 0.1
#pragma parameter glitch_ba             "BA"                 10.5 0 100 0.1
#pragma parameter glitch_phi            "PHI"                10.5 0 100 0.1
#pragma parameter glitch_ras            "RAS"                10.5 0 100 0.1
#pragma parameter glitch_cas            "CAS"                10.5 0 100 0.1
#pragma parameter autoEmu_cropLeft      "Crop Left"          0 0 100 1
#pragma parameter autoEmu_pal           "PAL"                1 0 1 1

PSInput VS(VSInput input) {
    PSInput output;
    output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
    output.tex = input.tex;
    return output;
}

float4 PS(PSInput input) : SV_TARGET {
    float4 color = t0.Sample(t0S, input.tex);
    float AEC = glitch_aec * glitch_aec * 0.0001;
    float BA = glitch_ba * glitch_ba * 0.0001;
    float PHI = glitch_phi * glitch_phi * 0.0001;
    float CAS = glitch_cas * glitch_cas * 0.0001;
    float RAS = glitch_ras * glitch_ras * 0.0001;

    float3 lumaChroma = color.xyz;
    float xposF = input.tex.x * SourceSize.x;
    int xpos = int(xposF);
    xpos += int(autoEmu_cropLeft) + (2 << (int(autoEmu_pal) ^ 1));
    xpos &= 7;
    int flags = int( color.w );
    lumaChroma.x *= 1.0 - ((flags & 1) * BA);
    lumaChroma.x *= 1.0 - (((flags >> 1) & 1) * AEC);
    lumaChroma.x *= 1.0 - (((xpos >> 2) & 1) * PHI0);
    lumaChroma.x *= 1.0 - (((xpos >> 1) & 1) * CAS);
    lumaChroma.x *= 1.0 - (((((~xpos >> 1) & 1) & (~xpos & 1)) ^ 1) * RAS);

    return float4(lumaChroma, 1.0);
}
