
#include "thread/renderThread.h"
#include "opengl/opengl.h"

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_SAMPLE_BUFFERS_ARB 0x2041
#define WGL_SAMPLES_ARB 0x2042
#define	WGL_SUPPORT_OPENGL_ARB 0x2010

namespace DRIVER {	

struct WGL : Video, OpenGL, RenderThread {
	~WGL() {
        RenderThread::enable(false);
        term();
    }

	auto (APIENTRY* wglCreateContextAttribs)(HDC, HGLRC, const int*) -> HGLRC = nullptr;
	auto (APIENTRY* wglSwapInterval)(int) -> BOOL = nullptr;

	HDC display = nullptr;
	HGLRC wglcontext = nullptr;
    HWND handle = nullptr;

    bool hasRendererContext = false;

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
    
    auto shaderFormat() -> ShaderType { return ShaderType::GLSL; }
    
    auto hardSync(bool state) -> void {
        wait();
        settings.hardSync = state;
    }

    auto setThreaded(bool state) -> void {

        if (state != settings.threaded) {
            wait();
            RenderThread::enable(state);

            RenderThread::reset();
            width = 0, height = 0;

            settings.threaded = state;

            clearCurrent();
        }
    }

    auto hasThreaded() -> bool { return settings.threaded; }

	auto setShader(std::vector<ShaderPass*> passes) -> void {
        wait();
        makeCurrent();
		settings.passes = passes;
		OpenGL::shader( passes );
        RenderThread::reset();
        clearCurrent();
	}   
    
    auto setShaderAttribute( std::string _program, std::string attribute, float value ) -> void {
        wait();
        makeCurrent();
        OpenGL::shaderAttribute( _program, attribute, value );
        clearCurrent();
    }
    
    auto setShaderAttribute( std::string _program, std::string attribute, int value ) -> void {
        wait();
        makeCurrent();
        OpenGL::shaderAttribute( _program, attribute, value );
        clearCurrent();
    }
    
    auto setShaderAttribute(std::string _program, std::string attribute, float* data, unsigned size) -> void {
        wait();
        makeCurrent();
        OpenGL::shaderAttribute( _program, attribute, data, size );
        clearCurrent();
    }
    
    auto setShaderAttribute(std::string _program, std::string attribute, uint32_t* data, unsigned _width, unsigned _height) -> void {
        wait();
        makeCurrent();
        OpenGL::shaderAttribute( _program, attribute, data, _width, _height );
        clearCurrent();
    }
    
	auto setFilter(Filter filter) -> void {
        wait();
        makeCurrent();
		settings.filter = filter;
		OpenGL::filter = filter == Filter::Linear ? GL_LINEAR : GL_NEAREST;
        clearCurrent();
	}

    auto lock(unsigned*& data, unsigned& pitch, unsigned _width, unsigned _height) -> bool {
        if (settings.threaded)
            return RenderThread::lock(data, pitch, _width, _height);

        makeCurrent(true);
        if (OpenGL::size(_width, _height)) {
            integerScalingHeight = _height;
            calcViewport();
        }
        return OpenGL::lock(data, pitch);
    }

    auto lock(float*& data, unsigned& pitch, unsigned _width, unsigned _height) -> bool {
        if (settings.threaded)
            return RenderThread::lock(data, pitch, _width, _height);

        makeCurrent(true);
        if (OpenGL::size(_width, _height)) {
            integerScalingHeight = _height;
            calcViewport();
        }
        return OpenGL::lock(data, pitch);
    }

    auto lock(int32_t*& data, unsigned& pitch, unsigned _width, unsigned _height) -> bool {
        if (settings.threaded)
            return RenderThread::lock(data, pitch, _width, _height);

        makeCurrent(true);
        if (OpenGL::size(_width, _height)) {
            integerScalingHeight = _height;
            calcViewport();
        }
        return OpenGL::lock(data, pitch);
    }

    auto unlock(bool disallowShader = false) -> void {
        if (settings.threaded) {
            resizeWindow();
            RenderThread::unlock(disallowShader);
        }
    }

    auto resize(RenderBuffer* _buffer, unsigned _width, unsigned _height) -> void {
        OpenGL::resize( _buffer, _width, _height );

        integerScalingHeight = _height;
        calcViewport();
    }

	auto clear() -> void {
        wait();
        makeCurrent();
		OpenGL::clear();
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
            if ( (_windowWidth == windowWidth) && (_windowHeight == windowHeight) )
                return;
        }

        windowWidth = _windowWidth;
        windowHeight = _windowHeight;

        calcViewport();
    }

    auto unlockAndRedraw(bool disallowShader = false, bool freeContext = false) -> void {
        if (settings.threaded) {
            resizeWindow();
            RenderThread::unlock(disallowShader);
        } else
            redraw(disallowShader);

        if (freeContext)
            clearCurrent();
    }

	auto lockResize() -> void {
        resizeMutex.lock();
        resizeMutexThreaded.lock();
    }

    auto unlockResize() -> void {
        resizeMutexThreaded.unlock();
        resizeMutex.unlock();
    }
	
	auto redraw(bool disallowShader = false) -> void {
		resizeMutex.lock();
        resizeWindow();
        makeCurrent(true);
        OpenGL::clear();
        OpenGLSurface::updateTexture( settings.threaded ? getLastBufferToRender() : nullptr );
		OpenGL::refresh(disallowShader);
#ifdef DRV_FREETYPE
        screenText.showText(outputWidth, outputHeight, -0.01, 0.01, OpenGLText::ALIGN_RIGHT | OpenGLText::VALIGN_BOTTOM);
#endif
        if (settings.vrr) {
            if (settings.hardSync) glFinish();
            waitVRR();
            SwapBuffers(display);
        } else {
            SwapBuffers(display);
            if (settings.hardSync && settings.synchronize) glFinish();
        }
		resizeMutex.unlock();
	}

    auto refresh() -> void {
		
        makeCurrent();
        OpenGL::clear();

        bool disallowShader = false;
        RenderBuffer* renderBuffer = getBufferToRender();
        if (renderBuffer) {
            renderBuffer->sharedMutex.lock();
            width = renderBuffer->width;
            height = renderBuffer->height;

            if (renderBuffer->updated) {
                renderBuffer->updated = false;
                createTexture(renderBuffer);
            }

            OpenGL::updateTexture(renderBuffer);
            disallowShader = renderBuffer->disallowShader;
            renderBuffer->sharedMutex.unlock();

            accessMutex.lock();
            frames--;
            accessMutex.unlock();
        }
		resizeMutexThreaded.lock();
        OpenGL::refresh(disallowShader);
#ifdef DRV_FREETYPE
        screenText.updateMessage();
        screenText.showText(outputWidth, outputHeight, -0.01, 0.01, OpenGLText::ALIGN_RIGHT | OpenGLText::VALIGN_BOTTOM);
#endif
        if (settings.vrr) {
            if (settings.hardSync) glFinish();
            waitVRR();
            SwapBuffers(display);
        } else {
            SwapBuffers(display);
            if (settings.hardSync && settings.synchronize) glFinish();
        }

		resizeMutexThreaded.unlock();
        clearCurrent();
		
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

		if(wglCreateContextAttribs) {
			int attributes[] = {
				WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
				WGL_CONTEXT_MINOR_VERSION_ARB, 2,
				0
			};
			HGLRC context = wglCreateContextAttribs(display, 0, attributes);
			if(context) {
				wglMakeCurrent(NULL, NULL);
				wglDeleteContext(wglcontext);
				wglMakeCurrent(display, wglcontext = context);
			}
		}

		if(wglSwapInterval) {
			wglSwapInterval(settings.synchronize ? 1 : 0);
		}

        RenderThread::reset();
        bool res = OpenGL::init();
        clearCurrent();
        return res;
	}
	
	auto showMessage(std::string message, bool critical = false) -> void {
#ifdef DRV_FREETYPE
        if (settings.threaded) {
            screenText.updateMessage(message, critical, false);
        } else {
            makeCurrent(true);
            screenText.updateMessage(message, critical, true);
        }
#endif
    }

    auto setAspectCorrection(float width, float height, bool integerScaling) -> void {
        wait();
        settings.aspectWidth = width;
        settings.aspectHeight = height;
        settings.integerScaling = integerScaling;

        calcViewport();
    }

    auto setVRR(bool state, float speed = 0.0) -> void {
        wait();
        settings.vrr = state;

        if (state)
            initVRR(speed);
    }

    auto hasVRR() -> bool { return settings.vrr; }

	auto term() -> void {
        wait();
		OpenGL::term();

		if(wglcontext) wglDeleteContext(wglcontext);
		wglcontext = nullptr;
	}

    auto makeCurrent(bool usePermanent = false) -> void {
        if (usePermanent) {
            if(!hasRendererContext) {
                hasRendererContext = true;
            } else
                // for non threaded mode, we don't want to bind context each frame for speed reasons
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
};

}
