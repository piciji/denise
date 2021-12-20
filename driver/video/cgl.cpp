
#define GL_ALPHA_TEST 0x0bc0
#include <Cocoa/Cocoa.h>
#include "opengl/opengl.h"
#include "thread/renderThread.h"

namespace DRIVER { struct CGL; }

@interface VideoCGL : NSOpenGLView {
@public
    DRIVER::CGL* video;
}
-(id) initWith:(DRIVER::CGL*)video pixelFormat:(NSOpenGLPixelFormat*)pixelFormat;
-(void) reshape;
@end

namespace DRIVER {
    
struct CGL : public Video, OpenGL, RenderThread {
    VideoCGL* view = nullptr;
    NSView* handle;

    bool init() {
        term();

        @autoreleasepool {
            NSOpenGLPixelFormatAttribute attributes[] = {
                NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
                NSOpenGLPFAColorSize, 24,
                NSOpenGLPFAAlphaSize, 8,
                NSOpenGLPFADoubleBuffer,
                0
            };

            auto size = [handle frame].size;
            auto format = [[[NSOpenGLPixelFormat alloc] initWithAttributes:attributes] autorelease];
            auto context = [[[NSOpenGLContext alloc] initWithFormat:format shareContext:nil] autorelease];

            view = [[VideoCGL alloc] initWith:this pixelFormat:format];
            [view setOpenGLContext:context];
            [view setFrame:NSMakeRect(0, 0, size.width, size.height)];
            [view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
            if ([view respondsToSelector:@selector(setWantsBestResolutionOpenGLSurface:)])
                [view setWantsBestResolutionOpenGLSurface:NO];
            [handle addSubview:view];
            [context setView:view];
            [view lockFocus];

            [[view openGLContext] makeCurrentContext];
            
            OpenGL::init();
            
            auto version = (const char*)glGetString(GL_VERSION);
            
            int synchronize = settings.synchronize;
            [[view openGLContext] setValues:&synchronize forParameter:NSOpenGLCPSwapInterval];

            [view unlockFocus];
        }

        clear();
        return true;
    }

    bool init(uintptr_t _handle) {
        handle = (NSView*) _handle;
        return init();
    }

    void term() {
        wait();
        OpenGL::term();

        @autoreleasepool {
            [view removeFromSuperview];
            [view release];
            view = nil;
        }
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
            auto area = [view frame];
            outputWidth = area.size.width, outputHeight = area.size.height;
            RenderThread::unlock(disallowShader);
        }
    }

    auto resize(RenderBuffer* _buffer, unsigned _width, unsigned _height) -> void {

        OpenGL::resize( _buffer, _width, _height );
    }

    void clear() {
        wait();
        @autoreleasepool {
            makeCurrent();
            [view lockFocus];
            OpenGL::clear();
            [[view openGLContext] flushBuffer];
            [view unlockFocus];
            clearCurrent();
        }
    }

    void redraw(bool disallowShader = false) {
        if (settings.threaded)
            return;

        @autoreleasepool {
            if([view lockFocusIfCanDraw]) {
                auto area = [view frame];
                outputWidth = area.size.width, outputHeight = area.size.height;

                OpenGL::clear();
                OpenGLSurface::updateTexture();
                OpenGL::refresh(disallowShader);
#ifdef DRV_FREETYPE
                screenText.showText(outputWidth, outputHeight, -0.01, 0.01, OpenGLText::ALIGN_RIGHT | OpenGLText::VALIGN_BOTTOM);
#endif

                [[view openGLContext] flushBuffer];
                if(settings.hardSync && settings.synchronize) glFinish();
                [view unlockFocus];
            }
        }
    }

    auto refresh() -> void {

        @autoreleasepool{
                makeCurrent();
                if ([view lockFocusIfCanDraw]) {
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

                    [[view
                    openGLContext] flushBuffer];
                    if (settings.hardSync && settings.synchronize) glFinish();
                    [view
                    unlockFocus];
                }

                clearCurrent();
        }
    }

    void synchronize(bool state) {
        wait();
        settings.synchronize = state;

        if(view) {
            @autoreleasepool {
                makeCurrent();
                int synchronize = settings.synchronize;
                [[view openGLContext] setValues:&synchronize forParameter:NSOpenGLCPSwapInterval];
                clearCurrent();
            }
        }
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
    
    auto showMessage(std::string message, bool critical = false) -> void {
#ifdef DRV_FREETYPE
        screenText.updateMessage(message, critical, !settings.threaded);
#endif
    }

    auto makeCurrent() -> void {
        if (settings.threaded)
            [[view openGLContext] makeCurrentContext];
    }

    auto clearCurrent() -> void {
        if (settings.threaded)
            [[view openGLContext] clearCurrentContext];
    }

    CGL() {
        view = nil;
        handle = nil;
    }
    
    ~CGL() {
        RenderThread::enable(false);
        term();
    }
    
};
    
}

@implementation VideoCGL : NSOpenGLView

-(id) initWith:(DRIVER::CGL*)videoPointer pixelFormat:(NSOpenGLPixelFormat*)pixelFormat {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0) pixelFormat:pixelFormat]) {
        video = videoPointer;
    }
    return self;
}

-(void) reshape {
    video->redraw();
}

@end
