
#include "thread/renderThread.h"
#include "opengl/opengl.h"
#include <gdk/gdkx.h>

#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092

namespace DRIVER {
    
struct GLX : public Video, OpenGL, RenderThread {

    auto (*glXCreateContextAttribs)(Display*, GLXFBConfig, GLXContext, signed, const signed*) -> GLXContext = nullptr;
    auto (*glXSwapInterval)(int) -> int = nullptr;
    auto (*glXSwapIntervalEXT)(Display*, GLXDrawable, int) -> void = nullptr;

    Display* display = nullptr;
    signed screen = 0;
    Window xwindow = 0;
    Colormap colormap = 0;
    GLXContext glxcontext = nullptr;
    GLXWindow glxwindow = 0;
    GdkWindow* handle;

    struct {
        signed version_major = 0;
        signed version_minor = 0;
        bool doubleBuffer = false;
        bool isDirect = false;
    } glx;

    auto init() -> bool {
        term();

        //window must be realized (appear onscreen)
        display = XOpenDisplay(0);
        if (display) {
            while (XPending(display)) {
                XEvent event;
                XNextEvent(display, &event);
            }
            XCloseDisplay(display);
        }

        display = GDK_WINDOW_XDISPLAY(handle);

		screen = DefaultScreen(display);

        glXQueryVersion(display, &glx.version_major, &glx.version_minor);
        //require GLX 1.3+ API
        if(glx.version_major < 1 || (glx.version_major == 1 && glx.version_minor < 3)) return false;

        XWindowAttributes window_attributes;
        XGetWindowAttributes(display, GDK_WINDOW_XID(handle), &window_attributes);

        //let GLX determine the best Visual to use for GL output; provide a few hints
        //note: some video drivers will override double buffering attribute
        signed attributeList[] = {
            GLX_X_RENDERABLE, True,
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
        colormap = XCreateColormap(display, GDK_WINDOW_XID(handle), vi->visual, AllocNone);
        XSetWindowAttributes attributes;
        attributes.colormap = colormap;
        attributes.border_pixel = 0;
        xwindow = XCreateWindow(display, /* parent = */ GDK_WINDOW_XID(handle),
        /* x = */ 0, /* y = */ 0, window_attributes.width, window_attributes.height,
        /* border_width = */ 0, vi->depth, InputOutput, vi->visual,
        CWColormap | CWBorderPixel, &attributes);
        XSetWindowBackground(display, xwindow, 0);
        XMapWindow(display, xwindow);
        XFlush(display);

        //window must be realized (appear onscreen) before we make the context current
//        while(XPending(display)) {
//            XEvent event;
//            XNextEvent(display, &event);
//        }

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

        if(glXSwapIntervalEXT) glXSwapIntervalEXT(display, glXGetCurrentDrawable(), settings.synchronize ? 1 : 0);
        else if(glXSwapInterval) glXSwapInterval(settings.synchronize ? 1 : 0);

        //read attributes of frame buffer for later use, as requested attributes from above are not always granted
        signed value = 0;
        glXGetConfig(display, vi, GLX_DOUBLEBUFFER, &value);
        glx.doubleBuffer = value;
        glx.isDirect = glXIsDirect(display, glxcontext);
        XFree(vi);
        XSync(display, False);
        OpenGL::init();
        clearCurrent();
        return true;
    }

    auto init(uintptr_t _handle) -> bool {
        handle = (GdkWindow*)_handle;
        return init();
    }
    
    auto synchronize(bool state) -> void {
        settings.synchronize = state;

        wait();
        makeCurrent();
        if(glXSwapIntervalEXT) glXSwapIntervalEXT(display, glXGetCurrentDrawable(), settings.synchronize ? 1 : 0);
        else if(glXSwapInterval) glXSwapInterval(settings.synchronize ? 1 : 0);
        clearCurrent();
    }
    
    auto hasSynchronized() -> bool { return settings.synchronize; }
    
    auto hardSync(bool state) -> void {
        wait();
        settings.hardSync = state;
    }

    auto setThreaded(bool state) -> void {

        if (state != settings.threaded) {
            wait();
            RenderThread::enable(state);
            settings.threaded = state;
            RenderThread::reset();
            width = 0, height = 0;
        }
    }
    
    auto shaderFormat() -> ShaderType { return ShaderType::GLSL; }
    
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
        makeCurrent();
		OpenGL::size(_width, _height);
        clearCurrent();
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
        if(glx.doubleBuffer) glXSwapBuffers(display, glxwindow);
        clearCurrent();
    }

    auto resizeWindow() -> void {
        XWindowAttributes parent, child;
        XGetWindowAttributes(display, GDK_WINDOW_XID(handle), &parent);
        XGetWindowAttributes(display, xwindow, &child);

        if(child.width != parent.width || child.height != parent.height) {
            XResizeWindow(display, xwindow, parent.width, parent.height);
        }

        outputWidth = parent.width, outputHeight = parent.height;
    }

    auto redraw(bool disallowShader = false) -> void {
        if (settings.threaded)
            return;
        makeCurrent();
        resizeWindow();

        OpenGL::clear();
        OpenGLSurface::updateTexture();
        OpenGL::refresh(disallowShader);
#ifdef DRV_FREETYPE
        screenText.showText(outputWidth, outputHeight, -0.01, 0.01, OpenGLText::ALIGN_RIGHT | OpenGLText::VALIGN_BOTTOM);
#endif

        if(glx.doubleBuffer) glXSwapBuffers(display, glxwindow);
        if(settings.hardSync && settings.synchronize) glFinish();
        clearCurrent();
    }

    auto refresh() -> void {

        RenderBuffer* renderBuffer = getBufferToRender();

        if (!renderBuffer)
            return;

        makeCurrent();
        OpenGL::clear();
        renderBuffer->sharedMutex.lock();
        width = renderBuffer->width;
        height = renderBuffer->height;

        if (renderBuffer->updated) {
            renderBuffer->updated = false;
            createTexture(renderBuffer);
        }

        OpenGL::updateTexture(renderBuffer);

        bool disallowShader = renderBuffer->disallowShader;
        renderBuffer->sharedMutex.unlock();

        OpenGL::refresh(disallowShader);
#ifdef DRV_FREETYPE
        screenText.updateMessage();
        screenText.showText(outputWidth, outputHeight, -0.01, 0.01, OpenGLText::ALIGN_RIGHT | OpenGLText::VALIGN_BOTTOM);
#endif

        accessMutex.lock();
        frames--;
        accessMutex.unlock();

        if(glx.doubleBuffer) glXSwapBuffers(display, glxwindow);
        if(settings.hardSync && settings.synchronize) glFinish();

        clearCurrent();
    }

    auto term() -> void {
        wait();
        OpenGL::term();

		if(glxcontext) glXDestroyContext(display, glxcontext);
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

    auto makeCurrent() -> void {
        glXMakeCurrent(display, glxwindow, glxcontext);
    }

    auto clearCurrent() -> void {
        glXMakeCurrent(display, 0, nullptr);
    }

    auto showMessage(std::string message, bool critical = false) -> void {
#ifdef DRV_FREETYPE
        makeCurrent();
        screenText.updateMessage(message, critical, !settings.threaded);
        clearCurrent();
#endif
    }

    GLX() {

    }

    ~GLX() { term(); }
};

}
