
auto Shader::buildOutputEncodingHLSL() -> std::string {
    std::string out = R"(
        uniform sampler t0S;
        uniform Texture2D <float4> t0;
        #define mod(x, y) (x - y * floor(x / y))

        struct UBO {
            float4x4 projection;
        };
        uniform UBO ubo;

        cbuffer Scene : register(b0) {
            float ts packoffset(c0);
            float4 SourceSize packoffset(c1);
            float4 OutputSize packoffset(c2);
        };

        cbuffer Push {
            float oddLine
            float rotU;
            float rotV;
			float CAS;
			float RAS;
			float PHI0;
			float AEC;
			float BA;
			float cyclePixel;
            float lace;
            float palMode;
        };

        struct VSInput {
            float2 pos : TEXCOORD0;
            float2 tex : TEXCOORD1;
        };

        struct PSInput {
            float4 pos : SV_POSITION;
            float2 tex : TEXCOORD0;
        };
    )";
    // lace : e, e, o, o, e, e, o, o, ...
    // non lace: e, o, e, o, e, o, ...

    // luma will be darkened when some vic lines changes their state
    // AEC and BA state will be transferred in unused alpha channel for each pixel
    // PHI0, CAS, RAS have the same behavior within each cycle.
    // PHI0: second half cycle is darkened
    // Pixel: 0, 0.5, 1, 1.5, 2, 2.5, 3, 3.5 -> first half cycle
    // CAS: third and fourth pixel of each half cycle are darkened
    // RAS: first pixel of each half cycle will be non darkened

    out += R"(
        PSInput VS(VSInput input) {
            PSInput output;
            output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
            output.tex = input.tex;
            return output;
        }

        float4 PS(PSInput input) : SV_TARGET {
            float4 color = t0.Sample(t0S, input.tex);
            float3 lumaChroma = color.xyz;
            float xposF = input.tex.x * SourceSize.x;
            int xpos = int(xposF);
            xpos += int(cyclePixel);
			xpos &= 7;
            int flags = int( color.w );
            lumaChroma.x *= 1.0 - ((flags & 1) * BA);
            lumaChroma.x *= 1.0 - (((flags >> 1) & 1) * AEC);
            lumaChroma.x *= 1.0 - (((xpos >> 2) & 1) * PHI0);
            lumaChroma.x *= 1.0 - (((xpos >> 1) & 1) * CAS);
            lumaChroma.x *= 1.0 - (((((~xpos >> 1) & 1) & (~xpos & 1)) ^ 1) * RAS);

            float3 yuvEven = float3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV, lumaChroma.z * rotU + lumaChroma.y * rotV);
            float3 yuvOdd = float3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV * -1, lumaChroma.z * rotU + lumaChroma.y * rotV * -1);

            if (palMode) {
                int oddLineFrame = int(floor(mod(floor(input.tex.y * OutputSize.y / float( 1 << int(lace) )), 2.0)));
                return float4( lerp(yuvOdd, yuvEven, (oddLineFrame ^ int(oddLine)) ), 1.0 );
            } else
                return float4(yuvEven, 1.0);
    )";

    return out;
}

auto Shader::buildLumaLatencyHLSL() -> std::string {

    std::string out = R"(
		uniform sampler t0S;
        uniform Texture2D <float4> t0;

        cbuffer Scene : register(b0) {
            float ts packoffset(c0);
            float4 SourceSize packoffset(c1);
            float4 OutputSize packoffset(c2);
            float4 OriginalSize packoffset(c3);
        };

        cbuffer Push {
            float lumaFall;
            float lumaRise;
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

        PSInput VS(VSInput input) {
            PSInput output;
            output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
            output.tex = input.tex;
            return output;
        }

        float4 PS(PSInput input) : SV_TARGET {
            float4 color = t0.Sample(t0S, input.tex);
            float2 xy = input.tex.xy;
            float ySrc = t0.Sample(t0S, xy + float2( -3.0 / float(SourceSize.x), 0.0)).x;
			float y = ySrc;
			float yTarget;
			float yDiff = 0.0;
			int yDirection;
            int yChanged;
            float _lumaRise = lumaRise;
            float _lumaFall = lumaFall;

            yTarget = t0.Sample(t0S, xy + float2( -2.0 / float(SourceSize.x), 0.0)).x;
            // check for a change of luma between 2 adjacent pixel
            yChanged = ySrc == yTarget ? 0 : 1;
            ySrc = yTarget;
            yDiff = yChanged == 1 ? (yTarget - y) : yDiff;
            yDirection = int( sign( yTarget - y ) );

            // a direction of 0 means no change, a direction of 1 means 'rise' only
            // a direction of -1 means 'fall' only
            y = yDirection == 1 ? min(y + (yDiff * _lumaRise), yTarget) :
                (yDirection == -1 ? max(y + (yDiff * _lumaFall), yTarget) : y);

            yTarget = t0.Sample(t0S, xy + float2( -1.0 / float(SourceSize.x), 0.0)).x;
            // check for a change of luma between 2 adjacent pixel
            yChanged = ySrc == yTarget ? 0 : 1;
            ySrc = yTarget;
            yDiff = yChanged == 1 ? (yTarget - y) : yDiff;
            yDirection = int( sign( yTarget - y ) );

            // a direction of 0 means no change, a direction of 1 means 'rise' only
            // a direction of -1 means 'fall' only
            y = yDirection == 1 ? min(y + (yDiff * _lumaRise), yTarget) :
                (yDirection == -1 ? max(y + (yDiff * _lumaFall), yTarget) : y);

            yTarget = t0.Sample(t0S, xy).x;
            // check for a change of luma between 2 adjacent pixel
            yChanged = ySrc == yTarget ? 0 : 1;
            ySrc = yTarget;
            yDiff = yChanged == 1 ? (yTarget - y) : yDiff;
            yDirection = int( sign( yTarget - y ) );

            // a direction of 0 means no change, a direction of 1 means 'rise' only
            // a direction of -1 means 'fall' only
            y = yDirection == 1 ? min(y + (yDiff * _lumaRise), yTarget) :
                (yDirection == -1 ? max(y + (yDiff * _lumaFall), yTarget) : y);

            return float4(y, color.yzw);
	)";

    return out;
}

auto Shader::buildNoiseHLSL() -> std::string {

    std::string out = R"(
        uniform sampler t0S;
        uniform Texture2D <float4> t0;

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

        cbuffer Scene : register(b0) {
            float ts packoffset(c0);
            float4 SourceSize packoffset(c1);
            float4 OutputSize packoffset(c2);
            float4 OriginalSize packoffset(c3);
        };

        cbuffer Push {
            float lumaNoise;
            float chromaNoise;
        };

        float random( float2 seed ) {
            int n = int((seed.x * 40.0) + (seed.y * 6400.0));
            n = (n<<13) ^ n;
            return 1.0 - float((n * (((n * n) * 15731) + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0;
        }

        PSInput VS(VSInput input) {
            PSInput output;
            output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
            output.tex = input.tex;
            return output;
        }

        float4 PS(PSInput input) : SV_TARGET {
            float time = ts / 1000000.0;
            float2 xy = input.tex.xy;
            float y = random(xy + float2(time * xy.x, time * xy.y)) * lumaNoise;
            float u = random(xy + float2(time * xy.y, time * xy.x)) * chromaNoise;
            float v = random(xy - float2(time * xy.x, time * xy.y)) * chromaNoise;
            return t0.Sample(t0S, xy).xyzw + float4(y, u, v, 0.0);
        }
    )";

    return out;
}

auto Shader::buildRandomLineOffsetHLSL() -> std::string {

    std::string out = R"(
        uniform sampler t0S;
        uniform Texture2D <float4> t0;

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

        cbuffer Scene : register(b0) {
            float ts packoffset(c0);
            float4 SourceSize packoffset(c1);
            float4 OutputSize packoffset(c2);
            float4 OriginalSize packoffset(c3);
        };

        cbuffer Push {
            float lineFactor;
        };

        float random( float2 seed ) {
            int n = int((seed.x * 40.0) + (seed.y * 6400.0));
            n = (n<<13) ^ n;
            return 1.0 - float((n * (((n * n) * 15731) + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0;
        }

        PSInput VS(VSInput input) {
            PSInput output;
            output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
            output.tex = input.tex;
            return output;
        }

        float4 PS(PSInput input) : SV_TARGET {
            float time = ts / 1000000.0;
            float2 xy = input.tex.xy;
            float offset = random(float2(time * xy.y, xy.y + (time * xy.y))) * lineFactor;
            float x0 = xy.x + offset;
            float x1 = x0 + targetSize.z;
            float4 tex0 = t0.Sample(t0S, float2( frac(x0), xy.y )).xyzw;
            float4 tex1 = t0.Sample(t0S, float2( frac(x1), xy.y )).xyzw;
            return lerp(tex0, tex1, frac(x0 * OutputSize.x));
        }
    )";

    return out;
}

auto Shader::buildBandwidthReductionHLSL() -> std::string {

    auto subRegion = vManager->emulator->getSubRegion();
    double videoBandWith;
    double subCarrier;

    switch(subRegion) {
        default:
        case Emulator::Interface::SubRegion::Pal_B:
            videoBandWith = 5000000.0;
            subCarrier = 4433618.75;
            break;
        case Emulator::Interface::SubRegion::Pal_N:
            videoBandWith = 4200000.0;
            subCarrier = 3582056.25;
            break;
        case Emulator::Interface::SubRegion::Pal_M:
            videoBandWith = 4200000.0;
            subCarrier = 3575611.0;
            break;
        case Emulator::Interface::SubRegion::Ntsc_M:
            videoBandWith = 4200000.0;
            subCarrier = 3579545.0;
            break;
    }

    // sample rate is 4 times the color sub carrier
    // The sampling rates for NTSC and PAL composite video signals are 14.3181818 Msamples/sec and 17.734475 Msamples/sec, respectively.
    //SincFirFilter fir( vManager->pal ? 5000000.0 : 4200000.0, vManager->pal ? 17734475.0 : 14318180.0 );
    SincFirFilter fir( videoBandWith, subCarrier * 4.0 );
    unsigned NLuma = vManager->firTaps;
    auto firLuma = fir.calculateLopass( NLuma, videoBandWith );

    // chrominance is modulated at subcarrier frequency
    // demodulated to baseband to use a lowpass filter and use bandwidth as cutoff frequency
    unsigned NChromaUI = vManager->firTaps;
    // U : 1.3 MHz  I : 1.5 MHz
    auto firChromaUI = fir.calculateLopass( NChromaUI, vManager->pal ? 1300000.0 : 1500000.0 );
    unsigned NChromaVQ = vManager->firTaps;
    // V : 1.3 MHz  Q : 0.5 MHz
    auto firChromaVQ = fir.calculateLopass( NChromaVQ, vManager->pal ? 1300000.0 : 500000.0 );

    unsigned maxTaps = std::max(std::max(NLuma, NChromaUI), NChromaVQ);
    unsigned centerLuma = NLuma / 2;
    unsigned centerChromaUI = NChromaUI / 2;
    unsigned centerChromaVQ = NChromaVQ / 2;

    std::string _sharp = "0.0";
    if (vManager->firSharp == 1)
        _sharp = "1.0";
    else if (vManager->firSharp == -1)
        _sharp = "-1.0";

    std::string out = R"(
        uniform sampler t0S;
        uniform Texture2D <float4> t0;

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

        cbuffer Scene : register(b0) {
            float ts packoffset(c0);
            float4 SourceSize packoffset(c1);
            float4 OutputSize packoffset(c2);
            float4 OriginalSize packoffset(c3);
        };

        PSInput VS(VSInput input) {
            PSInput output;
            output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
            output.tex = input.tex;
            return output;
        }

        float4 PS(PSInput input) : SV_TARGET {
            float screenWidth = OutputSize.x;
            float3 yuv=t0.Sample(t0S, input.tex + float2(
    )";

    out += _sharp;

    out += " / screenWidth, 0.0 )).xyz * ";

    for( int i = 0; i <= (maxTaps / 2); i++ ) {

        double luma = 0.0;
        double chromaUI = 0.0;
        double chromaVQ = 0.0;

        if ( (centerLuma + i) < NLuma )
            luma = firLuma[centerLuma + i];

        if ( (centerChromaUI + i) < NChromaUI )
            chromaUI = firChromaUI[centerChromaUI + i];

        if ( (centerChromaVQ + i) < NChromaVQ )
            chromaVQ = firChromaVQ[centerChromaVQ + i];

        if (i != 0)
            out += "    yuv += (t0.Sample(t0S, input.tex + float2(-1.0 * float(" + std::to_string(i) + ") / OutputSize.x, 0.0) ).xyz "
                     "+ t0.Sample(t0S, input.tex + float2(float(" + std::to_string(i) + ") / OutputSize.x, 0.0) ).xyz) * ";

        out += "float3(" + _doubleToStr(luma) + "," + _doubleToStr(chromaUI) + "," + _doubleToStr(chromaVQ) + ");";
    }

    out += R"( return float4( yuv, 1.0 );
    }
)";

    return out;
}

auto Shader::buildDelayLineAndConvertToRgbHLSL() -> std::string {

    std::string out = R"(
        uniform sampler t0S;
        uniform Texture2D <float4> t0;

        struct UBO {
            float4x4 projection;
        };
        uniform UBO ubo;

        #define mod(x, y) (x - y * floor(x / y))

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
            float hanoverBars;
            float hanoverBarsAlt;
		    float oddLine;
            float lace;
        };

        PSInput VS(VSInput input) {
            PSInput output;
            output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
            output.tex = input.tex;
            return output;
        }

        float4 PS(PSInput input) : SV_TARGET {
	)";

    if (vManager->pal) {

        out += R"(
				int lineFactor = int(floor(mod(floor(input.tex.y * OutputSize.y / float( 1 << int(lace) )), 2.0)));
				float3 yuv = (t0.Sample(t0S, input.tex.xy).xyz);
				float3 yuvLineBefore = (t0.Sample(t0S, input.tex.xy + float2(0.0, -1.0 * float( 1 << int(lace) ) / OutputSize.y )).xyz);
			)";

        out += R"(
				float2 merged = float2(yuv.y + yuvLineBefore.y, yuv.z + yuvLineBefore.z) * lerp(hanoverBars, hanoverBarsAlt, lineFactor ^ int(oddLine)) * 0.5;
				float3 color = mul(float3x3(1.0,0.0,1.140251,1.0,-0.39393070,-0.58080921,1.0,2.0283976,0.0), float3(yuv.x, merged.x, merged.y));
				return float4(color, 1.0);
			}
			)";
    } else {

        out += R"(
			float3 color = mul(float3x3(1.0, 1.630, 0.317, 1.0, -0.378, -0.466, 1.0, -1.089, 1.677), t0.Sample(t0S, input.tex.xy).xyz);
			return float4(color, 1.0);
		}
		)";
    }

    return out;
}

auto Shader::buildGammaHLSL() -> std::string {

    // gamma lookup texture has 768 values instead of 256 for an 8-bit channel.
    // Means, we provide gamma values for out of range colors.
    // because of color math before it could happen that a resulting color is bigger than
    // 255[1.0] or smaller than 0. these colors are out of range, and we should use
    // max or min values instead. A out-of-range color could have
    // a gamma color, which is in range again. If we clipped the color in range before gamma
    // lookup, the resulting value wouldn't be correct in a mathematical way.
    // after gamma lookup, the color will be finally clipped for display.

    // texture is normalized; means: positon / 768
    // center: 256 / 768 = 1 / 3
    // color is normalized between [0,1] -> * 255 (for denormalization) / 768 for lookup

    std::string out = R"(
        uniform sampler t0S;
        uniform sampler gammaS;
        uniform Texture2D <float4> t0;
        uniform Texture2D gamma;

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

        cbuffer Scene : register(b0) {
            float ts packoffset(c0);
            float4 SourceSize packoffset(c1);
            float4 OutputSize packoffset(c2);
            float4 OriginalSize packoffset(c3);
        };
        PSInput VS(VSInput input) {
            PSInput output;
            output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
            output.tex = input.tex;
            return output;
        }

        float4 PS(PSInput input) : SV_TARGET {
			float3 color = t0.Sample(t0S, input.tex).rgb;
			color.r = gamma.Sample(gammaS, 1.0/3.0 + color.r * 0.33203125 ).x;
			color.g = gamma.Sample(gammaS, 1.0/3.0 + color.g * 0.33203125 ).x;
			color.b = gamma.Sample(gammaS, 1.0/3.0 + color.b * 0.33203125 ).x;
			return float4( color, 1.0 );
		}
    )";

    return out;
}

auto Shader::buildGammaAndScanlinesHLSL() -> std::string {

    std::string out = R"(
        uniform sampler t0S;
        uniform sampler gammaWithShadeS;
        uniform sampler gammaS;
        uniform Texture2D <float4> t0;
        uniform Texture2D gammaWithShade;
        uniform Texture2D gamma;
        #define mod(x, y) (x - y * floor(x / y))

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

        cbuffer Scene : register(b0) {
            float ts packoffset(c0);
            float4 SourceSize packoffset(c1);
            float4 OutputSize packoffset(c2);
            float4 OriginalSize packoffset(c3);
        };

        cbuffer Push {
            float lace;
        };

        PSInput VS(VSInput input) {
            PSInput output;
            output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
            output.tex = input.tex;
            return output;
        }

        float4 PS(PSInput input) : SV_TARGET {
		    float3 color = t0.Sample(t0S, input.tex).rgb;
			float3 colorUp = t0.Sample(t0S, input.tex.xy + float2( 0.0, -1.0 / targetSize.y ) ).rgb;
			float3 colorDown = t0.Sample(t0S, input.tex.xy + float2( 0.0, 1.0 / targetSize.y ) ).rgb;
			int lineFactor = int(floor(mod(input.tex.y * OutputSize.y, 2.0))) | int(lace);

			color.r = lerp( gamma.Sample(gammaS, 1.0/3.0 + color.r * 0.33203125 ).x, gammaWithShade.Sample(gammaWithShadeS, 1.0/3.0 + 0.166015625 * colorUp.r + 0.166015625 * colorDown.r ).x, lineFactor );
			color.g = lerp( gamma.Sample(gammaS, 1.0/3.0 + color.g * 0.33203125 ).x, gammaWithShade.Sample(gammaWithShadeS, 1.0/3.0 + 0.166015625 * colorUp.g + 0.166015625 * colorDown.g ).x, lineFactor );
			color.b = lerp( gamma.Sample(gammaS, 1.0/3.0 + color.b * 0.33203125 ).x, gammaWithShade.Sample(gammaWithShadeS, 1.0/3.0 + 0.166015625 * colorUp.b + 0.166015625 * colorDown.b ).x, lineFactor );

			return float4( color, 1.0 );
		}
    )";

    return out;
}

auto Shader::buildRadialDistortionHLSL() -> std::string {

    std::string out = R"(
        uniform sampler t0S;
        uniform Texture2D <float4> t0;

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
            float Factor;
            float Scale;
        };

        float2 radialDistortion( float2 xy ){
            float2 center = xy - float2(0.5, 0.5);
            float dist = dot(center,center) * Factor;
            return xy + (center * (1.0 + dist) * dist);
        }

        PSInput VS(VSInput input) {
            PSInput output;
            output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
            output.tex = input.tex;
            return output;
        }

        float4 PS(PSInput input) : SV_TARGET {
            float2 xy = ((radialDistortion(input.tex.xy) - float2(0.5, 0.5)) * Scale) + float2(0.5, 0.5);

            return t0.Sample(t0S, xy);
        }
    )";

    return out;
}

auto Shader::buildMaskHLSL() -> std::string {

    std::string light = "";

    std::string out = R"(
        uniform sampler t0S;
        uniform Texture2D <float4> t0;
        struct UBO {
            float4x4 projection;
        };
        uniform UBO ubo;
    )";

    if (vManager->maskLevel)
        out += R"(
            uniform Texture2D maskLayer;
            uniform sampler maskLayerS;

        )";

    out += R"(
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
            float maskLevel;
            float maskScaleX;
            float maskScaleY;
            float lightFromCenter;
            float luminance;
        };

        PSInput VS(VSInput input) {
            PSInput output;
            output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
            output.tex = input.tex;
            return output;
        }

        float4 PS(PSInput input) : SV_TARGET {

            float3 color = t0.Sample(t0S, input.tex).xyz;
        )";

    if (vManager->maskLevel)
        out += "color *= lerp( float3(1.0, 1.0, 1.0), maskLayer.Sample( maskLayerS, input.tex * float2(maskScaleX, maskScaleY) ).xyz, maskLevel ); ";

    if (vManager->lightFromCenter) {
        out += R"(
            float2 lightVector = (input.tex - float2(0.5, 0.5)) * lightFromCenter;
        )";

        light = "exp(-dot(lightVector, lightVector)) * ";
    }

    out += " color *= " + light + "luminance; ";

    out += R"(
        return float4( color, 1.0 );
        }
    )";

    return out;
}

auto Shader::buildBloomHLSL( bool phase1 ) -> std::string {
    GaussianBlur gB( vManager->bloomRadius << 1, vManager->bloomVariance);

    std::string out = R"(
        uniform sampler t0S;
        uniform Texture2D <float4> t0;
        uniform sampler t1S;
        uniform Texture2D <float4> t1;
        struct UBO {
            float4x4 projection;
        };
        uniform UBO ubo;
    )";

    out += R"(
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
            float weight;
            float glow;
        };

        PSInput VS(VSInput input) {
            PSInput output;
            output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
            output.tex = input.tex;
            return output;
        }

		float4 PS(PSInput input) : SV_TARGET {

            float3 sum = float3(0.0, 0.0, 0.0);
	)";

    for ( int i = (vManager->bloomRadius << 1); i >= 0; i-- ) {

        if (i == 0) {
            out += " sum += t0.Sample(t0S, input.tex).rgb * ";

        } else if (phase1) {
            out += " sum += (t0.Sample(t0S, input.tex + float2(-1.0 * float(" + std::to_string(i) + ") / OutputSize.x, 0.0) ).rgb "
                 "+ t0.Sample(t0S, input.tex + float2(float(" + std::to_string(i) + ") / OutputSize.x, 0.0) ).rgb) * ";

        } else {
            out += " sum += (t0.Sample(t0S, input.tex + float2(0.0, -1.0 * float(" + std::to_string(i) + ") / OutputSize.y) ).rgb "
                 "+ t0.Sample(t0S, input.tex + float2(0.0, float(" + std::to_string(i) + ") / OutputSize.y) ).rgb) * ";
        }

        out += " " + GUIKIT::String::convertDoubleToString( gB.get( i ) ) + ";";
    }

    if (phase1)
        out += R"(
			return float4( sum.rgb, 1.0 );
			}
		)";
    else if (vManager->bloomWeight == 3.0)
        out += R"(
			return float4(clamp(t1.Sample(t1S, input.tex).rgb + ( sum.rgb * glow ), 0.0, 1.0), 1.0);
			}
		)";
    else
        out += R"(
			return float4(clamp(t1.Sample(t1S, input.tex).rgb + ( pow(sum.rgb, float3(weight, weight, weight)) * glow ), 0.0, 1.0), 1.0);
			}
		)";

    return out;
}