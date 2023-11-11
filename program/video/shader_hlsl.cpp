
auto Shader::buildOutputEncodingHLSL() -> std::string {
    bool c64Glitches = vManager->isC64() && vManager->useLineGlitch();

    std::string out = R"(
        uniform sampler t0S;
        uniform Texture2D <float4> t0;
        #define mod(x, y) (x - y * floor(x / y))

        struct UBO {
            float4x4 projection;
        };
        uniform UBO ubo;

        uniform int oddLine;
        uniform float rotU;
        uniform float rotV;

        cbuffer scene {
            int ts : packoffset(c0);
            float4 targetSize : packoffset(c1);
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

    if (c64Glitches) {
        out += R"(
			uniform float CAS;
			uniform float RAS;
			uniform float PHI0;
			uniform float AEC;
			uniform float BA;
			uniform int cyclePixel;
		)";
    }

    out += R"(
        PSInput VS(VSInput input) {
            PSInput output;
            output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
            output.tex = input.tex;
            return output;
        }

        float4 PS(PSInput input) : SV_TARGET {
            float4 color = t0.Sample(t0S, input.tex);
    )";

    std::string rgbToLumaChroma;
    std::string flags = "256.0 * color.w";

    if (vManager->pal)
        rgbToLumaChroma = R"(
            float3 lumaChroma = mul( float3x3(0.299,0.587,0.114,-0.147407,-0.289391,0.436798,0.614777,-0.514799,-0.099978), color.rgb);
        )";
    else
        rgbToLumaChroma = R"(
            float3 lumaChroma = mul( float3x3(0.23485876230514607, 0.6335007388077467, 0.13164049888710716, 0.4409594767911895, -0.27984362502847304, -0.16111585176271648, 0.14630060102591497, -0.5594814826856017, 0.4131808816596867), color.rgb);
        )";

    if (vManager->shaderInputPrecision) {
        // we already start with yuv/yiq
        rgbToLumaChroma = R"(
            float3 lumaChroma = color.xyz;
        )";

        flags = "color.w";
    }

    out += rgbToLumaChroma;

    if (c64Glitches) {

        if (vManager->firSharp == 0)
            out += "float xposF = input.tex.x * targetSize.x;";
        else
            // texture is doubled size: to make this working we need the xpos in original size.
            // means: 0 and 0.5 is Pixel 1, 1 and 1.5 is Pixel 2 and so on.

            out += "float xposF = input.tex.x * (targetSize.x / 2.0);";

        out += R"(

			// to align pixel pos within vic cycle
            int xpos = int(xposF);
			xpos += cyclePixel;
			xpos &= 7;
        )";

        out += "int flags = int( " + flags + " ); ";


        // luma will be darkened when some vic lines changes their state
        // AEC and BA state will be transfered in unused alpha channel for each pixel

        if (vManager->baGlitch > 0.0)
            out += "lumaChroma.x *= 1.0 - ((flags & 1) * BA);";

        if (vManager->aecGlitch > 0.0)
            out += "lumaChroma.x *= 1.0 - (((flags >> 1) & 1) * AEC);";

        // PHI0, CAS, RAS have the same behaviour within each cycle.

        if (vManager->phi0Glitch > 0.0)
            // PHI0: second half cycle is darkened
            // Pixel: 0, 0.5, 1, 1.5, 2, 2.5, 3, 3.5 -> first half cycle
            out += "lumaChroma.x *= 1.0 - (((xpos >> 2) & 1) * PHI0);";

        if (vManager->casGlitch > 0.0)
            // CAS: third and fourth pixel of each half cycle are darkened
            out += "lumaChroma.x *= 1.0 - (((xpos >> 1) & 1) * CAS);";

        if (vManager->rasGlitch > 0.0)
            // RAS: first pixel of each half cycle will be non darkened
            out += "lumaChroma.x *= 1.0 - (((((~xpos >> 1) & 1) & (~xpos & 1)) ^ 1) * RAS);";
    }

    out += R"(
        float3 yuvEven = float3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV, lumaChroma.z * rotU + lumaChroma.y * rotV);
        float3 yuvOdd = float3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV * -1, lumaChroma.z * rotU + lumaChroma.y * rotV * -1);
    )";

//    out += R"(
//        #define yuvEven float3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV, lumaChroma.z * rotU + lumaChroma.y * rotV)
//        #define yuvOdd float3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV * -1, lumaChroma.z * rotU + lumaChroma.y * rotV * -1)
//    )";

    if (vManager->pal) {
        if (lace) {
            out += R"( int oddLineFrame = int(floor(mod(floor(input.tex.y * targetSize.y / 2.0), 2.0))); )"; // e, e, o, o, e, e, o, o, ...
            out += "return float4( lerp(yuvOdd, yuvEven, oddLineFrame ^ oddLine), 1.0 ); ";
        } else {
            out += R"( int oddLineFrame = int(floor(mod(input.tex.y * targetSize.y, 2.0))); )";  // e, o, e, o, e, o, ...
            out += "return float4( lerp(yuvOdd, yuvEven, oddLineFrame ^ oddLine), 1.0 ); ";
        }
    } else
        out += "return float4(yuvEven, 1.0); ";

    out += " } ";

    return out;
}

auto Shader::buildLumaLatencyHLSL() -> std::string {

    std::string out = R"(
		uniform sampler t0S;
        uniform Texture2D <float4> t0;

        cbuffer scene {
            int ts : packoffset(c0);
            float4 targetSize : packoffset(c1);
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

        uniform float lumaFall;
        uniform float lumaRise;

        PSInput VS(VSInput input) {
            PSInput output;
            output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
            output.tex = input.tex;
            return output;
        }

        float4 PS(PSInput input) : SV_TARGET {
            float4 color = t0.Sample(t0S, input.tex);
	)";

    int rounds = vManager->firSharp == 0 ? -3 : -7;

    double pos = ((double) (rounds)) / ((double) (vManager->emulator->cropWidth() << (vManager->firSharp == 0 ? 0 : 1)));

    out += "float2 xy = input.tex.xy;";

    out += "float ySrc = t0.Sample(t0S, xy + float2( " + _doubleToStr(pos) + " , 0.0)).x; ";

    out += R"(
			float y = ySrc;
			float yTarget;
			float yDiff = 0.0;
			int yDirection;
            int yChanged;
            float _lumaRise = lumaRise;
            float _lumaFall = lumaFall;
		)";

    if(vManager->firSharp != 0) {
        out += R"(
            _lumaRise *= 0.57142857;
            _lumaFall *= 0.57142857;
        )";
    }

    while (rounds < 0) {

        rounds += 1;

        pos = ((double) (rounds)) / ((double) (vManager->emulator->cropWidth() << (vManager->firSharp == 0 ? 0 : 1)));

        out += " yTarget = t0.Sample(t0S, xy + float2( " + _doubleToStr(pos) + " , 0.0)).x; ";

        out += R"(
                // check for a change of luma between 2 adjacent pixel
                yChanged = ySrc == yTarget ? 0 : 1;
                ySrc = yTarget;
                yDiff = yChanged == 1 ? (yTarget - y) : yDiff;
				yDirection = int( sign( yTarget - y ) );

				// a direction of 0 means no change, a direction of 1 means 'rise' only
				// a direction of -1 means 'fall' only
                y = yDirection == 1 ? min(y + (yDiff * _lumaRise), yTarget) :
                    (yDirection == -1 ? max(y + (yDiff * _lumaFall), yTarget) : y);
			)";
    }

    out += R"(
			return float4(y, color.yzw);
		}
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

        uniform float lumaNoise, chromaNoise;

        cbuffer scene {
            int ts : packoffset(c0);
            float4 targetSize : packoffset(c1);
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
            float time = float(ts) / 1000000.0;
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

        uniform float lineFactor;

        cbuffer scene {
            int ts : packoffset(c0);
            float4 targetSize : packoffset(c1);
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
            float time = float(ts) / 1000000.0;
            float2 xy = input.tex.xy;
            float offset = random(float2(time * xy.y, xy.y + (time * xy.y))) * lineFactor;
            float x0 = xy.x + offset;
            float x1 = x0 + targetSize.z;
            float4 tex0 = t0.Sample(t0S, float2( frac(x0), xy.y )).xyzw;
            float4 tex1 = t0.Sample(t0S, float2( frac(x1), xy.y )).xyzw;
            return lerp(tex0, tex1, frac(x0 * targetSize.x));
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

        cbuffer scene {
            int ts : packoffset(c0);
            float4 targetSize : packoffset(c1);
        };

        PSInput VS(VSInput input) {
            PSInput output;
            output.pos = mul(ubo.projection, float4(input.pos.xy, 0.f, 1.f));
            output.tex = input.tex;
            return output;
        }

        float4 PS(PSInput input) : SV_TARGET {
            float screenWidth = targetSize.x;
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

        double pos = ((double)i ) / ((double)(vManager->emulator->cropWidth() << 1));

        if (i != 0)
            out += "    yuv += (t0.Sample(t0S, input.tex + float2(" + _doubleToStr(-pos) + ", 0.0) ).xyz "
                     "+ t0.Sample(t0S, input.tex + float2(" + _doubleToStr(pos) + ", 0.0) ).xyz) * ";
//            out += "    yuv += (texture(source[0], input.tex + float2(" + std::to_string(-i) + " / screenWidth, 0.0) ).xyz "
//                    "+ texture(source[0], input.tex + float2(" + std::to_string(i) + " / screenWidth, 0.0) ).xyz) * ";

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

        uniform float hanoverBars;
        uniform float hanoverBarsAlt;
		uniform int oddLine;
        #define mod(x, y) (x - y * floor(x / y))

        struct VSInput {
            float2 pos : TEXCOORD0;
            float2 tex : TEXCOORD1;
        };

        struct PSInput {
            float4 pos : SV_POSITION;
            float2 tex : TEXCOORD0;
        };

        cbuffer scene {
            int ts : packoffset(c0);
            float4 targetSize : packoffset(c1);
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

        if (lace)
            out += R"(
				int lineFactor = int(floor(mod(floor(input.tex.y * targetSize.y / 2.0), 2.0)));
				float3 yuv = (t0.Sample(t0S, input.tex.xy).xyz);
				float3 yuvLineBefore = (t0.Sample(t0S, input.tex.xy + float2(0.0, -2.0 / targetSize.y )).xyz);
			)";
        else
            out += R"(
				int lineFactor = int(floor(mod(input.tex.y * targetSize.y, 2.0)));
				float3 yuv = (t0.Sample(t0S, input.tex.xy).xyz);
				float3 yuvLineBefore = (t0.Sample(t0S, input.tex.xy + float2(0.0, -1.0 / targetSize.y )).xyz);
			)";

        out += R"(
				float2 merged = float2(yuv.y + yuvLineBefore.y, yuv.z + yuvLineBefore.z) * lerp(hanoverBars, hanoverBarsAlt, lineFactor ^ oddLine) * 0.5;
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

        cbuffer scene {
            int ts : packoffset(c0);
            float4 targetSize : packoffset(c1);
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

        cbuffer scene {
            int ts : packoffset(c0);
            float4 targetSize : packoffset(c1);
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
			int lineFactor = int(floor(mod(input.tex.y * targetSize.y, 2.0)));

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

        uniform float Factor;
        uniform float Scale;

        struct VSInput {
            float2 pos : TEXCOORD0;
            float2 tex : TEXCOORD1;
        };

        struct PSInput {
            float4 pos : SV_POSITION;
            float2 tex : TEXCOORD0;
        };

        cbuffer scene {
            int ts : packoffset(c0);
            float4 targetSize : packoffset(c1);
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

    std::string uniforms = "";
    std::string light = "";

    if (vManager->maskLevel)
        uniforms += R"(
            uniform Texture2D maskLayer;
            uniform sampler maskLayerS;
            uniform float maskLevel;
            uniform float maskScaleX;
            uniform float maskScaleY;
        )";

    if (vManager->lightFromCenter)
        uniforms += R"( uniform float lightFromCenter; )";

    std::string out = R"(
        uniform sampler t0S;
        uniform Texture2D <float4> t0;
        struct UBO {
            float4x4 projection;
        };
        uniform UBO ubo;
		uniform float luminance;
    )";

    out += uniforms;

    out += R"(
        struct VSInput {
            float2 pos : TEXCOORD0;
            float2 tex : TEXCOORD1;
        };

        struct PSInput {
            float4 pos : SV_POSITION;
            float2 tex : TEXCOORD0;
        };

        cbuffer scene {
            int ts : packoffset(c0);
            float4 targetSize : packoffset(c1);
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
    //out += "color = lerp( float3(1.0, 1.0, 1.0), maskLayer.Sample( maskLayerS, input.tex * float2(1.0) ).xyz, 1.0 );";

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

    std::string _uniforms = "";

    if (!phase1) {
        _uniforms = R"(
            uniform float weight;
            uniform float glow;
        )";
    }

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

    out += _uniforms;

    out += R"(
        struct VSInput {
            float2 pos : TEXCOORD0;
            float2 tex : TEXCOORD1;
        };

        struct PSInput {
            float4 pos : SV_POSITION;
            float2 tex : TEXCOORD0;
        };

        cbuffer scene {
            int ts : packoffset(c0);
            float4 targetSize : packoffset(c1);
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
            double pos = ((double)i ) / ((double)(vManager->emulator->cropWidth() << 1));

            out += " sum += (t0.Sample(t0S, input.tex + float2(" + _doubleToStr(-pos) + ", 0.0) ).rgb "
                 "+ t0.Sample(t0S, input.tex + float2(" + _doubleToStr(pos) + ", 0.0) ).rgb) * ";

        } else {
            double pos = ((double)i ) / ((double)(vManager->emulator->cropHeight() << 1));

            out += " sum += (t0.Sample(t0S, input.tex + float2(0.0, " + _doubleToStr(-pos) + ") ).rgb "
                 "+ t0.Sample(t0S, input.tex + float2(0.0, " + _doubleToStr(pos) + ") ).rgb) * ";
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