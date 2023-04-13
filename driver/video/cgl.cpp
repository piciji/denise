
#define GL_ALPHA_TEST 0x0bc0
#include "thread/renderThread.h"
#include <Cocoa/Cocoa.h>
#include "opengl/opengl.h"
#define NSAppKitVersionNumber10_14 1671

namespace DRIVER { struct CGL; }

@interface VideoCGL : NSOpenGLView {
@public
    DRIVER::CGL* video;
}
-(id) initWith:(DRIVER::CGL*)video pixelFormat:(NSOpenGLPixelFormat*)pixelFormat;
-(void) reshape;
-(void) update;
@end

namespace DRIVER {
    
struct CGL : public Video, OpenGL, RenderThread {
    VideoCGL* view = nullptr;
    NSView* handle;
    bool hasRendererContext = false;
    bool useVRR = false;
    bool useResizing = false;
    bool oldResizeBehaviour = false;

    bool init() {
        term();
        bool res;
        // before Mojave
        oldResizeBehaviour = NSAppKitVersionNumber < NSAppKitVersionNumber10_14;

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

    auto hintResizing(bool state) -> void {
        if (settings.vrr)
            lastCapTime = Chronos::getTimestampInMicroseconds();
            
        useResizing = state;
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

    auto lock(unsigned*& data, unsigned& pitch, unsigned _width, unsigned _height, bool reuse = false) -> bool {
        if (settings.threaded)
            return RenderThread::lock(data, pitch, _width, _height, reuse);
        
        bool _useResizing = useResizing;
        if (_useResizing)
            resizeMutex.lock();

        makeCurrent(true);
        if (OpenGL::size(_width, _height)) {
            integerScalingWidth = _width;
            integerScalingHeight = _height;
            calcViewport();
        }
        if (_useResizing) {
            clearCurrent();
            resizeMutex.unlock();
        }

        return OpenGL::lock(data, pitch);
    }

    auto lock(float*& data, unsigned& pitch, unsigned _width, unsigned _height, bool reuse = false) -> bool {
        if (settings.threaded)
            return RenderThread::lock(data, pitch, _width, _height, reuse);

        bool _useResizing = useResizing;
        if (_useResizing)
            resizeMutex.lock();

        makeCurrent(true);
        if (OpenGL::size(_width, _height)) {
            integerScalingWidth = _width;
            integerScalingHeight = _height;
            calcViewport();
        }
        if (_useResizing) {
            clearCurrent();
            resizeMutex.unlock();
        }

        return OpenGL::lock(data, pitch);
    }

    auto lock(int32_t*& data, unsigned& pitch, unsigned _width, unsigned _height, bool reuse = false) -> bool {
        if (settings.threaded)
            return RenderThread::lock(data, pitch, _width, _height, reuse);

        bool _useResizing = useResizing;
        if (_useResizing)
            resizeMutex.lock();

        makeCurrent(true);
        if (OpenGL::size(_width, _height)) {
            integerScalingWidth = _width;
            integerScalingHeight = _height;
            calcViewport();
        }
        if (_useResizing) {
            clearCurrent();
            resizeMutex.unlock();
        }

        return OpenGL::lock(data, pitch);
    }

    auto resize(RenderBuffer* _buffer, unsigned _width, unsigned _height) -> void {
        OpenGL::resize( _buffer, _width, _height );
        integerScalingWidth = _width;
        integerScalingHeight = _height;
        calcViewport();
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

    auto resizeWindow(bool _force = false) -> void {
        auto area = [view frame];

        unsigned _windowWidth = area.size.width;
        unsigned _windowHeight = area.size.height;

        if (!_force) {
            if ( (_windowWidth == windowWidth) && (_windowHeight == windowHeight) )
                return;
        }

        windowWidth = _windowWidth;
        windowHeight = _windowHeight;

        calcViewport();
    }
    
    auto forceResize() -> void {
        resizeWindow(true);
    }
    
    auto needResizingPreparations(bool useEmuThread) -> bool {
        return (useEmuThread) && (settings.synchronize || settings.vrr);
    }
    
    auto prepareResizing() -> void {
        if (!view)
            return;
        wait();
        @autoreleasepool {
            makeCurrent();
            if (settings.synchronize) {
                int synchronize = 0;
                [[view openGLContext] setValues:&synchronize forParameter:NSOpenGLCPSwapInterval];
            }
            useVRR = false;
            clearCurrent();
        }
    }
    
    auto changeThreadPriorityToRealtime(bool state) -> void {
        changePriorityToRealtime(state);
    }
    
    auto endResizing() -> void {
        if (!view)
            return;
        wait();
        @autoreleasepool {
            makeCurrent();
            if (settings.synchronize) {
                int synchronize = 1;
                [[view openGLContext] setValues:&synchronize forParameter:NSOpenGLCPSwapInterval];
            }
            useVRR = settings.vrr;
            clearCurrent();
        }
    }
    
    auto lockResize() -> void {
        if (!oldResizeBehaviour)
            return;
        resizeMutex.lock();
        resizeMutexThreaded.lock();
    }
    
    auto unlockResize() -> void {
        if (!oldResizeBehaviour)
            return;

//        if (NSAppKitVersionNumber < NSAppKitVersionNumber10_14) // before Mojave
        _redraw(false, settings.threaded ? getLastBufferToRender() : nullptr);
        
        resizeMutexThreaded.unlock();
        resizeMutex.unlock();
    }

    void redraw(bool disallowShader = false) {
        makeCurrent(true);
        _redraw(disallowShader, settings.threaded ? getLastBufferToRender() : nullptr);
        
        if (useResizing)
            clearCurrent();
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

                if (useResizing)
                    [[view openGLContext] flushBuffer];
                else if (useVRR) {
                    glFinish();
                    waitVRR();
                    [[view openGLContext] flushBuffer];
                } else {
                    [[view openGLContext] flushBuffer];
                    if (settings.hardSync && settings.synchronize) glFinish();
                }
              
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

                if (renderBuffer && renderBuffer->height) {
                    renderBuffer->sharedMutex.lock();
                    if ( (width != renderBuffer->width) || (height != renderBuffer->height) ) {
                        width = renderBuffer->width;
                        height = renderBuffer->height;
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
                if (useResizing)
                    [[view openGLContext] flushBuffer];
                else if (useVRR) {
                    glFinish();
                    waitVRR();
                    [[view openGLContext] flushBuffer];
                } else {
                    [[view openGLContext] flushBuffer];
                    if (settings.hardSync && settings.synchronize) glFinish();
                }
                    
                [view unlockFocus];
            }

            clearCurrent();
           
        }
        resizeMutexThreaded.unlock();
    }
    
    void synchronize(bool state) {
        wait();
        resizeMutex.lock();
        settings.synchronize = state;

        if(view) {
            @autoreleasepool {
                makeCurrent();
                int synchronize = settings.synchronize;
                [[view openGLContext] setValues:&synchronize forParameter:NSOpenGLCPSwapInterval];
                clearCurrent();
            }
        }
        resizeMutex.unlock();
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
        resizeMutex.lock();
        makeCurrent();
        settings.passes = passes;
        OpenGL::shader( passes );
        RenderThread::reset();
        clearCurrent();
        resizeMutex.unlock();
    }
    
    auto setShaderAttribute( std::string _program, std::string attribute, float value ) -> void {
        wait();
        resizeMutex.lock();
        makeCurrent();
        OpenGL::shaderAttribute( _program, attribute, value );
        clearCurrent();
        resizeMutex.unlock();
    }
    
    auto setShaderAttribute( std::string _program, std::string attribute, int value ) -> void {
        wait();
        resizeMutex.lock();
        makeCurrent();
        OpenGL::shaderAttribute( _program, attribute, value );
        clearCurrent();
        resizeMutex.unlock();
    }
    
    auto setShaderAttribute(std::string _program, std::string attribute, float* data, unsigned size) -> void {
        wait();
        resizeMutex.lock();
        makeCurrent();
        OpenGL::shaderAttribute( _program, attribute, data, size );
        clearCurrent();
        resizeMutex.unlock();
    }
    
    auto setShaderAttribute(std::string _program, std::string attribute, uint32_t* data, unsigned _width, unsigned _height) -> void {
        wait();
        resizeMutex.lock();
        makeCurrent();
        OpenGL::shaderAttribute( _program, attribute, data, _width, _height );
        clearCurrent();
        resizeMutex.unlock();
    }
    
    auto setFilter(Filter filter) -> void {
        if (settings.filter == filter)
            return;
        settings.filter = filter;
        wait();
        resizeMutex.lock();
     //   makeCurrent();
        OpenGL::filter = filter == Filter::Linear ? GL_LINEAR : GL_NEAREST;
      //  clearCurrent();
        resizeMutex.unlock();
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

    auto setRatio(int mode, bool integerScaling) -> void { // mode: 0: off, 1: TV, 2: Native
        if (settings.aspectMode == mode && settings.integerScaling == integerScaling)
            return;

        wait();
        settings.aspectMode = mode;
        settings.integerScaling = integerScaling;

        calcViewport();
    }

    auto setVRR(bool state, float speed = 0.0) -> void {
        wait();
        settings.vrr = state;
        useVRR = state;

        if (state)
            initVRR(speed);
    }

    auto hasVRR() -> bool { return settings.vrr; }

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
    
    auto innerUpdate() -> void {
        if (oldResizeBehaviour) {
            [[view openGLContext] update];
            return;
        }
        
        resizeMutex.lock();
        resizeMutexThreaded.lock();
        makeCurrent();
        [[view openGLContext] update];
        clearCurrent();
        resizeMutexThreaded.unlock();
        resizeMutex.unlock();
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

-(void) update {
    video->innerUpdate();
}

-(void) reshape {
    return;

    video->makeCurrent();
    video->_redraw(!video->useShader, video->settings.threaded ? video->getLastBufferToRender() : nullptr);
    video->clearCurrent();
}

@end
