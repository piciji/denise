
#include "thread/renderThread.h"
#include "opengl3/gl3.cpp"
#include <gdk/gdkx.h>

#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092

namespace DRIVER {
    
struct GLX : public Video, GL3, RenderThread {

    auto (*glXCreateContextAttribs)(Display*, GLXFBConfig, GLXContext, signed, const signed*) -> GLXContext = nullptr;
    auto (*glXSwapInterval)(int) -> int = nullptr;
    auto (*glXSwapIntervalEXT)(Display*, GLXDrawable, int) -> void = nullptr;

    Display* display = nullptr;
    signed screen = 0;
    Window xwindow = 0;
    Colormap colormap = 0;
    GLXContext glxcontext = nullptr;
    GLXWindow glxwindow = 0;
    GLXFBConfig* fbConfig = nullptr;
    XVisualInfo* vi = nullptr;
    GdkWindow* handle;
    bool useVRR = false;
    bool useResizing = false;
    uint8_t options = 0;
    unsigned shaderResizeTimer = 0;

    bool hasRendererContext = false;

    auto setDragnDropOverlay(uint8_t* _data, unsigned _width, unsigned _height, unsigned line = 0) -> void {
        dndOverlay.setDragnDropOverlay(_data, _width, _height, line);
    }

    auto setDragnDropOverlaySlots(unsigned slots) -> void {
        dndOverlay.setSlots(slots);
    }

    auto enableDragnDropOverlay(bool state) -> void {
        if (dndOverlay.enable != state)
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

    struct {
        bool doubleBuffer = false;
        bool isDirect = false;
    } glx;

    auto init() -> bool {
        term();

        display = GDK_WINDOW_XDISPLAY(handle);
        if (!display)
            return false;

        //window must be realized (appear onscreen)
        while (XPending(display)) {
            XEvent event;
            XNextEvent(display, &event);
        }

		screen = DefaultScreen(display);

        int glxMajor = 0, glxMinor = 0;
        glXQueryVersion(display, &glxMajor, &glxMinor);
        //require GLX 1.3+ API
        if(glxMajor < 1 || (glxMajor == 1 && glxMinor < 3))
            return false;

        XWindowAttributes window_attributes;
        XGetWindowAttributes(display, GDK_WINDOW_XID(handle), &window_attributes);

        //let GLX determine the best Visual to use for GL output; provide a few hints
        //note: some video drivers will override double buffering attribute
        signed attributeList[] = {
            GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
            GLX_RENDER_TYPE, GLX_RGBA_BIT,
            GLX_DOUBLEBUFFER, True,
            GLX_RED_SIZE, 8,
            GLX_GREEN_SIZE, 8,
            GLX_BLUE_SIZE, 8,
            None
        };

        signed fbCount;
        fbConfig = glXChooseFBConfig(display, screen, attributeList, &fbCount);
        if(!fbConfig || fbCount == 0)
            return false;

        vi = glXGetVisualFromFBConfig(display, fbConfig[0]);

        //Window handle has already been realized, most likely with DefaultVisual.
        //GLX requires that the GL output window has the same Visual as the GLX context.
        //it is not possible to change the Visual of an already realized (created) window.
        //therefore a new child window, using the same GLX Visual, must be created and binded to handle.
        colormap = XCreateColormap(display, GDK_WINDOW_XID(handle), vi->visual, AllocNone);
        XSetWindowAttributes attributes;
        attributes.colormap = colormap;
        attributes.border_pixel = 0;
        xwindow = XCreateWindow(display, /* parent = */ GDK_WINDOW_XID(handle),
        /* x = */ 0, /* y = */ 0, window_attributes.width, window_attributes.height,
        /* border_width = */ 0, vi->depth, InputOutput, vi->visual,
        CWColormap | CWBorderPixel, &attributes);
        // XSetWindowBackground(display, xwindow, 0);
        XMapWindow(display, xwindow);
        XFlush(display);

        glxcontext = glXCreateContext(display, vi, /* sharelist = */ 0, /* direct = */ GL_TRUE);
        glXMakeCurrent(display, glxwindow = xwindow, glxcontext);

        glXCreateContextAttribs = (GLXContext (*)(Display*, GLXFBConfig, GLXContext, signed, const signed*))glGetProcAddress("glXCreateContextAttribsARB");
		
        if(!glXSwapIntervalEXT) glXSwapIntervalEXT = (void (*)(Display*, GLXDrawable, int))glGetProcAddress("glXSwapIntervalEXT");

        if(!glXSwapIntervalEXT) {
            glXSwapInterval = (int (*)(int))glGetProcAddress("glXSwapIntervalMESA");
            if(!glXSwapInterval) glXSwapInterval = (int (*)(int))glGetProcAddress("glXSwapIntervalSGI");
        }

        glXMakeCurrent(display, 0, nullptr);
        glXDestroyContext(display, glxcontext);
        glxcontext = (GLXContext)getContext(false);
        glXMakeCurrent(display, glxwindow, glxcontext);

        glGetIntegerv(GL_MAJOR_VERSION, &version.major);
        glGetIntegerv(GL_MINOR_VERSION, &version.minor);
        version.glsl = glGetString( GL_SHADING_LANGUAGE_VERSION );

        if (version.major == 0) {
            const char* _version = (const char*) glGetString(GL_VERSION);

            if (!_version || sscanf(_version, "%u.%u", &version.major, &version.minor) != 2) {
                version.major = 3;
                version.minor = 2;
            }
        }

        if(glXSwapIntervalEXT) glXSwapIntervalEXT(display, glXGetCurrentDrawable(), settings.synchronize ? 1 : 0);
        else if(glXSwapInterval) glXSwapInterval(settings.synchronize ? 1 : 0);

        //read attributes of frame buffer for later use, as requested attributes from above are not always granted
        signed value = 0;
        glXGetConfig(display, vi, GLX_DOUBLEBUFFER, &value);
        glx.doubleBuffer = value;
        glx.isDirect = glXIsDirect(display, glxcontext);
        XFree(vi);
        XSync(display, False);

        RenderThread::reset();
        bool res = GL3::init();
        clearCurrent();
        return res;
    }

    auto getContext(bool shared = true) -> uintptr_t {
        GLUtility::sharedMutex.lock();
        GLXContext context = nullptr;

        if(glXCreateContextAttribs) {
            signed attributes[] = {
                    GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
                    GLX_CONTEXT_MINOR_VERSION_ARB, (version.major == 3 && version.minor == 1) ? 1 : 2,
                    None
            };

            context = glXCreateContextAttribs(display, fbConfig[0], shared ? glxcontext : 0, true, attributes);
        }

        if(!context)
            context = glXCreateContext(display, vi, 0, GL_TRUE);

        GLUtility::sharedMutex.unlock();
        return (uintptr_t)context;
    }

    auto deleteContext(uintptr_t context) -> void {
        if(context) {
            GLUtility::sharedMutex.lock();
            //glXMakeCurrent(display, 0, nullptr);
            glXDestroyContext(display, (GLXContext)context);
            GLUtility::sharedMutex.unlock();
        }
    }

    auto makeContextCurrent(uintptr_t context) -> void {
        GLUtility::sharedMutex.lock();
        glXMakeCurrent(display, glxwindow, (GLXContext)context);
        GLUtility::sharedMutex.unlock();
    }


    auto init(uintptr_t _handle) -> bool {
        handle = (GdkWindow*)_handle;
        return init();
    }
    
    auto synchronize(bool state) -> void {
        wait();
        resizeMutex.lock();
        settings.synchronize = state;
        makeCurrent();
        if(glXSwapIntervalEXT) glXSwapIntervalEXT(display, glXGetCurrentDrawable(), settings.synchronize ? 1 : 0);
        else if(glXSwapInterval) glXSwapInterval(settings.synchronize ? 1 : 0);
        clearCurrent();
        resizeMutex.unlock();
    }
    
    auto hasSynchronized() -> bool { return settings.synchronize; }
    
    auto hardSync(bool state) -> void {
        wait();
        settings.hardSync = state;
    }

    auto setThreaded(bool state) -> void {

        if (state != threadEnabled) {
            RenderThread::enable(state);

            RenderThread::reset();
            auto& tex = frame.textures[0];
            tex.width = 0, tex.height = 0;

            clearCurrent();
        }
    }

    auto hasThreaded() -> bool { return threadEnabled; }

    auto shaderSupport() -> bool { return true; }
    
    auto setShader(ShaderPreset* preset) -> void {
        wait();
        resizeMutex.lock();
        makeCurrent();
        GL3::shader( preset );
        RenderThread::reset();
        clearCurrent();
        resizeMutex.unlock();
    }

    auto setLinearFilter(bool state) -> void {
        if (state == settings.linearFilter)
            return;
        wait();
        settings.linearFilter = state;
        resizeMutex.lock();
     //   makeCurrent();

        GL3::updateFilter();
      //  clearCurrent();
        resizeMutex.unlock();
    }

	auto lock(unsigned*& data, unsigned& pitch, unsigned _width, unsigned _height, uint8_t options = 0) -> bool {
        if (shaderReady) {
            wait();
            makeCurrent();
            GL3::shaderPostBuild();
            clearCurrent();
            shaderReady = false;
        }

        if (threadEnabled)
            return RenderThread::lock(data, pitch, _width, _height, options);

        // resizing could generate 2 "makeCurrent" in a row without "clear" in between,
        bool _useResizing = useResizing;
        if (_useResizing)
            resizeMutex.lock();

        this->options = options;
        makeCurrent(true);
        if (GL3::initTexture(_width, _height, GL_RGBA8)) {
            updateRTS = true;
            updateHistory = true;
            viewScreen.update(viewport);
        }

        if (_useResizing) {
            clearCurrent();
            resizeMutex.unlock();
        }

		return GL3::lock(data, pitch);
	}
	
	auto lock(float*& data, unsigned& pitch, unsigned _width, unsigned _height, uint8_t options = 0) -> bool {
        if (shaderReady) {
            wait();
            makeCurrent();
            GL3::shaderPostBuild();
            clearCurrent();
            shaderReady = false;
        }

        if (!shaderPasses) // YUV input needs a shader to progress it
            return false;

        if (threadEnabled)
            return RenderThread::lock(data, pitch, _width, _height, options);

        bool _useResizing = useResizing;
        if (_useResizing)
            resizeMutex.lock();

        this->options = options;
        makeCurrent(true);
        if (GL3::initTexture(_width, _height, GL_RGBA32F)) {
            updateRTS = true;
            updateHistory = true;
            viewScreen.update(viewport);
        }

        if (_useResizing) {
            clearCurrent();
            resizeMutex.unlock();
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
        if(glx.doubleBuffer) glXSwapBuffers(display, glxwindow);
        clearCurrent();
    }

    auto forceResize() -> void {
        resizeWindow(true);
    }

    auto resizeWindow(bool _force = false) -> void {
        XWindowAttributes parent, child;
        XGetWindowAttributes(display, GDK_WINDOW_XID(handle), &parent);
        XGetWindowAttributes(display, xwindow, &child);

        if(child.width != parent.width || child.height != parent.height) {
            XResizeWindow(display, xwindow, parent.width, parent.height);
        }

        unsigned _windowWidth = parent.width;
        unsigned _windowHeight = parent.height;

        if (!_force) {
            if ( (_windowWidth == viewScreen.windowWidth) && (_windowHeight == viewScreen.windowHeight) )
                return;
        }

        viewScreen.update(viewport, _windowWidth, _windowHeight);
        shaderResizeTimer = 10;
    }

    auto lockResize() -> void {
        resizeMutex.lock();
    //    resizeMutexThreaded.lock();
    }

    auto unlockResize() -> void {
        resizeWindow();

      //  resizeMutexThreaded.unlock();
        resizeMutex.unlock();
    }

    auto hintResizing(bool state) -> void {
        if (useVRR && useResizing && !state)
            lastCapTime = Chronos::getTimestampInMicroseconds();

        useResizing = state;
    }

    auto needResizingPreparations(bool useEmuThread) -> bool {
        return settings.synchronize || settings.vrr;
    }

    auto prepareResizing() -> void {
        wait();
        makeCurrent();
        if (settings.synchronize) {
            if (glXSwapIntervalEXT) glXSwapIntervalEXT(display, glXGetCurrentDrawable(), 0);
            else if (glXSwapInterval) glXSwapInterval(0);
        }
        useVRR = false;
        clearCurrent();
    }

    auto endResizing() -> void {
        wait();
        makeCurrent();
        if (settings.synchronize) {
            if (glXSwapIntervalEXT) glXSwapIntervalEXT(display, glXGetCurrentDrawable(), 1);
            else if (glXSwapInterval) glXSwapInterval(1);
        }
        useVRR = settings.vrr;
        clearCurrent();
    }

    auto unlockAndRedraw() -> void {
        if (threadEnabled) {
            resizeWindow();
            RenderThread::unlock();
        } else
            _redraw(options & OPT_DisallowShader);
    }

    auto redraw(bool disallowShader = false) -> void {
        resizeMutex.lock();
        resizeMutexThreaded.lock();
        redrawCustom(disallowShader);
        resizeMutexThreaded.unlock();
        resizeMutex.unlock();
    }

    auto redrawCustom(bool disallowShader = false) -> void {
        resizeWindow();
        makeCurrent();
        GL3::updateMainTexture( threadEnabled ? getLastBufferToRender() : nullptr );
        GL3::_redraw(disallowShader, options & OPT_Interlace);
#ifdef DRV_FREETYPE
        screenText.showText(viewport.width, viewport.height, -0.01, 0.01, OpenGLText::ALIGN_RIGHT | OpenGLText::VALIGN_BOTTOM);
#endif
        clearCurrent();
    }

    auto _redraw(bool disallowShader = false) -> void {
        resizeWindow();
        resizeMutex.lock();
        makeCurrent(true);

        if (shaderResizeTimer && !--shaderResizeTimer) {
            GL3::updateFrameSize();
        }
        GL3::updateMainTexture( threadEnabled ? getLastBufferToRender() : nullptr );
        GL3::_redraw(disallowShader, options & OPT_Interlace);

        if (dndOverlay.enabled())
            dndOverlay.show(viewport);
#ifdef DRV_FREETYPE
        screenText.showText(viewport.width, viewport.height, -0.01, 0.01, OpenGLText::ALIGN_RIGHT | OpenGLText::VALIGN_BOTTOM);
#endif
        if (progressVisible && progress.initialized)
            progress.show(viewport, 20, 20);

        if (useVRR) {
            glFinish();
            waitVRR();
            glXSwapBuffers(display, glxwindow);
        } else {
            if (glx.doubleBuffer) glXSwapBuffers(display, glxwindow);
            if (settings.hardSync && settings.synchronize) glFinish();
        }

        if (useResizing)
            clearCurrent();

        resizeMutex.unlock();
    }

    auto refresh() -> void {
        resizeMutexThreaded.lock();
        makeCurrent();

        options = 0;
        RenderBuffer* renderBuffer = getBufferToRender();

        if (renderBuffer && renderBuffer->height) {
            renderBuffer->sharedMutex.lock();
            GL3::initTexture(renderBuffer->width, renderBuffer->height, renderBuffer->floatFormat ? GL_RGBA32F : GL_RGBA8);

            updateMainTexture(renderBuffer);
            options = renderBuffer->options;
            renderBuffer->sharedMutex.unlock();

            accessMutex.lock();
            frames--;
            accessMutex.unlock();
        }
        if (shaderResizeTimer && !--shaderResizeTimer) {
            GL3::updateFrameSize();
        }

        GL3::_redraw(options & OPT_DisallowShader, options & OPT_Interlace);

        if (dndOverlay.enabled())
            dndOverlay.show(viewport);
#ifdef DRV_FREETYPE
        screenText.updateMessage();
        screenText.showText(viewport.width, viewport.height, -0.01, 0.01, OpenGLText::ALIGN_RIGHT | OpenGLText::VALIGN_BOTTOM);
#endif
        if (progressVisible && progress.initialized)
            progress.show(viewport, 20, 20);

        if (useVRR) {
            glFinish();
            waitVRR();
            glXSwapBuffers(display, glxwindow);
        } else {
            if (glx.doubleBuffer) glXSwapBuffers(display, glxwindow);
            if (settings.hardSync && settings.synchronize) glFinish();
        }

        clearCurrent();
        resizeMutexThreaded.unlock();
    }

    auto term() -> void {
        wait();
        GL3::term();

        deleteContext((uintptr_t)glxcontext);
		//if(glxcontext) glXDestroyContext(display, glxcontext);
		glxcontext = nullptr;

        if(xwindow) XUnmapWindow(display, xwindow);
		xwindow = 0;

        if (glxwindow) XDestroyWindow(display, glxwindow);
        glxwindow = 0;

        if(colormap) XFreeColormap(display, colormap);
		colormap = 0;

//        if(display) XCloseDisplay(display);
//		display = nullptr;
        
        glXSwapInterval = nullptr;
        glXSwapIntervalEXT = nullptr;
    }

    auto makeCurrent(bool usePermanent = false) -> void {

        if (usePermanent) {
            if(!hasRendererContext) {
                hasRendererContext = true;
            } else
                // for non threaded mode, we don't want to bind context each frame for speed reasons
                return;
        }

        glXMakeCurrent(display, glxwindow, glxcontext);
    }

    auto setAspectRatio(int mode, bool _integerScaling) -> void { // mode: 0: off, 1: TV, 2: Native
        if ((int)viewScreen.mode == mode && viewScreen.hasIntegerScaling == _integerScaling)
            return;

        wait();
        viewScreen.mode = (ViewScreen::Mode)mode;
        viewScreen.hasIntegerScaling = _integerScaling;

        viewScreen.update(viewport);
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
    }

    auto setIntegerScalingDimension( unsigned _w, unsigned _h, bool _ds) -> void {
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
        useVRR = state;

        if (state)
            initVRR(speed);
    }

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

    auto clearCurrent() -> void {
        glXMakeCurrent(display, 0, nullptr);
        hasRendererContext = false;
    }

    auto showMessage(std::string message, bool critical = false) -> void {
#ifdef DRV_FREETYPE
        if (threadEnabled) {
            screenText.updateMessage(message, critical, false);
        } else {
            makeCurrent(true);
            screenText.updateMessage(message, critical, true);
        }
#endif
    }

    auto freeContext() -> void {
        clearCurrent();
    }

    auto canHardSync() -> bool { return true; }

    GLX() {

    }

    ~GLX() {
        GL3::stopShaderBuilding();
        wait();
        RenderThread::enable(false);
        term();
    }
};

}
