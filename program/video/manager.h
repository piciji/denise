
#pragma once

#include "../../emulation/interface.h"
#include <atomic>
#include <thread>
#include <condition_variable>
#include "shader.h"

#define VPARAMS _useSpectrum, _crtMode, _region, _useInterlace, _interlace, \
    _saturation, _contrast, _gamma, _brightness, _phase, _usePhaseError, _phaseError,  \
    _newLuma, _tvGamma, _hanoverBars, _useHanoverBars, \
    _useBlur, _blur, _useScanlines, _scanlines, _useLumaRise, _lumaRise, _useLumaFall, _lumaFall, \
    _useChromaNoise, _chromaNoise, _useLumaNoise, _lumaNoise, _useRadialDistortion, _radialDistortion, \
    _useBloomGlow, _bloomGlow, _bloomVariance, _bloomRadius, _useBloomWeight, _bloomWeight, _useAecGlitch, _aecGlitch,  \
    _useBaGlitch, _baGlitch, _useRasGlitch, _rasGlitch, _useCasGlitch, _casGlitch, _usePhi0Glitch, _phi0Glitch, \
    _maskPitch, _maskDpi, _useMaskLevel, _maskLevel, _maskLuminance, _firFilterLength, _firFilterSharp, \
    _hires, _distortionHires, _maskType, _luminance, _useLightFromCenter, _lightFromCenter, \
    _useRandomLineOffset, _randomLineOffset

#define VPARAMST bool, unsigned, unsigned, bool, unsigned, \
    unsigned, unsigned, unsigned, unsigned, int, bool, float, \
    bool, bool, int, bool, \
    bool, unsigned, bool, unsigned, bool, float, bool, float, \
    bool, float, bool, float, bool, unsigned, \
    bool, unsigned, float, unsigned, bool, float, bool, float, \
    bool, float, bool, float, bool, float, bool, float, \
    float, unsigned, bool, unsigned, unsigned, unsigned, int, \
    bool, bool, unsigned, unsigned, bool, unsigned, \
    bool, float

struct ColorLumaChroma {
    // yuv / yiq (Sony CXA2025AS)
    double y;    
    double u_i;
    double v_q ;
    // yuv / yiq scaled to integer
    int32_t y_s;    
    int32_t y_s_blur;   
    int32_t u_i_s;
    int32_t v_q_s;
    
    float y_n;
    float u_i_n;
    float v_q_n;
};

struct ColorRgb {
    double r;
    double g;
    double b;
    
    int16_t rInt;
    int16_t gInt;
    int16_t bInt;
};

struct ColorRgbLight {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct VideoManager {
    VideoManager(Emulator::Interface* emulator);
    ~VideoManager();
        
	static bool synchronized;
    static bool crtThreaded;
    static bool shaderInputPrecision;
    static uint8_t frameRenderPos;
    static uint8_t frameRenderTrigger;
    static unsigned placeHolderFrames;
    static bool needAUpdate;
	
    static auto setCrtThreaded(bool state) -> void;
    static auto setShaderInputPrecision(bool state) -> void;
    static auto setFrameRender(uint8_t limit) -> void;
    static auto setSynchronize() -> void;
    static auto setHardSync() -> void;

    enum class MaskType : unsigned { Aperture = 0u, ShadowMask = 1u, SlotMask = 2u } maskType;
    enum class CrtMode : unsigned { None = 0u, Cpu = 1u, Gpu = 2u, GpuExtern = 3u } crtMode;

    struct DataUpdates {
        std::string ident;
        unsigned dataU;
        int dataI;
        float dataF;
        bool dataB;
    };

    std::vector<DataUpdates> dataUpdates;
    bool dataUpdatesPending;
    unsigned softwareViewForegroundColorRef;
    unsigned softwareViewBackgroundColorRef;

    struct Render {        
        unsigned width;
        unsigned height;
        const uint8_t* src;
        unsigned srcPitch;
        unsigned* dest;
        unsigned destPitch;
        unsigned* scanlineDest;
        unsigned* fieldDest;
        uint8_t oddLine;
        std::atomic<bool> ready;
        std::atomic<bool> kill;
        std::condition_variable cv;
        uint8_t options = 0;
    } render[2];
    bool workerCreated = false;

    uint32_t* tempDest = nullptr;
    uint32_t* tempDestHold = nullptr;
    ColorLumaChroma delayLine[ 1024 ];
	ColorRgb lineBefore[ 1024 ];
    
    Emulator::Interface* emulator;
    GUIKIT::Settings* settings;
    Emulator::Interface::Palette* palette;
    Shader shader;
    
    uint32_t* colorTable = nullptr;
    uint32_t* colorTableNoGamma = nullptr;
    auto reinitCrtThread( bool initMem = false ) -> void;
    auto resetTempData( int offset = 0, bool onlyIfUsed = false ) -> void;

    unsigned countColorBits;
	
    bool colorSpectrum;
    bool pal;
    unsigned interlaceDecay;
    
    double saturation;
    double contrast;
    double brightness;
    double gamma;           
    double phase; // at degree on color wheel
    bool newLuma;
    bool crtRealGamma;
    float luminance;
    float lightFromCenter;
    
    double phaseError;
    int32_t hanoverBars;
    int32_t hanoverBarsAlt;
        
    double blur;    
    unsigned currentHeight;
    uint8_t scanlines;
    
    double lumaRise;
    double lumaFall;
    
    float bloomWeight;
    unsigned bloomRadius;
    float bloomVariance;
    float bloomGlow;
    
    float maskPitch;
    unsigned maskDpi;
    float maskLevel;
    float maskLuminance;
    
    float radialDistortion;
    bool distortionHires;
    bool hires;
    float chromaNoise;
    float lumaNoise;
    float randomLineOffset;
    
    float aecGlitch;
    float baGlitch;
    float phi0Glitch;
    float rasGlitch;
    float casGlitch;
    
    unsigned firTaps;
    int firSharp;
    
    float preCalcF[256 * 3];
    uint8_t preCalc[256 * 3];
    uint8_t preCalcScanline[512 * 3];    
    float preCalcScanlineF[512 * 3];  
    
    // precalc blur for rf modulated luma change
    int32_t preCalcLumaCenter[0xffff + 1];
    int32_t preCalcLumaNeighbour[0xffff + 1];
    
    unsigned colorCount;
    ColorLumaChroma* lumaChromaTable = nullptr;
    ColorLumaChroma* evenTable = nullptr;
    ColorLumaChroma* oddTable = nullptr;

    bool rgbCable = false;
    bool colorTableUpdated = false;
    inline auto needUpdate() -> bool { return !colorTableUpdated; }
    auto requestUpdate(bool withShader = false) -> void;
    auto useCrtMode() -> bool;
 
    auto isC64() -> bool;
	auto isAmiga() -> bool;
    auto generateC64ColorSpectrum() -> void;
    auto getForegroundColor() -> unsigned;
    auto getBackgroundColor() -> unsigned;
       
    auto uclamp8(double x) -> uint8_t;
    auto convertRGBToYIQ(ColorLumaChroma* dest, ColorRgb* src) -> void;
    auto convertRGBToYUV(ColorLumaChroma* dest, ColorRgb* src) -> void;
    auto setPalette(Emulator::Interface::Palette* palette) -> void;  
    
    template<typename T, bool interlace = false, bool field = false> auto renderToLumaChroma(unsigned width, unsigned height, const T* src, unsigned srcPitch, float* dest, unsigned destPitch) -> void;
    template<typename T, bool interlace = false, bool field = false> auto renderToRgb(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch) -> void;
    template<typename T, bool interlace = false, bool field = false> inline auto renderToRgbNoGamma(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch) -> void;
    template<typename T, uint8_t options = 0> auto renderFrame(const T* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
    template<typename T> inline auto renderCrtSelection(Render& re) -> void;
    template<typename T, uint8_t options = 0> auto renderCrt(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch ) -> void;
    template<typename T, uint8_t options = 0> auto renderCrtThreaded(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch ) -> void;
    template<typename T> auto renderCrtThreadedBlank(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch ) -> void;
    template<uint8_t options> auto getRenderOptions() -> uint8_t;
    auto convertYUVToRGB(ColorRgb* dest, ColorLumaChroma* src) -> void;
    auto convertYIQToRGB(ColorRgb* dest, ColorLumaChroma* src) -> void;
    auto update() -> void;
    auto adjustPalette() -> void;
    auto free() -> void;
    auto adjustSaturation(double& r, double& g, double& b) -> void;
    auto adjustContrast(double& c) -> void;
    auto adjustGamma(double& c) -> void;
    auto adjustBrightness(double& c) -> void;
    auto convertLumaChromaToRGB() -> void;
    auto normalizeColorSpectrumPalGamma( double& color ) -> void;
    auto denormalizeColorSpectrumPalGamma( double& color ) -> void;
    auto updateListingColors() -> void;
    auto injectPhaseTransferError() -> void;
    auto convertLumaChromaToInteger() -> void;
    auto convertPaletteToLumaChroma() -> void;
    auto preCalcGamma() -> void;
    auto preCalcLumaDelay() -> void;
    template<typename T> auto createWorker(Render* re) -> void;
    auto enableCrtThread( bool state) -> void;
    auto updateCrtThreads() -> void;
	auto waitForCrtRenderer() -> void;
    template<uint8_t options, typename T> auto renderPalCrt( Render& re ) -> void;
    template<uint8_t options, typename T> auto renderNtscCrt( Render& re ) -> void;
    auto powerOff() -> void;
    template<uint8_t options> auto renderMidScreen() -> void;
    
    static auto getInstance( Emulator::Interface* emulator ) -> VideoManager*;
	static auto updateAll() -> void;
    static auto hidePlaceHolder() -> void;
    
    auto useLumaDelay() -> bool;
    auto useLineGlitch() -> bool;
    auto usePostShading() -> bool;
    // seter props
    auto usePal(bool state) -> void; // pal or ntsc
    auto useColorSpectrum(bool state) -> void; // color spectrum or palette
    auto setCrtMode(CrtMode _mode) -> void;
    
    auto setSaturation(unsigned saturation) -> void;
    auto setBrightness(unsigned brightness) -> void;
    auto setGamma(unsigned gamma) -> void;
    auto setContrast(unsigned contrast) -> void;
    auto setNewLuma(bool state) -> void;
    auto setCrtRealGamma(bool state) -> void;
    auto setPhase( int degree ) -> void;
    auto setPhaseError(float phaseError) -> void;
    auto setHanoverBars( int saturationDelta ) -> void;
    auto setBlur( unsigned blur ) -> void;
    auto setScanlines(unsigned intensity) -> void;
    auto setInterlace(unsigned intensity) -> void;
    
    auto setBloomGlow( unsigned intensity ) -> void;
    auto setBloomRadius( unsigned intensity ) -> void;
    auto setBloomVariance( float intensity ) -> void;
    auto setBloomWeight( float intensity ) -> void;
    auto setRadialDistortion( unsigned intensity ) -> void;
    auto setMaskPitch( float intensity ) -> void;
    auto setMaskDpi( unsigned intensity ) -> void;
    auto setMaskLevel( unsigned intensity ) -> void;
    auto setMaskLuminance( unsigned intensity ) -> void;
    auto setLumaNoise( float intensity ) -> void;
    auto setChromaNoise( float intensity ) -> void;
    auto setRandomLineOffset( float intensity ) -> void;
    auto setBaGlitch( float intensity ) -> void;
    auto setAecGlitch( float intensity ) -> void;
    auto setPhi0Glitch( float intensity ) -> void;
    auto setCasGlitch( float intensity ) -> void;
    auto setRasGlitch( float intensity ) -> void;
    auto setFirFilterLength( unsigned length ) -> void;
    auto setFirFilterSharp( int sharp ) -> void;
    auto smoothIntensity( float intensity ) -> float;
    
    auto setLumaRise( float pixel ) -> void;
    auto setLumaFall( float pixel ) -> void;
    
    auto setLightFromCenter( unsigned intensity ) -> void;
    auto setLuminance( unsigned intensity ) -> void;
    
    auto setMaskType(MaskType maskType) -> void;
    auto useDistortionHires(bool state) -> void;
    auto useHires(bool state) -> void;
    
    template<typename T> auto updateShader(std::string program, std::string attribute, T& target, T intensity, T activationValue = (T)0 ) -> void;
    auto reloadSettings() -> void;
    auto getSettings() -> std::tuple<VPARAMST>;   
    auto resetSettings() -> void;
    auto getModeIdent() -> std::string;
    auto applyMeta() -> void;

    template<typename T> auto updateData(std::string ident, T data) -> void;
    auto applyDataUpdates() -> void;
};

extern std::vector<VideoManager*> videoManagers;
