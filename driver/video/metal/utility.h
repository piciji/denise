#pragma once

namespace DRIVER {
    
    struct MTLUtility {
        
        static auto getFormat(ShaderPreset::BufferType& bufferType) -> MTLPixelFormat {
            switch(bufferType) {
                case ShaderPreset::BufferType::R8_UNORM: return MTLPixelFormatR8Unorm;
                case ShaderPreset::BufferType::R8_UINT: return MTLPixelFormatR8Uint;
                case ShaderPreset::BufferType::R8_SINT: return MTLPixelFormatR8Sint;
                case ShaderPreset::BufferType::R8G8_UNORM: return MTLPixelFormatRG8Unorm;
                case ShaderPreset::BufferType::R8G8_UINT: return MTLPixelFormatRG8Uint;
                case ShaderPreset::BufferType::R8G8_SINT: return MTLPixelFormatRG8Sint;
                    
                case ShaderPreset::BufferType::R8G8B8A8_UNORM: return MTLPixelFormatBGRA8Unorm;
                case ShaderPreset::BufferType::R8G8B8A8_UINT: return MTLPixelFormatRGBA8Uint;
                case ShaderPreset::BufferType::R8G8B8A8_SINT: return MTLPixelFormatRGBA8Sint;
                case ShaderPreset::BufferType::R8G8B8A8_SRGB: return MTLPixelFormatBGRA8Unorm_sRGB;
                    
                case ShaderPreset::BufferType::A2B10G10R10_UNORM_PACK32: return MTLPixelFormatRGB10A2Unorm;
                case ShaderPreset::BufferType::A2B10G10R10_UINT_PACK32: return MTLPixelFormatRGB10A2Uint;
                    
                case ShaderPreset::BufferType::R16_UINT: return MTLPixelFormatR16Uint;
                case ShaderPreset::BufferType::R16_SINT: return MTLPixelFormatR16Sint;
                case ShaderPreset::BufferType::R16_SFLOAT: return MTLPixelFormatR16Float;
                case ShaderPreset::BufferType::R16G16_UINT: return MTLPixelFormatRG16Uint;
                case ShaderPreset::BufferType::R16G16_SINT: return MTLPixelFormatRG16Sint;
                case ShaderPreset::BufferType::R16G16_SFLOAT: return MTLPixelFormatRG16Float;
                case ShaderPreset::BufferType::R16G16B16A16_UINT: return MTLPixelFormatRGBA16Uint;
                case ShaderPreset::BufferType::R16G16B16A16_SINT: return MTLPixelFormatRGBA16Sint;
                case ShaderPreset::BufferType::R16G16B16A16_SFLOAT: return MTLPixelFormatRGBA16Float;
                    
                case ShaderPreset::BufferType::R32_UINT: return MTLPixelFormatR32Uint;
                case ShaderPreset::BufferType::R32_SINT: return MTLPixelFormatR32Sint;
                case ShaderPreset::BufferType::R32_SFLOAT: return MTLPixelFormatR32Float;
                case ShaderPreset::BufferType::R32G32_UINT: return MTLPixelFormatRG32Uint;
                case ShaderPreset::BufferType::R32G32_SINT: return MTLPixelFormatRG32Sint;
                case ShaderPreset::BufferType::R32G32_SFLOAT: return MTLPixelFormatRG32Float;
                case ShaderPreset::BufferType::R32G32B32A32_UINT: return MTLPixelFormatRGBA32Uint;
                case ShaderPreset::BufferType::R32G32B32A32_SINT: return MTLPixelFormatRGBA32Sint;
                case ShaderPreset::BufferType::R32G32B32A32_SFLOAT: return MTLPixelFormatRGBA32Float;
                    
                default:
                    break;
            }
            
            return MTLPixelFormatInvalid;
        }
        
        static auto releaseTexture(MTLTexture& tex) {
            if (tex.view) {
                // [tex.view setPurgeableState:MTLPurgeableStateEmpty];
                [tex.view release];
                tex.view = nil;
            }
        }
        
        static auto releaseProgram(MTLProgram& prg) -> void {
            prg.semanticTextures.clear();
            prg.semanticBuffer[SemanticBuffer::Ubo].variables.clear();
            prg.semanticBuffer[SemanticBuffer::Push].variables.clear();
            
            for (int b = 0; b < SemanticBuffer::Max; b++) {
                if (prg.buffers[b]) {
                    // [prg.buffers[b] setPurgeableState:MTLPurgeableStateEmpty];
                    [prg.buffers[b] release];
                    prg.buffers[b] = nil;
                }
            }
            
            prg.semanticBuffer[SemanticBuffer::Ubo].mask = 0;
            prg.semanticBuffer[SemanticBuffer::Push].mask = 0;
            
            releaseTexture(prg.renderTarget);
            releaseTexture(prg.feedbackTarget);
            releaseTexture(prg.cropTarget);
            releaseShader(prg);
        }
        
        static auto releaseShader(MTLProgram& prg) -> void {
            if (prg.pipelineState) {
                [prg.pipelineState release];
                prg.pipelineState = nil;
            }
            
            prg.reflection.clear();
        }
        
        static auto createProgram(std::string& vertex, std::string& fragment, id<MTLDevice> device, MTLProgram& prg) -> bool {
            
            @autoreleasepool {
                NSString* shaderVertex = [NSString stringWithUTF8String:vertex.c_str()];
                NSString* shaderFragment = [NSString stringWithUTF8String:fragment.c_str()];
                
                NSError* error;
                MTLVertexDescriptor* vd = [[MTLVertexDescriptor new] autorelease];
                vd.attributes[0].offset = offsetof(MTLVertexSlang, position);
                vd.attributes[0].format = MTLVertexFormatFloat4;
                vd.attributes[0].bufferIndex = 4;
                vd.attributes[1].offset = offsetof(MTLVertexSlang, texCoord);
                vd.attributes[1].format = MTLVertexFormatFloat2;
                vd.attributes[1].bufferIndex = 4;
                vd.layouts[4].stride = sizeof(MTLVertexSlang);
                vd.layouts[4].stepFunction = MTLVertexStepFunctionPerVertex;
                
                MTLRenderPipelineDescriptor* psd = [[MTLRenderPipelineDescriptor new] autorelease];
                MTLRenderPipelineColorAttachmentDescriptor* ca = psd.colorAttachments[0];
                
                ca.pixelFormat = prg.format;
                
                ca.blendingEnabled = NO;
                ca.sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
                ca.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
                ca.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
                ca.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
                
                psd.sampleCount = 1;
                psd.vertexDescriptor = vd;
                
                id<MTLLibrary> lib = [[device newLibraryWithSource:shaderVertex options:nil error:&error] autorelease];
                
                if (lib == nil) {
                    prg.error = "can't create vertex shader ";
                    if (error != nil)
                        prg.error += error.localizedDescription.UTF8String;
                    return false;
                }
                
                psd.vertexFunction = [lib newFunctionWithName:@"main0"];
                
                lib = [[device newLibraryWithSource:shaderFragment options:nil error:&error] autorelease];
                
                if (lib == nil) {
                    prg.error = "can't create fragment shader ";
                    if (error != nil)
                        prg.error += error.localizedDescription.UTF8String;
                    return false;
                }
                
                psd.fragmentFunction = [lib newFunctionWithName:@"main0"];
                
                prg.pipelineState = [device newRenderPipelineStateWithDescriptor:psd error:&error];
                
                if (error != nil) {
                    prg.error = "can't create shader pipeline state ";
                    prg.error += error.localizedDescription.UTF8String;
                    return false;
                }
            }
            
            return true;
        }
        
        static auto initTexture(MTLTexture& tex, unsigned newWidth, unsigned newHeight, MTLPixelFormat newFormat, id<MTLDevice> device, bool mipmapped = false, bool renderTarget = false) -> bool {
            
            if(tex.view != nil && tex.view.pixelFormat == newFormat && tex.width == newWidth && tex.height == newHeight)
                return false;
            
            tex.width = newWidth;
            tex.height = newHeight;
            
            if (tex.view)
                [tex.view release];
            
            MTLTextureDescriptor* td = [MTLTextureDescriptor
                                        texture2DDescriptorWithPixelFormat:newFormat
                                        width:(NSUInteger)newWidth
                                        height:(NSUInteger)newHeight
                                        mipmapped:(mipmapped ? YES : NO)];
            
            td.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
            if (renderTarget) {
                td.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
                td.storageMode = MTLStorageModeShared; // private causes problems while swapping feedbacks
            } else {
                tex.bytesPerRow = newWidth;
                if (newFormat == MTLPixelFormatBGRA8Unorm || newFormat == MTLPixelFormatRGBA8Unorm)
                    tex.bytesPerRow <<= 2;
                else if (newFormat == MTLPixelFormatRGBA32Float)
                    tex.bytesPerRow <<= 4;
            }
            
            if (mipmapped)
                td.mipmapLevelCount = getMipLevels(newWidth, newHeight);
            
            tex.view = [device newTextureWithDescriptor:td];
            
            tex.size = {(float)tex.width, (float)tex.height, 1.0f / float(tex.width), 1.0f / float(tex.height)};
            
            return true;
        }
        
        static auto translate(std::string& code, std::string& out, bool isFragment) -> bool {
            GLSlang glSlang;
            std::string error;
            std::vector<unsigned> spirv;
            spirv_cross::CompilerMSL* compiler = nullptr;
            SpirvReflection reflection;
            bool success = isFragment ? glSlang.compileFragment(code, spirv, error) : glSlang.compileVertex(code, spirv, error);

            if (!success) {
                out = "SLANG Shader to SPIRV conversion error:\n" + error;
                return false;
            }

            try {
                compiler = new spirv_cross::CompilerMSL(spirv);
                spirv_cross::ShaderResources resources = compiler->get_shader_resources();
                reflection.preProcess( *compiler, resources );

                spirv_cross::CompilerMSL::Options options;
                options.msl_version = 20000;
                compiler->set_msl_options(options);
                
                reflection.preProcessRemapPush(*compiler, resources);
                reflection.preProcessGenericResources(*compiler, resources.uniform_buffers);
                reflection.preProcessGenericResources(*compiler, resources.sampled_images);
                
                out = compiler->compile();
            } catch (const std::exception& e) {
                error = e.what();
                out = "SPIRV Shader to MSL conversion error:\n" + error;
                if (compiler) delete compiler;
                return false;
            }
            if (compiler) delete compiler;
            return true;
        }
    };
}
