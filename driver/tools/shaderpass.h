
#pragma once

struct ShaderPreset {
    enum WrapMode { WRAP_BORDER = 0, WRAP_EDGE, WRAP_REPEAT, WRAP_MIRRORED_REPEAT };
    enum Filter { FILTER_UNSPEC = 0, FILTER_LINEAR, FILTER_NEAREST };
    enum ScaleType { SCALE_NONE = -1, SCALE_INPUT = 0, SCALE_ABSOLUTE, SCALE_VIEWPORT };
    enum class BufferType  { UNKNOWN = 0, R8_UNORM, R8_UINT, R8_SINT, R8G8_UNORM, R8G8_UINT, R8G8_SINT, R8G8B8A8_UNORM, R8G8B8A8_UINT,
        R8G8B8A8_SINT, R8G8B8A8_SRGB, A2B10G10R10_UNORM_PACK32, A2B10G10R10_UINT_PACK32, R16_UINT, R16_SINT, R16_SFLOAT, R16G16_UINT, R16G16_SINT,
        R16G16_SFLOAT, R16G16B16A16_UINT, R16G16B16A16_SINT, R16G16B16A16_SFLOAT, R32_UINT, R32_SINT, R32_SFLOAT, R32G32_UINT, R32G32_SINT,
        R32G32_SFLOAT, R32G32B32A32_UINT, R32G32B32A32_SINT, R32G32B32A32_SFLOAT
    };

    bool rgb10BitInput = false; // incoming frame data use 10 bit RGB components to prevent accuracy loss when converting to YUV/YIC

    struct Pass {
        std::string src;
        std::string fragment;
        std::string vertex;

        Filter filter;
        WrapMode wrap;
        unsigned frameModulo;
        BufferType bufferType;
        bool mipmap;
        std::string alias;
        bool inUse;
        std::string error;
        bool subChain; // in the case of combined shader chains

        ScaleType scaleTypeX;
        ScaleType scaleTypeY;
        float scaleX;
        float scaleY;
        unsigned absX;
        unsigned absY;
    };
    std::vector<Pass> passes;

    struct Lut {
        Filter filter;
        WrapMode wrap;
        std::string id;
        std::string path;
        bool mipmap;
        bool error;
    };
    std::vector<Lut> luts;

    struct Param {
        float value;
        float minimum;
        float maximum;
        float initial;
        float initialOverridden;
        float step;
        std::string id;
        std::string desc;
    };
    std::vector<Param> params;

    auto clear() {
        params.clear();
        luts.clear();
        passes.clear();
        rgb10BitInput = false;
    }
};
