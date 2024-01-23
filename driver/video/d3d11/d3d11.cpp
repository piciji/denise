
#include <initguid.h>
#include <d3d11_4.h>
#include <d3dcompiler.h>
#include <thread>
#include <libloaderapi.h>
#include "../../tools/win.h"
#include "../../tools/tools.h"
#include "../../tools/chronos.h"
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

#define D3D_DEBUG

namespace DRIVER {

struct D3D11 : Video, RenderThread, DXGIHandler {

    struct Rectangle {
        D3DTexture texture;
        D3DShader shader;
        ID3D11Buffer* vbo;
    };

    Rectangle frame;
    Rectangle overlay;
    Rectangle message;
    Rectangle progress;

    std::vector<D3DProgram*> programs;
    std::vector<D3DProgram*> programsTemp;
    std::vector<D3DTexture*> luts;

#ifdef DRV_FREETYPE
    Freetype ft;
#endif
    DragndropOverlay dndOverlay;
    ViewScreen viewScreen;
    Viewport viewport;

    ID3D11Device* device;
    ID3D11DeviceContext* context;
    SwapChain swapChain;
    D3D_FEATURE_LEVEL featureLevel;

    ID3D11Buffer* ubo;
    ID3D11Buffer* uboRotated;
    ID3D11Buffer* uboChain;
    ID3D11SamplerState* samplers[3][4];
    ID3D11SamplerState* sampler;
    DXGI_FORMAT format;
    float seed;
    bool updateRTS;
    uint8_t options;

    unsigned progressDegree;
    bool progressVisible;
    ID3D11Buffer* constantBufferProgress;

    ShaderPreset* preset;
    std::atomic<int> shaderId = 0;
    std::atomic<bool> shaderReady = false;
    std::function<void (int pass, bool hasErrors)> onShaderCallback = nullptr;

    ID3D11BlendState* blendEnable;
    ID3D11BlendState* blendDisable;

    ID3D11RasterizerState* scissorEnable;
    ID3D11RasterizerState* scissorDisable;

    ID3D11InfoQueue* debugInfoQueue;
    ID3D11Debug* debug;
    D3D11Symbols symbols;

    float clearColor[4] = {0.0, 0.0, 0.0, 1.0};

    int64_t lastCapTime;
    int64_t minimumCapTime;
    bool legacy;

    const Matrix4x4 modelView = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };

    const Matrix4x4 projection = {
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
        bool threaded = false;

        int aspectMode = 1;
        bool integerScaling = false;

        bool vrr = false;

        std::string message = "";
        bool msgCritical = false;
        std::atomic<bool> msgUpdated = false;

        bool exclusiveFullscreen = false;
        float exclusiveFullscreenRate = 0.0;
        bool hintExclusiveFullscreen = false;
        unsigned rotation = ~0;
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
        constantBufferProgress = nullptr;
        uboChain = nullptr;
        frame.vbo = nullptr;
        message.vbo = nullptr;
        overlay.vbo = nullptr;
        progress.vbo = nullptr;
        updateRTS = false;

        blendEnable = nullptr;
        blendDisable = nullptr;
        scissorEnable = nullptr;
        scissorDisable = nullptr;
        settings.handle = nullptr;
        settings.msgUpdated = false;
        settings.hintExclusiveFullscreen = false;
        settings.exclusiveFullscreen = false;
        settings.rotation = ~0;
        options = 0;
        progressDegree = 0;
        progressVisible = false;

        for(auto& sampler : samplers)
            for(auto& _sampler : sampler)
                _sampler = nullptr;
    }

    ~D3D11() {
        RenderThread::enable(false);
        term();
    }

    auto setShader(ShaderPreset* preset) -> void {
        wait();
        shader( preset );
       // RenderThread::reset();
    }

    auto setShaderProgressCallback( std::function<void (int pass, bool hasErrors)> onCallback ) -> void {
        onShaderCallback = onCallback;
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
            initSwapChain(symbols, device, settings.handle, settings.hardSync, swapChain, true);
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
                    initSwapChain(symbols, device, parent, settings.hardSync, swapChain, false, settings.exclusiveFullscreenRate);
                    resizeMutexThreaded.unlock();
                    return;
                }
            }

            resizeMutexThreaded.lock();
            initSwapChain(symbols, device, settings.handle, settings.hardSync, swapChain, true);
            resizeMutexThreaded.unlock();
        }
    }

    auto checkFSE() -> void {
        RECT windowSize = Win::getDimension(settings.handle);
        if ((windowSize.right != viewScreen.windowWidth) || (windowSize.bottom != viewScreen.windowHeight)) {
            HWND parent = Win::getParentHandle(settings.handle);
            int adapterId = Win::getFullscreenAdapter(parent);
            if (adapterId >= 0) {
             //   wait();
                settings.exclusiveFullscreen = true;
                resizeMutexThreaded.lock();
                initSwapChain(symbols, device, parent, settings.hardSync, swapChain, false, settings.exclusiveFullscreenRate);
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

        for(auto p : programs)
            for(auto& b : p->bindTextures)
                b.sampler = samplers[b.filter][b.wrap];
    }

    auto hardSync(bool state) -> void {
        wait();
        settings.hardSync = state;
        if (settings.handle) {
            if (!initSwapChain(symbols, device, settings.handle, state, swapChain)) {

            }
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
          //  setViewport(viewport);
        }
    }

    auto getAspectRatio() -> int {
        return (int)viewScreen.mode;
    }

    auto setThreaded(bool state) -> void {
        if (state != settings.threaded) {
            wait();
            RenderThread::enable(state);
            RenderThread::reset();
            frame.texture.desc.Width = 0;
            frame.texture.desc.Height = 0;
            settings.threaded = state;
        }
    }

    auto hasThreaded() -> bool { return settings.threaded; }

    auto waitRenderThread() -> void { if (settings.threaded) wait(); }

    auto setIntegerScalingDimension( unsigned _w, unsigned _h, bool _ds) -> void {
        viewScreen.scaling.width = _w;
        viewScreen.scaling.height = _h;
        viewScreen.scaling.doubleSize = _ds;
    }

    auto getIntegerScalingDimension(unsigned& _w, unsigned& _h) -> void {
        _w = viewScreen.scaling.width >> 1;
        _h = viewScreen.scaling.height >> 1;
    }

    auto setDragnDropOverlay(uint8_t* _data, unsigned _width, unsigned _height, unsigned line = 0) -> void {
        dndOverlay.setDragnDropOverlay(_data, _width, _height, line);
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

        if (state) {
            minimumCapTime = (1000000.0 / speed) + 0.5;
            lastCapTime = Chronos::getTimestampInMicroseconds();
        }
    }

    auto hasVRR() -> bool { return settings.vrr; }

    auto getViewport() -> Viewport& { return viewport; }

    auto getRotation() -> unsigned { return settings.rotation; }

    auto setRotation(unsigned degree) -> void {
        Matrix4x4 projectionRotated;
        if (settings.rotation == degree)
            return;
        wait();
        viewScreen.flipped = degree == 90 || degree == 270;
        viewScreen.update(viewport);
        settings.rotation = degree;
        float radian = degree * (M_PI / 180.0f);
        D3D11_MAPPED_SUBRESOURCE mapped;
        Matrix4x4 rot = {
            { cosf(radian), sinf(radian), 0.0f, 0.0f ,
              -sinf(radian), cosf(radian), 0.0f, 0.0f ,
              0.0f, 0.0f, 0.0f, 0.0f ,
              0.0f, 0.0f, 0.0f, 1.0f }};

        MatrixMultiply(projectionRotated.data, projection.data, 4, 4, rot.data, 4, 4);
        context->Map( (ID3D11Resource*)uboRotated, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        *(Matrix4x4*)mapped.pData = projectionRotated;
        context->Unmap((ID3D11Resource*)uboRotated, 0);
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

        if (!initSwapChain(symbols, device, settings.handle, settings.hardSync, swapChain))
            return false;

        format = DXGI_FORMAT_B8G8R8A8_UNORM;
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

        if (FAILED(device->CreateBuffer(&descP, nullptr, &constantBufferProgress)))
            return term(), false;

        setRotation(0);

        uboData.pSysMem = &modelView;
        descP.ByteWidth = sizeof(modelView);

        if (FAILED(device->CreateBuffer(&descP, &uboData, &uboChain)))
            return term(), false;

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
            {{0.0f,  0.0f},  {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
            {{0.0f,  1.0f},  {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
            {{1.0f,  0.0f},  {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
            {{1.0f,  1.0f},  {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},

            { { -1.0f, -1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
            { { -1.0f,  1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
            { { 1.0f,  -1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
            { { 1.0f,   1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
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
        if (FAILED(device->CreateBuffer(&descV, nullptr, &progress.vbo)))
            return term(), false;

        D3D11_INPUT_ELEMENT_DESC descShader[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(D3DVertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(D3DVertex, texcoord), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(D3DVertex, color), D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        if (!D3D11Utility::createShader(symbols, featureLevel, device, D3D11outputShader, "PS", "VS", "", descShader, countof(descShader), &frame.shader))
            return term(), false;

        if (!D3D11Utility::createShader(symbols, featureLevel, device, D3D11messageShader, "PS", "VS", "", descShader, countof(descShader), &message.shader))
            return term(), false;

        if (!D3D11Utility::createShader(symbols, featureLevel, device, D3D11overlayShader, "PS", "VS", "", descShader, countof(descShader), &overlay.shader))
            return term(), false;

        if (!D3D11Utility::createShader(symbols, featureLevel, device, D3D11progressShader, "PS", "VS", "", descShader, countof(descShader), &progress.shader))
            return term(), false;

        dndOverlay.initialized = true;
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

        viewScreen.update(viewport);
        setViewport(viewport);
#ifdef DRV_FREETYPE
        if (ft.init())
            ft.setFontSize(12);
#endif
        RenderThread::reset();
        return true;
    }

    auto shader(ShaderPreset* preset) -> void {
        shaderReady = false;
        progressVisible = false;
        std::vector<D3DProgram*> _programs;

        for(auto program : programs) {
            D3D11Utility::releaseShader(program->shader);
            D3D11Utility::releaseTexture(program->renderTarget);
            D3D11Utility::releaseTexture(program->cropTarget);
            delete program;
        }
        for(auto lut : luts) {
            D3D11Utility::releaseTexture( *lut );
            delete lut;
        }
        programs.clear();
        luts.clear();

        shaderId++;

        this->preset = preset;
        if (!preset)
            return;

        for (unsigned passId = 0; passId < preset->passes.size(); passId++) {
            auto& pass = preset->passes[passId];
            if (!pass.inUse)
                continue;

            D3DProgram* program = new D3DProgram;
            program->passId = passId;
            program->mipmap = pass.mipmap;
            program->scaleX = pass.scaleX;
            program->absX = pass.absX;
            program->scaleTypeX = pass.scaleTypeX;
            program->scaleY = pass.scaleY;
            program->absY = pass.absY;
            program->scaleTypeY = pass.scaleTypeY;
            program->filter = pass.filter;
            program->wrap = pass.wrap;
            program->ident = pass.alias;
            program->crop.set( pass.crop );
            program->src = pass.code;
            program->format = D3D11Utility::getFormat( pass.bufferType );
            _programs.push_back(program);
        }

        if (!_programs.size())
            return;

        progressVisible = true;
        int _sid = shaderId;
        std::thread worker([this, _programs, _sid] {
            auto ts = Chronos::getTimestampInMilliseconds();

            for(auto p : _programs) {
                D3D11Utility::releaseTexture(p->renderTarget);
                D3D11Utility::releaseTexture(p->cropTarget);

         //       Sleep( 500 );
                bool success = D3D11Utility::buildProgram2(symbols, featureLevel, device, *p);
                if (!success)
                    D3D11Utility::releaseShader(p->shader);

                if (shaderId != _sid) {
                    for(auto p : _programs) {
                        D3D11Utility::releaseShader(p->shader);
                        delete p;
                    }
                    return;
                }

                auto ts2 = Chronos::getTimestampInMilliseconds();
                if (!success || ((ts2 - ts) >= 1000)) {
                    onShaderCallback(p->passId, !success);
                    ts = ts2;
                }
            }

            while(shaderReady) {
                std::this_thread::yield();
            }
            programsTemp = _programs;
            shaderReady = true;
            progressVisible = false;
        });
        worker.detach();
    }

    auto shaderPostBuild() -> void {
        for (auto& lut: preset->luts) {
            if (!lut.data)
                continue;

            D3DTexture* lutTex = new D3DTexture;
            D3D11Utility::releaseTexture(*lutTex);
            lutTex->ident = lut.id;
            lutTex->desc.Width = lut.width;
            lutTex->desc.Height = lut.height;
            lutTex->desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            if (lut.mipmap)
                lutTex->desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

            if (!D3D11Utility::initTexture(device, *lutTex, true)) {
                delete lutTex;
                continue;
            }

            if (!D3D11Utility::buildTexture(context, *lutTex, lut.data)) {
                delete lutTex;
                continue;
            }

            luts.push_back(lutTex);
        }

        programs.clear();
        bool hasError = false;
        for(auto p : programsTemp) {
            std::string& error = p->shader.error;
            if (!error.empty()) {
                if (p->passId < preset->passes.size()) {
                    preset->passes[p->passId].error = error;
                    hasError = true;
                }
            }

            if (p->shader.layout)
                programs.push_back(p);
        }

        D3DTexture* tex = &frame.texture;
        for(auto p : programs) {
            for(auto& b : p->bindTextures) {
                if (b.ident == "Source") {
                    b.texture = tex;
                    b.filter = p->filter;
                    b.wrap = p->wrap;
                    b.sampler = samplers[b.filter][b.wrap];
                    tex = p->crop.active ? &p->cropTarget : &p->renderTarget;
                } else {
                    for(auto lutTex : luts) {
                        if (lutTex->ident == b.ident) {
                            b.texture = lutTex;
                            for (auto& lut: preset->luts) {
                                if (lut.id == lutTex->ident) {
                                    b.filter = lut.filter;
                                    b.wrap = lut.wrap;
                                    b.sampler = samplers[lut.filter][lut.wrap];
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }

        for(auto p : programs) {
            for(auto& b : p->bindTextures) {
                if (b.texture)
                    continue;

                for(auto p2 : programs) {
                    if (p2->ident == b.ident) {
                        for(auto& b2 : p2->bindTextures) {
                            if (b2.ident == "Source") {
                                b.texture = b2.texture;
                                b.filter = b2.filter;
                                b.wrap = b2.wrap;
                                b.sampler = b2.sampler;
                                break;
                            }
                        }
                        break;
                    }
                }
            }
            for(int i = p->bindTextures.size() - 1; i >= 0; i--) {
                if (!p->bindTextures[i].texture)
                    p->bindTextures.erase(p->bindTextures.begin() + i);
            }
        }

        D3D11Utility::resolveParams(preset, programs, &frame.texture, &seed);
        onShaderCallback(-1, hasError);
        updateRTS = true;
    }

    auto lock(unsigned*& data, unsigned& pitch, unsigned _width, unsigned _height, uint8_t options = 0) -> bool {
        if (settings.hintExclusiveFullscreen)
            checkFSE();

        if (shaderReady) {
            wait();
            shaderPostBuild();
            shaderReady = false;
        }

        if (settings.threaded)
            return RenderThread::lock(data, pitch, _width, _height, options);

        this->options = options;
        if (swapChain.frameLatency && !settings.vrr)
            WaitForSingleObjectEx( swapChain.frameLatency, 500, true);

        format = DXGI_FORMAT_B8G8R8A8_UNORM;
        if(format != frame.texture.desc.Format || _width != frame.texture.desc.Width || _height != frame.texture.desc.Height) {
            if (!initMainTexture(_width, _height))
                return false;
            viewScreen.update(viewport);
            updateRTS = true;
        }

        D3D11_MAPPED_SUBRESOURCE mappedTexture;
        if (FAILED(context->Map((ID3D11Resource*)frame.texture.staging, 0, D3D11_MAP_WRITE, 0, &mappedTexture)))
            return false;

        data = (unsigned*)mappedTexture.pData;
        pitch = mappedTexture.RowPitch >> 2;

        return true;
    }

    auto lock(float*& data, unsigned& pitch, unsigned _width, unsigned _height, uint8_t options = 0) -> bool {
        if (settings.hintExclusiveFullscreen)
            checkFSE();

        if (shaderReady) {
            wait();
            shaderPostBuild();
            shaderReady = false;
        }

        if (!programs.size()) // YUV input needs a shader to progress it
            return false;

        if (settings.threaded)
            return RenderThread::lock(data, pitch, _width, _height, options);

        this->options = options;
        if (swapChain.frameLatency && !settings.vrr)
            WaitForSingleObjectEx( swapChain.frameLatency, 500, true);

        format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        if(format != frame.texture.desc.Format || _width != frame.texture.desc.Width || _height != frame.texture.desc.Height) {
            if (!initMainTexture(_width, _height))
                return false;
            viewScreen.update(viewport);
            updateRTS = true;
        }

        D3D11_MAPPED_SUBRESOURCE mappedTexture;
        if (FAILED(context->Map((ID3D11Resource*)frame.texture.staging, 0, D3D11_MAP_WRITE, 0, &mappedTexture)))
            return false;

        data = (float*)mappedTexture.pData;
        pitch = mappedTexture.RowPitch >> 4;

        return true;
    }

    auto resize(RenderBuffer* renderBuffer, unsigned w, unsigned h) -> void {
        if (renderBuffer->floatFormat)
            renderBuffer->dataFloat = new float[w * h * 4]();
        else
            renderBuffer->data = new uint32_t[w * h]();

        viewScreen.update(viewport);
        updateRTS = true;
    }

    auto unlockAndRedraw() -> void {
        if (settings.threaded) {
            RenderThread::unlock();
            return;
        }
        context->Unmap((ID3D11Resource*)frame.texture.staging, 0);
        context->CopyResource((ID3D11Resource*)frame.texture.ptr, (ID3D11Resource*)frame.texture.staging);

        redraw(options & OPT_DisallowShader);
    }

    auto refresh() -> void {
        options = 0;
        RenderBuffer* renderBuffer = getBufferToRender();
        if (renderBuffer && renderBuffer->height) {
            if (swapChain.frameLatency && !settings.vrr)
                WaitForSingleObjectEx( swapChain.frameLatency, 500, true);

            renderBuffer->sharedMutex.lock();

            format = renderBuffer->floatFormat ? DXGI_FORMAT_R32G32B32A32_FLOAT : DXGI_FORMAT_B8G8R8A8_UNORM;
            if(format != frame.texture.desc.Format || renderBuffer->width != frame.texture.desc.Width || renderBuffer->height != frame.texture.desc.Height) {
                if (!initMainTexture(renderBuffer->width, renderBuffer->height))
                    return;
            }

            D3D11_MAPPED_SUBRESOURCE mappedTexture;
            if (FAILED(context->Map((ID3D11Resource*)frame.texture.staging, 0, D3D11_MAP_WRITE, 0, &mappedTexture)))
                return;

            int srcPitch = renderBuffer->width << (renderBuffer->floatFormat ? 4 : 2);
            uint8_t* src = renderBuffer->floatFormat ? (uint8_t*)renderBuffer->dataFloat : (uint8_t*)renderBuffer->data;

            unsigned pitch = mappedTexture.RowPitch;
            uint8_t* dest = (uint8_t*)mappedTexture.pData;

            for(int i = 0; i < renderBuffer->height; i++) {
                std::memcpy(dest, src, srcPitch);
                src += srcPitch;
                dest += pitch;
            }

            context->Unmap((ID3D11Resource*)frame.texture.staging, 0);
            context->CopyResource((ID3D11Resource*)frame.texture.ptr, (ID3D11Resource*)frame.texture.staging);

            options = renderBuffer->options;
            renderBuffer->sharedMutex.unlock();

            accessMutex.lock();
            frames--;
            accessMutex.unlock();
        }
#ifdef DRV_FREETYPE
        if (settings.msgUpdated) {
            settings.msgUpdated = false;
            buildMessageTexture(settings.message);
        }
#endif
        redraw(options & OPT_DisallowShader);
    }

    auto redraw(bool disallowShader = false) -> void {
        ID3D11RenderTargetView* rtv = nullptr;
        ID3D11Texture2D* backBuffer = nullptr;
        RECT windowSize = Win::getDimension( settings.handle );

        if ((windowSize.right != viewScreen.windowWidth) || (windowSize.bottom != viewScreen.windowHeight)) {
            resizeMutexThreaded.lock();
            swapChain.ptr->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, swapChain.flags );
            resizeMutexThreaded.unlock();
            viewScreen.update(viewport, windowSize.right, windowSize.bottom);
            updateMessageParameter();
            updateRTS = true; // in the case of passes scaled by viewport
        }

        if (updateRTS) {
            updateRenderTargets(frame.texture.desc.Width, frame.texture.desc.Height);
            updateRTS = false;
        }

        context->RSSetState(scissorDisable);
        context->OMSetBlendState(blendDisable, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        UINT stride = sizeof(D3DVertex);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &frame.vbo, &stride, &offset);

        D3DTexture* texture = &frame.texture;

        if (!disallowShader) {
            seed = (float)Chronos::getTimestampInMilliseconds() / 1000.0;
            for(auto p : programs) {
                applyShader(p->shader);

                for (unsigned int i = 0; i < p->shader.constantBufferCount; i++) {
                    auto& cBuffer = p->shader.constantBuffers[i];

                    D3D11_MAPPED_SUBRESOURCE subRes;
                    if (SUCCEEDED(context->Map( (ID3D11Resource*)cBuffer.constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subRes))) {
                        for(auto& var : cBuffer.variables)
                            std::memcpy((uint8_t*)subRes.pData + var.byteOffset, (uint8_t*)var.value, var.size);

                        context->Unmap((ID3D11Resource*)cBuffer.constantBuffer, 0);
                        context->PSSetConstantBuffers(cBuffer.bindIndex, 1, &cBuffer.constantBuffer);
                    }
                }

                context->VSSetConstantBuffers(0, 1, p == programs.back() ? &ubo : &uboChain);

                ID3D11RenderTargetView* nullRT = nullptr;
                context->OMSetRenderTargets(1, &nullRT, nullptr);

                // it's important to clear unused slots for this pass
                ID3D11ShaderResourceView* textures[16] = { nullptr };
                ID3D11SamplerState*       samplers[16] = { nullptr };

                for(auto& bind : p->bindTextures) {
                    textures[bind.index] = bind.texture->view;
                    samplers[bind.indexSampler] = bind.sampler;
                }

                context->PSSetShaderResources(0, 16, textures);
                context->PSSetSamplers(0, 16, samplers);

                context->OMSetRenderTargets( 1, &p->renderTarget.rtView, nullptr);

                setViewport({p->renderTarget.desc.Width, p->renderTarget.desc.Height, 0, 0});

                context->Draw(4, (p == programs.back()) ? 0 : 4 );

                if (p->mipmap)
                    context->GenerateMips( p->renderTarget.view);

                if (p->crop.active) {
                    context->CopySubresourceRegion((ID3D11Resource*)p->cropTarget.ptr, 0, 0, 0, 0, (ID3D11Resource*)p->renderTarget.ptr, 0, &p->cropBox);
                    texture = &p->cropTarget;
                } else
                    texture = &p->renderTarget;
            }
        }

        resizeMutexThreaded.lock();
        if (FAILED(swapChain.ptr->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer))) {
            resizeMutexThreaded.unlock();
            return;
        }

        if (FAILED(device->CreateRenderTargetView((ID3D11Resource*)backBuffer, nullptr, &rtv)))
            return;

        dxRelease(backBuffer)

        context->OMSetRenderTargets(1, &rtv, nullptr);
        context->ClearRenderTargetView(rtv, clearColor);

        setViewport(viewport);
        applyShader(frame.shader);
        context->PSSetShaderResources(0, 1, &texture->view);
        context->PSSetSamplers(0, 1, &sampler);
        context->VSSetConstantBuffers(0, 1, &uboRotated);

        context->Draw(4, 0);

        context->VSSetConstantBuffers(0, 1, &ubo);
#ifdef DRV_FREETYPE
        if (ft.hasText()) {
            blendRect(message);
        }
#endif
        if (dndOverlay.enabled()) {
            buildOverlayTexture();
            blendRect(overlay);
        }

        if (progressVisible && progress.texture.ptr) {
            setProgressPosition();
            blendRectProgress(progress);
        }

        if (settings.vrr) {
            waitVRR();
        }

        if (settings.synchronize) {
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

        dxRelease(rtv)
    }

    auto updateRenderTargets(unsigned width, unsigned height) -> void {
        int cropScaleX = 1;
        int cropScaleY = 1;

        for(auto p : programs ) {
            if (p->scaleTypeX != ShaderPreset::SCALE_NONE || p->scaleTypeY != ShaderPreset::SCALE_NONE) {
                if (p->scaleTypeX == ShaderPreset::SCALE_INPUT) { width *= p->scaleX; cropScaleX *= p->scaleX; }
                else if (p->scaleTypeX == ShaderPreset::SCALE_VIEWPORT) width = viewport.width * p->scaleX;
                else if (p->scaleTypeX == ShaderPreset::SCALE_ABSOLUTE) width = p->absX;

                if (!width) width = viewport.width;

                if (p->scaleTypeY == ShaderPreset::SCALE_INPUT) { height *= p->scaleY; cropScaleY *= p->scaleY; }
                else if (p->scaleTypeY == ShaderPreset::SCALE_VIEWPORT) height = viewport.width * p->scaleY;
                else if (p->scaleTypeY == ShaderPreset::SCALE_ABSOLUTE) height = p->absY;

                if (!height) height = viewport.height;
            } else if (p == programs.back()) {
                width = viewport.width;
                height = viewport.height;
            }

            if(width != p->renderTarget.desc.Width || height != p->renderTarget.desc.Height) {
                D3D11Utility::releaseTexture(p->renderTarget);
                p->renderTarget.desc.Width = width;
                p->renderTarget.desc.Height = height;
                p->renderTarget.desc.Format = p->format;
                p->renderTarget.desc.BindFlags = D3D11_BIND_RENDER_TARGET;
                if (p->mipmap)
                    p->renderTarget.desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

                if (!D3D11Utility::initTexture(device, p->renderTarget))
                    continue;
            }

            if (p->crop.active) {
                uint8_t interlace = (options & OPT_Interlace) ? 1 : 0;
                width -= (p->crop.left + p->crop.right) * cropScaleX;
                height -= ((p->crop.top + p->crop.bottom) << interlace) * cropScaleY;

                p->cropBox.left   = p->crop.left * cropScaleX;
                p->cropBox.top    = (p->crop.top << interlace) * cropScaleY;
                p->cropBox.front  = 0;
                p->cropBox.right  = width + p->crop.left * cropScaleX;
                p->cropBox.bottom = height + (p->crop.top << interlace) * cropScaleY;
                p->cropBox.back   = 1;

                if(width != p->cropTarget.desc.Width || height != p->cropTarget.desc.Height) {
                    D3D11Utility::releaseTexture(p->cropTarget);
                    p->cropTarget.desc.Width = width;
                    p->cropTarget.desc.Height = height;
                    p->cropTarget.desc.Format = p->format;
                    if (!D3D11Utility::initTexture(device, p->cropTarget, false))
                        continue;
                }
            }
        }
    }

    auto applyShader(D3DShader& shader) -> void {
        context->IASetInputLayout(shader.layout);
        context->VSSetShader(shader.vs, nullptr, 0);
        context->PSSetShader(shader.ps, nullptr, 0);
        context->GSSetShader(shader.gs, nullptr, 0);
    }

    auto blendRect(Rectangle& rect) -> void {
        applyShader(rect.shader);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        UINT stride = sizeof(D3DVertex);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &rect.vbo, &stride, &offset);

        context->PSSetShaderResources(0, 1, &rect.texture.view);
        context->PSSetSamplers(0, 1, &samplers[ShaderPreset::FILTER_LINEAR][ShaderPreset::WRAP_EDGE]);
        context->OMSetBlendState(blendEnable, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
        context->Draw(4, 0);
    }

    auto blendRectProgress(Rectangle& rect) -> void {
        applyShader(rect.shader);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        UINT stride = sizeof(D3DVertex);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &rect.vbo, &stride, &offset);

        context->PSSetShaderResources(0, 1, &rect.texture.view);
        context->PSSetSamplers(0, 1, &samplers[ShaderPreset::FILTER_LINEAR][ShaderPreset::WRAP_EDGE]);

        D3D11_MAPPED_SUBRESOURCE subRes;
        if (SUCCEEDED(context->Map((ID3D11Resource*)constantBufferProgress, 0, D3D11_MAP_WRITE_DISCARD, 0, &subRes))) {
            std::memcpy((uint8_t*)subRes.pData, (uint8_t*)&progressDegree, 4);
            context->Unmap((ID3D11Resource*)constantBufferProgress, 0);
            context->PSSetConstantBuffers(0, 1, &constantBufferProgress);
        }

        if (++progressDegree == 360)
            progressDegree = 0;

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

        D3D11Utility::releaseTexture(frame.texture);
        D3D11Utility::releaseTexture(message.texture);
        D3D11Utility::releaseTexture(overlay.texture);
        D3D11Utility::releaseTexture(progress.texture);

        dxRelease(frame.vbo)
        dxRelease(message.vbo)
        dxRelease(overlay.vbo)
        dxRelease(progress.vbo)

        dxRelease(ubo)
        dxRelease(uboRotated)
        dxRelease(uboChain)
        dxRelease(constantBufferProgress)
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

        dxRelease(debugInfoQueue)
        dxRelease(debug)

        clearSwapChain(swapChain);
        dxRelease(context)
        dxRelease(device)

        dndOverlay.term();
#ifdef DRV_FREETYPE
        ft.term();
#endif
    }

    auto initMainTexture(unsigned w, unsigned h) -> bool {
        D3D11Utility::releaseTexture(frame.texture);
        frame.texture.desc.Width = w;
        frame.texture.desc.Height = h;
        frame.texture.desc.Format = format;
        return D3D11Utility::initTexture(device, frame.texture);
    }

    auto showMessage(std::string message, bool critical = false) -> void {
#ifdef DRV_FREETYPE
        if (settings.message != message || settings.msgCritical != critical) {
            settings.message = message;
            settings.msgCritical = critical;
            if (settings.threaded)
                settings.msgUpdated = true;
            else
                buildMessageTexture(message);
        }
#endif
    }

    auto updateMessageParameter() -> void {
#ifdef DRV_FREETYPE
        if (!ft.hasText())
            return;
        D3D11_MAPPED_SUBRESOURCE mappedVbo;
        if (FAILED(context->Map((ID3D11Resource*)message.vbo, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedVbo)))
            return;

        float screenx = 2.0f / (float)viewport.width, screeny = 2.0f / (float)viewport.height;
        float textx = (float)ft.totalWidth * screenx;
        float texty = (float)ft.totalHeight * screeny;

        float adjust = 0.01;
        float x = 1.0 - textx - adjust;
        float y = -1.0 + texty + adjust;

        float box[4][4] = {
            {x, y, 0, 0},
            {x + textx, y, 1, 0},
            {x, y - texty, 0, 1},
            {x + textx, y - texty, 1, 1}
        };

        bool critical = settings.msgCritical;
        D3DVertex* vertex = (D3DVertex*)mappedVbo.pData;

        for(int i = 0; i < 4; i++) {
            vertex->color[0] = critical ? 0.7 : 1.0;
            vertex->color[1] = critical ? 0.0 : 1.0;
            vertex->color[2] = critical ? 0.0 : 1.0;
            vertex->color[3] = critical ? 1.0 : 0.8;
            vertex->position[0] = box[i][0];
            vertex->position[1] = box[i][1];
            vertex->texcoord[0] = box[i][2];
            vertex->texcoord[1] = box[i][3];
            vertex++;
        }

        context->Unmap((ID3D11Resource*)message.vbo, 0);
#endif
    }

    auto buildMessageTexture(std::string& text) -> void {
#ifdef DRV_FREETYPE
        D3D11Utility::releaseTexture(message.texture);
        if (!ft.buildTexture(text))
            return;

        message.texture.desc.Width = ft.totalWidth;
        message.texture.desc.Height = ft.totalHeight;
        message.texture.desc.Format = DXGI_FORMAT_A8_UNORM;
        if(D3D11Utility::initTexture(device, message.texture, false))
            D3D11Utility::buildTexture(context, message.texture, ft.textBuffer);
#endif
        updateMessageParameter();
    }

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
            vertex->color[0] = 1.0;
            vertex->color[1] = 1.0;
            vertex->color[2] = 1.0;
            vertex->color[3] = 1.0;
            vertex->position[0] = box[i][0];
            vertex->position[1] = box[i][1];
            vertex->texcoord[0] = box[i][2];
            vertex->texcoord[1] = box[i][3];
            vertex++;
        }

        context->Unmap((ID3D11Resource*)overlay.vbo, 0);
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
            vertex->color[0] = 1.0;
            vertex->color[1] = 1.0;
            vertex->color[2] = 1.0;
            vertex->color[3] = 1.0;
            vertex->position[0] = box[i][0];
            vertex->position[1] = box[i][1];
            vertex->texcoord[0] = box[i][2];
            vertex->texcoord[1] = box[i][3];
            vertex++;
        }

        context->Unmap((ID3D11Resource*)progress.vbo, 0);
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

};

}
