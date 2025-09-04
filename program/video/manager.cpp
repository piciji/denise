
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdlib>
#include <cstring>
#include "manager.h"
#include "../../guikit/api.h"
#include "../../emulation/libc64/interface.h"
#include "../emuconfig/config.h"
#include "../media/media.h"
#include "../view/view.h"
#include "../thread/emuThread.h"
#include "shaderParser.h"
#include "props.cpp"
#include "sync.cpp"
#include "../tools/dataStorage.h"
#include "../emuconfig/layouts/presentation.h"

#include "../tools/chronos.h"
#include "../view/status.h"

bool VideoManager::synchronized = true;
uint8_t VideoManager::frameRenderPos = 0;
uint8_t VideoManager::frameRenderTrigger = 1;
unsigned VideoManager::placeHolderFrames = 0;
bool VideoManager::placeHolderSplashScreen = false;
bool VideoManager::needAUpdate = true;
unsigned VideoManager::takeScreenShots = 0;

std::vector<VideoManager*> videoManagers;

#define SHADER_OFFSCREEN_WIDTH 4

auto VideoManager::getInstance( Emulator::Interface* emulator ) -> VideoManager* {
	
	for (auto videoManager : videoManagers) {
		if (videoManager->emulator == emulator)
			return videoManager;
	}
    
	return nullptr;
}

auto VideoManager::updateAll() -> void {
    for (auto videoManager: videoManagers) {
        if (videoManager->dataUpdatesPending)
            videoManager->applyDataUpdates();

        if (videoManager->needUpdate())
            videoManager->update();
    }
    needAUpdate = false;
}

auto VideoManager::unloadDataStorage() -> void {
    ShaderParser::dataStorage->unload();
}

auto VideoManager::setFrameRender(uint8_t limit) -> void {
    frameRenderTrigger = limit;
    frameRenderPos = 0;
}

VideoManager::VideoManager(Emulator::Interface* emulator) {
    this->emulator = emulator;
    this->settings = program->getSettings(emulator);
    this->palette = &emulator->palettes[0];        
    this->colorCount = this->palette->paletteColors.size();        

    countColorBits = 0;
    workerCreated = false;
	
	if (isC64()) {
        countColorBits = 4;
        softwareViewForegroundColorRef = 14;
        softwareViewBackgroundColorRef = 6;
		
	} else if (isAmiga()) {
        countColorBits = 12;
        softwareViewForegroundColorRef = 4095;
        softwareViewBackgroundColorRef = 90;
	}

    gamma = 1.0;
    contrast = 1.0;
    brightness = 1.0;
	saturation = 1.0;    

    colorTableUpdated = false;
    dataUpdatesPending = false;

    lumaChromaTable = new ColorLumaChroma[this->colorCount];
    evenTable = new ColorLumaChroma[this->colorCount];
    oddTable = new ColorLumaChroma[this->colorCount];
    colorTable = new uint32_t[this->colorCount];
    
    newLuma = true;
    crtRealGamma = false;
	phase = 0.0;
	pal = true;
    colorSpectrum = false;  
    crtMode = CrtMode::None;
    scanlines = 0;

    phaseError = 22.5; 
    hanoverBars = (int32_t)(0.8 * 128.0); // 20% saturation loss
    hanoverBarsAlt = 0;

    currentHeight = 0;
	
    if (isAmiga())
	    tempDest = new uint32_t[2048 * 600]; // shres
    else
        tempDest = new uint32_t[1024 * 600];
	
	lumaRise = 1.0 / 2.0;
	lumaFall = 1.0 / 1.2;

    for(unsigned i = 0; i < 2; i++) {
        render[i].kill = false;
        render[i].ready = false;
        render[i].dest = nullptr;
    }

    reinitCrtThread(true);
    parser = new ShaderParser;
}

auto VideoManager::update() -> void {

    if (!colorSpectrum || !isC64() ) { // palette
        if ( (crtMode == CrtMode::Cpu) || ( (crtMode == CrtMode::Gpu) && shaderLumaChromaInput() ) ) {
            convertPaletteToLumaChroma();

            convertLumaChromaToRGB();
        } else
            adjustPalette();
        
    } else {
        generateC64ColorSpectrum();
		
        convertLumaChromaToRGB();
    }

    if ( crtMode == CrtMode::Cpu ) {
        calculateGamma();

        if ((countColorBits == 4) && useLumaDelay())
            calculateLumaDelay();

        injectPhaseTransferError();
        convertLumaChromaToInteger();
    }
    
    updateListingColors();
    
    colorTableUpdated = true;    
}

auto VideoManager::updateListingColors() -> void {
    emuThread->lockPaletteForSoftwareView();
    unsigned softwareViewForegroundColor = colorTable[softwareViewForegroundColorRef];
    unsigned softwareViewBackgroundColor = colorTable[softwareViewBackgroundColorRef];
    emuThread->unlockPaletteForSoftwareView();

    if (emuThread->enabled) {
        emuThread->events |= EmuThread::EVT_UPDATE_PALETTE_SOFTWARE;
        return;
    }

    auto emuView = EmuConfigView::TabWindow::getView( emulator );

    if (emuView && emuView->mediaLayout)
        emuView->mediaLayout->colorListing( softwareViewForegroundColor, softwareViewBackgroundColor );
}

auto VideoManager::getForegroundColor() -> unsigned {
    emuThread->lockPaletteForSoftwareView();
    unsigned _color = colorTable[softwareViewForegroundColorRef];
    emuThread->unlockPaletteForSoftwareView();
    return _color;
}

auto VideoManager::getBackgroundColor() -> unsigned {
    emuThread->lockPaletteForSoftwareView();
    unsigned _color = colorTable[softwareViewBackgroundColorRef];
    emuThread->unlockPaletteForSoftwareView();
    return _color;
}

auto VideoManager::generateC64ColorSpectrum() -> void {
    
    LIBC64::Interface* c64Emu = dynamic_cast<LIBC64::Interface*>(emulator);
    
    static double radian = M_PI / 180.0;
    static double baseSaturation = 40.0;
	static double baseContrast = 1.2;
	
	double sat = baseSaturation * saturation;
	double con = baseContrast * contrast;	
	
    double angle;
    
    for (unsigned c = 0; c < colorCount; c++) {
              
		ColorLumaChroma* lumaChroma = &lumaChromaTable[c];
		
        double luma = c64Emu->getLuma( c, newLuma );                

        lumaChroma->y = ( luma + brightness ) * con;
        lumaChroma->y_n = float(lumaChroma->y / 255.0);
        
        lumaChroma->u_i = lumaChroma->v_q = 0.0;
        lumaChroma->u_i_n = lumaChroma->v_q_n = 0.0;
        
        double chroma = c64Emu->getChroma( c );
        
        if (chroma == 0.0)
            continue; // luma only ... black, grey shades
        
		if (pal) {
			angle = (chroma + phase ) * radian;
            lumaChroma->u_i = (std::cos(angle) * sat) * con;
            lumaChroma->v_q = (std::sin(angle) * sat) * con;                    

		} else {
			// yiq is 33 degree rotated
            angle = (chroma + phase - (100.0 / 3.0) ) * radian;
            lumaChroma->u_i = (std::sin(angle) * sat) * con;
            lumaChroma->v_q = (std::cos(angle) * sat) * con;                    
        }
        
        lumaChroma->u_i_n = float(lumaChroma->u_i / 255.0);
        lumaChroma->v_q_n = float(lumaChroma->v_q / 255.0);
    }
}

auto VideoManager::normalizeColorSpectrumPalGamma( double& color ) -> void {
	
	color = std::pow(255, 1 - 2.8) * std::pow(color, 2.8);
	
	color = std::pow(255, 1 - (1.0 / 2.2)) * std::pow(color, 1.0 / 2.2 );
}

auto VideoManager::denormalizeColorSpectrumPalGamma( double& color ) -> void {
	
	color = std::pow(255, 1 - 2.2) * std::pow(color, 2.2);
	
	color = std::pow(255, 1 - (1.0 / 2.8)) * std::pow(color, 1.0 / 2.8 );
}

auto VideoManager::adjustPalette() -> void {
	
	ColorRgb rgb;
    
    for (unsigned c = 0; c < colorCount; c++) {
        
        auto& paletteColor = palette->paletteColors[c];
        
        rgb.r = paletteColor.r;
        rgb.g = paletteColor.g;
        rgb.b = paletteColor.b;
        
        adjustSaturation( rgb.r, rgb.g, rgb.b );        
        adjustBrightness(rgb.r); adjustBrightness(rgb.g); adjustBrightness(rgb.b);
        adjustContrast(rgb.r); adjustContrast(rgb.g); adjustContrast(rgb.b);
        adjustGamma(rgb.r); adjustGamma(rgb.g); adjustGamma(rgb.b);
        
        colorTable[c] = 255 << 24 | uclamp8( rgb.r ) << 16 | uclamp8( rgb.g ) << 8 | uclamp8( rgb.b );
    }
}

auto VideoManager::convertPaletteToLumaChroma() -> void {
	ColorRgb rgb;
	
    for (unsigned c = 0; c < colorCount; c++) {
        
        auto& paletteColor = palette->paletteColors[c];
        
        rgb.r = paletteColor.r;
        rgb.g = paletteColor.g;
        rgb.b = paletteColor.b;
                
        if (pal && crtRealGamma) {
            denormalizeColorSpectrumPalGamma(rgb.r);
            denormalizeColorSpectrumPalGamma(rgb.g);
            denormalizeColorSpectrumPalGamma(rgb.b);
        }

		ColorLumaChroma* lumaChroma = &lumaChromaTable[c];
		
		if (pal)
			convertRGBToYUV( lumaChroma, &rgb);
		else
			convertRGBToYIQ( lumaChroma, &rgb);
		
		lumaChroma->u_i = (lumaChroma->u_i * saturation) * contrast;
		lumaChroma->v_q = (lumaChroma->v_q * saturation) * contrast;		
		lumaChroma->y = (lumaChroma->y + brightness) * contrast;
		
		// for shader
        lumaChroma->y_n = float(lumaChroma->y / 255.0);
        lumaChroma->u_i_n = float(lumaChroma->u_i / 255.0);
        lumaChroma->v_q_n = float(lumaChroma->v_q / 255.0);
	}
}

auto VideoManager::convertLumaChromaToRGB() -> void {
	
    ColorRgb rgb;     	
	
    for (unsigned c = 0; c < colorCount; c++) {
        
        if (pal)
            convertYUVToRGB( &rgb, &lumaChromaTable[c] );
        else
			convertYIQToRGB( &rgb, &lumaChromaTable[c] );

		//if (pal && colorSpectrum) {
        if (pal && (colorSpectrum || crtRealGamma)) {
			normalizeColorSpectrumPalGamma(rgb.r);
			normalizeColorSpectrumPalGamma(rgb.g);
			normalizeColorSpectrumPalGamma(rgb.b);
		}

        adjustGamma(rgb.r);
        adjustGamma(rgb.g);
        adjustGamma(rgb.b);
		
		colorTable[c] = 255 << 24 | uclamp8( rgb.r ) << 16 | uclamp8( rgb.g ) << 8 | uclamp8( rgb.b );
    }
}

auto VideoManager::calculateGamma() -> void {
    double scanlineShade = 1.0 - (double)scanlines / 100.0;
    
	// we precalculate the requested adjustments for each color state.
	// besides, we apply adjustments on out-of-range color values to
	// use this later on intermediate values for more precise final results
	for(int c = 0; c < (256 * 3); c++) {
		
		double c1 = (double)(c - 256);
		
		if (pal && (colorSpectrum || crtRealGamma))
			normalizeColorSpectrumPalGamma( c1 );
		
		adjustGamma( c1 );
		
		// make sure the final color channel is integer with a precision of 8 bit
        preCalcGamma[c] = uclamp8( c1 );
		
		// to emulate scanlines with a given intensity, the colors of the
		// adjacent lines will be blended together with reduced luminance.
		// that's why we precalculate the result of mixing two colors too.
		// formula: (a + b) / 2
		// if there is no shade, the scanline is black and fully visible. 
		// otherwise, the original mixed color will be visible.
		preCalcScanline[c * 2] = uclamp8( c1 * scanlineShade );

		// the mixing formula above could produce uneven results: x.5
		// the color channel can be an integer value only, but
		// applying adjustments on a virtual fractional value results in more
		// precise final values.
		c1 = (double)((c - 256) + 0.5);
		
		if (pal && (colorSpectrum || crtRealGamma))
			normalizeColorSpectrumPalGamma(c1);
		
		adjustGamma( c1 );
                        
		preCalcScanline[c * 2 + 1] = uclamp8( c1 * scanlineShade );
	}
}

inline auto VideoManager::adjustBrightness(double& c) -> void {
    
    c += brightness;
}

inline auto VideoManager::adjustGamma(double& c) -> void {
    
    if(gamma == 1.0)
        return;
    
    static double reciprocal = 1.0 / 255.0;
    
    c = 255.0 * std::pow(c * reciprocal, gamma);    
}

inline auto VideoManager::adjustContrast(double& c) -> void {
    
    if (contrast == 1.0)
        return;
    
    double C = (contrast * 100.0) - 100.0;

//    double F = (259.0 * (C + 255.0)) / (255.0 * (259.0 - C));
//    c = F * (c - 128.0) + 128.0;
    
    // Michelson contrast
    double lmin = 0.0 - C;
    double lmax = 255.0 + C;

    c = ((lmax - lmin) / (lmax + lmin)) * c + lmin;
}

inline auto VideoManager::adjustSaturation(double& r, double& g, double& b) -> void {
    
    if (saturation == 1.0)
        return;
    
    double grayscale = (r + g + b) / 3.0;
            
    r = ((r - grayscale) * saturation) + grayscale;
    g = ((g - grayscale) * saturation) + grayscale;
    b = ((b - grayscale) * saturation) + grayscale;
}

auto VideoManager::injectPhaseTransferError() -> void {
    
    // Next we encode/decode PAL signal.
    // C64 transfers PAL's V component 180° phase shifted (change sign) each odd line.
    // The receiver translates it back by changing the sign of V component again.
    // If there is no phase error during transmission, this process would be useless.
    // You already assume it, the real world is not ideal.
    // To explain this, let's have a look at what's the problem with NTSC.
    // A sender adds some degree of hue(V component) errors while transmitting the signal.
    // There is a setting, called tint, on each NTSC TV.
    // This way you can correct a great portion of the hue error of a specific sender.
    // Of course, the hue error isn't constant, means you can't correct it completely.
    // That's why NTSC is called: never the same color
    // back to PAL and the approach to stabilize hue.
    // Even lines: same transmission as NTSC, the transferred signal is received with an unknown phase shift error.
    // In comparison to NTSC the information is stored in TV(delay line)
    // odd lines: V component is phase shifted by 180° before transmission. (simply means the V value change sign).
    // The transmission adds a similar phase shift error within such a short time of one display line.
    // Receiver (TV) reverses sign of V component, which includes a similar phase shift error like happened in even line.
    // Now the receiver already has the U/V data of the even line, which is shifted by an unknown phase error from the real U/V data.
    // And there is the U/V data of the odd line that is shifted by a similar phase error but in the other
    // direction of the even line data. So we simply have to calculate the average between odd and even data to get
    // the original value. A tint correction like NTSC would be unnecessary.
    // The hue error is a lot smaller than NTSC, because the error doesn't shift that much in the short period of a scanline.
    // Of course, there are some disadvantages of this approach.
    // 1. the vertical resolution is halved, because in PAL 2 lines will be blended.
    // 2. if sender adds a huge phase shift error to U/V, the receiver can indeed reconstruct the original hue
    // by averaging data from even and odd lines, but the overall saturation will be lowered.
    // Delay lines will be saved in an analog way. Depending on TV quality, adjacent lines are desaturated differently.
	// This effect is known as (Hanover Bars)
    
	// we have already calculated the values for U and V
	// U = cos( angle ) * modifier( contrast and/or saturation )
	// V = sin( angle ) * modifier( contrast and/or saturation )
	// we want to add the phase error without complete recalculation
	// to do this we have to use additions theorems:
	// i.e. sin( a + b ) = sin a cos b + cos a sin b
	// i.e. sin( a - b ) = sin a cos b - cos a sin b
	// a is the original phase angle
	// b is the phase error
	// the modifier for contrast/saturation is already applied on U/V
	// so we take this into account
	// i.e. sin( a + b ) * modifier = modifier * (sin a cos b + cos a sin b)
	//								= modifier * sin a cos b + modifier * cos a sin b
	// we have already calculated: V = modifier * sin a , U = modifier * cos a
	// after substitution we get:
	//								= V * cos b + U * sin b
	
    double rotU = std::cos( phaseError * M_PI / 180.0 );	// cos b
    double rotV = std::sin( phaseError * M_PI / 180.0 );	// sin b

    for (unsigned c = 0; c < colorCount; c++) {
		
		evenTable[c].y = lumaChromaTable[c].y;
		// phase shifted chroma, received in TV (PAL, NTSC)
		evenTable[c].u_i = lumaChromaTable[c].u_i * rotU - lumaChromaTable[c].v_q * rotV;
		evenTable[c].v_q = lumaChromaTable[c].v_q * rotU + lumaChromaTable[c].u_i * rotV;
        
		if (pal) {
			// same phase error but shifted in the opposite direction (PAL)
			oddTable[c].y = lumaChromaTable[c].y;
			oddTable[c].u_i = lumaChromaTable[c].u_i * rotU - lumaChromaTable[c].v_q * rotV * -1;
			oddTable[c].v_q = lumaChromaTable[c].v_q * rotU + lumaChromaTable[c].u_i * rotV * -1;
		}
    }
}

auto VideoManager::convertLumaChromaToInteger() -> void {
	// luma changes overshoot then continue to oscillate a few pixels.
    // i.e. a blur of 1 means the luma of both neighboring pixels are weighted with 25% each
    // and the center pixel is weighted with 50 %
    double neighbour = blur * 0.25;
    double center = 1.0 - neighbour * 2.0;
    unsigned scalerLuma = pal ? 11 : 10;    
	
	for (unsigned c = 0; c < colorCount; c++) {

        // for performance reasons, we want to calculate the crt frame with integer later on.
        // we need to scale up with at least 8 bits to reconvert back to RGB lossless.
		evenTable[c].u_i_s = (int32_t) (evenTable[c].u_i * double(1 << 8) + (evenTable[c].u_i < 0 ? -0.5 : 0.5));
		evenTable[c].v_q_s = (int32_t) (evenTable[c].v_q * double(1 << 8) + (evenTable[c].v_q < 0 ? -0.5 : 0.5));
        
        // we scale up with 11 bits instead of 8, why ? read on
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
        // for ntsc there isn't a delay line. we scale up 2 bits only.
        
        evenTable[c].y_s = (int32_t) (evenTable[c].y * center * double(1 << scalerLuma) + (evenTable[c].y < 0 ? -0.5 : 0.5));
        evenTable[c].y_s_blur = (int32_t) (evenTable[c].y * neighbour * double(1 << scalerLuma) + (evenTable[c].y < 0 ? -0.5 : 0.5));
		
		if (pal) {
			oddTable[c].y_s = evenTable[c].y_s;
			oddTable[c].y_s_blur = evenTable[c].y_s_blur;
			oddTable[c].u_i_s = (int32_t) (oddTable[c].u_i * double(1 << 8) + (oddTable[c].u_i < 0 ? -0.5 : 0.5));
			oddTable[c].v_q_s = (int32_t) (oddTable[c].v_q * double(1 << 8) + (oddTable[c].v_q < 0 ? -0.5 : 0.5));						
		}				
	}
}

auto VideoManager::calculateLumaDelay() -> void {
	
	double yStep[4];
	double y;
	double yNext;
	double diff;
	bool stepChange;
	int direction;
    
    double _lumaRise = lumaRise == 0.0 ? 1.0 : lumaRise;
    double _lumaFall = lumaFall == 0.0 ? 1.0 : lumaFall;
	
	double neighbour = blur * 0.25;
    double center = 1.0 - neighbour * 2.0;
    unsigned scalerLuma = pal ? 11 : 10;  
	
	// calculate all combinations for 4 adjacent pixel
	for (unsigned j = 0; j <= 0xffff; j++ ) {
		
		uint8_t p3 = j & 0xf;
		uint8_t p2 = (j >> 4) & 0xf;
		uint8_t p1 = (j >> 8) & 0xf;
		uint8_t p0 = (j >> 12) & 0xf;
						
		yStep[0] = lumaChromaTable[p0].y;
		yStep[1] = lumaChromaTable[p1].y;
		yStep[2] = lumaChromaTable[p2].y;
		yStep[3] = lumaChromaTable[p3].y;		
		
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

template<typename T, uint8_t options> auto VideoManager::renderFrame(const T* src, unsigned width, unsigned height, unsigned srcPitch ) -> void {
	unsigned gpuPitch;
    unsigned* gpuData;
	float* gpuDataFloat;
    unsigned cropTop, cropLeft;
    constexpr bool interlace = options & 3;
    constexpr bool field = options & 2;
    constexpr bool hires = options & 4;
    constexpr bool shres = options & 8;
    constexpr bool lores = !hires && !shres;
    bool iHold = interlace && !field && !interlaceFields;
    bool suppressShader = program->warp.active;
    uint8_t gpuOptions = iHold | (interlace << 1) | (suppressShader << 2);

    if (needAUpdate)
        updateAll();

    bool cropCoordUpdated = emulator->cropCoordUpdated(cropTop, cropLeft);
    if (rebuildShader && !placeHolderSplashScreen) {
        if (crtMode == CrtMode::Gpu) {
            setData("autoEmu_cropTop", (float)cropTop);
            setData("autoEmu_cropLeft", (float)cropLeft);
            setData("autoEmu_lace", (float)interlace);
            setData("autoEmu_hires", (float)hires);
            setData("autoEmu_pal", (float)pal);
            setData("autoEmu_subRegion", emulator->getSubRegion());
            setData("autoEmu_tvGamma", (float)(shaderLumaChromaInput() && (colorSpectrum || crtRealGamma)));
            setData("autoEmu_lumaChroma", (float)(shaderLumaChromaInput()));
            setData("autoEmu_driveLED", (float)0);
            videoDriver->setShader( &parser->shaderPreset);
        } else
            videoDriver->setShader(nullptr);

        rebuildShader  = false;
    } else if (cropCoordUpdated && !placeHolderSplashScreen) {
        setData("autoEmu_cropTop", (float)cropTop);
        setData("autoEmu_cropLeft", (float)cropLeft);
    }

    frameOptions &= ~0x80; // init lace toggle
    if (frameOptions != ( (hires << 1) | interlace) ) {
        frameOptions = 0;
        if (interlace && ((frameOptions & 1) == 0) ) {
            gpuOptions &= ~1; // prevent hold in first lace frame
            frameOptions |= 0x80; // lace off -> on
            iHold = false;
        } //else
            //resetTempData(0, true);

        setData("autoEmu_lace", (float)interlace);
        setData("autoEmu_hires", (float)hires);
        frameOptions |= (hires << 1) | interlace;
    }

    if (++frameRenderPos != frameRenderTrigger) {
        return;
    }

    if (unlikely(takeScreenShots)) {
        auto& screenshot = view->screenshot;

        if (!--screenshot.intervalPos) {
            screenshot.intervalPos = screenshot.interval;

            if (screenshot.unscaled) {
                takeScreenshot<T>(screenshot.unscaled, src, width, height, srcPitch, options);
            } else {
                gpuOptions |= (uint8_t)DRIVER::OPT_TakeScreenshot;
                if (!screenshot.withEffects) {
                    gpuOptions |= (uint8_t)DRIVER::OPT_DisallowShader | (uint8_t)DRIVER::OPT_DisallowFilter;
                    suppressShader = true;
                }
            }

            if (!placeHolderFrames) {
                if (!--takeScreenShots && screenshot.pause)
                    program->isPause = screenshot.pause;
            }
        }
    }

    frameRenderPos = 0;
    videoDriver->setIntegerScalingDimension( lores ? (width << 1) : (hires ? width : (width >> 1)), interlace ? height : (height << 1), shres | hires | interlace);

    if (unlikely(placeHolderFrames)) {
        if (!placeHolderSplashScreen || ((placeHolderFrames & 3) == 0)) {
            takeScreenShots = 0;
            if (!view->renderPlaceholder(gpuOptions))
                return hidePlaceHolder();
        }

        if (!--placeHolderFrames) {
            if (emuThread->enabled) {
                emuThread->events |= EmuThread::EVT_DISMISS_PLACEHOLDER;
            } else {
                hidePlaceHolder();
            }
        }
        return;

    } else if ( suppressShader || (crtMode == CrtMode::None) ) {
        if (!videoDriver->lock(gpuData, gpuPitch, width, height, gpuOptions))
            return;

        renderToRgb<T, interlace, field>(width, height, src, srcPitch, gpuData, gpuPitch - width);

	} else if (crtMode == CrtMode::Gpu) {
        if (shaderLumaChromaInput()) {
            width += SHADER_OFFSCREEN_WIDTH << 1;
            srcPitch -= SHADER_OFFSCREEN_WIDTH << 1;
            src -= SHADER_OFFSCREEN_WIDTH;

            if (!videoDriver->lock(gpuDataFloat, gpuPitch, width, height + (interlace ? 2 : 1), gpuOptions ))
                goto Typical;

            renderToLumaChroma<T, interlace, field>(width, height, src, srcPitch, gpuDataFloat, gpuPitch - width, cropTop);

        } else {
Typical:
            if (!videoDriver->lock(gpuData, gpuPitch, width, height, gpuOptions))
                return;

            renderToRgb<T, interlace, field>(width, height, src, srcPitch, gpuData, gpuPitch - width);
        }
	} else if (crtThreaded) {
        if (!videoDriver->lock(gpuData, gpuPitch, width, (scanlines && !interlace) ? (height << 1) : height, gpuOptions))
            renderCrtThreadedBlank(width, height, src, srcPitch, iHold ? 0 : gpuData, gpuPitch - width);
        else
            renderCrtThreaded<T, options>(width, height, src, srcPitch, iHold ? 0 : gpuData, gpuPitch - width, cropTop);

    } else {
        if (!videoDriver->lock(gpuData, gpuPitch, width, (scanlines && !interlace) ? (height << 1) : height, gpuOptions))
            return;

        renderCrt<T, options>(width, height, src, srcPitch, iHold ? 0 : gpuData, gpuPitch - width, cropTop);
	}		           

    videoDriver->unlockAndRedraw( );
}

template<typename T> auto VideoManager::takeScreenshot(unsigned unscaled, const T* _src, unsigned _width, unsigned _height, unsigned _pitch, uint8_t _options) -> void {
    const T* _fT = _src;

    if (unscaled > 1) {
        if (unscaled == 2)
            _width = 320;
        else if (unscaled == 3)
            _width = isC64() ? 384 : 344;

        uint8_t* _f = activeEmulator->cropAlternatively(_width, _height, _pitch);
        if (!_f)
            return;
        _fT = (const T*)_f;
    }

    unsigned w = _width;
    unsigned h = _height;

    if (_options) {
        bool interlace = _options & 3;
        bool hires = _options & 4;
        bool shres = _options & 8;

        if (shres && !interlace)
            _height <<= 2;
        else if (shres && interlace)
            _height <<= 1;
        else if (hires && !interlace)
            _height <<= 1;
        else if (!hires && interlace)
            _width <<= 1;
    }

    uint8_t* _dest = new uint8_t[_width * _height * 3];
    renderToScreenshot<T>(w, h, _fT, _pitch, _dest, _options);
    program->takeScreenshot(_dest, _width, _height);
    delete[] _dest;
}

template<typename T, bool interlace, bool field> inline auto VideoManager::renderToRgb(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch) -> void {
    unsigned mask = (1 << countColorBits) - 1;

    if constexpr (interlace) {
        unsigned color;
        ColorRgbLight colorDecayed;
        bool laceToggle = !!(frameOptions & 0x80);
        unsigned iRate = (100 - interlaceDecay); // simulate phosphor decay
        if (laceToggle)
            iRate = 100;
        if (!laceToggle && !field && !interlaceFields) // hold -> update full frames only
            return;

        for(unsigned h = 0; h < height; h++) {

            if ((iRate != 100) && ((!field && (h & 1)) || (field && !(h & 1)))) {

                for (unsigned w = 0; w < width; w++) {
                    color = colorTable[*src++ & mask];
                    colorDecayed.r = (color >> 16) & 0xff;
                    colorDecayed.g = (color >> 8) & 0xff;
                    colorDecayed.b = (color >> 0) & 0xff;

                    colorDecayed.r = (colorDecayed.r * iRate) / 100;
                    colorDecayed.g = (colorDecayed.g * iRate) / 100;
                    colorDecayed.b = (colorDecayed.b * iRate) / 100;

                    *dest++ = colorDecayed.r << 16 | colorDecayed.g << 8 | colorDecayed.b;
                }
            } else {
                for (unsigned w = 0; w < width; w++) {
                    *dest++ = colorTable[*src++ & mask];
                }
            }

            src += srcPitch;
            dest += destPitch;
        }

    } else {
        for(unsigned h = 0; h < height; h++) {
            for(unsigned w = 0; w < width; w++)
                *dest++ = colorTable[ *src++ & mask ];

            src += srcPitch;
            dest += destPitch;
        }
    }
}

#define screenshot3channel(x) { auto& _x = x; *dest++ = _x.r; *dest++ = _x.g; *dest++ = _x.b; }
#define screenshot3channel2x(x) { auto& _x = x; *dest++ = _x.r; *dest++ = _x.g; *dest++ = _x.b; *dest++ = _x.r; *dest++ = _x.g; *dest++ = _x.b; }

template<typename T> auto VideoManager::renderToScreenshot(unsigned width, unsigned height, const T* src, unsigned srcPitch, uint8_t* dest, uint8_t _options) -> void {
    unsigned mask = (1 << countColorBits) - 1;
    bool interlace = _options & 3;
    bool field = _options & 2;
    bool hires = _options & 4;
    bool shres = _options & 8;

    if (interlace) {
        bool repeatWidth = !hires && !shres;
        unsigned color;
        ColorRgbLight colorDecayed;
        bool laceToggle = !!(frameOptions & 0x80);
        unsigned iRate = (100 - interlaceDecay); // simulate phosphor decay
        if (laceToggle)
            iRate = 100;

        for (unsigned h = 0; h < height; h++) {
            uint8_t* _p = dest;

            if ((iRate != 100) && ((!field && (h & 1)) || (field && !(h & 1)))) {

                for (unsigned w = 0; w < width; w++) {
                    color = palette->paletteColors[*src++ & mask].rgb;
                    colorDecayed.r = (color >> 16) & 0xff;
                    colorDecayed.g = (color >> 8) & 0xff;
                    colorDecayed.b = (color >> 0) & 0xff;

                    colorDecayed.r = (colorDecayed.r * iRate) / 100;
                    colorDecayed.g = (colorDecayed.g * iRate) / 100;
                    colorDecayed.b = (colorDecayed.b * iRate) / 100;

                    screenshot3channel(colorDecayed)
                }
            } else {
                for (unsigned w = 0; w < width; w++) {
                    if (repeatWidth)
                        screenshot3channel2x(palette->paletteColors[*src++ & mask])
                    else
                        screenshot3channel(palette->paletteColors[*src++ & mask])
                }
            }

            if (shres) {
                std::memcpy(dest, _p, width * 3);
                dest += width * 3;
            }

            src += srcPitch;
        }
    } else if (view->screenshot.writePalette) {
        for (unsigned h = 0; h < height; h++) {
            for (unsigned w = 0; w < width; w++)
                *dest++ = *src++ & mask;

            src += srcPitch;
        }
    } else {
        int repeatHeight = shres ? 3 : (hires ? 1 : 0);

        for (unsigned h = 0; h < height; h++) {
            uint8_t* _p = dest;
            for (unsigned w = 0; w < width; w++) {
                screenshot3channel(palette->paletteColors[*src++ & mask])
            }

            for(int i = 0; i < repeatHeight; i++) {
                std::memcpy(dest, _p, width * 3);
                dest += width * 3;
            }

            src += srcPitch;
        }
    }
}
#undef screenshot3channel

template<typename T, bool interlace, bool field> inline auto VideoManager::renderToLumaChroma(unsigned width, unsigned height, const T* src, unsigned srcPitch, float* dest, unsigned destPitch, unsigned& cropTop) -> void {
	const T* srcDelay = src;
    T color;
	unsigned _dp = destPitch * 4;
    unsigned metaShift = countColorBits;
    unsigned mask = (1 << metaShift) - 1;
    bool laceToggle = !!(frameOptions & 0x80);

    if (interlace && !field && !laceToggle && !interlaceFields) // hold -> update full frames only
        return;

    if(cropTop) {
        srcDelay -= width + srcPitch;
        if constexpr (interlace)
            srcDelay -= width + srcPitch;
    }

    for (int i = 0; i <= interlace; i++) {
        for (unsigned w = 0; w < width; w++) { // delayline
            color = *srcDelay++;

            *dest++ = lumaChromaTable[color & mask].y_n;
            *dest++ = lumaChromaTable[color & mask].u_i_n;
            *dest++ = lumaChromaTable[color & mask].v_q_n;
            *dest++ = color >> metaShift; // aec, ba
        }
        dest += _dp;
        srcDelay += srcPitch;
    }

    if (interlace && !laceToggle) {
        float iRate = (float)(100 - interlaceDecay);

        for (unsigned h = 0; h < height; h++) {
            if (field == (h & 1)) {
                for (unsigned w = 0; w < width; w++) {
                    color = *src++;

                    *dest++ = lumaChromaTable[color & mask].y_n;
                    *dest++ = lumaChromaTable[color & mask].u_i_n;
                    *dest++ = lumaChromaTable[color & mask].v_q_n;
                    *dest++ = color >> metaShift;
                }
            } else {
                for (unsigned w = 0; w < width; w++) {
                    color = *src++;

                    *dest++ = lumaChromaTable[color & mask].y_n * iRate / 100.0;
                    *dest++ = lumaChromaTable[color & mask].u_i_n;
                    *dest++ = lumaChromaTable[color & mask].v_q_n;
                    *dest++ = color >> metaShift;
                }
            }

            src += srcPitch;
            dest += _dp;
        }
    } else {
        for (unsigned h = 0; h < height; h++) {
            for (unsigned w = 0; w < width; w++) {
                color = *src++;

                *dest++ = lumaChromaTable[color & mask].y_n;
                *dest++ = lumaChromaTable[color & mask].u_i_n;
                *dest++ = lumaChromaTable[color & mask].v_q_n;
                *dest++ = color >> metaShift;
            }

            src += srcPitch;
            dest += _dp;
        }
    }
}

auto VideoManager::updateCrtThreads(bool light) -> void {
    for (auto videoManager : videoManagers) {
        bool useRenderThread = false;

        if (videoManager == activeVideoManager) {
            if ((videoManager->crtMode == CrtMode::Cpu) && videoManager->crtThreaded)
                useRenderThread = true;
        }

        if (!light) {
            videoManager->emulator->setLineCallback(useRenderThread);
            videoManager->reinitCrtThread(true);
        }

        if (!program->warp.active)
            videoManager->enableCrtThread(useRenderThread);
    }
}

auto VideoManager::enableCrtThread( bool state) -> void {

    if (workerCreated == state)
        return;

    for( unsigned t = 0; t < 2; t++ ) {
        Render* re = &render[t];
        re->dest = nullptr;

        if (t == 1)
            continue;

        if (state) {
            while (re->kill) {
                std::this_thread::yield();
            }

            if (countColorBits > 8)
                createWorker<uint16_t>( re );
            else
                createWorker<uint8_t>( re );

        } else {
            re->kill = true;
            re->cv.notify_one();
            while (re->kill)
                std::this_thread::yield();
        }
    }

    workerCreated = state;
}

template<typename T> auto VideoManager::createWorker(Render* re) -> void {

    std::thread worker([this, re] {

        std::chrono::milliseconds duration(5);
        std::mutex cvM;
        std::unique_lock<std::mutex> lk(cvM);

        re->dest = nullptr;
        re->kill = false;

        while (1) {
            re->ready = false;

            while (!re->ready.load()) {

                if (re->cv.wait_for(lk, duration, [re]() {
					return re->ready.load() || re->kill.load();
				})) {
					if (re->kill) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(20));
						re->kill = false;
						return;
					}
					break;
				}
            }

            renderCrtSelection<T>( *re );
        }
    });

    worker.detach();

}

auto VideoManager::waitForCrtRenderer() -> void {

    if (!crtThreaded || (crtMode != CrtMode::Cpu ) )
        return;

    Render* re = &render[0];

    while (re->ready.load())
        std::this_thread::yield();
}

// half screen
template<uint8_t options> auto VideoManager::renderMidScreen() -> void {
    constexpr bool interlace = options & 3;
	Render* re = &render[0];
	
	if (!colorTableUpdated)
		reinitCrtThread();
	
	if (interlace || !re->dest || frameRenderPos || placeHolderFrames)
		return;

    re->options = getRenderOptions<options>();

    if((re->options ^ render[1].options) & (1 | 4 | 32 | 0x100)) { // scanlines, interlace, hires changes
        re->dest = nullptr; // disable threaded emulation one frame to prevent artifacts
        return;
    }

    re->options |= 0x80;
	re->ready.store(1);
	re->cv.notify_one();
}

auto VideoManager::reinitCrtThread( bool initMem ) -> void {
    Render* re = &render[0];
    re->dest = nullptr;

    if (initMem)
        resetTempData();
}

auto VideoManager::resetTempData( int offset, bool onlyIfUsed ) -> void {
    if (onlyIfUsed && (crtMode != CrtMode::Cpu))
        return;

    std::memset(tempDest + (offset * 1024), 0, 1024 * (600 - offset) * 4);
}

template<typename T, uint8_t options> auto VideoManager::renderCrt(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch, unsigned& cropTop ) -> void {
    constexpr bool interlace = options & 3;
    Render& re = render[0];
	re.width = width;
    re.height = height;
	re.srcPitch = srcPitch;
	re.destPitch = destPitch;
    re.options = getRenderOptions<options>();
    re.fieldDest = tempDest;
	re.dest = dest ? dest : tempDest;
	re.scanlineDest = nullptr;
    re.oddLine = !cropTop ? 0x80 : ((cropTop >> interlace) & 1);
    re.src = (uint8_t*)src;
	renderCrtSelection<T>( re );
}

template<typename T> inline auto VideoManager::renderCrtSelection(Render& re) -> void {
    switch(re.options & 0x5f) {
        default:
        case 0: pal ? renderPalCrt<0, T>(re) : renderNtscCrt<0, T>(re); break;
        case 1: pal ? renderPalCrt<1, T>(re) : renderNtscCrt<1, T>(re); break; // Scanlines
        case 2: pal ? renderPalCrt<2, T>(re) : renderNtscCrt<2, T>(re); break; // RF
        case 3: pal ? renderPalCrt<3, T>(re) : renderNtscCrt<3, T>(re); break; // Scanlines + RF
        case 4: pal ? renderPalCrt<4, T>(re) : renderNtscCrt<4, T>(re); break; // Interlace
        case 6: pal ? renderPalCrt<6, T>(re) : renderNtscCrt<6, T>(re); break; // Interlace + RF
        case 12: pal ? renderPalCrt<12, T>(re) : renderNtscCrt<12, T>(re); break; // Interlace other field
        case 14: pal ? renderPalCrt<14, T>(re) : renderNtscCrt<14, T>(re); break; // Interlace other field + RF

        case 20: pal ? renderPalCrt<20, T>(re) : renderNtscCrt<20, T>(re); break; // Interlace(Hold)
        case 22: pal ? renderPalCrt<22, T>(re) : renderNtscCrt<22, T>(re); break; // Interlace(Hold) + RF
        case 28: pal ? renderPalCrt<28, T>(re) : renderNtscCrt<28, T>(re); break; // Interlace(Hold) other field
        case 30: pal ? renderPalCrt<30, T>(re) : renderNtscCrt<30, T>(re); break; // Interlace(Hold) other field + RF

        case 68: pal ? renderPalCrt<68, T>(re) : renderNtscCrt<68, T>(re); break; // Interlace toggle
        case 70: pal ? renderPalCrt<70, T>(re) : renderNtscCrt<70, T>(re); break; // Interlace toggle + RF

        // other combinations don't make sense, like Scanlines + Interlace
    }
}

auto VideoManager::useLumaDelay() -> bool {

	return lumaFall > 0.0 || lumaRise > 0.0;
}

auto VideoManager::useRegionEncoding() -> bool {
    return (hanoverBars && pal) || phaseError > 0.0;
}

template<typename T> auto VideoManager::renderCrtThreadedBlank(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch ) -> void {
    Render* re = &render[0];

    while (re->ready.load())
        std::this_thread::yield();

    re->dest = nullptr;
    re->fieldDest = nullptr;
    re->scanlineDest = nullptr;
}

template<typename T, uint8_t options> auto VideoManager::renderCrtThreaded(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch, unsigned& cropTop ) -> void {
    constexpr bool interlace = options & 3;
    //constexpr bool field = options & 2;

    static unsigned scaler = (2.0 / 3.0) * 256.0;
	Render& re = render[0];
	Render& re1 = render[1];
    re1.options = getRenderOptions<options>();

    if (height != currentHeight) {
        currentHeight = height;
        while (re.ready.load())
            std::this_thread::yield();
        re.dest = nullptr;
    }

    unsigned heightFirstHalfScreen = ((height * scaler) >> 8) & ~1;
	unsigned destOffset = (width + destPitch) * (heightFirstHalfScreen << ((!interlace && scanlines) ? 1 : 0));
    
    if (interlace || program->isPause || !re.dest || ((re.options ^ re1.options) & (1 | 4 | 32 | 0x100)) ) { // mostly to check if mid-screen callback is lores and switches to hires later on
        while (re.ready.load())
            std::this_thread::yield();

        renderCrt<T, options>( width, height, src, srcPitch, dest, destPitch, cropTop );
        re.dest = nullptr;

        if constexpr (interlace)
            return;
        if (program->isPause)
            return;
    } else {                
        re1.width = width;
        re1.srcPitch = srcPitch;
        re1.destPitch = destPitch;
        re1.height = height - heightFirstHalfScreen;
		
		re1.dest = dest ? (dest + destOffset) : (tempDest + destOffset);
        if (!interlace && scanlines) {
            destOffset -= width + destPitch;
            re1.scanlineDest = dest + destOffset;
        } else
		    re1.scanlineDest = nullptr;

        while (re.ready.load())
            std::this_thread::yield();
        
		re1.src = re.src;
		re1.oddLine = re.oddLine;
        re1.fieldDest = tempDest + destOffset;
        renderCrtSelection<T>( re1 );
	}
    
    emulator->setLineCallback( true, cropTop + heightFirstHalfScreen );

	if (dest && re.dest) {
        std::memcpy(dest, tempDest, destOffset << 2);
    }
			
	re.height = heightFirstHalfScreen;
	re.width = width;
	re.srcPitch = srcPitch;
	re.destPitch = destPitch;
	re.src = (uint8_t*)src;
	re.dest = tempDest;
	re.scanlineDest = nullptr;
    re.oddLine = !cropTop ? 0x80 : ((cropTop >> interlace) & 1);
    re.fieldDest = tempDest;
}

template<uint8_t options, typename T> auto VideoManager::renderPalCrt( Render& re ) -> void {
    
	static int32_t x1 = (int32_t) (((double) 1.0 / (double) 0.493) * double(1 << 8) + 0.5);
    static int32_t x2 = (int32_t) (((double) 1.0 / (double) 0.877) * double(1 << 8) + 0.5);
    static int32_t x3 = (int32_t) (0.3939307027516405140450117660881 * double(1 << 8) + 0.5);
    static int32_t x4 = (int32_t) (0.58080920903109757400461150856936 * double(1 << 8) + 0.5);

    constexpr bool withScanlines = options & 1;
    constexpr bool lumaDelay = options & 2;
    constexpr bool interlace = options & 4;
    constexpr bool field = options & 8;
    constexpr bool iHold = options & 16;
    constexpr bool laceToggle = options & 64;

	bool secondHalf = &re == &render[1];
    unsigned iRate = (100 - interlaceDecay);
	
	const T* _src = (T*)re.src;
	
	ColorLumaChroma* lineTable;
	ColorRgb* lineBeforeDest = nullptr;
	ColorLumaChroma yuv;
    ColorRgbLight colorI;
	int32_t uSubSample, vSubSample;
    unsigned mask = (1 << countColorBits) - 1;
	
	if (!secondHalf) {
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
                    yuv.u_i_s = (yuv.u_i_s * this->hanoverBars) >> 7;
                    yuv.v_q_s = (yuv.v_q_s * this->hanoverBars) >> 7;

                } else if (this->hanoverBarsAlt) {
                    yuv.u_i_s = (yuv.u_i_s * this->hanoverBarsAlt) >> 7;
                    yuv.v_q_s = (yuv.v_q_s * this->hanoverBarsAlt) >> 7;
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
        if ((re.options & 0x80) == 0) {
            lineBeforeDest = &lineBefore[0];
            for (unsigned w = 0; w < re.width; w++) {
                *re.scanlineDest++ = 255 << 24 | preCalcScanline[(lineBeforeDest->rInt << 1) + 512] << 16
                                     | preCalcScanline[(lineBeforeDest->gInt << 1) + 512] << 8
                                     | preCalcScanline[(lineBeforeDest->bInt << 1) + 512];

                lineBeforeDest++;
            }
        }
    }
	
	re.src = (uint8_t*)_src;
}

template<uint8_t options, typename T> auto VideoManager::renderNtscCrt( Render& re ) -> void {
	
	static int32_t x1 = (int32_t) (1.630 * double(1 << 8) + 0.5);
    static int32_t x2 = (int32_t) (0.317 * double(1 << 8) + 0.5);
    static int32_t x3 = (int32_t) (0.378 * double(1 << 8) + 0.5);
    static int32_t x4 = (int32_t) (0.466 * double(1 << 8) + 0.5);
	static int32_t x5 = (int32_t) (1.089 * double(1 << 8) + 0.5);
	static int32_t x6 = (int32_t) (1.677 * double(1 << 8) + 0.5);

    constexpr bool withScanlines = options & 1;
    constexpr bool lumaDelay = options & 2;
    constexpr bool interlace = options & 4;
    constexpr bool field = options & 8;
    constexpr bool iHold = options & 16;
    constexpr bool laceToggle = options & 64;

	ColorLumaChroma yiq;
    ColorRgbLight colorI;
	ColorRgb* lineBeforeDest = nullptr;
    unsigned mask = (1 << countColorBits) - 1;
	
	int32_t iSubSample, qSubSample;	
	bool secondHalf = &re == &render[1];
    unsigned iRate = (100 - interlaceDecay);
	const T* _src = (T*)re.src;
	
	if (!secondHalf) {
		_src -= 2;       	                       	
	}
	
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
        if ((re.options & 0x80) == 0) {
            lineBeforeDest = &lineBefore[0];
            for (unsigned w = 0; w < re.width; w++) {
                *re.scanlineDest++ = 255 << 24 | preCalcScanline[(lineBeforeDest->rInt << 1) + 512] << 16
                                     | preCalcScanline[(lineBeforeDest->gInt << 1) + 512] << 8
                                     | preCalcScanline[(lineBeforeDest->bInt << 1) + 512];

                lineBeforeDest++;
            }
        }
    }
	
	re.src = (uint8_t*)_src;
}

auto VideoManager::convertRGBToYIQ(ColorLumaChroma* dest, ColorRgb* src) -> void {
    dest->y  = 0.23485876230514607f * src->r + 0.6335007388077467f  * src->g + 0.13164049888710716f * src->b;
    dest->u_i = 0.4409594767911895f  * src->r - 0.27984362502847304f * src->g - 0.16111585176271648f * src->b;
    dest->v_q = 0.14630060102591497f * src->r - 0.5594814826856017f  * src->g + 0.4131808816596867f  * src->b;
}

auto VideoManager::convertRGBToYUV(ColorLumaChroma* dest, ColorRgb* src) -> void {
    dest->y = 0.299 * src->r + 0.587 * src->g + 0.114 * src->b;
    dest->u_i = (src->b - dest->y) * 0.493;
    dest->v_q = (src->r - dest->y) * 0.877;
}

auto VideoManager::convertYUVToRGB(ColorRgb* dest, ColorLumaChroma* src) -> void {
//    dest->r = src->y + 1.140 * src->v_q;
//    dest->g = src->y - 0.396 * src->u_i - 0.581 * src->v_q;
//    dest->b = src->y + 2.029 * src->u_i;
    dest->r = src->y + ((double)1.0 / (double)0.877) * src->v_q;
    dest->g = src->y - 0.3939307027516405140450117660881 * src->u_i - 0.58080920903109757400461150856936 * src->v_q;
    dest->b = src->y + ((double)1.0 / (double)0.493) * src->u_i;
}

// sony decoder matrix
auto VideoManager::convertYIQToRGB(ColorRgb* dest, ColorLumaChroma* src) -> void {
	dest->r = src->y + 1.630 * src->u_i + 0.317 * src->v_q;
	dest->g = src->y - 0.378 * src->u_i - 0.466 * src->v_q;
	dest->b = src->y - 1.089 * src->u_i + 1.677 * src->v_q;
}

auto VideoManager::powerOff() -> void {
	reinitCrtThread(true);
    currentHeight = 0;
}

auto VideoManager::updateData(int offset, float data) -> void {
    DataUpdates dataUpdate;
    dataUpdate.offset = offset;
    dataUpdate.dataF = data;

    emuThread->lockVideo();
    dataUpdates.push_back( dataUpdate );
    dataUpdatesPending = true;
    needAUpdate = true;
    emuThread->unlockVideo();
}

template<typename T> auto VideoManager::updateData(std::string ident, T data) -> void {
    DataUpdates dataUpdate;
    dataUpdate.ident = ident;
    dataUpdate.offset = -1;

    if (std::is_same<T, float>::value) {
        dataUpdate.dataF = data;
    } else if (std::is_same<T, bool>::value) {
        dataUpdate.dataB = data;
    } else if (std::is_same<T, unsigned>::value) {
        dataUpdate.dataU = data;
    } else if (std::is_same<T, int>::value) {
        dataUpdate.dataI = data;
    }

    emuThread->lockVideo();
    dataUpdates.push_back( dataUpdate );
    dataUpdatesPending = true;
    needAUpdate = true;
    emuThread->unlockVideo();
}

auto VideoManager::applyDataUpdates() -> void {

    emuThread->lockVideo();
    dataUpdatesPending = false;
    auto _dataUpdates = dataUpdates;
    dataUpdates.clear();
    emuThread->unlockVideo();

    for(auto& dataUpdate : _dataUpdates) {
        if (dataUpdate.offset >= 0)                         setData(dataUpdate.offset, dataUpdate.dataF);

        else if (dataUpdate.ident == "gamma")               setGamma( dataUpdate.dataU );
        else if (dataUpdate.ident == "saturation")          setSaturation( dataUpdate.dataU );
        else if (dataUpdate.ident == "brightness")          setBrightness( dataUpdate.dataU );
        else if (dataUpdate.ident == "contrast")            setContrast( dataUpdate.dataU );
        else if (dataUpdate.ident == "phase")               setPhase( dataUpdate.dataI );
        else if (dataUpdate.ident == "scanlines")           setScanlines( dataUpdate.dataU );
        else if (dataUpdate.ident == "interlace")           setInterlace( dataUpdate.dataU );
        else if (dataUpdate.ident == "interlace_fields")    setInterlaceFields( dataUpdate.dataB );
        else if (dataUpdate.ident == "blur")                setBlur( dataUpdate.dataU );
        else if (dataUpdate.ident == "phase_error")         setPhaseError( dataUpdate.dataF );
        else if (dataUpdate.ident == "hanover_bars")        setHanoverBars( dataUpdate.dataI );

        else if (dataUpdate.ident == "luma_rise")           setLumaRise( dataUpdate.dataF );
        else if (dataUpdate.ident == "luma_fall")           setLumaFall( dataUpdate.dataF );

        else if (dataUpdate.ident == "new_luma")            setNewLuma( dataUpdate.dataB );
        else if (dataUpdate.ident == "tv_gamma")            setCrtRealGamma( dataUpdate.dataB );
    }
}

auto VideoManager::isC64() -> bool {    
    return dynamic_cast<LIBC64::Interface*>(emulator);
}

auto VideoManager::isAmiga() -> bool {    
    return dynamic_cast<LIBAMI::Interface*>(emulator);
}

inline auto VideoManager::uclamp8(double x) -> uint8_t {
    return std::min( std::max((int)(x + 0.5), 0), 255 );
}

template<uint8_t options> auto VideoManager::getRenderOptions() -> unsigned {
    unsigned out = 0;
    constexpr bool interlace = options & 3;
    constexpr bool field = options & 2;
    constexpr bool hires = options & 4;
    constexpr bool shres = options & 8;

    if (scanlines && !interlace) out |= 1; // surpress user requested scanlines if software requests interlace
    if ((countColorBits == 4) && useLumaDelay()) out |= 2;
    if (interlace) {
        bool laceToggle = !!(frameOptions & 0x80);
        out |= 4;
        if (laceToggle) out |= 64;
        else if (field) out |= 8;

        if (!interlaceFields && !laceToggle) out |= 16;
    }
    if (hires) out |= 32;
    if (shres) out |= 0x100;
    return out;
}

auto VideoManager::hidePlaceHolder() -> void {
    if (placeHolderSplashScreen) {
        placeHolderSplashScreen = false;
        program->setVideoFilter();
        view->setDefaultCursor();
    }
    placeHolderFrames = 0;
}

auto VideoManager::free() -> void {
    if (colorTable)
        delete[] colorTable;

    if (lumaChromaTable)
        delete[] lumaChromaTable;                
    
    if (evenTable)
        delete[] evenTable;
        
    if (oddTable)
        delete[] oddTable;
        
    evenTable = oddTable = lumaChromaTable = nullptr;
    colorTable = nullptr;
	
	if (tempDest)
		delete[] tempDest;
}

auto VideoManager::loadPreset() -> bool {
    std::string path = GUIKIT::File::resolveRelativePath(settings->get<std::string>("slang_loaded", ""));
    if (path.empty()) {
        clearPreset();
        return false;
    }

    std::vector<std::string> errors;
    return loadPreset(path, errors) != nullptr;
}

auto VideoManager::loadPreset(const std::string& path) -> void {
    std::vector<std::string> errors;
    loadPreset(path, errors);
}

auto VideoManager::loadPreset(const std::string& path, std::vector<std::string>& errors) -> ShaderPreset* {
    ShaderParser* tempParser = new ShaderParser;

    bool res = tempParser->loadPreset(path);
    GUIKIT::Vector::combine(errors, tempParser->errors);

    if (!res) {
        delete tempParser;
        return nullptr;
    }
    // improtant, parameter changes when temp parser replaces current parser.
    // old shader is still active ... need to run to a safe spot before
    videoDriver->waitRenderThread();
    settings->set<std::string>("slang_loaded", GUIKIT::File::buildRelativePath(path));
    parser->clear();
    delete parser;

    parser = tempParser; // emu and render thread must be paused until the current shader is unloaded.
    // The new shader can then be built asynchronously.

    rebuildShader = true;
    requestUpdate(); // check for FP mode
    applyMeta();
    driveLedParam = getData("autoEmu_driveLED");

    return &parser->shaderPreset;
}

auto VideoManager::addPreset(std::string path, bool prepend, std::vector<std::string>& errors) -> ShaderPreset* {
    ShaderParser* tempParser = new ShaderParser;
    bool res = tempParser->loadPreset(path);
    GUIKIT::Vector::combine(errors, tempParser->errors);

    if (!res) {
        delete tempParser;
        return nullptr;
    }

    videoDriver->waitRenderThread();
    if (!parser->addPreset( tempParser, prepend )) {
        GUIKIT::Vector::combine(errors, parser->errors);
        delete tempParser;
        return nullptr;
    }

    delete tempParser;
    rebuildShader = true;
    requestUpdate(); // check for FP mode
    applyMeta();
    driveLedParam = getData("autoEmu_driveLED");
    return &parser->shaderPreset;
}

auto VideoManager::savePreset(std::string path) -> bool {
    bool res = parser->savePreset(path);

    if (res)
        settings->set<std::string>("slang_loaded", GUIKIT::File::buildRelativePath(path));

    return res;
}

auto VideoManager::getPreset(std::vector<std::string>& errors) -> ShaderPreset* {
    GUIKIT::Vector::combine(errors, parser->errors);
    return parser->entryPaths.size() ? &parser->shaderPreset : nullptr;
}

auto VideoManager::getPreset() -> ShaderPreset* {
    return &parser->shaderPreset;
}

auto VideoManager::finishPreset() -> void {
    parser->addBrokenLUT();
    auto emuView = EmuConfigView::TabWindow::getView(emulator);
    if (emuView && emuView->presentationLayout)
        emuView->presentationLayout->presentShaderError();
}

auto VideoManager::clearPreset() -> void {
    bool locked = emuThread->lock();
    // don't wait for the renderer if the emu thread is running and constantly submitting new images.
    // This could lead to an endless loop
    videoDriver->waitRenderThread();
    parser->clear();
    applyMeta();
    driveLedParam = nullptr;
    rebuildShader = true;
    if (locked) // avoid releasing a lock prematurely, when locked before
        emuThread->unlock();
    settings->set<std::string>("slang_loaded", "");    
}

auto VideoManager::getPresetPath() -> std::string {
    return parser->getPresetPath();
}

auto VideoManager::getPresetPathDetailed() -> std::string {
    return parser->getPresetPathDetailed();
}

auto VideoManager::movePass(unsigned& passId, bool up) -> void {
    parser->movePass(passId, up);
    rebuildShader = true;
}

auto VideoManager::togglePassUsage(unsigned passId) -> ShaderPreset::Pass* {
    auto pass = parser->togglePassUsage(passId);
    applyMeta();
    rebuildShader = true;
    return pass;
}

auto VideoManager::setPassFilter(unsigned passId, ShaderPreset::Filter filter) -> void {
    parser->setPassFilter(passId, filter);
    rebuildShader = true;
}

auto VideoManager::setPassMipmap(unsigned passId, bool state) -> void {
    parser->setPassMipmap(passId, state);
    rebuildShader = true;
}

auto VideoManager::setPassScaleX(unsigned passId, float scale) -> void {
    parser->setPassScaleX(passId, scale);
    rebuildShader = true;
}

auto VideoManager::setPassScaleY(unsigned passId, float scale) -> void {
    parser->setPassScaleY(passId, scale);
    rebuildShader = true;
}

auto VideoManager::shaderLumaChromaInput() -> bool {
    return parser->shaderPreset.lumaChroma;
}

auto VideoManager::translateShaderBufferType(ShaderPreset::BufferType& bufferType) -> const std::string {
    return parser->translateBufferType(bufferType);
}

auto VideoManager::fetchShader(ShaderPreset::Pass& pass, unsigned passId) -> bool {
    auto& preset = parser->shaderPreset;
    if (passId >= preset.passes.size())
        return false;

    ShaderParser::dataStorage->unload();
    ShaderParser temp;
    pass.src = preset.passes[passId].src;
    return temp.fetchShaderSource( pass );
}

VideoManager::~VideoManager() {
    enableCrtThread(false);
    free();
    delete parser;
}

template auto VideoManager::renderFrame<uint8_t>(const uint8_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 0>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 1>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 2>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 4>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 5>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 6>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 8>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 9>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 10>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;

template auto VideoManager::renderMidScreen<0>() -> void;
template auto VideoManager::renderMidScreen<1>() -> void;
template auto VideoManager::renderMidScreen<2>() -> void;
template auto VideoManager::renderMidScreen<4>() -> void;
template auto VideoManager::renderMidScreen<5>() -> void;
template auto VideoManager::renderMidScreen<6>() -> void;
template auto VideoManager::renderMidScreen<8>() -> void;
template auto VideoManager::renderMidScreen<9>() -> void;
template auto VideoManager::renderMidScreen<10>() -> void;

template auto VideoManager::updateData<bool>(std::string ident, bool data) -> void;
template auto VideoManager::updateData<int>(std::string ident, int data) -> void;
template auto VideoManager::updateData<unsigned>(std::string ident, unsigned data) -> void;
template auto VideoManager::updateData<float>(std::string ident, float data) -> void;
