
namespace DRIVER {

struct GLUtility {
    static std::mutex sharedMutex;

    static auto getFormat(ShaderPreset::BufferType& bufferType) -> GLenum {
        if (bufferType == ShaderPreset::BufferType::R8_UNORM) return GL_R8;
        if (bufferType == ShaderPreset::BufferType::R8_UINT) return GL_R8UI;
        if (bufferType == ShaderPreset::BufferType::R8_SINT) return GL_R8I;
        if (bufferType == ShaderPreset::BufferType::R8G8_UNORM) return GL_RG8;
        if (bufferType == ShaderPreset::BufferType::R8G8_UINT) return GL_RG8UI;
        if (bufferType == ShaderPreset::BufferType::R8G8_SINT) return GL_RG8I;
        if (bufferType == ShaderPreset::BufferType::R8G8B8A8_UNORM) return GL_RGBA8;
        if (bufferType == ShaderPreset::BufferType::R8G8B8A8_UINT) return GL_RGBA8UI;
        if (bufferType == ShaderPreset::BufferType::R8G8B8A8_SINT) return GL_RGBA8I;
        if (bufferType == ShaderPreset::BufferType::R8G8B8A8_SRGB) return GL_SRGB8_ALPHA8;

        if (bufferType == ShaderPreset::BufferType::A2B10G10R10_UNORM_PACK32) return GL_RGB10_A2;
        if (bufferType == ShaderPreset::BufferType::A2B10G10R10_UINT_PACK32) return GL_RGB10_A2UI;

        if (bufferType == ShaderPreset::BufferType::R16_UINT) return GL_R16UI;
        if (bufferType == ShaderPreset::BufferType::R16_SINT) return GL_R16I;
        if (bufferType == ShaderPreset::BufferType::R16_SFLOAT) return GL_R16F;
        if (bufferType == ShaderPreset::BufferType::R16G16_UINT) return GL_RG16UI;
        if (bufferType == ShaderPreset::BufferType::R16G16_SINT) return GL_RG16I;
        if (bufferType == ShaderPreset::BufferType::R16G16_SFLOAT) return GL_RG16F;
        if (bufferType == ShaderPreset::BufferType::R16G16B16A16_UINT) return GL_RGBA16UI;
        if (bufferType == ShaderPreset::BufferType::R16G16B16A16_SINT) return GL_RGBA16I;
        if (bufferType == ShaderPreset::BufferType::R16G16B16A16_SFLOAT) return GL_RGBA16F;

        if (bufferType == ShaderPreset::BufferType::R32_UINT) return GL_R32UI;
        if (bufferType == ShaderPreset::BufferType::R32_SINT) return GL_R32I;
        if (bufferType == ShaderPreset::BufferType::R32_SFLOAT) return GL_R32F;
        if (bufferType == ShaderPreset::BufferType::R32G32_UINT) return GL_RG32UI;
        if (bufferType == ShaderPreset::BufferType::R32G32_SINT) return GL_RG32I;
        if (bufferType == ShaderPreset::BufferType::R32G32_SFLOAT) return GL_RG32F;
        if (bufferType == ShaderPreset::BufferType::R32G32B32A32_UINT) return GL_RGBA32UI;
        if (bufferType == ShaderPreset::BufferType::R32G32B32A32_SINT) return GL_RGBA32I;
        if (bufferType == ShaderPreset::BufferType::R32G32B32A32_SFLOAT) return GL_RGBA32F;
        return GL_RGBA8;
    }

    static auto glParameters(GLuint wrap, GLuint minFilter, GLuint magFilter) -> void {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    }

    static auto glWrap(const ShaderPreset::WrapMode& wrap ) -> GLuint {
        if(wrap == ShaderPreset::WRAP_BORDER) return GL_CLAMP_TO_BORDER;
        if(wrap == ShaderPreset::WRAP_EDGE  ) return GL_CLAMP_TO_EDGE;
        if(wrap == ShaderPreset::WRAP_REPEAT) return GL_REPEAT;
        if(wrap == ShaderPreset::WRAP_MIRRORED_REPEAT) return GL_MIRRORED_REPEAT;
        return GL_CLAMP_TO_EDGE;
    }

    static auto initTexture(GLTexture& tex, bool withRenderTarget) -> bool {
        unsigned levels = tex.mipmap ? getMipLevels(tex.width, tex.height) : 1;

        glGenTextures(1, &tex.view);
        glBindTexture(GL_TEXTURE_2D, tex.view);
        glTexStorage2D(GL_TEXTURE_2D, levels, tex.format, tex.width, tex.height);

        if (withRenderTarget) {
            glGenFramebuffers(1, &tex.frameBuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, tex.frameBuffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.view, 0);
            auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

            if (status != GL_FRAMEBUFFER_COMPLETE) {
                if (status == GL_FRAMEBUFFER_UNSUPPORTED) {
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
                    glDeleteTextures(1, &tex.view);
                    glGenTextures(1, &tex.view);
                    glBindTexture(GL_TEXTURE_2D, tex.view);

                    glTexStorage2D(GL_TEXTURE_2D, levels, GL_RGBA8, tex.width, tex.height);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.view, 0);
                    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                        releaseTexture(tex);
                        return false;
                    }
                } else {
                    releaseTexture(tex);
                    return false;
                }
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

        } else {
            if (tex.format == GL_RGBA32F)
                tex.data = new uint8_t[tex.width * tex.height * 4 * 4]();
            else
                tex.data = new uint8_t[tex.width * tex.height * 4]();
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        tex.size = {(float)tex.width, (float)tex.height, 1.0f / float(tex.width), 1.0f / float(tex.height)};
        return true;
    }

    static auto releaseTexture(GLTexture& tex) -> void {
        if(tex.frameBuffer) {
            glDeleteFramebuffers(1, &tex.frameBuffer);
            tex.frameBuffer = 0;
        }
        if(tex.view) {
            glDeleteTextures(1, &tex.view);
            tex.view = 0;
        }
        if (tex.data) {
            delete[] tex.data;
            tex.data = nullptr;
        }
        tex.width = 0;
        tex.height = 0;
    }

    static auto releaseProgram(GLProgram& prg) -> void {
        deleteProgram(prg.prg);
        releaseTexture(prg.renderTarget);
        releaseTexture(prg.feedbackTarget);
        releaseTexture(prg.cropTarget);

        if (prg.uboData) {
            delete[] prg.uboData;
            prg.uboData = nullptr;
        }

        if (prg.uboBuffer) {
            glDeleteBuffers(1, &prg.uboBuffer);
            prg.uboBuffer = 0;
        }

        prg.semanticTextures.clear();
        prg.semanticBuffer[SemanticBuffer::Ubo].variables.clear();
        prg.semanticBuffer[SemanticBuffer::Push].variables.clear();

        prg.semanticBuffer[SemanticBuffer::Ubo].mask = 0;
        prg.semanticBuffer[SemanticBuffer::Push].mask = 0;
        prg.crop.release();
        prg.cropBox.release();
    }

    static auto getGLSLVersion(GLint& major, GLint& minor) -> unsigned {
        if (major == 3) {
            switch (minor) {
                case 2: return 150;
                case 1: return 140;
                case 0: return 130;
            }
        }
        else if (major == 2) {
            switch (minor) {
                case 1: return 120;
                case 0: return 110;
            }
        }

        // for version 3.3 and above
        return 100 * major + 10 * minor;
    }

    static auto createShader(GLuint type, const char* source, std::string& error) -> GLuint {
        GLint result = GL_FALSE;
        sharedMutex.lock();
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, 0);
        sharedMutex.unlock();

        glCompileShader(shader);

        sharedMutex.lock();
        glGetShaderiv(shader, GL_COMPILE_STATUS, &result);

        if(result == GL_FALSE) {
            GLint length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
            if (length > 0) {
                char* text = new char[length + 1];
                glGetShaderInfoLog(shader, length, &length, text);
                text[length] = 0;
                error = (std::string) text;
                delete[] text;
            }
            glDeleteShader(shader);
            shader = 0;
        }
        sharedMutex.unlock();
        return shader;
    }

    static auto deleteShader(GLuint& shader) -> void {
        if (shader) {
            sharedMutex.lock();
            glDeleteShader(shader);
            shader = 0;
            sharedMutex.unlock();
        }
    }

    static auto createProgram(GLuint vertex, GLuint fragment, std::string& error, bool noValidation = true) -> GLuint {
        sharedMutex.lock();
        GLuint program = glCreateProgram();
        glAttachShader(program, vertex);
        glAttachShader(program, fragment);

        glLinkProgram(program);
        GLint result = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &result);

        if(result == GL_FALSE) {
            getProgramError(program, error);
            glDeleteProgram(program);
            program = 0;
        } else if (!noValidation) {
            glValidateProgram(program);
            result = GL_FALSE;
            glGetProgramiv(program, GL_VALIDATE_STATUS, &result);

            if(result == GL_FALSE) {
                getProgramError(program, error, "validation error: ");
                glDeleteProgram(program);
                program = 0;
            }
        }

        sharedMutex.unlock();
        return program;
    }

    static auto createProgram(GLenum& binaryFormat, const void* binary, GLsizei& length, std::string& error) -> GLuint {
        sharedMutex.lock();
        GLuint program = glCreateProgram();
        if (program != 0) {
            glProgramBinary(program, binaryFormat, binary, length);

            GLint status;
            glGetProgramiv(program, GL_LINK_STATUS, &status);
            if (status == GL_FALSE) {
                getProgramError(program, error);
                glDeleteProgram(program);
                program = 0;
            }
        }
        sharedMutex.unlock();

        return program;
    }

    static auto deleteProgram(GLuint& program) -> void {
        if (program) {
            sharedMutex.lock();
            glDeleteProgram(program);
            program = 0;
            sharedMutex.unlock();
        }
    }

    static auto getBinary(GLuint& program, GLenum& binaryFormat, uint8_t*& binary, GLsizei& length) -> bool {
        sharedMutex.lock();
        glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &length);

        if (length) {
            binary = new uint8_t[length];
            glGetProgramBinary(program, length, nullptr, &binaryFormat, (void*)binary);
        }
        sharedMutex.unlock();
        return binary != nullptr;
    }

    static auto getProgramError(GLuint& program, std::string& error, const std::string& prefix = "") -> void {
        GLint length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        if (length > 0) {
            char* text = new char[length + 1];
            glGetProgramInfoLog(program, length, &length, text);
            text[length] = 0;
            error += prefix + (std::string) text;
            delete[] text;
        }
    }

    static auto translate(Version& version, std::string& code, std::string& out, bool isFragment) -> bool {
        GLSlang glSlang;
        std::string error;
        std::vector<unsigned> spirv;
        spirv_cross::CompilerGLSL* compiler = nullptr;
        SpirvReflection reflection;
        bool success = isFragment ? glSlang.compileFragment(code, spirv, error) : glSlang.compileVertex(code, spirv, error);

        if (!success) {
            out = "SLANG Shader to SPIRV conversion error:\n" + error;
            return false;
        }

        try {
            compiler = new spirv_cross::CompilerGLSL(spirv);
            spirv_cross::ShaderResources resources = compiler->get_shader_resources();
            reflection.preProcessBindNames( "GLSL", isFragment ? "FRAGMENT" : "VERTEX", *compiler, resources);

            spirv_cross::CompilerGLSL::Options opt;
            opt.es = false;
            opt.version = GLUtility::getGLSLVersion(version.major, version.minor);
            opt.fragment.default_float_precision = spirv_cross::CompilerGLSL::Options::Precision::Highp;
            opt.fragment.default_int_precision   = spirv_cross::CompilerGLSL::Options::Precision::Highp;
            opt.enable_420pack_extension         = false;
            compiler->set_common_options(opt);

            out = compiler->compile();
        } catch (const std::exception& e) {
            error = e.what();
            out = "SPIRV Shader to GLSL conversion error:\n" + error;
            if (compiler) delete compiler;
            return false;
        }
        if (compiler) delete compiler;
        return true;
    }
};

std::mutex GLUtility::sharedMutex;

}
