#version 450

/*
   Author: PiCiJi
   License: Public domain
*/

layout(push_constant) uniform Push
{
    vec4 SourceSize;

    float lumaFall;
    float lumaRise;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

#pragma parameter lumaLatency_head  "[LUMA LATENCY]"    0 0 0.001 0.001
#pragma parameter lumaRise          "Luma Rise"         2.0 1.0 4.0 0.1
#pragma parameter lumaFall          "Luma Fall"         1.2 1.0 4.0 0.1

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
    vec2 xy = vTexCoord.xy;
    float ySrc = texture(Source, xy + vec2( -3.0 / float(params.SourceSize.x), 0.0)).x;
    float y = ySrc;
    float yTarget;
    float yDiff = 0.0;
    int yDirection;
    int yChanged;

    float _lumaRise = params.lumaRise;
    float _lumaFall = params.lumaFall;

    if (_lumaRise > 0.0) _lumaRise = 1.0 / _lumaRise;
    if (_lumaFall > 0.0) _lumaFall = 1.0 / _lumaFall;

    yTarget = texture(Source, xy + vec2( -2.0 / float(params.SourceSize.x), 0.0)).x;
    // check for a change of luma between 2 adjacent pixel
    yChanged = ySrc == yTarget ? 0 : 1;
    ySrc = yTarget;
    yDiff = yChanged == 1 ? (yTarget - y) : yDiff;
    yDirection = int( sign( yTarget - y ) );

    // a direction of 0 means no change, a direction of 1 means 'rise' only
    // a direction of -1 means 'fall' only
    y = yDirection == 1 ? min(y + (yDiff * _lumaRise), yTarget) :
        (yDirection == -1 ? max(y + (yDiff * _lumaFall), yTarget) : y);

    yTarget = texture(Source, xy + vec2( -1.0 / float(params.SourceSize.x), 0.0)).x;
    // check for a change of luma between 2 adjacent pixel
    yChanged = ySrc == yTarget ? 0 : 1;
    ySrc = yTarget;
    yDiff = yChanged == 1 ? (yTarget - y) : yDiff;
    yDirection = int( sign( yTarget - y ) );

    // a direction of 0 means no change, a direction of 1 means 'rise' only
    // a direction of -1 means 'fall' only
    y = yDirection == 1 ? min(y + (yDiff * _lumaRise), yTarget) :
        (yDirection == -1 ? max(y + (yDiff * _lumaFall), yTarget) : y);

    yTarget = texture(Source, xy).x;
    // check for a change of luma between 2 adjacent pixel
    yChanged = ySrc == yTarget ? 0 : 1;
    ySrc = yTarget;
    yDiff = yChanged == 1 ? (yTarget - y) : yDiff;
    yDirection = int( sign( yTarget - y ) );

    // a direction of 0 means no change, a direction of 1 means 'rise' only
    // a direction of -1 means 'fall' only
    y = yDirection == 1 ? min(y + (yDiff * _lumaRise), yTarget) :
        (yDirection == -1 ? max(y + (yDiff * _lumaFall), yTarget) : y);

    FragColor = vec4(y, color.yzw);
}
