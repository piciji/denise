
#pragma once

#include <vector>
#include <string>
#include "../../guikit/api.h"

#define SHADER_OFFSCREEN_WIDTH 4 // same value for left and right side

struct VideoManager;
struct ShaderPass;

struct Shader {        
    
    VideoManager* vManager;
    std::vector<ShaderPass*> externalPasses;
    std::vector<ShaderPass*> internalPasses;
    std::vector<ShaderPass*> internalPassesPost;

    ShaderPass* primaryPass = nullptr;
    bool externalLoaded = false;
    bool recreate = true;
    GUIKIT::Image imageAperture;
    GUIKIT::Image imageShadowMask;
    GUIKIT::Image imageSlotMask;    
    
    std::vector<std::string> loadErrors;
    
    auto sendToDriver(bool retry = false) -> bool;
    auto getPrimary(std::vector<ShaderPass*>& passes) -> ShaderPass*;
    auto loadInternal() -> void;
    auto loadExternal() -> bool;
    auto addActiveShader(std::string shader) -> void;
    auto removeActiveShader(std::string shader) -> void;
    auto getActiveShaders() -> std::vector<std::string>;
    auto mapPass(GUIKIT::Setting* theme, ShaderPass* pass, std::string path = "") -> void;
    auto loadShader(std::string path, std::string shaderFile, ShaderPass* pass) -> std::string;
    auto clean(std::vector<ShaderPass*>& passes) -> void;
    auto removeIncompleteShader() -> void;
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
    
    auto buildOutputEncoding() -> std::string;
    auto buildBandwidthReduction() -> std::string;
    auto buildDelayLineAndConvertToRgb() -> std::string;
    auto buildNoise() -> std::string;
    auto buildLumaLatency() -> std::string;    
    auto buildRadialDistortion() -> std::string;
    auto buildMask() -> std::string;
    auto buildRandomLineOffset() -> std::string;
    auto buildBloom(bool phase1) -> std::string;
    
    auto buildGamma() -> std::string;
    auto buildMaskTexture() -> void;

    auto buildGammaAndScanlines() -> std::string;
    auto addBaseProps( ShaderPass* pass ) -> void;
    auto calcRadialScale(float intensity) -> float;    
    
    auto normaliseDimension( unsigned& widthScale, unsigned& heightScale ) -> void;
        
    Shader(VideoManager* vManager);
    ~Shader();
};
