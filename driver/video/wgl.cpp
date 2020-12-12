#include "opengl/opengl.h"

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_SAMPLE_BUFFERS_ARB 0x2041
#define WGL_SAMPLES_ARB 0x2042
#define	WGL_SUPPORT_OPENGL_ARB 0x2010

namespace DRIVER {	

struct WGL : Video, OpenGL {
	~WGL() { term(); }

	auto (APIENTRY* wglCreateContextAttribs)(HDC, HGLRC, const int*) -> HGLRC = nullptr;
	auto (APIENTRY* wglSwapInterval)(int) -> BOOL = nullptr;

	HDC display = nullptr;
	HGLRC wglcontext = nullptr;
    HWND handle = nullptr;

	auto synchronize(bool state) -> void {
		settings.synchronize = state;
        
        if(wglSwapInterval)
			wglSwapInterval(settings.synchronize);
	}
    
    auto hasSynchronized() -> bool { return settings.synchronize; }
    
    auto shaderFormat() -> ShaderType { return ShaderType::GLSL; }
    
    auto hardSync(bool state) -> void {
        settings.hardSync = state;
    }
	
	auto setShader(std::vector<ShaderPass*> passes) -> void {
		settings.passes = passes;
		OpenGL::shader( passes );
	}   
    
    auto setShaderAttribute( std::string _program, std::string attribute, float value ) -> void {        
        OpenGL::shaderAttribute( _program, attribute, value );
    }
    
    auto setShaderAttribute( std::string _program, std::string attribute, int value ) -> void {        
        OpenGL::shaderAttribute( _program, attribute, value );
    }
    
    auto setShaderAttribute(std::string _program, std::string attribute, float* data, unsigned size) -> void {
        OpenGL::shaderAttribute( _program, attribute, data, size );
    }
    
    auto setShaderAttribute(std::string _program, std::string attribute, uint32_t* data, unsigned _width, unsigned _height) -> void {
        OpenGL::shaderAttribute( _program, attribute, data, _width, _height );
    }
    
	auto setFilter(Filter filter) -> void {
		settings.filter = filter;
		OpenGL::filter = filter == Filter::Linear ? GL_LINEAR : GL_NEAREST;
	}

	auto lock(unsigned*& data, unsigned& pitch, unsigned width, unsigned height) -> bool {
		OpenGL::size(width, height);
		return OpenGL::lock(data, pitch);
	}
	
	auto lock(float*& data, unsigned& pitch, unsigned width, unsigned height) -> bool {
        OpenGL::size(width, height);
        return OpenGL::lock(data, pitch);
    }
    
    auto lock(int32_t*& data, unsigned& pitch, unsigned width, unsigned height) -> bool {
		OpenGL::size(width, height);
		return OpenGL::lock(data, pitch);
	}

	auto clear() -> void {
		OpenGL::clear();
		SwapBuffers(display);
	}

	auto redraw(bool disallowShader = false) -> void {
		RECT rc;
		GetClientRect(handle, &rc);
		outputWidth = rc.right - rc.left, outputHeight = rc.bottom - rc.top;
		OpenGL::refresh(disallowShader);
		SwapBuffers(display);
        if(settings.hardSync && settings.synchronize) glFinish();
        //OpenGL::hardSync();
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
			wglSwapInterval(settings.synchronize);
		}

		return OpenGL::init();
	}
	
	auto showMessage(std::string message, bool critical = false) -> void {
        OpenGL::showMessage(message, critical);
    }

	auto term() -> void {
		OpenGL::term();

		if(wglcontext) wglDeleteContext(wglcontext);
		wglcontext = nullptr;
	}
};

}
