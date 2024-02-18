
#pragma once

#include <string>
#include <functional>
#include "../driver.h"

struct SpirvReflection;

namespace DRIVER {

struct ShaderCache {

    ShaderCache(const std::string& type) : type(type) {}
    ~ShaderCache() { clear(); }

    std::string type;
    std::string cacheFile;

    uint8_t* cacheData = nullptr;
    uint8_t* nativeV = nullptr;
    uint8_t* nativeF = nullptr;
    unsigned vertexSize = 0;
    unsigned fragmentSize = 0;

    std::function<void (DiskFile& diskFile)> onShaderCacheCallback = nullptr;

    auto write(uint8_t* vs, unsigned vsSize, uint8_t* ps, unsigned psSize, SpirvReflection& reflection) -> void;
    auto read(std::string& vertex, std::string& fragment, SpirvReflection& reflection) -> bool;
    auto fetchTextures(std::string& textures, SpirvReflection& reflection) -> bool;
    auto fetchUniforms(std::string& uniforms, bool push, SpirvReflection& reflection) -> bool;
    auto clear() -> void;

    auto trim(std::string& str) -> std::string&;
    auto explode(std::string str, const std::string& delimiter) -> std::vector<std::string>;
    auto removeFromBeginning(std::vector<std::string>& vec, unsigned elements) -> void;
};

}
