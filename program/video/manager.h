
#pragma once

#include "../../emulation/interface.h"
#include <atomic>
#include <thread>
#include <condition_variable>
#include "shader.h"

#define VPARAMS _useSpectrum, _crtMode, _region, \
    _saturation, _contrast, _gamma, _brightness, _phase, _usePhaseError, _phaseError,  \
    _newLuma, _crtRealGamma, _hanoverBars, _useHanoverBars, \
    _useBlur, _blur, _useScanlines, _scanlines, _useLumaRise, _lumaRise, _useLumaFall, _lumaFall, \
    _useChromaNoise, _chromaNoise, _useLumaNoise, _lumaNoise, _useRadialDistortion, _radialDistortion, \
    _useBloomGlow, _bloomGlow, _bloomVariance, _bloomRadius, _useBloomWeight, _bloomWeight, _useAecGlitch, _aecGlitch,  \
    _useBaGlitch, _baGlitch, _useRasGlitch, _rasGlitch, _useCasGlitch, _casGlitch, _usePhi0Glitch, _phi0Glitch, \
    _maskPitch, _maskDpi, _useMaskLevel, _maskLevel, _maskLuminance, _firFilterLength, _firFilterSharp, \
    _hires, _distortionHires, _maskType, _luminance, _useLightFromCenter, _lightFromCenter, \
    _useRandomLineOffset, _randomLineOffset

#define VPARAMST bool, unsigned, unsigned, \
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

struct VideoManager {
    VideoManager(Emulator::Interface* emulator);
    ~VideoManager();
        
	static bool synchronized;
    static bool threaded;
    static bool shaderInputPrecision;
	static bool integerScaling;
	static bool aspectCorrect;
    static bool fpsLimit;
	
    static auto setThreaded(bool state) -> void;
    static auto setShaderInputPrecision(bool state) -> void;
    static auto setAspectCorrect(bool state) -> void;
    static auto setIntegerScaling(bool state) -> void;
    static auto setFpsLimit(bool state) -> void;
    
    enum class MaskType : unsigned { Aperture = 0u, ShadowMask = 1u, SlotMask = 2u } maskType;
    enum class CrtMode : unsigned { None = 0u, Cpu = 1u, Gpu = 2u } crtMode;
    
    struct Render {        
        unsigned width;
        unsigned height;
        const uint8_t* src;
        unsigned srcPitch;
        unsigned* dest;
        unsigned destPitch;
        unsigned* scanlineDest;
        bool oddLine;
        bool reuseFirstLine;
        std::atomic<bool> ready;
        std::condition_variable cv;
    } render[2];  
	
    unsigned scalingCount = 0;
    
    uint32_t* tempDest = nullptr;
    ColorLumaChroma delayLine[ 512 ];
	ColorRgb lineBefore[ 512 ];
    
    Emulator::Interface* emulator;
    GUIKIT::Settings* settings;
    Emulator::Interface::Palette* palette;
    Shader shader;
    
    uint32_t* colorTable = nullptr;
    uint32_t* colorTableNoGamma = nullptr;
    auto reinitThread( bool initMem = false ) -> void;
        
    uint16_t mask;
	uint8_t metaShift;
	bool use16BitSrc;
	
    bool colorSpectrum;
    bool pal;  
    
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
    
    int64_t lastCapTime;
    int64_t minimumCapTime;    
    
    bool colorTableUpdated = false;
    inline auto needUpdate() -> bool { return !colorTableUpdated; }
    auto useCrtMode() -> bool;
 
    auto isC64() -> bool;
	auto isAmiga() -> bool;
    auto generateC64ColorSpectrum() -> void;
    auto getC64Foreground() -> unsigned;
    auto getC64Background() -> unsigned;
       
    auto uclamp8(double x) -> uint8_t;
    auto convertRGBToYIQ(ColorLumaChroma* dest, ColorRgb* src) -> void;
    auto convertRGBToYUV(ColorLumaChroma* dest, ColorRgb* src) -> void;
    auto setPalette(Emulator::Interface::Palette* palette) -> void;  
    
    template<typename T> auto renderToLumaChroma(unsigned width, unsigned height, const T* src, unsigned srcPitch, float* dest, unsigned destPitch) -> void;
    template<typename T> auto renderToRgb(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch) -> void;
    template<typename T> inline auto renderToRgbNoGamma(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch) -> void;
    template<typename T> auto renderFrame(const T* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
    template<typename T> inline auto renderCrtSelection(Render* re) -> void;
    template<typename T> auto renderCrt(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch ) -> void;
    template<typename T> auto renderCrtThreaded(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch ) -> void;
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
    auto preCalcRfModulation() -> void;
    template<bool _16bitSrc> auto createWorker() -> void;
    auto waitForRenderer() -> void;
    template<bool withScanlines, bool rfModulation, typename T> auto renderPalCrt( Render* re ) -> void;
    template<bool withScanlines, bool rfModulation, typename T> auto renderNtscCrt( Render* re ) -> void;
    auto powerOff() -> void;    
    auto renderMidScreen( ) -> void;
    
    static auto getInstance( Emulator::Interface* emulator ) -> VideoManager*;
    static auto unlockDriver() -> void;
	static auto updateWhenNotRunning() -> void;    
    
    auto useRfModulation() -> bool;
    auto useLineGlitch() -> bool;
    auto usePostShading() -> bool;
    // seter props
    auto usePal(bool state) -> void; // pal or ntsc
    auto useColorSpectrum(bool state) -> void; // color spectrum or palette
    auto setCrtMode(CrtMode _mode) -> void;
    
    auto setSaturation(int saturation) -> void;
    auto setBrightness(int brightness) -> void;
    auto setGamma(int gamma) -> void;
    auto setContrast(int contrast) -> void;
    auto setNewLuma(bool state) -> void;
    auto setCrtRealGamma(bool state) -> void;
    auto setPhase( int degree ) -> void;
    auto setPhaseError(double phaseError) -> void;
    auto setHanoverBars( int saturationDelta ) -> void;
    auto setBlur( unsigned blur ) -> void;
    auto setScanlines(unsigned intensity) -> void;
    
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
    
    auto applyFpsLimit() -> void;
    auto initFpsLimit() -> void;
};

extern std::vector<VideoManager*> videoManagers;
