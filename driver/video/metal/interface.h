
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include <simd/simd.h>

namespace DRIVER { struct METAL; }

struct MTLTexture {
    id<MTLTexture> view = nil;
    Float4 size;
    unsigned bytesPerRow;
    
    // seems redundant, but we need this when swapping crop with render target to keep track of real size
    unsigned width = 0;
    unsigned height = 0;
};

struct MTLVertex {
    vector_float2 position;
    vector_float2 texCoord;
};

struct MTLVertexSlang {
   vector_float4 position;
   vector_float2 texCoord;
};

struct MTLProgram {
    MTLTexture renderTarget;
    MTLTexture feedbackTarget;
    bool inUse;
    std::string codeFragment;
    std::string codeVertex;
    id<MTLRenderPipelineState> pipelineState;
    SpirvReflection reflection;
    std::string error;
    MTLPixelFormat format;
    std::string ident;
    MTLViewport viewport;
    
    bool mipmap = false;
    bool feedback = false;
    unsigned frameCount = 0;
    unsigned frameModulo = 0;
    
    std::vector<SemanticTexture> semanticTextures;
    SemanticBuffer semanticBuffer[2];
    
    id<MTLBuffer> buffers[2];
    
    ShaderPreset::ScaleType scaleTypeX;
    ShaderPreset::ScaleType scaleTypeY;
    float scaleX;
    float scaleY;
    unsigned absX;
    unsigned absY;
    ShaderPreset::Filter filter;
    ShaderPreset::WrapMode wrap;
};

@interface MetalView : MTKView<MTKViewDelegate>
-(id) initWithDriver:(DRIVER::METAL*)_driver;
@end
