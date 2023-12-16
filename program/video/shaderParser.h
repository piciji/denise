
#pragma once
#include <string>
#include "../../guikit/api.h"
#include "../../driver/driver.h"

struct ShaderParser {
    GUIKIT::Settings rootSettings;
    ShaderPreset shaderPreset;

    auto loadPreset(std::string path) -> bool;

    auto findRootConfig(GUIKIT::Settings& settings, int depth = 0) -> bool;

    auto parsePass(unsigned pos) -> bool;

    auto translateWrapType(std::string wrap) -> ShaderPreset::WrapType;

    auto translateFilter(int filter) -> ShaderPreset::Filter;

    auto parseTextures() -> bool;

    auto fetchParameters() -> void;

    auto createSinglePass(std::string path) -> void;

    auto applyOverrides(std::string& path, std::vector<GUIKIT::Settings*>& settingsList, int depth = 0) -> void;
};

