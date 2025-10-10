#version 450

/*
   Author: PiCiJi
   License: Public domain
*/

layout(push_constant) uniform Push
{
    vec4 OutputSize;
    vec4 SourceSize;

    float hanoverBars;
    float autoEmu_cropTop;
    float gamma;
    float autoEmu_tvGamma;
    float autoEmu_lace;
    float autoEmu_pal;
    float lumaFall;
    float lumaRise;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

#pragma parameter desaturation_head     "[PAL de-saturation]" 0 0 0.001 0.001
#pragma parameter hanoverBars           "Hanover Bars"        0 -100 100 1
#pragma parameter gamma                 "Gamma"               100 30 280 1
#pragma parameter Luma_delay_head       "[Luminance delay]"   0 0 0.001 0.001
#pragma parameter lumaRise              "Luma Rise"           2.0 1.0 2.0 0.1
#pragma parameter lumaFall              "Luma Fall"           1.2 1.0 2.0 0.1
#pragma parameter autoEmu_cropTop       "Crop Top"            1 0 100 1
#pragma parameter autoEmu_tvGamma       "TV Gamma"            0 0 1 1
#pragma parameter autoEmu_pal           "PAL"                 1 0 1 1
#pragma parameter autoEmu_lace          "Interlace"           0 0 1 1


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

float lumaLatency() {
    vec2 xy = vTexCoord.xy;
    float ySrc = texture(Source, xy + vec2( -1.0 / float(params.SourceSize.x), 0.0)).x;

    float y = ySrc;
    float yTarget;
    float yDiff = 0.0;
    int yDirection;
    int yChanged;

    float _lumaRise = 1.0 / params.lumaRise;
    float _lumaFall = 1.0 / params.lumaFall;

//     yTarget = texture(Source, xy + vec2( -1.0 / float(params.SourceSize.x), 0.0)).x;
//
//     // check for a change of luma between 2 adjacent pixel
//     yChanged = ySrc == yTarget ? 0 : 1;
//     ySrc = yTarget;
//     if (yChanged == 1)
//         yDiff = yTarget - y;
//
//     yDirection = int( sign( yTarget - y ) );
//
//     // a direction of 0 means no change, a direction of 1 means 'rise' only
//     // a direction of -1 means 'fall' only
//     y = yDirection == 1 ? min(y + (yDiff * _lumaRise), yTarget) :
//         (yDirection == -1 ? max(y + (yDiff * _lumaFall), yTarget) : y);
//
//     yTarget = texture(Source, xy + vec2( -1.0 / float(params.SourceSize.x), 0.0)).x;
//
//     // check for a change of luma between 2 adjacent pixel
//     yChanged = ySrc == yTarget ? 0 : 1;
//     ySrc = yTarget;
//     if (yChanged == 1)
//         yDiff = yTarget - y;
//     yDirection = int( sign( yTarget - y ) );
//
//     // a direction of 0 means no change, a direction of 1 means 'rise' only
//     // a direction of -1 means 'fall' only
//     y = yDirection == 1 ? min(y + (yDiff * _lumaRise), yTarget) :
//         (yDirection == -1 ? max(y + (yDiff * _lumaFall), yTarget) : y);

    yTarget = texture(Source, xy).x;
    // check for a change of luma between 2 adjacent pixel
    yChanged = ySrc == yTarget ? 0 : 1;
    ySrc = yTarget;
    if (yChanged == 1)
        yDiff = yTarget - y;

    yDirection = int( sign( yTarget - y ) );

    // a direction of 0 means no change, a direction of 1 means 'rise' only
    // a direction of -1 means 'fall' only
    y = yDirection == 1 ? min(y + (yDiff * _lumaRise), yTarget) :
        (yDirection == -1 ? max(y + (yDiff * _lumaFall), yTarget) : y);

    return y;
}

void main() {
    vec3 color;
    vec3 yuv = (texture(Source, vTexCoord.xy).xyz); // yuv/yic
    float _gamma = params.gamma / 100.0;

    if (params.lumaRise > 1.0 || params.lumaFall > 1.0)
        yuv.x = lumaLatency();

    if (bool(params.autoEmu_pal)) {
        int oddLine = ((int(params.autoEmu_cropTop) >> int(params.autoEmu_lace)) & 1) ^ 1;
        int lineFactor = int(floor(mod(floor(vTexCoord.y * params.OutputSize.y / float( 1 << int(params.autoEmu_lace) )), 2.0)));
        float _hanoverBars = (100.0 - abs(params.hanoverBars)) / 100.0;
        float _hanoverBarsAlt = 1.0;
        if (params.hanoverBars > 0)
            _hanoverBarsAlt = (100.0 + params.hanoverBars) / 100.0;

        vec3 yuvLineBefore;
        float yOffset = float( 1 << int(params.autoEmu_lace) ) / params.OutputSize.y;

        if (vTexCoord.y < yOffset) {
            // no hidden delay line
            yuvLineBefore = texture(Source, vTexCoord.xy + vec2(0.0, yOffset) ).xyz;
        } else {
            yuvLineBefore = texture(Source, vTexCoord.xy + vec2(0.0, -1.0 * yOffset) ).xyz;
        }

        vec2 merged = vec2(yuv.y + yuvLineBefore.y, yuv.z + yuvLineBefore.z) * mix(_hanoverBars, _hanoverBarsAlt, lineFactor ^ int(oddLine)) * 0.5;
        color = vec3(yuv.x, merged.x, merged.y) * mat3(1.0,0.0,1.140251,1.0,-0.39393070,-0.58080921,1.0,2.0283976,0.0);

        if (bool(params.autoEmu_tvGamma) )
            _gamma *= 2.8 / 2.2;

    } else {
        color = yuv.xyz * mat3(1.0, 1.630, 0.317, 1.0, -0.378, -0.466, 1.0, -1.089, 1.677);
    }

    color = clamp(color, 0.0, 1.0);
    color = pow(color, vec3(_gamma));

    FragColor = vec4(color, 1.0);
}
