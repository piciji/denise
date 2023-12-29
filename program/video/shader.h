
#pragma once

#include <vector>
#include <string>
#include "../../guikit/api.h"
#include "../../driver/driver.h"
#include "shaderParser.h"

typedef DRIVER::Video::ShaderType ShaderFormat;
#define SHADER_OFFSCREEN_WIDTH 4 // same value for left and right side

struct VideoManager;
struct ShaderPass;

struct Shader {

    VideoManager* vManager;
    std::vector<ShaderPass*> internalPasses;

    ShaderPass* primaryPass = nullptr;
    bool recreate = true;
    bool lace = false;
    GUIKIT::Image imageAperture;
    GUIKIT::Image imageShadowMask;
    GUIKIT::Image imageSlotMask;
    ShaderParser* parser = nullptr;
    
    std::vector<std::string> loadErrors;
    
    auto sendToDriver(bool retry = false) -> bool;
    auto getPrimary(std::vector<ShaderPass*>& passes) -> ShaderPass*;
    auto loadInternal() -> void;
    auto clean(std::vector<ShaderPass*>& passes) -> void;
    auto setAttribute(std::string program, std::string attribute, float value) -> void;
    auto setAttribute(std::string program, std::string attribute, int value) -> void;

    auto transferDataToShader() -> void;
    auto transferDelayLine() -> void;
    auto transferOutputEncoding() -> void;
    auto transferGammaAndScanlines() -> void;
    auto transferNoise() -> void;
    auto transferLumaLatency() -> void;
    auto transferRadialDistortion() -> void;
    auto transferMask() -> void;
    auto transferMaskTexture() -> void;
    auto transferLuminance() -> void;   
    auto transferRandomLine() -> void;
    auto transferBloom() -> void;
    
    auto buildOutputEncoding(ShaderFormat& format) -> std::string;
    auto buildOutputEncodingGLSL() -> std::string;
    auto buildOutputEncodingHLSL() -> std::string;

    auto buildBandwidthReduction(ShaderFormat& format) -> std::string;
    auto buildBandwidthReductionGLSL() -> std::string;
    auto buildBandwidthReductionHLSL() -> std::string;
    auto buildDelayLineAndConvertToRgb(ShaderFormat& format) -> std::string;
    auto buildDelayLineAndConvertToRgbGLSL() -> std::string;
    auto buildDelayLineAndConvertToRgbHLSL() -> std::string;
    auto buildNoise(ShaderFormat& format) -> std::string;
    auto buildNoiseGLSL() -> std::string;
    auto buildNoiseHLSL() -> std::string;
    auto buildLumaLatency(ShaderFormat& format) -> std::string;
    auto buildLumaLatencyGLSL() -> std::string;
    auto buildLumaLatencyHLSL() -> std::string;
    auto buildRadialDistortion(ShaderFormat& format) -> std::string;
    auto buildRadialDistortionGLSL() -> std::string;
    auto buildRadialDistortionHLSL() -> std::string;
    auto buildMask(ShaderFormat& format) -> std::string;
    auto buildMaskGLSL() -> std::string;
    auto buildMaskHLSL() -> std::string;
    auto buildRandomLineOffset(ShaderFormat& format) -> std::string;
    auto buildRandomLineOffsetGLSL() -> std::string;
    auto buildRandomLineOffsetHLSL() -> std::string;
    auto buildBloom(ShaderFormat& format, bool phase1) -> std::string;
    auto buildBloomGLSL(bool phase1) -> std::string;
    auto buildBloomHLSL(bool phase1) -> std::string;
    
    auto buildGamma(ShaderFormat& format) -> std::string;
    auto buildGammaGLSL() -> std::string;
    auto buildGammaHLSL() -> std::string;
    auto buildMaskTexture() -> void;

    auto buildGammaAndScanlines(ShaderFormat& format) -> std::string;
    auto buildGammaAndScanlinesGLSL() -> std::string;
    auto buildGammaAndScanlinesHLSL() -> std::string;
    auto addBaseProps( ShaderPass* pass ) -> void;
    auto calcRadialScale(float intensity) -> float;    
    
    auto normaliseDimension( unsigned& widthScale, unsigned& heightScale ) -> void;

    auto loadPreset(const std::string& path, std::vector<std::string>& brokenPaths) -> ShaderPreset*;
    auto addPreset(std::string path, bool prepend, std::vector<std::string>& brokenPaths) -> ShaderPreset*;
    auto savePreset(std::string path) -> bool;
    auto getPreset() -> ShaderPreset*;
    auto getPreset(std::vector<std::string>& brokenPaths) -> ShaderPreset*;
    auto clearPreset() -> void;
    auto getPresetPath() -> std::string;
    auto getPresetPathCombined() -> std::string;
    auto movePass(unsigned& passId, bool up) -> void;
    auto togglePassUsage(unsigned passId) -> ShaderPreset::Pass*;
    auto setPassFilter(unsigned passId, ShaderPreset::Filter filter) -> void;
        
    Shader(VideoManager* vManager);
    ~Shader();
};
