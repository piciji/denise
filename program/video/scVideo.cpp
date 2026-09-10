
#define _USE_MATH_DEFINES
#include "scVideo.h"
#include <cmath>
#include <cstring>

SCVideo::SCVideo(VideoManager* manager) : manager( manager ) {
    evenTable = new ColorLumaChroma[manager->colorCount];
    oddTable = new ColorLumaChroma[manager->colorCount];

    if (manager->isAmiga()) {
        tempDest = new uint32_t[2048 * 600]; // shres
        std::memset(tempDest, 0, 2048 * 600 * 4);
    } else {
        tempDest = new uint32_t[1024 * 600];
        std::memset(tempDest, 0, 1024 * 600 * 4);
    }

    render.dest = nullptr;
}

SCVideo::~SCVideo() {
    delete[] evenTable;
    delete[] oddTable;
    delete[] tempDest;
}

template<typename T> auto SCVideo::renderCrt(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch, unsigned cropTop, unsigned options ) -> void {
    bool interlace = options & 3;
	render.width = width;
    render.height = height;
	render.srcPitch = srcPitch;
	render.destPitch = destPitch;
    render.options = options;
    render.fieldDest = tempDest;
	render.dest = dest ? dest : tempDest;
	render.scanlineDest = nullptr;
    render.oddLine = !cropTop ? 0x80 : ((cropTop >> interlace) & 1);
    render.src = (uint8_t*)src;
    bool pal = manager->pal;

    switch(render.options & 0x5f) {
        default:
        case 0: pal ? renderPalCrt<0, T>() : renderNtscCrt<0, T>(); break;
        case 1: pal ? renderPalCrt<1, T>() : renderNtscCrt<1, T>(); break; // Scanlines
        case 2: pal ? renderPalCrt<2, T>() : renderNtscCrt<2, T>(); break; // RF
        case 3: pal ? renderPalCrt<3, T>() : renderNtscCrt<3, T>(); break; // Scanlines + RF
        case 4: pal ? renderPalCrt<4, T>() : renderNtscCrt<4, T>(); break; // Interlace
        case 6: pal ? renderPalCrt<6, T>() : renderNtscCrt<6, T>(); break; // Interlace + RF
        case 12: pal ? renderPalCrt<12, T>() : renderNtscCrt<12, T>(); break; // Interlace other field
        case 14: pal ? renderPalCrt<14, T>() : renderNtscCrt<14, T>(); break; // Interlace other field + RF

        case 20: pal ? renderPalCrt<20, T>() : renderNtscCrt<20, T>(); break; // Interlace(Hold)
        case 22: pal ? renderPalCrt<22, T>() : renderNtscCrt<22, T>(); break; // Interlace(Hold) + RF
        case 28: pal ? renderPalCrt<28, T>() : renderNtscCrt<28, T>(); break; // Interlace(Hold) other field
        case 30: pal ? renderPalCrt<30, T>() : renderNtscCrt<30, T>(); break; // Interlace(Hold) other field + RF

        case 68: pal ? renderPalCrt<68, T>() : renderNtscCrt<68, T>(); break; // Interlace toggle
        case 70: pal ? renderPalCrt<70, T>() : renderNtscCrt<70, T>(); break; // Interlace toggle + RF
        // other combinations don't make sense, like Scanlines + Interlace
    }
}

template<uint8_t options, typename T> auto SCVideo::renderPalCrt( ) -> void {

	constexpr static int32_t x1 = (int32_t) ((1.0 / 0.493) * double(1 << 8) + 0.5);
    constexpr static int32_t x2 = (int32_t) ((1.0 / 0.877) * double(1 << 8) + 0.5);
    constexpr static int32_t x3 = (int32_t) (0.3939307027516405140450117660881 * double(1 << 8) + 0.5);
    constexpr static int32_t x4 = (int32_t) (0.58080920903109757400461150856936 * double(1 << 8) + 0.5);

    constexpr bool withScanlines = options & 1;
    constexpr bool lumaDelay = options & 2;
    constexpr bool interlace = options & 4;
    constexpr bool field = options & 8;
    constexpr bool iHold = options & 16;
    constexpr bool laceToggle = options & 64;

    Render& re = render;

    unsigned iRate = (100 - manager->interlaceDecay);

	const T* _src = (T*)re.src;

	ColorLumaChroma* lineTable;
	ColorRgb* lineBeforeDest = nullptr;
	ColorLumaChroma yuv;
    RGBDescriptor colorI;
	int32_t uSubSample, vSubSample;
    unsigned mask = (1 << manager->countColorBits) - 1;
    int32_t hanoverBars = manager->hanoverBars;
    int32_t hanoverBarsAlt = manager->hanoverBarsAlt;

	_src -= 2;
    const T* srcDelay = _src;

    if (re.oddLine & 0x80) { // there is no line before, so we reuse this line for delay line
        re.oddLine = 0;
        if (interlace && field)
            srcDelay += re.width + re.srcPitch;
    } else {
        srcDelay -= re.width + re.srcPitch;
        if (interlace)
            srcDelay -= re.width + re.srcPitch;
    }

    lineTable = !re.oddLine ? oddTable : evenTable;

	uSubSample = lineTable[ srcDelay[0] & mask ].u_i_s + lineTable[ srcDelay[1] & mask ].u_i_s + lineTable[ srcDelay[2] & mask ].u_i_s;
	vSubSample = lineTable[ srcDelay[0] & mask ].v_q_s + lineTable[ srcDelay[1] & mask ].v_q_s + lineTable[ srcDelay[2] & mask ].v_q_s;

	// delay line
	for (unsigned w = 0; w < re.width; w++) {
		uSubSample += lineTable[ srcDelay[3] & mask ].u_i_s;
		vSubSample += lineTable[ srcDelay[3] & mask ].v_q_s;

		delayLine[w].u_i_s = uSubSample;
		delayLine[w].v_q_s = vSubSample;

		uSubSample -= lineTable[ srcDelay[0] & mask ].u_i_s;
		vSubSample -= lineTable[ srcDelay[0] & mask ].v_q_s;

        srcDelay++;
	}

	for(unsigned h = 0; h < re.height; h++) {

        if (interlace && !laceToggle && ((!field && (h & 1)) || (field && !(h & 1)))) {
            if (!iHold || field) {
                if (re.fieldDest) {
                    std::memcpy(re.dest, re.fieldDest, re.width * 4);
                    re.fieldDest += re.width;
                }
            }
            _src += re.width;
            re.dest += re.width;

        } else {
            lineTable = re.oddLine ? oddTable : evenTable;
            lineBeforeDest = &lineBefore[0];

            uSubSample = lineTable[ _src[0] & mask ].u_i_s + lineTable[ _src[1] & mask ].u_i_s + lineTable[ _src[2] & mask ].u_i_s;
            vSubSample = lineTable[ _src[0] & mask ].v_q_s + lineTable[ _src[1] & mask ].v_q_s + lineTable[ _src[2] & mask ].v_q_s;

            for(unsigned w = 0; w < re.width; w++) {

                uSubSample += lineTable[ _src[3] & mask ].u_i_s;
                vSubSample += lineTable[ _src[3] & mask ].v_q_s;

                yuv.u_i_s = uSubSample + delayLine[w].u_i_s;
                yuv.v_q_s = vSubSample + delayLine[w].v_q_s;

                if (!lumaDelay)
                    yuv.y_s = lineTable[ _src[1] & mask ].y_s_blur + lineTable[ _src[2] & mask ].y_s + lineTable[ _src[3] & mask ].y_s_blur;

                else {
                    uint16_t _pos = ((_src[-1] & mask) << 12) | ((_src[0] & mask) << 8) | ((_src[1] & mask) << 4) | (_src[2] & mask);
                    uint16_t _posL = ((_src[-2] & mask) << 12) | ((_src[-1] & mask) << 8) | ((_src[0] & mask) << 4) | (_src[1] & mask);
                    uint16_t _posR = ((_src[0] & mask) << 12) | ((_src[1] & mask) << 8) | ((_src[2] & mask) << 4) | (_src[3] & mask);

                    yuv.y_s = preCalcLumaNeighbour[_posL] + preCalcLumaCenter[_pos] + preCalcLumaNeighbour[_posR];
                }

                delayLine[w].u_i_s = uSubSample;
                delayLine[w].v_q_s = vSubSample;

                if (re.oddLine) {
                    yuv.u_i_s = (yuv.u_i_s * hanoverBars) >> 7;
                    yuv.v_q_s = (yuv.v_q_s * hanoverBars) >> 7;

                } else if (hanoverBarsAlt) {
                    yuv.u_i_s = (yuv.u_i_s * hanoverBarsAlt) >> 7;
                    yuv.v_q_s = (yuv.v_q_s * hanoverBarsAlt) >> 7;
                }

                int16_t r = (yuv.y_s + ((x2 * yuv.v_q_s) >> 8) + 1024) >> 11;
                int16_t g = (yuv.y_s - ((x3 * yuv.u_i_s + x4 * yuv.v_q_s) >> 8) + 1024) >> 11;
                int16_t b = (yuv.y_s + ((x1 * yuv.u_i_s) >> 8) + 1024) >> 11;

                if constexpr (interlace) {
                    colorI.r = preCalcGamma[ r + 256 ];
                    colorI.g = preCalcGamma[ g + 256 ];
                    colorI.b = preCalcGamma[ b + 256 ];
                    *re.dest++ = 255 << 24 | colorI.r << 16 | colorI.g << 8 | colorI.b;

                    if constexpr(laceToggle)
                        *re.fieldDest++ = 255 << 24 | colorI.r << 16 | colorI.g << 8 | colorI.b;

                    else if constexpr(!iHold) {
                        colorI.r = (colorI.r * iRate) / 100;
                        colorI.g = (colorI.g * iRate) / 100;
                        colorI.b = (colorI.b * iRate) / 100;
                        *re.fieldDest++ = 255 << 24 | colorI.r << 16 | colorI.g << 8 | colorI.b;
                    }
                } else
                    *re.dest++ = 255 << 24 | preCalcGamma[ r + 256 ] << 16 | preCalcGamma[ g + 256 ] << 8 | preCalcGamma[ b + 256 ];

                if constexpr (withScanlines) {
                    if (re.scanlineDest) {
                        *re.scanlineDest++ = 255 << 24 | preCalcScanline[r + lineBeforeDest->rInt + 512] << 16
                                              | preCalcScanline[g + lineBeforeDest->gInt + 512] << 8
                                              | preCalcScanline[b + lineBeforeDest->bInt + 512];
                    }

                    lineBeforeDest->rInt = r;
                    lineBeforeDest->gInt = g;
                    lineBeforeDest->bInt = b;
                    lineBeforeDest++;
                }

                uSubSample -= lineTable[ _src[0] & mask ].u_i_s;
                vSubSample -= lineTable[ _src[0] & mask ].v_q_s;

                _src++;
            }

            if constexpr (interlace && iHold)
                re.fieldDest += re.width;

            re.oddLine ^= 1;
		}
		_src += re.srcPitch;
		re.dest += re.destPitch;

        if constexpr (interlace) {
            re.fieldDest += re.destPitch;

        } else if constexpr (withScanlines) {
			re.scanlineDest = re.dest;
			re.dest +=	re.width + re.destPitch;
		}
	}

    if constexpr (withScanlines) {
        lineBeforeDest = &lineBefore[0];
        for (unsigned w = 0; w < re.width; w++) {
            *re.scanlineDest++ = 255 << 24 | preCalcScanline[(lineBeforeDest->rInt << 1) + 512] << 16
                                 | preCalcScanline[(lineBeforeDest->gInt << 1) + 512] << 8
                                 | preCalcScanline[(lineBeforeDest->bInt << 1) + 512];

            lineBeforeDest++;
        }
    }

	re.src = (uint8_t*)_src;
}

template<uint8_t options, typename T> auto SCVideo::renderNtscCrt( ) -> void {

	constexpr static int32_t x1 = static_cast<int32_t>(1.630 * double( 1 << 8 ) + 0.5);
    constexpr static int32_t x2 = static_cast<int32_t>(0.317 * double( 1 << 8 ) + 0.5);
    constexpr static int32_t x3 = static_cast<int32_t>(0.378 * double( 1 << 8 ) + 0.5);
    constexpr static int32_t x4 = static_cast<int32_t>(0.466 * double( 1 << 8 ) + 0.5);
	constexpr static int32_t x5 = static_cast<int32_t>(1.089 * double( 1 << 8 ) + 0.5);
	constexpr static int32_t x6 = static_cast<int32_t>(1.677 * double( 1 << 8 ) + 0.5);

    constexpr bool withScanlines = options & 1;
    constexpr bool lumaDelay = options & 2;
    constexpr bool interlace = options & 4;
    constexpr bool field = options & 8;
    constexpr bool iHold = options & 16;
    constexpr bool laceToggle = options & 64;

    Render& re = render;
	ColorLumaChroma yiq;
    RGBDescriptor colorI;
	ColorRgb* lineBeforeDest = nullptr;
    unsigned mask = (1 << manager->countColorBits) - 1;

	int32_t iSubSample, qSubSample;
    unsigned iRate = (100 - manager->interlaceDecay);
	const T* _src = (T*)re.src;
    _src -= 2;

	for(unsigned h = 0; h < re.height; h++) {

        if (interlace && !laceToggle && ((!field && (h & 1)) || (field && !(h & 1)))) {
            if (!iHold || field) {
                std::memcpy(re.dest, re.fieldDest, re.width * 4);
                re.fieldDest += re.width;
            }
            _src += re.width;
            re.dest += re.width;

        } else {
            iSubSample = evenTable[ _src[0] & mask ].u_i_s + evenTable[ _src[1] & mask ].u_i_s + evenTable[ _src[2] & mask ].u_i_s;
            qSubSample = evenTable[ _src[0] & mask ].v_q_s + evenTable[ _src[1] & mask ].v_q_s + evenTable[ _src[2] & mask ].v_q_s;

            lineBeforeDest = &lineBefore[0];

            for(unsigned w = 0; w < re.width; w++) {

                iSubSample += evenTable[ _src[3] & mask ].u_i_s;
                qSubSample += evenTable[ _src[3] & mask ].v_q_s;

                yiq.u_i_s = iSubSample;
                yiq.v_q_s = qSubSample;

                if (!lumaDelay)
                    yiq.y_s = evenTable[ _src[1] & mask ].y_s_blur + evenTable[ _src[2] & mask ].y_s + evenTable[ _src[3] & mask ].y_s_blur;
                else {
                    uint16_t _pos = ((_src[-1] & mask) << 12) | ((_src[0] & mask) << 8) | ((_src[1] & mask) << 4) | (_src[2] & mask);
                    uint16_t _posL = ((_src[-2] & mask) << 12) | ((_src[-1] & mask) << 8) | ((_src[0] & mask) << 4) | (_src[1] & mask);
                    uint16_t _posR = ((_src[0] & mask) << 12) | ((_src[1] & mask) << 8) | ((_src[2] & mask) << 4) | (_src[3] & mask);

                    yiq.y_s = preCalcLumaNeighbour[_posL] + preCalcLumaCenter[_pos] + preCalcLumaNeighbour[_posR];
                }

                int16_t r = (yiq.y_s + ((x1 * yiq.u_i_s + x2 * yiq.v_q_s) >> 8) + 512) >> 10;
                int16_t g = (yiq.y_s - ((x3 * yiq.u_i_s + x4 * yiq.v_q_s) >> 8) + 512) >> 10;
                int16_t b = (yiq.y_s - ((x5 * yiq.u_i_s - x6 * yiq.v_q_s) >> 8) + 512) >> 10;

                if constexpr (interlace) {
                    colorI.r = preCalcGamma[ r + 256 ];
                    colorI.g = preCalcGamma[ g + 256 ];
                    colorI.b = preCalcGamma[ b + 256 ];
                    *re.dest++ = 255 << 24 | colorI.r << 16 | colorI.g << 8 | colorI.b;

                    if constexpr(laceToggle)
                        *re.fieldDest++ = 255 << 24 | colorI.r << 16 | colorI.g << 8 | colorI.b;

                    else if constexpr (!iHold) {
                        colorI.r = (colorI.r * iRate) / 100;
                        colorI.g = (colorI.g * iRate) / 100;
                        colorI.b = (colorI.b * iRate) / 100;
                        *re.fieldDest++ = 255 << 24 | colorI.r << 16 | colorI.g << 8 | colorI.b;
                    }
                } else
                    *re.dest++ = 255 << 24 | preCalcGamma[ r + 256 ] << 16 | preCalcGamma[ g + 256 ] << 8 | preCalcGamma[ b + 256 ];

                if constexpr (withScanlines) {
                    if ( re.scanlineDest) {
                        *re.scanlineDest++ = 255 << 24 | preCalcScanline[r + lineBeforeDest->rInt + 512] << 16
                              | preCalcScanline[g + lineBeforeDest->gInt + 512] << 8
                              | preCalcScanline[b + lineBeforeDest->bInt + 512];
                    }

                    lineBeforeDest->rInt = r;
                    lineBeforeDest->gInt = g;
                    lineBeforeDest->bInt = b;
                    lineBeforeDest++;
                }

                iSubSample -= evenTable[ _src[0] & mask ].u_i_s;
                qSubSample -= evenTable[ _src[0] & mask ].v_q_s;

                _src++;
            }

            if constexpr (interlace && iHold)
                re.fieldDest += re.width;
        }

		_src += re.srcPitch;
		re.dest += re.destPitch;

        if constexpr (interlace) {
            re.fieldDest += re.destPitch;

        } else if constexpr (withScanlines) {
            re.scanlineDest = re.dest;
			re.dest +=	re.width + re.destPitch;
		}
	}

    if constexpr (withScanlines) {
        lineBeforeDest = &lineBefore[0];
        for (unsigned w = 0; w < re.width; w++) {
            *re.scanlineDest++ = 255 << 24 | preCalcScanline[(lineBeforeDest->rInt << 1) + 512] << 16
                                 | preCalcScanline[(lineBeforeDest->gInt << 1) + 512] << 8
                                 | preCalcScanline[(lineBeforeDest->bInt << 1) + 512];

            lineBeforeDest++;
        }
    }

	re.src = (uint8_t*)_src;
}

auto SCVideo::update() -> void {
    calculateGamma();

    if ((manager->countColorBits == 4) && manager->useLumaDelay())
        calculateLumaDelay();

    injectPhaseTransferError();
    convertLumaChromaToInteger();
}

auto SCVideo::calculateGamma() -> void {
    double scanlineShade = 1.0 - (double)manager->scanlines / 100.0;

    // we precalculate the requested adjustments for each color state.
    // besides, we apply adjustments on out-of-range color values to
    // use this later on intermediate values for more precise final results
    for(int c = 0; c < (256 * 3); c++) {

        double c1 = (double)(c - 256);

        if (manager->pal && (manager->colorSpectrum == 2))
            VideoManager::normalizeColorSpectrumPalGamma( c1 );

        manager->adjustGamma( c1 );

        // make sure the final color channel is integer with a precision of 8 bit
        preCalcGamma[c] = VideoManager::uclamp8( c1 );

        // to emulate scanlines with a given intensity, the colors of the
        // adjacent lines will be blended together with reduced luminance.
        // that's why we precalculate the result of mixing two colors too.
        // formula: (a + b) / 2
        // if there is no shade, the scanline is black and fully visible.
        // otherwise, the original mixed color will be visible.
        preCalcScanline[c * 2] = VideoManager::uclamp8( c1 * scanlineShade );

        // the mixing formula above could produce uneven results: x.5
        // the color channel can be an integer value only, but
        // applying adjustments on a virtual fractional value results in more
        // precise final values.
        c1 = (double)((c - 256) + 0.5);

        if (manager->pal && (manager->colorSpectrum == 2))
            VideoManager::normalizeColorSpectrumPalGamma(c1);

        manager->adjustGamma( c1 );

        preCalcScanline[c * 2 + 1] = VideoManager::uclamp8( c1 * scanlineShade );
    }
}

auto SCVideo::calculateLumaDelay() -> void {
    double yStep[4];
    double y;
    double yNext;
    double diff;
    bool stepChange;
    int direction;

    double _lumaRise = manager->lumaRise == 0.0 ? 1.0 : manager->lumaRise;
    double _lumaFall = manager->lumaFall == 0.0 ? 1.0 : manager->lumaFall;

    double neighbour = manager->blur * 0.25;
    double center = 1.0 - neighbour * 2.0;
    unsigned scalerLuma = manager->pal ? 11 : 10;

    // calculate all combinations for 4 adjacent pixel
    for (unsigned j = 0; j <= 0xffff; j++ ) {

        uint8_t p3 = j & 0xf;
        uint8_t p2 = (j >> 4) & 0xf;
        uint8_t p1 = (j >> 8) & 0xf;
        uint8_t p0 = (j >> 12) & 0xf;

        yStep[0] = manager->lumaChromaTable[p0].y;
        yStep[1] = manager->lumaChromaTable[p1].y;
        yStep[2] = manager->lumaChromaTable[p2].y;
        yStep[3] = manager->lumaChromaTable[p3].y;

        diff = 0.0;
        y = yStep[0];

        for (unsigned i = 0; i < 3; i++) {

            yNext = yStep[i+1];

            stepChange = yStep[i] != yNext;

            double stepDiff = yNext - y;

            if (stepChange)
                diff = stepDiff;

            direction = (stepDiff < 0.0) ? -1 : ( (stepDiff > 0.0) ? 1 : 0 );

            if (direction == 1)
                // don't rise higher than real value
                    y = std::min( y + (diff * _lumaRise), yNext );
            else if (direction == -1)
                // don't fall lower than real value
                    y = std::max( y + (diff * _lumaFall), yNext );
        }

        preCalcLumaCenter[j] = (int32_t) (y * center * double(1 << scalerLuma) + (y < 0 ? -0.5 : 0.5));
        preCalcLumaNeighbour[j] = (int32_t) (y * neighbour * double(1 << scalerLuma) + (y < 0 ? -0.5 : 0.5));
    }
}

auto SCVideo::injectPhaseTransferError() -> void {
    // Composite/S-Video transfers PAL's V component 180° phase shifted each odd line.
    // receiver translates it back by changing the sign of V component again.
    // if there is no phase error during transmission, this process would be useless.
    // but the real world is not ideal.
    // to explain this, let's have a look at what's the problem with NTSC.
    // a sender adds some degree of HUE (V component) errors while transmitting the signal.
    // on each NTSC TV there is a setting called tint.
    // this way you can correct a great portion of HUE error of a specific sender.
    // of course the HUE error isn't constant, means you can't correct it completely.
    // that's why NTSC is called: never the same color
    // back to PAL and the approach to stabilize HUE
    // even lines: same transmission as NTSC, the transferred signal is received with an unknown phase shift error.
    // in comparison to NTSC the information is stored in TV (delay line)
    // odd lines: V component is phase shifted by 180° before transmission. (simply means the V value change sign).
    // the transmission adds a similar phase shift error within such a short time of one display line.
    // Receiver (TV) reverses sign of V component, which includes a similar phase shift error like happened in even line.
    // Now the receiver already has the U/V data of the even line, which is shifted by an unknown phase error from the real U/V data.
    // And there is the U/V data of the odd line that is shifted by a similar phase error but in the other
    // direction of the even line data. by calculating the average between odd and even data one get the original value.
    // a tint correction like NTSC would be unnecessary.
    // The HUE error is a lot smaller than NTSC, because the error doesn't shift that much in the short period of a scanline.
    // there are some disadvantages of this approach.
    // 1. the vertical resolution is halved, because in PAL 2 lines will be mixed.
    // 2. if sender adds a huge phase shift error to U/V, the receiver can indeed reconstruct the original HUE
    // by averaging data from even and odd lines, but the overall saturation will be lowered.
    // delay lines will be saved in an analog way. depending on TV quality, adjacent lines are desaturated differently.
	// this effect is known as (Hanover Bars)

	// U = cos( angle ) * modifier( contrast and/or saturation )
	// V = sin( angle ) * modifier( contrast and/or saturation )
	// i.e. sin( a + b ) = sin a cos b + cos a sin b
	// i.e. sin( a - b ) = sin a cos b - cos a sin b
	// a is the original phase angle
	// b is the phase error
	// the modifier for contrast/saturation is already applied on U/V
	// i.e. sin( a + b ) * modifier = modifier * (sin a cos b + cos a sin b)
	//								= modifier * sin a cos b + modifier * cos a sin b
	// we have already calculated: V = modifier * sin a , U = modifier * cos a
	// after substitution we get:
	//								= V * cos b + U * sin b

    double rotU = std::cos( manager->phaseError * M_PI / 180.0 );	// cos b
    double rotV = std::sin( manager->phaseError * M_PI / 180.0 );	// sin b

    for (unsigned c = 0; c < manager->colorCount; c++) {
        ColorLumaChroma* lumaChroma = &manager->lumaChromaTable[c];

		evenTable[c].y = lumaChroma->y;
		// phase shifted chroma, received in TV (PAL, NTSC)
		evenTable[c].u_i = lumaChroma->u_i * rotU - lumaChroma->v_q * rotV;
		evenTable[c].v_q = lumaChroma->v_q * rotU + lumaChroma->u_i * rotV;

		if (manager->pal) {
			// same phase error but shifted in the opposite direction (PAL)
			oddTable[c].y = lumaChroma->y;
			oddTable[c].u_i = lumaChroma->uOdd * rotU - lumaChroma->vOdd * rotV * -1;
			oddTable[c].v_q = lumaChroma->vOdd * rotU + lumaChroma->uOdd * rotV * -1;
		}
    }
}

auto SCVideo::convertLumaChromaToInteger() -> void {
	// luma changes overshoot then continue to oscillate a few pixels.
    // i.e. a blur of 1 means the luma of both neighboring pixels are weighted with 25% each
    // and the center pixel is weighted with 50 %
    double neighbour = manager->blur * 0.25;
    double center = 1.0 - neighbour * 2.0;
    unsigned scalerLuma = manager->pal ? 11 : 10;

	for (unsigned c = 0; c < manager->colorCount; c++) {

        // for performance reasons, we want to calculate the crt frame with integer later on.
        // we need to scale up with at least 8 bits to reconvert back to RGB lossless.
		evenTable[c].u_i_s = (int32_t) (evenTable[c].u_i * double(1 << 8) + (evenTable[c].u_i < 0 ? -0.5 : 0.5));
		evenTable[c].v_q_s = (int32_t) (evenTable[c].v_q * double(1 << 8) + (evenTable[c].v_q < 0 ? -0.5 : 0.5));

        // we scale up with 11 bits instead of 8
        // after chroma subsampling (adding 4 pixels), the scaling of chroma increases to 10 bits,
        // and after adding the final pixel to delayed pixel the scaling of chroma increases to 11 bits.
        // chroma and luma need to be scaled the same to apply math on it.
        // average color (horizontal) = (pixel1 + pixel2 + pixel3 + pixel4) / 4
        // average color (vertical)   = (average color (horizontal) + delayed average color of last line) / 2
        //
        // pFinal = ((p1 + p2 + p3 + p4) / 4 + (pLast1 + pLast2 + pLast3 + pLast4) / 4 ) / 2
        // pFinal * 2 = (p1 + p2 + p3 + p4) / 4 + (pLast1 + pLast2 + pLast3 + pLast4) / 4
        // pFinal * 8 = p1 + p2 + p3 + p4 + pLast1 + pLast2 + pLast3 + pLast4

        // so we can add all 8 pixels first and divide later on by 8 to get the averaged pixel.
        // for NTSC there isn't a delay line. we scale up 2 bits only.

        evenTable[c].y_s = (int32_t) (evenTable[c].y * center * double(1 << scalerLuma) + (evenTable[c].y < 0 ? -0.5 : 0.5));
        evenTable[c].y_s_blur = (int32_t) (evenTable[c].y * neighbour * double(1 << scalerLuma) + (evenTable[c].y < 0 ? -0.5 : 0.5));

		if (manager->pal) {
			oddTable[c].y_s = evenTable[c].y_s;
			oddTable[c].y_s_blur = evenTable[c].y_s_blur;
			oddTable[c].u_i_s = (int32_t) (oddTable[c].u_i * double(1 << 8) + (oddTable[c].u_i < 0 ? -0.5 : 0.5));
			oddTable[c].v_q_s = (int32_t) (oddTable[c].v_q * double(1 << 8) + (oddTable[c].v_q < 0 ? -0.5 : 0.5));
		}
	}
}

template auto SCVideo::renderCrt<uint8_t>(unsigned width, unsigned height, const uint8_t* src, unsigned srcPitch, unsigned* dest, unsigned destPitch, unsigned cropTop, unsigned options ) -> void;
template auto SCVideo::renderCrt<uint16_t>(unsigned width, unsigned height, const uint16_t* src, unsigned srcPitch, unsigned* dest, unsigned destPitch, unsigned cropTop, unsigned options ) -> void;