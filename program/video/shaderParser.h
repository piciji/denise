
#pragma once
#include <string>
#include "../../guikit/api.h"
#include "../../driver/driver.h"

struct DataStorage;

struct ShaderParser {
    ShaderParser();
    ~ShaderParser();

    enum class Stage { Vertex, Fragment };

    GUIKIT::Settings rootSettings;
    ShaderPreset shaderPreset;
    std::vector<std::string> entryPaths;
    std::vector<std::string> errors;
    bool modified;
    static DataStorage* dataStorage;

    auto loadPreset(std::string path) -> bool;

    auto addPreset(ShaderParser* parser, bool prepend) -> bool;

    auto savePreset(std::string path) -> bool;

    auto findRootConfig(GUIKIT::Settings& settings, int depth = 0) -> bool;

    auto parsePass(unsigned pos) -> bool;

    auto translateWrapMode(std::string wrap) -> ShaderPreset::WrapMode;
    auto translateWrapMode(ShaderPreset::WrapMode wrapMode) -> std::string;

    auto translateScaleType(std::string scaleType) -> ShaderPreset::ScaleType;
    auto translateScaleType(ShaderPreset::ScaleType scaleType) -> std::string;

    auto translateBufferType(ShaderPreset::BufferType& type ) ->  const std::string;
    auto translateBufferType(const std::string& str ) -> ShaderPreset::BufferType;

    auto translateFilter(int filter) -> ShaderPreset::Filter;

    auto parseTextures() -> void;

    auto addParameter(ShaderPreset::Param& param) -> void;

    auto createSinglePass(std::string& path) -> bool;

    auto applyOverrides(std::string& path, std::vector<GUIKIT::Settings*>& settingsList, int depth = 0) -> void;

    auto getPresetPath() -> std::string { return entryPaths.size() ? entryPaths[0] : ""; }

    auto getPresetPathDetailed() -> std::string;

    auto movePass(unsigned& passId, bool up) -> void;

    auto togglePassUsage(unsigned passId) -> ShaderPreset::Pass*;

    auto setPassFilter(unsigned passId, ShaderPreset::Filter filter) -> void;
    auto setPassMipmap(unsigned passId, bool state) -> void;

    auto setPassScaleX(unsigned passId, float scale) -> void;
    auto setPassScaleY(unsigned passId, float scale) -> void;

    auto writeLine(FILE* fp, unsigned passId, std::string key, std::string value) -> void;
    auto writeLine(FILE* fp, std::string key, std::string value) -> void;

    auto needMetaData() -> bool;

    auto clear() -> void;

    auto checkLUT(ShaderPreset::Lut& lut) -> bool;
    auto addBrokenLUT() -> void;
    auto updateCrop() -> void;

    auto fetchShaderSource(const std::string& path, ShaderPreset::Pass& pass, std::vector<Stage>& stages, int depth = 0, bool optional = false) -> bool;
    auto fetchShaderSource(ShaderPreset::Pass& pass) -> bool;
    template<bool checkNewLine = false> auto addLineToStage(std::vector<Stage>& stages, const std::string& line, ShaderPreset::Pass& pass) -> void;
    auto addNewLine(std::string& str) -> void;

    auto useScale(ShaderPreset::Pass& pass) -> bool;

    auto getValue(std::string& line) -> std::string;

    static auto buildLutBloom() -> void;
    static auto buildLutMask() -> void;
    static auto buildLutBandwidth() -> void;
    static auto writeLut(const std::string& path, uint8_t* data, unsigned width, unsigned height) -> void;
};

