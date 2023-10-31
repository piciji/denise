
#include <d3d11_4.h>
#include <d3dcompiler.h>
#include <wrl.h>
#include <thread>
#include <libloaderapi.h>
#include "../../tools/win.h"
#include "../../tools/tools.h"
#include "../../tools/chronos.h"
#include "../viewport.h"
#include "../thread/renderThread.h"
#include "shaders.h"

#ifdef DRV_FREETYPE
#include "../freetype.h"
#endif

//#define D3D_DEBUG

namespace DRIVER {

struct D3D11 : Video, RenderThread {
    struct Matrix4x4 {
        float data[16];
    };

    struct Vertex {
        float position[2];
        float texcoord[2];
        float color[4];
    };

    struct Shader {
        ID3D11VertexShader* vs = nullptr;
        ID3D11PixelShader* ps = nullptr;
        ID3D11GeometryShader* gs = nullptr;
        ID3D11InputLayout* layout = nullptr;
    };

    struct Texture {
        D3D11_TEXTURE2D_DESC desc;
        ID3D11Texture2D* ptr = nullptr;
        ID3D11Texture2D* staging = nullptr;
        ID3D11ShaderResourceView* view = nullptr;
    };

    struct Rectangle {
        Texture texture;
        Shader shader;
        ID3D11Buffer* vbo;
    };

    Rectangle frame;
    Rectangle overlay;
    Rectangle message;

#ifdef DRV_FREETYPE
    Freetype ft;
#endif
    DragndropOverlay dndOverlay;
    ViewScreen viewScreen;
    Viewport viewport;

    ID3D11Device* device;
    ID3D11DeviceContext* context;
    IDXGISwapChain2* swapChain;

    HANDLE frameLatency;

    ID3D11Buffer* ubo;
    ID3D11SamplerState* samplers[2][4];

    ID3D11BlendState* blendEnable;
    ID3D11BlendState* blendDisable;

    ID3D11RasterizerState* scissorEnable;
    ID3D11RasterizerState* scissorDisable;

    ID3D11InfoQueue* debugInfoQueue;
    ID3D11Debug* debug;

    float clearColor[4] = {0.0, 0.0, 0.0, 1.0};
    UINT swapFlags;

    int64_t lastCapTime;
    int64_t minimumCapTime;

    // a matrix multiplication with this model-view doesn't change the result, so we don't apply it for now
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
        bool hintExclusiveFullscreen = false;
        std::vector<ShaderPass*> passes = {};
    } settings;

    D3D11() {
        debugInfoQueue = nullptr;
        debug = nullptr;
        frameLatency = nullptr;
        swapChain = nullptr;
        device = nullptr;
        context = nullptr;
        ubo = nullptr;
        frame.vbo = nullptr;
        message.vbo = nullptr;
        overlay.vbo = nullptr;

        blendEnable = nullptr;
        blendDisable = nullptr;
        scissorEnable = nullptr;
        scissorDisable = nullptr;
        swapFlags = 0;
        settings.handle = nullptr;
        settings.msgUpdated = false;
        settings.hintExclusiveFullscreen = false;
        settings.exclusiveFullscreen = false;

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

    auto hintExclusiveFullscreen(bool state, float rate = 0.0) -> void {
        settings.hintExclusiveFullscreen = state;
    }

    auto canExclusiveFullscreen() -> bool { return true; }

    auto hasExclusiveFullscreen() -> bool { return settings.exclusiveFullscreen; }

    auto disableExclusiveFullscreen() -> void {
        if (settings.exclusiveFullscreen) {
            wait();
            swapChain->SetFullscreenState(false, nullptr);
            settings.exclusiveFullscreen = false;
        }
    }

    auto canHardSync() -> bool { return true; }

    auto setFilter(Filter filter) -> void {
        if (filter == settings.filter)
            return;
        wait();
        settings.filter = filter;
    }

    auto hardSync(bool state) -> void {
        wait();
        settings.hardSync = state;
        if (settings.handle)
            init();
    }

    auto synchronize(bool state) -> void {
        settings.synchronize = state;
    }

    auto hasSynchronized() -> bool { return settings.synchronize; }

    auto setRatio(int mode, bool integerScaling) -> void { // mode: 0: off, 1: TV, 2: Native
        if ((int)viewScreen.mode == mode && viewScreen.hasIntegerScaling == integerScaling)
            return;

        wait();
        viewScreen.mode = (ViewScreen::Mode)mode;
        viewScreen.hasIntegerScaling = integerScaling;
        if (settings.handle) {
            viewScreen.update(viewport);
            setViewport(viewport);
        }
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

    auto init(uintptr_t handle) -> bool {
        settings.handle = (HWND) handle;
        return init();
    }

    auto checkSupport() -> int {
        Microsoft::WRL::ComPtr<IDXGIFactory1> dxgiFactory1 = nullptr;
        Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory2 = nullptr;
        Microsoft::WRL::ComPtr<IDXGIFactory5> dxgiFactory5 = nullptr;
        int state = 0;

        if (SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory2), (void**)dxgiFactory2.ReleaseAndGetAddressOf()))) {
            state |= 1;
            if (SUCCEEDED(dxgiFactory2->QueryInterface(__uuidof(IDXGIFactory5), (void**)dxgiFactory5.ReleaseAndGetAddressOf()))) {
                state |= 2;
                if (dxgiFactory5) {
                    int allowTearing;
                    if (SUCCEEDED(dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE::DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing)))) {
                        if (allowTearing)
                            state |= 8;
                    }
                }
            }
        } else if (SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)dxgiFactory1.ReleaseAndGetAddressOf()))) {
            state |= 4;
        }

//        dxRelease(dxgiFactory1);
//        dxRelease(dxgiFactory2);
//        dxRelease(dxgiFactory5);
        return state;
    }

    auto init() -> bool {
        term();
        int support = checkSupport();

        logger->log("sup: " + std::to_string(support));
        if (!support)
            return false;

        D3D_FEATURE_LEVEL features[] = {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0
        };

        unsigned deviceFlags = 0;
#ifdef D3D_DEBUG
        deviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif

        if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, deviceFlags, features, 3, D3D11_SDK_VERSION, &device, nullptr, &context))) {
            logger->log("error device");
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

        if (support & 4) {
            IDXGIDevice1* dxgiDevice = nullptr;
            IDXGIAdapter* dxgiAdapter = nullptr;
            IDXGIFactory1* dxgiFactory = nullptr;
            DXGI_SWAP_CHAIN_DESC desc;
            std::memset(&desc, 0, sizeof(desc) );

            if (FAILED(device->QueryInterface(__uuidof(IDXGIDevice1), (void**)&dxgiDevice))) {
                logger->log("error dxgi");
                return term(), false;
            }

            if (FAILED(dxgiDevice->GetAdapter(&dxgiAdapter))) {
                logger->log("error adapter");
                dxRelease(dxgiDevice)
                return term(), false;
            }

            if (FAILED(dxgiAdapter->GetParent(__uuidof(IDXGIFactory1), (void**)&dxgiFactory))) {
                logger->log("error factory");
                dxRelease(dxgiAdapter)
                dxRelease(dxgiDevice)
                return term(), false;
            }

            desc.BufferDesc.Width = 0;
            desc.BufferDesc.Height = 0;
            desc.BufferCount = 2;
            desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.OutputWindow = settings.handle;
            desc.Windowed = true;

            desc.Flags = (support & 8) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
            if (settings.hardSync)
                desc.Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
            desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

            if (FAILED(dxgiFactory->CreateSwapChain((IUnknown*)device, &desc, (IDXGISwapChain**)&swapChain))) {
                desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
                desc.Flags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
                desc.Flags &= ~DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
                logger->log("error flip");
                if (FAILED(dxgiFactory->CreateSwapChain((IUnknown*) device, &desc, (IDXGISwapChain**)&swapChain))) {
                    logger->log("error non flip");
                    dxRelease(dxgiAdapter)
                    dxRelease(dxgiDevice)
                    dxRelease(dxgiFactory)
                    return term(), false;
                }
            }

            swapFlags = desc.Flags;

            if (FAILED(dxgiFactory->MakeWindowAssociation(settings.handle, DXGI_MWA_NO_ALT_ENTER))) {}

            if (swapFlags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT) {
                if SUCCEEDED(dxgiDevice->SetMaximumFrameLatency(1)) {
                    logger->log("success frame latency");
                }
            }

            dxRelease(dxgiFactory)
            dxRelease(dxgiAdapter)
            dxRelease(dxgiDevice)

        } else {
            IDXGIDevice2* dxgiDevice = nullptr;
            IDXGIAdapter* dxgiAdapter = nullptr;
            IDXGIFactory2* dxgiFactory = nullptr;
            DXGI_SWAP_CHAIN_DESC1 desc;
            std::memset(&desc, 0, sizeof(desc) );

            if (FAILED(device->QueryInterface(__uuidof(IDXGIDevice2), (void**) &dxgiDevice))) {
                logger->log("error dxgi");
                return term(), false;
            }

            if (FAILED(dxgiDevice->GetAdapter(&dxgiAdapter))) {
                logger->log("error adapter");
                dxRelease(dxgiDevice)
                return term(), false;
            }

            if (FAILED(dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), (void**) &dxgiFactory))) {
                logger->log("error factory");
                dxRelease(dxgiAdapter)
                dxRelease(dxgiDevice)
                return term(), false;
            }

            desc.Width = 0;
            desc.Height = 0;
            desc.BufferCount = 2;
            desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;

            desc.Flags = (support & 8) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
            if (settings.hardSync)
                desc.Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
            desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

            if (FAILED(dxgiFactory->CreateSwapChainForHwnd((IUnknown*) device, settings.handle, &desc, nullptr, nullptr, (IDXGISwapChain1**) &swapChain))) {
                desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
                desc.Flags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
                desc.Flags &= ~DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
                logger->log("error flip");

                if (FAILED(dxgiFactory->CreateSwapChainForHwnd((IUnknown*) device, settings.handle, &desc, nullptr, nullptr, (IDXGISwapChain1**) &swapChain))) {
                    logger->log("error non flip");
                    dxRelease(dxgiAdapter)
                    dxRelease(dxgiDevice)
                    dxRelease(dxgiFactory)
                    return term(), false;
                }
            }

            swapFlags = desc.Flags;

            if (FAILED(dxgiFactory->MakeWindowAssociation(settings.handle, DXGI_MWA_NO_ALT_ENTER))) {}

            if (swapFlags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT) {
                frameLatency = swapChain->GetFrameLatencyWaitableObject();
                logger->log("wait");
                if (frameLatency)
                    if (SUCCEEDED( swapChain->SetMaximumFrameLatency(1))) {
                        logger->log("wait 1");
                    }
            }

            dxRelease(dxgiFactory)
            dxRelease(dxgiAdapter)
            dxRelease(dxgiDevice)
        }

        if (!initMainTexture(32, 32))
            return term(), false;

        D3D11_SUBRESOURCE_DATA uboData;
        uboData.pSysMem = &projection;
        uboData.SysMemPitch = 0;
        uboData.SysMemSlicePitch = 0;

        D3D11_BUFFER_DESC descP;
        std::memset(&descP, 0, sizeof(descP));
        descP.ByteWidth           = sizeof(projection);
        descP.Usage               = D3D11_USAGE_DYNAMIC;
        descP.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
        descP.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
        descP.MiscFlags           = 0;
        descP.StructureByteStride = 0;

        if (FAILED(device->CreateBuffer(&descP, &uboData, &ubo)))
            return term(), false;

        D3D11_SAMPLER_DESC descS;
        std::memset(&descS, 0, sizeof(descS));
        descS.MaxAnisotropy = 1;
        descS.ComparisonFunc = D3D11_COMPARISON_NEVER;
        descS.MinLOD = -D3D11_FLOAT32_MAX;
        descS.MaxLOD = D3D11_FLOAT32_MAX;
        logger->log("suc 1");
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
        logger->log("suc 2");
        D3D11_SUBRESOURCE_DATA vertexData;
        D3D11_BUFFER_DESC descV;
        Vertex vertices[] = {
            {{0.0f,  0.0f},  {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
            {{0.0f,  1.0f},  {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
            {{1.0f,  0.0f},  {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
            {{1.0f,  1.0f},  {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
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
        logger->log("suc 3");
        descV.Usage = D3D11_USAGE_DYNAMIC;
        descV.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        descV.ByteWidth = sizeof(Vertex) * 4;
        if (FAILED(device->CreateBuffer(&descV, nullptr, &message.vbo)))
            return term(), false;
        if (FAILED(device->CreateBuffer(&descV, nullptr, &overlay.vbo)))
            return term(), false;
        logger->log("suc 4");
        D3D11_INPUT_ELEMENT_DESC descShader[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, texcoord), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, color), D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        if (!createShader(device, D3D11outputShader, "PS", "VS", "", descShader, countof(descShader), &frame.shader))
            return term(), false;
        logger->log("suc 5");
        if (!createShader(device, D3D11messageShader, "PS", "VS", "", descShader, countof(descShader), &message.shader))
            return term(), false;
        logger->log("suc 6");
        if (!createShader(device, D3D11overlayShader, "PS", "VS", "", descShader, countof(descShader), &overlay.shader))
            return term(), false;
        logger->log("suc 7");
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
        logger->log("suc 8");
        blendDesc.RenderTarget[0].BlendEnable           = false;
        if (FAILED(device->CreateBlendState(&blendDesc, &blendDisable)))
            return term(), false;
        logger->log("suc 9");
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
        logger->log("suc ok");
        return true;
    }

    auto shader(std::vector<ShaderPass*>& passes) -> void {
        ShaderPass* primaryPass = nullptr;

        for(auto pass : passes) {
            if (pass->primary) {
                primaryPass = pass;
                break;
            }
        }

        for(auto pass : passes) {
            if (pass->primary)
                continue;


        }
    }

    auto lock(unsigned*& data, unsigned& pitch, unsigned _width, unsigned _height, bool reuse = false) -> bool {
        if (settings.threaded)
            return RenderThread::lock(data, pitch, _width, _height, reuse);

        if (frameLatency && !settings.vrr)
            WaitForSingleObjectEx( frameLatency, 500, true);

        if(_width != frame.texture.desc.Width || _height != frame.texture.desc.Height) {
            if (!initMainTexture(_width, _height))
                return false;
        }

        D3D11_MAPPED_SUBRESOURCE mappedTexture;
        if (FAILED(context->Map((ID3D11Resource*)frame.texture.staging, 0, D3D11_MAP_WRITE, 0, &mappedTexture)))
            return false;

        data = (unsigned*)mappedTexture.pData;
        pitch = mappedTexture.RowPitch >> 2;

        return true;
    }

    auto resize(RenderBuffer* renderBuffer, unsigned w, unsigned h) -> void {
        renderBuffer->data = new uint32_t[w * h]();
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
            if (frameLatency && !settings.vrr)
                WaitForSingleObjectEx( frameLatency, 500, true);

            renderBuffer->sharedMutex.lock();

            if(renderBuffer->width != frame.texture.desc.Width || renderBuffer->height != frame.texture.desc.Height) {
                if (!initMainTexture(renderBuffer->width, renderBuffer->height))
                    return;
            }

            D3D11_MAPPED_SUBRESOURCE mappedTexture;
            if (FAILED(context->Map((ID3D11Resource*)frame.texture.staging, 0, D3D11_MAP_WRITE, 0, &mappedTexture)))
                return;

            int srcPitch = renderBuffer->width << 2;
            uint8_t* src = (uint8_t*)renderBuffer->data;
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
        RECT windowSize = getDimension( settings.handle );

        if ((windowSize.right != viewScreen.windowWidth) || (windowSize.bottom != viewScreen.windowHeight)) {
            if(settings.hintExclusiveFullscreen) {
                int adapterId = Win::getFullscreenAdapter( Win::getParentHandle( settings.handle ) );
                if (adapterId >= 0) {
                    settings.exclusiveFullscreen = true;
                    swapChain->SetFullscreenState(true, nullptr);
                }
            } else if(settings.exclusiveFullscreen) {
                settings.exclusiveFullscreen = false;
                swapChain->SetFullscreenState(false, nullptr);
            }

            swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, swapFlags );
            viewScreen.update(viewport, windowSize.right, windowSize.bottom);
            setViewport(viewport);
            updateMessageParameter();
        }

        if (FAILED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer)))
            return;

        if (FAILED(device->CreateRenderTargetView((ID3D11Resource*)backBuffer, nullptr, &rtv)))
            return;

        dxRelease(backBuffer)

        context->RSSetState(scissorDisable);
        context->OMSetBlendState(blendDisable, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &frame.vbo, &stride, &offset);
        context->OMSetRenderTargets(1, &rtv, nullptr);
        context->ClearRenderTargetView(rtv, clearColor);

        setShader(frame.shader);
        context->PSSetShaderResources(0, 1, &frame.texture.view);
        context->PSSetSamplers(0, 1, &samplers[(int)settings.filter][1]);
        context->VSSetConstantBuffers(0, 1, &ubo);

        context->Draw(4, 0);

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
            swapChain->Present(1, 0);
            IDXGIOutput* pOutput;
            swapChain->GetContainingOutput(&pOutput);
            pOutput->WaitForVBlank();
            pOutput->Release();
        } else
            swapChain->Present(0, (swapFlags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) ? DXGI_PRESENT_ALLOW_TEARING : 0);

        if (settings.vrr) {
            context->Flush();
        }

        dxRelease(rtv)
    }

    auto setShader(Shader& shader) -> void {
        context->IASetInputLayout(shader.layout);
        context->VSSetShader(shader.vs, nullptr, 0);
        context->PSSetShader(shader.ps, nullptr, 0);
        context->GSSetShader(shader.gs, nullptr, 0);
    }

    auto blendRect(Rectangle& rect) -> void {
        setShader(rect.shader);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &rect.vbo, &stride, &offset);

        context->PSSetShaderResources(0, 1, &rect.texture.view);
        context->PSSetSamplers(0, 1, &samplers[(int)Video::Filter::Linear][0]);
        context->OMSetBlendState(blendEnable, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
        context->Draw(4, 0);
    }

    auto setViewport(Viewport& viewport) -> void {
        D3D11_VIEWPORT d3D11Viewport;
        d3D11Viewport.Width = viewport.width;
        d3D11Viewport.Height = viewport.height;
        d3D11Viewport.TopLeftX = viewport.x;
        d3D11Viewport.TopLeftY = viewport.y;
        d3D11Viewport.MaxDepth = 1.0;
        d3D11Viewport.MinDepth = 0.0;
        context->RSSetViewports(1, &d3D11Viewport);
    }

    auto createShader( ID3D11Device* device, const std::string& data, const std::string psEntry, const std::string vsEntry, const std::string gsEntry,
         const D3D11_INPUT_ELEMENT_DESC* inputElementDescs, unsigned numElements, Shader* out) -> bool {

        ID3DBlob* error = nullptr;
        const char* msg = nullptr;

        ID3DBlob* psCode = nullptr;
        ID3DBlob* vsCode = nullptr;
        ID3DBlob* gsCode = nullptr;

        if (!psEntry.empty() && FAILED(D3DCompile(data.c_str(), data.size(), nullptr, nullptr, nullptr, psEntry.c_str(), "ps_5_0", 0, 0, &psCode, &error)))
            msg = (const char*)error->GetBufferPointer();

        if (!vsEntry.empty() && FAILED(D3DCompile(data.c_str(), data.size(), nullptr, nullptr, nullptr, vsEntry.c_str(), "vs_5_0", 0, 0, &vsCode, &error)))
            msg = (const char*)error->GetBufferPointer();

        if (!gsEntry.empty() && FAILED(D3DCompile(data.c_str(), data.size(), nullptr, nullptr, nullptr, gsEntry.c_str(), "gs_5_0", 0, 0, &gsCode, &error)))
            msg = (const char*)error->GetBufferPointer();

        if (psCode)
            if (FAILED(device->CreatePixelShader( psCode->GetBufferPointer(), psCode->GetBufferSize(), nullptr, &out->ps)))
                msg = (const char*)-1;

        if (vsCode) {
            LPVOID bufPtr  = vsCode->GetBufferPointer();
            SIZE_T bufSize = vsCode->GetBufferSize();
            if (FAILED(device->CreateVertexShader(bufPtr, bufSize, nullptr, &out->vs)))
                msg = (const char*)-2;

            if (inputElementDescs)
                if (FAILED(device->CreateInputLayout(inputElementDescs, numElements, bufPtr, bufSize, &out->layout)))
                    msg = (const char*)-3;
        }

        if (gsCode)
            if (FAILED(device->CreateGeometryShader(gsCode->GetBufferPointer(), gsCode->GetBufferSize(), nullptr, &out->gs)))
                msg = (const char*)-4;

        dxRelease(vsCode)
        dxRelease(psCode)
        dxRelease(gsCode)
        dxRelease(error)

        return msg == nullptr;
    }

    auto term() -> void {
        wait();
        if(context) {
            context->ClearState();
            context->Flush();
        }

        if (frameLatency) {
            CloseHandle(frameLatency);
            frameLatency = nullptr;
        }

        releaseTexture(frame.texture);
        releaseTexture(message.texture);
        releaseTexture(overlay.texture);

        dxRelease(frame.vbo)
        dxRelease(message.vbo)
        dxRelease(overlay.vbo)

        dxRelease(ubo)
        dxRelease(blendEnable)
        dxRelease(blendDisable)
        dxRelease(scissorEnable)
        dxRelease(scissorDisable)

        for(int i = 0; i < 4; i++) {
            dxRelease(samplers[(int)Video::Filter::Nearest][i])
            dxRelease(samplers[(int)Video::Filter::Linear][i])
        }

        releaseShader(&frame.shader);
        releaseShader(&message.shader);
        releaseShader(&overlay.shader);

        dxRelease(debugInfoQueue)
        dxRelease(debug)

        dxRelease(swapChain)
        dxRelease(context)
        dxRelease(device)

        dndOverlay.term();
#ifdef DRV_FREETYPE
        ft.term();
#endif
    }

    auto initMainTexture(unsigned w, unsigned h, bool useStaging = true) -> bool {
        releaseTexture(frame.texture);
        frame.texture.desc.Width = w;
        frame.texture.desc.Height = h;
        frame.texture.desc.Format = DXGI_FORMAT_B8G8R8X8_UNORM;
        return initTexture(frame.texture, useStaging);
    }

    auto initTexture(Texture& tex, bool useStaging = true) -> bool {
        tex.desc.MipLevels          = 1;
        tex.desc.ArraySize          = 1;
        tex.desc.SampleDesc.Count   = 1;
        tex.desc.SampleDesc.Quality = 0;
        tex.desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
        tex.desc.Usage              = useStaging ? D3D11_USAGE_DEFAULT : D3D11_USAGE_DYNAMIC;
        if (!useStaging)
            tex.desc.CPUAccessFlags     = D3D11_CPU_ACCESS_WRITE;

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

        if (useStaging) {
            D3D11_TEXTURE2D_DESC desc = tex.desc;
            desc.MipLevels = 1;
            desc.BindFlags = 0;
            desc.MiscFlags = 0;
            desc.Usage = D3D11_USAGE_STAGING;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            if (FAILED(device->CreateTexture2D(&desc, nullptr, &tex.staging)))
                return false;
        }
        return true;
    }

    auto releaseTexture(Texture& tex) -> void {
        std::memset(&tex.desc, 0, sizeof(tex.desc));
        dxRelease(tex.view)
        dxRelease(tex.staging)
        dxRelease(tex.ptr)
    }

    auto getDimension(HWND handle) -> RECT {
        RECT rect;
        GetClientRect(handle, &rect);

        return rect;
    }

    auto releaseShader(Shader* shader) -> void {
        dxRelease(shader->layout)
        dxRelease(shader->vs)
        dxRelease(shader->ps)
        dxRelease(shader->gs)
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
        Vertex* vertex = (Vertex*)mappedVbo.pData;

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
        releaseTexture(message.texture);
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
        releaseTexture(overlay.texture);
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
        Vertex* vertex = (Vertex*) mappedVbo.pData;

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

    auto waitVRR() -> void {

        lastCapTime += minimumCapTime;
        int64_t remaining  = lastCapTime - Chronos::getTimestampInMicroseconds();

        if (remaining <= 0) {
            lastCapTime = Chronos::getTimestampInMicroseconds();
            return;
        }

        if (remaining >= 10000) {

            remaining -= 8000;

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
