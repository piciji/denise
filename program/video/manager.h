
#pragma once

#include "../../emulation/interface.h"
#include "../../driver/tools/shaderpass.h"

#define VPARAMS _useSpectrum, _crtMode, _region, _useInterlace, _interlace, \
    _saturation, _contrast, _gamma, _brightness, _phase, _usePhaseError, _phaseError,  \
    _newLuma, _hanoverBars, _useHanoverBars, \
    _useBlur, _blur, _useScanlines, _scanlines, _useLumaRise, _lumaRise, _useLumaFall, _lumaFall

#define VPARAMST unsigned, unsigned, unsigned, bool, unsigned, \
    unsigned, unsigned, unsigned, unsigned, int, bool, float, \
    bool, int, bool, \
    bool, unsigned, bool, unsigned, bool, float, bool, float

struct ShaderParser;
struct DmaColor;

namespace GUIKIT {
    struct Settings;
}

struct ColorLumaChroma {
    // yuv / yiq (Sony CXA2025AS)
    double y;    
    double u_i;
    double v_q;
    double uOdd;
    double vOdd;

    // yuv / yiq scaled to integer
    int32_t y_s;
    int32_t y_s_blur;
    int32_t u_i_s;
    int32_t v_q_s;
};

struct ColorRgb {
    double r;
    double g;
    double b;
    
    int16_t rInt;
    int16_t gInt;
    int16_t bInt;
};

struct RGBDescriptor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct C64ColorSpectrum {
    double luminance;
    double angle;
    double amplitude;

    double angleOdd;
    double amplitudeOdd;
    double angleEven;
    double amplitudeEven;
};

struct VideoManager {
    VideoManager(Emulator::Interface* emulator);
    ~VideoManager();

    bool rebuildShader = true;
    ShaderParser* parser = nullptr;
        
	static bool synchronized;
    static uint8_t frameRenderPos;
    static uint8_t frameRenderTrigger;
    static bool placeHolder;
    static bool needAUpdate;
    static unsigned takeScreenShots;

    static auto setFrameRender(uint8_t limit) -> void;
    static auto setSynchronize() -> void;
    static auto setHardSync() -> void;
    static auto unloadDataStorage() -> void;

    enum class CrtMode : unsigned { None = 0u, Cpu = 1u, Gpu = 2u } crtMode;

    struct DataUpdates {
        std::string ident;
        int offset;
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
        unsigned options = 0;
    } render;

    uint32_t* tempDest = nullptr;
    ColorLumaChroma delayLine[ 1024 ];
	ColorRgb lineBefore[ 1024 ];
    
    Emulator::Interface* emulator;
    GUIKIT::Settings* settings;
    Emulator::Interface::Palette* palette;
    
    uint32_t* colorTable = nullptr;
    uint32_t* colorTableRGB10Even = nullptr;
    uint32_t* colorTableRGB10Odd = nullptr;

    unsigned countColorBits;
	
    unsigned colorSpectrum;
    bool pal;
    unsigned interlaceDecay;
    bool interlaceFields;
    
    double saturation;
    double contrast;
    double brightness;
    double gamma;           
    double phase; // at degree on color wheel
    bool newLuma;

    double phaseError;
    int32_t hanoverBars;
    int32_t hanoverBarsAlt;
        
    double blur;    
    unsigned currentHeight;
    uint8_t scanlines;
    
    double lumaRise;
    double lumaFall;

    uint8_t preCalcGamma[256 * 3];
    uint8_t preCalcScanline[512 * 3];

    int32_t preCalcLumaCenter[0xffff + 1];
    int32_t preCalcLumaNeighbour[0xffff + 1];
    
    unsigned colorCount;
    ColorLumaChroma* lumaChromaTable = nullptr;
    ColorLumaChroma* evenTable = nullptr;
    ColorLumaChroma* oddTable = nullptr;

    DmaColor* dmaColors = nullptr;

    ShaderPreset::Param* driveLedParam = nullptr;
    uint8_t frameOptions = 0;
    bool colorTableUpdated = false;
    auto needUpdate() -> bool { return !colorTableUpdated; }
    auto requestUpdate() -> void;
 
    auto isC64() -> bool;
	auto isAmiga() -> bool;
    auto generateC64ColorSpectrum() -> void;
    auto getForegroundColor() -> unsigned;
    auto getBackgroundColor() -> unsigned;
       
    static auto uclamp8(double x) -> uint8_t;
    static auto uclamp10(double x) -> uint16_t;
    static auto convertRGBToYIQ(ColorLumaChroma* dest, ColorRgb* src) -> void;
    static auto convertRGBToYUV(ColorLumaChroma* dest, ColorRgb* src) -> void;
    auto setPalette(Emulator::Interface::Palette* palette) -> void;

    template<typename T, bool interlace = false, bool field = false> auto renderToLumaChroma(unsigned width, unsigned height, const T* src, unsigned srcPitch, uint32_t* dest, unsigned destPitch, bool odd) -> void;
    template<typename T, bool interlace = false, bool field = false> auto renderToRgb(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch) -> void;
    template<typename T, bool interlace = false, bool field = false> auto renderToRgbWithDma(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch) -> void;
    template<typename T> auto renderToScreenshot(unsigned width, unsigned height, const T* src, unsigned srcPitch, uint8_t* dest, uint8_t _options) -> void;
    template<typename T, uint8_t options = 0> auto renderFrame(const T* src, unsigned width, unsigned height, unsigned srcPitch) -> void;
    template<typename T, uint8_t options = 0> auto renderCrt(unsigned width, unsigned height, const T* src, unsigned srcPitch, unsigned* dest, unsigned destPitch, unsigned& cropTop ) -> void;
    template<uint8_t options> auto getRenderOptions() -> unsigned;
    auto convertYUVToRGB(ColorRgb* dest, ColorLumaChroma* src, bool odd) -> void;
    auto convertYIQToRGB(ColorRgb* dest, ColorLumaChroma* src, bool odd) -> void;
    auto update() -> void;
    auto adjustPalette() -> void;
    auto free() -> void;
    auto adjustSaturation(double& r, double& g, double& b) -> void;
    auto adjustContrast(double& c) -> void;
    auto adjustGamma(double& c) -> void;
    auto adjustBrightness(double& c) -> void;
    auto convertLumaChromaToRGB() -> void;
    static auto normalizeColorSpectrumPalGamma( double& color ) -> void;
    auto updateListingColors() -> void;
    auto injectPhaseTransferError() -> void;
    auto convertLumaChromaToInteger() -> void;
    auto convertPaletteToLumaChroma() -> void;
    auto calculateGamma() -> void;
    auto calculateLumaDelay() -> void;
    template<uint8_t options, typename T> auto renderPalCrt() -> void;
    template<uint8_t options, typename T> auto renderNtscCrt() -> void;
    auto powerOff() -> void;

    static auto getInstance( Emulator::Interface* emulator ) -> VideoManager*;
	static auto updateAll() -> void;
    auto useLumaDelay() -> bool;
    auto useRegionEncoding() -> bool;
    // seter props
    auto usePal(bool state) -> void; // pal or ntsc
    auto useColorSpectrum(unsigned state) -> void; // color spectrum or palette
    auto setCrtMode(CrtMode _mode) -> void;
    
    auto setSaturation(unsigned saturation) -> void;
    auto setBrightness(unsigned brightness) -> void;
    auto setGamma(unsigned gamma) -> void;
    auto setContrast(unsigned contrast) -> void;
    auto setNewLuma(bool state) -> void;
    auto setPhase( int degree ) -> void;
    auto setPhaseError(float phaseError) -> void;
    auto setHanoverBars( int saturationDelta ) -> void;
    auto setBlur( unsigned blur ) -> void;
    auto setScanlines(unsigned intensity) -> void;
    auto setInterlace(unsigned intensity) -> void;
    auto setInterlaceFields(bool state) -> void;

    auto setLumaRise( float pixel ) -> void;
    auto setLumaFall( float pixel ) -> void;

    auto reloadSettings(bool reloadPreset) -> void;
    auto getSettings() -> std::tuple<VPARAMST>;   
    auto resetSettings() -> void;
    auto getModeIdent() -> std::string;
    auto applyMeta() -> void;

    auto updateData(int offset, float data) -> void;
    template<typename T> auto updateData(std::string ident, T data) -> void;
    auto setData(const std::string& ident, float value) -> void;
    auto setData( unsigned offset, float value) -> void;
    auto getData(const std::string& ident) -> ShaderPreset::Param*;
    auto applyDataUpdates() -> void;

    auto loadPreset(const std::string& path, std::vector<std::string>& errors) -> ShaderPreset*;
    auto loadPreset(const std::string& path) -> void;
    auto loadPreset() -> bool;

    auto addPreset(std::string path, bool prepend, std::vector<std::string>& errors) -> ShaderPreset*;
    auto savePreset(std::string path) -> bool;
    auto getPreset() -> ShaderPreset*;
    auto getPreset(std::vector<std::string>& errors) -> ShaderPreset*;
    auto getPresetPath() -> std::string;
    auto getPresetPathDetailed() -> std::string;
    auto clearPreset() -> void;
    auto finishPreset() -> void;
    auto movePass(unsigned& passId, bool up) -> void;
    auto togglePassUsage(unsigned passId) -> ShaderPreset::Pass*;
    auto setPassFilter(unsigned passId, ShaderPreset::Filter filter) -> void;
    auto setPassMipmap(unsigned passId, bool state) -> void;
    auto setPassScaleX(unsigned passId, float scale) -> void;
    auto setPassScaleY(unsigned passId, float scale) -> void;
    auto shaderLumaChromaInput() -> bool;
    auto translateShaderBufferType(ShaderPreset::BufferType& bufferType) -> const std::string;
    auto getColorSpectrum(unsigned id, unsigned col) -> C64ColorSpectrum&;
    auto lumaChromaMode() -> bool;

    auto fetchShader(ShaderPreset::Pass& pass, unsigned passId) -> bool;
    template<typename T> auto takeScreenshot(unsigned unscaled, const T* _src, unsigned _width, unsigned _height, unsigned _pitch, uint8_t _options) -> void;
};

extern std::vector<VideoManager*> videoManagers;
