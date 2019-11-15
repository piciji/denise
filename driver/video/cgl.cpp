
#define GL_ALPHA_TEST 0x0bc0
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
    
struct CGL : public Video, OpenGL {
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
            [handle addSubview:view];
            [context setView:view];
            [view lockFocus];

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
        OpenGL::term();

        @autoreleasepool {
            [view removeFromSuperview];
            [view release];
            view = nil;
        }
    }

    bool lock(unsigned*& data, unsigned& pitch, unsigned width, unsigned height) {
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
    
    void unlock() {}
    void clear() {
        @autoreleasepool {
            [view lockFocus];
            OpenGL::clear();
            [[view openGLContext] flushBuffer];
            [view unlockFocus];
        }
    }
    void redraw() {
        @autoreleasepool {
            if([view lockFocusIfCanDraw]) {
                auto area = [view frame];
                outputWidth = area.size.width, outputHeight = area.size.height;
                OpenGL::refresh();
                [[view openGLContext] flushBuffer];
                if(settings.hardSync && settings.synchronize) glFinish();
                [view unlockFocus];
            }
        }
    }
    void synchronize(bool state) {
        settings.synchronize = state;

        if(view) {
            @autoreleasepool {
                [[view openGLContext] makeCurrentContext];
                int synchronize = settings.synchronize;
                [[view openGLContext] setValues:&synchronize forParameter:NSOpenGLCPSwapInterval];
            }
        }
    }
    
    auto shaderFormat() -> ShaderType { return ShaderType::GLSL; }

    auto hardSync(bool state) -> void {
        settings.hardSync = state;
    }

    auto setShader(std::vector<ShaderPass*> passes) -> void {
        @autoreleasepool {
            [[view openGLContext] makeCurrentContext];
        }
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
    
    auto showMessage(std::string message, bool critical = false) -> void {
        OpenGL::showMessage(message, critical);
    }

    CGL() {
        view = nil;
        handle = nil;
    }
    
    ~CGL() { term(); }
    
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
