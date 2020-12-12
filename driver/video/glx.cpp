
#include "opengl/opengl.h"

#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092

namespace DRIVER {
    
struct GLX : public Video, OpenGL {

    auto (*glXCreateContextAttribs)(Display*, GLXFBConfig, GLXContext, signed, const signed*) -> GLXContext = nullptr;
    auto (*glXSwapInterval)(int) -> int = nullptr;
    auto (*glXSwapIntervalEXT)(Display*, GLXDrawable, int) -> void = nullptr;

    Display* display = nullptr;
    signed screen = 0;
    Window xwindow = 0;
    Colormap colormap = 0;
    GLXContext glxcontext = nullptr;
    GLXWindow glxwindow = 0;
	Window handle = 0;

    struct {
        signed version_major = 0;
        signed version_minor = 0;
        bool doubleBuffer = false;
        bool isDirect = false;
    } glx;

    auto init() -> bool {
        term();
		display = XOpenDisplay(0);
		screen = DefaultScreen(display);

        glXQueryVersion(display, &glx.version_major, &glx.version_minor);
        //require GLX 1.2+ API
        if(glx.version_major < 1 || (glx.version_major == 1 && glx.version_minor < 2)) return false;

        XWindowAttributes window_attributes;
        XGetWindowAttributes(display, handle, &window_attributes);

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
        GLXFBConfig* fbConfig = glXChooseFBConfig(display, screen, attributeList, &fbCount);
        if(fbCount == 0) return false;

        XVisualInfo* vi = glXGetVisualFromFBConfig(display, fbConfig[0]);

        //Window handle has already been realized, most likely with DefaultVisual.
        //GLX requires that the GL output window has the same Visual as the GLX context.
        //it is not possible to change the Visual of an already realized (created) window.
        //therefore a new child window, using the same GLX Visual, must be created and binded to handle.
        colormap = XCreateColormap(display, RootWindow(display, vi->screen), vi->visual, AllocNone);
        XSetWindowAttributes attributes;
        attributes.colormap = colormap;
        attributes.border_pixel = 0;
        xwindow = XCreateWindow(display, /* parent = */ handle,
        /* x = */ 0, /* y = */ 0, window_attributes.width, window_attributes.height,
        /* border_width = */ 0, vi->depth, InputOutput, vi->visual,
        CWColormap | CWBorderPixel, &attributes);
        XSetWindowBackground(display, xwindow, /* color = */ 0);
        XMapWindow(display, xwindow);
        XFlush(display);

        //window must be realized (appear onscreen) before we make the context current
        while(XPending(display)) {
            XEvent event;
            XNextEvent(display, &event);
        }

        glxcontext = glXCreateContext(display, vi, /* sharelist = */ 0, /* direct = */ GL_TRUE);
        glXMakeCurrent(display, glxwindow = xwindow, glxcontext);

        glXCreateContextAttribs = (GLXContext (*)(Display*, GLXFBConfig, GLXContext, signed, const signed*))glGetProcAddress("glXCreateContextAttribsARB");
		
        if(!glXSwapIntervalEXT) glXSwapIntervalEXT = (void (*)(Display*, GLXDrawable, int))glGetProcAddress("glXSwapIntervalEXT");
        
        if(!glXSwapIntervalEXT) {
            glXSwapInterval = (int (*)(int))glGetProcAddress("glXSwapIntervalMESA");
            if(!glXSwapInterval) glXSwapInterval = (int (*)(int))glGetProcAddress("glXSwapIntervalSGI");
        }
               
        if(glXCreateContextAttribs) {
            signed attributes[] = {
                GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
                GLX_CONTEXT_MINOR_VERSION_ARB, 2,
                None
            };
            GLXContext context = glXCreateContextAttribs(display, fbConfig[0], nullptr, true, attributes);
            if(context) {
                glXMakeCurrent(display, 0, nullptr);
                glXDestroyContext(display, glxcontext);
                glXMakeCurrent(display, glxwindow, glxcontext = context);
            }
        }

        if(glXSwapIntervalEXT) glXSwapIntervalEXT(display, xwindow, settings.synchronize ? 1 : 0);
        else if(glXSwapInterval) glXSwapInterval(settings.synchronize ? 1 : 0);

        //read attributes of frame buffer for later use, as requested attributes from above are not always granted
        signed value = 0;
        glXGetConfig(display, vi, GLX_DOUBLEBUFFER, &value);
        glx.doubleBuffer = value;
        glx.isDirect = glXIsDirect(display, glxcontext);

        OpenGL::init();
        return true;
    }

    auto init(uintptr_t _handle) -> bool {
        handle = (Window)_handle;
        return init();
    }
    
    auto synchronize(bool state) -> void {
        settings.synchronize = state;
        
        if(glXSwapIntervalEXT) glXSwapIntervalEXT(display, xwindow, settings.synchronize ? 1 : 0);
        else if(glXSwapInterval) glXSwapInterval(settings.synchronize ? 1 : 0);
    }
    
    auto hasSynchronized() -> bool { return settings.synchronize; }
    
    auto hardSync(bool state) -> void {
        settings.hardSync = state;
    }
    
    auto shaderFormat() -> ShaderType { return ShaderType::GLSL; }
    
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
        if(glx.doubleBuffer) glXSwapBuffers(display, glxwindow);
    }

    auto redraw(bool disallowShader = false) -> void {
        XWindowAttributes parent, child;
        XGetWindowAttributes(display, handle, &parent);
        XGetWindowAttributes(display, xwindow, &child);
		
        if(child.width != parent.width || child.height != parent.height) {
            XResizeWindow(display, xwindow, parent.width, parent.height);
        }

        outputWidth = parent.width, outputHeight = parent.height;
        OpenGL::refresh(disallowShader);
        if(glx.doubleBuffer) glXSwapBuffers(display, glxwindow);
        if(settings.hardSync && settings.synchronize) glFinish();
    }

    auto term() -> void {
        OpenGL::term();

		if(glxcontext) glXDestroyContext(display, glxcontext);
		glxcontext = nullptr;

        if(xwindow) XUnmapWindow(display, xwindow);
		xwindow = 0;

        if(colormap) XFreeColormap(display, colormap);
		colormap = 0;

        if(display) XCloseDisplay(display);
		display = nullptr;
        
        glXSwapInterval = nullptr;
        glXSwapIntervalEXT = nullptr;
    }
    
    auto showMessage(std::string message, bool critical = false) -> void {
        OpenGL::showMessage(message, critical);
    }

    ~GLX() { term(); }
};

}
