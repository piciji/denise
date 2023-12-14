
#pragma once
#include <string>
#include "../../guikit/api.h"

struct ShaderParser {
    enum WrapType { WRAP_BORDER = 0, WRAP_EDGE, WRAP_REPEAT, WRAP_MIRRORED_REPEAT };
    enum Filter { FILTER_UNSPEC = 0, FILTER_LINEAR, FILTER_NEAREST };
    enum BufferType { BUFFER_UNORM = 0, BUFFER_SRGB, BUFFER_FP };
    enum ScaleType { SCALE_INPUT, SCALE_ABSOLUTE, SCALE_VIEWPORT };

    GUIKIT::Settings rootSettings;
    unsigned passCount;
    unsigned lutCount;
    int feedback;

    struct Pass {
        std::string src;
        Filter filter;
        WrapType wrap;
        unsigned frameModulo;
        BufferType bufferType;
        bool mipmap;
        std::string alias;

        ScaleType scaleTypeX;
        ScaleType scaleTypeY;
        float scaleX;
        float scaleY;
        unsigned absX;
        unsigned absY;
    };

    Pass passes[64];

    struct Lut {
        Filter filter;
        WrapType wrap;
        std::string id;
        std::string path;
        bool mipmap;
    };
    std::vector<Lut> luts;

    struct Param {
        int pass;
        float value;
        float minimum;
        float maximum;
        float initial;
        float step;
        std::string id;
        std::string desc;
    };
    std::vector<Param> params;

    auto loadPreset(std::string path) -> bool;

    auto findRootConfig(GUIKIT::Settings& settings, int depth = 0) -> bool;

    auto parsePass(unsigned pos) -> bool;

    auto translateWrapType(std::string wrap) -> WrapType;

    auto translateFilter(int filter) -> Filter;

    auto parseTextures() -> bool;

    auto fetchParameters() -> void;

    auto applyOverrides(std::string& path, std::vector<GUIKIT::Settings*>& settingsList, int depth = 0) -> void;
};

