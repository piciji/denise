
#pragma once

namespace DRIVER {

    struct Version {
        GLint major = 0;
        GLint minor = 0;
        const GLubyte* glsl;
    };

    struct GLSampler {
        GLuint magFilter;
        GLuint minFilter;
        GLuint wrap;
    };

    struct GLTexture {
        GLuint view = 0;
        GLuint frameBuffer = 0;
        uint8_t* data = nullptr;

        Float4 size;
        unsigned width = 0;
        unsigned height = 0;
        GLenum format;
        bool mipmap = false;
    };

    struct GLProgram {
        bool inUse;
        GLuint prg = 0;
        std::string error = "";
        SpirvReflection reflection;
        std::string ident;

        bool feedback = false;
        unsigned frameCount = 0;
        unsigned frameModulo = 0;
        CropPass crop;
        CropPass cropBox;
        GLuint indexUboVertex;
        GLuint indexUboFragment;
        GLuint uboBuffer = 0;
        uint8_t* uboData = nullptr;

        GLTexture renderTarget;
        GLTexture feedbackTarget;
        GLTexture cropTarget;
        std::string codeFragment;
        std::string codeVertex;
        std::vector<SemanticTexture> semanticTextures;
        SemanticBuffer semanticBuffer[2];

        ShaderPreset::ScaleType scaleTypeX;
        ShaderPreset::ScaleType scaleTypeY;
        float scaleX;
        float scaleY;
        unsigned absX;
        unsigned absY;
        ShaderPreset::Filter filter;
        ShaderPreset::WrapMode wrap;
    };

}