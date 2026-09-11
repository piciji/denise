
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
#include "../helper/settingsHelper.h"
#include "sync.cpp"
#include "../debugger/dmaDebugger.h"
#include "../tools/colors.h"
#include "../tools/dataStorage.h"
#include "../emuconfig/layouts/presentation.h"
#include "scVideo.h"

#include "../tools/chronos.h"
#include "../view/status.h"

uint8_t VideoManager::frameRenderPos = 0;
uint8_t VideoManager::frameRenderTrigger = 1;
bool VideoManager::needUpdateForAllInstances = true;
unsigned VideoManager::takeScreenShots = 0;

std::vector<VideoManager*> videoManagers;

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
    needUpdateForAllInstances = false;
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
    this->settings = Program::getSettings(emulator);
    this->palette = &emulator->palettes[0];        
    this->colorCount = this->palette->paletteColors.size();

    countColorBits = 0;
	
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
    brightness = 0.0;
	saturation = 1.0;

    colorTableUpdated = false;
    dataUpdatesPending = false;

    lumaChromaTable = new ColorLumaChroma[this->colorCount];
    colorTable = new uint32_t[this->colorCount];
    colorTableRGB10Even = new uint32_t[this->colorCount];
    colorTableRGB10Odd = new uint32_t[this->colorCount];
    
    newLuma = true;
	phase = 0.0;
	pal = true;
    colorSpectrum = 0;
    legacyCRTonCPU = false;
    suppressShaderByHotkey = false;
    scanlines = 0;

    phaseError = 22.5; 
    hanoverBars = (int32_t)(0.8 * 128.0); // 20% saturation loss
    hanoverBarsAlt = 0;

    currentHeight = 0;
	
	lumaRise = 1.0 / 2.0;
	lumaFall = 1.0 / 1.2;

    parser = new ShaderParser;
    scVideo = new SCVideo(this);
}

VideoManager::~VideoManager() {
    free();
    delete parser;
    delete scVideo;
}

auto VideoManager::update() -> void {

    if (!colorSpectrum || !isC64() ) { // palette
        if ( lumaChromaMode() ) {
            convertPaletteToLumaChroma();

            convertLumaChromaToRGB();
        } else
            adjustPalette();
        
    } else {
        generateC64ColorSpectrum();
		
        convertLumaChromaToRGB();
    }

    if (legacyCRTonCPU)
        scVideo->update();
    
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
    static double radian = M_PI / 180.0;

	double con = contrast;

    if (colorSpectrum == 2) // colodore
        con *= 1.2;
	
    double angle;
    
    for (unsigned c = 0; c < colorCount; c++) {
              
		ColorLumaChroma* lumaChroma = &lumaChromaTable[c];

        C64ColorSpectrum& colSpec = getColorSpectrum(colorSpectrum - 1, c);

        lumaChroma->y = ( colSpec.luminance + brightness ) * con;
        
        lumaChroma->u_i = lumaChroma->v_q = 0.0;
        lumaChroma->uOdd = lumaChroma->vOdd = 0.0;
        
        if (colSpec.amplitude == 0.0)
            continue; // luma only ... black, white and grey shades

		if (pal) {
            if (lumaChromaMode() && (colorSpectrum == 1)) {
		        angle = (colSpec.angleEven + phase ) * radian;
		        lumaChroma->u_i = (std::cos(angle) * colSpec.amplitudeEven * saturation) * con;
		        lumaChroma->v_q = (std::sin(angle) * colSpec.amplitudeEven * saturation) * con;
                angle = (colSpec.angleOdd + phase ) * radian;
                lumaChroma->uOdd = (std::cos(angle) * colSpec.amplitudeOdd * saturation) * con;
                lumaChroma->vOdd = (std::sin(angle) * colSpec.amplitudeOdd * saturation) * con;
		    } else {
		        angle = (colSpec.angle + phase ) * radian;
		        lumaChroma->u_i = (std::cos(angle) * colSpec.amplitude * saturation) * con;
		        lumaChroma->v_q = (std::sin(angle) * colSpec.amplitude * saturation) * con;
		        lumaChroma->uOdd = lumaChroma->u_i;
		        lumaChroma->vOdd = lumaChroma->v_q;
		    }
		} else {
			// yiq is 33 degree rotated
		    if (lumaChromaMode() && (colorSpectrum == 1)) {
		        angle = (colSpec.angleEven + phase - (100.0 / 3.0) ) * radian;
		        lumaChroma->u_i = (std::sin(angle) * colSpec.amplitudeEven * saturation) * con;
		        lumaChroma->v_q = (std::cos(angle) * colSpec.amplitudeEven * saturation) * con;
		    } else {
		        angle = (colSpec.angle + phase - (100.0 / 3.0) ) * radian;
		        lumaChroma->u_i = (std::sin(angle) * colSpec.amplitude * saturation) * con;
		        lumaChroma->v_q = (std::cos(angle) * colSpec.amplitude * saturation) * con;
		    }

		    lumaChroma->uOdd = lumaChroma->u_i;
		    lumaChroma->vOdd = lumaChroma->v_q;
        }
    }
}

auto VideoManager::lumaChromaMode() -> bool {
    return legacyCRTonCPU || shaderRgb10BitInput();
}

auto VideoManager::normalizeColorSpectrumPalGamma( double& color ) -> void {
    if (color < 0.0)
        color = 0.0;

	color = std::pow(255, 1 - 2.8) * std::pow(color, 2.8);
	
	color = std::pow(255, 1 - (1.0 / 2.2)) * std::pow(color, 1.0 / 2.2 );
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

		ColorLumaChroma* lumaChroma = &lumaChromaTable[c];
		
		if (pal)
			convertRGBToYUV( lumaChroma, &rgb);
		else
			convertRGBToYIQ( lumaChroma, &rgb);
		
		lumaChroma->u_i = (lumaChroma->u_i * saturation) * contrast;
		lumaChroma->v_q = (lumaChroma->v_q * saturation) * contrast;
        lumaChroma->uOdd = lumaChroma->u_i;
        lumaChroma->vOdd = lumaChroma->v_q;
		lumaChroma->y = (lumaChroma->y + brightness) * contrast;
	}
}

auto VideoManager::convertLumaChromaToRGB() -> void {
    ColorRgb rgbOdd;
    ColorRgb rgbEven;
	
    for (unsigned c = 0; c < colorCount; c++) {
        
        if (pal) {
            convertYUVToRGB( &rgbOdd, &lumaChromaTable[c], true );
            convertYUVToRGB( &rgbEven, &lumaChromaTable[c], false );
        } else {
            convertYIQToRGB( &rgbEven, &lumaChromaTable[c], false );
            rgbOdd = rgbEven;
        }

        colorTableRGB10Odd[c] = (uclamp10( rgbOdd.r + 256.0) << 20) | (uclamp10( rgbOdd.g + 256.0 ) << 10) | uclamp10( rgbOdd.b + 256.0 );
        colorTableRGB10Even[c] = (uclamp10( rgbEven.r + 256.0) << 20) | (uclamp10( rgbEven.g + 256.0 ) << 10) | uclamp10( rgbEven.b + 256.0 );

        if (pal && (colorSpectrum == 2)) {
            normalizeColorSpectrumPalGamma(rgbEven.r);
            normalizeColorSpectrumPalGamma(rgbEven.g);
            normalizeColorSpectrumPalGamma(rgbEven.b);
        }

        adjustGamma(rgbEven.r);
        adjustGamma(rgbEven.g);
        adjustGamma(rgbEven.b);
		
		colorTable[c] = 255 << 24 | uclamp8( rgbEven.r ) << 16 | uclamp8( rgbEven.g ) << 8 | uclamp8( rgbEven.b );
    }
}

inline auto VideoManager::adjustBrightness(double& c) -> void {
    
    c += brightness;
}

auto VideoManager::adjustGamma(double& c) -> void {
    
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

auto VideoManager::toggleShaderTemporary() -> bool {
    suppressShaderByHotkey ^= 1;
    return suppressShaderByHotkey;
}

template<typename T, uint8_t options> auto VideoManager::renderFrame(const T* src, unsigned width, unsigned height, unsigned srcPitch ) -> void {
	unsigned gpuPitch;
    unsigned* gpuData;
    unsigned cropTop, cropLeft;
    constexpr bool interlace = options & 3;
    constexpr bool field = options & 2;
    constexpr bool hires = options & 4;
    constexpr bool shres = options & 8;
    constexpr bool isPause = options & 0x10;
    constexpr bool lores = !hires && !shres;
    bool iHold = interlace && !field && !interlaceFields;
    bool suppressShader = suppressShaderByHotkey
    || (
        program->warp.disableShader && ((program->warp.mode == Program::Warp::Normal) || (program->warp.mode == Program::Warp::Aggressive))
    );
    bool rewind = audioManager->rewind;
    uint8_t gpuOptions = iHold | (interlace << 1) | (suppressShader << 2) | (isPause << 5) | (rewind << 6);

    if (needUpdateForAllInstances)
        updateAll();

    bool cropCoordUpdated = emulator->cropCoordUpdated(cropTop, cropLeft);
    if (rebuildShader) {
        if (!legacyCRTonCPU) {
            auto appData = videoDriver->getAppData();
            if (appData) {
                appData->cropTop = (float)cropTop;
                appData->cropLeft = (float)cropLeft;
                appData->lace = (float)interlace;
                appData->hires = (float)hires;
                appData->pal = (float)pal;
                appData->subRegion = (float)emulator->getSubRegion();
                appData->flags = (float)( pal && (colorSpectrum == 2));
            }

            videoDriver->setShader( &parser->shaderPreset);
        } else
            videoDriver->setShader(nullptr);

        rebuildShader  = false;
    } else if (cropCoordUpdated) {
        auto appData = videoDriver->getAppData();
        if (appData) {
            appData->cropTop = (float)cropTop;
            appData->cropLeft = (float)cropLeft;
        }
    }

    frameOptions &= ~0x80; // init lace toggle
    if (frameOptions != ( (hires << 1) | interlace) ) {
        frameOptions = 0;
        if (interlace && ((frameOptions & 1) == 0) ) {
            gpuOptions &= ~1; // prevent hold in first lace frame
            frameOptions |= 0x80; // lace off -> on
            iHold = false;
        }

        auto appData = videoDriver->getAppData();
        if (appData) {
            appData->lace = (float)interlace;
            appData->hires = (float)hires;
        }

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

            if (!--takeScreenShots && screenshot.pause)
                program->isPause = screenshot.pause;
        }
    }

    frameRenderPos = 0;
    videoDriver->setIntegerScalingDimension( lores ? (width << 1) : (hires ? width : (width >> 1)), interlace ? height : (height << 1), shres | hires | interlace);

    if (dmaColors) {
        gpuOptions |= (uint8_t)DRIVER::OPT_DisallowShader | (uint8_t)DRIVER::OPT_DisallowFilter;

        if (!videoDriver->lock(gpuData, gpuPitch, width, height, gpuOptions))
            return;

        renderToRgbWithDma<T, interlace, field>(width, height, src, srcPitch, gpuData, gpuPitch - width);

    } else if (suppressShader) {
        goto Typical;

    } else if (!legacyCRTonCPU) {

        if (shaderRgb10BitInput()) {
            if (!videoDriver->lock(gpuData, gpuPitch, width, height, gpuOptions | (uint8_t)DRIVER::OPT_RGB10 ))
                goto Typical;

            renderToLumaChroma<T, interlace, field>(width, height, src, srcPitch, gpuData, gpuPitch - width, cropTop & 1);

        } else {
Typical:
            if (!videoDriver->lock(gpuData, gpuPitch, width, height, gpuOptions))
                return;

            renderToRgb<T, interlace, field>(width, height, src, srcPitch, gpuData, gpuPitch - width);
        }
	} else { // old legacy CRT over CPU
        if (!videoDriver->lock(gpuData, gpuPitch, width, (scanlines && !interlace) ? (height << 1) : height, gpuOptions))
            return;

        scVideo->renderCrt<T>(width, height, src, srcPitch, iHold ? nullptr : gpuData, gpuPitch - width, cropTop, getRenderOptions<options>());
	}

    if (audioManager->lumaInterference.enabled)
        sampleLuma(width, height, (uint8_t*)src, srcPitch);

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

template<typename T, bool interlace, bool field> auto VideoManager::renderToRgbWithDma(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch) -> void {
    unsigned mask = (1 << countColorBits) - 1;
    uint8_t* dmaDump = emulator->getDmaDump();
    if (!dmaDump)
        return renderToRgb<T, interlace, field>(width, height, src, srcPitch, dest, destPitch);

    unsigned color;
    unsigned dmaColor;
    DmaColor* dmaPtr;

    for(unsigned h = 0; h < height; h++) {
        for(unsigned w = 0; w < width; w++) {
            dmaPtr = &dmaColors[*dmaDump++];

            if (dmaPtr->enabled) {
                color = colorTable[ *src++ & mask ];
                dmaColor = dmaPtr->color;
                *dest++ = ColorTools::mix(dmaColor, color, dmaPtr->alpha);
            } else
                *dest++ = colorTable[ *src++ & mask ];
        }

        src += srcPitch;
        dmaDump += srcPitch;
        dest += destPitch;
    }
}

template<typename T, bool interlace, bool field> inline auto VideoManager::renderToRgb(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch) -> void {
    unsigned mask = (1 << countColorBits) - 1;

    if constexpr (interlace) {
        unsigned color;
        RGBDescriptor colorDecayed;
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

auto VideoManager::sampleLuma(unsigned width, unsigned height, const uint8_t* src, unsigned srcPitch) -> void {
    unsigned mask = (1 << countColorBits) - 1;
    float lum;
    auto& ali = audioManager->lumaInterference;

    for(unsigned h = 0; h < height; h++) {
        lum = 0.0;
        for(unsigned w = 0; w < width; w++) {
            lum += static_cast<float>(lumaChromaTable[*src++ & mask].y);
        }

        ali.avgLines[h] = lum / (static_cast<float>(width) * 3.0f);
        src += srcPitch;
    }

    lum = 0.0;
    for(unsigned h = 0; h < height; h++)
        lum += ali.avgLines[h];

    ali.avgFrame = lum / static_cast<float>(height);

    ali.avgLines[height] = 0.0;
    ali.lines = height + 1;
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
        RGBDescriptor colorDecayed;
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

template<typename T, bool interlace, bool field> auto VideoManager::renderToLumaChroma(unsigned width, unsigned height, const T* src, unsigned srcPitch, uint32_t* dest, unsigned destPitch, bool odd) -> void {
    T color;
    unsigned metaShift = countColorBits;
    unsigned mask = (1 << metaShift) - 1;
    bool laceToggle = !!(frameOptions & 0x80);
    uint32_t* colorTableRGB10;

    if (interlace && !field && !laceToggle && !interlaceFields) // hold -> update full frames only
        return;

    if (interlace && !laceToggle) {
        float iRate = (float)(100 - interlaceDecay);
        uint32_t _c;
        uint16_t _r, _g, _b;

        for (unsigned h = 0; h < height; h++) {
            colorTableRGB10 = odd ? colorTableRGB10Odd : colorTableRGB10Even;

            if (field == (h & 1)) {
                for (unsigned w = 0; w < width; w++) {
                    color = *src++;
                    *dest++ = colorTableRGB10[ color & mask ] | (color >> metaShift) << 30;
                }
            } else {
                for (unsigned w = 0; w < width; w++) {
                    color = *src++;
                    _c = colorTableRGB10[color & mask];

                    _r = (_c >> 20) & 0x3ff;
                    _g = (_c >> 10) & 0x3ff;
                    _b = (_c >> 0) & 0x3ff;

                    _r = (_r * iRate) / 100;
                    _g = (_g * iRate) / 100;
                    _b = (_b * iRate) / 100;

                    *dest++ = _r << 20 | _g << 10 | _b | (color >> metaShift) << 30;
                }
            }

            src += srcPitch;
            dest += destPitch;
            odd = !odd;
        }
    } else {
        for (unsigned h = 0; h < height; h++) {
            colorTableRGB10 = odd ? colorTableRGB10Odd : colorTableRGB10Even;

            for (unsigned w = 0; w < width; w++) {
                color = *src++;
                *dest++ = colorTableRGB10[ color & mask ] | (color >> metaShift) << 30;
            }

            src += srcPitch;
            dest += destPitch;
            odd = !odd;
        }
    }
}

auto VideoManager::useLumaDelay() -> bool {

	return lumaFall > 0.0 || lumaRise > 0.0;
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

auto VideoManager::convertYUVToRGB(ColorRgb* dest, ColorLumaChroma* src, bool odd) -> void {
    if (odd) {
        dest->r = src->y + (1.0 / 0.877) * src->vOdd;
        dest->g = src->y - 0.3939307027516405140450117660881 * src->uOdd - 0.58080920903109757400461150856936 * src->vOdd;
        dest->b = src->y + (1.0 / 0.493) * src->uOdd;
    } else {
        dest->r = src->y + (1.0 / 0.877) * src->v_q;
        dest->g = src->y - 0.3939307027516405140450117660881 * src->u_i - 0.58080920903109757400461150856936 * src->v_q;
        dest->b = src->y + (1.0 / 0.493) * src->u_i;
    }
}

// sony decoder matrix
auto VideoManager::convertYIQToRGB(ColorRgb* dest, ColorLumaChroma* src, bool odd) -> void {
    if (odd) {
        dest->r = src->y + 1.630 * src->uOdd + 0.317 * src->vOdd;
        dest->g = src->y - 0.378 * src->uOdd - 0.466 * src->vOdd;
        dest->b = src->y - 1.089 * src->uOdd + 1.677 * src->vOdd;
    } else {
        dest->r = src->y + 1.630 * src->u_i + 0.317 * src->v_q;
        dest->g = src->y - 0.378 * src->u_i - 0.466 * src->v_q;
        dest->b = src->y - 1.089 * src->u_i + 1.677 * src->v_q;
    }
}

auto VideoManager::powerOff() -> void {
    currentHeight = 0;
}

auto VideoManager::updateData(int offset, float data) -> void {
    DataUpdates dataUpdate;
    dataUpdate.offset = offset;
    dataUpdate.dataF = data;

    emuThread->lockVideo();
    dataUpdates.push_back( dataUpdate );
    dataUpdatesPending = true;
    needUpdateForAllInstances = true;
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
    needUpdateForAllInstances = true;
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
    }
}

auto VideoManager::isC64() -> bool {    
    return dynamic_cast<LIBC64::Interface*>(emulator);
}

auto VideoManager::isAmiga() -> bool {    
    return dynamic_cast<LIBAMI::Interface*>(emulator);
}

auto VideoManager::uclamp8(double x) -> uint8_t {
    return std::min( std::max((int)(x + 0.5), 0), 255 );
}

inline auto VideoManager::uclamp10(double x) -> uint16_t {
    return std::min( std::max((int)(x + 0.5), 0), 1023 );
}

template<uint8_t options> auto VideoManager::getRenderOptions() -> unsigned {
    unsigned out = 0;
    constexpr bool interlace = options & 3;
    constexpr bool field = options & 2;
    constexpr bool hires = options & 4;
    constexpr bool shres = options & 8;

    if (scanlines && !interlace) out |= 1; // suppress user requested scanlines if software requests interlace
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

auto VideoManager::free() -> void {
    delete[] colorTable;
    delete[] colorTableRGB10Odd;
    delete[] colorTableRGB10Even;
    delete[] lumaChromaTable;
    
    lumaChromaTable = nullptr;
    colorTable = nullptr;
    colorTableRGB10Odd = nullptr;
    colorTableRGB10Even = nullptr;
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
    suppressShaderByHotkey = false;
    std::vector<std::string> errors;
    loadPreset(path, errors);
}

auto VideoManager::loadPreset(const std::string& path, std::vector<std::string>& errors) -> ShaderPreset* {
    suppressShaderByHotkey = false;
    ShaderParser* tempParser = new ShaderParser;

    bool res = tempParser->loadPreset(path);
    GUIKIT::Vector::combine(errors, tempParser->errors);

    if (!res) {
        delete tempParser;
        return nullptr;
    }

    if (settings->get<bool>("prepend_yuv_shader", dynamic_cast<LIBC64::Interface*>(emulator) )) {
        if (!tempParser->hasYUVPrepend()) {
            ShaderParser* tempParserPre = new ShaderParser;
            std::string _emuIdent = emulator->ident;
            if (tempParserPre->loadPreset(ShaderParser::internalPresetFolder() + GUIKIT::String::toLowerCase(_emuIdent) + "_yuv.slangp"))
                tempParser->addPreset( tempParserPre, true );
            delete tempParserPre;
        }
    }

    // important, parameter changes when temp parser replaces current parser.
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

    return &parser->shaderPreset;
}

auto VideoManager::addPreset(std::string path, bool prepend, std::vector<std::string>& errors) -> ShaderPreset* {
    suppressShaderByHotkey = false;
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

auto VideoManager::shaderRgb10BitInput() -> bool {
    return parser->shaderPreset.rgb10BitInput;
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

auto VideoManager::getColorSpectrum(unsigned id, unsigned col) -> C64ColorSpectrum& {

    static C64ColorSpectrum c64ColorSpectrum[2][2][16] = {
    { // PALette
            { // 8565
                { 0.000 * 256.0,    0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                { 1.000 * 256.0,    0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                { 0.306 * 256.0,  102.50, 0.214 * 256.0, 93.50, 0.212 * 256.0, 111.50, 0.215 * 256.0},
                { 0.639 * 256.0,  281.50, 0.216 * 256.0, 273.00, 0.215 * 256.0, 290.00, 0.217 * 256.0},
                { 0.363 * 256.0,   51.00, 0.214 * 256.0, 43.00, 0.214 * 256.0, 59.00, 0.213 * 256.0},
                { 0.500 * 256.0,  238.70, 0.214 * 256.0, 231.70, 0.216 * 256.0, 245.70, 0.212 * 256.0},
                { 0.237 * 256.0,  345.10, 0.214 * 256.0, -24.40, 0.215 * 256.0, 354.60, 0.213 * 256.0},
                { 0.763 * 256.0,  165.10, 0.214 * 256.0, 169.60, 0.215 * 256.0, 160.60, 0.214 * 256.0},
                { 0.363 * 256.0,  126.50, 0.213 * 256.0, 120.50, 0.211 * 256.0, 132.50, 0.215 * 256.0},
                { 0.237 * 256.0,  146.00, 0.141 * 256.0, 146.50, 0.140 * 256.0, 145.50, 0.142 * 256.0},
                { 0.500 * 256.0,  102.50, 0.214 * 256.0, 93.50, 0.212 * 256.0, 111.50, 0.215 * 256.0},
                { 0.306 * 256.0,    0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                { 0.461 * 256.0,    0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                { 0.763 * 256.0,  238.70, 0.214 * 256.0, 231.70, 0.216 * 256.0, 245.70, 0.212 * 256.0},
                { 0.461 * 256.0,  345.10, 0.214 * 256.0, -24.40, 0.215 * 256.0, 354.60, 0.213 * 256.0},
                { 0.639 * 256.0,    0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0}
            },{ // 6569R1
                { 0.000 * 256.0,    0.00,  0.000 * 256.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                { 1.000 * 256.0,    0.00,  0.000 * 256.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                { 0.237 * 256.0,   95.50,  0.217 * 256.0, 89.00, 0.202 * 256.0, 102.00, 0.232 * 256.0},
                { 0.763 * 256.0,  275.50, 0.210 * 256.0, 269.25, 0.191 * 256.0, 281.75, 0.228 * 256.0},
                { 0.500 * 256.0,   54.00, 0.219 * 256.0, 48.50, 0.226 * 256.0, 59.50, 0.211 * 256.0},
                { 0.500 * 256.0,  241.70,  0.213 * 256.0, 235.45, 0.222 * 256.0, 247.95, 0.205 * 256.0},
                { 0.237 * 256.0,  355.60,  0.217 * 256.0, -12.40, 0.234 * 256.0, 363.60, 0.200 * 256.0},
                { 0.763 * 256.0,  175.60, 0.211 * 256.0, 168.60, 0.231 * 256.0, 182.60, 0.191 * 256.0},
                { 0.500 * 256.0,  128.25, 0.217 * 256.0, 122.00, 0.213 * 256.0, 134.50, 0.221 * 256.0},
                { 0.237 * 256.0,  146.50,  0.140 * 256.0, 140.00, 0.153 * 256.0, 153.00, 0.131 * 256.0},
                { 0.500 * 256.0,   95.50,  0.217 * 256.0, 89.00, 0.202 * 256.0, 102.00, 0.232 * 256.0},
                { 0.237 * 256.0,    0.00,  0.000 * 256.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                { 0.500 * 256.0,    0.00,  0.000 * 256.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                { 0.763 * 256.0,  241.70,  0.213 * 256.0, 235.45, 0.222 * 256.0, 247.95, 0.205 * 256.0},
                { 0.500 * 256.0,  355.60,  0.217 * 256.0, -12.40, 0.234 * 256.0, 363.60, 0.200 * 256.0},
                { 0.763 * 256.0,    0.00,  0.000 * 256.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0}
            }
        },{ // Colodore
            { // 8565
                {0.0 * 8.0, 0.0, 0.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {32.0 * 8.0, 0.0, 0.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {10.0 * 8.0, 101.25, 40.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {20.0 * 8.0, 281.25, 40.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {12.0 * 8.0, 56.25, 40.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {16.0 * 8.0, 236.25, 40.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {8.0 * 8.0, 348.75, 40.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {24.0 * 8.0, 168.75, 40.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {12.0 * 8.0, 123.75, 40.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {8.0 * 8.0, 146.25, 40.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {16.0 * 8.0, 101.25, 40.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {10.0 * 8.0, 0.0, 0.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {15.0 * 8.0, 0.0, 0.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {24.0 * 8.0, 236.25, 40.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {15.0 * 8.0, 348.75, 40.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {20.0 * 8.0, 0.0, 0.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
            },{ // 6569R1
                {0.0 * 8.0, 0.0, 0.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {32.0 * 8.0, 0.0, 0.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {8.0 * 8.0, 101.25, 40.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {24.0 * 8.0, 281.25, 40.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {16.0 * 8.0, 56.25, 40.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {16.0 * 8.0, 236.25, 40.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {8.0 * 8.0, 348.75, 40.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {24.0 * 8.0, 168.75, 40.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {16.0 * 8.0, 123.75, 40.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {8.0 * 8.0, 146.25, 40.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {16.0 * 8.0, 101.25, 40.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {8.0 * 8.0, 0.0, 0.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {16.0 * 8.0, 0.0, 0.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {24.0 * 8.0, 236.25, 40.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {16.0 * 8.0, 348.75, 40.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
                {24.0 * 8.0, 0.0, 0.0, 0.00, 0.000 * 256.0, 0.00, 0.000 * 256.0},
            }
        }
    };

    return c64ColorSpectrum[id & 1][newLuma ? 0 : 1][col & 15];
}

auto VideoManager::usePal(bool state) -> void {
	pal = state;
    requestUpdate();

    auto appData = videoDriver->getAppData();
    if (appData) {
        appData->pal = (float)pal;
        appData->subRegion = (float)emulator->getSubRegion();
    }
}

auto VideoManager::useColorSpectrum(unsigned state) -> void {
    colorSpectrum = !isC64() ? 0 : state;
    requestUpdate();
}

auto VideoManager::setLegacyCrtMode(bool state) -> void {

    if (this->legacyCRTonCPU != state)
        rebuildShader = true;

    this->legacyCRTonCPU = state;
    requestUpdate();
}

auto VideoManager::setPalette(Emulator::Interface::Palette* palette) -> void {
    this->palette = palette;

    if (!colorSpectrum)
        requestUpdate();
}

auto VideoManager::setSaturation(unsigned saturation) -> void {
    this->saturation = (double)saturation / 100.0;
    requestUpdate();
}

auto VideoManager::setContrast(unsigned contrast) -> void {
    this->contrast = (double)contrast / 100.0;
    requestUpdate();
}

auto VideoManager::setBrightness(unsigned brightness) -> void {
    this->brightness = (double)brightness - 100.0;
    requestUpdate();
}

auto VideoManager::setGamma(unsigned gamma) -> void {
    this->gamma = (double)gamma / 100.0;
    requestUpdate();
}

auto VideoManager::setNewLuma(bool state) -> void {
    newLuma = state;
    requestUpdate();
}

auto VideoManager::setPhase( int degree ) -> void {
    phase = degree;
    requestUpdate();
}

auto VideoManager::setPhaseError( float phaseError ) -> void {
    this->phaseError = (double)phaseError;
    requestUpdate();
}

auto VideoManager::setHanoverBars( int hanoverBars ) -> void {
    int _oddSat = -1 * std::abs(hanoverBars);
    this->hanoverBars = (int32_t)(((double)(100 + _oddSat) / 100.0) * 128.0);
    this->hanoverBarsAlt = 0;

    if (hanoverBars > 0)
        this->hanoverBarsAlt = (int32_t)(((double)(100 + hanoverBars) / 100.0) * 128.0);

    requestUpdate();
}

auto VideoManager::setBlur( unsigned blur ) -> void {
    this->blur = (double)blur / 50.0;
    requestUpdate();
}

auto VideoManager::setLumaRise( float pixel ) -> void {
    lumaRise = pixel == 0.0 ? 0.0 : (double)(1.0 / (double)pixel);
    requestUpdate();
}

auto VideoManager::setLumaFall( float pixel ) -> void {
    lumaFall = pixel == 0.0 ? 0.0 : (double)(1.0 / (double)pixel);
    requestUpdate();
}

auto VideoManager::setScanlines(unsigned intensity) -> void {
    scanlines = intensity;
    requestUpdate();
}

auto VideoManager::setInterlace(unsigned intensity) -> void {
    interlaceDecay = intensity;
    requestUpdate();
}

auto VideoManager::setInterlaceFields(bool state) -> void {
    interlaceFields = state;
    requestUpdate();
}

auto VideoManager::setData(const std::string& ident, float value) -> void {
    if (activeEmulator != emulator)
        rebuildShader = true;
    else
        for(auto& param : parser->shaderPreset.params) {
            if (param.id == ident) {
              //  videoDriver->waitRenderThread();
                param.value = value;
                break;
            }
        }
}

auto VideoManager::setData( unsigned offset, float value) -> void {
    if (activeEmulator != emulator)
        rebuildShader = true;
    else {
        auto& params = parser->shaderPreset.params;
        if (offset < params.size()) {
            // videoDriver->waitRenderThread();
            params[offset].value = value;
        }
    }
}

auto VideoManager::getData(const std::string& ident) -> ShaderPreset::Param* {
    for(auto& param : parser->shaderPreset.params) {
        if (param.id == ident)
            return &param;
    }
    return nullptr;
}

auto VideoManager::resetSettings() -> void {
    settings->remove( "video_new_luma" );
    settings->remove( "video_saturation" );
    settings->remove( "video_brightness" );
    settings->remove( "video_gamma" );
    settings->remove( "video_contrast" );
    settings->remove( "video_phase" );
    settings->remove( "video_interlace_use" );
    settings->remove( "video_interlace" );
}

auto VideoManager::resetLegacySettings() -> void {
    settings->remove( "video_hanover_bars" );
    settings->remove( "video_hanover_bars_use" );
    settings->remove( "video_phase_error_use" );
    settings->remove( "video_phase_error" );
    settings->remove( "video_scanlines_use" );
    settings->remove( "video_scanlines" );
    settings->remove( "video_blur_use" );
    settings->remove( "video_blur" );
    settings->remove( "video_luma_rise_use" );
    settings->remove( "video_luma_rise" );
    settings->remove( "video_luma_fall_use" );
    settings->remove( "video_luma_fall" );
}

auto VideoManager::getSettings() -> std::tuple<VPARAMST> {
    unsigned _useSpectrum = settings->get<unsigned>("video_spectrum", 1);
	unsigned _region = emulator->getRegionEncoding();
	bool _pal = _region == Emulator::Interface::Region::Pal;

    bool _legacyCrtMode = settings->get<bool>("video_crt_legacy", false);
    bool moreError = isC64();

    unsigned _saturation = settings->get<unsigned>("video_saturation", 100u,{0u, 200u});
    unsigned _contrast = settings->get<unsigned>("video_contrast", 100u,{0u, 200u});
    unsigned _gamma = settings->get<unsigned>("video_gamma", 100u,{30u, 280u});
    unsigned _brightness = settings->get<unsigned>("video_brightness", 100u,{0, 200u});
    int _phase = settings->get<int>("video_phase", 0,{-180, 180});
    float _phaseError = settings->get<float>("video_phase_error", _pal ? (moreError ? 22.5f : 3.5f ) : 0, {-45.0, 45.0});
    bool _usePhaseError = settings->get<bool>("video_phase_error_use", true);
    bool _newLuma = settings->get<bool>("video_new_luma", true);
    int _hanoverBars = settings->get<int>("video_hanover_bars", -10, {-100, 100});
    bool _useHanoverBars = settings->get<bool>("video_hanover_bars_use", true);
    unsigned _blur = settings->get<unsigned>("video_blur", 30,{0, 100});
    bool _useBlur = settings->get<bool>("video_blur_use", true);
    bool _useScanlines = settings->get<bool>("video_scanlines_use", false);
    unsigned _scanlines = settings->get<unsigned>("video_scanlines", 33, {0, 100});
    bool _useInterlace = settings->get<bool>("video_interlace_use", true);
    unsigned _interlace = settings->get<unsigned>("video_interlace", 0, {0u, 100});

    bool _useLumaRise = settings->get<bool>("video_luma_rise_use", moreError);
	float _lumaRise = settings->get<float>("video_luma_rise", 2.0, {1.0, 4.0});
	bool _useLumaFall = settings->get<bool>("video_luma_fall_use", moreError);
	float _lumaFall = settings->get<float>("video_luma_fall", 1.2, {1.0, 4.0});

    return std::make_tuple( VPARAMS);
}

auto VideoManager::reloadSettings(bool reloadPreset) -> void {
    auto [VPARAMS] = getSettings();

    setSaturation(_saturation);
    setContrast(_contrast);
    setBrightness(_brightness);
    setGamma(_gamma);
    setPhase(_phase);
    setNewLuma(_newLuma);
    setPhaseError(_usePhaseError ? _phaseError : 0 );
    setHanoverBars( _useHanoverBars ? _hanoverBars : 0);
    setScanlines(_useScanlines ? _scanlines : 0);
    setInterlace(_useInterlace ? _interlace : 0);
    setInterlaceFields( _useInterlace );
    setBlur( _useBlur ? _blur : 0 );
	setLumaRise( _useLumaRise ? _lumaRise : 0.0 );
	setLumaFall( _useLumaFall ? _lumaFall : 0.0 );

    usePal(_region == 0);
    useColorSpectrum(_useSpectrum);
    setLegacyCrtMode( _legacyCrtMode );

    auto appData = videoDriver->getAppData();
    if (appData)
        appData->flags = (float)( pal && (colorSpectrum == 2));

    if (reloadPreset)
        loadPreset();

    applyMeta();
}

auto VideoManager::applyMeta() -> void {
    emulator->videoAddMeta( !legacyCRTonCPU && parser->needMetaData() );
}

auto VideoManager::requestUpdate() -> void {
    colorTableUpdated = false;
    needUpdateForAllInstances = true;
}

template auto VideoManager::renderFrame<uint8_t>(const uint8_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint8_t, 0x10>(const uint8_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;

template auto VideoManager::renderFrame<uint16_t, 0>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 1>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 2>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 4>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 5>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 6>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 8>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 9>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 10>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;

template auto VideoManager::renderFrame<uint16_t, 0x10 | 0>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 0x10 | 1>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 0x10 | 2>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 0x10 | 4>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 0x10 | 5>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 0x10 | 6>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 0x10 | 8>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 0x10 | 9>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
template auto VideoManager::renderFrame<uint16_t, 0x10 | 10>(const uint16_t* src, unsigned width, unsigned height, unsigned srcPitch) -> void;

template auto VideoManager::updateData<bool>(std::string ident, bool data) -> void;
template auto VideoManager::updateData<int>(std::string ident, int data) -> void;
template auto VideoManager::updateData<unsigned>(std::string ident, unsigned data) -> void;
template auto VideoManager::updateData<float>(std::string ident, float data) -> void;
