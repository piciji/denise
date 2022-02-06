
#define GL_ALPHA_TEST 0x0bc0
#include "thread/renderThread.h"
#include <Cocoa/Cocoa.h>
#include "opengl/opengl.h"

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
    bool hasRendererContext = false;
    bool useReshaping = true;

    bool init() {
        term();
        bool res;

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
            
            res = OpenGL::init();
            
            auto version = (const char*)glGetString(GL_VERSION);
            
            int synchronize = settings.synchronize;
            [[view openGLContext] setValues:&synchronize forParameter:NSOpenGLCPSwapInterval];

            [view unlockFocus];
        }

        resizeWindow();
        RenderThread::reset();
        clear();
        clearCurrent();
        return res;
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

        makeCurrent(true);
        OpenGL::size(_width, _height);
        return OpenGL::lock(data, pitch);
    }

    auto lock(float*& data, unsigned& pitch, unsigned _width, unsigned _height) -> bool {
        if (settings.threaded)
            return RenderThread::lock(data, pitch, _width, _height);

        makeCurrent(true);
        OpenGL::size(_width, _height);
        return OpenGL::lock(data, pitch);
    }

    auto lock(int32_t*& data, unsigned& pitch, unsigned _width, unsigned _height) -> bool {
        if (settings.threaded)
            return RenderThread::lock(data, pitch, _width, _height);

        makeCurrent(true);
        OpenGL::size(_width, _height);
        return OpenGL::lock(data, pitch);
    }

    auto unlock(bool disallowShader = false) -> void {
        if (settings.threaded) {
            //resizeWindow();
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

    auto resizeWindow() -> void {
        auto area = [view frame];
        outputWidth = area.size.width, outputHeight = area.size.height;
    }
    
    auto forceResize() -> void {
        resizeWindow();
    }
    
    auto redrawCustom(bool disallowShader = false) -> void {
        redraw(disallowShader);
    }
    
    auto lockResize() -> void {
        resizeMutex.lock();
        resizeMutexThreaded.lock();
    }
    
    auto unlockResize() -> void {
        _redraw(true, settings.threaded ? getLastBufferToRender() : nullptr);

        resizeMutexThreaded.unlock();
        resizeMutex.unlock();
    }

    void redraw(bool disallowShader = false) {
        makeCurrent(true);
        _redraw(disallowShader);
    }
    
    auto unlockAndRedraw(bool disallowShader = false, bool freeContext = false) -> void {
        if (settings.threaded) {
            resizeWindow();
            RenderThread::unlock(disallowShader);
        } else {
            resizeMutex.lock();
            redraw(disallowShader);
            resizeMutex.unlock();
        }
            
        if (freeContext)
            clearCurrent();
    }

    void _redraw(bool disallowShader, RenderBuffer* renderBuffer = nullptr) {

        @autoreleasepool {
            if([view lockFocusIfCanDraw]) {
                resizeWindow();
    
                OpenGL::clear();
                OpenGLSurface::updateTexture(renderBuffer);
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
        resizeMutexThreaded.lock();
        @autoreleasepool {
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

                [[view openGLContext] flushBuffer];
                if (settings.hardSync && settings.synchronize) glFinish();
                    
                [view unlockFocus];
            }

            clearCurrent();
           
        }
        resizeMutexThreaded.unlock();
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

    auto hasReshaping() -> bool {
        return true;
    }
    
    auto setReshaping(bool state) -> void {
        useReshaping = state;
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

    auto makeCurrent(bool usePermanent = false) -> void {
        if (usePermanent) {
            if(!hasRendererContext) {
                hasRendererContext = true;
            } else
                // for non threaded mode, we don't want to bind context each frame for speed reasons
                return;
        }

        [[view openGLContext] makeCurrentContext];
    }

    auto clearCurrent() -> void {
        [NSOpenGLContext clearCurrentContext];
        hasRendererContext = false;
    }
    
    auto freeContext() -> void {
        clearCurrent();
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
    if (!video->useReshaping)
        return;

    video->makeCurrent();
    video->_redraw(true, video->settings.threaded ? video->getLastBufferToRender() : nullptr);
    video->clearCurrent();
}

@end
