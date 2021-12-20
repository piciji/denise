
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

            if (!state)
                makeCurrent();

            settings.threaded = state;

            if (state)
                clearCurrent();
        }
    }

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

        OpenGL::size(_width, _height);
        return OpenGL::lock(data, pitch);
    }

    auto lock(float*& data, unsigned& pitch, unsigned _width, unsigned _height) -> bool {
        if (settings.threaded)
            return RenderThread::lock(data, pitch, _width, _height);

        OpenGL::size(_width, _height);
        return OpenGL::lock(data, pitch);
    }

    auto lock(int32_t*& data, unsigned& pitch, unsigned _width, unsigned _height) -> bool {
        if (settings.threaded)
            return RenderThread::lock(data, pitch, _width, _height);

        OpenGL::size(_width, _height);
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
    }

	auto clear() -> void {
        wait();
        makeCurrent();
		OpenGL::clear();
		SwapBuffers(display);
        clearCurrent();
	}

    auto resizeWindow() -> void {
        RECT rc;
        GetClientRect(handle, &rc);
        outputWidth = rc.right - rc.left, outputHeight = rc.bottom - rc.top;
    }

	auto redraw(bool disallowShader = false) -> void {
        if (settings.threaded)
            return;

        resizeWindow();

        OpenGL::clear();
        OpenGLSurface::updateTexture();
		OpenGL::refresh(disallowShader);
#ifdef DRV_FREETYPE
        screenText.showText(outputWidth, outputHeight, -0.01, 0.01, OpenGLText::ALIGN_RIGHT | OpenGLText::VALIGN_BOTTOM);
#endif
		SwapBuffers(display);
        if(settings.hardSync && settings.synchronize) glFinish();
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

        OpenGL::refresh(disallowShader);
#ifdef DRV_FREETYPE
        screenText.updateMessage();
        screenText.showText(outputWidth, outputHeight, -0.01, 0.01, OpenGLText::ALIGN_RIGHT | OpenGLText::VALIGN_BOTTOM);
#endif

        SwapBuffers(display);
        if(settings.hardSync && settings.synchronize) glFinish();

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
		return OpenGL::init();
	}
	
	auto showMessage(std::string message, bool critical = false) -> void {
#ifdef DRV_FREETYPE
        screenText.updateMessage(message, critical, !settings.threaded);
#endif
    }

	auto term() -> void {
        wait();
		OpenGL::term();

		if(wglcontext) wglDeleteContext(wglcontext);
		wglcontext = nullptr;
	}

    auto makeCurrent() -> void {
        if (settings.threaded)
            wglMakeCurrent(display, wglcontext);
    }

    auto clearCurrent() -> void {
        if (settings.threaded)
            wglMakeCurrent(display, nullptr);
    }
};

}
