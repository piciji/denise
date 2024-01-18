
uniform sampler SourceS;
uniform Texture2D <float4> Source;
uniform Texture2D lutMaskAperture;
uniform sampler lutMaskApertureS;
uniform Texture2D lutMaskShadow;
uniform sampler lutMaskShadowS;
uniform Texture2D lutMaskSlot;
uniform sampler lutMaskSlotS;

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
    float4 OriginalSize : packoffset(c0);
};

cbuffer Push {
    float mask_level;
    float mask_layer;
    float mask_dpi;
    float mask_pitch;
    float luminance;
    float light_center;
    float light_distance;
};

#pragma parameter mask_head         "[MASK]"                                                0 0 0.001 0.001
#pragma parameter mask_layer        "Mask Layer 0 (Aperture) 1 (Shadow) 2 (Slot) 3 (Off)"   0 0 3 1
#pragma parameter mask_level        "Mask Level"                                            45 0 100 1
#pragma parameter mask_dpi          "Mask DPI"                                              124 1 200 1
#pragma parameter mask_pitch        "Mask Pitch"                                            0.28 0.01 1.0 0.01
#pragma parameter luminance         "Luminance"                                             100 1 500 1
#pragma parameter light_center      "Light From Center"                                     0 0 1 1
#pragma parameter light_distance    "Light Distance"                                        100 1 300 1

PSInput VS(VSInput input) {
    PSInput output;
    output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
    output.tex = input.tex;
    return output;
}

float4 PS(PSInput input) : SV_TARGET {
    float3 color = Source.Sample(SourceS, input.tex).xyz;

    float scaleX = (OriginalSize.x * (25.4 / mask_dpi)) / mask_pitch;
    float scaleY = (OriginalSize.x * scaleX) / OriginalSize.y;
    int layer = int(mask_layer);
    float _luminance = luminance;

    if (layer == 0) {
        color *= lerp( float3(1.0, 1.0, 1.0), lutMaskAperture.Sample( lutMaskApertureS, input.tex * float2(scaleX, scaleY) ).xyz, mask_level / 100.0 );
        if (mask_level) _luminance += 55.0;
    } else if (layer == 1) {
        color *= lerp( float3(1.0, 1.0, 1.0), lutMaskShadow.Sample( lutMaskShadowS, input.tex * float2(scaleX, scaleY) ).xyz, mask_level / 100.0 );
        if (mask_level) _luminance += 60.0;
    } else if (layer == 2) {
        color *= lerp( float3(1.0, 1.0, 1.0), lutMaskSlot.Sample( lutMaskSlotS, input.tex * float2(scaleX, scaleY) ).xyz, mask_level / 100.0 );
        if (mask_level) _luminance += 60.0;
    }

    if (light_center) {
        float2 lightVector = (input.tex - float2(0.5, 0.5)) * (100.0 / light_distance);
        color *= exp(-dot(lightVector, lightVector));
    }

    color *= _luminance / 100.0;

    return float4( color, 1.0 );
}