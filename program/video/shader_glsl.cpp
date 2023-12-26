
auto Shader::buildOutputEncodingGLSL() -> std::string {
    bool c64Glitches = vManager->useLineGlitch();

    std::string out = R"(
        #version 140

        uniform int oddLine;
        uniform sampler2D source[];
        uniform float rotU, rotV;
        uniform vec4 targetSize;
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
        in vec2 texCoordFrag;

        out vec4 fragColor;

        void main() {
            vec4 color = texture(source[0], texCoordFrag).xyzw;
    )";

    std::string rgbToLumaChroma;
    std::string flags = "256.0 * color.w";

    if (vManager->pal)
        rgbToLumaChroma = R"(
            vec3 lumaChroma = vec3( color.rgb * mat3(0.299,0.587,0.114,-0.147407,-0.289391,0.436798,0.614777,-0.514799,-0.099978));
        )";
    else
        rgbToLumaChroma = R"(
            vec3 lumaChroma = vec3( color.rgb * mat3(0.23485876230514607, 0.6335007388077467, 0.13164049888710716, 0.4409594767911895, -0.27984362502847304, -0.16111585176271648, 0.14630060102591497, -0.5594814826856017, 0.4131808816596867));
        )";

    if (vManager->shaderInputPrecision) {
        // we already start with yuv/yiq
        rgbToLumaChroma = R"(
            vec3 lumaChroma = color.xyz;
        )";

        flags = "color.w";
    }

    out += rgbToLumaChroma;

    if (c64Glitches) {

        if (vManager->firSharp == 0)
            out += "float xposF = texCoordFrag.x * targetSize.x;";
        else
            // texture is doubled size: to make this working we need the xpos in original size.
            // means: 0 and 0.5 is Pixel 1, 1 and 1.5 is Pixel 2 and so on.

            out += "float xposF = texCoordFrag.x * (targetSize.x / 2.0);";

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
        vec3 yuvEven = vec3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV, lumaChroma.z * rotU + lumaChroma.y * rotV);
        vec3 yuvOdd = vec3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV * -1, lumaChroma.z * rotU + lumaChroma.y * rotV * -1);
    )";

//    out += R"(
//        #define yuvEven vec3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV, lumaChroma.z * rotU + lumaChroma.y * rotV)
//        #define yuvOdd vec3(lumaChroma.x, lumaChroma.y * rotU - lumaChroma.z * rotV * -1, lumaChroma.z * rotU + lumaChroma.y * rotV * -1)
//    )";

    if (vManager->pal) {
        if (lace) {
            out += R"( int oddLineFrame = int(floor(mod(floor(texCoordFrag.y * targetSize.y / 2.0), 2.0))); )"; // e, e, o, o, e, e, o, o, ...
            out += "fragColor=vec4( mix(yuvOdd, yuvEven, oddLineFrame ^ oddLine), 1.0 ); ";
        } else {
            out += R"( int oddLineFrame = int(floor(mod(texCoordFrag.y * targetSize.y, 2.0))); )";  // e, o, e, o, e, o, ...
            out += "fragColor=vec4( mix(yuvOdd, yuvEven, oddLineFrame ^ oddLine), 1.0 ); ";
        }
    } else
        out += "fragColor=vec4(yuvEven, 1.0); ";

    out += " } ";

    return out;
}

auto Shader::buildLumaLatencyGLSL() -> std::string {

    std::string out = R"(
		#version 140

		uniform sampler2D source[];
        uniform float lumaFall;
        uniform float lumaRise;
        uniform vec4 targetSize;

        in vec2 texCoordFrag;

        out vec4 fragColor;

		void main() {
			vec4 color = texture(source[0], texCoordFrag ).xyzw;
	)";

    int rounds = vManager->firSharp == 0 ? -3 : -7;

    double pos = ((double) (rounds)) / ((double) (vManager->emulator->cropWidth() << (vManager->firSharp == 0 ? 0 : 1)));

    out += "vec2 xy = texCoordFrag.xy;";

    out += "float ySrc = texture(source[0], xy + vec2( " + _doubleToStr(pos) + " , 0.0)).x; ";

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

        out += " yTarget = texture(source[0], xy + vec2( " + _doubleToStr(pos) + " , 0.0)).x; ";

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
			fragColor = vec4(y, color.yzw);
		}
	)";

    return out;
}

auto Shader::buildNoiseGLSL() -> std::string {

    std::string out = R"(
        #version 140

        in vec2 texCoordFrag;

        out vec4 fragColor;
        uniform sampler2D source[];

        uniform float lumaNoise, chromaNoise;
        uniform int ts;

        float random( vec2 seed ) {
            int n = int((seed.x * 40.0) + (seed.y * 6400.0));
            n = (n<<13) ^ n;
            return 1.0 - float((n * (((n * n) * 15731) + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0;
        }

        void main() {
            float time = float(ts) / 1000000.0;
            vec2 xy = texCoordFrag.xy;
            float y = random(xy + vec2(time * xy.x, time * xy.y)) * lumaNoise;
            float u = random(xy + vec2(time * xy.y, time * xy.x)) * chromaNoise;
            float v = random(xy - vec2(time * xy.x, time * xy.y)) * chromaNoise;
            fragColor = texture(source[0], xy).xyzw + vec4(y, u, v, 0.0);
        }
    )";

    return out;
}

auto Shader::buildRandomLineOffsetGLSL() -> std::string {

    std::string out = R"(
        #version 140

        in vec2 texCoordFrag;

        out vec4 fragColor;
        uniform sampler2D source[];

        uniform vec4 targetSize;
        uniform float lineFactor;
        uniform int ts;

        float random( vec2 seed ) {
            int n = int((seed.x * 40.0) + (seed.y * 6400.0));
            n = (n<<13) ^ n;
            return 1.0 - float((n * (((n * n) * 15731) + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0;
        }

        void main() {
            float time = float(ts) / 1000000.0;
            vec2 xy = texCoordFrag.xy;
            float offset = random(vec2(time * xy.y, xy.y + (time * xy.y))) * lineFactor;
            float x0 = xy.x + offset;
            float x1 = x0 + targetSize.z;
            vec4 tex0 = texture(source[0], vec2( fract(x0), xy.y )).xyzw;
            vec4 tex1 = texture(source[0], vec2( fract(x1), xy.y )).xyzw;
            fragColor = mix(tex0, tex1, fract(x0 * targetSize.x));
        }
    )";

    return out;
}

auto Shader::buildBandwidthReductionGLSL() -> std::string {

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
        #version 140

        uniform sampler2D source[];
        uniform vec4 targetSize;

        in vec2 texCoordFrag;

        out vec4 fragColor;

        void main() {
            float screenWidth = targetSize.x;
            vec3 yuv=texture(source[0], texCoordFrag + vec2(
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
            out += "    yuv += (texture(source[0], texCoordFrag + vec2(" + _doubleToStr(-pos) + ", 0.0) ).xyz "
                        "+ texture(source[0], texCoordFrag + vec2(" + _doubleToStr(pos) + ", 0.0) ).xyz) * ";
//            out += "    yuv += (texture(source[0], texCoordFrag + vec2(" + std::to_string(-i) + " / screenWidth, 0.0) ).xyz "
//                    "+ texture(source[0], texCoordFrag + vec2(" + std::to_string(i) + " / screenWidth, 0.0) ).xyz) * ";

        out += "vec3(" + _doubleToStr(luma) + "," + _doubleToStr(chromaUI) + "," + _doubleToStr(chromaVQ) + ");";

    }

    out += R"( fragColor = vec4( yuv, 1.0 ); } )";

    return out;
}

auto Shader::buildDelayLineAndConvertToRgbGLSL() -> std::string {

    std::string out = R"(
        #version 140

        uniform sampler2D source[];
        uniform float hanoverBars;
        uniform float hanoverBarsAlt;
        uniform vec4 targetSize;
		uniform int oddLine;

        in vec2 texCoordFrag;

        out vec4 fragColor;

		void main() {
	)";

    if (vManager->pal) {

        if (lace)
            out += R"(
				int lineFactor = int(floor(mod(floor(texCoordFrag.y * targetSize.y / 2.0), 2.0)));
				vec3 yuv = (texture(source[0], texCoordFrag.xy).xyz);
				vec3 yuvLineBefore = (texture(source[0], texCoordFrag.xy + vec2(0.0, -2.0 / targetSize.y )).xyz);
			)";
        else
            out += R"(
				int lineFactor = int(floor(mod(texCoordFrag.y * targetSize.y, 2.0)));
				vec3 yuv = (texture(source[0], texCoordFrag.xy).xyz);
				vec3 yuvLineBefore = (texture(source[0], texCoordFrag.xy + vec2(0.0, -1.0 / targetSize.y )).xyz);
			)";

        out += R"(
				vec2 merged = vec2(yuv.y + yuvLineBefore.y, yuv.z + yuvLineBefore.z) * mix(hanoverBars, hanoverBarsAlt, lineFactor ^ oddLine) * 0.5;
				vec3 color = vec3(yuv.x, merged.x, merged.y) * mat3(1.0,0.0,1.140251,1.0,-0.39393070,-0.58080921,1.0,2.0283976,0.0);
				fragColor = vec4(color, 1.0);
			}
			)";
    } else {

        out += R"(
			vec3 color = texture(source[0], texCoordFrag).xyz * mat3(1.0, 1.630, 0.317, 1.0, -0.378, -0.466, 1.0, -1.089, 1.677);
			fragColor = vec4(color, 1.0);
		}
		)";
    }

    return out;
}

auto Shader::buildGammaGLSL() -> std::string {

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
        #version 140

        uniform sampler2D source[];
        uniform sampler1D gamma;
        uniform vec4 targetSize;

        in vec2 texCoordFrag;

        out vec4 fragColor;

        void main() {
			vec3 color = texture(source[0], texCoordFrag).rgb;
			color.r = texture(gamma, 1.0/3.0 + color.r * 0.33203125 ).x;
			color.g = texture(gamma, 1.0/3.0 + color.g * 0.33203125 ).x;
			color.b = texture(gamma, 1.0/3.0 + color.b * 0.33203125 ).x;
			fragColor = vec4( color, 1.0 );
		}
    )";

    return out;
}

auto Shader::buildGammaAndScanlinesGLSL() -> std::string {
    std::string out = R"(
        #version 140

        uniform sampler2D source[];
        uniform sampler1D gammaWithShade;
        uniform sampler1D gamma;
        uniform vec4 targetSize;

        in vec2 texCoordFrag;

        out vec4 fragColor;

        void main() {
		    vec3 color = texture(source[0], texCoordFrag).rgb;
			vec3 colorUp = texture(source[0], texCoordFrag.xy + vec2( 0.0, -1.0 / targetSize.y ) ).rgb;
			vec3 colorDown = texture(source[0], texCoordFrag.xy + vec2( 0.0, 1.0 / targetSize.y ) ).rgb;
			int lineFactor = int(floor(mod(texCoordFrag.y * targetSize.y, 2.0)));

			color.r = mix( texture(gamma, 1.0/3.0 + color.r * 0.33203125 ).x, texture(gammaWithShade, 1.0/3.0 + 0.166015625 * colorUp.r + 0.166015625 * colorDown.r ).x, lineFactor );
			color.g = mix( texture(gamma, 1.0/3.0 + color.g * 0.33203125 ).x, texture(gammaWithShade, 1.0/3.0 + 0.166015625 * colorUp.g + 0.166015625 * colorDown.g ).x, lineFactor );
			color.b = mix( texture(gamma, 1.0/3.0 + color.b * 0.33203125 ).x, texture(gammaWithShade, 1.0/3.0 + 0.166015625 * colorUp.b + 0.166015625 * colorDown.b ).x, lineFactor );

			fragColor = vec4( color, 1.0 );
		}
    )";

    return out;
}

auto Shader::buildRadialDistortionGLSL() -> std::string {

    std::string out = R"(
        #version 140

        uniform sampler2D source[];
        uniform float Factor;
        uniform float Scale;

        in vec2 texCoordFrag;

        out vec4 fragColor;

        vec2 radialDistortion( vec2 xy ){
            vec2 center = xy - vec2(0.5, 0.5);
            float dist = dot(center,center) * Factor;
            return xy + (center * (1.0 + dist) * dist);
        }

        void main(void) {
            vec2 xy = ((radialDistortion(texCoordFrag.xy) - vec2(0.5, 0.5)) * Scale) + vec2(0.5, 0.5);

            fragColor = texture(source[0], xy);
        }
    )";

    return out;
}

auto Shader::buildMaskGLSL() -> std::string {

    std::string uniforms = "";
    std::string light = "";

    if (vManager->maskLevel)
        uniforms += R"(
            uniform sampler2D maskLayer;
            uniform float maskLevel;
            uniform float maskScaleX;
            uniform float maskScaleY;
        )";

    if (vManager->lightFromCenter)
        uniforms += R"( uniform float lightFromCenter; )";

    std::string out = R"(
        #version 140

        uniform sampler2D source[];
		uniform float luminance;
    )";

    out += uniforms;

    out += R"(
        in vec2 texCoordFrag;

        out vec4 fragColor;

        void main(void) {

            vec3 color = texture(source[0], texCoordFrag).xyz;
        )";

    if (vManager->maskLevel)
        out += "color *= mix( vec3(1.0), texture( maskLayer, texCoordFrag * vec2(maskScaleX, maskScaleY) ).xyz, maskLevel ); ";
    //out += "color = mix( vec3(1.0), texture( maskLayer, texCoordFrag * vec2(1.0) ).xyz, 1.0 );";

    if (vManager->lightFromCenter) {
        out += R"(
            vec2 lightVector = (texCoordFrag - vec2(0.5)) * lightFromCenter;
        )";

        light = "exp(-dot(lightVector, lightVector)) * ";
    }

    out += " color *= " + light + "luminance; ";

    out += R"(
        fragColor = vec4( color, 1.0 );
        }
    )";

    return out;
}

auto Shader::buildBloomGLSL( bool phase1 ) -> std::string {

    std::string _uniforms = "";

    if (!phase1) {
        _uniforms = R"(
            uniform float weight;
            uniform float glow;
        )";
    }

    GaussianBlur gB( vManager->bloomRadius << 1, vManager->bloomVariance);

    std::string out = R"(
		#version 140

		in vec2 texCoordFrag;

		uniform sampler2D source[];
    )";

    out += _uniforms;

    out += R"(
        out vec4 fragColor;

		vec3 sum = vec3(0.0, 0.0, 0.0);

		void main(void) {
	)";

    for ( int i = (vManager->bloomRadius << 1); i >= 0; i-- ) {

        if (i == 0) {
            out += " sum += texture(source[0], texCoordFrag).rgb * ";

        } else if (phase1) {
            double pos = ((double)i ) / ((double)(vManager->emulator->cropWidth() << 1));

            out += " sum += (texture(source[0], texCoordFrag + vec2(" + _doubleToStr(-pos) + ", 0.0) ).rgb "
                                                                                         "+ texture(source[0], texCoordFrag + vec2(" + _doubleToStr(pos) + ", 0.0) ).rgb) * ";

        } else {
            double pos = ((double)i ) / ((double)(vManager->emulator->cropHeight() << 1));

            out += " sum += (texture(source[0], texCoordFrag + vec2(0.0, " + _doubleToStr(-pos) + ") ).rgb "
                                                                                              "+ texture(source[0], texCoordFrag + vec2(0.0, " + _doubleToStr(pos) + ") ).rgb) * ";
        }

        out += " " + GUIKIT::String::convertDoubleToString( gB.get( i ) ) + ";";
    }

    if (phase1)
        out += R"(
			fragColor = vec4( sum.rgb, 1.0 );
			}
		)";
    else if (vManager->bloomWeight == 3.0)
        out += R"(
			fragColor = vec4(clamp(texture(source[1], texCoordFrag).rgb + ( sum.rgb * glow ), 0.0, 1.0), 1.0);
			}
		)";
    else
        out += R"(
			fragColor = vec4(clamp(texture(source[1], texCoordFrag).rgb + ( pow(sum.rgb, vec3(weight)) * glow ), 0.0, 1.0), 1.0);
			}
		)";

    return out;
}