
#include "../../tools/tools.h"
#include "interface.h"
#include "utility.h"
#include "../viewport.h"
#include "shaders.h"
#include "../../tools/tools.h"
#include "../thread/renderThread.h"
#include "../../tools/ShaderCache.h"
#include "../../tools/glslang.h"
#include "../../tools/spirvReflection.h"
#include "../../../deps/SPIRV-Cross/spirv_msl.hpp"

#ifdef DRV_FREETYPE
#include "../freetype.h"
#endif

#define NSAppKitVersionNumber11 2022

namespace DRIVER {

#ifdef DRV_FREETYPE
    struct METAL : public Video, RenderThread, Freetype {
#else
    struct METAL : public Video, RenderThread {
#endif

    NSView* handle;
    MetalView* view;
    ViewScreen viewScreen;
    Viewport viewport;
    CGSize area;
    
    bool resizeWithEmuThread;
    uint8_t options;
    uint8_t* frameData;
    unsigned shaderResizeTimer = 0;
    
    int64_t lastCapTime;
    int64_t minimumCapTime;
    
    ShaderPreset* preset;
    std::atomic<int> shaderId;
    bool shaderReady;
    unsigned progressDegree;
    bool progressVisible;
    NSUInteger shaderPasses;
    NSUInteger historySize;
    bool threadAlive;
    unsigned frameCount;
    int frameDirection;

    unsigned deltaTime;
    unsigned lastTime;
    unsigned subFrame;
    unsigned totalFrames;

    GLSlang glSlang;
    MTLTexture luts[MAX_TEXTURES];
    MTLProgram programs[MAX_SHADERS];
    
    std::vector<MTLProgram*> programsTemp;
    std::vector<DiskFile*> lutsTemp;
    
    bool updateRTS;
    bool updateHistory;
    
    std::function<void (int pass, bool hasErrors)> onShaderProgressCallback = nullptr;
    std::function<void (DiskFile& diskFile)> onShaderCacheCallback = nullptr;

    DragndropOverlay dndOverlay;
    SplashScreen splashScreen;
    
    id<MTLDevice> device;
    CAMetalLayer* layer;
    id<MTLCommandQueue> commandQueue;
    MTLClearColor clearColor;
    AppData appData;
    
    id<MTLRenderPipelineState> outputPipelineState;
    id<MTLRenderPipelineState> messagePipelineState;
    id<MTLRenderPipelineState> dndOverlayPipelineState;
    id<MTLRenderPipelineState> splashScreenPipelineState;
    id<MTLRenderPipelineState> progressPipelineState;
    id<MTLRenderPipelineState> hdrPipelineState;
        
    MTLPixelFormat curStockFormat;
    
    matrix_float4x4 projectionMatrix;
    matrix_float4x4 rotatedMatrix;
    
    MTLVertex vertices[4];
    MTLVertex verticesDndOverlay[4];
    MTLVertex verticesSplashScreen[4];
    MTLVertex verticesProgress[4];
    MTLVertexSlang verticesSlang[4];
    
    dispatch_semaphore_t semaphore;
    id<CAMetalDrawable> drawable;
    
    MTLRenderPassDescriptor* rpd;
    
    id<MTLSamplerState> samplers[3][4][2];
    id<MTLSamplerState> sampler;
        
    Video::ScreenshotCallback screenshotCallback = nullptr;

    struct {
        MTLTexture textures[MAX_FRAME_HISTORY + 1];
        Float4 size;
        MTLViewport viewport;
        matrix_float4x4 mvp;
    } frame;
        
    struct HDRUniforms {
        float contrast;
        float paperWhiteNits;
        float maxNits;
        float expandGamut;
        float inverseTonemap;
        float hdr10;
    } hdrUniforms;
    
    MTLTexture messageTex;
    MTLTexture dndOverlayTex;
    MTLTexture splashScreenTex;
    MTLTexture progressTex;
    MTLTexture hdrTex;

    id<MTLBuffer> messageColBuffer;
    id<MTLBuffer> hdrBuffer;
    
    METAL() {
        view = nil;
        handle = nil;
        layer = nullptr;
        resizeWithEmuThread = false;
        settings.rotation = ROT_0;
        settings.hardSync = false;
        settings.synchronize = false;
        settings.linearFilter = true;
        settings.vrr = false;
        settings.useShaderCache = false;
        settings.hdrEnable = false;
        options = 0;
        shaderId = 0;
        preset = nullptr;
        shaderReady = false;
        progressVisible = false;
        progressDegree = 0;
        shaderPasses = 0;
        frameCount = 0;
        historySize = 0;
        threadAlive = false;
        frameData = nullptr;
        updateRTS = false;
        updateHistory = false;
        hdrBuffer = nil;
        messageColBuffer = nil;
        deltaTime = 0;
        lastTime = 0;
        subFrame = 1;
        totalFrames = 1;
        frameDirection = 1;
        
        hdrUniforms.hdr10 = true;
        hdrUniforms.inverseTonemap = true;
        
        for(auto& program : programs) {
            program.renderTarget.view = nil;
            program.feedbackTarget.view = nil;
            program.pipelineState = nil;
            program.buffers[0] = nil;
            program.buffers[1] = nil;
        }
        
        for(auto& lut : luts)
            lut.view = nil;

        for(auto& tex : frame.textures)
            tex.view = nil;
    }
    
    ~METAL() {
        shaderId++;
        while(threadAlive)
            std::this_thread::yield();
        wait();
        RenderThread::enable(false);
        term();
    }
    
    struct {
        bool linearFilter;
        bool synchronize;
        bool vrr;
        float vrrSpeed = 0.0;
        Rotation rotation;
        bool hardSync;
        bool useShaderCache = false;
        bool hdrEnable = false;
        
        unsigned bfiFrames = 0;
        unsigned darkFrames = 0;
        unsigned lightFrames = 0;
    } settings;
    
    auto needResizingPreparations(bool useEmuThread) -> bool {
        resizeWithEmuThread = useEmuThread;
        return false;
    }

    auto init(uintptr_t _handle) -> bool {
        handle = (NSView*)_handle;
        return init();
    }
    
    auto init() -> bool {
        area = [handle frame].size;
        view = [[MetalView alloc] initWithDriver:this];
       
        device = MTLCreateSystemDefaultDevice();
        
        view.device = device;
        layer = (CAMetalLayer*)view.layer;
        
        layer.framebufferOnly = YES;
        layer.displaySyncEnabled = settings.synchronize ? YES : NO;
        
        if (settings.hdrEnable) {
            layer.wantsExtendedDynamicRangeContent = YES;
            layer.pixelFormat = MTLPixelFormatBGR10A2Unorm;
            CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(NSAppKitVersionNumber < NSAppKitVersionNumber11 ? CFSTR("kCGColorSpaceITUR_2020_PQ_EOTF") : CFSTR("kCGColorSpaceITUR_2100_PQ"));
            layer.colorspace = colorSpace;
            CGColorSpaceRelease(colorSpace);
        }
    //     layer.needsDisplayOnBoundsChange = true;
        commandQueue = [device newCommandQueue];
        clearColor = MTLClearColorMake(0, 0, 0, 1);
        semaphore = dispatch_semaphore_create(1);
        
        messageTex.view = nil;
        dndOverlayTex.view = nil;
        splashScreenTex.view = nil;
        
        if (!initStockShader(settings.hdrEnable ? MTLPixelFormatBGRA8Unorm : layer.pixelFormat))
            return false;
        
        MTLSamplerDescriptor* sd = [MTLSamplerDescriptor new];

        for (int i = 0; i < 4; i++) {
            switch(i) {
               case 0: sd.sAddressMode = MTLSamplerAddressModeClampToBorderColor; break;
               case 1: sd.sAddressMode = MTLSamplerAddressModeClampToEdge; break;
               case 2: sd.sAddressMode = MTLSamplerAddressModeRepeat; break;
               case 3: sd.sAddressMode = MTLSamplerAddressModeMirrorRepeat; break;
            }

            sd.tAddressMode = sd.sAddressMode;
            sd.rAddressMode = sd.sAddressMode;
            
            sd.minFilter = MTLSamplerMinMagFilterLinear;
            sd.magFilter = MTLSamplerMinMagFilterLinear;
            sd.mipFilter = MTLSamplerMipFilterNotMipmapped;

            samplers[ShaderPreset::FILTER_LINEAR][i][0] = [device newSamplerStateWithDescriptor:sd];
            
            sd.mipFilter = MTLSamplerMipFilterLinear;
            samplers[ShaderPreset::FILTER_LINEAR][i][1] = [device newSamplerStateWithDescriptor:sd];
            
            
            sd.minFilter = MTLSamplerMinMagFilterNearest;
            sd.magFilter = MTLSamplerMinMagFilterNearest;
            sd.mipFilter = MTLSamplerMipFilterNotMipmapped;

            samplers[ShaderPreset::FILTER_NEAREST][i][0] = [device newSamplerStateWithDescriptor:sd];
            
            sd.mipFilter = MTLSamplerMipFilterNearest;
            samplers[ShaderPreset::FILTER_NEAREST][i][1] = [device newSamplerStateWithDescriptor:sd];
        }
        [sd release];
        
        updateFilter();

        projectionMatrix = {
            simd_make_float4( 2.0,  0.0,  0.0, 0.0),
            simd_make_float4( 0.0,  2.0,  0.0, 0.0),
            simd_make_float4( 0.0,  0.0, -1.0, 0.0),
            simd_make_float4(-1.0, -1.0,  0.0, 1.0)
        };
          
        updateRotation();
        
        vertices[0] = {simd_make_float2(0, 0), simd_make_float2(0, 1)};
        vertices[1] = {simd_make_float2(0, 1), simd_make_float2(0, 0)};
        vertices[2] = {simd_make_float2(1, 0), simd_make_float2(1, 1)};
        vertices[3] = {simd_make_float2(1, 1), simd_make_float2(1, 0)};
        
        verticesSlang[0] = {simd_make_float4(0, 0, 0, 1), simd_make_float2(0, 1)};
        verticesSlang[1] = {simd_make_float4(0, 1, 0, 1), simd_make_float2(0, 0)};
        verticesSlang[2] = {simd_make_float4(1, 0, 0, 1), simd_make_float2(1, 1)};
        verticesSlang[3] = {simd_make_float4(1, 1, 0, 1), simd_make_float2(1, 0)};
        
        rpd = [MTLRenderPassDescriptor new];
        rpd.colorAttachments[0].clearColor = clearColor;
        rpd.colorAttachments[0].loadAction = MTLLoadActionClear;

        [handle addSubview:view];
        updateRTS = true;
        resizeWindow(true);
        
        [view setFrame:NSMakeRect(0, 0, area.width, area.height)];
        [view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

        dndOverlay.initialized = true;
        splashScreen.initialized = true;
        return true;
    }
    
    auto initStockShader(MTLPixelFormat _format) -> bool {
        @autoreleasepool {
            curStockFormat = _format;
            NSError* error;
            MTLRenderPipelineDescriptor* psd;
            MTLRenderPipelineColorAttachmentDescriptor* ca;
            MTLVertexDescriptor* vd = [MTLVertexDescriptor new];
            vd.attributes[0].offset = 0;
            vd.attributes[0].format = MTLVertexFormatFloat2;
            vd.attributes[1].offset = offsetof(MTLVertex, texCoord);
            vd.attributes[1].format = MTLVertexFormatFloat2;
            vd.layouts[0].stride = sizeof(MTLVertex);

            psd = [[MTLRenderPipelineDescriptor new] autorelease];
            psd.label = @"output";

            ca = psd.colorAttachments[0];
            ca.pixelFormat = _format;
            ca.blendingEnabled = NO;
            ca.sourceAlphaBlendFactor= MTLBlendFactorSourceAlpha;
            ca.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
            ca.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
            ca.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

            psd.sampleCount = 1;
            psd.vertexDescriptor = vd;
            
            NSString* outputShaderStr = [NSString stringWithUTF8String:MTLOutputShader.c_str()];
            NSString* messageShaderStr = [NSString stringWithUTF8String:MTLMessageShader.c_str()];
            NSString* dndOverlayShaderStr = [NSString stringWithUTF8String:MTLDndOverlayShader.c_str()];
            NSString* progressShaderStr = [NSString stringWithUTF8String:MTLprogressShader.c_str()];
            NSString* hdrShaderStr = [NSString stringWithUTF8String:MTLhdrShader.c_str()];
            
            //MTLCompileOptions* compileOptions = [MTLCompileOptions new];
            //compileOptions.languageVersion = MTLLanguageVersion2_0;
            
            id<MTLLibrary> lib = [device newLibraryWithSource:outputShaderStr options:nil error:&error];
            if (error != nil)
                return false;
            
            psd.vertexFunction = [lib newFunctionWithName:@"_vertex"];
            psd.fragmentFunction = [lib newFunctionWithName:@"_fragment"];

            outputPipelineState = [device newRenderPipelineStateWithDescriptor:psd error:&error];
            if (error != nil)
                return false;
            
            ca.blendingEnabled = YES;
#ifdef DRV_FREETYPE
            lib = [device newLibraryWithSource:messageShaderStr options:nil error:&error];
            if (error == nil) {
                psd.vertexFunction = [lib newFunctionWithName:@"_vertex"];
                psd.fragmentFunction = [lib newFunctionWithName:@"_fragment"];

                psd.label = @"blend msg";

                messageColBuffer = [device newBufferWithLength:32 options:MTLResourceStorageModeManaged];
                messagePipelineState = [device newRenderPipelineStateWithDescriptor:psd error:&error];
                if (error == nil)
                    ftInitialized = true;
            }
#endif
            lib = [device newLibraryWithSource:dndOverlayShaderStr options:nil error:&error];
            if (error != nil)
                return false;
            
            psd.vertexFunction = [lib newFunctionWithName:@"_vertex"];
            psd.fragmentFunction = [lib newFunctionWithName:@"_fragment"];

            psd.label = @"blend dnd overlay";
            
            dndOverlayPipelineState = [device newRenderPipelineStateWithDescriptor:psd error:&error];
            if (error != nil)
                return false;
            
            splashScreenPipelineState = [device newRenderPipelineStateWithDescriptor:psd error:&error];
            if (error != nil)
                return false;
            
            lib = [device newLibraryWithSource:progressShaderStr options:nil error:&error];
            if (error != nil)
                return false;
            
            psd.vertexFunction = [lib newFunctionWithName:@"_vertex"];
            psd.fragmentFunction = [lib newFunctionWithName:@"_fragment"];

            psd.label = @"blend progress";
            
            progressPipelineState = [device newRenderPipelineStateWithDescriptor:psd error:&error];
            if (error != nil)
                return false;
            
            ca.pixelFormat = MTLPixelFormatBGR10A2Unorm;
            ca.blendingEnabled = NO;
            lib = [device newLibraryWithSource:hdrShaderStr options:nil error:&error];
            if (error != nil)
                return false;
            
            psd.vertexFunction = [lib newFunctionWithName:@"_vertex"];
            psd.fragmentFunction = [lib newFunctionWithName:@"_fragment"];

            psd.label = @"hdr conversion";
            
            hdrPipelineState = [device newRenderPipelineStateWithDescriptor:psd error:&error];
            if (error != nil)
                return false;

            hdrBuffer = [device newBufferWithLength:sizeof(hdrUniforms) options:MTLResourceStorageModeManaged];
            
            updateHDRParams();
        }
        return true;
    }
    
    auto term() -> void {
        wait();
        
        dndOverlay.term();
        [rpd release]; rpd = nil;
        releaseStockShader();
        
        [commandQueue release]; commandQueue = nil;
        [device release]; device = nil;
        
        for(int i = 0; i < 4; i++) {
            [samplers[ShaderPreset::FILTER_LINEAR][i][0] release];
            [samplers[ShaderPreset::FILTER_NEAREST][i][0] release];
            
            [samplers[ShaderPreset::FILTER_LINEAR][i][1] release];
            [samplers[ShaderPreset::FILTER_NEAREST][i][1] release];
        }
        
        releaseShader(true);

        if (frameData) {
            delete[] frameData;
            frameData = nil;
        }
        
        if (view) {
            [view removeFromSuperview];
            [view release];
            view = nil;
        }
    }
        
    auto releaseStockShader() -> void {
        [outputPipelineState release]; outputPipelineState = nil;
#ifdef DRV_FREETYPE
        if (messagePipelineState)
            [messagePipelineState release]; messagePipelineState = nil;
        
        [messageColBuffer release]; messageColBuffer = nil;
#endif
        [dndOverlayPipelineState release]; dndOverlayPipelineState = nil;
        [splashScreenPipelineState release]; splashScreenPipelineState = nil;
        [progressPipelineState release]; progressPipelineState = nil;
        [hdrPipelineState release]; hdrPipelineState = nil;
        [hdrBuffer release]; hdrBuffer = nil;
    }
    
    auto releaseShader(bool withMainTexture = false) -> void {
        for(auto& program : programs)
            MTLUtility::releaseProgram(program);

        for (int i = (withMainTexture ? 0 : 1); i <= MAX_FRAME_HISTORY; i++)
            MTLUtility::releaseTexture(frame.textures[i]);

        for(auto& lut : luts)
            MTLUtility::releaseTexture(lut);
    }
    
    auto lock(unsigned*& data, unsigned& pitch, unsigned _width, unsigned _height, uint8_t options = 0) -> bool {
        
        if (shaderReady) {
            wait();
            RenderThread::reset();
            shaderPostBuild();
            shaderReady = false;
        }

        if (!shaderPasses && (options & OPT_RGB10) ) // YUV input needs a shader to progress it
            return false;
        
        if (threadEnabled)
            return RenderThread::lock(data, pitch, _width, _height, options);
        
        this->options = options;
        MTLTexture& tex = frame.textures[0];
        
        if (MTLUtility::initTexture(tex, _width, _height, MTLPixelFormatBGRA8Unorm, device)) {
            if (frameData)
                delete[] frameData;
            frameData = new uint8_t[tex.bytesPerRow * tex.view.height];
            
            viewScreen.update(viewport);
            updateViewport();
            updateRTS = true;
            updateHistory = true;
        }
        
        data = (unsigned*)frameData;
        pitch = tex.view.width;

        return true;
    }
    
    auto unlockAndRedraw() -> void {
        if (threadEnabled) {
            resizeWindow();
            RenderThread::unlock();
        } else {
            resizeMutex.lock();
            resizeWindow();
            
            MTLTexture& tex = frame.textures[0];

            [tex.view replaceRegion:MTLRegionMake2D(0, 0, (NSUInteger)tex.view.width, (NSUInteger)tex.view.height)
                        mipmapLevel:0 withBytes:frameData bytesPerRow: tex.bytesPerRow];
            
            redrawBase();
            
            resizeMutex.unlock();
        }
    }
    
    auto refresh() -> void {
        resizeMutexThreaded.lock();
        options = 0;
        RenderBuffer* renderBuffer = getBufferToRender();
        
        if (renderBuffer && renderBuffer->height) {
            MTLTexture& tex = frame.textures[0];
            renderBuffer->sharedMutex.lock();
            MTLUtility::initTexture(tex, renderBuffer->width, renderBuffer->height,MTLPixelFormatBGRA8Unorm, device);

            [tex.view replaceRegion:MTLRegionMake2D(0, 0, (NSUInteger)tex.view.width, (NSUInteger)tex.view.height)
                        mipmapLevel:0 withBytes:renderBuffer->data bytesPerRow: tex.bytesPerRow];
            
            options = renderBuffer->options;
            renderBuffer->sharedMutex.unlock();

            accessMutex.lock();
            frames--;
            accessMutex.unlock();
        }
        
        redrawBase();
     
        resizeMutexThreaded.unlock();
    }
    
    auto redraw(CGSize _size) -> void {
        resizeMutex.lock();
        area = _size;
        if (!resizeWithEmuThread) {
            resizeWindow();
            redrawBase();
        }
        resizeMutex.unlock();
    }
    
    auto setLinearFilter(bool state) -> void {
        if (state == settings.linearFilter)
            return;
        wait();
        settings.linearFilter = state;
        updateFilter();
    }
    
    auto adjustSize(unsigned& w, unsigned& h) -> void {
        viewScreen.update(viewport);
        updateViewport();
        updateRTS = true;
        updateHistory = true;
    }
    
    auto setAspectRatio(int mode, bool integerScaling) -> void { // mode: 0: off, 1: TV, 2: Native
        if ((int)viewScreen.mode == mode && viewScreen.hasIntegerScaling == integerScaling)
            return;
        wait();
        viewScreen.mode = (ViewScreen::Mode)mode;
        viewScreen.hasIntegerScaling = integerScaling;
        if (handle) {
            viewScreen.update(viewport);
            updateViewport();
            updateFrameSize();
#ifdef DRV_FREETYPE
            ftUpdateCoords();
#endif
            updateHistory = true;
        }
    }

    auto getAspectRatio() -> int {
        return (int)viewScreen.mode;
    }
    
    auto setIntegerScalingDimension( unsigned _w, unsigned _h, bool _ds) -> void {
        viewScreen.scaling.width = _w;
        viewScreen.scaling.height = _h;
        viewScreen.scaling.doubleSize = _ds;
        // we don't need to update now because the input dimension will be changed too
    }

    auto getIntegerScalingDimension(unsigned& _w, unsigned& _h) -> void {
        _w = viewScreen.scaling.width >> 1;
        _h = viewScreen.scaling.height >> 1;
    }
    
    auto resizeWindow(bool _force = false) -> void {
        unsigned _windowWidth = area.width;
        unsigned _windowHeight = area.height;

        if (!_force) {
            if ( (_windowWidth == viewScreen.windowWidth) && (_windowHeight == viewScreen.windowHeight) )
                return;
        }

        viewScreen.update(viewport, _windowWidth, _windowHeight);
        
        shaderResizeTimer = 5;
        //updateFrameSize();
        updateViewport();
        
#ifdef DRV_FREETYPE
        ftUpdateCoords();
#endif
        if (settings.hdrEnable)
            updateHDRTexture();
    }
    
    void synchronize(bool state) {
        wait();
        resizeMutex.lock();
        settings.synchronize = state;
        if (layer && handle)
            layer.displaySyncEnabled = state ? YES : NO;
        resizeMutex.unlock();
    }
    
    auto hasSynchronized() -> bool { return settings.synchronize; }
    
    auto getViewport() -> Viewport& { return viewport; }
    
    auto getRotation() -> Rotation { return settings.rotation; }

    auto setRotation(Rotation rotation) -> void {
        if (settings.rotation == rotation)
            return;
        wait();
        settings.rotation = rotation;
        viewScreen.flipped = settings.rotation == ROT_90 || settings.rotation == ROT_270;
        viewScreen.update(viewport);
        
        updateFrameSize();
        updateViewport();
        updateRotation();
        
        updateRTS = true;
        updateHistory = true;
    }
    
    auto changeThreadPriorityToRealtime(bool state) -> void {
        if (threadEnabled) {
            wait();
            changePriorityToRealtime(state);
        }
    }
    
    auto setThreaded(bool state) -> void {
        if (state != threadEnabled) {
            RenderThread::enable(state);
            RenderThread::reset();
            
            auto& tex = frame.textures[0];
            if (tex.view) {
                [tex.view release];
                tex.view = nil;
            }
            
        }
    }

    auto hasThreaded() -> bool { return threadEnabled; }
    
#ifdef DRV_FREETYPE
    auto showScreenText(const std::string text, unsigned duration, bool warn = false) -> void {
        ftUpdateMessage(text, duration, warn);
    }

    auto setScreenTextDescription(ScreenTextDescription& desc) -> void {
        ftSetScreenTextDescription(desc);
    }

    auto freeFont() -> void {
        wait();
        ftUnload();
    }
#endif
    
    auto hardSync(bool state) -> void {
        wait();
        settings.hardSync = state;
    }
    
    auto canHardSync() -> bool { return true; }
        
    auto getAppData() -> AppData* { return &appData; }
        
    auto showSplashScreen(unsigned frames, SplashscreenCallback callback) -> void {
        splashScreen.prepare(frames, callback);
    }

    auto hideSplashScreen() -> void {
        splashScreen.hide();
    }

    auto visibleSplashScreen() -> bool {
        return splashScreen.isVisible();
    }
    
    auto setDragnDropOverlayCallback(DnDOverlayCallback callback) -> void {
        dndOverlay.callback = callback;
    }

    auto setDragnDropOverlaySlots(unsigned slots) -> void {
        dndOverlay.setSlots(slots);
    }

    auto enableDragnDropOverlay(bool state) -> void {
        dndOverlay.setEnable(state);
    }

    auto sendDragnDropOverlayCoordinates(int x, int y) -> int {
        return dndOverlay.sendDragnDropOverlayCoordinates(x, y);
    }
    
    auto setVRR(bool state, float speed = 0.0) -> void {
        wait();
        settings.vrr = state;
        if (speed != 0.0)
            settings.vrrSpeed = speed;
        updateVRR();
    }
        
    auto updateVRR() -> void {
        if (settings.vrr) {
            if (settings.bfiFrames)
                minimumCapTime = (1000000.0 / (settings.vrrSpeed + ((float)settings.bfiFrames * settings.vrrSpeed) ) ) + 0.5;
            else
                minimumCapTime = (1000000.0 / settings.vrrSpeed) + 0.5;
            lastCapTime = Chronos::getTimestampInMicroseconds();
        }
    }
        
    auto setBFI(unsigned frames, unsigned darkFrames) -> void {
        wait();
        subFrame = 1;
        totalFrames = frames + 1;
        settings.bfiFrames = frames;
        settings.darkFrames = darkFrames > frames ? frames : darkFrames;
        settings.lightFrames = frames - settings.darkFrames;
        updateVRR();
    }

    auto waitRenderThread() -> void { if (threadEnabled) wait(); }

    auto hasVRR() -> bool { return settings.vrr; }

    auto waitVRR() -> void {
        lastCapTime += minimumCapTime;
        int64_t remaining  = lastCapTime - Chronos::getTimestampInMicroseconds();

        if (remaining <= 0) {
            lastCapTime = Chronos::getTimestampInMicroseconds();
            return;
        }

        if (remaining >= 3000) {

            remaining -= 1500;

            unsigned sleepInMilli = (unsigned) ((float) remaining / 1000.0);

            usleep( sleepInMilli * 1000 );
            
            remaining = lastCapTime - Chronos::getTimestampInMicroseconds();
        }

        // we need exact frame pacing
        while(remaining > 0) {
            //std::this_thread::yield();
            remaining = lastCapTime - Chronos::getTimestampInMicroseconds();
        }
    }
    
    auto setShaderProgressCallback( std::function<void (int pass, bool hasErrors)> onCallback ) -> void {
        onShaderProgressCallback = onCallback;
    }

    auto setShaderCacheCallback( std::function<void (DiskFile& diskFile)> onCallback ) -> void {
        onShaderCacheCallback = onCallback;
    }

    auto useShaderCache(bool state) -> void {
        settings.useShaderCache = state;
    }
    
    auto shaderSupport() -> bool { return true; }
    
    auto redrawBase(bool bfiLock = false) -> void {
        @autoreleasepool {
            if (shaderResizeTimer && !--shaderResizeTimer) {
                updateFrameSize();
            }
            
            bool requestScreenshot = options & OPT_TakeScreenshot;
            bool disallowShader = options & OPT_DisallowShader;
            layer.framebufferOnly = requestScreenshot ? NO : YES;
            MTLTexture& mainTex = frame.textures[0];
            
            if (updateRTS)
                updateRenderTargets(mainTex.view.width, mainTex.view.height);
            
            id<MTLRenderCommandEncoder> rce = 0;
            dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
            id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
            drawable = layer.nextDrawable;
            
            MTLTexture* texture = &frame.textures[0];
            
            bool useHDRShader = settings.hdrEnable && (hdrTex.view.pixelFormat != MTLPixelFormatBGR10A2Unorm);
            
            if (!disallowShader && shaderPasses) {
                frameCount += 1;
                unsigned curTime = Chronos::getTimestampInMicrosecondsPrecise();
                deltaTime = curTime - lastTime;
                lastTime = curTime;
                frameDirection = (options & OPT_Rewind) ? -1 : 1;

                if (!bfiLock) {
                    subFrame = 1;
                    if (settings.bfiFrames)
                        totalFrames = (options & OPT_Pause) ? 1 : (settings.bfiFrames + 1);
                }

              // if (!updateRTS) {
                    for(int i = 0; i < shaderPasses; i++) {
                        auto& p = programs[i];
                        if (!p.inUse || !p.feedback)
                            continue;

                        MTLTexture tmp = p.feedbackTarget;
                        p.feedbackTarget = p.renderTarget;
                        p.renderTarget = tmp;
                    }
             //   }
                
                rpd.colorAttachments[0].loadAction = MTLLoadActionDontCare;
                //rpd.colorAttachments[0].storeAction = MTLStoreActionStore;

                for(int i = 0; i < shaderPasses; i++) {
                    auto& p = programs[i];
                    if (!p.inUse)
                        continue;
                    
                    if (p.renderTarget.view == nil) { // shader handles last pass
                        rpd.colorAttachments[0].loadAction = MTLLoadActionClear;
                        if (useHDRShader)
                            rpd.colorAttachments[0].texture = hdrTex.view;
                        else
                            rpd.colorAttachments[0].texture = drawable.texture;
                    } else
                        rpd.colorAttachments[0].texture = p.renderTarget.view;
                    
                    rce = [commandBuffer renderCommandEncoderWithDescriptor:rpd];
                    [rce setRenderPipelineState:p.pipelineState];

                    if (p.frameModulo)
                        p.frameCount = frameCount % p.frameModulo;
                    else
                        p.frameCount = frameCount;

                    for (int b = 0; b < SemanticBuffer::Max; b++) {
                        id<MTLBuffer> buffer = p.buffers[b];
                        auto& semBuffer = p.semanticBuffer[b];
                        void* bufData = buffer.contents;

                        if (semBuffer.mask && bufData) {
            
                            for(auto& var : semBuffer.variables) {
                                if (var.data)
                                    memcpy((uint8_t*)bufData + var.offset, var.data, var.size);
                            }

                            if(semBuffer.mask & SpirvReflection::Vertex)
                                [rce setVertexBuffer:buffer offset:0 atIndex:semBuffer.binding];
                            if(semBuffer.mask & SpirvReflection::Fragment)
                                [rce setFragmentBuffer:buffer offset:0 atIndex:semBuffer.binding];
                            
                         // next command informs GPU, required only for storageModeManaged (default on Intel)
                          [buffer didModifyRange:NSMakeRange(0, buffer.length)];
                        }
                    }

                    id<MTLTexture> textures[SPIRV_MAX_BINDINGS] = { nil };
                    id<MTLSamplerState> _samplers[SPIRV_MAX_BINDINGS] = { nil };

                    for(auto& semTex : p.semanticTextures) {
                        textures[semTex.binding] = ( id<MTLTexture>)*(void**)semTex.data;
                        _samplers[semTex.binding] = (id<MTLSamplerState>)semTex.sampler;
                    }

                    [rce setFragmentTextures:textures withRange:NSMakeRange(0, SPIRV_MAX_BINDINGS)];
                    [rce setFragmentSamplerStates:_samplers withRange:NSMakeRange(0, SPIRV_MAX_BINDINGS)];
                    [rce setVertexBytes:&verticesSlang length:sizeof(verticesSlang) atIndex:4];

                    if (p.renderTarget.view == nil) {
                        texture = nullptr;
                        break;
                    }

                    [rce setViewport:p.viewport];
                    
                    [rce drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];

                    [rce endEncoding];
                    
                    if (p.mipmap) {
                        // note: one encoder at a time for same commandbuffer, so "[rce endEncoding]" before
                        id<MTLBlitCommandEncoder> bce = [commandBuffer blitCommandEncoder];
                        
                        if (p.mipmap)
                            [bce generateMipmapsForTexture:(p.renderTarget.view)];
                        
                        [bce endEncoding];
                        bce = nil;
                    }

                    texture = &p.renderTarget;
                }
                
                if (historySize) {
                    if (updateHistory) {
                        for(int i = 1; i <= historySize; i++) {
                            MTLUtility::releaseTexture(frame.textures[i]);
                            MTLUtility::initTexture(frame.textures[i], mainTex.view.width, mainTex.view.height, mainTex.view.pixelFormat, device);
                        }
                        
                        updateHistory = false;
                    } else {
                        MTLTexture tmp = frame.textures[historySize];
                        for (int i = historySize; i > 0; i--)
                            frame.textures[i] = frame.textures[i - 1];
                        frame.textures[0] = tmp;
                    }
                }
                
                updateRTS = false;
            }
            
            if (texture) {
                rpd.colorAttachments[0].loadAction = MTLLoadActionClear;

                if (useHDRShader)
                    rpd.colorAttachments[0].texture = hdrTex.view;
                else
                    rpd.colorAttachments[0].texture = drawable.texture;
                
                rce = [commandBuffer renderCommandEncoderWithDescriptor:rpd];
                
                [rce setRenderPipelineState:outputPipelineState];
                [rce setVertexBytes:&rotatedMatrix length:sizeof(matrix_float4x4) atIndex:1];
                if (options & OPT_DisallowFilter)
                    [rce setFragmentSamplerState:samplers[ShaderPreset::FILTER_NEAREST][ShaderPreset::WRAP_EDGE][0] atIndex : 0];
                else
                    [rce setFragmentSamplerState:sampler atIndex:0];
                [rce setVertexBytes:&vertices length:sizeof(vertices) atIndex:0];
                [rce setFragmentTexture:texture->view atIndex:0];
            }
            
            if (rce) {
                [rce setViewport:frame.viewport];
                [rce drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
            }
            
#ifdef DRV_FREETYPE
            showText(rce);
#endif
            
            if (splashScreen.enable) {
                if(buildSplashScreenTexture()) {
                    rpd.colorAttachments[0].clearColor = clearColor;
                    rpd.colorAttachments[0].loadAction = MTLLoadActionClear;
                    showSplashScreen(rce);
                }
            }
            
            if (dndOverlay.enabled()) {
                buildDndOverlayTexture();
                showDndOverlay(rce);
            }
            
            if (progressVisible && progressTex.view) {
                setProgressPosition();
                showProgress(rce);
            }
            
            if (useHDRShader) {
                if (requestScreenshot)
                    updateHDRParams(true);
                
                rpd.colorAttachments[0].texture = drawable.texture;
                
                if (rce)
                    [rce endEncoding];
                rce = [commandBuffer renderCommandEncoderWithDescriptor:rpd];
                
                [rce setRenderPipelineState:hdrPipelineState];
                [rce setFragmentBuffer:hdrBuffer offset:0 atIndex:1];
              //  [hdrBuffer didModifyRange:NSMakeRange(0, hdrBuffer.length)];
                [rce setVertexBytes:&projectionMatrix length:sizeof(matrix_float4x4) atIndex:1];
                [rce setFragmentSamplerState:samplers[ShaderPreset::FILTER_NEAREST][ShaderPreset::WRAP_EDGE][0] atIndex:0];
                [rce setVertexBytes:&vertices length:sizeof(vertices) atIndex:0];
                [rce setFragmentTexture:hdrTex.view atIndex:0];
                
                [rce setViewport:{0.0, 0.0, (double)viewScreen.windowWidth, (double)viewScreen.windowHeight, 0.0, 1.0}];
                [rce drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
            }
            
            if (rce) {
                [rce endEncoding];
                rce = nil;
            }
            
            __block dispatch_semaphore_t _semaphore = semaphore;
            [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> _buf) {
                dispatch_semaphore_signal(_semaphore);
            }];
            
            if (drawable)
                [commandBuffer presentDrawable:drawable];
            
            if (settings.vrr) {
                //MTLCommandBufferStatus s = [commandBuffer status];
                //if (s != MTLCommandBufferStatusNotEnqueued)
                    //[_commandBuffer waitUntilCompleted];
                waitVRR();
                [commandBuffer commit];
                [commandBuffer waitUntilCompleted];
            } else {
                [commandBuffer commit];
                if (settings.hardSync && settings.synchronize)
                    [commandBuffer waitUntilCompleted];
            }
        
            if (requestScreenshot) {
                [commandBuffer waitUntilCompleted];
                takeScreenshot();
            }

            commandBuffer = nil;
            drawable = nil;
            
            if (settings.bfiFrames && !(options & (OPT_DisallowShader | OPT_Pause) ) && !bfiLock) {
                for (int i = 0; i < settings.lightFrames; i++) {
                    subFrame++;
                    redrawBase(true);
                }
                
                for (int i = 0; i < settings.darkFrames; i++) {
                    commandBuffer = [commandQueue commandBuffer];
                    drawable = layer.nextDrawable;
                    rpd.colorAttachments[0].clearColor = clearColor;
                    rpd.colorAttachments[0].loadAction = MTLLoadActionClear;
                    rpd.colorAttachments[0].texture = drawable.texture;
                    
                    rce = [commandBuffer renderCommandEncoderWithDescriptor:rpd];
                    [rce endEncoding];
                    
                    __block dispatch_semaphore_t _semaphore = semaphore;
                    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> _buf) {
                        dispatch_semaphore_signal(_semaphore);
                    }];
                    
                    [commandBuffer presentDrawable:drawable];
                    if (settings.vrr) {
                        waitVRR();
                        [commandBuffer commit];
                        [commandBuffer waitUntilCompleted];
                    } else
                        [commandBuffer commit];
                }
            }
        }
    }

#ifdef DRV_FREETYPE
    auto ftBuildTexture(std::string text, bool keepOldSize = false) -> void {
        if (!ftBuildText(text, keepOldSize))
            return;

        if (!messageTex.view || !keepOldSize)
            MTLUtility::initTexture(messageTex, ftTotalWidth, ftTotalHeight, MTLPixelFormatA8Unorm, device);

        [messageTex.view replaceRegion:MTLRegionMake2D(0, 0, (NSUInteger)ftTotalWidth, (NSUInteger)ftTotalHeight) mipmapLevel:0 withBytes:ftTextBuffer bytesPerRow: ftTotalWidth];
    }

    auto ftSetColor(FtColNorm& _colNorm, FtColNorm& _colBgNorm) -> void {
        void* bufData = messageColBuffer.contents;
        if (bufData) {
            memcpy((uint8_t*)bufData + 0, &_colNorm, 16);
            memcpy((uint8_t*)bufData +16, &_colBgNorm, 16);

            [messageColBuffer didModifyRange:NSMakeRange(0, messageColBuffer.length)];
        }
    }

    auto showText(id<MTLRenderCommandEncoder> rce) -> void {
        if (ftUpdated)
            ftProcessUpdates(viewport);

        if (!ftTextBuffer)
            return;

        if (ftTs) // animations
            if (ftHandleAnimation(viewport))
                return;
        
        [rce setRenderPipelineState:messagePipelineState];
        [rce setFragmentBuffer:messageColBuffer offset:0 atIndex:1];

        [rce setFragmentSamplerState:samplers[ShaderPreset::FILTER_NEAREST][ShaderPreset::WRAP_EDGE][0] atIndex:0];

        [rce setVertexBytes:&ftPosCoords length:sizeof(ftPosCoords) atIndex:0];
        [rce setFragmentTexture:messageTex.view atIndex:0];

        [rce drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    }
#endif

    auto buildDndOverlayTexture() -> void {
        dndOverlay.update(viewport);
        if (!dndOverlay.buffer)
            return;
        
        dndOverlay.updateAlpha();
        
        MTLUtility::initTexture(dndOverlayTex, dndOverlay.texWidth, dndOverlay.texHeight, MTLPixelFormatRGBA8Unorm, device);
        
        [dndOverlayTex.view replaceRegion:MTLRegionMake2D(0, 0, (NSUInteger)dndOverlay.texWidth, (NSUInteger)dndOverlay.texHeight) mipmapLevel:0 withBytes:dndOverlay.buffer bytesPerRow: dndOverlay.texWidth * 4];
        
        float screenx = 2.0f / (float)viewport.width, screeny = 2.0f / (float)viewport.height;
        float x = -1.0 + dndOverlay.texX * screenx;
        float y = 1.0 - dndOverlay.texY * screeny;

        float w = (float)dndOverlay.texWidth * screenx;
        float h = (float)dndOverlay.texHeight * screeny;
        
        verticesDndOverlay[0] = {simd_make_float2(x    , y),      simd_make_float2(0, 0)};
        verticesDndOverlay[1] = {simd_make_float2(x + w, y),      simd_make_float2(1, 0)};
        verticesDndOverlay[2] = {simd_make_float2(x    , y - h),  simd_make_float2(0, 1)};
        verticesDndOverlay[3] = {simd_make_float2(x + w, y - h),  simd_make_float2(1, 1)};
    }

    auto buildSplashScreenTexture() -> bool {
        auto s = splashScreen.update(viewport);
        
        if (s == SplashScreen::FINISH)
            return false;
        
        if (splashScreenTex.view == nil || s == SplashScreen::TEXTURE_UPDATE) {
            MTLUtility::initTexture(splashScreenTex, splashScreen.viewport.width, splashScreen.viewport.height, MTLPixelFormatRGBA8Unorm, device);
            
            if (splashScreenTex.view == nil) {
                splashScreen.finish();
                return false;
            }
        }
        
        if (s == SplashScreen::DATA_UPDATE || s == SplashScreen::TEXTURE_UPDATE) {
            [splashScreenTex.view replaceRegion:MTLRegionMake2D(0, 0, (NSUInteger)splashScreen.viewport.width, (NSUInteger)splashScreen.viewport.height) mipmapLevel:0 withBytes:splashScreen.screenData bytesPerRow: splashScreen.viewport.width * 4];
        }
        
        float screenx = 2.0f / (float)viewport.width, screeny = 2.0f / (float)viewport.height;
        float x = -1.0 + (float)splashScreen.viewport.x * screenx;
        float y = 1.0 - (float)splashScreen.viewport.y * screeny;

        float w = (float)splashScreen.viewport.width * screenx;
        float h = (float)splashScreen.viewport.height * screeny;

        verticesDndOverlay[0] = {simd_make_float2(x    , y),      simd_make_float2(0, 0)};
        verticesDndOverlay[1] = {simd_make_float2(x + w, y),      simd_make_float2(1, 0)};
        verticesDndOverlay[2] = {simd_make_float2(x    , y - h),  simd_make_float2(0, 1)};
        verticesDndOverlay[3] = {simd_make_float2(x + w, y - h),  simd_make_float2(1, 1)};
        
        return true;
    }
    
    auto setProgressAnimation(uint8_t* _data, unsigned _width, unsigned _height) -> void {
        MTLUtility::releaseTexture(progressTex);
        MTLUtility::initTexture(progressTex, _width, _height, MTLPixelFormatRGBA8Unorm, device);
        
        [progressTex.view replaceRegion:MTLRegionMake2D(0, 0, (NSUInteger)_width, (NSUInteger)_height) mipmapLevel:0 withBytes:_data bytesPerRow: progressTex.bytesPerRow];
    }
    
    auto setProgressPosition() -> void {
        float screenx = 2.0f / (float)viewport.width, screeny = 2.0f / (float)viewport.height;
        
        float x = -1.0 + (viewport.width - progressTex.view.width - 20) * screenx;
        float y = 1.0 -  20.0 * screeny;

        float w = progressTex.size.x * screenx;
        float h = progressTex.size.y * screeny;
        
        verticesProgress[0] = {simd_make_float2(x    , y),      simd_make_float2(0, 0)};
        verticesProgress[1] = {simd_make_float2(x + w, y),      simd_make_float2(1, 0)};
        verticesProgress[2] = {simd_make_float2(x    , y - h),  simd_make_float2(0, 1)};
        verticesProgress[3] = {simd_make_float2(x + w, y - h),  simd_make_float2(1, 1)};
    }

    auto showDndOverlay(id<MTLRenderCommandEncoder> rce) -> void {
        [rce setRenderPipelineState:dndOverlayPipelineState];
        [rce setFragmentSamplerState:samplers[ShaderPreset::FILTER_LINEAR][ShaderPreset::WRAP_EDGE][0] atIndex:0];
        [rce setVertexBytes:&verticesDndOverlay length:sizeof(verticesDndOverlay) atIndex:0];
        [rce setFragmentTexture:dndOverlayTex.view atIndex:0];

        [rce drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    }
        
    auto showSplashScreen(id<MTLRenderCommandEncoder> rce) -> void {
        [rce setRenderPipelineState:splashScreenPipelineState];
        [rce setFragmentSamplerState:samplers[ShaderPreset::FILTER_LINEAR][ShaderPreset::WRAP_EDGE][0] atIndex:0];
        [rce setVertexBytes:&verticesSplashScreen length:sizeof(verticesSplashScreen) atIndex:0];
        [rce setFragmentTexture:splashScreenTex.view atIndex:0];

        [rce drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    }
    
    auto showProgress(id<MTLRenderCommandEncoder> rce) -> void {
        [rce setRenderPipelineState:progressPipelineState];
        [rce setVertexBytes:&progressDegree length:sizeof(unsigned) atIndex:1];
        [rce setFragmentSamplerState:samplers[ShaderPreset::FILTER_LINEAR][ShaderPreset::WRAP_EDGE][0] atIndex:0];
        [rce setVertexBytes:&verticesProgress length:sizeof(verticesProgress) atIndex:0];
        [rce setFragmentTexture:progressTex.view atIndex:0];
        
        if (++progressDegree == 360)
            progressDegree = 0;
        
        [rce drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    }

    auto updateFrameSize() -> void {
        frame.size.x = viewport.width;
        frame.size.y = viewport.height;
        frame.size.z = 1.0f / (float)viewport.width;
        frame.size.w = 1.0f / (float)viewport.height;
        updateRTS = true; // in the case of passes scaled by viewport
    }
    
    auto updateViewport() -> void {
        layer.drawableSize = CGSizeMake(viewScreen.windowWidth, viewScreen.windowHeight);
        
        frame.viewport.originX = viewport.x;
        frame.viewport.originY = viewport.y;
        frame.viewport.width   = viewport.width;
        frame.viewport.height  = viewport.height;
        frame.viewport.znear   = 0.0f;
        frame.viewport.zfar    = 1.0f;
    }
    
    auto updateFilter() -> void {
        ShaderPreset::Filter filter = settings.linearFilter ? ShaderPreset::FILTER_LINEAR : ShaderPreset::FILTER_NEAREST;

        for (int i = 0; i < 4; i++) {
            samplers[ShaderPreset::FILTER_UNSPEC][i][0] = samplers[filter][i][0];
            samplers[ShaderPreset::FILTER_UNSPEC][i][1] = samplers[filter][i][1];
        }

        sampler = samplers[filter][ShaderPreset::WRAP_EDGE][0];
        
        for(int i = 0; i < shaderPasses; i++) {
            auto& p = programs[i];
            if (!p.inUse)
                continue;

            for(auto& tex : p.semanticTextures) {
                tex.sampler = (uintptr_t)samplers[tex.filter][tex.wrap][tex.mipmap];
            }
        }
    }
    
    auto updateRotation() -> void {
        float radian = (float)settings.rotation * 90.0 * (M_PI / 180.0f);
        
        float cz, sz;
        __sincosf(radian, &sz, &cz);

        matrix_float4x4 rot = {
            simd_make_float4(cz, -sz, 0, 0),
            simd_make_float4(sz,  cz, 0, 0),
            simd_make_float4( 0,   0, 1, 0),
            simd_make_float4( 0,   0, 0, 1)
        };

        rotatedMatrix = simd_mul(rot, projectionMatrix);
    }
    
    auto updateRenderTargets(unsigned width, unsigned height) -> void {
        frame.mvp = projectionMatrix; // assume: last shader pass is NOT final pass

        for(int i = 0; i < shaderPasses; i++) {
            auto& p = programs[i];
            if (!p.inUse)
                continue;

            bool lastPass = i == (shaderPasses - 1);

            if (p.scaleTypeX != ShaderPreset::SCALE_NONE || p.scaleTypeY != ShaderPreset::SCALE_NONE) {
                if (p.scaleTypeX == ShaderPreset::SCALE_INPUT) width *= p.scaleX;
                else if (p.scaleTypeX == ShaderPreset::SCALE_VIEWPORT) width = viewport.width * p.scaleX;
                else if (p.scaleTypeX == ShaderPreset::SCALE_ABSOLUTE) width = p.absX;

                if (!width) width = viewport.width;

                if (p.scaleTypeY == ShaderPreset::SCALE_INPUT) height *= p.scaleY;
                else if (p.scaleTypeY == ShaderPreset::SCALE_VIEWPORT) height = viewport.height * p.scaleY;
                else if (p.scaleTypeY == ShaderPreset::SCALE_ABSOLUTE) height = p.absY;

                if (!height) height = viewport.height;
            } else if (lastPass) {
                width = viewport.width;
                height = viewport.height;
            }

            if (!lastPass || p.feedback || (width != viewport.width) || (height != viewport.height)
                // Unlike D3D11, the pixel format must be specified when creating metal shaders,
                // and it must match that of the render texture. Blended objects have the same pixel
                // format as the output stock shader. If this is not used and the final shader pass
                // has a different format, a crash will occur.
                || (p.format != MTLPixelFormatBGRA8Unorm ) ) {
                if (p.mipmap)
                    MTLUtility::releaseTexture(p.renderTarget);
                
                if (!MTLUtility::initTexture(p.renderTarget, width, height, p.format, device, p.mipmap, true))
                    continue;

                if (p.feedback) {
                    MTLUtility::releaseTexture(p.feedbackTarget);
                    MTLUtility::initTexture(p.feedbackTarget, width, height, p.format, device, p.mipmap, true);
                }
                
                p.viewport.originX = 0;
                p.viewport.originY = 0;
                p.viewport.width   = width;
                p.viewport.height  = height;
                p.viewport.znear   = 0.0f;
                p.viewport.zfar    = 1.0f;

            } else {
                MTLUtility::releaseTexture(p.renderTarget);
                
                if (viewScreen.flipped && (viewScreen.mode != ViewScreen::Mode::Window)) {
                    unsigned tmp = width;
                    width = height;
                    height = tmp;
                }

                p.renderTarget.size = {(float)width, (float)height, 1.0f / float(width), 1.0f / float(height)};
                
                frame.mvp = rotatedMatrix; // last shader pass is final pass
            }
        }
        
        MTLPixelFormat _format;
        
        if (settings.hdrEnable) {
            _format = MTLPixelFormatBGRA8Unorm;
            updateHDRTexture();
            
            if (shaderPasses) {
                auto& p = programs[shaderPasses-1];
                
                if (p.renderTarget.view != nil)
                    _format = p.format;
            }
        } else
            _format = layer.pixelFormat;
        
        if (_format != curStockFormat) {
            releaseStockShader();
            initStockShader(_format);
        }
    }
        
    auto updateHDRTexture() -> void {
        MTLUtility::releaseTexture(hdrTex);
        MTLPixelFormat _format;
        hdrUniforms.inverseTonemap = true;
        
        if (shaderPasses) {
            auto& p = programs[shaderPasses-1];
            
            if (!preset->lumaChroma && (p.format == MTLPixelFormatRGBA16Float))
                hdrUniforms.inverseTonemap = false;
            
            _format = p.format;
        } else
            _format = MTLPixelFormatBGRA8Unorm;
        
        updateHDRParams();
        
        MTLUtility::initTexture(hdrTex, viewScreen.windowWidth, viewScreen.windowHeight, _format, device, false, true);
    }
    
    auto setShader(ShaderPreset* preset) -> void {
        wait();
        resizeMutex.lock();
     //   @autoreleasepool {
            loadShader( preset );
        //}
        RenderThread::reset();
        resizeMutex.unlock();
    }
    
    auto loadShader(ShaderPreset* preset) -> void {
        shaderId++;
        shaderReady = false;
        progressVisible = false;
        shaderPasses = 0;
        historySize = 0;
        std::vector<MTLProgram*> _programs;
        std::vector<DiskFile*> _luts;
        
        releaseShader();
        
        this->preset = preset;
        
        if (!preset || (preset->passes.size() > MAX_SHADERS) || (preset->luts.size() > MAX_TEXTURES)) {
            if (settings.hdrEnable)
                updateRTS = true;
            return;
        }
        
        bool todo = false;
        _programs.reserve(preset->passes.size());

        for (auto& pass : preset->passes) {
            MTLProgram* program = new MTLProgram;
            program->pipelineState = nil;
            program->inUse = pass.inUse;
            program->codeVertex = pass.vertex;
            program->codeFragment = pass.fragment;
            program->format =   MTLUtility::getFormat(pass.bufferType);
            pass.error = "";
            _programs.push_back(program);
            todo |= pass.inUse;
        }

        if (!todo) {
            if (settings.hdrEnable)
                updateRTS = true;
            return;
        }

        _luts.reserve(preset->luts.size());
        for (auto& lut : preset->luts) {
            DiskFile* diskFile = new DiskFile;
            diskFile->path = lut.path;
            diskFile->ident = lut.id;
            diskFile->data = nullptr;
            diskFile->size = 0;
            diskFile->isLUT = true;
            _luts.push_back(diskFile);
        }

        progressVisible = true;
        int _sid = shaderId;
        bool useCache = settings.useShaderCache;
        
        std::thread worker([this, _programs, _luts, _sid, useCache] {
            DRIVER::ShaderCache shaderCache("msl");
            shaderCache.onShaderCacheCallback = onShaderCacheCallback;
            
            auto ts = Chronos::getTimestampInMilliseconds();
            
            for(int i = 0; i < _programs.size(); i++) {
                auto p = _programs[i];
                if (!p->inUse)
                    continue;
                
                bool success;
                bool cacheSuccess = false;
                std::string nativeV;
                std::string nativeF;
                spirv_cross::CompilerMSL* vCompiler = nullptr;
                spirv_cross::CompilerMSL* fCompiler = nullptr;
                
                if (useCache)
                    cacheSuccess = shaderCache.read(p->codeVertex, p->codeFragment, p->reflection);
                
                if (!cacheSuccess) {
                    std::vector<unsigned> spirvVertex;
                    std::vector<unsigned> spirvFragment;

                    success = glSlang.compileVertex(p->codeVertex, spirvVertex, p->error);
                    if (!success) {
                        p->error = "SLANG Vertex Shader #" + std::to_string(i) + " to SPIRV conversion error:\n" + p->error;
                        goto Next;
                    }

                    success = glSlang.compileFragment(p->codeFragment, spirvFragment, p->error);
                    if (!success) {
                        p->error = "SLANG Fragment Shader #" + std::to_string(i) + " to SPIRV conversion error:\n" + p->error;
                        goto Next;
                    }

                    try {
                        vCompiler = new spirv_cross::CompilerMSL( spirvVertex );
                        fCompiler = new spirv_cross::CompilerMSL( spirvFragment );

                        spirv_cross::ShaderResources vResources = vCompiler->get_shader_resources();
                        spirv_cross::ShaderResources fResources = fCompiler->get_shader_resources();

                        p->reflection.preProcess( *vCompiler, vResources );
                        p->reflection.preProcess( *fCompiler, fResources );

                        spirv_cross::CompilerMSL::Options opt;
                        opt.msl_version = 20000;
                        vCompiler->set_msl_options(opt);
                        fCompiler->set_msl_options(opt);
                        
                        p->reflection.preProcessRemapPush(*vCompiler, vResources);
                        p->reflection.preProcessRemapPush(*fCompiler, fResources);
                        p->reflection.preProcessGenericResources(*vCompiler, vResources.uniform_buffers);
                        p->reflection.preProcessGenericResources(*fCompiler, fResources.uniform_buffers);
                        p->reflection.preProcessGenericResources(*vCompiler, vResources.sampled_images);
                        p->reflection.preProcessGenericResources(*fCompiler, fResources.sampled_images);
                        
                        nativeV = vCompiler->compile();
                        nativeF = fCompiler->compile();

                        success = p->reflection.process( *vCompiler, *fCompiler, vResources, fResources );
                        if (!success) {
                            p->error = "SPIRV Shader #" + std::to_string(i) + " reflection error:\n" + p->reflection.error;
                            goto Next;
                        }

                    } catch (const std::exception& e) {
                        p->error = "SPIRV Shader #" + std::to_string(i) + " to MSL conversion error:\n" + e.what();
                        success = false;
                        goto Next;
                    }
                    
                    success = MTLUtility::createProgram(nativeV, nativeF, device, *p);
                    
                    if (!success) {
                        p->error = "MSL Shader #" + std::to_string(i) + " compilation error: " + p->error + "\n";
                        goto Next;
                    }

                    if (useCache)
                        shaderCache.write((uint8_t*)nativeV.data(), nativeV.size(), (uint8_t*)nativeF.data(), nativeF.size(), p->reflection);

                } else {
                    nativeV.assign((char*)shaderCache.nativeV, shaderCache.vertexSize);
                    nativeF.assign((char*)shaderCache.nativeF, shaderCache.fragmentSize);
                    
                    success = MTLUtility::createProgram(nativeV, nativeF, device, *p);

                    if (!success) {
                        p->error = "MSL Shader #" + std::to_string(i) + " from cache creation error:\n" + p->error;
                        goto Next;
                    }
                }

                Next:
                if (vCompiler) delete vCompiler;
                if (fCompiler) delete fCompiler;

                if (shaderId != _sid)
                    break;

                if (!success)
                    MTLUtility::releaseShader(*p);

                auto tsNow = Chronos::getTimestampInMilliseconds();
                if (!success || ((tsNow - ts) >= 1000)) {
                    onShaderProgressCallback(i, !success);
                    ts = tsNow;
                }
            }
            
            for(auto lut : _luts) {
                onShaderCacheCallback(*lut);

                if (shaderId != _sid)
                    break;
            }

            if (shaderId != _sid) { // the user is impatient
                for(auto p : _programs) {
                    MTLUtility::releaseShader(*p);
                    delete p;
                }
                for(auto l : _luts) {
                    if (l->data) delete[] l->data;
                    delete l;
                }
                threadAlive = false;
                return;
            }

            while(shaderReady)
                std::this_thread::yield();

            programsTemp = _programs;
            lutsTemp = _luts;
            shaderReady = true;
            progressVisible = false;
            threadAlive = false;
        });
        
        threadAlive = true;
        worker.detach();
    }
    
    auto shaderPostBuild() -> void {
        bool lastPass = true;
        bool mipMapInput = false;

        SemanticMap map = {{
           {(uintptr_t)(&frame.textures[0].view), &frame.textures[0].size, sizeof(MTLTexture), MAX_FRAME_HISTORY},
           {(uintptr_t)(&programs[0].renderTarget.view), &programs[0].renderTarget.size, sizeof(MTLProgram), MAX_SHADERS},
           {(uintptr_t)(&programs[0].feedbackTarget.view), &programs[0].feedbackTarget.size, sizeof(MTLProgram), MAX_SHADERS},
           {(uintptr_t)(&luts[0].view), &luts[0].size, sizeof(MTLTexture), MAX_TEXTURES},
        }, {nullptr, nullptr, &frame.size, nullptr, &frameDirection, &deltaTime, &settings.vrrSpeed, &settings.rotation,
            &viewport.ratio, &viewport.ratioRot, &totalFrames, &subFrame, &historySize, &appData.ledDriveState} };

        shaderPasses = 0;
        for(int i = programsTemp.size() - 1; i >= 0; i--) {
            auto p = programsTemp[i];
            auto& program = programs[i];
            auto& pass = preset->passes[i];

            program.feedback = false;
            program.pipelineState = p->pipelineState;
            program.format = p->format;

            program.inUse = pass.inUse;
            program.ident = pass.alias;
            program.scaleX = pass.scaleX;
            program.absX = pass.absX;
            program.scaleTypeX = pass.scaleTypeX;
            program.scaleY = pass.scaleY;
            program.absY = pass.absY;
            program.scaleTypeY = pass.scaleTypeY;
            program.filter = pass.filter;
            program.wrap = pass.wrap;
            program.frameModulo = pass.frameModulo;
            program.mipmap = false;

            if (!pass.inUse)
                goto Next;

            if (mipMapInput) {
                program.mipmap = true;
                mipMapInput = false;
            }

            if (pass.mipmap)
                mipMapInput = true;

            if (!p->error.empty()) {
                pass.error = p->error;
                shaderPasses = 0;
                lastPass = false;
                goto Next;
            }

            map.uniforms[SemanticMap::MVP] = lastPass ? (void*)&frame.mvp : &projectionMatrix;
            map.uniforms[SemanticMap::Output] = &program.renderTarget.size;
            map.uniforms[SemanticMap::FrameCount] = &program.frameCount;

            if (lastPass) {
                lastPass = false;
                shaderPasses = i + 1;
            }

            if (!p->reflection.bindTextures(preset, i, map, program.semanticTextures)) {
                pass.error = "Shader #" + std::to_string(i) + " texture resolve error:\n" + p->reflection.error;
                shaderPasses = 0;
                goto Next;
            }

            if (!p->reflection.bindUbo(preset, i, map, &program.semanticBuffer[SemanticBuffer::Ubo])) {
                pass.error = "Shader #" + std::to_string(i) + " ubo uniform resolve error:\n" + p->reflection.error;
                shaderPasses = 0;
                goto Next;
            }

            if (!p->reflection.bindPush(preset, i, map, &program.semanticBuffer[SemanticBuffer::Push])) {
                pass.error = "Shader #" + std::to_string(i) + " push uniform resolve error:\n" + p->reflection.error;
                shaderPasses = 0;
                goto Next;
            }

            for (int b = 0; b < SemanticBuffer::Max; b++) {
                unsigned _size = program.semanticBuffer[b].size;
                
                if (_size) {
                    program.buffers[b] = [device newBufferWithLength:_size options:MTLResourceStorageModeManaged];
                }
            }

            Next:
            delete p;
        }
        programsTemp.clear();
        
        if (shaderPasses) {
            for(int i = 0; i < shaderPasses; i++) {
                auto& p = programs[i];

                if (p.inUse) {
                    for(auto& tex : p.semanticTextures) {
                        if ( (tex.feedbackPass != -1) && (tex.feedbackPass < shaderPasses))
                            programs[tex.feedbackPass].feedback = true;
                    }
                }
            }
            id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
            id<MTLBlitCommandEncoder> bce = [commandBuffer blitCommandEncoder];
            
            for (int l = 0; l < preset->luts.size(); l++) {
                auto& lut = preset->luts[l];
                DiskFile* lutFile = nullptr;
                for(auto lutPtr : lutsTemp) {
                    if (lutPtr->ident == lut.id) {
                        lutFile = lutPtr;
                        break;
                    }
                }
                if (!lutFile || !lutFile->data) {
                    preset->luts[l].error = true;
                    shaderPasses = 0;
                    continue;
                }

                MTLTexture& lutTex = luts[l];
                MTLUtility::releaseTexture(lutTex);
                MTLUtility::initTexture(lutTex, lutFile->width, lutFile->height, MTLPixelFormatRGBA8Unorm, device, lut.mipmap);
                
                [lutTex.view replaceRegion:MTLRegionMake2D(0, 0, (NSUInteger)lutTex.view.width, (NSUInteger)lutTex.view.height)
                            mipmapLevel:0 // fill in original texture, next command generates mips for the requested mipmapLevelCount
                            withBytes:lutFile->data bytesPerRow: lutTex.bytesPerRow];
                
                if (lut.mipmap)
                    [bce generateMipmapsForTexture:lutTex.view];
            }
            
            updateFilter();
            
            // in case we have mips
            [bce endEncoding];
            [commandBuffer commit];
            [commandBuffer waitUntilCompleted];
            commandBuffer = nil;
        }

        for(auto lutPtr : lutsTemp) {
            if(lutPtr->data) delete[] lutPtr->data;
            delete lutPtr;
        }
        lutsTemp.clear();

        onShaderProgressCallback(-1, !shaderPasses);
        updateRTS = true;
        updateHistory = true;
        updateFrameSize();
    }
    
    auto getShaderNativeVertexCode(std::string& slang, std::string& out) -> bool {
        return MTLUtility::translate(slang, out, false);
    }

    auto getShaderNativeFragmentCode(std::string& slang, std::string& out) -> bool {
        return MTLUtility::translate(slang, out, true);
    }
        
    auto setScreenshotCallback(ScreenshotCallback callback) -> void {
        this->screenshotCallback = callback;
    }
        
    auto takeScreenshot() -> void {
        options &= ~OPT_TakeScreenshot;
        if (!screenshotCallback)
            return;
        
        size_t bufferSize = viewport.width * viewport.height * 4;
        uint8_t* buffer = new uint8_t[bufferSize];
        MTLRegion region = MTLRegionMake2D(viewport.x, viewport.y, viewport.width, viewport.height);
        
        [drawable.texture getBytes:buffer bytesPerRow: (viewport.width * 4) fromRegion:region mipmapLevel: 0];
        
        unsigned rgba;
        uint32_t* pSource = (uint32_t*)buffer;
        uint8_t* pTarget = buffer;
        
        if (drawable.layer.pixelFormat == MTLPixelFormatBGR10A2Unorm) {
            for (int y = 0; y < viewport.height; ++y) {
                for (int x = 0; x < viewport.width; ++x) {
                    rgba = *pSource++;
                    *pTarget++ = (rgba >> 22) & 0xff;
                    *pTarget++ = (rgba >> 12) & 0xff;
                    *pTarget++ = (rgba >> 2) & 0xff;
                }
            }
        } else {
            for (int y = 0; y < viewport.height; ++y) {
                for (int x = 0; x < viewport.width; ++x) {
                    rgba = *pSource++;
                    *pTarget++ = (rgba >> 16) & 0xff;
                    *pTarget++ = (rgba >> 8) & 0xff;
                    *pTarget++ = rgba & 0xff;
                }
            }
        }

        screenshotCallback(buffer, viewport.width, viewport.height);

        delete[] buffer;
        
        if (settings.hdrEnable)
            updateHDRParams();
    }
        
    auto HDRsupport() -> bool { return true; }
        
    auto setHDR(bool state, float maxNits, float paperWhiteNits, float contrast, bool expandGamut) -> void {
        wait();
        hdrUniforms.maxNits = maxNits;
        hdrUniforms.paperWhiteNits = paperWhiteNits;
        hdrUniforms.contrast = contrast;
        hdrUniforms.expandGamut = expandGamut;
        
        if (!layer || !handle) {
            settings.hdrEnable = state;
            return;
        }
        
        if (settings.hdrEnable != state) {
            wait();
            settings.hdrEnable = state;
            
            if (settings.hdrEnable) {
                layer.wantsExtendedDynamicRangeContent = YES;
                layer.pixelFormat = MTLPixelFormatBGR10A2Unorm;
                CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(NSAppKitVersionNumber < NSAppKitVersionNumber11 ? CFSTR("kCGColorSpaceITUR_2020_PQ_EOTF") : CFSTR("kCGColorSpaceITUR_2100_PQ"));
                layer.colorspace = colorSpace;
                CGColorSpaceRelease(colorSpace);
            } else {
                layer.wantsExtendedDynamicRangeContent = NO;
                layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
                CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
                layer.colorspace = colorSpace;
                CGColorSpaceRelease(colorSpace);
            }
            updateRTS = true;
        } else if (settings.hdrEnable) {
            wait();
            updateHDRParams();
        }
    }
        
    auto updateHDRParams(bool disableConversion = false) -> void {
        void* bufData = hdrBuffer.contents;
        if (bufData) {
            float _inverse = hdrUniforms.inverseTonemap;
            float _hdr10 = hdrUniforms.hdr10;
            if (disableConversion) {
                _inverse = _hdr10 = (float)false;
            }
            
            memcpy((uint8_t*)bufData + 0, &hdrUniforms.contrast, 4);
            memcpy((uint8_t*)bufData + 4, &hdrUniforms.paperWhiteNits, 4);
            memcpy((uint8_t*)bufData + 8, &hdrUniforms.maxNits, 4);
            memcpy((uint8_t*)bufData + 12, &hdrUniforms.expandGamut, 4);
            memcpy((uint8_t*)bufData + 16, &_inverse, 4);
            memcpy((uint8_t*)bufData + 20, &_hdr10, 4);

            [hdrBuffer didModifyRange:NSMakeRange(0, hdrBuffer.length)];
        }
    }
};

}


@implementation MetalView {
    DRIVER::METAL* driver;
}
-(id) initWithDriver:(DRIVER::METAL*)_driver {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        driver = _driver;
        self.paused = NO;
        self.enableSetNeedsDisplay = NO;
        [self setDelegate:self];
    }
    return self;
}

- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
    driver->redraw(size);
}

- (void)drawInMTKView:(MTKView*)view {
   // driver->redraw( [view drawableSize]);
}

@end
