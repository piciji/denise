
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

//#define D3D_DEBUG

namespace DRIVER {

struct D3D11 : Video, RenderThread, DXGIHandler {
    struct Matrix4x4 {
        float data[16];
    };

    struct Rectangle {
        D3DTexture texture;
        D3DShader shader;
        ID3D11Buffer* vbo;
    };

    Rectangle frame;
    Rectangle overlay;
    Rectangle message;
    std::vector<D3DProgram> programs;

#ifdef DRV_FREETYPE
    Freetype ft;
#endif
    DragndropOverlay dndOverlay;
    ViewScreen viewScreen;
    Viewport viewport;

    ID3D11Device* device;
    ID3D11DeviceContext* context;
    SwapChain swapChain;

    ID3D11Buffer* ubo;
    ID3D11Buffer* uboRotated;
    ID3D11Buffer* uboChain;
    ID3D11SamplerState* samplers[2][4];
    DXGI_FORMAT format;

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

    Matrix4x4 projectionRotated;

    struct {
        bool synchronize = false;
        bool hardSync = false;
        Filter filter = Video::Filter::Linear;
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
        std::vector<ShaderPass*> passes = {};
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
        uboChain = nullptr;
        frame.vbo = nullptr;
        message.vbo = nullptr;
        overlay.vbo = nullptr;

        blendEnable = nullptr;
        blendDisable = nullptr;
        scissorEnable = nullptr;
        scissorDisable = nullptr;
        settings.handle = nullptr;
        settings.msgUpdated = false;
        settings.hintExclusiveFullscreen = false;
        settings.exclusiveFullscreen = false;
        settings.rotation = ~0;

        for(auto& sampler : samplers)
            for(auto& _sampler : sampler)
                _sampler = nullptr;
    }

    ~D3D11() {
        RenderThread::enable(false);
        term();
    }

    auto setShader(std::vector<ShaderPass*> passes) -> void {
        wait();
        settings.passes = passes;
        shader( passes );
        RenderThread::reset();
    }

    auto setShaderAttribute( std::string _program, std::string attribute, float value ) -> void {
        shaderAttribute<float>(_program, attribute, value);
    }

    auto setShaderAttribute( std::string _program, std::string attribute, int value ) -> void {
        shaderAttribute<int>(_program, attribute, value);
    }

    auto setShaderAttribute(std::string _program, std::string attribute, float* data, unsigned size) -> void {
        shaderAttribute(_program, attribute, data, size);
    }

    auto setShaderAttribute(std::string _program, std::string attribute, uint32_t* data, unsigned _width, unsigned _height) -> void {
        shaderAttribute(_program, attribute, data, _width, _height);
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

    auto setFilter(Filter filter) -> void {
        if (filter == settings.filter)
            return;
        wait();
        settings.filter = filter;
        frame.texture.sampler = samplers[(int)filter][1];
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

    auto setIntegerScalingDimension( unsigned _w, unsigned _h, bool _ds) -> void {
        viewScreen.scaling.width = _w;
        viewScreen.scaling.height = _h;
        viewScreen.scaling.doubleSize = _ds;
    }

    auto getIntegerScalingDimension(unsigned& _w, unsigned& _h) -> void {
        _w = viewScreen.scaling.width;
        _h = viewScreen.scaling.height;
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

    auto shaderFormat() -> ShaderType { return ShaderType::HLSL; }

    auto setVRR(bool state, float speed = 0.0) -> void {
        wait();
        settings.vrr = state;

        if (state) {
            minimumCapTime = (1000000.0 / speed) + 0.5;
            lastCapTime = Chronos::getTimestampInMicroseconds();
        }
    }

    auto hasVRR() -> bool { return settings.vrr; }

    auto getRotation() -> unsigned { return settings.rotation; }

    auto setRotation(unsigned degree) -> void {
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

        if (FAILED(symbols.D3D11Create(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, deviceFlags, features, 3, D3D11_SDK_VERSION, &device, nullptr, &context))) {
            return term(), false;
        }

#ifdef D3D_DEBUG
        if (SUCCEEDED(device->QueryInterface(__uuidof(ID3D11Debug), (void**)&debug))) {
            if (SUCCEEDED(debug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&debugInfoQueue))) {
                debugInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
                debugInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
                debugInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, TRUE);
            }
        }
#endif

        if (!initSwapChain(symbols, device, settings.handle, settings.hardSync, swapChain))
            return false;

        format = D3D11Utility::_format();
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
            if (FAILED(device->CreateSamplerState( &descS, &samplers[(int)Video::Filter::Linear][i])))
                return term(), false;

            descS.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
            if (FAILED(device->CreateSamplerState( &descS, &samplers[(int)Video::Filter::Nearest][i])))
                return term(), false;
        }
        frame.texture.sampler = samplers[(int)settings.filter][1];

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

        D3D11_INPUT_ELEMENT_DESC descShader[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(D3DVertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(D3DVertex, texcoord), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(D3DVertex, color), D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        if (!D3D11Utility::createShader(symbols, device, D3D11outputShader, "PS", "VS", "", descShader, countof(descShader), &frame.shader))
            return term(), false;

        if (!D3D11Utility::createShader(symbols, device, D3D11messageShader, "PS", "VS", "", descShader, countof(descShader), &message.shader))
            return term(), false;

        if (!D3D11Utility::createShader(symbols, device, D3D11overlayShader, "PS", "VS", "", descShader, countof(descShader), &overlay.shader))
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

    auto shader(std::vector<ShaderPass*>& passes) -> void {
        for(auto& program : programs) {
            D3D11Utility::releaseShader(program.shader);
            D3D11Utility::releaseTexture(program.renderTarget);
            for (auto& tex : program.textures) {
                D3D11Utility::releaseTexture(tex);
            }
        }
        programs.clear();

        ShaderPass* primaryPass = nullptr;

        for(auto pass : passes) {
            if (pass->primary) {
                primaryPass = pass;
                continue;
            }
            programs.push_back({});
            D3DProgram& program = programs.back();
            D3D11Utility::buildProgram(symbols, device, program, *pass);
            program.renderTarget.sampler = samplers[ program.filter ][ program.wrap ];
        }

        if (primaryPass) {
            format = D3D11Utility::_format(primaryPass->format);
            frame.texture.sampler = samplers[ D3D11Utility::_filter(primaryPass->filter) ][D3D11Utility::_wrap( primaryPass->wrap )];
        } else {
            format = D3D11Utility::_format();
            frame.texture.sampler = samplers[ (int)settings.filter ][1];
        }

        if (format != frame.texture.desc.Format)
            initMainTexture( frame.texture.desc.Width, frame.texture.desc.Height );
    }

    auto lock(unsigned*& data, unsigned& pitch, unsigned _width, unsigned _height, bool reuse = false) -> bool {
        if (settings.hintExclusiveFullscreen)
            checkFSE();

        if (settings.threaded)
            return RenderThread::lock(data, pitch, _width, _height, reuse);

        if (swapChain.frameLatency && !settings.vrr)
            WaitForSingleObjectEx( swapChain.frameLatency, 500, true);

        if(_width != frame.texture.desc.Width || _height != frame.texture.desc.Height) {
            if (!initMainTexture(_width, _height))
                return false;
            viewScreen.update(viewport);
        }

        D3D11_MAPPED_SUBRESOURCE mappedTexture;
        if (FAILED(context->Map((ID3D11Resource*)frame.texture.staging, 0, D3D11_MAP_WRITE, 0, &mappedTexture)))
            return false;

        data = (unsigned*)mappedTexture.pData;
        pitch = mappedTexture.RowPitch >> 2;

        return true;
    }

    auto lock(float*& data, unsigned& pitch, unsigned _width, unsigned _height, bool reuse = false) -> bool {
        if (settings.hintExclusiveFullscreen)
            checkFSE();

        if (settings.threaded)
            return RenderThread::lock(data, pitch, _width, _height, reuse);

        if (swapChain.frameLatency && !settings.vrr)
            WaitForSingleObjectEx( swapChain.frameLatency, 500, true);

        if(_width != frame.texture.desc.Width || _height != frame.texture.desc.Height) {
            if (!initMainTexture(_width, _height))
                return false;
            viewScreen.update(viewport);
        }

        D3D11_MAPPED_SUBRESOURCE mappedTexture;
        if (FAILED(context->Map((ID3D11Resource*)frame.texture.staging, 0, D3D11_MAP_WRITE, 0, &mappedTexture)))
            return false;

        data = (float*)mappedTexture.pData;
        pitch = mappedTexture.RowPitch >> 4;

        return true;
    }

    auto resize(RenderBuffer* renderBuffer, unsigned w, unsigned h) -> void {
        if (format == DXGI_FORMAT_R32G32B32A32_FLOAT)
            renderBuffer->dataFloat = new float[w * h * 4]();
        else
            renderBuffer->data = new uint32_t[w * h]();

        viewScreen.update(viewport);
    }

    auto unlockAndRedraw(bool disallowShader = false, bool freeContext = false) -> void {
        if (settings.threaded) {
            RenderThread::unlock(disallowShader);
            return;
        }
        context->Unmap((ID3D11Resource*)frame.texture.staging, 0);
        context->CopyResource((ID3D11Resource*)frame.texture.ptr, (ID3D11Resource*)frame.texture.staging);

        redraw(disallowShader);
    }

    auto refresh() -> void {
        bool disallowShader = false;
        RenderBuffer* renderBuffer = getBufferToRender();
        if (renderBuffer && renderBuffer->height) {
            if (swapChain.frameLatency && !settings.vrr)
                WaitForSingleObjectEx( swapChain.frameLatency, 500, true);

            renderBuffer->sharedMutex.lock();

            if(renderBuffer->width != frame.texture.desc.Width || renderBuffer->height != frame.texture.desc.Height) {
                if (!initMainTexture(renderBuffer->width, renderBuffer->height))
                    return;
            }

            D3D11_MAPPED_SUBRESOURCE mappedTexture;
            if (FAILED(context->Map((ID3D11Resource*)frame.texture.staging, 0, D3D11_MAP_WRITE, 0, &mappedTexture)))
                return;

            int srcPitch = renderBuffer->width << (format == DXGI_FORMAT_R32G32B32A32_FLOAT ? 4 : 2);
            uint8_t* src = (uint8_t*)renderBuffer->data;
            if (format == DXGI_FORMAT_R32G32B32A32_FLOAT)
                src = (uint8_t*)renderBuffer->dataFloat;

            unsigned pitch = mappedTexture.RowPitch;
            uint8_t* dest = (uint8_t*)mappedTexture.pData;

            for(int i = 0; i < renderBuffer->height; i++) {
                std::memcpy(dest, src, srcPitch);
                src += srcPitch;
                dest += pitch;
            }

            context->Unmap((ID3D11Resource*)frame.texture.staging, 0);
            context->CopyResource((ID3D11Resource*)frame.texture.ptr, (ID3D11Resource*)frame.texture.staging);

            disallowShader = renderBuffer->disallowShader;
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
        redraw(disallowShader);
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
        }

        context->RSSetState(scissorDisable);
        context->OMSetBlendState(blendDisable, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        UINT stride = sizeof(D3DVertex);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &frame.vbo, &stride, &offset);

        D3DTexture* textures[2] = {&frame.texture, nullptr};

        if (!disallowShader) {
            auto ts = Chronos::getTimestampInMicroseconds();
            unsigned targetWidth;
            unsigned targetHeight;

            for(auto& p : programs) {
                if (p.crop.active) {
                    targetWidth = textures[0]->desc.Width - p.crop.left - p.crop.right;
                    targetHeight = textures[0]->desc.Height - p.crop.top - p.crop.bottom;

                    if(targetWidth != p.renderTarget.desc.Width || targetHeight != p.renderTarget.desc.Height) {
                        D3D11Utility::releaseTexture(p.renderTarget);
                        p.renderTarget.desc.Width = targetWidth;
                        p.renderTarget.desc.Height = targetHeight;
                        p.renderTarget.desc.Format = textures[0]->desc.Format;
                        if (!initTexture(p.renderTarget, false))
                            return;
                    }

                    D3D11_BOX frameBox;
                    frameBox.left   = p.crop.left;
                    frameBox.top    = p.crop.top;
                    frameBox.front  = 0;
                    frameBox.right  = targetWidth + p.crop.left;
                    frameBox.bottom = targetHeight + p.crop.top;
                    frameBox.back   = 1;
                    context->CopySubresourceRegion((ID3D11Resource*)p.renderTarget.ptr, 0, 0, 0, 0, (ID3D11Resource*)textures[0]->ptr, 0, &frameBox);
                    textures[1] = textures[0];
                    textures[0] = &p.renderTarget;
                    continue;
                }

                targetWidth = viewport.width;
                targetHeight = viewport.height;
                if(p.relativeWidth) targetWidth = textures[0]->desc.Width * p.relativeWidth;
                if(p.relativeHeight) targetHeight = textures[0]->desc.Height * p.relativeHeight;

                if(targetWidth != p.renderTarget.desc.Width || targetHeight != p.renderTarget.desc.Height) {
                    D3D11Utility::releaseTexture(p.renderTarget);
                    p.renderTarget.desc.Width = targetWidth;
                    p.renderTarget.desc.Height = targetHeight;
                    p.renderTarget.desc.Format = p.format;
                    p.renderTarget.desc.BindFlags = D3D11_BIND_RENDER_TARGET;
                    if (p.mipmap)
                        p.renderTarget.desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

                    if (!initTexture(p.renderTarget))
                        return;
                }

                applyShader(p.shader);

                D3D11Utility::setConstantInt(p.shader, "ts", ts);
                float _targetSize[4] = {(float)targetWidth, (float)targetHeight, 1.0f / (float)targetWidth, 1.0f / (float)targetHeight};
                D3D11Utility::setConstantFloat4(p.shader, "targetSize", _targetSize );

                float srcWidth = textures[0]->desc.Width;
                float srcHeight = textures[0]->desc.Height;
                float _sourceSize[4] = {srcWidth, srcHeight, 1.0f / srcWidth, 1.0f / srcHeight};
                D3D11Utility::setConstantFloat4(p.shader, "sourceSize", _sourceSize );
                D3D11Utility::updateConstantData(context, p.shader, "scene");

                for (unsigned int i = 0; i < p.shader.constantBufferCount; i++)
                    context->PSSetConstantBuffers(p.shader.constantBuffers[i].bindIndex, 1, &p.shader.constantBuffers[i].constantBuffer);

                context->VSSetConstantBuffers(0, 1, &p == &programs.back() ? &ubo : &uboChain);

                ID3D11RenderTargetView* null_rt = nullptr;
                context->OMSetRenderTargets(1, &null_rt, nullptr);

                if (p.bindTexture.index >= 0) {
                    context->PSSetShaderResources(p.bindTexture.index, 1, &textures[0]->view);
                    context->PSSetSamplers(p.bindTexture.indexSampler, 1, &textures[0]->sampler);
                }

                if (textures[1] && (p.bindPrevTexture.index >= 0) ) {
                    context->PSSetShaderResources(p.bindPrevTexture.index, 1, &textures[1]->view);
                    context->PSSetSamplers(p.bindPrevTexture.indexSampler, 1, &textures[1]->sampler);
                }

                for(auto& texture : p.textures) {
                    context->PSSetShaderResources(texture.bind.index, 1, &texture.view);
                    context->PSSetSamplers(texture.bind.indexSampler, 1, &texture.sampler);
                }

                context->OMSetRenderTargets( 1, &p.renderTarget.rtView, nullptr);

                setViewport({targetWidth, targetHeight, 0, 0});

                context->Draw(4, (&p == &programs.back()) ? 0 : 4 );

                if (p.mipmap)
                    context->GenerateMips( p.renderTarget.view);

                textures[1] = textures[0];
                textures[0] = &p.renderTarget;
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
        context->PSSetShaderResources(0, 1, &textures[0]->view);
        context->PSSetSamplers(0, 1, &textures[0]->sampler);
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
        context->PSSetSamplers(0, 1, &samplers[(int)Video::Filter::Linear][0]);
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

        dxRelease(frame.vbo)
        dxRelease(message.vbo)
        dxRelease(overlay.vbo)

        dxRelease(ubo)
        dxRelease(uboRotated)
        dxRelease(uboChain)
        dxRelease(blendEnable)
        dxRelease(blendDisable)
        dxRelease(scissorEnable)
        dxRelease(scissorDisable)

        for(int i = 0; i < 4; i++) {
            dxRelease(samplers[(int)Video::Filter::Nearest][i])
            dxRelease(samplers[(int)Video::Filter::Linear][i])
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
        return initTexture(frame.texture);
    }

    auto initTexture(D3DTexture& tex, bool useStaging = true) -> bool {
        tex.desc.MipLevels          = 1;
        tex.desc.ArraySize          = 1;
        tex.desc.SampleDesc.Count   = 1;
        tex.desc.SampleDesc.Quality = 0;
        tex.desc.BindFlags          |= D3D11_BIND_SHADER_RESOURCE;
        tex.desc.Usage              = useStaging ? D3D11_USAGE_DEFAULT : D3D11_USAGE_DYNAMIC;
        if (!useStaging)
            tex.desc.CPUAccessFlags     = D3D11_CPU_ACCESS_WRITE;

        bool render = tex.desc.BindFlags & D3D11_BIND_RENDER_TARGET;

        if (tex.desc.MiscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS) {
            tex.desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
            unsigned width = tex.desc.Width >> 2;
            unsigned height = tex.desc.Height >> 2;

            while (width && height) { // based on log2
                width >>= 1; 
                height >>= 1;
                tex.desc.MipLevels++;
            }
        }

        if (FAILED(device->CreateTexture2D(&tex.desc, nullptr, &tex.ptr)))
            return false;

        D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
        std::memset(&viewDesc, 0, sizeof(viewDesc));
        viewDesc.Format                          = tex.desc.Format;
        viewDesc.ViewDimension                   = D3D_SRV_DIMENSION_TEXTURE2D;
        viewDesc.Texture2D.MostDetailedMip       = 0;
        viewDesc.Texture2D.MipLevels             = -1;
        if (FAILED(device->CreateShaderResourceView((ID3D11Resource*)tex.ptr, &viewDesc, &tex.view)))
            return false;

        if (render) {
            if (FAILED(device->CreateRenderTargetView((ID3D11Resource*)tex.ptr, nullptr, &tex.rtView)))
                return false;

        } else if (useStaging) {
            D3D11_TEXTURE2D_DESC desc = tex.desc;
            desc.BindFlags = 0;
            desc.MiscFlags = 0;
            desc.Usage = D3D11_USAGE_STAGING;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            if (FAILED(device->CreateTexture2D(&desc, nullptr, &tex.staging)))
                return false;
        }
        return true;
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
        initTexture(message.texture, false);

        D3D11_MAPPED_SUBRESOURCE mappedTexture;
        if (FAILED(context->Map( (ID3D11Resource*)message.texture.ptr, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedTexture)))
            return;

        int srcPitch = ft.totalWidth;
        uint8_t* src = ft.textBuffer;
        unsigned pitch = mappedTexture.RowPitch;
        uint8_t* dest = (uint8_t*)mappedTexture.pData;

        for(int i = 0; i < ft.totalHeight; i++) {
            std::memcpy(dest, src, srcPitch);
            src += srcPitch;
            dest += pitch;
        }

        context->Unmap((ID3D11Resource*)message.texture.ptr, 0);
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
        initTexture(overlay.texture, false);

        D3D11_MAPPED_SUBRESOURCE mappedTexture;
        if (FAILED(context->Map( (ID3D11Resource*)overlay.texture.ptr, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedTexture)))
            return;

        int srcPitch = dndOverlay.texWidth << 2;
        uint8_t* src = dndOverlay.buffer;
        unsigned pitch = mappedTexture.RowPitch;
        uint8_t* dest = (uint8_t*)mappedTexture.pData;

        for(int i = 0; i < dndOverlay.texHeight; i++) {
            std::memcpy(dest, src, srcPitch);
            src += srcPitch;
            dest += pitch;
        }

        context->Unmap((ID3D11Resource*)overlay.texture.ptr, 0);

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

    template<typename T> auto shaderAttribute( std::string _program, std::string attribute, T value ) -> void {
        wait();
        bool success = false;
        for(auto& program : programs) {
            if (program.ident == _program) {
                if (std::is_same<T, int>::value) {
                    if (D3D11Utility::setConstantInt(program.shader, attribute, value)) { success = true; }
                } else {
                    if (D3D11Utility::setConstantFloat(program.shader, attribute, value)) { success = true; }
                }
                if (success) {
                    D3D11Utility::updateConstantData(context, program.shader, "$Globals");
                }
                break;
            }
        }
    }

    auto shaderAttribute(std::string _program, std::string attribute, float* data, unsigned size) -> void {
        D3DTexture* customTexture = nullptr;
        wait();
        for(auto& program : programs) {
            if (program.ident == _program) {
                customTexture = D3D11Utility::findRessource(program, attribute);
                break;
            }
        }
        if (!customTexture)
            return;

        D3D11Utility::releaseTexture( *customTexture );
        customTexture->desc.Width = size;
        customTexture->desc.Height = 1;
        customTexture->desc.Format = DXGI_FORMAT_R32_FLOAT;
        customTexture->sampler = samplers[(int)Video::Filter::Nearest][0];
        if (!initTexture(*customTexture, false))
            return;

        D3D11_MAPPED_SUBRESOURCE mappedTexture;
        if (FAILED(context->Map( (ID3D11Resource*)customTexture->ptr, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedTexture)))
            return;

        unsigned pitch = mappedTexture.RowPitch;
        uint8_t* dest = (uint8_t*)mappedTexture.pData;
        std::memcpy(dest, (uint8_t*)data, size * 4);

        context->Unmap((ID3D11Resource*)customTexture->ptr, 0);
    }

    auto shaderAttribute(std::string _program, std::string attribute, uint32_t* data, unsigned _width, unsigned _height ) -> void {
        D3DTexture* customTexture = nullptr;
        wait();

        for(auto& program : programs) {
            if (program.ident == _program) {
                customTexture = D3D11Utility::findRessource(program, attribute);
                break;
            }
        }
        if (!customTexture)
            return;

        D3D11Utility::releaseTexture( *customTexture );
        customTexture->desc.Width = _width;
        customTexture->desc.Height = _height;
        customTexture->desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        customTexture->desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
        customTexture->sampler = samplers[(int)Video::Filter::Linear][2];
        if (!initTexture(*customTexture, true))
            return;

        D3D11_MAPPED_SUBRESOURCE mappedTexture;
        if (FAILED(context->Map( (ID3D11Resource*)customTexture->staging, 0, D3D11_MAP_WRITE, 0, &mappedTexture)))
            return;

        int srcPitch = _width << 2;
        uint8_t* src = (uint8_t*)data;
        unsigned pitch = mappedTexture.RowPitch;
        uint8_t* dest = (uint8_t*)mappedTexture.pData;

        for(int i = 0; i < _height; i++) {
            std::memcpy(dest, src, srcPitch);
            src += srcPitch;
            dest += pitch;
        }

        context->Unmap((ID3D11Resource*)customTexture->staging, 0);
        context->CopyResource((ID3D11Resource*)customTexture->ptr, (ID3D11Resource*)customTexture->staging);
        context->GenerateMips( customTexture->view);
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
