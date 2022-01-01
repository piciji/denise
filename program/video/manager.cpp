
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdlib>
#include <cstring>
#include "manager.h"
#include "../../emulation/libc64/interface.h"
#include "../emuconfig/config.h"
#include "../media/media.h"
#include "../view/view.h"
#include "props.cpp"
#include "sync.cpp"
#include "../../driver/tools/shaderpass.h"
#include "../tools/chronos.h"
#include "../view/status.h"

bool VideoManager::synchronized = true;
bool VideoManager::crtThreaded = true;
bool VideoManager::shaderInputPrecision = false;
bool VideoManager::aspectCorrect = true;
bool VideoManager::integerScaling = false;
bool VideoManager::fpsLimit = false;
uint8_t VideoManager::frameRenderPos = 0;
uint8_t VideoManager::frameRenderTrigger = 1;

std::vector<VideoManager*> videoManagers;

auto VideoManager::getInstance( Emulator::Interface* emulator ) -> VideoManager* {
	
	for (auto videoManager : videoManagers) {
		if (videoManager->emulator == emulator)
			return videoManager;
	}
    
	return nullptr;
}

auto VideoManager::updateWhenNotRunning() -> void {
	
	for (auto videoManager : videoManagers) {
		if (videoManager->needUpdate())
			videoManager->update();
	}
}

auto VideoManager::setFrameRender(uint8_t limit) -> void {
    frameRenderTrigger = limit;
    frameRenderPos = 0;
}

VideoManager::VideoManager(Emulator::Interface* emulator) : shader(this) {
    this->emulator = emulator;
    this->settings = program->getSettings(emulator);
    this->palette = &emulator->palettes[0];        
    this->colorCount = this->palette->paletteColors.size();        

	mask = 0xffff;
	metaShift = 0;
	use16BitSrc = true;
    workerCreated = false;
	
	if (isC64()) {
		mask = 0xf;
		metaShift = 4;
		use16BitSrc = false;
		
	} else if (isAmiga()) {
		mask = 0xfff;
		metaShift = 12;
		use16BitSrc = true;		
	}
	
    gamma = 1.0;
    contrast = 1.0;
    brightness = 1.0;
	saturation = 1.0;    

    colorTableUpdated = false;

    lumaChromaTable = new ColorLumaChroma[this->colorCount];      
    evenTable = new ColorLumaChroma[this->colorCount];  
    oddTable = new ColorLumaChroma[this->colorCount];          
    colorTable = new uint32_t[this->colorCount];     
    colorTableNoGamma = new uint32_t[this->colorCount];     
    
    newLuma = true;
    crtRealGamma = false;
	phase = 0.0;
	pal = true;
    colorSpectrum = false;  
    crtMode = CrtMode::None;
    
    firSharp = 0;
    firTaps = 9;
    
    scanlines = 0;
    
    chromaNoise = lumaNoise = 0.0;
    randomLineOffset = 0.0;
    
    maskPitch = radialDistortion = 0;
    maskDpi = maskLevel = 0;
    maskType = MaskType::Aperture;
    maskLuminance = 1.6;
    aecGlitch = baGlitch = phi0Glitch = rasGlitch = casGlitch = 0;
	bloomWeight = bloomVariance = 2.7;
	bloomGlow = 0.0;
	bloomRadius = 1;
    
    distortionHires = false;
    hires = false;
    phaseError = 22.5; 
    hanoverBars = (int32_t)(0.8 * 128.0); // 20% saturation loss
    hanoverBarsAlt = 0;
    
    integerScaling = false;
    currentHeight = 0;
	
	tempDest = new uint32_t[ 512 * 768 ];
	
	lumaRise = 1.0 / 2.0;
	
	lumaFall = 1.0 / 1.2;
    
    luminance = 1.0;
    
    lightFromCenter = 1.0;

    render[0].kill = false;
    render[1].kill = false;
    reinitCrtThread(true);
}

auto VideoManager::update() -> void {
	
    if (!colorSpectrum || !isC64() ) { // palette
        if ( useCrtMode() ) {
            convertPaletteToLumaChroma();
            // to get color table with applied base properties for list view colors in GUI
            convertLumaChromaToRGB();
        } else
            adjustPalette();
        
    } else {
        generateC64ColorSpectrum();
		
        convertLumaChromaToRGB();
    }

    if ( useCrtMode() ) {
        preCalcGamma();
		
		if (useRfModulation())
			preCalcRfModulation();
        
        if (crtMode == CrtMode::Cpu) {
            injectPhaseTransferError();
            convertLumaChromaToInteger();   
        }             
    }   
    
    updateListingColors();
    
    colorTableUpdated = true;    
}

auto VideoManager::updateListingColors() -> void {    
    if (!isC64())
        return;

    auto emuView = EmuConfigView::TabWindow::getView( emulator );

    if (emuView && emuView->mediaLayout)
        emuView->mediaLayout->colorListing( colorTable[14], colorTable[6] );
}

auto VideoManager::getC64Foreground() -> unsigned {
    return colorTable[14];
}

auto VideoManager::getC64Background() -> unsigned {
    return colorTable[6];
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

        colorTableNoGamma[c] = uclamp8( rgb.r ) << 16 | uclamp8( rgb.g ) << 8 | uclamp8( rgb.b );    
        
		if (pal && colorSpectrum) {
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

auto VideoManager::preCalcGamma() -> void {
    double scanlineShade = 1.0 - (double)scanlines / 100.0;
    
	// we precalculate the requested adjustments for each color state.
	// besides we apply adjustments on out of range color values in order to 
	// use this later on intermediate values for more precise final results
	for(int c = 0; c < (256 * 3); c++) {
		
		double c1 = (double)(c - 256);
		
		if (pal && (colorSpectrum || crtRealGamma))
			normalizeColorSpectrumPalGamma( c1 );
		
		adjustGamma( c1 );
		
		// make sure the final color channel is integer with a precision of 8 bit
		preCalc[c] = uclamp8( c1 );
        
        preCalcF[c] = (float)preCalc[c] / 255.0;
		
		// to emulatate scanlines with a given intensity, the colors of the 
		// adjacent lines will be blended together with reduced luminance.
		// thats why we precalculate the result of mixing two colors too.
		// formula: (a + b) / 2
		// if there is no shade, the scanline is black and fully visible. 
		// otherwise the original mixed color will be visible.
		preCalcScanline[c * 2] = uclamp8( c1 * scanlineShade ); 
        
        preCalcScanlineF[c * 2] = preCalcScanline[c * 2] / 255.0;

		// the mixing formula above could produce uneven results: x.5
		// of course the color channel can be an integer value only, but
		// applying adjustments on a virtual fractional value results in more
		// precise final values.
		c1 = (double)((c - 256) + 0.5);
		
		if (pal && (colorSpectrum || crtRealGamma))
			normalizeColorSpectrumPalGamma(c1);
		
		adjustGamma( c1 );
                        
		preCalcScanline[c * 2 + 1] = uclamp8( c1 * scanlineShade );
        
        preCalcScanlineF[c * 2 + 1] = preCalcScanline[c * 2 + 1] / 255.0;
	}
    
    if ( !shader.recreate )
        shader.transferGammaAndScanlines();
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
    
    // next we encode/decode pal signal.
    // sender(c64, amiga) transfers pal's V component 180° phase shifted (change sign) each odd line.
    // receiver translates it back by changing sign of V component again.
    // if there is no phase error during transmission, this process would be completly useless.
    // you already assume it, the real world is not ideal.
    // to explain this let's have a look what's the problem with NTSC.
    // a sender adds some degree of hue(V component) errors while transmitting the signal.
    // there is a setting, called tint, on each NTSC TV. this way you can correct a great portion
    // of the hue error of a specific sender. of course the hue error isn't constant, means you
    // can't correct it completly. thats why ntsc is called: never the same color :-)
    // back to pal and the approach to stabilize hue.
    // even lines: same transmission as NTSC, the transfered signal is received with an unknown
    // phase shift error. in comparision to NTSC the information is stored in TV. (delay line)
    // odd lines: V component is phase shifted by 180° before transmission. (simply means the V value change sign).
    // the transmission adds a similiar phase shift error within such a short time of one display line.
    // receiver (TV) reverses sign of V component, which includes a similiar phase shift error like happened in
    // even line. 
    // now the receiver already has the U/V data of the even line, which is shifted by an unknown phase error from the 
    // real U/V data and there is the U/V data of the odd line which is shifted by a similiar phase error but in the other
    // direction of the even line data. so we simply have to calculate the average between odd and even data to get
    // the original value. a tint correction like NTSC would be unnecessary.
    // the hue error is a lot of smaller than NTSC, because the error doesn't shift that much in the short
    // period of a scanline.
    // of course there are some disadvantages of this approach.
    // 1. the vertical resolution is halved, because 2 lines will be blended together.
    // 2. if sender adds a huge phase shift error to U/V, the receiver can indeed reconstruct the original hue
    // by averaging data from even and odd lines but the overall saturation will be lowered.
    // delay lines will be saved in an analog way. depending on TV quality adjacent lines are desaturated differently.
	// this effect is known as (Hanover Bars)
    
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
	// so after substituaion we get:
	//								= V * cos b + U * sin b
	
    double rotU = std::cos( phaseError * M_PI / 180.0 );	// cos b
    double rotV = std::sin( phaseError * M_PI / 180.0 );	// sin b

    for (unsigned c = 0; c < colorCount; c++) {
		
		evenTable[c].y = lumaChromaTable[c].y;
		// phase shifted chroma, received in TV (pal, ntsc)
		evenTable[c].u_i = lumaChromaTable[c].u_i * rotU - lumaChromaTable[c].v_q * rotV;
		evenTable[c].v_q = lumaChromaTable[c].v_q * rotU + lumaChromaTable[c].u_i * rotV;
        
		if (pal) {
			// same phase error but shifted in opposite direction (pal)
			oddTable[c].y = lumaChromaTable[c].y;
			oddTable[c].u_i = lumaChromaTable[c].u_i * rotU - lumaChromaTable[c].v_q * rotV * -1;
			oddTable[c].v_q = lumaChromaTable[c].v_q * rotU + lumaChromaTable[c].u_i * rotV * -1;
		}
    }
}

auto VideoManager::convertLumaChromaToInteger() -> void {
	// luma changes overshoot then continue to oscillate a few pixel.
    // i.e. a blur of 1 means the luma of both neighbouring pixel are weighted with 25% each    
    // and the center pixel is weighted with 50 %
    double neighbour = blur * 0.25;
    double center = 1.0 - neighbour * 2.0;
    unsigned scalerLuma = pal ? 11 : 10;    
	
	for (unsigned c = 0; c < colorCount; c++) {

        // for performance reasons we want to calculate the crt frame with integer later on.
        // we need to scale up with at least 8 bits to reconvert back to RGB lossless.
		evenTable[c].u_i_s = (int32_t) (evenTable[c].u_i * double(1 << 8) + (evenTable[c].u_i < 0 ? -0.5 : 0.5));
		evenTable[c].v_q_s = (int32_t) (evenTable[c].v_q * double(1 << 8) + (evenTable[c].v_q < 0 ? -0.5 : 0.5));
        
        // we scale up with 11 bits instead of 8, why ? read on
        // after chroma subsampling (adding 4 pixel), the saling of chroma increases to 10 bits
        // and after adding the final pixel to delayed pixel the scaling of chroma increases to 11 bits.
        // chroma and luma need to be scaled the same in order to apply math on it.
        // average color (horizontal) = (pixel1 + pixel2 + pixel3 + pixel4) / 4
        // average color (vertical)   = (average color (horizontal) + delayed average color of last line) / 2
        // 
        // pFinal = ((p1 + p2 + p3 + p4) / 4 + (pLast1 + pLast2 + pLast3 + pLast4) / 4 ) / 2
        // pFinal * 2 = (p1 + p2 + p3 + p4) / 4 + (pLast1 + pLast2 + pLast3 + pLast4) / 4
        // pFinal * 8 = p1 + p2 + p3 + p4 + pLast1 + pLast2 + pLast3 + pLast4
        
        // so we can add all 8 pixel first and divide later on by 8 to get the averaged pixel.
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

auto VideoManager::preCalcRfModulation() -> void {
	
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

template<typename T> auto VideoManager::renderFrame(const T* src, unsigned width, unsigned height, unsigned srcPitch ) -> void {			

	unsigned gpuPitch;
    unsigned* gpuData;
	float* gpuDataFloat;

    if ( needUpdate() )
		update(); 
    
    if ( shader.recreate ) {
        shader.loadInternal();
        
        bool error = false;        
        if (!shader.externalLoaded)
            error = !shader.loadExternal();

        error |= !shader.sendToDriver();

        if (error)
            view->updateShader();
		
        shader.recreate  = false;
    }           
        
	if (scalingCount) {
		if (--scalingCount == 0)
			view->updateViewport();                
	}
	
    if (height != currentHeight) {     
        currentHeight = height;
        
        if(integerScaling)
            scalingCount = 10;	
        
        reinitCrtThread();
    }

    if (++frameRenderPos != frameRenderTrigger) {
        return;
    }

    frameRenderPos = 0;

//    int64_t res = (int64_t)Chronos::getTimestampInMicroseconds() - lastCapTime;
//
//    if (res > 5000)
//        logger->log(std::to_string( res ));
    
	if ( !useCrtMode() ) {
		if (!videoDriver->lock(gpuData, gpuPitch, width, height))
			return; 
		
		renderToRgb(width, height, src, srcPitch, gpuData, gpuPitch - width);
		
	} else if (crtMode == CrtMode::Gpu) {
        width += SHADER_OFFSCREEN_WIDTH << 1;
        srcPitch -= SHADER_OFFSCREEN_WIDTH << 1;
        src -= SHADER_OFFSCREEN_WIDTH;
                
		// we need delay line before first visible line to calculate mixed line in pal
		// hence height + 1
		if (shaderInputPrecision) {
			if (!videoDriver->lock(gpuDataFloat, gpuPitch, width, height + 1))
				return;

			renderToLumaChroma( width, height, src, srcPitch, gpuDataFloat, gpuPitch - width );
		} else {
			if (!videoDriver->lock(gpuData, gpuPitch, width, height + 1))
				return;

			renderToRgbNoGamma(width, height, src, srcPitch, gpuData, gpuPitch - width );
		}
		
	} else if (crtThreaded) {
		if (!videoDriver->lock(gpuData, gpuPitch, width, scanlines ? (height << 1) : height ))
            return renderCrtThreadedBlank(width, height, src, srcPitch, gpuData, gpuPitch - width);
		
		return renderCrtThreaded(width, height, src, srcPitch, gpuData, gpuPitch - width);
	} else {
		if (!videoDriver->lock(gpuData, gpuPitch, width, scanlines ? (height << 1) : height))
			return;
		
		renderCrt(width, height, src, srcPitch, gpuData, gpuPitch - width);
	}		           
    
	videoDriver->unlock();
    
    //if (fpsLimit)
      //  applyFpsLimit();
    
	videoDriver->redraw();
    
    //lastCapTime = Chronos::getTimestampInMicroseconds();    
}

template<typename T> inline auto VideoManager::renderToRgb(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch) -> void {

	for(unsigned h = 0; h < height; h++) {
		for(unsigned w = 0; w < width; w++)
			*dest++ = colorTable[ *src++ & mask ];		

		src += srcPitch;
		dest += destPitch;
	}
}

template<typename T> inline auto VideoManager::renderToRgbNoGamma(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch) -> void {
	T _src;
    unsigned cropTop = this->emulator->cropTop(); 
    
    if (cropTop)
        src -= width + srcPitch; 
        
    for(unsigned w = 0; w < width; w++) {
        _src = *src++;
        *dest++ = colorTableNoGamma[ _src & mask ] | ((_src >> metaShift) << 24);		
    }

    src += srcPitch;
    dest += destPitch;
    
    if (!cropTop)
        src -= width + srcPitch; 
    
	for(unsigned h = 0; h < height; h++) {
		for(unsigned w = 0; w < width; w++) {
            _src = *src++;
			*dest++ = colorTableNoGamma[ _src & mask ] | ((_src >> metaShift) << 24);		
        }
		src += srcPitch;
		dest += destPitch;
	}
}

template<typename T> inline auto VideoManager::renderToLumaChroma(unsigned width, unsigned height, const T* src, unsigned srcPitch, float* dest, unsigned destPitch) -> void {
	T _src;
	unsigned _dp = destPitch * 4;
	
    unsigned cropTop = this->emulator->cropTop(); 
    
    if (cropTop)
        src -= width + srcPitch;            
    
    for (unsigned w = 0; w < width; w++) {
        uint16_t _src = *src++;
        
        *dest++ = lumaChromaTable[_src & mask].y_n;
        *dest++ = lumaChromaTable[_src & mask].u_i_n;
        *dest++ = lumaChromaTable[_src & mask].v_q_n;
        *dest++ = _src >> metaShift; // aec, ba
    }

    src += srcPitch;
    dest += _dp;

    if (!cropTop)
        src -= width + srcPitch;            
    
    for (unsigned h = 0; h < height; h++) {
        for (unsigned w = 0; w < width; w++) {            
            _src = *src++;
            
			*dest++ = lumaChromaTable[_src & mask].y_n ;
            *dest++ = lumaChromaTable[_src & mask].u_i_n ;
            *dest++ = lumaChromaTable[_src & mask].v_q_n ;
			*dest++ = _src >> metaShift;
        }

        src += srcPitch;
        dest += _dp;
    }
}

auto VideoManager::updateCrtThreads() -> void {
    for (auto videoManager : videoManagers) {
        bool useRenderThread = false;

        if (videoManager == this) {
            if ((crtMode == CrtMode::Cpu) && crtThreaded)
                useRenderThread = true;
        }

        videoManager->enableCrtThread(useRenderThread);
    }
}

auto VideoManager::enableCrtThread( bool state) -> void {

    if (workerCreated == state)
        return;

    for( unsigned t = 0; t < 2; t++ ) {
        Render* re = &render[t];

        if (state) {
            while (re->kill) {
                std::this_thread::yield();
            }

            if (use16BitSrc)
                createWorker<true>( re );
            else
                createWorker<false>( re );

        } else {
            re->kill = true;
            re->cv.notify_one();
        }
    }

    workerCreated = state;
}

template<bool _16bitSrc> auto VideoManager::createWorker(Render* re) -> void {

    std::thread worker([this, re] {

        std::chrono::milliseconds duration(5);
        std::mutex cvM;
        std::unique_lock<std::mutex> lk(cvM);

        re->dest = nullptr;
        re->kill = false;

        while (1) {
            re->ready = false;

            while (!re->ready.load()) {

                if (re->kill) {
                    re->kill = false;
                    return;
                }

                if (re->cv.wait_for(lk, duration, [re]() { return re->ready.load(); }))
                    break;
            }
            if (_16bitSrc)
                renderCrtSelection<uint16_t>( re );
            else
                renderCrtSelection<uint8_t>( re );
        }
    });

    worker.detach();

}

auto VideoManager::waitForRenderer() -> bool {

	Render* re = &render[1];

	while (re->ready.load())
		std::this_thread::yield();

    return frameRenderPos == 0;
}

// 1. start 1. thread half screen
// 2. vblank: lock driver, start 2. thread
// 3. wait first thread and copy result, update render params for first thread
// 4. end vblank: wait 2. thread and unlock + redraw

// half screen
auto VideoManager::renderMidScreen( ) -> void {
	
	Render* re = &render[0];
	
	if (!colorTableUpdated)
		reinitCrtThread();
	
	if (!re->dest || frameRenderPos)
		return;	

	re->ready.store(1);
	re->cv.notify_one();	
}

auto VideoManager::reinitCrtThread( bool initMem ) -> void {

    Render* re = &render[0];
    re->dest = nullptr;

    if (initMem)
        std::memset(tempDest, 0, 512 * 768 * 4);
}

template<typename T> auto VideoManager::renderCrt(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch ) -> void {			    
	unsigned cropTop = this->emulator->cropTop();    
	Render* re = &render[0];
	re->width = width;
    re->height = height;
	re->srcPitch = srcPitch;
	re->destPitch = destPitch;
	re->oddLine = cropTop & 1;
	re->src = (uint8_t*)src;
	
	re->dest = dest;
	re->scanlineDest = nullptr;
	
	if (!pal)
		re->reuseFirstLine = false;
	else if (cropTop) {
		re->reuseFirstLine = false;
		if (use16BitSrc)
			re->src -= (re->width + re->srcPitch) << 1;
		else
			re->src -= re->width + re->srcPitch;
	} else
		re->reuseFirstLine = true;
	
	renderCrtSelection<T>( re );
}

template<typename T> inline auto VideoManager::renderCrtSelection(Render* re) -> void {
	
	bool rfModulation = useRfModulation();
	
	if (pal) {
		if (scanlines && rfModulation)			renderPalCrt<true, true, T>(re);
		else if (scanlines && !rfModulation)	renderPalCrt<true, false, T>(re);
		else if (!scanlines && !rfModulation)	renderPalCrt<false, false, T>(re);		
		else									renderPalCrt<false, true, T>(re);
	} else {
		if (scanlines && rfModulation)			renderNtscCrt<true, true, T>(re);
		else if (scanlines && !rfModulation)	renderNtscCrt<true, false, T>(re);
		else if (!scanlines && !rfModulation)	renderNtscCrt<false, false, T>(re);		
		else									renderNtscCrt<false, true, T>(re);
	}
}

auto VideoManager::useCrtMode() -> bool {
    
    return crtMode == CrtMode::Cpu || crtMode == CrtMode::Gpu;
}

auto VideoManager::usePostShading() -> bool {
	
	return maskLevel > 0.0 || radialDistortion > 0.0 || lightFromCenter > 0.0 || (maskLevel ? maskLuminance : luminance) != 1.0;
}

auto VideoManager::useRfModulation() -> bool {
	
	return lumaFall > 0.0 || lumaRise > 0.0;
}

auto VideoManager::useLineGlitch() -> bool {
	
	return baGlitch > 0.0 || aecGlitch > 0.0 || phi0Glitch > 0.0 || casGlitch > 0.0 || rasGlitch > 0.0;
}

template<typename T> auto VideoManager::renderCrtThreadedBlank(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch ) -> void {
    static unsigned scaler = (3.0 / 4.0) * 256.0;
    Render* re = &render[0];

    while (re->ready.load())
        std::this_thread::yield();

    unsigned cropTop = this->emulator->cropTop();
    unsigned heightFirstHalfScreen = (height * scaler) >> 8;
    unsigned destOffset = (width + destPitch) * (heightFirstHalfScreen << (scanlines ? 1 : 0));

    emulator->setLineCallback( true, cropTop + heightFirstHalfScreen );

    re->height = heightFirstHalfScreen;
    re->width = width;
    re->srcPitch = srcPitch;
    re->destPitch = destPitch;
    re->src = (uint8_t*)src;
    re->dest = nullptr;
    re->scanlineDest = nullptr;
    re->oddLine = cropTop & 1;

    if (!pal)
        re->reuseFirstLine = false;
    else if (cropTop) {
        re->reuseFirstLine = false;
        if (use16BitSrc)
            re->src -= (re->width + re->srcPitch) << 1;
        else
            re->src -= re->width + re->srcPitch;
    } else
        re->reuseFirstLine = true;
}

template<typename T> auto VideoManager::renderCrtThreaded(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch ) -> void {			    
    static unsigned scaler = (3.0 / 4.0) * 256.0;
	Render* re = &render[0];
	Render* re1 = &render[1];
	
    unsigned cropTop = this->emulator->cropTop();     
    unsigned heightFirstHalfScreen = (height * scaler) >> 8;
	unsigned destOffset = (width + destPitch) * (heightFirstHalfScreen << (scanlines ? 1 : 0));
    
    if (!re->dest) {
        while (re->ready.load())
            std::this_thread::yield();
        
        renderCrt( width, height, src, srcPitch, dest, destPitch );        
        re->dest = nullptr;
        
    } else {                
        re1->width = width;        
        re1->srcPitch = srcPitch;
        re1->destPitch = destPitch;
        re1->height = height - heightFirstHalfScreen;
		
		re1->dest = dest + destOffset;
        if (scanlines)
            destOffset -= width + destPitch;
        
		re1->scanlineDest = scanlines ? (dest + destOffset) : nullptr;

        while (re->ready.load())
            std::this_thread::yield();
        
		re1->src = re->src;
		
		re1->oddLine = re->oddLine;		
		
        re1->ready.store(1);
        re1->cv.notify_one();
	}
    
    emulator->setLineCallback( true, cropTop + heightFirstHalfScreen );
	
	//copy result to already locked gpu	
	if (re->dest)        
		std::memcpy(dest, tempDest, destOffset << 2);    
			
	re->height = heightFirstHalfScreen;
	re->width = width;        
	re->srcPitch = srcPitch;
	re->destPitch = destPitch;
	re->src = (uint8_t*)src;
	re->dest = tempDest;
	re->scanlineDest = nullptr;
	re->oddLine = cropTop & 1;
	
	if (!pal)
		re->reuseFirstLine = false;		
	else if (cropTop) {
		re->reuseFirstLine = false;	
		if (use16BitSrc)
			re->src -= (re->width + re->srcPitch) << 1;
		else
			re->src -= re->width + re->srcPitch;
	} else
		re->reuseFirstLine = true;			
}

template<bool withScanlines, bool rfModulation, typename T> auto VideoManager::renderPalCrt( Render* re ) -> void {
    
	static int32_t x1 = (int32_t) (((double) 1.0 / (double) 0.493) * double(1 << 8) + 0.5);
    static int32_t x2 = (int32_t) (((double) 1.0 / (double) 0.877) * double(1 << 8) + 0.5);
    static int32_t x3 = (int32_t) (0.3939307027516405140450117660881 * double(1 << 8) + 0.5);
    static int32_t x4 = (int32_t) (0.58080920903109757400461150856936 * double(1 << 8) + 0.5);

	bool secondHalf = re == &render[1];
	
	const T* _src = (T*)re->src;
	
	ColorLumaChroma* lineTable;
	ColorRgb* lineBeforeDest = nullptr;
	ColorRgb rgb;
	ColorLumaChroma yuv;	
	int32_t uSubSample, vSubSample;	
	
	if (!secondHalf) {
		_src -= 2;   

		if (re->reuseFirstLine)
			lineTable = oddTable;
		else
			lineTable = !re->oddLine ? oddTable : evenTable;

		uSubSample = lineTable[ _src[0] & mask ].u_i_s + lineTable[ _src[1] & mask ].u_i_s + lineTable[ _src[2] & mask ].u_i_s;
		vSubSample = lineTable[ _src[0] & mask ].v_q_s + lineTable[ _src[1] & mask ].v_q_s + lineTable[ _src[2] & mask ].v_q_s;

		// delay line	
		for (unsigned w = 0; w < re->width; w++) {
			uSubSample += lineTable[ _src[3] & mask ].u_i_s;
			vSubSample += lineTable[ _src[3] & mask ].v_q_s;

			delayLine[w].u_i_s = uSubSample;
			delayLine[w].v_q_s = vSubSample;

			uSubSample -= lineTable[ _src[0] & mask ].u_i_s;				
			vSubSample -= lineTable[ _src[0] & mask ].v_q_s;   

			_src++;
		}

		if (re->reuseFirstLine)
			_src -= re->width;
		else
			_src += re->srcPitch;                         	
	}
	
	for(unsigned h = 0; h < re->height; h++) {		
		
		lineTable = re->oddLine ? oddTable : evenTable;	
		lineBeforeDest = &lineBefore[0];
		
		uSubSample = lineTable[ _src[0] & mask ].u_i_s + lineTable[ _src[1] & mask ].u_i_s + lineTable[ _src[2] & mask ].u_i_s;
		vSubSample = lineTable[ _src[0] & mask ].v_q_s + lineTable[ _src[1] & mask ].v_q_s + lineTable[ _src[2] & mask ].v_q_s;
		
		for(unsigned w = 0; w < re->width; w++) {
						
			uSubSample += lineTable[ _src[3] & mask ].u_i_s;
			vSubSample += lineTable[ _src[3] & mask ].v_q_s;            
			
            yuv.u_i_s = uSubSample + delayLine[w].u_i_s;
			yuv.v_q_s = vSubSample + delayLine[w].v_q_s;

			if (!rfModulation)
				yuv.y_s = lineTable[ _src[1] & mask ].y_s_blur + lineTable[ _src[2] & mask ].y_s + lineTable[ _src[3] & mask ].y_s_blur;
            
			else {				
				uint16_t _pos = ((_src[-1] & mask) << 12) | ((_src[0] & mask) << 8) | ((_src[1] & mask) << 4) | (_src[2] & mask);
				uint16_t _posL = ((_src[-2] & mask) << 12) | ((_src[-1] & mask) << 8) | ((_src[0] & mask) << 4) | (_src[1] & mask);
				uint16_t _posR = ((_src[0] & mask) << 12) | ((_src[1] & mask) << 8) | ((_src[2] & mask) << 4) | (_src[3] & mask);
					 
				yuv.y_s = preCalcLumaNeighbour[_posL] + preCalcLumaCenter[_pos] + preCalcLumaNeighbour[_posR];
			}
            
            delayLine[w].u_i_s = uSubSample;
            delayLine[w].v_q_s = vSubSample;
            
            if (re->oddLine) {
                yuv.u_i_s = (yuv.u_i_s * this->hanoverBars) >> 7;
                yuv.v_q_s = (yuv.v_q_s * this->hanoverBars) >> 7;
                
            } else if (this->hanoverBarsAlt) {
                yuv.u_i_s = (yuv.u_i_s * this->hanoverBarsAlt) >> 7;
                yuv.v_q_s = (yuv.v_q_s * this->hanoverBarsAlt) >> 7;                
            }
                       
			int16_t r = (yuv.y_s + ((x2 * yuv.v_q_s) >> 8) + 1024) >> 11;
			int16_t g = (yuv.y_s - ((x3 * yuv.u_i_s + x4 * yuv.v_q_s) >> 8) + 1024) >> 11;
			int16_t b = (yuv.y_s + ((x1 * yuv.u_i_s) >> 8) + 1024) >> 11;

			*re->dest++ = 255 << 24 | preCalc[ r + 256 ] << 16 | preCalc[ g + 256 ] << 8 | preCalc[ b + 256 ];    

			if (withScanlines && re->scanlineDest) {
				*re->scanlineDest++ = 255 << 24 | preCalcScanline[ r + lineBeforeDest->rInt + 512 ] << 16
					| preCalcScanline[ g + lineBeforeDest->gInt + 512 ] << 8
					| preCalcScanline[ b + lineBeforeDest->bInt + 512 ];
			}

			lineBeforeDest->rInt = r;
			lineBeforeDest->gInt = g;
			lineBeforeDest->bInt = b;
			lineBeforeDest++;
			
			uSubSample -= lineTable[ _src[0] & mask ].u_i_s;				
			vSubSample -= lineTable[ _src[0] & mask ].v_q_s;				
				
			_src++;			
		}
		
		_src += re->srcPitch;	
		re->dest += re->destPitch;
		
		if (withScanlines) {
			re->scanlineDest = re->dest;
			re->dest +=	re->width + re->destPitch;			
		}
			
        re->oddLine ^= 1;
	}	
	
	re->src = (uint8_t*)_src;
}

template<bool withScanlines, bool rfModulation, typename T> auto VideoManager::renderNtscCrt( Render* re ) -> void {
	
	static int32_t x1 = (int32_t) (1.630 * double(1 << 8) + 0.5);
    static int32_t x2 = (int32_t) (0.317 * double(1 << 8) + 0.5);
    static int32_t x3 = (int32_t) (0.378 * double(1 << 8) + 0.5);
    static int32_t x4 = (int32_t) (0.466 * double(1 << 8) + 0.5);
	static int32_t x5 = (int32_t) (1.089 * double(1 << 8) + 0.5);
	static int32_t x6 = (int32_t) (1.677 * double(1 << 8) + 0.5);

	ColorRgb rgb;
	ColorLumaChroma yiq;	
	ColorRgb* lineBeforeDest = nullptr;
	
	int32_t iSubSample, qSubSample;	
	bool secondHalf = re == &render[1];
	const T* _src = (T*)re->src;
	
	if (!secondHalf) {
		_src -= 2;       	                       	
	}
	
	for(unsigned h = 0; h < re->height; h++) {				
		
		iSubSample = evenTable[ _src[0] & mask ].u_i_s + evenTable[ _src[1] & mask ].u_i_s + evenTable[ _src[2] & mask ].u_i_s;
		qSubSample = evenTable[ _src[0] & mask ].v_q_s + evenTable[ _src[1] & mask ].v_q_s + evenTable[ _src[2] & mask ].v_q_s;
		
		lineBeforeDest = &lineBefore[0];
		
		for(unsigned w = 0; w < re->width; w++) {
						
			iSubSample += evenTable[ _src[3] & mask ].u_i_s;
			qSubSample += evenTable[ _src[3] & mask ].v_q_s;            
			
			yiq.u_i_s = iSubSample;
			yiq.v_q_s = qSubSample;

			if (!rfModulation)
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

			*re->dest++ = 255 << 24 | preCalc[ r + 256 ] << 16 | preCalc[ g + 256 ] << 8 | preCalc[ b + 256 ];  

			if (withScanlines && re->scanlineDest) {
				*re->scanlineDest++ = 255 << 24 | preCalcScanline[ r + lineBeforeDest->rInt + 512 ] << 16
					| preCalcScanline[ g + lineBeforeDest->gInt + 512 ] << 8
					| preCalcScanline[ b + lineBeforeDest->bInt + 512 ];
			}

			lineBeforeDest->rInt = r;
			lineBeforeDest->gInt = g;
			lineBeforeDest->bInt = b;
			lineBeforeDest++;
			
			iSubSample -= evenTable[ _src[0] & mask ].u_i_s;				
			qSubSample -= evenTable[ _src[0] & mask ].v_q_s;				
				
			_src++;
		}
		
		_src += re->srcPitch;
		re->dest += re->destPitch;
		
		if (withScanlines) {
			re->scanlineDest = re->dest;
			re->dest +=	re->width + re->destPitch;			
		}
	}
	
	re->src = (uint8_t*)_src;
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

auto VideoManager::unlockDriver() -> void {
    if (!activeVideoManager)
        return;

    if (!activeVideoManager->crtThreaded || (activeVideoManager->crtMode != CrtMode::Cpu ) )
        return;

    if (!activeVideoManager->waitForRenderer() || !videoDriver)
        return;
    videoDriver->unlock();
    videoDriver->redraw();
}

auto VideoManager::powerOff() -> void {
    unlockDriver();         
	reinitCrtThread();
    currentHeight = 0;        
}

auto VideoManager::initFpsLimit() -> void {

    auto stat = emulator->getStatsForSelectedRegion();

    double fps = stat.fps;

    if ( globalSettings->get<bool>("video_override_exact", true) ) {
        // todo
    }

    minimumCapTime = (1000000.0 / fps) + 0.5;
    
    lastCapTime = Chronos::getTimestampInMicroseconds();
}

auto VideoManager::applyFpsLimit() -> void {
                   
    int64_t sleepMs  = ((lastCapTime + minimumCapTime) - (int64_t)Chronos::getTimestampInMicroseconds()) / 1000;    
    
    if (sleepMs > 0) {
        lastCapTime += minimumCapTime;
        
        GUIKIT::System::sleep( sleepMs );
        
        return;
    }    
    
    lastCapTime = Chronos::getTimestampInMicroseconds();
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

auto VideoManager::free() -> void {
    if (colorTable)
        delete[] colorTable;

    if (colorTableNoGamma)
        delete[] colorTableNoGamma;
        
    if (lumaChromaTable)
        delete[] lumaChromaTable;                
    
    if (evenTable)
        delete[] evenTable;
        
    if (oddTable)
        delete[] oddTable;
        
    evenTable = oddTable = lumaChromaTable = nullptr;
    colorTable = colorTableNoGamma = nullptr;
	
	if (tempDest)
		delete[] tempDest;
}

VideoManager::~VideoManager() {
    enableCrtThread(false);
    free();
}

template auto VideoManager::renderFrame<uint8_t>(const uint8_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;