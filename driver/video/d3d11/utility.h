
#pragma once

namespace DRIVER {

struct D3D11Utility {

    static auto getFormat(ShaderPreset::BufferType& bufferType) -> DXGI_FORMAT {
        if (bufferType == ShaderPreset::BufferType::R8_UNORM) return DXGI_FORMAT_R8_UNORM;
        if (bufferType == ShaderPreset::BufferType::R8_UINT) return DXGI_FORMAT_R8_UINT;
        if (bufferType == ShaderPreset::BufferType::R8_SINT) return DXGI_FORMAT_R8_SINT;
        if (bufferType == ShaderPreset::BufferType::R8G8_UNORM) return DXGI_FORMAT_R8G8_UNORM;
        if (bufferType == ShaderPreset::BufferType::R8G8_UINT) return DXGI_FORMAT_R8G8_UINT;
        if (bufferType == ShaderPreset::BufferType::R8G8_SINT) return DXGI_FORMAT_R8G8_SINT;
        if (bufferType == ShaderPreset::BufferType::R8G8B8A8_UNORM) return DXGI_FORMAT_R8G8B8A8_UNORM;
        if (bufferType == ShaderPreset::BufferType::R8G8B8A8_UINT) return DXGI_FORMAT_R8G8B8A8_UINT;
        if (bufferType == ShaderPreset::BufferType::R8G8B8A8_SINT) return DXGI_FORMAT_R8G8B8A8_SINT;
        if (bufferType == ShaderPreset::BufferType::R8G8B8A8_SRGB) return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

        if (bufferType == ShaderPreset::BufferType::A2B10G10R10_UNORM_PACK32) return DXGI_FORMAT_R10G10B10A2_UNORM;
        if (bufferType == ShaderPreset::BufferType::A2B10G10R10_UINT_PACK32) return DXGI_FORMAT_R10G10B10A2_UINT;

        if (bufferType == ShaderPreset::BufferType::R16_UINT) return DXGI_FORMAT_R16_UINT;
        if (bufferType == ShaderPreset::BufferType::R16_SINT) return DXGI_FORMAT_R16_SINT;
        if (bufferType == ShaderPreset::BufferType::R16_SFLOAT) return DXGI_FORMAT_R16_FLOAT;
        if (bufferType == ShaderPreset::BufferType::R16G16_UINT) return DXGI_FORMAT_R16G16_UINT;
        if (bufferType == ShaderPreset::BufferType::R16G16_SINT) return DXGI_FORMAT_R16G16_SINT;
        if (bufferType == ShaderPreset::BufferType::R16G16_SFLOAT) return DXGI_FORMAT_R16G16_FLOAT;
        if (bufferType == ShaderPreset::BufferType::R16G16B16A16_UINT) return DXGI_FORMAT_R16G16B16A16_UINT;
        if (bufferType == ShaderPreset::BufferType::R16G16B16A16_SINT) return DXGI_FORMAT_R16G16B16A16_SINT;
        if (bufferType == ShaderPreset::BufferType::R16G16B16A16_SFLOAT) return DXGI_FORMAT_R16G16B16A16_FLOAT;

        if (bufferType == ShaderPreset::BufferType::R32_UINT) return DXGI_FORMAT_R32_UINT;
        if (bufferType == ShaderPreset::BufferType::R32_SINT) return DXGI_FORMAT_R32_SINT;
        if (bufferType == ShaderPreset::BufferType::R32_SFLOAT) return DXGI_FORMAT_R32_FLOAT;
        if (bufferType == ShaderPreset::BufferType::R32G32_UINT) return DXGI_FORMAT_R32G32_UINT;
        if (bufferType == ShaderPreset::BufferType::R32G32_SINT) return DXGI_FORMAT_R32G32_SINT;
        if (bufferType == ShaderPreset::BufferType::R32G32_SFLOAT) return DXGI_FORMAT_R32G32_FLOAT;
        if (bufferType == ShaderPreset::BufferType::R32G32B32A32_UINT) return DXGI_FORMAT_R32G32B32A32_UINT;
        if (bufferType == ShaderPreset::BufferType::R32G32B32A32_SINT) return DXGI_FORMAT_R32G32B32A32_SINT;
        if (bufferType == ShaderPreset::BufferType::R32G32B32A32_SFLOAT) return DXGI_FORMAT_R32G32B32A32_FLOAT;
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    }

    static auto createShader( D3D11Symbols& symbols, D3D_FEATURE_LEVEL& featureLevel, ID3D11Device* device, const std::string& data, const std::string psEntry, const std::string vsEntry, const std::string gsEntry,
                       const D3D11_INPUT_ELEMENT_DESC* inputElementDescs, unsigned numElements, D3DShader* out, bool dumpByteCode = false) -> bool {

        ID3DBlob* error = nullptr;
        const char* msg = nullptr;

        ID3DBlob* psCode = nullptr;
        ID3DBlob* vsCode = nullptr;
        ID3DBlob* gsCode = nullptr;

        bool level5Support = featureLevel >= D3D_FEATURE_LEVEL_11_0;

        if (!psEntry.empty() && FAILED(symbols.D3DCompile(data.c_str(), data.size(), nullptr, nullptr, nullptr, psEntry.c_str(), level5Support ? "ps_5_0" : "ps_4_0", 0, 0, &psCode, &error)))
            msg = (const char*)error->GetBufferPointer();

        if (!vsEntry.empty() && FAILED(symbols.D3DCompile(data.c_str(), data.size(), nullptr, nullptr, nullptr, vsEntry.c_str(), level5Support ? "vs_5_0" : "vs_4_0", 0, 0, &vsCode, &error)))
            msg = (const char*)error->GetBufferPointer();

        if (!gsEntry.empty() && FAILED(symbols.D3DCompile(data.c_str(), data.size(), nullptr, nullptr, nullptr, gsEntry.c_str(), level5Support ? "gs_5_0" : "gs_4_0", 0, 0, &gsCode, &error)))
            msg = (const char*)error->GetBufferPointer();

        if (psCode) {
            if (FAILED(device->CreatePixelShader( psCode->GetBufferPointer(), psCode->GetBufferSize(), nullptr, &out->ps)))
                msg = "can't create pixel shader";
        }

        if (vsCode) {
            LPVOID bufPtr  = vsCode->GetBufferPointer();
            SIZE_T bufSize = vsCode->GetBufferSize();
            if (FAILED(device->CreateVertexShader(bufPtr, bufSize, nullptr, &out->vs)))
                msg = "can't create vertex shader";

            if (inputElementDescs)
                if (FAILED(device->CreateInputLayout(inputElementDescs, numElements, bufPtr, bufSize, &out->layout)))
                    msg = "can't create shader layout";
        }

        if (gsCode)
            if (FAILED(device->CreateGeometryShader(gsCode->GetBufferPointer(), gsCode->GetBufferSize(), nullptr, &out->gs)))
                msg = "can't create geometry shader";

        out->error = msg ? msg : "";

        if (!dumpByteCode) {
            dxRelease(vsCode)
            dxRelease(psCode)
        } else {
            if (psCode) out->psCode = psCode;
            if (vsCode) out->vsCode = vsCode;
        }
        dxRelease(gsCode)
        dxRelease(error)

        return msg == nullptr;
    }

    static auto createShader( D3D11Symbols& symbols, ID3D11Device* device, uint8_t* vs, unsigned vsSize, uint8_t* ps, unsigned psSize, const D3D11_INPUT_ELEMENT_DESC* inputElementDescs, unsigned numElements, D3DShader* out) -> bool {
        ID3DBlob* psCode = nullptr;
        ID3DBlob* vsCode = nullptr;

        LPVOID bufPtr;
        SIZE_T bufSize;

        if (FAILED(symbols.D3DCreateBlob(vsSize, &vsCode))) {
            out->error = "can't allocate memory for vertex shader";
            return false;
        }

        if (vsCode) {
            bufPtr = vsCode->GetBufferPointer();
            bufSize = vsCode->GetBufferSize();
            std::memcpy(bufPtr, vs, bufSize);

            if (FAILED(device->CreateVertexShader(bufPtr, bufSize, nullptr, &out->vs)))
                out->error = "can't create vertex shader from precompiled dump";

            if (inputElementDescs)
                if (FAILED(device->CreateInputLayout(inputElementDescs, numElements, bufPtr, bufSize, &out->layout)))
                    out->error = "can't create shader layout";
        }

        if (FAILED(symbols.D3DCreateBlob(psSize, &psCode))) {
            out->error = "can't allocate memory for pixel shader";
            return false;
        }

        if (psCode) {
            bufPtr = psCode->GetBufferPointer();
            bufSize = psCode->GetBufferSize();
            std::memcpy( bufPtr, ps, bufSize );

            if (FAILED(device->CreatePixelShader( bufPtr, bufSize, nullptr, &out->ps)))
                out->error = "can't create pixel shader from precompiled dump";
        }

        dxRelease(vsCode)
        dxRelease(psCode)
        return true;
    }

    static auto buildTexture(ID3D11DeviceContext* context, D3DTexture& tex, uint8_t* data) -> bool {
        D3D11_MAPPED_SUBRESOURCE mappedTexture;
        if (tex.staging) {
            if (FAILED(context->Map((ID3D11Resource*)tex.staging, 0, D3D11_MAP_WRITE, 0, &mappedTexture)))
                return false;
        }
        else {
            if (FAILED(context->Map((ID3D11Resource*)tex.ptr, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedTexture)))
                return false;
        }

        int srcPitch = tex.desc.Width << (tex.desc.Format == DXGI_FORMAT_A8_UNORM ? 0 : 2);
        uint8_t* src = data;
        unsigned pitch = mappedTexture.RowPitch;
        uint8_t* dest = (uint8_t*) mappedTexture.pData;

        for (int i = 0; i < tex.desc.Height; i++) {
            std::memcpy(dest, src, srcPitch);
            src += srcPitch;
            dest += pitch;
        }

        if (tex.staging) {
            context->Unmap((ID3D11Resource*) tex.staging, 0);
            context->CopyResource((ID3D11Resource*) tex.ptr, (ID3D11Resource*) tex.staging);
        } else
            context->Unmap((ID3D11Resource*)tex.ptr, 0);

        if (tex.desc.MiscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS)
            context->GenerateMips(tex.view);

        return true;
    }

    static auto initTexture(ID3D11Device* device, D3DTexture& tex, bool useStaging = true) -> bool {
        tex.desc.MipLevels          = 1;
        tex.desc.ArraySize          = 1;
        tex.desc.SampleDesc.Count   = 1;
        tex.desc.SampleDesc.Quality = 0;
        tex.desc.BindFlags          |= D3D11_BIND_SHADER_RESOURCE;
        tex.desc.Usage              = useStaging ? D3D11_USAGE_DEFAULT : D3D11_USAGE_DYNAMIC;
        if (!useStaging)
            tex.desc.CPUAccessFlags     = D3D11_CPU_ACCESS_WRITE;

        bool render = tex.desc.BindFlags & D3D11_BIND_RENDER_TARGET;

        if (tex.desc.MiscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS) {
            tex.desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
            tex.desc.MipLevels = getMipLevels(tex.desc.Width, tex.desc.Height);
        }

        if (FAILED(device->CreateTexture2D(&tex.desc, nullptr, &tex.ptr)))
            return false;

        D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
        std::memset(&viewDesc, 0, sizeof(viewDesc));
        viewDesc.Format                          = tex.desc.Format;
        viewDesc.ViewDimension                   = D3D_SRV_DIMENSION_TEXTURE2D;
        viewDesc.Texture2D.MostDetailedMip       = 0;
        viewDesc.Texture2D.MipLevels             = -1;
        if (FAILED(device->CreateShaderResourceView((ID3D11Resource*)tex.ptr, &viewDesc, &tex.view)))
            return false;

        if (render) {
            if (FAILED(device->CreateRenderTargetView((ID3D11Resource*)tex.ptr, nullptr, &tex.rtView)))
                return false;

        } else if (useStaging) {
            D3D11_TEXTURE2D_DESC desc = tex.desc;
            desc.BindFlags = 0;
            desc.MiscFlags = 0;
            desc.Usage = D3D11_USAGE_STAGING;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            if (FAILED(device->CreateTexture2D(&desc, nullptr, &tex.staging)))
                return false;
        }

        tex.size = {(float)tex.desc.Width, (float)tex.desc.Height, 1.0f / float(tex.desc.Width), 1.0f / float(tex.desc.Height)};
        return true;
    }

    static auto releaseShader(D3DShader& shader) -> void {
        dxRelease(shader.layout)
        dxRelease(shader.vs)
        dxRelease(shader.ps)
        dxRelease(shader.gs)
        dxRelease(shader.psCode)
        dxRelease(shader.vsCode)
        shader.reflection.clear();
    }

    static auto releaseTexture(D3DTexture& tex) -> void {
        std::memset(&tex.desc, 0, sizeof(tex.desc));
        dxRelease(tex.view)
        dxRelease(tex.rtView)
        dxRelease(tex.staging)
        dxRelease(tex.ptr)
    }

    static auto releaseProgram(D3DProgram& prg) -> void {
        releaseShader(prg.shader);
        releaseTexture(prg.renderTarget);
        releaseTexture(prg.feedbackTarget);

        for (int b = 0; b < SemanticBuffer::Max; b++) {
            dxRelease(prg.buffers[b])
        }

        prg.semanticTextures.clear();
        prg.semanticBuffer[SemanticBuffer::Ubo].variables.clear();
        prg.semanticBuffer[SemanticBuffer::Push].variables.clear();

        prg.semanticBuffer[SemanticBuffer::Ubo].mask = 0;
        prg.semanticBuffer[SemanticBuffer::Push].mask = 0;
    }

    static auto translate(D3D_FEATURE_LEVEL& featureLevel, std::string& code, std::string& out, bool isFragment) -> bool {
        GLSlang glSlang;
        std::string error;
        std::vector<unsigned> spirv;
        spirv_cross::CompilerHLSL* compiler = nullptr;
        SpirvReflection reflection;
        bool success = isFragment ? glSlang.compileFragment(code, spirv, error) : glSlang.compileVertex(code, spirv, error);

        if (!success) {
            out = "SLANG Shader to SPIRV conversion error:\n" + error;
            return false;
        }

        try {
            compiler = new spirv_cross::CompilerHLSL(spirv);
            spirv_cross::ShaderResources resources = compiler->get_shader_resources();
            reflection.preProcess( *compiler, resources );

            spirv_cross::CompilerHLSL::Options options;
            options.shader_model = featureLevel >= D3D_FEATURE_LEVEL_11_0 ? 50 : 40;
            compiler->set_hlsl_options(options);
            out = compiler->compile();
        } catch (const std::exception& e) {
            error = e.what();
            out = "SPIRV Shader to HLSL conversion error:\n" + error;
            if (compiler) delete compiler;
            return false;
        }
        if (compiler) delete compiler;
        return true;
    }
};

}
