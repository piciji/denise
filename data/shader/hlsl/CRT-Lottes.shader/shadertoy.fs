//
// PUBLIC DOMAIN CRT STYLED SCAN-LINE SHADER
//
//   by Timothy Lottes
//
// This is more along the style of a really good CGA arcade monitor.
// With RGB inputs instead of NTSC.
// The shadow mask example has the mask rotated 90 degrees for less chromatic aberration.
//
// Left it unoptimized to show the theory behind the algorithm.
//
// It is an example what I personally would want as a display option for pixel art games.
// Please take and use, change, or whatever.
//

uniform sampler t0S;
uniform Texture2D <float4> t0;

cbuffer scene {
    int ts : packoffset(c0);
    float4 targetSize : packoffset(c1);
    float4 sourceSize : packoffset(c2);
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

// Hardness of scanline.
//  -8.0 = soft
// -16.0 = medium
static float hardScan=-12.0;

// Hardness of pixels in scanline.
// -2.0 = soft
// -4.0 = hard
static float hardPix=-4.0;

// Display warp.
// 0.0 = none
// 1.0/8.0 = extreme
static float2 warp=float2(0.0/32.0,0.0/24.0);

// Amount of shadow mask.
static float maskDark=0.5;
static float maskLight=1.5;

float ToLinear1(float c) {return(c<=0.04045) ? c/12.92 :pow((c+0.055)/1.055,2.4);}
float3 ToLinear(float3 c){return float3(ToLinear1(c.r),ToLinear1(c.g),ToLinear1(c.b));}

float ToSrgb1(float c){return(c<0.0031308?c*12.92:1.055*pow(c,0.41666)-0.055);}
float3 ToSrgb(float3 c){return float3(ToSrgb1(c.r),ToSrgb1(c.g),ToSrgb1(c.b));}

float3 Fetch(float2 pos,float2 off){
    float2 res=sourceSize.xy;
    pos=(floor(pos*res+off)+float2(0.5,0.5))/res;
    return ToLinear(1.2 * t0.SampleBias(t0S,pos.xy,-16.0).rgb);
 }

float2 Dist(float2 pos){
    float2 res=sourceSize.xy;
    pos=pos*res;
    return -((pos-floor(pos))-float2(0.5, 0.5));
}
    
float Gaus(float pos,float scale){return exp2(scale*pos*pos);}

float3 Horz3(float2 pos,float off){
  float3 b=Fetch(pos,float2(-1.0,off));
  float3 c=Fetch(pos,float2( 0.0,off));
  float3 d=Fetch(pos,float2( 1.0,off));
  float dst=Dist(pos).x;
  float scale=hardPix;
  float wb=Gaus(dst-1.0,scale);
  float wc=Gaus(dst+0.0,scale);
  float wd=Gaus(dst+1.0,scale);
  return (b*wb+c*wc+d*wd)/(wb+wc+wd);}

float3 Horz5(float2 pos,float off){
  float3 a=Fetch(pos,float2(-2.0,off));
  float3 b=Fetch(pos,float2(-1.0,off));
  float3 c=Fetch(pos,float2( 0.0,off));
  float3 d=Fetch(pos,float2( 1.0,off));
  float3 e=Fetch(pos,float2( 2.0,off));
  float dst=Dist(pos).x;
  float scale=hardPix;
  float wa=Gaus(dst-2.0,scale);
  float wb=Gaus(dst-1.0,scale);
  float wc=Gaus(dst+0.0,scale);
  float wd=Gaus(dst+1.0,scale);
  float we=Gaus(dst+2.0,scale);
  return (a*wa+b*wb+c*wc+d*wd+e*we)/(wa+wb+wc+wd+we);}

float Scan(float2 pos,float off){
  float dst=Dist(pos).y;
  return Gaus(dst+off,hardScan);}

float3 Tri(float2 pos){
  float3 a=Horz3(pos,-1.0);
  float3 b=Horz5(pos, 0.0);
  float3 c=Horz3(pos, 1.0);
  float wa=Scan(pos,-1.0);
  float wb=Scan(pos, 0.0);
  float wc=Scan(pos, 1.0);
  return a*wa+b*wb+c*wc;}

float2 Warp(float2 pos){
  pos=pos*2.0-1.0;
  pos*=float2(1.0+(pos.y*pos.y)*warp.x,1.0+(pos.x*pos.x)*warp.y);
  return pos*0.5+0.5;}

float3 Mask(float2 pos){
  pos.x+=pos.y*3.0;
  float3 mask1=float3(maskDark,maskDark,maskDark);
  pos.x=frac(pos.x/6.0);
  if(pos.x<0.333)mask1.r=maskLight;
  else if(pos.x<0.666)mask1.g=maskLight;
  else mask1.b=maskLight;
  return mask1;}

PSInput VS(VSInput input) {
    PSInput output;
    output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
    output.tex = input.tex;
    return output;
}

float4 PS(PSInput input) : SV_TARGET {
    float2 pos = input.pos.xy/targetSize.xy;
//      hardScan=-12.0;
      pos=Warp(input.pos.xy/targetSize.xy);
      float4 fragColor;
    fragColor.rgb=Tri(pos)*Mask(input.pos.xy);
  fragColor.a=1.0;
  return float4(ToSrgb(fragColor.rgb), fragColor.a);
}
