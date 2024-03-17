
#pragma once

#include "../../deps/SPIRV-Cross/spirv_cross.hpp"
#include "shaderpass.h"
#define SPIRV_MAX_BINDINGS 16

struct SemanticVariable {
    void* data;
    unsigned size;
    spirv_cross::SPIRType::BaseType type;
    unsigned offset;
    std::string name;

    int vertexLocation;
    int fragmentLocation;
};

struct SemanticBuffer {
    enum {Ubo = 0, Push, Max};

    unsigned mask;
    unsigned binding;
    unsigned size;
    std::vector<SemanticVariable> variables;
};

struct SemanticTexture {
    uintptr_t data;
    unsigned binding;
    uintptr_t sampler;
    ShaderPreset::Filter filter;
    ShaderPreset::WrapMode wrap;
    bool mipmap;
    int feedbackPass;
};

struct SemanticMap {
    enum TexTypes { History = 0, PassOutput, PassFeedback, PassLuts, TextureNum };
    enum UniformTypes { MVP, Output, FinalViewport, FrameCount, FrameDirection, Rotation, HistorySize, UniformNum };

    struct ImageMap {
        uintptr_t image;
        void* size;
        size_t stride;
        unsigned maxElements;
    };

    ImageMap textures[TextureNum];
    void* uniforms[UniformNum];
};

struct SpirvVariable {
    std::string name;
    uint32_t size;
    spirv_cross::SPIRType::BaseType type;
    unsigned offset;
};

struct SpirvBuffer {
    uint8_t mask = 0;
    unsigned size = 0;
    unsigned binding;
    std::vector<SpirvVariable> variables;
};

struct SpirvTexture {
    std::string name;
    unsigned binding;
};

struct SpirvReflection {
    enum Stage { Vertex = 1, Fragment = 2 };

    SpirvBuffer ubo;
    SpirvBuffer push;
    std::vector<SpirvTexture> textures;

    std::string error = "";

    auto preProcessBindNames(const std::string& prefix, const std::string& stage, spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& resources) -> void;
    auto preProcess(spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& resources) -> void;
    auto process(spirv_cross::Compiler& vCompiler, spirv_cross::Compiler& fCompiler,spirv_cross::ShaderResources& vResources,spirv_cross::ShaderResources& fResources) -> bool;
    auto clear() -> void;

    auto reflectBuffer(const spirv_cross::SmallVector<spirv_cross::Resource>& buffer, const spirv_cross::Compiler& compiler, SpirvBuffer& result ) -> bool;
    auto combineBuffer(const SpirvBuffer& vBuffer, const SpirvBuffer& fBuffer, SpirvBuffer& out) -> bool;

    auto bindTextures(ShaderPreset* preset, unsigned passId, SemanticMap& map, std::vector<SemanticTexture>& semanticTextures ) -> bool;
    auto bindUbo(ShaderPreset* preset, unsigned passId, SemanticMap& map, SemanticBuffer* semanticBuffer) -> bool;
    auto bindPush(ShaderPreset* preset, unsigned passId, SemanticMap& map, SemanticBuffer* semanticBuffer) -> bool;
    auto bindUniforms(SpirvBuffer& spirvBuffer, ShaderPreset* preset, unsigned passId, SemanticMap& map, SemanticBuffer* semanticBuffer) -> bool;

    auto findString(const std::string& strHaystack, const std::string& strNeedle) -> bool;
    auto isNumber(const std::string& str) -> bool;
    auto getSubChainIndex(ShaderPreset* preset, unsigned start) -> unsigned;
};
