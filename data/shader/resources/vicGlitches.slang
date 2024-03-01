#version 450

/*
   Author: PiCiJi
   License: Public domain
*/

layout(push_constant) uniform Push
{
    vec4 SourceSize;

    float glitch_cas;
    float glitch_ras;
    float glitch_phi;
    float glitch_aec;
    float glitch_ba;
    float autoEmu_cropLeft;
    float autoEmu_pal;
    float autoEmu_lumaChroma;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

// luma will be darkened when some VIC lines changes their state
// AEC and BA state will be transferred in unused alpha channel for each pixel
// PHI0, CAS, RAS have the same behavior within each cycle.
// PHI0: second half cycle is darkened
// Pixel: 0, 0.5, 1, 1.5, 2, 2.5, 3, 3.5 -> first half cycle
// CAS: third and fourth pixel of each half cycle are darkened
// RAS: first pixel of each half cycle will be non darkened

#pragma parameter vicGlitches_head      "[VICII GLITCHES]"   0 0 0.001 0.001
#pragma parameter glitch_aec            "AEC"                10.5 0 50 0.1
#pragma parameter glitch_ba             "BA"                 10.5 0 50 0.1
#pragma parameter glitch_phi            "PHI"                10.5 0 50 0.1
#pragma parameter glitch_ras            "RAS"                10.5 0 50 0.1
#pragma parameter glitch_cas            "CAS"                10.5 0 50 0.1
#pragma parameter autoEmu_cropLeft      "Crop Left"          6 0 100 1
#pragma parameter autoEmu_pal           "PAL"                1 0 1 1
#pragma parameter autoEmu_lumaChroma    "Luma Chroma Input"  0 0 1 1


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


void main() {
    vec4 color = texture(Source, vTexCoord);
    float AEC = params.glitch_aec * params.glitch_aec * 0.0001;
    float BA = params.glitch_ba * params.glitch_ba * 0.0001;
    float PHI0 = params.glitch_phi * params.glitch_phi * 0.0001;
    float CAS = params.glitch_cas * params.glitch_cas * 0.0001;
    float RAS = params.glitch_ras * params.glitch_ras * 0.0001;

    vec3 lumaChroma = color.xyz;
    float xposF = vTexCoord.x * params.SourceSize.x;
    int xpos = int(xposF);
    xpos += int(params.autoEmu_cropLeft) + (2 << (int(params.autoEmu_pal) ^ 1));
    if (!(bool(params.autoEmu_lumaChroma)))
        xpos += 4; // no crop

    xpos &= 7;
    int flags = int( color.w );
    lumaChroma.x *= 1.0 - ((flags & 1) * BA);
    lumaChroma.x *= 1.0 - (((flags >> 1) & 1) * AEC);
    lumaChroma.x *= 1.0 - (((xpos >> 2) & 1) * PHI0);
    lumaChroma.x *= 1.0 - (((xpos >> 1) & 1) * CAS);
    lumaChroma.x *= 1.0 - (((((~xpos >> 1) & 1) & (~xpos & 1)) ^ 1) * RAS);

    FragColor = vec4(lumaChroma, 1.0);
}
