
#include <initguid.h>
#include <d3d11_4.h>
#include <d3dcompiler.h>
#include <thread>
#include <libloaderapi.h>
#include "../../tools/win.h"
#include "../../tools/tools.h"
#include "../../tools/chronos.h"
#include "../../tools/glslang.h"
#include "../../tools/spirvReflection.h"
#include "../../../deps/SPIRV-Cross/spirv_hlsl.hpp"
#include "../../tools/ShaderCache.h"
#include "../viewport.h"
#include "../thread/renderThread.h"
#include "types.h"
#include "symbols.h"
#include "shaders.h"
#include "utility.h"
#include "dxgiHandler.h"

#ifdef DRV_FREETYPE
#include "../freetype.h"
#endif

//#define D3D_DEBUG

namespace DRIVER {

#ifdef DRV_FREETYPE
    struct D3D11 : Video, RenderThread, DXGIHandler, Freetype {
#else
    struct D3D11 : Video, RenderThread, DXGIHandler {
#endif

    struct Rectangle {
        D3DTexture texture;
        D3DShader shader;
        ID3D11Buffer* vbo;
        ID3D11Buffer* buffer;
    };

    struct {
        D3DTexture textures[MAX_FRAME_HISTORY + 1];
        D3DShader shader;
        ID3D11Buffer* vbo;
        Float4 size;
        Matrix4x4 mvp;
        Matrix4x4 mvpRotated;
    } frame;

    struct alignas(16) HdrUniforms {
        Matrix4x4 mvp;
        float contrast;
        float paperWhiteNits;
        float maxNits;
        float expandGamut;
        float inverseTonemap;
        float hdr10;
    } hdrUniforms;
    
    ScreenshotCallback screenshotCallback = nullptr;

    Rectangle splash;
    Rectangle overlay;
    Rectangle message;
    Rectangle progress;
    Rectangle hdr;

    D3DProgram programs[MAX_SHADERS];
    std::vector<D3DProgram*> programsTemp;
    std::vector<DiskFile*> lutsTemp;
    D3DTexture luts[MAX_TEXTURES];
    unsigned shaderPasses = 0;

    DragndropOverlay dndOverlay;
    SplashScreen splashScreen;
    ViewScreen viewScreen;
    Viewport viewport;

    ID3D11Device* device;
    ID3D11DeviceContext* context;
    SwapChain swapChain;
    D3D_FEATURE_LEVEL featureLevel;

    ID3D11Buffer* ubo;
    ID3D11Buffer* uboRotated;
    ID3D11SamplerState* samplers[3][4];
    ID3D11SamplerState* sampler;

    bool updateRTS;
    bool updateHistory;
    uint8_t options;
    GLSlang glSlang;
    unsigned historySize;
    unsigned deltaTime;
    unsigned lastTime;
    unsigned subFrame;
    unsigned totalFrames;
    AppData appData;

    unsigned frameCount;
    unsigned progressDegree;
    bool progressVisible;
    bool threadAlive;
    int frameDirection;

    ShaderPreset* preset;
    std::atomic<int> shaderId = 0;
    std::atomic<bool> shaderReady = false;
    std::function<void (int pass, bool hasErrors)> onShaderProgressCallback = nullptr;
    std::function<void (DiskFile& diskFile)> onShaderCacheCallback = nullptr;

    ID3D11BlendState* blendEnable;
    ID3D11BlendState* blendDisable;

    ID3D11RasterizerState* scissorEnable;
    ID3D11RasterizerState* scissorDisable;

    ID3D11InfoQueue* debugInfoQueue;
    ID3D11Debug* debug;
    D3D11Symbols symbols;

    float clearColor[4] = {0.0, 0.0, 0.0, 0.0};

    int64_t lastCapTime;
    int64_t minimumCapTime;
    bool legacy;

    Matrix4x4 modelView = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };

    Matrix4x4 projection = {
        2.0f,  0.0f, 0.0f, 0.0f,
        0.0f,  2.0f, 0.0f, 0.0f,
        0.0f,  0.0f,-1.0f, 0.0f,
        -1.0f,-1.0f, 0.0f, 1.0f,
    };

    struct {
        bool synchronize = false;
        bool hardSync = false;
        bool linearFilter = true;

        HWND handle;
        bool vrr = false;
        float vrrSpeed = 0.0;

        bool exclusiveFullscreen = false;
        float exclusiveFullscreenRate = 0.0;
        bool hintExclusiveFullscreen = false;
        Rotation rotation = ROT_0;
        bool useShaderCache = false;
        bool hdrEnable = false;
        unsigned bfiFrames = 0;
        unsigned darkFrames = 0;
        unsigned lightFrames = 0;
    } settings;

    D3D11(bool legacy) {
        this->legacy = legacy;
        debugInfoQueue = nullptr;
        debug = nullptr;
        swapChain.ptr = nullptr;
        swapChain.frameLatency = nullptr;
        device = nullptr;
        context = nullptr;
        ubo = nullptr;
        uboRotated = nullptr;
        frame.vbo = nullptr;
        message.vbo = nullptr;
        message.buffer = nullptr;
        overlay.vbo = nullptr;
        overlay.buffer = nullptr;
        splash.vbo = nullptr;
        splash.buffer = nullptr;
        progress.vbo = nullptr;
        progress.buffer = nullptr;
        hdr.buffer = nullptr;
        updateRTS = false;
        updateHistory = false;
        shaderPasses = 0;
        frameCount = 0;
        historySize = 0;
        deltaTime = 0;
        lastTime = 0;
        subFrame = 1;
        totalFrames = 1;
        frameDirection = 1;

        blendEnable = nullptr;
        blendDisable = nullptr;
        scissorEnable = nullptr;
        scissorDisable = nullptr;
        settings.handle = nullptr;
        settings.hintExclusiveFullscreen = false;
        settings.exclusiveFullscreen = false;
        settings.rotation = ROT_0;
        settings.useShaderCache = false;
        options = 0;
        progressDegree = 0;
        progressVisible = false;
        threadAlive = false;

        for(auto& sampler : samplers)
            for(auto& _sampler : sampler)
                _sampler = nullptr;
    }

    ~D3D11() {
        shaderId++;
        while(threadAlive)
            std::this_thread::yield();
        wait();
        RenderThread::enable(false);
        term();
    }

    auto setShader(ShaderPreset* preset) -> void {
        wait();
        shader( preset );
       // RenderThread::reset();
    }

    auto setShaderProgressCallback( std::function<void (int pass, bool hasErrors)> onCallback ) -> void {
        onShaderProgressCallback = onCallback;
    }

    auto setShaderCacheCallback( std::function<void (DiskFile& diskFile)> onCallback ) -> void {
        onShaderCacheCallback = onCallback;
    }

    auto useShaderCache(bool state) -> void {
        settings.useShaderCache = state;
    }

    auto hintExclusiveFullscreen(bool state, float rate = 0.0) -> void {
        settings.hintExclusiveFullscreen = state;
        settings.exclusiveFullscreenRate = rate;
    }

    auto canExclusiveFullscreen() -> bool { return true; }

    auto hasExclusiveFullscreen() -> bool { return settings.exclusiveFullscreen; }

    auto disableExclusiveFullscreen() -> void {
        if (settings.exclusiveFullscreen) {
            //wait();
            resizeMutexThreaded.lock();
            initSwapChain(symbols, device, settings.handle, settings.hardSync, settings.hdrEnable, swapChain, true);
            if (settings.hdrEnable)
                setHdrChain(swapChain, DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, hdrUniforms.maxNits);

            resizeMutexThreaded.unlock();
            settings.exclusiveFullscreen = false;
        }
    }

    auto activateApp(bool state) -> void {
        if (settings.handle && settings.hintExclusiveFullscreen) {
         //   wait();
            if (state) {
                HWND parent = Win::getParentHandle(settings.handle);
                int adapterId = Win::getFullscreenAdapter(parent);
                if (adapterId >= 0) {
                    settings.exclusiveFullscreen = true;
                    resizeMutexThreaded.lock();
                    initSwapChain(symbols, device, parent, settings.hardSync, settings.hdrEnable, swapChain, false, settings.exclusiveFullscreenRate);
                    if (settings.hdrEnable)
                        setHdrChain(swapChain, DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, hdrUniforms.maxNits);

                    resizeMutexThreaded.unlock();
                    return;
                }
            }

            resizeMutexThreaded.lock();
            initSwapChain(symbols, device, settings.handle, settings.hardSync, settings.hdrEnable, swapChain, true);
            if (settings.hdrEnable)
                setHdrChain(swapChain, DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, hdrUniforms.maxNits);

            resizeMutexThreaded.unlock();
        }
    }

    auto checkFSE() -> void {
        RECT windowSize = Win::getDimension(settings.handle);
        if ((windowSize.right != viewScreen.windowWidth) || (windowSize.bottom != viewScreen.windowHeight)) {
            HWND parent = Win::getParentHandle(settings.handle);
            int adapterId = Win::getFullscreenAdapter(parent);
            if (adapterId >= 0) {
                wait();
                settings.exclusiveFullscreen = true;
                resizeMutexThreaded.lock();
                initSwapChain(symbols, device, parent, settings.hardSync, settings.hdrEnable, swapChain, false, settings.exclusiveFullscreenRate);
                if (settings.hdrEnable)
                    setHdrChain(swapChain, DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, hdrUniforms.maxNits);

                resizeMutexThreaded.unlock();
            }
        }
    }

    auto canHardSync() -> bool { return true; }

    auto setLinearFilter(bool state) -> void {
        if (state == settings.linearFilter)
            return;
        wait();
        settings.linearFilter = state;
        updateFilter();
    }

    auto updateFilter() -> void {
        ShaderPreset::Filter filter = settings.linearFilter ? ShaderPreset::FILTER_LINEAR : ShaderPreset::FILTER_NEAREST;

        for (int i = 0; i < 4; i++)
            samplers[ShaderPreset::FILTER_UNSPEC][i] = samplers[filter][i];

        sampler = samplers[filter][ShaderPreset::WRAP_EDGE];

        for(int i = 0; i < shaderPasses; i++) {
            auto& p = programs[i];
            if (!p.inUse)
                continue;

            for(auto& tex : p.semanticTextures) {
                tex.sampler = (uintptr_t)samplers[tex.filter][tex.wrap];
            }
        }
    }

    auto hardSync(bool state) -> void {
        wait();
        settings.hardSync = state;
        if (settings.handle) {
            initSwapChain(symbols, device, settings.handle, state, settings.hdrEnable, swapChain);
            if (settings.hdrEnable)
                setHdrChain(swapChain, DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, hdrUniforms.maxNits);
        }
    }

    auto synchronize(bool state) -> void {
        wait();
        settings.synchronize = state;
    }

    auto hasSynchronized() -> bool { return settings.synchronize; }

    auto setAspectRatio(int mode, bool integerScaling) -> void { // mode: 0: off, 1: TV, 2: Native
        if ((int)viewScreen.mode == mode && viewScreen.hasIntegerScaling == integerScaling)
            return;

        wait();
        viewScreen.mode = (ViewScreen::Mode)mode;
        viewScreen.hasIntegerScaling = integerScaling;
        if (settings.handle) {
            viewScreen.update(viewport);
            updateFrameSize();
#ifdef DRV_FREETYPE
            ftUpdateCoords();
#endif
            updateHistory = true;
        }
    }

    auto getAspectRatio() -> int {
        return (int)viewScreen.mode;
    }

    auto setThreaded(bool state) -> void {
        if (state != threadEnabled) {
            RenderThread::enable(state);
            RenderThread::reset();
            frame.textures[0].desc.Width = 0;
            frame.textures[0].desc.Height = 0;
        }
    }

    auto hasThreaded() -> bool { return threadEnabled; }

    auto waitRenderThread() -> void { if (threadEnabled) wait(); }

    auto setIntegerScalingDimension( unsigned _w, unsigned _h, bool _ds) -> void {
        viewScreen.scaling.width = _w;
        viewScreen.scaling.height = _h;
        viewScreen.scaling.doubleSize = _ds;
        // we don't need to update now because the input dimension will be changed too
    }

    auto getIntegerScalingDimension(unsigned& _w, unsigned& _h) -> void {
        _w = viewScreen.scaling.width >> 1;
        _h = viewScreen.scaling.height >> 1;
    }

    auto showSplashScreen(unsigned frames, SplashscreenCallback callback) -> void {
        splashScreen.prepare(frames, callback);
    }

    auto hideSplashScreen() -> void {
        splashScreen.hide();
    }

    auto visibleSplashScreen() -> bool {
        return splashScreen.isVisible();
    }

    auto setDragnDropOverlayCallback(DnDOverlayCallback callback) -> void {
        dndOverlay.callback = callback;
    }

    auto setDragnDropOverlaySlots(unsigned slots) -> void {
        dndOverlay.setSlots(slots);
    }

    auto enableDragnDropOverlay(bool state) -> void {
        dndOverlay.setEnable(state);
    }

    auto sendDragnDropOverlayCoordinates(int x, int y) -> int {
        return dndOverlay.sendDragnDropOverlayCoordinates(x, y);
    }

    auto shaderSupport() -> bool { return true; }

    auto setVRR(bool state, float speed = 0.0) -> void {
        wait();
        settings.vrr = state;
        if (speed != 0.0) // otherwise keep speed (use as Shader param too)
            settings.vrrSpeed = speed;
        updateVRR();
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

    auto hasVRR() -> bool { return settings.vrr; }

    auto getViewport() -> Viewport& { return viewport; }

    auto getRotation() -> Rotation { return settings.rotation; }

    auto setRotation(Rotation rotation) -> void {
        if (settings.rotation == rotation)
            return;
        wait();
        settings.rotation = rotation;
        updateRotation();
        updateRTS = true;
        updateHistory = true;
        frameCount = 0;
    }

    auto init(uintptr_t handle) -> bool {
        if (!symbols.initializeSymbols())
            return false;

        settings.handle = (HWND) handle;
        return init();
    }

    auto init() -> bool {
        term();

        D3D_FEATURE_LEVEL features[] = {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0
        };

        unsigned deviceFlags = 0;
#ifdef D3D_DEBUG
        deviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif

        if (FAILED(symbols.D3D11Create(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, deviceFlags, features, 3, D3D11_SDK_VERSION, &device, &featureLevel, &context))) {
            return term(), false;
        }

#ifdef D3D_DEBUG
        if (SUCCEEDED(device->QueryInterface(__uuidof(ID3D11Debug), (void**)&debug))) {
            if (SUCCEEDED(debug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&debugInfoQueue))) {
                debugInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
                debugInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
           //     debugInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, TRUE);
            }
        }
#endif

        if (!initSwapChain(symbols, device, settings.handle, settings.hardSync, settings.hdrEnable, swapChain))
            return false;

        if (settings.hdrEnable)
            setHdrChain(swapChain, DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, hdrUniforms.maxNits);

        if (!initMainTexture(32, 32))
            return term(), false;

        D3D11_SUBRESOURCE_DATA uboData;
        uboData.pSysMem = &projection;
        uboData.SysMemPitch = 0;
        uboData.SysMemSlicePitch = 0;

        D3D11_BUFFER_DESC descP;
        std::memset(&descP, 0, sizeof(descP));
        descP.ByteWidth = sizeof(projection);
        descP.Usage = D3D11_USAGE_DYNAMIC;
        descP.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        descP.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        descP.MiscFlags = 0;
        descP.StructureByteStride = 0;

        if (FAILED(device->CreateBuffer(&descP, &uboData, &ubo)))
            return term(), false;

        if (FAILED(device->CreateBuffer(&descP, nullptr, &uboRotated)))
            return term(), false;

        descP.ByteWidth = 16;
        if (FAILED(device->CreateBuffer(&descP, nullptr, &progress.buffer)))
            return term(), false;

#ifdef DRV_FREETYPE
        descP.ByteWidth = 4 * 4 * 2;
        if (FAILED(device->CreateBuffer(&descP, nullptr, &message.buffer)))
            return term(), false;
#endif

        hdrUniforms.mvp = projection;
        descP.ByteWidth = sizeof(hdrUniforms);
        uboData.pSysMem = &hdrUniforms.mvp;
        if (FAILED(device->CreateBuffer(&descP, &uboData, &hdr.buffer)))
            return term(), false;

        hdrUniforms.hdr10 = 1.0f;
        hdrUniforms.inverseTonemap = 1.0f;
        updateHDRParams();

        updateRotation();

        D3D11_SAMPLER_DESC descS;
        std::memset(&descS, 0, sizeof(descS));
        descS.MaxAnisotropy = 1;
        descS.ComparisonFunc = D3D11_COMPARISON_NEVER;
        descS.MinLOD = -D3D11_FLOAT32_MAX;
        descS.MaxLOD = D3D11_FLOAT32_MAX;
        std::copy(std::begin(clearColor), std::end(clearColor), std::begin(descS.BorderColor) );

        for (int i = 0; i < 4; i++) {
            switch(i) {
                case 0: descS.AddressU = D3D11_TEXTURE_ADDRESS_BORDER; break;
                case 1: descS.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP; break;
                case 2: descS.AddressU = D3D11_TEXTURE_ADDRESS_WRAP; break;
                case 3: descS.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR; break;
            }

            descS.AddressV = descS.AddressU;
            descS.AddressW = descS.AddressU;
            descS.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            if (FAILED(device->CreateSamplerState( &descS, &samplers[ShaderPreset::FILTER_LINEAR][i])))
                return term(), false;

            descS.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
            if (FAILED(device->CreateSamplerState( &descS, &samplers[ShaderPreset::FILTER_NEAREST][i])))
                return term(), false;
        }
        updateFilter();

        D3D11_SUBRESOURCE_DATA vertexData;
        D3D11_BUFFER_DESC descV;
        D3DVertex vertices[] = {
            {{0.0f,  0.0f},  {0.0f, 1.0f}},
            {{0.0f,  1.0f},  {0.0f, 0.0f}},
            {{1.0f,  0.0f},  {1.0f, 1.0f}},
            {{1.0f,  1.0f},  {1.0f, 0.0f}},

            { { -1.0f, -1.0f }, { 0.0f, 1.0f } },
            { { -1.0f,  1.0f }, { 0.0f, 0.0f } },
            { { 1.0f,  -1.0f }, { 1.0f, 1.0f } },
            { { 1.0f,   1.0f }, { 1.0f, 0.0f } },
        };

        vertexData.pSysMem = vertices;
        vertexData.SysMemPitch = 0;
        vertexData.SysMemSlicePitch = 0;

        descV.ByteWidth = sizeof(vertices);
        descV.Usage = D3D11_USAGE_IMMUTABLE;
        descV.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        descV.CPUAccessFlags = 0;
        descV.MiscFlags = 0;
        descV.StructureByteStride = 0;

        if (FAILED(device->CreateBuffer(&descV, &vertexData, &frame.vbo)))
            return term(), false;

        descV.Usage = D3D11_USAGE_DYNAMIC;
        descV.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        descV.ByteWidth = sizeof(D3DVertex) * 4;
        if (FAILED(device->CreateBuffer(&descV, nullptr, &message.vbo)))
            return term(), false;
        if (FAILED(device->CreateBuffer(&descV, nullptr, &overlay.vbo)))
            return term(), false;
        if (FAILED(device->CreateBuffer(&descV, nullptr, &splash.vbo)))
            return term(), false;
        if (FAILED(device->CreateBuffer(&descV, nullptr, &progress.vbo)))
            return term(), false;

        D3D11_INPUT_ELEMENT_DESC descShader[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(D3DVertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(D3DVertex, texcoord), D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        if (!D3D11Utility::createShader(symbols, featureLevel, device, D3D11outputShader, "PS", "VS", "", descShader, countof(descShader), &frame.shader))
            return term(), false;
#ifdef DRV_FREETYPE
        if (D3D11Utility::createShader(symbols, featureLevel, device, D3D11messageShader, "PS", "VS", "", descShader, countof(descShader), &message.shader))
            ftInitialized = true;
#endif
        if (!D3D11Utility::createShader(symbols, featureLevel, device, D3D11overlayShader, "PS", "VS", "", descShader, countof(descShader), &overlay.shader))
            return term(), false;

        if (!D3D11Utility::createShader(symbols, featureLevel, device, D3D11overlayShader, "PS", "VS", "", descShader, countof(descShader), &splash.shader))
            return term(), false;

        if (!D3D11Utility::createShader(symbols, featureLevel, device, D3D11progressShader, "PS", "VS", "", descShader, countof(descShader), &progress.shader))
            return term(), false;

        if (!D3D11Utility::createShader(symbols, featureLevel, device, D3D11hdrShader, "PS", "VS", "", descShader, countof(descShader), &hdr.shader))
            return term(), false;

        dndOverlay.initialized = true;
        splashScreen.initialized = true;
        D3D11_BLEND_DESC blendDesc;
        std::memset(&blendDesc, 0, sizeof(blendDesc));
        blendDesc.AlphaToCoverageEnable                 = false;
        blendDesc.IndependentBlendEnable                = false;
        blendDesc.RenderTarget[0].BlendEnable           = true;
        blendDesc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (FAILED(device->CreateBlendState(&blendDesc, &blendEnable)))
            return term(), false;

        blendDesc.RenderTarget[0].BlendEnable           = false;
        if (FAILED(device->CreateBlendState(&blendDesc, &blendDisable)))
            return term(), false;

        D3D11_RASTERIZER_DESC descR;
        descR.CullMode              = D3D11_CULL_NONE;
        descR.FillMode              = D3D11_FILL_SOLID;
        descR.FrontCounterClockwise = false;
        descR.MultisampleEnable     = false;
        descR.DepthBias             = 0;
        descR.DepthBiasClamp        = 0.0f;
        descR.SlopeScaledDepthBias  = 0.0f;
        descR.AntialiasedLineEnable = false;
        descR.DepthClipEnable       = false;
        descR.ScissorEnable         = true;
        device->CreateRasterizerState(&descR, &scissorEnable);
        descR.ScissorEnable         = false;
        device->CreateRasterizerState(&descR, &scissorDisable);

        RenderThread::reset();
        return true;
    }

    auto shader(ShaderPreset* preset) -> void {
        shaderId++;
        shaderReady = false;
        progressVisible = false;
        shaderPasses = 0;
        historySize = 0;
        std::vector<D3DProgram*> _programs;
        std::vector<DiskFile*> _luts;

        for(auto& program : programs)
            D3D11Utility::releaseProgram(program);

        for (int i = 1; i <= MAX_FRAME_HISTORY; i++)
            D3D11Utility::releaseTexture(frame.textures[i]);

        for(auto& lut : luts)
            D3D11Utility::releaseTexture( lut );

        this->preset = preset;

        if (!preset || (preset->passes.size() > MAX_SHADERS) || (preset->luts.size() > MAX_TEXTURES)) {
            if (settings.hdrEnable)
                updateRTS = true;
            return;
        }

        bool todo = false;
        _programs.reserve(preset->passes.size());
        for (auto& pass : preset->passes) {
            D3DProgram* program = new D3DProgram;
            program->inUse = pass.inUse;
            program->codeVertex = pass.vertex;
            program->codeFragment = pass.fragment;
            pass.error = "";
            _programs.push_back(program);
            todo |= pass.inUse;
        }

        if (!todo) {
            if (settings.hdrEnable)
                updateRTS = true;
            return;
        }

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
        bool useCache = settings.useShaderCache;

        std::thread worker([this, _programs, _luts, _sid, useCache] {
            ShaderCache shaderCache("hlsl");
            shaderCache.onShaderCacheCallback = onShaderCacheCallback;

            static const D3D11_INPUT_ELEMENT_DESC desc[] = {
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(D3DVertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(D3DVertex, texcoord), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };

            auto ts = Chronos::getTimestampInMilliseconds();

            for(int i = 0; i < _programs.size(); i++) {
                auto p = _programs[i];
                if (!p->inUse)
                    continue;

                bool success;
                bool cacheSuccess = false;
                std::string nativeV;
                std::string nativeF;
                spirv_cross::CompilerHLSL* vCompiler = nullptr;
                spirv_cross::CompilerHLSL* fCompiler = nullptr;

                if (useCache)
                    cacheSuccess = shaderCache.read(p->codeVertex, p->codeFragment, p->shader.reflection);

                if (!cacheSuccess) {
                    std::vector<unsigned> spirvVertex;
                    std::vector<unsigned> spirvFragment;

                    success = glSlang.compileVertex(p->codeVertex, spirvVertex, p->shader.error);
                    if (!success) {
                        p->shader.error = "SLANG Vertex Shader #" + std::to_string(i) + " to SPIRV conversion error:\n" + p->shader.error;
                        goto Next;
                    }

                    success = glSlang.compileFragment(p->codeFragment, spirvFragment, p->shader.error);
                    if (!success) {
                        p->shader.error = "SLANG Fragment Shader #" + std::to_string(i) + " to SPIRV conversion error:\n" + p->shader.error;
                        goto Next;
                    }

                    try {
                        vCompiler = new spirv_cross::CompilerHLSL( spirvVertex );
                        fCompiler = new spirv_cross::CompilerHLSL( spirvFragment );

                        spirv_cross::ShaderResources vResources = vCompiler->get_shader_resources();
                        spirv_cross::ShaderResources fResources = fCompiler->get_shader_resources();

                        p->shader.reflection.preProcess( *vCompiler, vResources );
                        p->shader.reflection.preProcess( *fCompiler, fResources );

                        spirv_cross::CompilerHLSL::Options opt;
                        opt.shader_model = featureLevel >= D3D_FEATURE_LEVEL_11_0 ? 50 : 40;
                        vCompiler->set_hlsl_options(opt);
                        fCompiler->set_hlsl_options(opt);
                        nativeV = vCompiler->compile();
                        nativeF = fCompiler->compile();

                        success = p->shader.reflection.process( *vCompiler, *fCompiler, vResources, fResources );
                        if (!success) {
                            p->shader.error = "SPIRV Shader #" + std::to_string(i) + " reflection error:\n" + p->shader.reflection.error;
                            goto Next;
                        }

                    } catch (const std::exception& e) {
                        p->shader.error = "SPIRV Shader #" + std::to_string(i) + " to HLSL conversion error:\n" + e.what();
                        success = false;
                        goto Next;
                    }

                    success = D3D11Utility::createShader(symbols, featureLevel, device, nativeV, "", "main", "", desc, countof(desc), &p->shader, useCache);
                    if (!success) {
                        p->shader.error = "HLSL Vertex Shader #" + std::to_string(i) + " compilation error: " + p->shader.error + "\n";
                        goto Next;
                    }

                    success = D3D11Utility::createShader(symbols, featureLevel, device, nativeF, "main", "", "", nullptr, 0, &p->shader, useCache);
                    if (!success) {
                        p->shader.error = "HLSL Fragment Shader #" + std::to_string(i) + " compilation error: " + p->shader.error + "\n";
                        goto Next;
                    }

                    if (useCache && p->shader.psCode && p->shader.vsCode)
                        shaderCache.write((uint8_t*)p->shader.vsCode->GetBufferPointer(), p->shader.vsCode->GetBufferSize(),
                                          (uint8_t*)p->shader.psCode->GetBufferPointer(), p->shader.psCode->GetBufferSize(),
                                          p->shader.reflection);

                } else {
                    success = D3D11Utility::createShader( symbols, device, shaderCache.nativeV, shaderCache.vertexSize, shaderCache.nativeF, shaderCache.fragmentSize,
                                  desc, countof(desc), &p->shader);

                    if (!success) {
                        p->shader.error = "HLSL Shader #" + std::to_string(i) + " creation error:\n" + p->shader.error;
                        goto Next;
                    }
                }

                Next:
                dxRelease(p->shader.psCode)
                dxRelease(p->shader.vsCode)

                if (vCompiler) delete vCompiler;
                if (fCompiler) delete fCompiler;

                if (shaderId != _sid)
                    break;

                if (!success)
                    D3D11Utility::releaseShader(p->shader);

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
                    D3D11Utility::releaseShader(p->shader);
                    delete p;
                }
                for(auto l : _luts) {
                    if (l->data) delete[] l->data;
                    delete l;
                }
                threadAlive = false;
                return;
            }

            while(shaderReady)
                std::this_thread::yield();

            programsTemp = _programs;
            lutsTemp = _luts;
            shaderReady = true;
            progressVisible = false;
            threadAlive = false;
        });

        threadAlive = true;
        worker.detach();
    }

    auto shaderPostBuild() -> void {
        bool lastPass = true;
        bool mipMapInput = false;

        SemanticMap map = {{
           {(uintptr_t)(&frame.textures[0].view), &frame.textures[0].size, sizeof(D3DTexture), MAX_FRAME_HISTORY},
           {(uintptr_t)(&programs[0].renderTarget.view), &programs[0].renderTarget.size, sizeof(D3DProgram), MAX_SHADERS},
           {(uintptr_t)(&programs[0].feedbackTarget.view), &programs[0].feedbackTarget.size, sizeof(D3DProgram), MAX_SHADERS},
           {(uintptr_t)(&luts[0].view), &luts[0].size, sizeof(D3DTexture), MAX_TEXTURES},
        }, {nullptr, nullptr, &frame.size, nullptr, &frameDirection, &deltaTime, &settings.vrrSpeed, &settings.rotation,
            &viewport.ratio, &viewport.ratioRot, &totalFrames, &subFrame, &historySize,
            &appData.ledDriveState, &appData.cropTop, &appData.cropLeft, &appData.lace, &appData.hires, &appData.pal, &appData.subRegion, &appData.flags} };

        shaderPasses = 0;
        for(int i = programsTemp.size() - 1; i >= 0; i--) {
            auto p = programsTemp[i];
            auto& program = programs[i];
            auto& pass = preset->passes[i];

            program.feedback = false;
            program.shader.layout = p->shader.layout;
            program.shader.vs = p->shader.vs;
            program.shader.ps = p->shader.ps;
            program.shader.gs = p->shader.gs;

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
            program.format = D3D11Utility::getFormat( pass.bufferType );
            program.mipmap = false;

            if (!pass.inUse)
                goto Next;

            if (mipMapInput) {
                program.mipmap = true;
                mipMapInput = false;
            }

            if (pass.mipmap)
                mipMapInput = true;

            if (!p->shader.error.empty()) {
                pass.error = p->shader.error;
                shaderPasses = 0;
                lastPass = false;
                goto Next;
            }

            map.uniforms[SemanticMap::MVP] = lastPass ? &frame.mvp : &modelView;
            map.uniforms[SemanticMap::Output] = &program.renderTarget.size;
            map.uniforms[SemanticMap::FrameCount] = &program.frameCount;

            if (lastPass) {
                lastPass = false;
                shaderPasses = i + 1;
            }

            if (!p->shader.reflection.bindTextures(preset, i, map, program.semanticTextures)) {
                pass.error = "Shader #" + std::to_string(i) + " texture resolve error:\n" + p->shader.reflection.error;
                shaderPasses = 0;
                goto Next;
            }

            if (!p->shader.reflection.bindUbo(preset, i, map, &program.semanticBuffer[SemanticBuffer::Ubo])) {
                pass.error = "Shader #" + std::to_string(i) + " ubo uniform resolve error:\n" + p->shader.reflection.error;
                shaderPasses = 0;
                goto Next;
            }

            if (!p->shader.reflection.bindPush(preset, i, map, &program.semanticBuffer[SemanticBuffer::Push])) {
                pass.error = "Shader #" + std::to_string(i) + " push uniform resolve error:\n" + p->shader.reflection.error;
                shaderPasses = 0;
                goto Next;
            }

            for (int b = 0; b < SemanticBuffer::Max; b++) {
                D3D11_BUFFER_DESC desc;
                desc.ByteWidth           = program.semanticBuffer[b].size;
                desc.Usage               = D3D11_USAGE_DYNAMIC;
                desc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
                desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
                desc.MiscFlags           = 0;
                desc.StructureByteStride = 0;
                if (!desc.ByteWidth)
                    continue;

                device->CreateBuffer(&desc, nullptr, &program.buffers[b]);
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

                D3DTexture& lutTex = luts[l];
                D3D11Utility::releaseTexture(lutTex);

                lutTex.desc.Width = lutFile->width;
                lutTex.desc.Height = lutFile->height;
                lutTex.desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                if (lut.mipmap)
                    lutTex.desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

                if (!D3D11Utility::initTexture(device, lutTex, true))
                    continue;

                if (!D3D11Utility::buildTexture(context, lutTex, lutFile->data))
                    continue;
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
        frameCount = 0;
        updateFrameSize();
    }

    auto getShaderNativeVertexCode(std::string& slang, std::string& out) -> bool {
        return D3D11Utility::translate(featureLevel, slang, out, false);
    }

    auto getShaderNativeFragmentCode(std::string& slang, std::string& out) -> bool {
        return D3D11Utility::translate(featureLevel, slang, out, true);
    }

    auto lock(unsigned*& data, unsigned& pitch, unsigned _width, unsigned _height, uint8_t options = 0) -> bool {
        if (settings.hintExclusiveFullscreen)
            checkFSE();

        if (shaderReady) {
            wait();
            shaderPostBuild();
            shaderReady = false;
        }

        if (!shaderPasses && (options & OPT_RGB10) ) // YUV input needs a shader to progress it
            return false;

        if (threadEnabled)
            return RenderThread::lock(data, pitch, _width, _height, options);

        this->options = options;
        if (swapChain.frameLatency && !settings.vrr)
            WaitForSingleObjectEx( swapChain.frameLatency, 500, true);

        if(_width != frame.textures[0].desc.Width || _height != frame.textures[0].desc.Height) {
            if (!initMainTexture(_width, _height))
                return false;
            viewScreen.update(viewport);
            updateRTS = true;
            updateHistory = true;
        }

        D3D11_MAPPED_SUBRESOURCE mappedTexture;
        if (FAILED(context->Map((ID3D11Resource*)frame.textures[0].staging, 0, D3D11_MAP_WRITE, 0, &mappedTexture)))
            return false;

        data = (unsigned*)mappedTexture.pData;
        pitch = mappedTexture.RowPitch >> 2;

        return true;
    }

    auto adjustSize(unsigned& w, unsigned& h) -> void {
        viewScreen.update(viewport);
        updateRTS = true;
        updateHistory = true;
    }

    auto checkForResize() -> void {
        RECT windowSize = Win::getDimension( settings.handle );
        if ((windowSize.right != viewScreen.windowWidth) || (windowSize.bottom != viewScreen.windowHeight)) {
            resizeMutexThreaded.lock();
            if (settings.hdrEnable)
                swapChain.ptr->ResizeBuffers(0, 0, 0, DXGI_FORMAT_R10G10B10A2_UNORM, swapChain.flags);
            else
                swapChain.ptr->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, swapChain.flags );
            resizeMutexThreaded.unlock();
            viewScreen.update(viewport, windowSize.right, windowSize.bottom);
#ifdef DRV_FREETYPE
            ftUpdateCoords();
#endif
            updateFrameSize();
        }
    }

    auto unlockAndRedraw() -> void {
        if (threadEnabled) {
            RenderThread::unlock();
            return;
        }
        context->Unmap((ID3D11Resource*)frame.textures[0].staging, 0);
        context->CopyResource((ID3D11Resource*)frame.textures[0].ptr, (ID3D11Resource*)frame.textures[0].staging);

        _redraw();
    }

    auto refresh() -> void {
        options = 0;
        RenderBuffer* renderBuffer = getBufferToRender();
        if (renderBuffer && renderBuffer->height) {
            if (swapChain.frameLatency && !settings.vrr)
                WaitForSingleObjectEx( swapChain.frameLatency, 500, true);

            renderBuffer->sharedMutex.lock();

            if(renderBuffer->width != frame.textures[0].desc.Width || renderBuffer->height != frame.textures[0].desc.Height) {
                if (!initMainTexture(renderBuffer->width, renderBuffer->height)) {
                    renderBuffer->sharedMutex.unlock();
                    return;
                }
            }

            D3D11_MAPPED_SUBRESOURCE mappedTexture;
            if (FAILED(context->Map((ID3D11Resource*)frame.textures[0].staging, 0, D3D11_MAP_WRITE, 0, &mappedTexture))) {
                renderBuffer->sharedMutex.unlock();
                return;
            }

            int srcPitch = renderBuffer->width << 2;
            uint8_t* src = renderBuffer->data;

            unsigned pitch = mappedTexture.RowPitch;
            uint8_t* dest = (uint8_t*)mappedTexture.pData;

            for(int i = 0; i < renderBuffer->height; i++) {
                std::memcpy(dest, src, srcPitch);
                src += srcPitch;
                dest += pitch;
            }

            context->Unmap((ID3D11Resource*)frame.textures[0].staging, 0);
            context->CopyResource((ID3D11Resource*)frame.textures[0].ptr, (ID3D11Resource*)frame.textures[0].staging);

            options = renderBuffer->options;
            renderBuffer->sharedMutex.unlock();

            accessMutex.lock();
            frames--;
            accessMutex.unlock();
        }

        _redraw();
    }

    auto redraw() -> void {
        _redraw();
    }

    auto _redraw(bool bfiLock = false) -> void {
        ID3D11RenderTargetView* rtv = nullptr;
        ID3D11Texture2D* backBuffer = nullptr;

        if (!bfiLock)
            checkForResize();

        if (updateRTS) {
            updateRenderTargets(frame.textures[0].desc.Width, frame.textures[0].desc.Height);
            updateRTS = false;
        }

        context->RSSetState(scissorDisable);
        context->OMSetBlendState(blendDisable, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        UINT stride = sizeof(D3DVertex);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &frame.vbo, &stride, &offset);

        D3DTexture* texture = &frame.textures[0];

        if (!(options & OPT_DisallowShader) && shaderPasses) {
            frameCount += 1;
            unsigned curTime = Chronos::getTimestampInMicrosecondsPrecise();
            deltaTime = curTime - lastTime;
            lastTime = curTime;
            frameDirection = (options & OPT_Rewind) ? -1 : 1;

            if (!bfiLock) {
                subFrame = 1;
                if (settings.bfiFrames)
                    totalFrames = (options & OPT_Pause) ? 1 : (settings.bfiFrames + 1);
            }

            for(int i = 0; i < shaderPasses; i++) {
                auto& p = programs[i];
                if (!p.inUse || !p.feedback)
                    continue;

                D3DTexture tmp = p.feedbackTarget;
                p.feedbackTarget = p.renderTarget;
                p.renderTarget = tmp;
            }

            for(int i = 0; i < shaderPasses; i++) {
                auto& p = programs[i];
                if (!p.inUse)
                    continue;
                bool lastPass = i == (shaderPasses - 1);

                applyShader(p.shader);

                if (p.frameModulo)
                    p.frameCount = frameCount % p.frameModulo;
                else
                    p.frameCount = frameCount;

                for (int b = 0; b < SemanticBuffer::Max; b++) {
                    ID3D11Buffer* buffer = p.buffers[b];
                    auto& semBuffer = p.semanticBuffer[b];

                    if (semBuffer.mask && buffer) {
                        D3D11_MAPPED_SUBRESOURCE res;
                        context->Map((ID3D11Resource*)buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);

                        for(auto& var : semBuffer.variables) {
                            if (var.data)
                                memcpy((uint8_t*)res.pData + var.offset, var.data, var.size);
                        }
                        context->Unmap((ID3D11Resource*)buffer, 0);

                        if(semBuffer.mask & SpirvReflection::Vertex)
                            context->VSSetConstantBuffers(semBuffer.binding, 1, &buffer);
                        if(semBuffer.mask & SpirvReflection::Fragment)
                            context->PSSetConstantBuffers(semBuffer.binding, 1, &buffer);
                    }
                }

                ID3D11RenderTargetView* nullRT = nullptr;
                context->OMSetRenderTargets(1, &nullRT, nullptr);

                // it's important to clear unused slots for this pass
                ID3D11ShaderResourceView* textures[SPIRV_MAX_BINDINGS] = { nullptr };
                ID3D11SamplerState* samplers[SPIRV_MAX_BINDINGS] = { nullptr };

                for(auto& semTex : p.semanticTextures) {
                    textures[semTex.binding] = *(ID3D11ShaderResourceView**)semTex.data;
                    samplers[semTex.binding] = (ID3D11SamplerState*)semTex.sampler;
                }

                context->PSSetShaderResources(0, SPIRV_MAX_BINDINGS, textures);
                context->PSSetSamplers(0, SPIRV_MAX_BINDINGS, samplers);

                if (!p.renderTarget.ptr) {
                    texture = nullptr;
                    break;
                }

                context->OMSetRenderTargets( 1, &p.renderTarget.rtView, nullptr);
            //    context->ClearRenderTargetView(p.renderTarget.rtView, clearColor);

                setViewport({p.renderTarget.desc.Width, p.renderTarget.desc.Height, 0, 0});

                context->Draw(4, lastPass ? 0 : 4 );

                if (p.mipmap)
                    context->GenerateMips( p.renderTarget.view);

                texture = &p.renderTarget;
            }

            if (historySize) {
                if (updateHistory) {
                    auto& mainTex = frame.textures[0];
                    for(int i = 1; i <= historySize; i++)
                        initMainTexture( mainTex.desc.Width, mainTex.desc.Height, i );
                    updateHistory = false;
                } else {
                    D3DTexture tmp = frame.textures[historySize];
                    for (int i = historySize; i > 0; i--)
                        frame.textures[i] = frame.textures[i - 1];
                    frame.textures[0] = tmp;
                }
            }
        }

        resizeMutexThreaded.lock();
        if (FAILED(swapChain.ptr->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer))) {
            resizeMutexThreaded.unlock();
            return;
        }

        if (FAILED(device->CreateRenderTargetView((ID3D11Resource*)backBuffer, nullptr, &rtv))) {
            dxRelease(backBuffer)
            resizeMutexThreaded.unlock();
            return;
        }

        dxRelease(backBuffer)

        bool useHDRShader = settings.hdrEnable && (hdr.texture.desc.Format != DXGI_FORMAT_R10G10B10A2_UNORM);

        if (useHDRShader) {
            context->OMSetRenderTargets(1, &hdr.texture.rtView, nullptr);
            context->ClearRenderTargetView(hdr.texture.rtView, clearColor);
        } else {
            context->OMSetRenderTargets(1, &rtv, nullptr);
            context->ClearRenderTargetView(rtv, clearColor);
        }

        setViewport(viewport);

        if (texture) {
            applyShader(frame.shader);
            context->PSSetShaderResources(0, 1, &texture->view);
            if (options & OPT_DisallowFilter)
                context->PSSetSamplers(0, 1, &samplers[ShaderPreset::FILTER_NEAREST][ShaderPreset::WRAP_EDGE]);
            else
                context->PSSetSamplers(0, 1, &sampler);
            context->VSSetConstantBuffers(0, 1, &uboRotated);
        }

        context->Draw(4, 0);

        context->VSSetConstantBuffers(0, 1, &ubo);
#ifdef DRV_FREETYPE
        showText();
#endif

        if (splashScreen.enable) {
            if (buildSplashscreenTexture()) {
                blendRect<false, true>(splash);
            }
        }

        if (dndOverlay.enabled()) {
            buildOverlayTexture();
            blendRect<false, true>(overlay);
        }

        if (progressVisible && progress.texture.ptr) {
            setProgressPosition();
            setProgressRotation();
            blendRect<true, true>(progress);
        }

        if (useHDRShader) {
            if (options & OPT_TakeScreenshot)
                updateHDRParams(true);
            context->OMSetRenderTargets(1, &rtv, nullptr);
            context->ClearRenderTargetView(rtv, clearColor);
            setViewport({viewScreen.windowWidth, viewScreen.windowHeight, 0, 0});
            applyShader(hdr.shader);
            context->VSSetConstantBuffers(0, 1, &ubo);
            context->PSSetShaderResources(0, 1, &hdr.texture.view);
            //context->PSSetSamplers(0, 1, &sampler);
            context->PSSetSamplers(0, 1, &samplers[ShaderPreset::FILTER_NEAREST][ShaderPreset::WRAP_EDGE]);
            context->PSSetConstantBuffers(0, 1, &hdr.buffer);
            context->IASetVertexBuffers(0, 1, &frame.vbo, &stride, &offset);
            context->RSSetState(scissorDisable);
            context->OMSetBlendState(blendDisable, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
            context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            context->Draw(4, 0);
        }

        if (settings.vrr) {
            waitVRR();
        }

        if (options & OPT_TakeScreenshot)
            takeScreenshot();

        if (settings.synchronize || (settings.bfiFrames && !settings.vrr) ) {
            swapChain.ptr->Present(1, 0);
            if (!legacy) {
                IDXGIOutput* pOutput;
                if (SUCCEEDED(swapChain.ptr->GetContainingOutput(&pOutput))) {
                    pOutput->WaitForVBlank();
                    pOutput->Release();
                }
            }
        } else
            swapChain.ptr->Present(0, (swapChain.flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) ? DXGI_PRESENT_ALLOW_TEARING : 0);

        resizeMutexThreaded.unlock();

        if (settings.vrr)
            context->Flush();

        if (settings.bfiFrames && !(options & (OPT_DisallowShader | OPT_Pause) ) && !bfiLock) {
            for (int i = 0; i < settings.lightFrames; i++) {
                subFrame++;
                _redraw(true);
            }

            for (int i = 0; i < settings.darkFrames; i++) {
                context->OMSetRenderTargets(1, &rtv, nullptr);
                context->ClearRenderTargetView(rtv, clearColor);
                if (settings.vrr) {
                    waitVRR();
                    swapChain.ptr->Present(1, 0);
                    context->Flush();
                } else {
                    swapChain.ptr->Present(1, 0);
                }
            }
        }

        dxRelease(rtv)
    }

    auto updateRenderTargets(unsigned width, unsigned height) -> void {
        frame.mvp = projection; // assume: last shader pass is NOT final pass
        bool flp = viewScreen.flipped;

        for(int i = 0; i < shaderPasses; i++) {
            auto& p = programs[i];
            if (!p.inUse)
                continue;

            bool lastPass = i == (shaderPasses - 1);

            if (p.scaleTypeX != ShaderPreset::SCALE_NONE || p.scaleTypeY != ShaderPreset::SCALE_NONE) {
                if (p.scaleTypeX == ShaderPreset::SCALE_INPUT) width *= p.scaleX;
                else if (p.scaleTypeX == ShaderPreset::SCALE_VIEWPORT) {
                    width = (flp ? viewport.height : viewport.width) * p.scaleX;
                } else if (p.scaleTypeX == ShaderPreset::SCALE_ABSOLUTE) width = p.absX;

                if (!width) width = viewport.width;

                if (p.scaleTypeY == ShaderPreset::SCALE_INPUT) height *= p.scaleY;
                else if (p.scaleTypeY == ShaderPreset::SCALE_VIEWPORT) {
                    height = (flp ? viewport.width : viewport.height) * p.scaleY;
                }
                else if (p.scaleTypeY == ShaderPreset::SCALE_ABSOLUTE) height = p.absY;

                if (!height) height = viewport.height;
            } else if (lastPass) {
                width = flp ? viewport.height : viewport.width;
                height = flp ? viewport.width : viewport.height;
            }

            D3D11Utility::releaseTexture(p.renderTarget);

            //if(1) {
            if (!lastPass || p.feedback || (width != viewport.width) || (height != viewport.height) ) {
                p.renderTarget.desc.Width = width;
                p.renderTarget.desc.Height = height;
                p.renderTarget.desc.Format = p.format;
                p.renderTarget.desc.BindFlags = D3D11_BIND_RENDER_TARGET;
                if (p.mipmap)
                    p.renderTarget.desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

                if (!D3D11Utility::initTexture(device, p.renderTarget))
                    continue;

                if (p.feedback) {
                    D3D11Utility::releaseTexture(p.feedbackTarget);
                    p.feedbackTarget.desc = p.renderTarget.desc;
                    if (!D3D11Utility::initTexture(device, p.feedbackTarget))
                        continue;
                }

            } else {
                if (flp && (viewScreen.mode != ViewScreen::Mode::Window)) {
                    unsigned tmp = width;
                    width = height;
                    height = tmp;
                }

                p.renderTarget.size = {(float)width, (float)height, 1.0f / float(width), 1.0f / float(height)};
                frame.mvp = frame.mvpRotated; // last shader pass is final pass
            }

        }

        if (settings.hdrEnable) {
            D3D11Utility::releaseTexture(hdr.texture);
            hdr.texture.desc.Width = viewScreen.windowWidth;
            hdr.texture.desc.Height = viewScreen.windowHeight;
            hdr.texture.desc.BindFlags = D3D11_BIND_RENDER_TARGET;

            if (shaderPasses) {
                auto& p = programs[shaderPasses-1];
                if (!preset->rgb10BitInput && p.format == DXGI_FORMAT_R16G16B16A16_FLOAT)
                    setInternalHDRParams(false, true);
                else
                    setInternalHDRParams(true, true);

                hdr.texture.desc.Format = p.format;
            } else {
                setInternalHDRParams(true, true);
                hdr.texture.desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            }

            D3D11Utility::initTexture(device, hdr.texture);
        }
    }

    auto applyShader(D3DShader& shader) -> void {
        context->IASetInputLayout(shader.layout);
        context->VSSetShader(shader.vs, nullptr, 0);
        context->PSSetShader(shader.ps, nullptr, 0);
        context->GSSetShader(shader.gs, nullptr, 0);
    }

    template<bool hasFragmentBuffer, bool hasLinearFilter>
    auto blendRect(Rectangle& rect) -> void {
        applyShader(rect.shader);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        UINT stride = sizeof(D3DVertex);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &rect.vbo, &stride, &offset);

        context->PSSetShaderResources(0, 1, &rect.texture.view);
        if constexpr (hasLinearFilter)
            context->PSSetSamplers(0, 1, &samplers[ShaderPreset::FILTER_LINEAR][ShaderPreset::WRAP_EDGE]);
        else
            context->PSSetSamplers(0, 1, &samplers[ShaderPreset::FILTER_NEAREST][ShaderPreset::WRAP_EDGE]);

        if constexpr (hasFragmentBuffer)
            context->PSSetConstantBuffers(0, 1, &rect.buffer);
        context->OMSetBlendState(blendEnable, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
        context->Draw(4, 0);
    }

    auto setViewport(const Viewport& viewport) -> void {
        D3D11_VIEWPORT d3D11Viewport;
        d3D11Viewport.Width = viewport.width;
        d3D11Viewport.Height = viewport.height;
        d3D11Viewport.TopLeftX = viewport.x;
        d3D11Viewport.TopLeftY = viewport.y;
        d3D11Viewport.MaxDepth = 1.0;
        d3D11Viewport.MinDepth = 0.0;
        context->RSSetViewports(1, &d3D11Viewport);
    }

    auto term() -> void {
        wait();
        if(context) {
            context->ClearState();
            context->Flush();
        }

        for(auto p : programsTemp) {
            D3D11Utility::releaseShader(p->shader);
            delete p;
        }

        for(auto l : lutsTemp) {
            if (l->data) delete[] l->data;
            delete l;
        }

        for(auto& texture : frame.textures)
            D3D11Utility::releaseTexture(texture);

        for(auto& program : programs)
            D3D11Utility::releaseProgram(program);

        for(auto& lut : luts)
            D3D11Utility::releaseTexture( lut );
        
        D3D11Utility::releaseTexture(message.texture);
        D3D11Utility::releaseTexture(overlay.texture);
        D3D11Utility::releaseTexture(progress.texture);
        D3D11Utility::releaseTexture(hdr.texture);

        dxRelease(frame.vbo)
        dxRelease(message.vbo)
        dxRelease(overlay.vbo)
        dxRelease(progress.vbo)

        dxRelease(ubo)
        dxRelease(uboRotated)
        dxRelease(message.buffer)
        dxRelease(progress.buffer)
        dxRelease(hdr.buffer)
        dxRelease(blendEnable)
        dxRelease(blendDisable)
        dxRelease(scissorEnable)
        dxRelease(scissorDisable)

        for(int i = 0; i < 4; i++) {
            dxRelease(samplers[ShaderPreset::FILTER_LINEAR][i])
            dxRelease(samplers[ShaderPreset::FILTER_NEAREST][i])
        }

        D3D11Utility::releaseShader(frame.shader);
        D3D11Utility::releaseShader(message.shader);
        D3D11Utility::releaseShader(overlay.shader);
        D3D11Utility::releaseShader(progress.shader);
        D3D11Utility::releaseShader(hdr.shader);

        dxRelease(debugInfoQueue)
        dxRelease(debug)

        clearSwapChain(swapChain);
        dxRelease(context)
        dxRelease(device)

        dndOverlay.term();
    }

    auto initMainTexture(unsigned w, unsigned h, unsigned pos = 0) -> bool {
        D3D11Utility::releaseTexture(frame.textures[pos]);
        frame.textures[pos].desc.Width = w;
        frame.textures[pos].desc.Height = h;
        frame.textures[pos].desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        return D3D11Utility::initTexture(device, frame.textures[pos]);
    }

#ifdef DRV_FREETYPE
    auto showScreenText(const std::string text, unsigned duration, bool warn = false) -> void {
        ftUpdateMessage(text, duration, warn);
    }

    auto setScreenTextDescription(ScreenTextDescription& desc) -> void {
        ftSetScreenTextDescription(desc);
    }

    auto freeFont() -> void {
        wait();
        ftUnload();
    }

    auto showText() -> void {
        if (ftUpdated)
            ftProcessUpdates(viewport);

        if (!ftTextBuffer || !message.texture.ptr)
            return;

        if (ftTs) // animations
            if (ftHandleAnimation(viewport))
                return;

        blendRect<true, false>(message);
    }

    auto ftBuildTexture(std::string text, bool keepOldSize = false) -> void {
        if (!ftBuildText(text, keepOldSize))
            return;

        if (!message.texture.ptr || !keepOldSize) {
            D3D11Utility::releaseTexture(message.texture);

            message.texture.desc.Width = ftTotalWidth;
            message.texture.desc.Height = ftTotalHeight;
            message.texture.desc.Format = DXGI_FORMAT_A8_UNORM;
            if(!D3D11Utility::initTexture(device, message.texture, false))
                return;
        }

        D3D11Utility::buildTexture(context, message.texture, ftTextBuffer);
    }

    auto ftSetCoordsPosition() -> void {
        D3D11_MAPPED_SUBRESOURCE mappedVbo;
        if (FAILED(context->Map((ID3D11Resource*)message.vbo, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedVbo)))
            return;

        std::memcpy(mappedVbo.pData, ftPosCoords, 16 * 4);
        context->Unmap((ID3D11Resource*)message.vbo, 0);
    }

    auto ftSetColor(FtColNorm& _colNorm, FtColNorm& _colBgNorm) -> void {
        D3D11_MAPPED_SUBRESOURCE res;
        context->Map((ID3D11Resource*)message.buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);

        memcpy((uint8_t*)res.pData + 0, &_colNorm, 16);
        memcpy((uint8_t*)res.pData + 16, &_colBgNorm, 16);

        context->Unmap((ID3D11Resource*)message.buffer, 0);
        //context->PSSetConstantBuffers(0, 1, &message.buffer);
    }
#endif

    auto buildOverlayTexture() -> void {
        dndOverlay.update(viewport);
        if (!dndOverlay.buffer)
            return;

        dndOverlay.updateAlpha();
        D3D11Utility::releaseTexture(overlay.texture);
        overlay.texture.desc.Width = dndOverlay.texWidth;
        overlay.texture.desc.Height = dndOverlay.texHeight;
        overlay.texture.desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        if(D3D11Utility::initTexture(device, overlay.texture, false))
            if (!D3D11Utility::buildTexture(context, overlay.texture, dndOverlay.buffer))
                return;

        D3D11_MAPPED_SUBRESOURCE mappedVbo;
        if (FAILED(context->Map((ID3D11Resource*)overlay.vbo, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedVbo)))
            return;

        float screenx = 2.0f / (float)viewport.width, screeny = 2.0f / (float)viewport.height;

        float x = -1.0 + dndOverlay.texX * screenx;
        float y = 1.0 - dndOverlay.texY * screeny;

        float w = (float)dndOverlay.texWidth * screenx;
        float h = (float)dndOverlay.texHeight * screeny;

        float box[4][4] = {
            {x,     y,     0, 0},
            {x + w, y,     1, 0},
            {x,     y - h, 0, 1},
            {x + w, y - h, 1, 1}
        };
        D3DVertex* vertex = (D3DVertex*) mappedVbo.pData;

        for (int i = 0; i < 4; i++) {
            vertex->position[0] = box[i][0];
            vertex->position[1] = box[i][1];
            vertex->texcoord[0] = box[i][2];
            vertex->texcoord[1] = box[i][3];
            vertex++;
        }

        context->Unmap((ID3D11Resource*)overlay.vbo, 0);
    }

    auto buildSplashscreenTexture() -> bool {
        auto s = splashScreen.update(viewport);

        if (s == SplashScreen::FINISH)
            return false;

        if (s == SplashScreen::FADE_OUT)
            frameCount = 0;

        if (!splash.texture.ptr || (s == SplashScreen::TEXTURE_UPDATE)) {
            D3D11Utility::releaseTexture(splash.texture);
            splash.texture.desc.Width = splashScreen.viewport.width;
            splash.texture.desc.Height = splashScreen.viewport.height;
            splash.texture.desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

            if(!D3D11Utility::initTexture(device, splash.texture, false)) {
                splashScreen.finish();
                return false;
            }
        }

        if (s == SplashScreen::TEXTURE_UPDATE || s == SplashScreen::DATA_UPDATE) {
            if (!D3D11Utility::buildTexture(context, splash.texture, splashScreen.screenData)) {
                splashScreen.finish();
                return false;
            }
        }

        D3D11_MAPPED_SUBRESOURCE mappedVbo;
        if (FAILED(context->Map((ID3D11Resource*)splash.vbo, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedVbo))) {
            splashScreen.finish();
            return false;
        }
        float screenx = 2.0f / (float)viewport.width, screeny = 2.0f / (float)viewport.height;

        float x = -1.0 + (float)splashScreen.viewport.x * screenx;
        float y = 1.0 - (float)splashScreen.viewport.y * screeny;

        float w = (float)splashScreen.viewport.width * screenx;
        float h = (float)splashScreen.viewport.height * screeny;

        float box[4][4] = {
        {x,     y,     0, 0},
        {x + w, y,     1, 0},
        {x,     y - h, 0, 1},
        {x + w, y - h, 1, 1}
        };
        D3DVertex* vertex = (D3DVertex*) mappedVbo.pData;

        for (int i = 0; i < 4; i++) {
            vertex->position[0] = box[i][0];
            vertex->position[1] = box[i][1];
            vertex->texcoord[0] = box[i][2];
            vertex->texcoord[1] = box[i][3];
            vertex++;
        }

        context->Unmap((ID3D11Resource*)splash.vbo, 0);

        return true;
    }

    auto setProgressAnimation(uint8_t* _data, unsigned _width, unsigned _height) -> void {
        D3D11Utility::releaseTexture(progress.texture);
        progress.texture.desc.Width = _width;
        progress.texture.desc.Height = _height;
        progress.texture.desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        if (D3D11Utility::initTexture(device, progress.texture, false)) {
            if (!D3D11Utility::buildTexture(context, progress.texture, _data))
                return;
        }
    }


    auto setProgressRotation() -> void {
        D3D11_MAPPED_SUBRESOURCE subRes;
        if (SUCCEEDED(context->Map((ID3D11Resource*)progress.buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subRes))) {
            std::memcpy((uint8_t*)subRes.pData, (uint8_t*)&progressDegree, 4);
            context->Unmap((ID3D11Resource*)progress.buffer, 0);
        }
        if (++progressDegree == 360)
            progressDegree = 0;
    }

    auto setProgressPosition() -> void {
        D3D11_MAPPED_SUBRESOURCE mappedVbo;
        if (FAILED(context->Map((ID3D11Resource*)progress.vbo, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedVbo)))
            return;

        float screenx = 2.0f / (float)viewport.width, screeny = 2.0f / (float)viewport.height;

        float x = -1.0 + (viewport.width - progress.texture.size.x - 20) * screenx;
        float y = 1.0 -  20.0 * screeny;

        float w = progress.texture.size.x * screenx;
        float h = progress.texture.size.y * screeny;

        float box[4][4] = {
                {x,     y,     0, 0},
                {x + w, y,     1, 0},
                {x,     y - h, 0, 1},
                {x + w, y - h, 1, 1}
        };
        D3DVertex* vertex = (D3DVertex*) mappedVbo.pData;

        for (int i = 0; i < 4; i++) {
            vertex->position[0] = box[i][0];
            vertex->position[1] = box[i][1];
            vertex->texcoord[0] = box[i][2];
            vertex->texcoord[1] = box[i][3];
            vertex++;
        }

        context->Unmap((ID3D11Resource*)progress.vbo, 0);
    }

    auto setScreenshotCallback(ScreenshotCallback callback) -> void {
        this->screenshotCallback = callback;
    }

    auto takeScreenshot() -> void {
        D3DTexture screenBuffer;
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        ID3D11RenderTargetView* rtv = nullptr;
        options &= ~OPT_TakeScreenshot;

        if (!screenshotCallback)
            return;

        if (FAILED(swapChain.ptr->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&screenBuffer.ptr)))
            return;

        screenBuffer.ptr->GetDesc(&screenBuffer.desc);
        screenBuffer.desc.BindFlags = 0;
        screenBuffer.desc.Usage = D3D11_USAGE_STAGING;
        screenBuffer.desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        if (FAILED(device->CreateTexture2D(&screenBuffer.desc, nullptr, &screenBuffer.staging)))
            return;

        D3D11_BOX crop;
        crop.left = viewport.x;
        crop.right = viewport.width + viewport.x;
        crop.top = viewport.y;
        crop.bottom = viewport.height + viewport.y;
        crop.front = 0;
        crop.back = 1;

        context->CopySubresourceRegion((ID3D11Resource*)screenBuffer.staging, 0, 0, 0, 0,
            (ID3D11Resource*)screenBuffer.ptr, 0, &crop);

        if (SUCCEEDED(context->Map((ID3D11Resource*)screenBuffer.staging, 0, D3D11_MAP_READ, 0, &mappedResource))) {
            uint32_t* pPixels = (uint32_t*)mappedResource.pData;
            uint8_t* pTarget = (uint8_t*)mappedResource.pData;
            unsigned _pitch = mappedResource.RowPitch >> 2;
            uint32_t rgba;

            if (settings.hdrEnable) {
                for (int y = 0; y < viewport.height; ++y) {
                    for (int x = 0; x < viewport.width; ++x) {
                        rgba = pPixels[y * _pitch + x];

                        *pTarget++ = (rgba >> 2) & 0xff;
                        *pTarget++ = (rgba >> 12) & 0xff;
                        *pTarget++ = (rgba >> 22) & 0xff;
                    }
                }
            } else {
                for (int y = 0; y < viewport.height; ++y) {
                    for (int x = 0; x < viewport.width; ++x) {
                        rgba = pPixels[y * _pitch + x];

                        *pTarget++ = rgba & 0xff;
                        *pTarget++ = (rgba >> 8) & 0xff;
                        *pTarget++ = (rgba >> 16) & 0xff;
                    }
                }
            }

            screenshotCallback((uint8_t*)mappedResource.pData, viewport.width, viewport.height);
            context->Unmap((ID3D11Resource*)screenBuffer.staging, 0);
            D3D11Utility::releaseTexture(screenBuffer);
        }

        if (settings.hdrEnable)
            updateHDRParams();
    }

    auto waitVRR() -> void {
        lastCapTime += minimumCapTime;
        int64_t remaining  = lastCapTime - Chronos::getTimestampInMicroseconds();

        if (remaining <= 0) {
            lastCapTime = Chronos::getTimestampInMicroseconds();
            return;
        }

        if (remaining >= 5000) {

            remaining -= 3500;

            unsigned sleepInMilli = (unsigned) ((float) remaining / 1000.0);

            Sleep(sleepInMilli);

            remaining = lastCapTime - Chronos::getTimestampInMicroseconds();
        }

        // we need exact frame pacing
        while(remaining > 0) {
            remaining = lastCapTime - Chronos::getTimestampInMicroseconds();
        }
    }

    auto updateRotation() -> void {
        viewScreen.flipped = settings.rotation == ROT_90 || settings.rotation == ROT_270;
        viewScreen.update(viewport);
        updateFrameSize();

        float radian = (float)settings.rotation * 90.0 * (M_PI / 180.0f);
        D3D11_MAPPED_SUBRESOURCE mapped;
        Matrix4x4 rot = {
                { cosf(radian), sinf(radian), 0.0f, 0.0f ,
                  -sinf(radian), cosf(radian), 0.0f, 0.0f ,
                  0.0f, 0.0f, 0.0f, 0.0f ,
                  0.0f, 0.0f, 0.0f, 1.0f }};

        MatrixMultiply(frame.mvpRotated.data, projection.data, 4, 4, rot.data, 4, 4);
        context->Map( (ID3D11Resource*)uboRotated, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        *(Matrix4x4*)mapped.pData = frame.mvpRotated;
        context->Unmap((ID3D11Resource*)uboRotated, 0);
    }

    auto updateFrameSize() -> void {
        if (!viewport.width || !viewport.height)
            return;

        frame.size.x = viewport.width;
        frame.size.y = viewport.height;
        frame.size.z = 1.0f / (float)viewport.width;
        frame.size.w = 1.0f / (float)viewport.height;
        updateRTS = true; // in the case of passes scaled by viewport
    }

    auto setInternalHDRParams(bool inverseTonemap, bool hdr10) -> void {
        hdrUniforms.hdr10 = hdr10 ? 1.0f : 0.0f;
        hdrUniforms.inverseTonemap = inverseTonemap ? 1.0f : 0.0f;

        if (!settings.hdrEnable)
            return;

        updateHDRParams();
    }

    auto setHDR(bool state, float maxNits, float paperWhiteNits, float contrast, bool expandGamut) -> void {
        hdrUniforms.maxNits = maxNits;
        hdrUniforms.paperWhiteNits = paperWhiteNits;
        hdrUniforms.contrast = contrast;
        hdrUniforms.expandGamut = expandGamut;
        if (!settings.handle) {
            settings.hdrEnable = state;
            return;
        }

        if (settings.hdrEnable != state) {
            wait();
            settings.hdrEnable = state;

            resizeMutexThreaded.lock();
            initSwapChain(symbols, device, settings.handle, settings.hardSync, settings.hdrEnable, swapChain);

            updateRTS = true;
            resizeMutexThreaded.unlock();
        }

        if (settings.hdrEnable) {
            wait();
            updateHDRParams();
            setHdrChain(swapChain, DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, maxNits);
        }
    }

    auto updateHDRParams(bool disableConversion = false) -> void {
        D3D11_MAPPED_SUBRESOURCE mapped;
        HdrUniforms copy;
        HdrUniforms* pHdrUniforms;

        if (!hdr.buffer)
            return;

        if (disableConversion) {
            copy = hdrUniforms;
            copy.inverseTonemap = false;
            copy.hdr10 = false;
            pHdrUniforms = &copy;
        } else
            pHdrUniforms = &hdrUniforms;

        context->Map((ID3D11Resource*)hdr.buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        *(HdrUniforms*)mapped.pData = *pHdrUniforms;
        context->Unmap((ID3D11Resource*)hdr.buffer, 0);
    }

    auto setBFI(unsigned frames, unsigned darkFrames) -> void {
        if (settings.handle)
            wait();
        subFrame = 1;
        totalFrames = frames + 1;
        settings.bfiFrames = frames;
        settings.darkFrames = darkFrames > frames ? frames : darkFrames;
        settings.lightFrames = frames - settings.darkFrames;
        updateVRR();
    }

    auto getAppData() -> AppData* { return &appData; }

    auto HDRsupport() -> bool { return true; }
};

}
