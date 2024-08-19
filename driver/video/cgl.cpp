
#define GL_ALPHA_TEST 0x0bc0
#define GL_SILENCE_DEPRECATION
#include "thread/renderThread.h"
#include <Cocoa/Cocoa.h>
#include "opengl3/gl3.cpp"
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
    
struct CGL : public Video, GL3, RenderThread {
    VideoCGL* view = nullptr;
    NSRect area;
    NSView* handle;
    NSOpenGLPixelFormat* format = nil;
    NSOpenGLContext* cglContext = nil;
    bool hasRendererContext = false;
    bool useVRR = false;
    bool useResizing = false;
    bool oldResizeBehaviour = false;
    uint8_t options = 0;
    unsigned shaderResizeTimer = 0;

    auto setDragnDropOverlay(uint8_t* _data, unsigned _width, unsigned _height, unsigned line = 0) -> void {
        dndOverlay.setDragnDropOverlay(_data, _width, _height, line);
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
    
    auto init() -> bool {
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
            format = [[[NSOpenGLPixelFormat alloc] initWithAttributes:attributes] autorelease];
            //auto context = [[[NSOpenGLContext alloc] initWithFormat:format shareContext:nil] autorelease];
            cglContext = (NSOpenGLContext*)getContext(false);

            view = [[VideoCGL alloc] initWith:this pixelFormat:format];
            [view setOpenGLContext:cglContext];
            [view setFrame:NSMakeRect(0, 0, size.width, size.height)];
            [view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
            if ([view respondsToSelector:@selector(setWantsBestResolutionOpenGLSurface:)])
                [view setWantsBestResolutionOpenGLSurface:NO];
            [handle addSubview:view];
            [cglContext setView:view];
            [view lockFocus];

            [[view openGLContext] makeCurrentContext];
            
            res = GL3::init();

            glGetIntegerv(GL_MAJOR_VERSION, &version.major);
            glGetIntegerv(GL_MINOR_VERSION, &version.minor);
            version.glsl = glGetString( GL_SHADING_LANGUAGE_VERSION );
            
            int synchronize = settings.synchronize;
            [[view openGLContext] setValues:&synchronize forParameter:NSOpenGLCPSwapInterval];

            [view unlockFocus];
        }

        area = [view frame];
        resizeWindow();
        RenderThread::reset();
        clear();
        clearCurrent();
        return res;
    }

    auto getContext(bool shared = true) -> uintptr_t {
        NSOpenGLContext* context;
        GLUtility::sharedMutex.lock();

        if (shared)
            context = [[NSOpenGLContext alloc] initWithFormat:format shareContext:cglContext];
        else
            context = [[[NSOpenGLContext alloc] initWithFormat:format shareContext:nil] autorelease];

        GLUtility::sharedMutex.unlock();
        return (uintptr_t)context;
    }

    auto deleteContext(uintptr_t context) -> void {
        if(context) {
            GLUtility::sharedMutex.lock();
            [(id)context release];
            GLUtility::sharedMutex.unlock();
        }
    }

    auto makeContextCurrent(uintptr_t context) -> void {
        GLUtility::sharedMutex.lock();
        [(id)context makeCurrentContext];
        GLUtility::sharedMutex.unlock();
    }

    auto init(uintptr_t _handle) -> bool {
        handle = (NSView*) _handle;
        return init();
    }

    auto hintResizing(bool state) -> void {
        if (settings.vrr)
            lastCapTime = Chronos::getTimestampInMicroseconds();
            
        useResizing = state;
    }

    auto term() -> void {
        wait();
        GL3::term();

        if (view) {
            [view removeFromSuperview]; // releases view too
            view = nil;
        }
        
        if (format) {
            [format release];
            format = nil;
        }
        
        if (cglContext) {
            [cglContext release];
            cglContext = nil;
        }
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

    void clear() {
        wait();
        @autoreleasepool {
            makeCurrent();
            [view lockFocus];
            GL3::clear();
            [[view openGLContext] flushBuffer];
            [view unlockFocus];
            clearCurrent();
        }
    }

    auto resizeWindow(bool _force = false) -> void {
        //auto area = [view frame];

        unsigned _windowWidth = area.size.width;
        unsigned _windowHeight = area.size.height;

        if (!_force) {
            if ( (_windowWidth == viewScreen.windowWidth) && (_windowHeight == viewScreen.windowHeight) )
                return;
        }

        viewScreen.update(viewport, _windowWidth, _windowHeight);
        shaderResizeTimer = 10;
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
        if (threadEnabled) {
            wait();
            changePriorityToRealtime(state);
        }
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
        _redraw(false, threadEnabled ? getLastBufferToRender() : nullptr);
        
        resizeMutexThreaded.unlock();
        resizeMutex.unlock();
    }

    void redraw(bool disallowShader = false) {
        makeCurrent(true);
        _redraw(disallowShader, threadEnabled ? getLastBufferToRender() : nullptr);
        
        if (useResizing)
            clearCurrent();
    }
    
    auto unlockAndRedraw() -> void {
        if (threadEnabled) {
            resizeWindow();
            RenderThread::unlock();
        } else {
            resizeMutex.lock();
            redraw(options & OPT_DisallowShader);
            resizeMutex.unlock();
        }

    }

    void _redraw(bool disallowShader, RenderBuffer* renderBuffer = nullptr) {

        @autoreleasepool {
            if([view lockFocusIfCanDraw]) {
                resizeWindow();
                
                if (shaderResizeTimer && !--shaderResizeTimer) {
                    GL3::updateFrameSize();
                }

                GL3::updateMainTexture( renderBuffer );
                GL3::_redraw(disallowShader, options & OPT_Interlace);

                if (dndOverlay.enabled())
                    dndOverlay.show(viewport);
#ifdef DRV_FREETYPE
                screenText.showText(viewport.width, viewport.height, -0.01, 0.01, OpenGLText::ALIGN_RIGHT | OpenGLText::VALIGN_BOTTOM);
#endif
                if (progressVisible && progress.initialized)
                    progress.show(viewport, 20, 20);

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

    auto shaderSupport() -> bool { return true; }

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

    auto canHardSync() -> bool { return true; }
    
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
        cglContext = nil;
    }
    
    ~CGL() {
        GL3::stopShaderBuilding();
        wait();
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
    video->area = [self frame];
    video->innerUpdate();
}

-(void) reshape {

}

@end
