#version 450

/*
   Author: PiCiJi
   License: Public domain
*/

layout(push_constant) uniform Push
{
    vec4 OriginalSize;

    float mask_level;
    float mask_layer;
    float mask_dpi;
    float mask_pitch;
    float mask_pitch_hires;
    float luminance;
    float light_center;
    float light_distance;
    float autoEmu_hires;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

#pragma parameter mask_head         "[MASK]"                                                0 0 0.001 0.001
#pragma parameter mask_layer        "Mask Layer 0 (Aperture) 1 (Shadow) 2 (Slot) 3 (Off)"   0 0 3 1
#pragma parameter mask_level        "Mask Level"                                            45 0 100 1
#pragma parameter mask_dpi          "Mask DPI"                                              124 1 200 1
#pragma parameter autoEmu_hires     "Hires"                                                 0 0 1 1
#pragma parameter mask_pitch        "Mask Pitch"                                            0.31 0.01 1.0 0.01
#pragma parameter mask_pitch_hires  "Mask Pitch Hires"                                      0.41 0.01 1.0 0.01
#pragma parameter luminance         "Luminance"                                             100 5 500 5
#pragma parameter light_center      "Light From Center"                                     1 0 1 1
#pragma parameter light_distance    "Light Distance"                                        150 2 300 2


#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main() {
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D lutMaskAperture;
layout(set = 0, binding = 4) uniform sampler2D lutMaskShadow;
layout(set = 0, binding = 5) uniform sampler2D lutMaskSlot;


void main() {
    vec3 color = texture(Source, vTexCoord).xyz;
    float sizeX = int(params.OriginalSize.x) >> int(params.autoEmu_hires);
    float pitch = params.mask_pitch;
    if (bool(params.autoEmu_hires)) {
        pitch = params.mask_pitch_hires;
    }

    float scaleX = (sizeX * (25.4 / params.mask_dpi)) / pitch;
    float scaleY = (sizeX * scaleX) / params.OriginalSize.y;
    int layer = int(params.mask_layer);
    float _luminance = params.luminance;

    if (layer == 0) {
        color *= mix( vec3(1.0, 1.0, 1.0), texture( lutMaskAperture, vTexCoord * vec2(scaleX, scaleY) ).xyz, params.mask_level / 100.0 );
        if (bool(params.mask_level)) _luminance += 55.0;
    } else if (layer == 1) {
        color *= mix( vec3(1.0, 1.0, 1.0), texture( lutMaskShadow, vTexCoord * vec2(scaleX, scaleY) ).xyz, params.mask_level / 100.0 );
        if (bool(params.mask_level)) _luminance += 60.0;
    } else if (layer == 2) {
        color *= mix( vec3(1.0, 1.0, 1.0), texture( lutMaskSlot, vTexCoord * vec2(scaleX, scaleY) ).xyz, params.mask_level / 100.0 );
        if (bool(params.mask_level)) _luminance += 60.0;
    }

    if (bool(params.light_center)) {
        vec2 lightVector = (vTexCoord - vec2(0.5, 0.5)) * (100.0 / params.light_distance);
        color *= exp(-dot(lightVector, lightVector));
    }

    color *= _luminance / 100.0;

    FragColor = vec4( color, 1.0 );
}