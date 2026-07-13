
#include "../thread/renderThread.h"
#include "gl3.cpp"

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_SAMPLE_BUFFERS_ARB 0x2041
#define WGL_SAMPLES_ARB 0x2042
#define	WGL_SUPPORT_OPENGL_ARB 0x2010

namespace DRIVER {	

struct WGL : Video, GL3, RenderThread {
	~WGL() {
        GL3::stopShaderBuilding();
        wait();
        RenderThread::enable(false);
        term();
    }

	auto (APIENTRY* wglCreateContextAttribs)(HDC, HGLRC, const int*) -> HGLRC = nullptr;
	auto (APIENTRY* wglSwapInterval)(int) -> BOOL = nullptr;

	HDC display = nullptr;
	HGLRC wglcontext = nullptr;
    HWND handle = nullptr;
    uint8_t options = 0;

    bool hasRendererContext = false;

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
        dndOverlay.enable = state;
    }

    auto sendDragnDropOverlayCoordinates(int x, int y) -> int {
        return dndOverlay.sendDragnDropOverlayCoordinates(x, y);
    }

    auto setShaderCacheCallback( std::function<void (DiskFile& diskFile)> onCallback ) -> void {
        onShaderCacheCallback = onCallback;
    }

    auto setShaderProgressCallback( std::function<void (int pass, bool hasErrors)> onCallback ) -> void {
        onShaderProgressCallback = onCallback;
    }

    auto useShaderCache(bool state) -> void {
        settings.useShaderCache = state;
    }

	auto synchronize(bool state) -> void {
        wait();
        settings.synchronize = state;
        makeCurrent();
        if(wglSwapInterval) {
            wglSwapInterval(settings.synchronize ? 1 : 0);
        }
        clearCurrent();
	}
    
    auto hasSynchronized() -> bool { return settings.synchronize; }
    
    auto shaderSupport() -> bool { return true; }

    auto hardSync(bool state) -> void {
        wait();
        settings.hardSync = state;
    }

    auto setThreaded(bool state) -> void {

        if (state != threadEnabled) {
            wait();
			clearCurrent();
            RenderThread::enable(state);

            RenderThread::reset();
            auto& tex = frame.textures[0];
            tex.width = 0, tex.height = 0;
        }
    }

    auto hasThreaded() -> bool { return threadEnabled; }

	auto setShader(ShaderPreset* preset) -> void {
        wait();
        makeCurrent();
		GL3::shader( preset );
        clearCurrent();
	}

    auto setLinearFilter(bool state) -> void {
        if (state == settings.linearFilter)
            return;
        wait();
        settings.linearFilter = state;
        // makeCurrent();
        GL3::updateFilter();
        // clearCurrent();
	}

    auto lock(unsigned*& data, unsigned& pitch, unsigned _width, unsigned _height, uint8_t options = 0) -> bool {
        if (shaderReady) {
            wait();
            makeCurrent();
            GL3::shaderPostBuild();
            clearCurrent();
            shaderReady = false;
        }

        if (!shaderPasses && (options & OPT_RGB10) ) // YUV input needs a shader to progress it
            return false;

        if (threadEnabled)
            return RenderThread::lock(data, pitch, _width, _height, options);

        this->options = options;
        makeCurrent(true);

        if (GL3::initTexture(_width, _height, GL_RGBA8)) {
            updateRTS = true;
            updateHistory = true;
            viewScreen.update(viewport);
        }

        return GL3::lock(data, pitch);
    }

    auto adjustSize(unsigned& w, unsigned& h) -> void {
        viewScreen.update(viewport);
        updateRTS = true;
        updateHistory = true;
    }

	auto clear() -> void {
        wait();
        makeCurrent();
		GL3::clear();
		SwapBuffers(display);
        clearCurrent();
	}

    auto forceResize() -> void {
        resizeWindow(true);
    }

    auto resizeWindow(bool _force = false) -> void {
        RECT rc;
        GetClientRect(handle, &rc);
        unsigned _windowWidth = rc.right - rc.left;
        unsigned _windowHeight = rc.bottom - rc.top;

        if (!_force) {
            if ( (_windowWidth == viewScreen.windowWidth) && (_windowHeight == viewScreen.windowHeight) )
                return;
        }

        viewScreen.update(viewport, _windowWidth, _windowHeight);
#ifdef DRV_FREETYPE
        screenText.ftUpdateCoords();
#endif
        GL3::updateFrameSize();
    }

    auto unlockAndRedraw() -> void {
        if (threadEnabled)
            RenderThread::unlock();
        else
            redraw();
    }

	auto lockResize() -> void {
        resizeMutex.lock();
        resizeMutexThreaded.lock();
    }

    auto unlockResize() -> void {
        resizeMutexThreaded.unlock();
        resizeMutex.unlock();
    }

	auto redraw() -> void {
		resizeMutex.lock();
        resizeWindow();
        makeCurrent(true);
        //GL3::clear();
        GL3::updateMainTexture( threadEnabled ? getLastBufferToRender() : nullptr );

        subFrame = 1;
        if (settings.bfiFrames)
            totalFrames = (options & OPT_Pause) ? 1 : (settings.bfiFrames + 1);
        render();
        if (options & OPT_TakeScreenshot)
            takeScreenshot();

        if (settings.bfiFrames && (settings.synchronize || settings.vrr) && !(options & (OPT_DisallowShader | OPT_Pause)))
            bfi();

        resizeMutex.unlock();
	}

    auto refresh() -> void {
        makeCurrent();
        //GL3::clear();

        options = 0;
        RenderBuffer* renderBuffer = getBufferToRender();
        if (renderBuffer && renderBuffer->height) {
            renderBuffer->sharedMutex.lock();
            GL3::initTexture(renderBuffer->width, renderBuffer->height, GL_RGBA8);

            updateMainTexture(renderBuffer);
            options = renderBuffer->options;
            renderBuffer->sharedMutex.unlock();

            accessMutex.lock();
            frames--;
            accessMutex.unlock();
        }
		resizeMutexThreaded.lock();
        resizeWindow();
        subFrame = 1;
        if (settings.bfiFrames)
            totalFrames = (options & OPT_Pause) ? 1 : (settings.bfiFrames + 1);
        render();

		resizeMutexThreaded.unlock();
        if (options & OPT_TakeScreenshot)
            takeScreenshot();

        if (settings.bfiFrames && (settings.synchronize || settings.vrr) && !(options & (OPT_DisallowShader | OPT_Pause)))
            bfi();

        clearCurrent();
    }

    auto render() -> void {
        GL3::_redraw(options);

        if (splashScreen.enable)
            updateSplashScreen();

        if (dndOverlay.enabled())
            dndOverlay.show(viewport);
#ifdef DRV_FREETYPE
        screenText.showText(viewport);
#endif
        if (progressVisible && progress.initialized)
            progress.show(viewport, 20, 20);

        if (settings.vrr) {
            glFinish();
            waitVRR();
            SwapBuffers(display);
        } else {
            SwapBuffers(display);
            if (settings.hardSync && settings.synchronize) glFinish();
        }
    }

    auto bfi() -> void {
        for (int i = 0; i < settings.lightFrames; i++) {
            subFrame++;
            render();
        }

        for (int i = 0; i < settings.darkFrames; i++) {
            GL3::clear();
            if (settings.vrr) {
                glFinish();
                waitVRR();
            }
            SwapBuffers(display);
        }
    }
	
	bool init(uintptr_t _handle) {
        handle = (HWND)_handle;
        return init();
    }

	auto init() -> bool {
		GLuint pixel_format;
		PIXELFORMATDESCRIPTOR pfd;
		memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
		pfd.nSize      = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion   = 1;
		pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;

		display = GetDC(handle);
		pixel_format = ChoosePixelFormat(display, &pfd);
		SetPixelFormat(display, pixel_format, &pfd);

		wglcontext = wglCreateContext(display);
		wglMakeCurrent(display, wglcontext);

		wglCreateContextAttribs = (HGLRC (APIENTRY*)(HDC, HGLRC, const int*))glGetProcAddress("wglCreateContextAttribsARB");
		wglSwapInterval = (BOOL (APIENTRY*)(int))glGetProcAddress("wglSwapIntervalEXT");

        glGetIntegerv(GL_MAJOR_VERSION, &version.major);
        glGetIntegerv(GL_MINOR_VERSION, &version.minor);
        version.glsl = glGetString( GL_SHADING_LANGUAGE_VERSION );

        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(wglcontext);
        wglcontext = (HGLRC)getContext(false);
        wglMakeCurrent(display, wglcontext);

		if(wglSwapInterval) {
			wglSwapInterval(settings.synchronize ? 1 : 0);
		}

        RenderThread::reset();
        bool res = GL3::init();
        clearCurrent();
        return res;
	}

    auto getContext(bool shared = true) -> uintptr_t {
        GLUtility::sharedMutex.lock();
        HGLRC context = nullptr;

        if(wglCreateContextAttribs) {
            int attributes[] = {
                WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
                WGL_CONTEXT_MINOR_VERSION_ARB, (version.major == 3 && version.minor == 1) ? 1 : 2,
                0
            };

            context = wglCreateContextAttribs(display, shared ? wglcontext : 0, attributes);
        }

        if(!context)
            context = wglCreateContext(display);

        GLUtility::sharedMutex.unlock();
        return (uintptr_t)context;
    }

    auto deleteContext(uintptr_t context) -> void {
        if(context) {
            GLUtility::sharedMutex.lock();
            //wglMakeCurrent(display, nullptr);
            wglDeleteContext((HGLRC)context);
            GLUtility::sharedMutex.unlock();
        }
    }

    auto makeContextCurrent(uintptr_t context) -> void {
        GLUtility::sharedMutex.lock();
        wglMakeCurrent(display, (HGLRC)context);
        GLUtility::sharedMutex.unlock();
    }
#ifdef DRV_FREETYPE
    auto showScreenText(const std::string text, unsigned duration, bool warn = false) -> void {
        screenText.ftUpdateMessage(text, duration, warn);
    }

    auto setScreenTextDescription(ScreenTextDescription& desc) -> void {
        screenText.ftSetScreenTextDescription(desc);
    }

    auto freeFont() -> void {
        wait();
        screenText.ftUnload();
    }
#endif
    auto setAspectRatio(int mode, bool _integerScaling) -> void { // mode: 0: off, 1: TV, 2: Native
        if ((int)viewScreen.mode == mode && viewScreen.hasIntegerScaling == _integerScaling)
            return;

        wait();
        viewScreen.mode = (ViewScreen::Mode)mode;
        viewScreen.hasIntegerScaling = _integerScaling;

        viewScreen.update(viewport);
#ifdef DRV_FREETYPE
        screenText.ftUpdateCoords();
#endif
        GL3::updateFrameSize();
        updateHistory = true;
    }

    auto getAspectRatio() -> int {
        return (int)viewScreen.mode;
    }

    auto getRotation() -> Rotation { return settings.rotation; }

    auto setRotation(Rotation rotation) -> void {
        if (settings.rotation == rotation)
            return;
        wait();
        settings.rotation = rotation;
        makeCurrent();
        GL3::updateRotation();
        clearCurrent();
        updateRTS = true;
        updateHistory = true;
        frameCount = 0;
    }

    auto setIntegerScalingDimension( unsigned _w, unsigned _h, bool _ds) -> void {
        // update happens during next frame, if input width/height is changing
        viewScreen.scaling.width = _w;
        viewScreen.scaling.height = _h;
        viewScreen.scaling.doubleSize = _ds;
    }

    auto getIntegerScalingDimension(unsigned& _w, unsigned& _h) -> void {
        _w = viewScreen.scaling.width >> 1;
        _h = viewScreen.scaling.height >> 1;
    }

    auto getViewport() -> Viewport& { return viewport; }

    auto setVRR(bool state, float speed = 0.0) -> void {
        wait();
        settings.vrr = state;
        if (speed != 0.0)
            settings.vrrSpeed = speed;
        updateVRR();
    }

    auto setBFI(unsigned frames, unsigned darkFrames) -> void {
        wait();
        subFrame = 1;
        totalFrames = frames + 1;
        settings.bfiFrames = frames;
        settings.darkFrames = darkFrames > frames ? frames : darkFrames;
        settings.lightFrames = frames - settings.darkFrames;
        updateVRR();
    }

    auto waitRenderThread() -> void { if (threadEnabled) wait(); }

    auto hasVRR() -> bool { return settings.vrr; }

    auto getShaderNativeVertexCode(std::string& slang, std::string& out) -> bool {
        return GLUtility::translate(version, slang, out, false);
    }

    auto getShaderNativeFragmentCode(std::string& slang, std::string& out) -> bool {
        return GLUtility::translate(version, slang, out, true);
    }

    auto setProgressAnimation(uint8_t* _data, unsigned _width, unsigned _height) -> void {
        wait();
        makeCurrent();
        progress.setIcon(_data, _width, _height);
        clearCurrent();
    }

	auto term() -> void {
        wait();
		GL3::term();
        deleteContext((uintptr_t)wglcontext);
		//if(wglcontext) wglDeleteContext(wglcontext);
		//wglcontext = nullptr;
	}

    auto makeCurrent(bool usePermanent = false) -> void {
        if (usePermanent) {
            if(!hasRendererContext) {
                hasRendererContext = true;
            } else
                // for non threaded mode, we don't want to bind context each frame. it's costly.
                return;
        }

        wglMakeCurrent(display, wglcontext);
    }

    auto clearCurrent() -> void {
        wglMakeCurrent(display, nullptr);
        hasRendererContext = false;
    }

    auto freeContext() -> void {
        clearCurrent();
    }

    auto canHardSync() -> bool { return true; }

    auto setScreenshotCallback(ScreenshotCallback callback) -> void {
        GL3::setScreenshotCallback(callback);
    }

    auto getAppData() -> AppData* { return &appData; }
};

}
