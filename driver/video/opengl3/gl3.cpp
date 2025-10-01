
#ifdef __APPLE__
    #include <OpenGL/gl3.h>
    #include <OpenGL/gl3ext.h>
#elif _WIN32
    #include <GL/gl.h>
    #include <GL/glext.h>
    #define glGetProcAddress(name) wglGetProcAddress(name)
#else
    #include <GL/gl.h>
    #include <GL/glx.h>
    #define glGetProcAddress(name) (*glXGetProcAddress)((const GLubyte*)(name))
#endif

#ifndef DRV_WGL
#include <unistd.h>
#endif


#include <thread>
#include <atomic>
#include "../../tools/tools.h"
#include "../../tools/spirvReflection.h"
#include "../../tools/glslang.h"
#include "../../../deps/SPIRV-Cross/spirv_glsl.hpp"
#include "bind.h"
#include "types.h"
#include "utility.h"
#include "../../tools/ShaderCache.h"
#include "../viewport.h"
#include "../../tools/chronos.h"

namespace DRIVER {
    #include "shaders.h"
}

#ifdef DRV_FREETYPE
    #include "screenText.h"
#endif

#include "progress.h"

namespace DRIVER {
#include "dragndropOverlay.h"

#define UNIFORM_VERTEX_BLOCK_BINDING 1
#define UNIFORM_FRAGMENT_BLOCK_BINDING 0

struct GL3 {
    ShaderPreset* preset;
    std::atomic<bool> shaderReady;
    std::atomic<int> shaderId;
    std::function<void (int pass, bool hasErrors)> onShaderProgressCallback = nullptr;
    std::function<void (DiskFile& diskFile)> onShaderCacheCallback = nullptr;

    GLProgram programs[MAX_SHADERS];
    std::vector<GLProgram*> programsTemp;
    std::vector<DiskFile*> lutsTemp;
    GLTexture luts[MAX_TEXTURES];

    Matrix4x4 projection = {
        2.0f,  0.0f, 0.0f, 0.0f,
        0.0f,  2.0f, 0.0f, 0.0f,
        0.0f,  0.0f, 2.0f, 0.0f,
        -1.0f,-1.0f, 0.0f, 1.0f,
    };


    struct {
        GLTexture textures[MAX_FRAME_HISTORY + 1];
        GLuint prg = 0;
        Float4 size;
        Matrix4x4 mvp;
        Matrix4x4 mvpRotated;
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint filter;
    } frame;

    bool threadAlive;
    bool progressVisible;
    unsigned shaderPasses;
    unsigned historySize;
    unsigned frameCount;
    GLSlang glSlang;

    ViewScreen viewScreen;
    Viewport viewport;
    GLDragndropOverlay dndOverlay;
    OpenGLProgress progress;
#ifdef DRV_FREETYPE
        GlScreenText screenText;
#endif

    bool updateRTS;
    bool updateHistory;
    int frameDirection;

    int64_t lastCapTime;
    int64_t minimumCapTime;
    GLSampler samplers[3][4][2];

    unsigned deltaTime;
    unsigned lastTime;
    unsigned subFrame;
    unsigned totalFrames;

    struct {
        bool synchronize = false;
        bool hardSync = false;
        bool linearFilter = true;
        bool vrr = false;
        float vrrSpeed = 0.0;
        Rotation rotation = ROT_0;
        bool useShaderCache = false;

        unsigned bfiFrames = 0;
        unsigned darkFrames = 0;
        unsigned lightFrames = 0;
    } settings;

    Version version;
    DRIVER::Video::ScreenshotCallback screenshotCallback = nullptr;

    GL3() {
        shaderPasses = 0;
        historySize = 0;
        progressVisible = false;
        threadAlive = false;
        shaderReady = false;
        updateRTS = false;
        updateHistory = false;
        frameCount = 0;
        shaderReady = false;
        shaderId = 0;
        deltaTime = 0;
        lastTime = 0;
        subFrame = 1;
        totalFrames = 1;
        frameDirection = 1;

        settings.synchronize = false;
        settings.hardSync = false;
        settings.linearFilter = true;
        settings.vrr = false;
        settings.rotation = ROT_0;
        settings.useShaderCache = false;
    }

    virtual auto getContext(bool shared = true) -> uintptr_t = 0;
    virtual auto makeContextCurrent(uintptr_t context) -> void = 0;
    virtual auto deleteContext(uintptr_t context) -> void = 0;

    auto stopShaderBuilding() -> void {
        shaderId++;
        while(threadAlive)
            std::this_thread::yield();
    }

    auto lock(unsigned*& data, unsigned& pitch) -> bool {
        auto& tex = frame.textures[0];
        pitch = tex.width;
        return data = (unsigned*)tex.data;
    }

    auto lock(float*& data, unsigned& pitch) -> bool {
        auto& tex = frame.textures[0];
        pitch = tex.width;
        return data = (float*)tex.data;
    }

    auto updateMainTexture(RenderBuffer* renderBuffer) -> void {
        auto& tex = frame.textures[0];
        GLenum pixelFormat;
        GLuint type;
        void* data = renderBuffer ? renderBuffer->data : tex.data;

        if (tex.format == GL_RGBA32F) {
            pixelFormat = GL_RGBA;
            type = GL_FLOAT;
        } else {
            pixelFormat = GL_BGRA;
            type = GL_UNSIGNED_INT_8_8_8_8_REV;
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex.view);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex.width, tex.height, pixelFormat, type, data );
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    auto clear() -> void {
        glUseProgram(0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    auto initTexture(const unsigned& w, const unsigned& h, const GLenum& f, unsigned pos = 0) -> bool {
        auto& tex = frame.textures[pos];
        if(tex.format == f && tex.width == w && tex.height == h)
            return false;

        GLUtility::releaseTexture(tex);
        tex.width = w;
        tex.height = h;
        tex.format = f;
        tex.mipmap = false;
        return GLUtility::initTexture(tex, false);
    }

    auto _redraw(uint8_t options) -> void {
        clear();
        
        if (updateRTS)
            updateRenderTargets(frame.textures[0].width, frame.textures[0].height, options & OPT_Interlace);

        glBindVertexArray(frame.vao);

        GLTexture* texture = &frame.textures[0];

        if (!(options & OPT_DisallowShader) && shaderPasses) {
            frameCount += 1;
            unsigned curTime = Chronos::getTimestampInMicrosecondsPrecise();
            deltaTime = curTime - lastTime;
            lastTime = curTime;
            frameDirection = (options & OPT_Rewind) ? -1 : 1;

            for(int i = 0; i < shaderPasses; i++) {
                auto& p = programs[i];
                if (!p.inUse)
                    continue;

                glUseProgram(p.prg);

                if (p.frameModulo)
                    p.frameCount = frameCount % p.frameModulo;
                else
                    p.frameCount = frameCount;

                SemanticBuffer& semBufferUbo = p.semanticBuffer[SemanticBuffer::Ubo];
                if (semBufferUbo.mask) {
                    for(auto& var : semBufferUbo.variables) {
                        if (var.data)
                            memcpy(p.uboData + var.offset, var.data, var.size);
                    }

                    glBindBuffer(GL_UNIFORM_BUFFER, p.uboBuffer);
                    glBufferSubData(GL_UNIFORM_BUFFER, 0, semBufferUbo.size, (void*)p.uboData);
                    glBindBuffer(GL_UNIFORM_BUFFER, 0);

                    if (p.indexUboVertex != GL_INVALID_INDEX)
                        glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORM_VERTEX_BLOCK_BINDING, p.uboBuffer);
                    if (p.indexUboFragment != GL_INVALID_INDEX)
                        glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORM_FRAGMENT_BLOCK_BINDING, p.uboBuffer);
                }

                SemanticBuffer& semBufferPush = p.semanticBuffer[SemanticBuffer::Push];
                if (semBufferPush.mask) {
                    #define addUniform(loc) if (var.type == spirv_cross::SPIRType::BaseType::Float) { \
                        if (var.size == 4)  \
                            glUniform1f(loc, *(GLfloat*)var.data);   \
                        else if (var.size == 16)    \
                            glUniform4fv(loc, 1, (GLfloat*)var.data);    \
                        else if (var.size == 64)    \
                            glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat*)var.data);    \
                    } else if (var.type == spirv_cross::SPIRType::BaseType::UInt) \
                        glUniform1ui(loc, *(unsigned*)var.data); \
                    else if (var.type == spirv_cross::SPIRType::BaseType::Int) \
                        glUniform1i(loc, *(int*)var.data);

                    for (auto& var : semBufferPush.variables) {
                        if (var.vertexLocation >= 0) {
                            addUniform(var.vertexLocation)
                        }
                        if (var.fragmentLocation >= 0) {
                            addUniform(var.fragmentLocation)
                        }
                    }
                }

                for(auto& semTex : p.semanticTextures) {
                    glActiveTexture(GL_TEXTURE0 + semTex.binding);
                    glBindTexture(GL_TEXTURE_2D, *(GLuint*)semTex.data);
                    GLSampler* sampler = (GLSampler*)semTex.sampler;
                    GLUtility::glParameters( sampler->wrap, sampler->minFilter, sampler->magFilter );
                }

                if (!p.renderTarget.view) {
                    texture = nullptr;
                    break;
                }

                if (p.crop.active) {
                    glBindFramebuffer(GL_FRAMEBUFFER, p.cropTarget.frameBuffer);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p.cropTarget.view, 0);
                } else
                    glBindFramebuffer(GL_FRAMEBUFFER, p.renderTarget.frameBuffer);

                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                glViewport(0, 0, p.renderTarget.width, p.renderTarget.height);

                if (p.renderTarget.format == GL_SRGB8_ALPHA8)
                    glEnable(GL_FRAMEBUFFER_SRGB);

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                glDisable(GL_FRAMEBUFFER_SRGB);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                if (p.renderTarget.mipmap) {
                    glBindTexture(GL_TEXTURE_2D, p.crop.active ? p.cropTarget.view : p.renderTarget.view);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }

                if (p.crop.active) {
                    glBindFramebuffer(GL_READ_FRAMEBUFFER, p.cropTarget.frameBuffer);
                    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p.cropTarget.view, 0);
                    glReadBuffer(GL_COLOR_ATTACHMENT0);

                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, p.renderTarget.frameBuffer);
                    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, p.renderTarget.view, 0);
                    glDrawBuffer(GL_COLOR_ATTACHMENT1);

                    glBlitFramebuffer(
                            p.cropBox.left, p.cropBox.top, p.cropBox.right, p.cropBox.bottom,
                            0, 0, p.cropTarget.width, p.cropTarget.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
                }

                texture = &p.renderTarget;
            }

            if (!updateRTS) {
                for (int i = 0; i < shaderPasses; i++) {
                    auto& p = programs[i];
                    if (!p.inUse || !p.feedback)
                        continue;

                    GLTexture tmp = p.feedbackTarget;
                    p.feedbackTarget = p.renderTarget;
                    p.renderTarget = tmp;
                }
            }

            if (historySize) {
                if (updateHistory) {
                    auto& mainTex = frame.textures[0];
                    for(int i = 1; i <= historySize; i++)
                        initTexture( mainTex.width, mainTex.height, mainTex.format, i );
                    updateHistory = false;
                } else {
                    GLTexture tmp = frame.textures[historySize];
                    for (int i = historySize; i > 0; i--)
                        frame.textures[i] = frame.textures[i - 1];
                    frame.textures[0] = tmp;
                }
            }
        }

        if (texture) {
            glUseProgram(frame.prg);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture->view);
            if (options & OPT_DisallowFilter)
                GLUtility::glParameters(GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
            else
                GLUtility::glParameters(GL_CLAMP_TO_EDGE, frame.filter, frame.filter);
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // to screen

        glViewport(viewport.x, viewport.y, viewport.width, viewport.height);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindVertexArray(0);
        updateRTS = false;
    }

    auto init() -> bool {
        bool initialized = false;
        std::string error;
        GLuint stockVertex = 0;
        GLuint stockFragment = 0;
        GLint position;
        GLint texCoord;

        if(!OpenGLBind())
            return false;

        if (!progress.init())
            progress.term();

        if (!dndOverlay.init())
            dndOverlay.term();

#ifdef DRV_FREETYPE
        if (!screenText.init()) {
            screenText.term();
        }
#endif

        glDisable(GL_ALPHA_TEST);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_POLYGON_SMOOTH);
        glDisable(GL_STENCIL_TEST);

        glEnable(GL_DITHER);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_TEXTURE_1D);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        stockVertex = GLUtility::createShader(GL_VERTEX_SHADER, OpenGLStockVertex.c_str(), error);
        stockFragment = GLUtility::createShader(GL_FRAGMENT_SHADER, OpenGLStockFragment.c_str(), error);

        if (!stockVertex || !stockFragment)
            goto End;

        frame.prg = GLUtility::createProgram(stockVertex, stockFragment, error, true);

        if (!frame.prg)
            goto End;

        glUseProgram(frame.prg);
        glUniform1i(glGetUniformLocation(frame.prg, "source"), 0);

        glGenVertexArrays(1, &frame.vao);
        glBindVertexArray(frame.vao);

        static float coords[] = {
            0.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
        };

        glGenBuffers(1, &frame.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, frame.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(coords), coords, GL_STATIC_DRAW);

        // we use the same VAO for all passes and expect the same locations for all vertex inputs.
        // so we don't need to scan later on each pass for different vertex input locations.
        position = glGetAttribLocation(frame.prg, "Position");
        texCoord = glGetAttribLocation(frame.prg, "TexCoord");

        glEnableVertexAttribArray(position);
        glEnableVertexAttribArray(texCoord);
        glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
        glVertexAttribPointer(texCoord, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (const void*)(GLintptr)(2 * sizeof(float)) );

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        updateRotation();

        for (int i = 0; i < 4; i++) {
            GLSampler* sampler;
            GLuint wrap = GLUtility::glWrap( ShaderPreset::WrapMode(i) );

            sampler = &samplers[ShaderPreset::FILTER_LINEAR][i][0];
            sampler->wrap = wrap;
            sampler->magFilter = GL_LINEAR;
            sampler->minFilter = GL_LINEAR;

            sampler = &samplers[ShaderPreset::FILTER_LINEAR][i][1];
            sampler->wrap = wrap;
            sampler->magFilter = GL_LINEAR;
            sampler->minFilter = GL_LINEAR_MIPMAP_LINEAR;

            sampler = &samplers[ShaderPreset::FILTER_NEAREST][i][0];
            sampler->wrap = wrap;
            sampler->magFilter = GL_NEAREST;
            sampler->minFilter = GL_NEAREST;

            sampler = &samplers[ShaderPreset::FILTER_NEAREST][i][1];
            sampler->wrap = wrap;
            sampler->magFilter = GL_NEAREST;
            sampler->minFilter = GL_NEAREST_MIPMAP_LINEAR;
        }
        updateFilter();

        initialized = true;

End:
        GLUtility::deleteShader(stockVertex);
        GLUtility::deleteShader(stockFragment);

        if (!initialized)
            term();

        return initialized;
    }

    auto term() -> void {
        if(frame.vao) {
            glDeleteVertexArrays(1, &frame.vao);
            frame.vao = 0;
        }
        if(frame.vbo) {
            glDeleteVertexArrays(1, &frame.vbo);
            frame.vbo = 0;
        }
        GLUtility::deleteProgram(frame.prg);

        for(auto& program : programs)
            GLUtility::releaseProgram(program);
       
        for (int i = 0; i <= MAX_FRAME_HISTORY; i++)
            GLUtility::releaseTexture(frame.textures[i]);

        for(auto& lut : luts)
            GLUtility::releaseTexture( lut );
    }

    auto updateFilter() -> void {
        ShaderPreset::Filter filter = settings.linearFilter ? ShaderPreset::FILTER_LINEAR : ShaderPreset::FILTER_NEAREST;

        for (int i = 0; i < 4; i++) {
            samplers[ShaderPreset::FILTER_UNSPEC][i][0] = samplers[filter][i][0];
            samplers[ShaderPreset::FILTER_UNSPEC][i][1] = samplers[filter][i][1];
        }

        frame.filter = settings.linearFilter ? GL_LINEAR : GL_NEAREST;

        for(int i = 0; i < shaderPasses; i++) {
            auto& p = programs[i];
            if (!p.inUse)
                continue;

            for(auto& tex : p.semanticTextures) {
                tex.sampler = (uintptr_t)(GLSampler*)&samplers[tex.filter][tex.wrap][tex.mipmap];
            }
        }
    }

    auto shader(ShaderPreset* preset) -> void {
        shaderId++;
        shaderReady = false;
        progressVisible = false;
        shaderPasses = 0;
        historySize = 0;
        std::vector<GLProgram*> _programs;
        std::vector<DiskFile*> _luts;

        for(auto& program : programs)
            GLUtility::releaseProgram(program);

        for (int i = 1; i <= MAX_FRAME_HISTORY; i++)
            GLUtility::releaseTexture(frame.textures[i]);

        for(auto& lut : luts)
            GLUtility::releaseTexture( lut );

        this->preset = preset;
        if (!preset || (preset->passes.size() > MAX_SHADERS) || (preset->luts.size() > MAX_TEXTURES))
            return;

        bool todo = false;
        _programs.reserve(preset->passes.size());
        for (auto& pass : preset->passes) {
            GLProgram* program = new GLProgram;
            program->inUse = pass.inUse;
            program->codeVertex = pass.vertex;
            program->codeFragment = pass.fragment;
            pass.error = "";
            _programs.push_back(program);
            todo |= pass.inUse;
        }

        if (!todo)
            return;

        _luts.reserve(preset->luts.size());
        for (auto& lut : preset->luts) {
            DiskFile* diskFile = new DiskFile;
            diskFile->path = lut.path;
            diskFile->ident = lut.id;
            diskFile->data = nullptr;
            diskFile->size = 0;
            diskFile->isLUT = true;
            _luts.push_back(diskFile);
        }

        progressVisible = true;
        int _sid = shaderId;
        int cacheMode = !!settings.useShaderCache;
        if (cacheMode) {
            GLint formats = 0;
            glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats);
            if (formats > 0) // check support for shader binary cache
                cacheMode = 2;
        }
        uintptr_t context = getContext();

        std::thread worker([this, _programs, _luts, _sid, cacheMode, context] {
            static std::string prefix = "glsl";

            ShaderCache shaderCache( prefix );
            shaderCache.onShaderCacheCallback = onShaderCacheCallback;
            makeContextCurrent(context);

            auto ts = Chronos::getTimestampInMilliseconds();

            for(int i = 0; i < _programs.size(); i++) {
                auto p = _programs[i];
                if (!p->inUse)
                    continue;

                bool success = true;
                bool cacheSuccess = false;
                std::string nativeV;
                std::string nativeF;
                spirv_cross::CompilerGLSL* vCompiler = nullptr;
                spirv_cross::CompilerGLSL* fCompiler = nullptr;
                GLuint vCompiled = 0;
                GLuint fCompiled = 0;

                if (cacheMode)
                    cacheSuccess = shaderCache.read(p->codeVertex, p->codeFragment, p->reflection);

                if (!cacheSuccess) {
                    std::vector<unsigned> spirvVertex;
                    std::vector<unsigned> spirvFragment;

                    success = glSlang.compileVertex(p->codeVertex, spirvVertex, p->error);
                    if (!success) {
                        p->error = "SLANG Vertex Shader #" + std::to_string(i) + " to SPIRV conversion error:\n" + p->error;
                        goto Next;
                    }

                    success = glSlang.compileFragment(p->codeFragment, spirvFragment, p->error);
                    if (!success) {
                        p->error = "SLANG Fragment Shader #" + std::to_string(i) + " to SPIRV conversion error:\n" + p->error;
                        goto Next;
                    }

                    try {
                        vCompiler = new spirv_cross::CompilerGLSL(spirvVertex);
                        fCompiler = new spirv_cross::CompilerGLSL(spirvFragment);

                        spirv_cross::ShaderResources vResources = vCompiler->get_shader_resources();
                        spirv_cross::ShaderResources fResources = fCompiler->get_shader_resources();

                        success = p->reflection.process( *vCompiler, *fCompiler, vResources, fResources );
                        if (!success) {
                            p->error = "SPIRV Shader #" + std::to_string(i) + " reflection error:\n" + p->reflection.error;
                            goto Next;
                        }

                        p->reflection.preProcessBindNames( prefix, "VERTEX", *vCompiler, vResources);
                        p->reflection.preProcessBindNames( prefix, "FRAGMENT", *fCompiler, fResources);

                        spirv_cross::CompilerGLSL::Options opt;
                        opt.es = false;
                        opt.version = GLUtility::getGLSLVersion(version.major, version.minor);
                        opt.fragment.default_float_precision = spirv_cross::CompilerGLSL::Options::Precision::Highp;
                        opt.fragment.default_int_precision   = spirv_cross::CompilerGLSL::Options::Precision::Highp;
                        opt.enable_420pack_extension         = false;
                        vCompiler->set_common_options(opt);
                        fCompiler->set_common_options(opt);

                        nativeV = vCompiler->compile();
                        nativeF = fCompiler->compile();

                        vCompiled = GLUtility::createShader(GL_VERTEX_SHADER, nativeV.c_str(), p->error);
                        if (!vCompiled) {
                            p->error = "GLSL Vertex Shader #" + std::to_string(i) + " compilation error: " + p->error + "\n";
                            success = false;
                            goto Next;
                        }

                        fCompiled = GLUtility::createShader(GL_FRAGMENT_SHADER, nativeF.c_str(), p->error);
                        if (!fCompiled) {
                            p->error = "GLSL Fragment Shader #" + std::to_string(i) + " compilation error: " + p->error + "\n";
                            success = false;
                            goto Next;
                        }

                        p->prg = GLUtility::createProgram(vCompiled, fCompiled, p->error);
                        if (!p->prg) {
                            p->error = "GLSL Program #" + std::to_string(i) + " linking error: " + p->error + "\n";
                            success = false;
                            goto Next;
                        }

                    } catch (const std::exception& e) {
                        p->error = "SPIRV Shader #" + std::to_string(i) + " to GLSL conversion error:\n" + e.what();
                        success = false;
                        goto Next;
                    }

                    if (cacheMode) {
                        if (cacheMode == 2) {
                            uint8_t* binary = nullptr;
                            GLsizei binaryLength = 0;
                            GLenum binaryFormat = 0;
                            if (GLUtility::getBinary(p->prg, binaryFormat, binary, binaryLength)) {
                                shaderCache.write(binary, binaryLength, (uint8_t*) (uintptr_t) &binaryFormat,
                                                  sizeof(binaryFormat), p->reflection);

                                if (binary)
                                    delete[] binary;
                            }
                        } else {
                            shaderCache.write((uint8_t*)nativeV.data(), nativeV.size(), (uint8_t*)nativeF.data(), nativeF.size(), p->reflection);
                        }
                    }

                } else {
                    if (cacheMode == 2) {
                        GLenum binaryFormat = copyBufferToInt<GLenum>(shaderCache.nativeF);
                        const void* binary = (const void*)shaderCache.nativeV;
                        GLsizei binaryLength = shaderCache.vertexSize;
                        p->prg = GLUtility::createProgram(binaryFormat, binary, binaryLength, p->error);

                    } else {
                        nativeV.assign((char*)shaderCache.nativeV, shaderCache.vertexSize);
                        nativeF.assign((char*)shaderCache.nativeF, shaderCache.fragmentSize);

                        vCompiled = GLUtility::createShader(GL_VERTEX_SHADER, nativeV.c_str(), p->error);

                        if (!vCompiled) {
                            p->error = "GLSL Vertex Shader #" + std::to_string(i) + " from cache (CLEAR CACHE!) compilation error: " + p->error + "\n";
                            success = false;
                            goto Next;
                        }

                        fCompiled = GLUtility::createShader(GL_FRAGMENT_SHADER, nativeF.c_str(), p->error);
                        if (!fCompiled) {
                            p->error = "GLSL Fragment Shader #" + std::to_string(i) + " from cache (CLEAR CACHE!) compilation error: " + p->error + "\n";
                            success = false;
                            goto Next;
                        }

                        p->prg = GLUtility::createProgram(vCompiled, fCompiled, p->error);
                    }

                    if (!p->prg) {
                        p->error = "GLSL Program #" + std::to_string(i) + " create from cache (CLEAR CACHE!) error: " + p->error + "\n";
                        success = false;
                        goto Next;
                    }
                }

                Next:
                GLUtility::deleteShader(vCompiled);
                GLUtility::deleteShader(fCompiled);

                if(vCompiler) delete vCompiler;
                if(fCompiler) delete fCompiler;

                if (shaderId != _sid)
                    break;

                if (!success)
                    GLUtility::deleteProgram(p->prg);

                auto tsNow = Chronos::getTimestampInMilliseconds();
                if (!success || ((tsNow - ts) >= 1000)) {
                    onShaderProgressCallback(i, !success);
                    ts = tsNow;
                }
            }

            for(auto lut : _luts) {
                onShaderCacheCallback(*lut);

                if (shaderId != _sid)
                    break;
            }

            if (shaderId != _sid) { // the user is impatient
                for(auto p : _programs) {
                    GLUtility::deleteProgram(p->prg);
                    delete p;
                }
                for(auto l : _luts) {
                    if (l->data) delete[] l->data;
                    delete l;
                }
                threadAlive = false;
                deleteContext(context);
                return;
            }

            while(shaderReady)
                std::this_thread::yield();

            if (programsTemp.size()) {
                for(auto p : programsTemp)
                    GLUtility::deleteProgram(p->prg);
                programsTemp.clear();
            }

            programsTemp = _programs;
            lutsTemp = _luts;
            shaderReady = true;
            progressVisible = false;
            threadAlive = false;
            deleteContext(context);
        });

        threadAlive = true;
        worker.detach();
    }

    auto shaderPostBuild() -> void {
        static std::string prefix = "glsl";
        bool lastPass = true;
        bool mipMapInput = false;

        SemanticMap map = {{
            {(uintptr_t)(&frame.textures[0].view), &frame.textures[0].size, sizeof(GLTexture), MAX_FRAME_HISTORY},
            {(uintptr_t)(&programs[0].renderTarget.view), &programs[0].renderTarget.size, sizeof(GLProgram), MAX_SHADERS},
            {(uintptr_t)(&programs[0].feedbackTarget.view), &programs[0].feedbackTarget.size, sizeof(GLProgram), MAX_SHADERS},
            {(uintptr_t)(&luts[0].view), &luts[0].size, sizeof(GLTexture), MAX_TEXTURES},
       }, {nullptr, nullptr, &frame.size, nullptr, &frameDirection, &deltaTime, &settings.vrrSpeed, &settings.rotation,
            &viewport.ratio, &viewport.ratioRot, &totalFrames, &subFrame, &historySize} };

        shaderPasses = 0;
        for(int i = programsTemp.size() - 1; i >= 0; i--) {
            auto p = programsTemp[i];
            auto& program = programs[i];
            auto& pass = preset->passes[i];

            program.feedback = false;
            program.prg = p->prg;

            program.inUse = pass.inUse;
            program.ident = pass.alias;
            program.scaleX = pass.scaleX;
            program.absX = pass.absX;
            program.scaleTypeX = pass.scaleTypeX;
            program.scaleY = pass.scaleY;
            program.absY = pass.absY;
            program.scaleTypeY = pass.scaleTypeY;
            program.filter = pass.filter;
            program.wrap = pass.wrap;
            program.frameModulo = pass.frameModulo;
            program.renderTarget.format = GLUtility::getFormat( pass.bufferType );
            program.feedbackTarget.format = program.renderTarget.format;
            program.crop = pass.crop;
            program.renderTarget.mipmap = program.feedbackTarget.mipmap = false;

            if (!pass.inUse)
                goto Next;

            if (mipMapInput) {
                program.renderTarget.mipmap = program.feedbackTarget.mipmap = true;
                mipMapInput = false;
            }

            if (pass.mipmap)
                mipMapInput = true;

            if (!p->error.empty()) {
                pass.error = p->error;
                shaderPasses = 0;
                lastPass = false;
                goto Next;
            }

            map.uniforms[SemanticMap::MVP] = lastPass ? &frame.mvp : &projection;
            map.uniforms[SemanticMap::Output] = &program.renderTarget.size;
            map.uniforms[SemanticMap::FrameCount] = &program.frameCount;

            if (lastPass) {
                lastPass = false;
                shaderPasses = i + 1;
            }

            if (!p->reflection.bindTextures(preset, i, map, program.semanticTextures)) {
                pass.error = "Shader #" + std::to_string(i) + " texture resolve error:\n" + p->reflection.error;
                shaderPasses = 0;
                goto Next;
            }

            if (!p->reflection.bindUbo(preset, i, map, &program.semanticBuffer[SemanticBuffer::Ubo])) {
                pass.error = "Shader #" + std::to_string(i) + " ubo uniform resolve error:\n" + p->reflection.error;
                shaderPasses = 0;
                goto Next;
            }

            if (!p->reflection.bindPush(preset, i, map, &program.semanticBuffer[SemanticBuffer::Push])) {
                pass.error = "Shader #" + std::to_string(i) + " push uniform resolve error:\n" + p->reflection.error;
                shaderPasses = 0;
                goto Next;
            }

            glUseProgram(program.prg);
            program.indexUboVertex = glGetUniformBlockIndex(program.prg, (prefix + "_UBO_VERTEX").c_str());
            program.indexUboFragment = glGetUniformBlockIndex(program.prg, (prefix + "_UBO_FRAGMENT").c_str());
            
            glUniformBlockBinding(program.prg, program.indexUboVertex, UNIFORM_VERTEX_BLOCK_BINDING);
            glUniformBlockBinding(program.prg, program.indexUboFragment, UNIFORM_FRAGMENT_BLOCK_BINDING);

            for(auto& tex : program.semanticTextures) {
                GLint location = glGetUniformLocation(program.prg, (prefix + "_TEXTURE_" + std::to_string(tex.binding)).c_str());
                if (location >= 0)
                    glUniform1i(location, tex.binding);
            }

            glUseProgram(0);

            glGenBuffers(1, &program.uboBuffer);
            glBindBuffer(GL_UNIFORM_BUFFER, program.uboBuffer);
            glBufferData(GL_UNIFORM_BUFFER, program.semanticBuffer[SemanticBuffer::Ubo].size, NULL, GL_STREAM_DRAW);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
            program.uboData = new uint8_t[program.semanticBuffer[SemanticBuffer::Ubo].size]; // because glMapBuffer is slower


            // WTF: for openGL push constants we need to collect locations
            for(auto& var : program.semanticBuffer[SemanticBuffer::Push].variables) {
                GLint vertexLocation = glGetUniformLocation(program.prg, (prefix + "_PUSH_VERTEX_INSTANCE." + var.name).c_str());
                GLint fragmentLocation = glGetUniformLocation(program.prg, (prefix + "_PUSH_FRAGMENT_INSTANCE." + var.name).c_str());

                if (vertexLocation >= 0)
                    var.vertexLocation = vertexLocation;
                if (fragmentLocation >= 0)
                    var.fragmentLocation = fragmentLocation;
            }

            Next:
            delete p;
        }
        programsTemp.clear();

        if (shaderPasses) {
            for(int i = 0; i < shaderPasses; i++) {
                auto& p = programs[i];

                if (p.inUse) {
                    for(auto& tex : p.semanticTextures) {
                        if ( (tex.feedbackPass != -1) && (tex.feedbackPass < shaderPasses))
                            programs[tex.feedbackPass].feedback = true;
                    }
                }
            }

            for (int l = 0; l < preset->luts.size(); l++) {
                auto& lut = preset->luts[l];
                DiskFile* lutFile = nullptr;
                for(auto lutPtr : lutsTemp) {
                    if (lutPtr->ident == lut.id) {
                        lutFile = lutPtr;
                        break;
                    }
                }
                if (!lutFile || !lutFile->data) {
                    preset->luts[l].error = true;
                    shaderPasses = 0;
                    continue;
                }

                GLTexture& lutTex = luts[l];
                GLUtility::releaseTexture(lutTex);

                lutTex.width = lutFile->width;
                lutTex.height = lutFile->height;

                glGenTextures(1, &lutTex.view );
                glBindTexture(GL_TEXTURE_2D, lutTex.view);

                unsigned levels = lut.mipmap ? getMipLevels(lutTex.width, lutTex.height) : 1;
                glTexStorage2D(GL_TEXTURE_2D, levels, GL_RGBA8, lutTex.width, lutTex.height);

                glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
               // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, lutTex.width, lutTex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (uint32_t*)lutFile->data);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, lutTex.width, lutTex.height, GL_RGBA, GL_UNSIGNED_BYTE, lutFile->data );

                if (lut.mipmap)
                    glGenerateMipmap(GL_TEXTURE_2D);

                glBindTexture(GL_TEXTURE_2D, 0);
            }
            updateFilter();
        }

        for(auto lutPtr : lutsTemp) {
            if(lutPtr->data) delete[] lutPtr->data;
            delete lutPtr;
        }
        lutsTemp.clear();

        onShaderProgressCallback(-1, !shaderPasses);
        updateRTS = true;
        updateHistory = true;
        updateFrameSize();
    }

    auto updateRenderTargets(unsigned width, unsigned height, bool interlace) -> void {
        unsigned sourceWidth = width;
        unsigned sourceHeight = height;
        frame.mvp = projection; // assume: last shader pass is NOT final pass

        for(int i = 0; i < shaderPasses; i++) {
            auto& p = programs[i];
            if (!p.inUse)
                continue;

            bool lastPass = i == (shaderPasses - 1);

            if (p.scaleTypeX != ShaderPreset::SCALE_NONE || p.scaleTypeY != ShaderPreset::SCALE_NONE) {
                if (p.scaleTypeX == ShaderPreset::SCALE_INPUT) width *= p.scaleX;
                else if (p.scaleTypeX == ShaderPreset::SCALE_VIEWPORT) width = viewport.width * p.scaleX;
                else if (p.scaleTypeX == ShaderPreset::SCALE_ABSOLUTE) width = p.absX;

                if (!width) width = viewport.width;

                if (p.scaleTypeY == ShaderPreset::SCALE_INPUT) height *= p.scaleY;
                else if (p.scaleTypeY == ShaderPreset::SCALE_VIEWPORT) height = viewport.height * p.scaleY;
                else if (p.scaleTypeY == ShaderPreset::SCALE_ABSOLUTE) height = p.absY;

                if (!height) height = viewport.height;
            } else if (lastPass) {
                width = viewport.width;
                height = viewport.height;
            }

            GLUtility::releaseTexture(p.renderTarget);

            //if(1) {
            if (!lastPass || p.feedback || (width != viewport.width) || (height != viewport.height) ) {
                p.renderTarget.width = width;
                p.renderTarget.height = height;

                if (!GLUtility::initTexture(p.renderTarget, true))
                    continue;

                if (p.feedback) {
                    GLUtility::releaseTexture(p.feedbackTarget);
                    p.feedbackTarget.width = width;
                    p.feedbackTarget.height = height;
                    if (!GLUtility::initTexture(p.feedbackTarget, true))
                        continue;
                }

                if (p.crop.active) {
                    auto& crop = p.crop;
                    unsigned croppedLeft = (width * crop.left) / sourceWidth;
                    unsigned croppedRight = (width * crop.right) / sourceWidth;
                    unsigned croppedTop = (height * (crop.top << (uint8_t)interlace) ) / sourceHeight;
                    unsigned croppedBottom = (height * (crop.bottom << (uint8_t)interlace) ) / sourceHeight;

                    width -= croppedLeft + croppedRight;
                    height -= croppedTop + croppedBottom;

                    p.cropBox.left = croppedLeft;
                    p.cropBox.right = width + croppedLeft;
                    p.cropBox.top = croppedTop;
                    p.cropBox.bottom = height + croppedTop;

                    GLUtility::releaseTexture(p.cropTarget);
                    p.cropTarget.width = width;
                    p.cropTarget.height = height;
                    p.cropTarget.format = p.renderTarget.format;
                    p.cropTarget.mipmap = false;

                    if (!GLUtility::initTexture(p.cropTarget, true))
                        continue;

                    std::swap(p.renderTarget.view, p.cropTarget.view);
                    std::swap(p.renderTarget.frameBuffer, p.cropTarget.frameBuffer);
                }
            } else {
                if (viewScreen.flipped && (viewScreen.mode != ViewScreen::Mode::Window)) {
                    unsigned tmp = width;
                    width = height;
                    height = tmp;
                }

                p.renderTarget.size = {(float)width, (float)height, 1.0f / float(width), 1.0f / float(height)};
                frame.mvp = frame.mvpRotated; // last shader pass is final pass
            }

        }
    }

    auto updateRotation() -> void {
        viewScreen.flipped = settings.rotation == ROT_90 || settings.rotation == ROT_270;
        viewScreen.update(viewport);
        updateFrameSize();

        auto _projection = projection;
        _projection.data[10] = -1.0;

        if (settings.rotation) {
            float radian = M_PI * ((float) settings.rotation * 270.0 / 180.0f);

            Matrix4x4 rot = {
                {cosf(radian), sinf(radian), 0.0f, 0.0f,
                 -sinf(radian), cosf(radian), 0.0f, 0.0f,
                 0.0f, 0.0f, 0.0f, 0.0f,
                 0.0f, 0.0f, 0.0f, 1.0f}};

            MatrixMultiply(frame.mvpRotated.data, _projection.data, 4, 4, rot.data, 4, 4);
        } else
            std::memcpy(frame.mvpRotated.data, _projection.data, 16 * sizeof(GLfloat));

        // y flip
        frame.mvpRotated.data[1] *= -1.0f;
        frame.mvpRotated.data[5] *= -1.0f;
        frame.mvpRotated.data[9] *= -1.0f;
        frame.mvpRotated.data[13] *= -1.0f;

        glUseProgram(frame.prg);
        glUniformMatrix4fv(glGetUniformLocation(frame.prg, "MVP"), 1, GL_FALSE, frame.mvpRotated.data);
        glUseProgram(0);
    }

    auto updateVRR() -> void {
        if (settings.vrr) {
            if (settings.bfiFrames)
                minimumCapTime = (1000000.0 / (settings.vrrSpeed + ((float)settings.bfiFrames * settings.vrrSpeed) ) ) + 0.5;
            else
                minimumCapTime = (1000000.0 / settings.vrrSpeed) + 0.5;
            lastCapTime = Chronos::getTimestampInMicroseconds();
        }
    }

    auto waitVRR() -> void {
        lastCapTime += minimumCapTime;
        int64_t remaining  = lastCapTime - Chronos::getTimestampInMicroseconds();

        if (remaining <= 0) {
            lastCapTime = Chronos::getTimestampInMicroseconds();
            return;
        }

        if (remaining >= 3000) {

            remaining -= 1500;

            unsigned sleepInMilli = (unsigned) ((float) remaining / 1000.0);

#ifdef DRV_WGL
            Sleep(sleepInMilli);
#else
            usleep( sleepInMilli * 1000 );
#endif

            remaining = lastCapTime - Chronos::getTimestampInMicroseconds();
        }

        // we need exact frame pacing
        while(remaining > 0) {
            //std::this_thread::yield();
            remaining = lastCapTime - Chronos::getTimestampInMicroseconds();
        }
    }

    auto updateFrameSize() -> void {
        frame.size.x = viewport.width;
        frame.size.y = viewport.height;
        frame.size.z = 1.0f / (float)viewport.width;
        frame.size.w = 1.0f / (float)viewport.height;
        updateRTS = true;
    }

    auto setScreenshotCallback(DRIVER::Video::ScreenshotCallback callback) -> void {
        this->screenshotCallback = callback;
    }

    auto takeScreenshot() -> void {
        if (!screenshotCallback)
            return;

        uint8_t* buffer = new uint8_t[viewport.width * viewport.height * 4];

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadBuffer(GL_FRONT);

        glReadPixels(viewport.x, viewport.y, viewport.width, viewport.height, GL_RGB, GL_UNSIGNED_BYTE, buffer);

        for (int line = 0; line != viewport.height / 2; ++line) {
            std::swap_ranges(
                buffer + 3 * viewport.width * line,
                buffer + 3 * viewport.width * (line + 1),
                buffer + 3 * viewport.width * (viewport.height - line - 1));
        }

        screenshotCallback(buffer, viewport.width, viewport.height);

        delete[] buffer;
    }
};

}
