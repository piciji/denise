/*
   CRT - Trinitron
   
   Copyright (C) 2019 guest(r) - guest.r@gmail.com

   Thanks to Retro Nerd and Dr. Venom for inspiration. :)
   
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

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

#define brightboost  1.25     // adjust brightness
#define saturation   3.00     // 1.0 is normal saturation
#define scanline     8.0      // scanline param, vertical sharpness
#define beam_min     1.40     // dark area beam min - narrow
#define beam_max     0.90     // bright area beam max - wide
#define h_sharp      2.0      // pixel sharpness
#define bloompix     0.50     // glow shape, more is harder
#define bloompixy    0.80     // glow shape, more is harder
#define glow         0.07     // glow ammount
#define mcut         0.20     // Mask 5&6 cutoff
#define maskDark     0.50     // Dark "Phosphor"
#define maskLight    1.50     // Light "Phosphor"

#define eps 1e-8

float3 sw(float x, float3 color)
{	
	float mx = max(max(color.r, color.g),color.b);
	x = lerp (x, beam_min*x, max(x-0.4*mx,0.0));
	float3 tmp = lerp(float3(1.2*beam_min,1.2*beam_min,1.2*beam_min),float3(beam_max,beam_max,beam_max), color);
	float3 ex = float3(x, x, x)*tmp;
	float br = clamp(beam_min - 1.0, 0.2, 0.45);
	return exp2(-scanline*ex*ex)/(1.0-br+br*color);
}

float3 Mask(float2 pos, float3 c)
{
	float3 mask = float3(maskDark, maskDark, maskDark);
	
	float mx = max(max(c.r,c.g),c.b);
	float mt = min( 1.25*max(mx-mcut,0.0)/(1.0-mcut) ,maskDark);
	float3 maskTmp = float3(mt,mt,mt );
	float adj = maskLight - 0.4*(maskLight - 1.0)*mx + 0.75*(1.0-mx)*(1.0+0.4*mcut);	
	mask = maskTmp;
	pos.x = frac(pos.x/3.0);
	if      (pos.x < 0.333) mask.r = adj;
	else if (pos.x < 0.666) mask.g = adj;
	else                    mask.b = adj;
	
	return mask;
}


PSInput VS(VSInput input) {
    PSInput output;
    output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
    output.tex = input.tex;
    return output;
}

float4 PS(PSInput input) : SV_TARGET {

	float2 ps = 1.0/sourceSize.xy;
	float2 OGL2Pos = input.tex.xy / ps - float2(0.5,0.5);
	float2 fp = frac(OGL2Pos);
	float2 dx = float2(ps.x,0.0);
	float2 dy = float2(0.0, ps.y);
	
	float2 pC4 = floor(OGL2Pos) * ps + 0.5*ps;	
	
	// Reading the texels
	float3 ul = t0.Sample(t0S, pC4     ).xyz; ul*=ul;
	float3 ur = t0.Sample(t0S, pC4 + dx).xyz; ur*=ur;
	float3 dl = t0.Sample(t0S, pC4 + dy).xyz; dl*=dl;
	float3 dr = t0.Sample(t0S, pC4 + ps).xyz; dr*=dr;
	
	float lx = fp.x;        lx = pow(lx, h_sharp);
	float rx = 1.0 - fp.x;  rx = pow(rx, h_sharp);
	
	float3 color1 = (ur*lx + ul*rx)/(lx+rx);
	float3 color2 = (dr*lx + dl*rx)/(lx+rx);

// calculating scanlines
	
	float f = fp.y;

	float3 w1 = sw(f,color1);
	float3 w2 = sw(1.0-f,color2);
	
	float3 color = color1*w1 + color2*w2;
	float3 ctemp = color / (w1 + w2);
	
	color*=brightboost;
	color = min(color, 1.0);

	color*=Mask(input.pos.xy, sqrt(ctemp));
	
	float2 x2 = 2.0*dx; float2 x3 = 3.0*dx;
	float2 y2 = 2.0*dy;

	float wl3 = 2.0 + fp.x; wl3*=wl3; wl3 = exp2(-bloompix*wl3);
	float wl2 = 1.0 + fp.x; wl2*=wl2; wl2 = exp2(-bloompix*wl2);
	float wl1 =       fp.x; wl1*=wl1; wl1 = exp2(-bloompix*wl1);
	float wr1 = 1.0 - fp.x; wr1*=wr1; wr1 = exp2(-bloompix*wr1);
	float wr2 = 2.0 - fp.x; wr2*=wr2; wr2 = exp2(-bloompix*wr2);
	float wr3 = 3.0 - fp.x; wr3*=wr3; wr3 = exp2(-bloompix*wr3);	
	
	float wt = 1.0/(wl3+wl2+wl1+wr1+wr2+wr3);
	
	float3 l3 = t0.Sample(t0S, pC4 -x2 ).xyz; l3*=l3;
	float3 l2 = t0.Sample(t0S, pC4 -dx ).xyz; l2*=l2;
	float3 l1 = t0.Sample(t0S, pC4     ).xyz; l1*=l1;
	float3 r1 = t0.Sample(t0S, pC4 +dx ).xyz; r1*=r1;
	float3 r2 = t0.Sample(t0S, pC4 +x2 ).xyz; r2*=r2;
	float3 r3 = t0.Sample(t0S, pC4 +x3 ).xyz; r3*=r3;

	float3 t1 = (l3*wl3 + l2*wl2 + l1*wl1 + r1*wr1 + r2*wr2 + r3*wr3)*wt;
	
	l3 = t0.Sample(t0S, pC4 -x2 -dy).xyz; l3*=l3;
	l2 = t0.Sample(t0S, pC4 -dx -dy).xyz; l2*=l2;
	l1 = t0.Sample(t0S, pC4     -dy).xyz; l1*=l1;
	r1 = t0.Sample(t0S, pC4 +dx -dy).xyz; r1*=r1;
	r2 = t0.Sample(t0S, pC4 +x2 -dy).xyz; r2*=r2;
	r3 = t0.Sample(t0S, pC4 +x3 -dy).xyz; r3*=r3;
	
	float3 t2 = (l3*wl3 + l2*wl2 + l1*wl1 + r1*wr1 + r2*wr2 + r3*wr3)*wt;	
	
	l3 = t0.Sample(t0S, pC4 -x2 +dy).xyz; l3*=l3;
	l2 = t0.Sample(t0S, pC4 -dx +dy).xyz; l2*=l2;
	l1 = t0.Sample(t0S, pC4     +dy).xyz; l1*=l1;
	r1 = t0.Sample(t0S, pC4 +dx +dy).xyz; r1*=r1;
	r2 = t0.Sample(t0S, pC4 +x2 +dy).xyz; r2*=r2;
	r3 = t0.Sample(t0S, pC4 +x3 +dy).xyz; r3*=r3;

	float3 b1 = (l3*wl3 + l2*wl2 + l1*wl1 + r1*wr1 + r2*wr2 + r3*wr3)*wt;

	l3 = t0.Sample(t0S, pC4 -x2 +y2).xyz; l3*=l3;
	l2 = t0.Sample(t0S, pC4 -dx +y2).xyz; l2*=l2;
	l1 = t0.Sample(t0S, pC4     +y2).xyz; l1*=l1;
	r1 = t0.Sample(t0S, pC4 +dx +y2).xyz; r1*=r1;
	r2 = t0.Sample(t0S, pC4 +x2 +y2).xyz; r2*=r2;
	r3 = t0.Sample(t0S, pC4 +x3 +y2).xyz; r3*=r3;
	
	float3 b2 = (l3*wl3 + l2*wl2 + l1*wl1 + r1*wr1 + r2*wr2 + r3*wr3)*wt;	
	
	wl2 = 1.0 + fp.y; wl2*=wl2; wl2 = exp2(-bloompixy*wl2);
	wl1 =       fp.y; wl1*=wl1; wl1 = exp2(-bloompixy*wl1);
	wr1 = 1.0 - fp.y; wr1*=wr1; wr1 = exp2(-bloompixy*wr1);
	wr2 = 2.0 - fp.y; wr2*=wr2; wr2 = exp2(-bloompixy*wr2);
	
	wt = 1.0/(wl2+wl1+wr1+wr2);	
	
	float3 Bloom = (t2*wl2 + t1*wl1 + b1*wr1 + b2*wr2)*wt;

	color += Bloom*glow; 
	
	color = min(color, 1.0);

	color = pow(color, float3(0.475, 0.475, 0.475));
	
	float l = length(color);
	color = normalize(pow(color + float3(eps,eps,eps), float3(saturation,saturation,saturation)))*l;

	return float4(color,1.0);
}