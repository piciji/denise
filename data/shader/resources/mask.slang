
uniform sampler t0S;
uniform Texture2D <float4> t0;
uniform Texture2D maskLayerAperture;
uniform sampler maskLayerApertureS;
uniform Texture2D maskLayerShadowMask;
uniform sampler maskLayerShadowMaskS;
uniform Texture2D maskLayerSlotMask;
uniform sampler maskLayerSlotMaskS;

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
    float mask_level;
    float mask_layer;
    float mask_dpi;
    float mask_pitch;
};

#pragma parameter mask_head         "[MASK]"                                        0 0 0.001 0.001
#pragma parameter mask_layer        "Mask Layer 0 (Aperture) 1 (Shadow) 2 (Slot)"   0 0 2 1
#pragma parameter mask_level        "Mask Level"                                    45 0 100 1
#pragma parameter mask_dpi          "Mask DPI"                                      124 1 200 1
#pragma parameter mask_pitch        "Mask Pitch"                                    0.28 0.01 1.0 0.01

PSInput VS(VSInput input) {
    PSInput output;
    output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
    output.tex = input.tex;
    return output;
}

float4 PS(PSInput input) : SV_TARGET {
    float3 color = t0.Sample(t0S, input.tex).xyz;

    float scaleX = (OutputSize.x * (25.4 / mask_dpi)) / mask_pitch;
    float scaleY = (OutputSize.x * scaleX) / OutputSize.y;

    if (maskLayer == 0) {
        color *= lerp( float3(1.0, 1.0, 1.0), maskLayerAperture.Sample( maskLayerApertureS, input.tex * float2(scaleX, scaleY) ).xyz, mask_level / 100.0 );
    } else if (maskLayer == 1) {
        color *= lerp( float3(1.0, 1.0, 1.0), maskLayerShadowMask.Sample( maskLayerShadowMaskS, input.tex * float2(scaleX, scaleY) ).xyz, mask_level / 100.0 );
    } else if (maskLayer == 2) {
        color *= lerp( float3(1.0, 1.0, 1.0), maskLayerSlotMask.Sample( maskLayerSlotMaskS, input.tex * float2(scaleX, scaleY) ).xyz, mask_level / 100.0 );
    }

    return float4( color, 1.0 );
}