
#pragma once

#include <wrl.h>

#define DXGI_CLEAR \
    dxRelease(dxgiAdapter) \
    dxRelease(dxgiDevice) \
    dxRelease(dxgiFactory)

namespace DRIVER {

struct DXGIHandler {

    auto initSwapChain(D3D11Symbols& symbols, ID3D11Device* device, HWND handle, bool hardSync, bool& useHdr, SwapChain& swapChain, bool windowed = true, float rate = 0.0) -> bool {
        if (swapChain.ptr )
            swapChain.ptr->SetFullscreenState(false, nullptr);

        clearSwapChain(swapChain);
        int support = checkSupport(symbols);
        RECT outScreenParent;
        if (!windowed)
            outScreenParent = Win::getDimension( handle );

        //logger->log("D3D11 support: " + std::to_string(support));
        if (!support)
            return false;

        //support = 4;
        if (support & 4) {
            useHdr = false;
            IDXGIDevice1* dxgiDevice = nullptr;
            IDXGIAdapter* dxgiAdapter = nullptr;
            IDXGIFactory1* dxgiFactory = nullptr;
            DXGI_SWAP_CHAIN_DESC desc;
            std::memset(&desc, 0, sizeof(desc) );

            if (FAILED(device->QueryInterface(__uuidof(IDXGIDevice1), (void**)&dxgiDevice))) {
                DXGI_CLEAR
                return false;
            }

            if (FAILED(dxgiDevice->GetAdapter(&dxgiAdapter))) {
                DXGI_CLEAR
                return false;
            }

            if (FAILED(dxgiAdapter->GetParent(__uuidof(IDXGIFactory1), (void**)&dxgiFactory))) {
                DXGI_CLEAR
                return false;
            }

            desc.BufferDesc.Width = windowed ? 0 : outScreenParent.right;
            desc.BufferDesc.Height = windowed ? 0 : outScreenParent.bottom;
            desc.BufferCount = 2;
            desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.OutputWindow = handle;
            desc.Windowed = windowed;
            desc.BufferDesc.RefreshRate.Numerator = windowed ? 0 : ((UINT)(rate * 1000.0));
            desc.BufferDesc.RefreshRate.Denominator = windowed ? 0 : 1000;
            desc.Flags = 0;
            desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            if (FAILED(dxgiFactory->CreateSwapChain((IUnknown*) device, &desc, (IDXGISwapChain**)&swapChain.ptr))) {
                DXGI_CLEAR
                return false;
            }

            if (FAILED(dxgiFactory->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES))) {}

            if (hardSync || !windowed) {
                if SUCCEEDED(dxgiDevice->SetMaximumFrameLatency(1)) {}
            }

            DXGI_CLEAR

        } else {
            IDXGIDevice2* dxgiDevice = nullptr;
            IDXGIAdapter* dxgiAdapter = nullptr;
            IDXGIFactory2* dxgiFactory = nullptr;
            DXGI_SWAP_CHAIN_DESC1 desc;
            DXGI_SWAP_CHAIN_FULLSCREEN_DESC descF;

            if (FAILED(device->QueryInterface(__uuidof(IDXGIDevice2), (void**) &dxgiDevice))) {
                DXGI_CLEAR
                return false;
            }

            if (FAILED(dxgiDevice->GetAdapter(&dxgiAdapter))) {
                DXGI_CLEAR
                return false;
            }

            if (FAILED(dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), (void**) &dxgiFactory))) {
                DXGI_CLEAR
                return false;
            }

            std::memset(&desc, 0, sizeof(desc) );
            desc.Width = windowed ? 0 : outScreenParent.right;
            desc.Height = windowed ? 0 : outScreenParent.bottom;
            desc.BufferCount = 2;
            desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            desc.Format = useHdr ? DXGI_FORMAT_R10G10B10A2_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;

            if (!windowed) { // FSE
                desc.Flags = 0;
                desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // to prevent eFSE, we want old FSE here
                std::memset(&descF, 0, sizeof(descF));
                descF.Windowed = false;
                descF.RefreshRate.Numerator = (UINT) (rate * 1000.0);
                descF.RefreshRate.Denominator = 1000;
                descF.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
            } else {
                desc.Flags = (support & 8) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
                if (hardSync)
                    desc.Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
                desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // FSO in Win10/11
            }

            if (FAILED(dxgiFactory->CreateSwapChainForHwnd((IUnknown*) device, handle, &desc, windowed ? nullptr : &descF, nullptr, (IDXGISwapChain1**) &swapChain.ptr))) {
                desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // FSB or FSE in Win7
                desc.Flags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
                desc.Flags &= ~DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

                if (FAILED(dxgiFactory->CreateSwapChainForHwnd((IUnknown*) device, handle, &desc, windowed ? nullptr : &descF, nullptr, (IDXGISwapChain1**) &swapChain.ptr))) {
                    DXGI_CLEAR
                    return false;
                }
            }

            swapChain.flags = desc.Flags;

            if (FAILED(dxgiFactory->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES))) {}

            if (swapChain.flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT) {
                swapChain.frameLatency = swapChain.ptr->GetFrameLatencyWaitableObject();
                if (swapChain.frameLatency) {
                    if (SUCCEEDED(swapChain.ptr->SetMaximumFrameLatency(1))) {}
                }
            } else if (hardSync || !windowed) {
                if SUCCEEDED(dxgiDevice->SetMaximumFrameLatency(1)) {}
            }

            DXGI_CLEAR
        }

        if (useHdr) {
            if (!setColorSpace(swapChain, DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)) {
                useHdr = false;
                return initSwapChain(symbols, device, handle, hardSync, useHdr, swapChain);
            }
        }

        return true;
    }

    auto clearSwapChain(SwapChain& swapChain) -> void {
        if (swapChain.frameLatency) {
            CloseHandle(swapChain.frameLatency);
            swapChain.frameLatency = nullptr;
        }

        dxRelease(swapChain.ptr)
        swapChain.flags = 0;
    }

    static auto checkSupport(D3D11Symbols& symbols) -> int {
        static int support = -1;

        if (support >= 0)
            return support;

        support = 0;
        Microsoft::WRL::ComPtr<IDXGIFactory1> dxgiFactory1 = nullptr;
        Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory2 = nullptr;
        Microsoft::WRL::ComPtr<IDXGIFactory5> dxgiFactory5 = nullptr;

        if (SUCCEEDED(symbols.CreateDXGIFactory1(__uuidof(IDXGIFactory2), (void**) dxgiFactory2.ReleaseAndGetAddressOf()))) {
            support |= 1; // minimum: Windows 7 with "Platform update"
            if (SUCCEEDED(dxgiFactory2->QueryInterface(__uuidof(IDXGIFactory5), (void**) dxgiFactory5.ReleaseAndGetAddressOf()))) {
                support |= 2; // minimum: Windows 8
                if (dxgiFactory5) {
                    int allowTearing;
                    if (SUCCEEDED( dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE::DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing)))) {
                        if (allowTearing)
                            support |= 8;
                    }
                }
            }
        } else if (SUCCEEDED( symbols.CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**) dxgiFactory1.ReleaseAndGetAddressOf()))) {
            support |= 4; // Windows 7 without "Platform update"
        }

        return support;
    }

    auto setColorSpace(SwapChain& swapChain, DXGI_COLOR_SPACE_TYPE colorSpace) -> bool {
        UINT colorSpaceSupport = 0;

        if (SUCCEEDED(((IDXGISwapChain4*)swapChain.ptr)->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport))) {
            if ((colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)
                == DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) {

                if (SUCCEEDED(((IDXGISwapChain4*)swapChain.ptr)->SetColorSpace1(colorSpace)))
                    return true;
            }
        }
        return false;
    }

    auto setHdrChain(SwapChain& swapChain, DXGI_COLOR_SPACE_TYPE colorSpace, float maxNits) -> void {

        if (colorSpace != DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020) {
            ((IDXGISwapChain4*)swapChain.ptr)->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_NONE, 0, NULL);
            return;
        }

        DXGI_HDR_METADATA_HDR10 hdr10MetaData;
        hdr10MetaData.RedPrimary[0] = 0.708f * 50000.0f;
        hdr10MetaData.RedPrimary[1] = 0.292f * 50000.0f;
        hdr10MetaData.GreenPrimary[0] = 0.170f * 50000.0f;
        hdr10MetaData.GreenPrimary[1] = 0.797f * 50000.0f;
        hdr10MetaData.BluePrimary[0] = 0.131f * 50000.0f;
        hdr10MetaData.BluePrimary[1] = 0.046f * 50000.0f;
        hdr10MetaData.WhitePoint[0] = 0.3127f * 50000.0f;
        hdr10MetaData.WhitePoint[1] = 0.3290f * 50000.0f;

        hdr10MetaData.MinMasteringLuminance = 0.001 * 10000.0f;
        hdr10MetaData.MaxMasteringLuminance = maxNits;
        hdr10MetaData.MaxContentLightLevel = 0;
        hdr10MetaData.MaxFrameAverageLightLevel = 0;

        if (FAILED(((IDXGISwapChain4*)swapChain.ptr)->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(DXGI_HDR_METADATA_HDR10), &hdr10MetaData))) {
            
        }
    }

};

}

#undef DXGI_CLEAR