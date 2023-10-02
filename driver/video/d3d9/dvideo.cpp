
#include "../thread/renderThread.h"
#include "../viewport.h"
#include "../../tools/win.h"
#include "../../tools/tools.h"
#include "../../tools/chronos.h"
#include <d3d9.h>
#include <d3dx9.h>
#include <math.h>
#include <uxtheme.h>
#include <cstring>

namespace DRIVER {

#include "dragnDropOverlay.h"

#define D3DVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)
#ifndef MAX_MONITORS
#define MAX_MONITORS 9
#endif

static HMONITOR monList[MAX_MONITORS];
static unsigned monPos = 0;

auto CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) -> BOOL;

struct DVideo : Video, RenderThread {
    D3d9DragndropOverlay dndOverlay;
    LPDIRECT3D9 lpD3D;
    D3DPRESENT_PARAMETERS d3dpp;
    LPDIRECT3DDEVICE9 lpD3DDevice;
    D3DCAPS9 d3dcaps;
    LPDIRECT3DSURFACE9 surface;
    LPDIRECT3DTEXTURE9 texture;
    LPDIRECT3DVERTEXBUFFER9 vertex_buffer, *vertex_ptr;
    D3DLOCKED_RECT d3dlr;
    std::vector<LPD3DXEFFECT> effects;
    ID3DXFont* mFont;
    const unsigned FONT_WIDTH = 1024;
    unsigned textureWidth = 0;
    unsigned textureHeight = 0;

    ViewScreen viewScreen;
    Viewport viewport;

    unsigned inputWidth, inputHeight;
    std::mutex noteMutex;
	const bool XPMode;

    int64_t lastCapTime;
    int64_t minimumCapTime;

    struct {
        bool synchronize;
        Filter filter = Video::Filter::Linear;
        std::vector<ShaderPass*> passes = {};
        HWND handle;
        HWND parent;
        bool hintExclusiveFullscreen = false;
        float exclusiveFullscreenRate = 0.0;
        bool exclusiveFullscreen = false;
        bool threaded = false;

        bool vrr = false;
    } settings;

    struct {
        bool dynamic;
        bool shader;
    } caps;

    struct {
        unsigned t_usage, v_usage;
        unsigned t_pool, v_pool;
        unsigned lock;
        unsigned filter;
    } flags;

    struct {
        wchar_t* message;
        bool enable = false;
        D3DCOLOR fontColor;
    } note;

    struct d3dvertex {
        float x, y, z, rhw; //screen coords
        unsigned color; //diffuse color
        float u, v; //texture coords
    };

    bool lost;

    auto releaseResources() -> void {
        releaseShader();
        dxRelease(surface);
        dxRelease(texture);
        dxRelease(vertex_buffer);
        dxRelease(mFont);
    }

    auto releaseShader() -> void {
        for (auto effect : effects) {
            dxRelease(effect);
        }
        effects.clear();
    }

    auto setDragnDropOverlay(uint8_t* _data, unsigned _width, unsigned _height, unsigned line = 0) -> void {
        dndOverlay.setDragnDropOverlay(_data, _width, _height, line);
    }

    auto setDragnDropOverlaySlots(unsigned slots) -> void {
        dndOverlay.setSlots(slots);
    }

    auto enableDragnDropOverlay(bool state) -> void {
        dndOverlay.enable = state;
    }

    auto sendDragnDropOverlayCoordinates(int x, int y) -> int {
        return dndOverlay.sendDragnDropOverlayCoordinates(x, y);
    }

    auto updateFilter() -> void {
        if (!lpD3DDevice) return;
        if (lost && !recover()) {
            if (!init())
                return;
        }

        flags.filter = D3DTEXF_POINT;
        if (settings.filter == Video::Filter::Nearest) flags.filter = D3DTEXF_POINT;
        else if (settings.filter == Video::Filter::Linear) flags.filter = D3DTEXF_LINEAR;

		lpD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
		lpD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
		
        lpD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, flags.filter);
        lpD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, flags.filter);
        lpD3DDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, flags.filter);
    }

    auto setVertex(unsigned src_w, unsigned src_h, unsigned tex_w, unsigned tex_h, unsigned dest_x, unsigned dest_y, unsigned dest_w, unsigned dest_h) -> void {
        d3dvertex vertex[4];

        vertex[0].x = vertex[2].x = ((float) dest_x - 0.5f);
        vertex[1].x = vertex[3].x = ((float) dest_x + (float) dest_w - 0.5f);
        vertex[0].y = vertex[1].y = ((float) dest_y - 0.5f);
        vertex[2].y = vertex[3].y = ((float) dest_y + (float) dest_h - 0.5f);

        vertex[0].z = vertex[1].z = 1.0;
        vertex[2].z = vertex[3].z = 1.0;
        vertex[0].rhw = vertex[1].rhw = 1.0;
        vertex[2].rhw = vertex[3].rhw = 1.0;

        if (settings.filter == Video::Filter::Linear) {
            vertex[0].u = vertex[2].u = 0.0f;
            vertex[1].u = vertex[3].u = ((float) (src_w) - 0.5f) / (float) tex_w;
            vertex[0].v = vertex[1].v = 0.0f;
            vertex[2].v = vertex[3].v = ((float) (src_h) - 0.5f) / (float) tex_h;
        } else {
            vertex[0].u = vertex[2].u = 0.0f;
            vertex[1].u = vertex[3].u = ((float) (src_w) + 0.0f) / (float) tex_w;
            vertex[0].v = vertex[1].v = 0.0f;
            vertex[2].v = vertex[3].v = ((float) (src_h) + 0.0f) / (float) tex_h;
        }

        vertex_buffer->Lock(0, sizeof (d3dvertex) * 4, (void**) &vertex_ptr, 0);
        std::memcpy(vertex_ptr, vertex, sizeof (d3dvertex) * 4);
        vertex_buffer->Unlock();

        lpD3DDevice->SetStreamSource(0, vertex_buffer, 0, sizeof (d3dvertex));
    }

    auto setShader(std::vector<ShaderPass*> passes) -> void {

        settings.passes = passes;
        releaseShader();

        for (auto pass : passes) {

            if (pass->fragment.empty())
                continue;

            if (!caps.shader || !lpD3DDevice) {
                pass->error = "no shader support";
                continue;
            }

            if (pass->fragment.empty()) continue;

            LPD3DXBUFFER pBufferErrors = nullptr;
            LPD3DXEFFECT effect = nullptr;

            HRESULT hr = D3DXCreateEffect(lpD3DDevice, (const void*) pass->fragment.c_str(), lstrlenA(pass->fragment.c_str()), NULL, NULL, 0, NULL, &effect, &pBufferErrors);

            if (pBufferErrors) {
                char* errorMessage = (char*) pBufferErrors->GetBufferPointer();
                std::string str(errorMessage);
                pass->error = str;
                delete pBufferErrors;
            }

            if (!SUCCEEDED(hr)) continue;

            D3DXHANDLE hTech;
            effect->FindNextValidTechnique(NULL, &hTech);
            effect->SetTechnique(hTech);

            effects.push_back(effect);
        }
    }

    auto getFullscreenAdapter(HWND handle) -> int {
        MONITORINFO info;
        info.cbSize = sizeof(info);
        HMONITOR hMon = MonitorFromWindow( handle, MONITOR_DEFAULTTONEAREST);

        if (!hMon)
            return -1;

        if (GetMonitorInfo(hMon, &info)) {
            RECT outScreenParent = getDimension( handle );
            unsigned monitorWidth = std::abs(info.rcMonitor.right - info.rcMonitor.left);
            unsigned monitorHeight = std::abs(info.rcMonitor.bottom - info.rcMonitor.top);

            if ( (outScreenParent.right == monitorWidth)
                 && (outScreenParent.bottom == monitorHeight) ) {

                for (unsigned i = 0; i < monPos; i++) {
                    if (monList[i] == hMon) {
                        return i;
                    }
                }
            }
        }
        return -1;
    }

    auto reset(bool exclusiveFullscreenNeedInit = true) -> bool {
        bool exclusiveFullscreen = false;

        HWND handle = settings.handle;
        RECT windowSize = getDimension( handle );
        viewScreen.update( viewport, windowSize.right, windowSize.bottom );

        RECT outScreenParent;
		settings.parent = getParentHandle();

        if (settings.hintExclusiveFullscreen) {
            handle = settings.parent;
            outScreenParent = getDimension( handle );

            if ( getFullscreenAdapter(handle) >= 0 ) {
                exclusiveFullscreen = true;
				_clear();

                if (exclusiveFullscreenNeedInit)
                    return false;
            } else {
                handle = settings.handle;
            }
        }

        ZeroMemory(&d3dpp, sizeof (d3dpp));

        d3dpp.hDeviceWindow = handle;
        d3dpp.Windowed = !exclusiveFullscreen;
        d3dpp.BackBufferFormat = exclusiveFullscreen ? D3DFMT_X8R8G8B8 : D3DFMT_UNKNOWN;
        d3dpp.BackBufferWidth = exclusiveFullscreen ? outScreenParent.right : 0;
        d3dpp.BackBufferHeight = exclusiveFullscreen ? outScreenParent.bottom : 0;
        d3dpp.FullScreen_RefreshRateInHz = (exclusiveFullscreen && (settings.exclusiveFullscreenRate > 0.0))
                                           ? settings.exclusiveFullscreenRate : D3DPRESENT_RATE_DEFAULT;

        d3dpp.PresentationInterval = settings.synchronize ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
        d3dpp.BackBufferCount = 1;
        d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER | D3DPRESENTFLAG_VIDEO;

        d3dpp.MultiSampleQuality = 0; //antialiasing
        d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE; //antialiasing
        d3dpp.EnableAutoDepthStencil = false; //Z-Buffer
        d3dpp.AutoDepthStencilFormat = D3DFMT_UNKNOWN; //Z-Buffer

        settings.exclusiveFullscreen = exclusiveFullscreen;

        lost = true;
        return recover();
    }

    auto recover() -> bool {
        if (!lpD3DDevice) {
            return false;
        }

        if (lost) {
            releaseResources();
            if (lpD3DDevice->Reset(&d3dpp) != D3D_OK) return false;
        }

        lost = false;

        lpD3DDevice->SetDialogBoxMode(false);

        lpD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        lpD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE); //Textur color
        lpD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE); //Vertex color -ignore

        lpD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        lpD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE); //Textur alpha
        lpD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE); //Vertex alpha -ignore
		
        lpD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        lpD3DDevice->SetRenderState(D3DRS_LIGHTING, false);
        lpD3DDevice->SetRenderState(D3DRS_ZENABLE, false);

        lpD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        lpD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        lpD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);

        lpD3DDevice->SetFVF(D3DVERTEX); //set vertex format

        lpD3DDevice->CreateVertexBuffer(sizeof (d3dvertex) * 4, flags.v_usage, D3DVERTEX, static_cast<D3DPOOL> (flags.v_pool), &vertex_buffer, NULL);

        textureWidth = 0;
        textureHeight = 0;
        resize(inputWidth, inputHeight);

        D3DXCreateFont(lpD3DDevice, 15, 0, 0, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                       DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Source Code Pro"), &mFont);
        note.enable = false;

        updateFilter();
        //RenderThread::reset();
        return true;
    }

    auto term() -> void {
        wait();
        releaseResources();
        dxRelease(lpD3DDevice);
        dxRelease(lpD3D);
    }

    auto init(bool disallowExclusiveFullscreen = false) -> bool {
        int adapterId = -1;
		HWND taskbar = NULL;
		if (XPMode)
			taskbar = FindWindow(L"Shell_TrayWnd", NULL);
        term();
	
        if (monPos == 0) {
            EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)NULL );
        }

        if ((lpD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL) {
            return false;
        }

        bool exclusiveFullscreen = false;

        HWND handle = settings.handle;
        RECT windowSize = getDimension( handle );
        viewScreen.update( viewport, windowSize.right, windowSize.bottom );

        RECT outScreenParent;
        settings.parent = getParentHandle();

        if (settings.hintExclusiveFullscreen && !disallowExclusiveFullscreen) {
            handle = settings.parent;
            outScreenParent = getDimension( handle );
            adapterId = getFullscreenAdapter(handle);

            if (adapterId >= 0) {
                exclusiveFullscreen = true;				
				showTaskbar(taskbar, false);
            } else {
                handle = settings.handle;
            }
        }
		if (!exclusiveFullscreen )
			showTaskbar(taskbar, true);
		
        ZeroMemory(&d3dpp, sizeof (d3dpp));

        d3dpp.hDeviceWindow = handle;

        d3dpp.Windowed = !exclusiveFullscreen;
        d3dpp.BackBufferFormat = exclusiveFullscreen ? D3DFMT_X8R8G8B8 : D3DFMT_UNKNOWN;
        d3dpp.BackBufferWidth = exclusiveFullscreen ? outScreenParent.right : 0;
        d3dpp.BackBufferHeight = exclusiveFullscreen ? outScreenParent.bottom : 0;
        d3dpp.FullScreen_RefreshRateInHz = (exclusiveFullscreen && (settings.exclusiveFullscreenRate > 0.0))
                                           ? settings.exclusiveFullscreenRate : D3DPRESENT_RATE_DEFAULT;

        d3dpp.PresentationInterval = settings.synchronize ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
        d3dpp.BackBufferCount = 1;
        d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER | D3DPRESENTFLAG_VIDEO;

        d3dpp.MultiSampleQuality = 0; //antialiasing
        d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE; //antialiasing
        d3dpp.EnableAutoDepthStencil = false; //Z-Buffer
        d3dpp.AutoDepthStencilFormat = D3DFMT_UNKNOWN; //Z-Buffer

        HRESULT hr = lpD3D->CreateDevice(exclusiveFullscreen ? adapterId : D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, handle,
                                         D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &lpD3DDevice);

        if (!SUCCEEDED(hr) || !lpD3DDevice) {
            return false;
        }

        lpD3DDevice->GetDeviceCaps(&d3dcaps);

        caps.dynamic = !!(d3dcaps.Caps2 & D3DCAPS2_DYNAMICTEXTURES);
        caps.shader = d3dcaps.PixelShaderVersion > D3DPS_VERSION(1, 4);

        if (caps.dynamic) {
            flags.t_usage = D3DUSAGE_DYNAMIC;
            flags.v_usage = D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC;
            flags.t_pool = D3DPOOL_DEFAULT;
            flags.v_pool = D3DPOOL_DEFAULT;
            flags.lock = D3DLOCK_NOSYSLOCK | D3DLOCK_DISCARD;
        } else {
            flags.t_usage = 0;
            flags.v_usage = D3DUSAGE_WRITEONLY;
            flags.t_pool = D3DPOOL_MANAGED;
            flags.v_pool = D3DPOOL_MANAGED;
            flags.lock = D3DLOCK_NOSYSLOCK | D3DLOCK_DISCARD;
        }

        settings.exclusiveFullscreen = exclusiveFullscreen;
        lost = false;
        inputWidth = 256, inputHeight = 256;
        recover();
        RenderThread::reset();

        setShader(settings.passes);
        dndOverlay.init(lpD3DDevice);
        return true;
    }

    auto init(uintptr_t handle) -> bool {
        settings.handle = (HWND) handle;
        return init(true);
    }

	auto showTaskbar(HWND taskbar, bool state) -> void {
		
		if (taskbar != NULL) {
			if (!state && IsWindowVisible(taskbar)) {
				HMONITOR hMon = MonitorFromWindow( settings.handle, MONITOR_DEFAULTTONEAREST);
				const POINT ptZero = { 0, 0 };
				
				if (hMon == MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY))
					ShowWindow(taskbar, SW_HIDE);
			
			} else if (state && !IsWindowVisible(taskbar))
				ShowWindow(taskbar, SW_SHOW);
		}
	}
	
    auto unlockAndRedraw(bool disallowShader = false, bool freeContext = false) -> void {

        if (settings.threaded) {
            resizeMutex.lock();
            RenderThread::unlock(disallowShader);
            resizeMutex.unlock();
        } else {
            resizeMutex.lock();
            if (surface) {
                surface->UnlockRect();
                dxRelease(surface);
            }

            _redraw(disallowShader);

            resizeMutex.unlock();
        }
    }


    auto redraw(bool disallowShader = false) -> void {
		if (XPMode)
			return redrawCustom(disallowShader);			
		
        resizeMutexThreaded.lock();
		_redraw(disallowShader);
        resizeMutexThreaded.unlock();
    }

    auto redrawCustom(bool disallowShader = false) -> void {
        resizeMutexThreaded.lock();

        lpD3DDevice->BeginScene();

        lpD3DDevice->SetTexture(0, texture);

        lpD3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

        lpD3DDevice->EndScene();

        if (lpD3DDevice->Present(0, 0, 0, 0) == D3DERR_DEVICELOST) {
            lost = true;
        }
		
        resizeMutexThreaded.unlock();
    }
	
    auto _redraw(bool disallowShader) -> void {
        if( settings.synchronize && IsIconic( settings.parent ) )
            return;

        lpD3DDevice->BeginScene();
        setVertex(inputWidth, inputHeight, textureWidth, textureHeight, viewport.x, viewport.y, viewport.width, viewport.height);
        lpD3DDevice->SetTexture(0, texture);

        if (!disallowShader && caps.shader && effects.size() > 0) {
            applyShader(viewport.width, viewport.height);
        } else {
            lpD3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
        }

        if (dndOverlay.enabled()) {
            dndOverlay.show(viewport);
            setVertex(dndOverlay.texWidth, dndOverlay.texHeight, dndOverlay.texStorageWidth, dndOverlay.texStorageHeight, dndOverlay.texX + viewport.x, dndOverlay.texY + viewport.y, dndOverlay.texWidth, dndOverlay.texHeight);
            lpD3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
        }

        if (note.enable)
            applyNote(viewport.width, viewport.height, viewport.x, viewport.y);

        lpD3DDevice->EndScene();

        if (settings.vrr) {
            waitVRR();
        }

        if (lpD3DDevice->Present(0, 0, 0, 0) == D3DERR_DEVICELOST) {
            lost = true;
        }
    }

    auto refresh() -> void {
        resizeMutexThreaded.lock();

        bool disallowShader = false;
        RenderBuffer* renderBuffer = getBufferToRender();

        if (renderBuffer && renderBuffer->data) {
            renderBuffer->sharedMutex.lock();

            //if (renderBuffer->updated || (inputWidth != renderBuffer->width) || (inputHeight != renderBuffer->height) ) {
            if ( (inputWidth != renderBuffer->width) || (inputHeight != renderBuffer->height) ) {
             //   renderBuffer->updated = false;
                resize(inputWidth = renderBuffer->width, inputHeight = renderBuffer->height);
            }

            texture->GetSurfaceLevel(0, &surface);
            surface->LockRect(&d3dlr, 0, flags.lock);

            std::memcpy( d3dlr.pBits, renderBuffer->data, textureWidth * textureHeight * 4 );

            disallowShader = renderBuffer->disallowShader;

            surface->UnlockRect();
            dxRelease(surface);

            renderBuffer->sharedMutex.unlock();

            accessMutex.lock();
            frames--;
            accessMutex.unlock();
        }

        lpD3DDevice->BeginScene();
        setVertex(inputWidth, inputHeight, textureWidth, textureHeight, viewport.x, viewport.y, viewport.width, viewport.height);
        lpD3DDevice->SetTexture(0, texture);

        if (!disallowShader && caps.shader && effects.size() > 0) {
            applyShader(viewport.width, viewport.height);
        } else {
            lpD3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
        }

        if (dndOverlay.enabled()) {
            dndOverlay.show(viewport);
            setVertex(dndOverlay.texWidth, dndOverlay.texHeight, dndOverlay.texStorageWidth, dndOverlay.texStorageHeight, dndOverlay.texX + viewport.x, dndOverlay.texY + viewport.y, dndOverlay.texWidth, dndOverlay.texHeight);
            lpD3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
        }

        noteMutex.lock();
        if (note.enable)
            applyNote(viewport.width, viewport.height, viewport.x, viewport.y);
        noteMutex.unlock();

        lpD3DDevice->EndScene();

        if (settings.vrr) {
            waitVRR();
        }

        if (lpD3DDevice->Present(0, 0, 0, 0) == D3DERR_DEVICELOST) {
            lost = true;
        }

        resizeMutexThreaded.unlock();
    }

    inline auto applyNote(unsigned outWidth, unsigned outHeight, unsigned outLeft, unsigned outTop ) -> void {
        RECT fontRect;
        int pos = outLeft + outWidth - FONT_WIDTH;
        if (pos < 0)
            pos = 0;
        SetRect(&fontRect, pos, outHeight + outTop - 18, outLeft + outWidth - 5, outHeight + outTop - 0);
        mFont->DrawTextW(NULL, note.message, -1, &fontRect, DT_RIGHT, note.fontColor);
    }

    auto applyShader(unsigned outWidth, unsigned outHeight) -> void {
        D3DXVECTOR4 inputSize;
        inputSize.x = inputWidth;
        inputSize.y = inputHeight;
        inputSize.z = 1.0 / inputHeight;
        inputSize.w = 1.0 / inputWidth;

        D3DXVECTOR4 outputSize;
        outputSize.x = outWidth;
        outputSize.y = outHeight;
        outputSize.z = 1.0 / outHeight;
        outputSize.w = 1.0 / outWidth;

        for (auto effect : effects) {
            UINT passes;
            effect->Begin(&passes, 0);
            effect->SetTexture("texture0", texture);
            effect->SetVector("inputSize", &inputSize);
            effect->SetVector("outputSize", &outputSize);

            for (unsigned pass = 0; pass < passes; pass++) {
                effect->BeginPass(pass);
                lpD3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
                effect->EndPass();
            }
            effect->End();
        }
    }

    auto clear() -> void {
        wait();
        lost = true;
        _clear();
    }

    auto forceResize() -> void {
        if (settings.handle) {
            init();
        }
    }

    auto _clear() -> void {
        if (!lpD3DDevice) return;
        if (lost && !recover()) {
            if (!init())
                return;
        }

        texture->GetSurfaceLevel(0, &surface);

        if (surface) {
            lpD3DDevice->ColorFill(surface, 0, D3DCOLOR_XRGB(0x00, 0x00, 0x00));
            dxRelease(surface);
        }

        //clear primary display and all backbuffers
        for (unsigned i = 0; i < 2; i++) {
            lpD3DDevice->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0x00, 0x00, 0x00), 1.0f, 0);
            lpD3DDevice->Present(0, 0, 0, 0);
        }
    }

    auto lock(unsigned*& data, unsigned& pitch, unsigned _width, unsigned _height, bool reuse = false) -> bool {
        resizeMutex.lock();

        bool result = _lock(data, pitch, _width, _height, reuse);

        resizeMutex.unlock();

        return result;
    }

    inline auto _lock(unsigned*& data, unsigned& pitch, unsigned _width, unsigned _height, bool reuse = false) -> bool {
        RECT windowsize = getDimension( settings.handle );

        if (settings.threaded) {
            if (lost || (viewScreen.windowWidth != windowsize.right) || (viewScreen.windowHeight != windowsize.bottom)) {
                wait();
                if (!reset()) {
                    if (!init())
                        return false;
                }
            }

            if( settings.synchronize && IsIconic( settings.parent ) ) {
                wait();
                return false;
            }

            return RenderThread::lock(data, pitch, _width, _height, reuse);
        }

        if ((viewScreen.windowWidth != windowsize.right) || (viewScreen.windowHeight != windowsize.bottom)) {
            if (!resetOrInit())
                return false;
        }

        if(_width != inputWidth || _height != inputHeight) {
            resize( inputWidth = _width, inputHeight = _height );
            RECT windowSize = getDimension( settings.handle );
            viewScreen.update( viewport, windowSize.right, windowSize.bottom );
            lpD3DDevice->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0x00, 0x00, 0x00), 1.0f, 0);
        }

        texture->GetSurfaceLevel(0, &surface);

        surface->LockRect(&d3dlr, 0, flags.lock);
        pitch = d3dlr.Pitch;
        pitch >>= 2;
        data = (unsigned*) d3dlr.pBits;
        return true;
    }

    auto resize(RenderBuffer* renderBuffer, unsigned w, unsigned h) -> void {

        w = roundUpPowerOfTwo( w + 1 );
        h = roundUpPowerOfTwo( h );

        if(d3dcaps.MaxTextureWidth < w)
            w = d3dcaps.MaxTextureWidth;

        if (d3dcaps.MaxTextureHeight < h)
            h = d3dcaps.MaxTextureHeight;

        renderBuffer->data = new uint32_t[w * h]();
        RECT windowSize = getDimension( settings.handle );
        viewScreen.update( viewport, windowSize.right, windowSize.bottom );
        lpD3DDevice->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0x00, 0x00, 0x00), 1.0f, 0);
    }

    auto calcPitch( unsigned _width ) -> unsigned {

        return roundUpPowerOfTwo( _width + 1 );
    }

    auto resize(unsigned width, unsigned height) -> void {

        width = roundUpPowerOfTwo( width + 1 );
        height = roundUpPowerOfTwo( height );

        if (width == textureWidth && height == textureHeight)
            return;

        textureWidth = width;
        textureHeight = height;

        if(d3dcaps.MaxTextureWidth < textureWidth)
            textureWidth = d3dcaps.MaxTextureWidth;

        if (d3dcaps.MaxTextureHeight < textureHeight)
            textureHeight = d3dcaps.MaxTextureHeight;

        if(texture)
            texture->Release();

        lpD3DDevice->CreateTexture( textureWidth, textureHeight, 1, flags.t_usage, D3DFMT_X8R8G8B8,
                                    static_cast<D3DPOOL> (flags.t_pool), &texture, nullptr);
    }

    auto synchronize(bool state) -> void {
        wait();
        settings.synchronize = state;
        if (!settings.handle) return;

        if (!reset(false) && !init())
            return;
    }

    auto hasSynchronized() -> bool { return settings.synchronize; }

    auto setFilter(Filter filter) -> void {
        if (filter == settings.filter)
            return;
        wait();
        settings.filter = filter;
        updateFilter();
    }

    auto getParentHandle() -> HWND {
        HWND parent = GetParent( settings.handle );
        if(!parent) return settings.handle;
        return parent;
    }

    auto getDimension(HWND handle) -> RECT {
        RECT rect;
        GetClientRect(handle, &rect);

        return rect;
    }

    auto hintExclusiveFullscreen(bool state, float rate = 0.0) -> void {
        /**
         * next time when view port expands to fullscreen, exclusive fullscreen is used instead of a borderless window
         * this is a special microsoft direct 3D feature, means lower input latency because of gpu bypasses stuff
         * like aero
         */
        settings.hintExclusiveFullscreen = state;
        settings.exclusiveFullscreenRate = rate;
    }

    auto hasExclusiveFullscreen() -> bool { return settings.exclusiveFullscreen; }

    auto disableExclusiveFullscreen() -> void {
        if (settings.exclusiveFullscreen) {
            wait();
            init(true);
        }
    }

    auto setThreaded(bool state) -> void {

        if (state != settings.threaded) {
            wait();
            RenderThread::enable(state);

            textureWidth = 0, textureHeight = 0;
			if (settings.exclusiveFullscreen)
				reset(false);
			else
				init();
			
            settings.threaded = state;
        }
    }

    auto hasThreaded() -> bool { return settings.threaded; }

    auto showMessage(std::string message, bool critical = false) -> void {
        if (settings.threaded)
            noteMutex.lock();

        note.enable = !message.empty();
        if (!note.enable) {
            goto out;
        }

        note.message = Win::utf16_t( message );

        if (critical)
            note.fontColor = D3DCOLOR_ARGB(255, 155, 0, 0);
        else
            note.fontColor = D3DCOLOR_ARGB(155, 255, 255, 255);

        out:
        if (settings.threaded)
            noteMutex.unlock();
    }

    auto shaderFormat() -> ShaderType { return ShaderType::HLSL; }

    auto lockResize() -> void {
        resizeMutex.lock();
        resizeMutexThreaded.lock();
    }

    auto unlockResize() -> void {
        resizeMutexThreaded.unlock();
        resizeMutex.unlock();
    }

    auto setRatio(int mode, bool integerScaling) -> void { // mode: 0: off, 1: TV, 2: Native

        if ((int)viewScreen.mode == mode && viewScreen.hasIntegerScaling == integerScaling)
            return;

        wait();
        viewScreen.mode = (ViewScreen::Mode)mode;
        viewScreen.hasIntegerScaling = integerScaling;

        if (settings.handle)
            init();
    }

    auto setIntegerScalingDimension( unsigned _w, unsigned _h, bool _ds) -> void {
        viewScreen.scaling.width = _w;
        viewScreen.scaling.height = _h;
        viewScreen.scaling.doubleSize = _ds;
    }

    auto getViewport() -> Viewport& { return viewport; }

    auto setVRR(bool state, float speed = 0.0) -> void {
        wait();
        settings.vrr = state;

        if (state) {
            minimumCapTime = (1000000.0 / speed) + 0.5;
            lastCapTime = Chronos::getTimestampInMicroseconds();
        }
    }

    auto hasVRR() -> bool { return settings.vrr; }

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

            Sleep(sleepInMilli);

            remaining = lastCapTime - Chronos::getTimestampInMicroseconds();
        }

        // we need exact frame pacing
        while(remaining > 0) {
            //std::this_thread::yield();
            remaining = lastCapTime - Chronos::getTimestampInMicroseconds();
        }
    }
	
	auto resetOrInit() -> bool {
		wait();

		if (!reset()) {
			if (!init())
				return false;
		}
		
		return true;
	}
	
    DVideo(bool XPMode) : XPMode(XPMode) {
        lpD3DDevice = 0;
        lpD3D = 0;
        mFont = 0;
        monPos = 0;

        vertex_buffer = 0;
        surface = 0;
        texture = 0;

        lost = true;		
        settings.filter = Filter::Nearest;
        settings.synchronize = false;
        settings.handle = nullptr;
        settings.hintExclusiveFullscreen = false;
        settings.exclusiveFullscreen = false;
        settings.threaded = false;
		settings.vrr = false;

        #include "../../tools/fonts.c"
        DWORD nFonts;
        AddFontMemResourceEx( &sourceCodePro, sizeof(sourceCodePro), NULL, &nFonts );
    }

    ~DVideo() {
        RenderThread::enable(false);
        term();
    }
};

#undef D3DVERTEX

auto CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) -> BOOL {

    monList[monPos++] = hMonitor;
    return true;  // continue enumerating
}

}
