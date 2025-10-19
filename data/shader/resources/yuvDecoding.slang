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
    float autoEmu_palGamma;
    float autoEmu_lace;
    float autoEmu_pal;
    float autoEmu_subRegion;
    float lumaFall;
    float lumaRise;
    float chromaSubsampling;
    float firTaps;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

#pragma parameter desaturation_head     "[PAL de-saturation]" 0 0 0.001 0.001
#pragma parameter hanoverBars           "Hanover Bars (PAL only)" 0 -50 50 1
#pragma parameter gamma                 "Gamma"               100 30 280 1
#pragma parameter Luma_delay_head       "[Luminance delay]"   0 0 0.001 0.001
#pragma parameter lumaRise              "Luma Rise"           2.0 1.0 4.0 0.1
#pragma parameter lumaFall              "Luma Fall"           1.2 1.0 4.0 0.1
#pragma parameter chromaSubsampling     "Chroma Subsampling"  0 0 1 1
#pragma parameter firTaps               "Chroma Bandwidth"    5 1 21 2
#pragma parameter autoEmu_cropTop       "Crop Top"            1 0 100 1
#pragma parameter autoEmu_palGamma      "PAL Gamma"           0 0 1 1
#pragma parameter autoEmu_pal           "PAL"                 1 0 1 1
#pragma parameter autoEmu_lace          "Interlace"           0 0 1 1
#pragma parameter autoEmu_subRegion     "Sub Region"          0 0 3 1

#define lutBandwidthTexWidth 74.0
#define lutBandwidthTexHeight 88.0
#define lutBandwidthTexRoundsStorageOffset 24.0

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
layout(set = 0, binding = 3) uniform sampler2D lutChromaBandwidth;

float lumaLatency() {
    vec2 xy = vTexCoord.xy;
    float ySrc = texture(Source, xy + vec2( -3.0 / float(params.SourceSize.x), 0.0)).x;

    float y = ySrc;
    float yTarget;
    float yDiff = 0.0;
    int yDirection;
    int yChanged;

    float _lumaRise = 1.0 / params.lumaRise;
    float _lumaFall = 1.0 / params.lumaFall;

    yTarget = texture(Source, xy + vec2( -2.0 / float(params.SourceSize.x), 0.0)).x;

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

    yTarget = texture(Source, xy + vec2( -1.0 / float(params.SourceSize.x), 0.0)).x;

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

float bandwidth( float pos, float posY ) {
    vec4 packed = texture(lutChromaBandwidth, vec2(pos / lutBandwidthTexWidth, posY / lutBandwidthTexHeight) ).rgba;

    int unpacked = int((uint(packed.r * 255.0) & 0xff)
        | ((uint(packed.g * 255.0) & 0xff) << 8)
        | ((uint(packed.b * 255.0) & 0xff) << 16)
        | ((uint(packed.a * 255.0) & 0xff) << 24));

    return float(unpacked) / 1000000.0f;
}

vec2 calcBandwidth( vec2 chroma, float lineOffset ) {

    int n = 1;
    float offsetY = int(params.autoEmu_subRegion) * 22 + int(params.firTaps);

    int rounds = int(texture(lutChromaBandwidth, vec2( lutBandwidthTexRoundsStorageOffset / lutBandwidthTexWidth, offsetY / lutBandwidthTexHeight) ).r * 255.0);

    vec2 uv = chroma * vec2( bandwidth(25, offsetY), bandwidth(50, offsetY) );

    do {
        uv += (texture(Source, vTexCoord + vec2((-1.0 * float(n)) / params.OutputSize.x, lineOffset) ).yz + texture(Source, vTexCoord + vec2(float(n) / params.OutputSize.x, lineOffset) ).yz) *
            vec2(
                bandwidth(25 + n, offsetY),
                bandwidth(50 + n, offsetY)
            );

        n += 1;
    } while(n <= rounds);

    return uv;
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
        } else {
            yOffset *= -1.0;
        }

        yuvLineBefore = texture(Source, vTexCoord.xy + vec2(0.0, yOffset) ).xyz;

        if (bool(params.chromaSubsampling)) {
            yuvLineBefore.yz = calcBandwidth(yuvLineBefore.yz, yOffset);
            yuv.yz = calcBandwidth(yuv.yz, 0.0);
        }

        vec2 merged = vec2(yuv.y + yuvLineBefore.y, yuv.z + yuvLineBefore.z) * mix(_hanoverBars, _hanoverBarsAlt, lineFactor ^ int(oddLine)) * 0.5;
        color = vec3(yuv.x, merged.x, merged.y) * mat3(1.0,0.0,1.140251,1.0,-0.39393070,-0.58080921,1.0,2.0283976,0.0);

        if (bool(params.autoEmu_palGamma) )
            _gamma *= 2.8 / 2.2;

    } else {
        yuv.yz = calcBandwidth(yuv.yz, 0.0);
        color = yuv.xyz * mat3(1.0, 1.630, 0.317, 1.0, -0.378, -0.466, 1.0, -1.089, 1.677);
    }

    color = clamp(color, 0.0, 1.0);
    color = pow(color, vec3(_gamma));

    FragColor = vec4(color, 1.0);
}
