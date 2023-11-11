
#pragma once

namespace DRIVER {

struct D3D11Utility {

    static auto _filter(const std::string& filter) -> int {
        if(filter == "nearest") return 0;
        if(filter == "linear" ) return 1;
        return 1;
    }

    static auto _wrap(const std::string& wrap) -> int {
        if(wrap == "border") return 0;
        if(wrap == "edge"  ) return 1;
        if(wrap == "repeat") return 2;
        if(wrap == "mirror") return 3;
        return 0;
    }

    static auto _format(const std::string& format = "") -> DXGI_FORMAT {
        if(format == "") return DXGI_FORMAT_B8G8R8A8_UNORM;
        if(format == "r32i"   ) return DXGI_FORMAT_R32_SINT;
        if(format == "r32ui"  ) return DXGI_FORMAT_R32_UINT;
        if(format == "rgba8"  ) return DXGI_FORMAT_B8G8R8A8_UNORM;
        if(format == "rgb10a2") return DXGI_FORMAT_R10G10B10A2_UNORM;
        if(format == "rgba16" ) return DXGI_FORMAT_R16G16B16A16_UINT;
        if(format == "rgba16f") return DXGI_FORMAT_R16G16B16A16_FLOAT;
        if(format == "rgba32f") return DXGI_FORMAT_R32G32B32A32_FLOAT;
        if(format == "rgba32i") return DXGI_FORMAT_R32G32B32A32_SINT;
        if(format == "rgb32f") return DXGI_FORMAT_R32G32B32_FLOAT;
        if(format == "rgb32i") return DXGI_FORMAT_R32G32B32_SINT;
        return DXGI_FORMAT_B8G8R8A8_UNORM;
    }

    auto buildProgram(ID3D11Device* device, D3DProgram& program, ShaderPass& pass) -> bool {
        program.ident = pass.ident;
        program.filter = _filter( pass.filter );
        program.wrap = _wrap( pass.wrap );
        program.format = _format( pass.format );
        program.mipmap = pass.mipmap;
        program.crop.set( pass.crop );
        program.relativeWidth =  program.relativeHeight = 0;
        if (pass.relativeWidth) program.relativeWidth = float(pass.relativeWidth) / 100.0;
        if (pass.relativeHeight) program.relativeHeight = float(pass.relativeHeight) / 100.0;
        if (program.crop.active)
            return true;

        static const D3D11_INPUT_ELEMENT_DESC desc[] = {
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(D3DVertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(D3DVertex, texcoord), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        if (!createShader(device, pass.fragment, "PS", "VS", "", desc, countof(desc), &program.shader)) {
            pass.error = program.shader.error;
            return false;
        }

        if (!createPSConsts(device, program)) {
            return false;
        }

        return true;
    }

    static auto createShader( ID3D11Device* device, const std::string& data, const std::string psEntry, const std::string vsEntry, const std::string gsEntry,
                       const D3D11_INPUT_ELEMENT_DESC* inputElementDescs, unsigned numElements, D3DShader* out) -> bool {

        ID3DBlob* error = nullptr;
        const char* msg = nullptr;

        ID3DBlob* psCode = nullptr;
        ID3DBlob* vsCode = nullptr;
        ID3DBlob* gsCode = nullptr;

        if (!psEntry.empty() && FAILED(D3DCompile(data.c_str(), data.size(), nullptr, nullptr, nullptr, psEntry.c_str(), "ps_4_0", 0, 0, &psCode, &error)))
            msg = (const char*)error->GetBufferPointer();

        if (!vsEntry.empty() && FAILED(D3DCompile(data.c_str(), data.size(), nullptr, nullptr, nullptr, vsEntry.c_str(), "vs_4_0", 0, 0, &vsCode, &error)))
            msg = (const char*)error->GetBufferPointer();

        if (!gsEntry.empty() && FAILED(D3DCompile(data.c_str(), data.size(), nullptr, nullptr, nullptr, gsEntry.c_str(), "gs_4_0", 0, 0, &gsCode, &error)))
            msg = (const char*)error->GetBufferPointer();

        if (psCode) {
            if (FAILED(device->CreatePixelShader( psCode->GetBufferPointer(), psCode->GetBufferSize(), nullptr, &out->ps)))
                msg = "can't create pixel shader";
            else
                D3DReflect( psCode->GetBufferPointer(), psCode->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&out->reflPS);
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
        dxRelease(vsCode)
        dxRelease(psCode)
        dxRelease(gsCode)
        dxRelease(error)

        return msg == nullptr;
    }

    static auto createPSConsts(ID3D11Device* device, D3DProgram& prg ) -> bool {
        D3DShader& shader = prg.shader;

        if (!shader.reflPS)
            return false;

        D3D11_SHADER_DESC shaderDesc;
        if (FAILED(shader.reflPS->GetDesc(&shaderDesc)))
            return false;

        int resourceCount = shaderDesc.BoundResources;
        for (int r = 0; r < resourceCount; r++) {
            D3D11_SHADER_INPUT_BIND_DESC resourceDesc;
            shader.reflPS->GetResourceBindingDesc(r, &resourceDesc);
            bool found = false;

            switch (resourceDesc.Type) {
                case D3D_SIT_TEXTURE: {
                    std::string ident = resourceDesc.Name;
                    if (ident == "t0") {
                        prg.bindTexture.index = resourceDesc.BindPoint;
                        break;
                    } if (ident == "t1") {
                        prg.bindPrevTexture.index = resourceDesc.BindPoint;
                        break;
                    }

                    for(auto& tex : prg.textures) {
                        if (tex.attribute == ident) {
                            tex.bind.index = resourceDesc.BindPoint;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        D3DTexture texture;
                        texture.attribute = ident;
                        texture.bind.index = resourceDesc.BindPoint;
                        prg.textures.push_back(texture);
                    }
                } break;

                case D3D_SIT_SAMPLER: {
                    std::string ident = resourceDesc.Name;
                    ident.pop_back(); // remove "S"

                    if (ident == "t0") {
                        prg.bindTexture.indexSampler = resourceDesc.BindPoint;
                        break;
                    } if (ident == "t1") {
                        prg.bindPrevTexture.indexSampler = resourceDesc.BindPoint;
                        break;
                    }

                    for(auto& tex : prg.textures) {
                        if (tex.attribute == ident) {
                            tex.bind.indexSampler = resourceDesc.BindPoint;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        D3DTexture texture;
                        texture.attribute = ident;
                        texture.bind.indexSampler = resourceDesc.BindPoint;
                        prg.textures.push_back(texture);
                    }
                } break;
            }
        }

        shader.constantBufferCount = shaderDesc.ConstantBuffers;
        shader.constantBuffers = new D3DConstantBuffer[shader.constantBufferCount];

        for (int i = 0; i < shader.constantBufferCount; i++) {
            ID3D11ShaderReflectionConstantBuffer* cb = shader.reflPS->GetConstantBufferByIndex(i);

            D3D11_SHADER_BUFFER_DESC bufferDesc;
            cb->GetDesc(&bufferDesc);

            D3D11_SHADER_INPUT_BIND_DESC bindDesc;
            shader.reflPS->GetResourceBindingDescByName(bufferDesc.Name, &bindDesc);

            shader.constantBuffers[i].bindIndex = bindDesc.BindPoint;
            shader.constantBuffers[i].name = bufferDesc.Name;

            D3D11_BUFFER_DESC newBuffDesc;
            newBuffDesc.Usage = D3D11_USAGE_DEFAULT;
            newBuffDesc.ByteWidth = bufferDesc.Size;
            newBuffDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            newBuffDesc.CPUAccessFlags = 0;
            newBuffDesc.MiscFlags = 0;
            newBuffDesc.StructureByteStride = 0;
            device->CreateBuffer(&newBuffDesc, 0, &shader.constantBuffers[i].constantBuffer);

            shader.constantBuffers[i].size = bufferDesc.Size;
            shader.constantBuffers[i].localBuffer = new unsigned char[bufferDesc.Size];
            ZeroMemory(shader.constantBuffers[i].localBuffer, bufferDesc.Size);

            for (int v = 0; v < bufferDesc.Variables; v++) {
                ID3D11ShaderReflectionVariable* var = cb->GetVariableByIndex(v);

                D3D11_SHADER_VARIABLE_DESC varDesc;
                var->GetDesc(&varDesc);

                D3DShaderVariable varStruct;
                varStruct.constantBufferIndex = i;
                varStruct.byteOffset = varDesc.StartOffset;
                varStruct.size = varDesc.Size;
                varStruct.name = std::string(varDesc.Name);
                shader.constantBuffers[i].variables.push_back(varStruct);
            }
        }

        dxRelease( shader.reflPS )
        return true;
    }

    static auto findConstantVar( D3DShader& shader, std::string name, int size ) -> D3DShaderVariable* {
        for (unsigned int i = 0; i < shader.constantBufferCount; i++) {
            auto& cBuffer = shader.constantBuffers[i];
            for(auto& var : cBuffer.variables) {
                if (var.name == name && var.size == size) {
                    return &var;
                }
            }
        }
        return nullptr;
    }

    auto setConstantInt(D3DShader& shader, std::string name, int data) -> bool {
        return this->setConstantData(shader, name, (void*)(&data), sizeof(int));
    }

    auto setConstantFloat(D3DShader& shader, std::string name, float data) -> bool {
        return this->setConstantData(shader, name, (void*)(&data), sizeof(float));
    }

    auto setConstantFloat4(D3DShader& shader, std::string name, const float data[4]) -> bool {
        return this->setConstantData(shader, name, (void*)(data), sizeof(float) * 4);
    }

    static auto setConstantData(D3DShader& shader, std::string name, const void* data, unsigned int size) -> bool {
        D3DShaderVariable* var = findConstantVar(shader, name, size);
        if (!var)
            return false;

        std::memcpy(shader.constantBuffers[var->constantBufferIndex].localBuffer + var->byteOffset, data, size);

        return true;
    }

    static auto updateConstantData(ID3D11DeviceContext* context, D3DShader& shader, const std::string& name) -> void {
        for (unsigned int i = 0; i < shader.constantBufferCount; i++) {
            auto& cBuffer = shader.constantBuffers[i];
            if (cBuffer.name == name) {
                context->UpdateSubresource( cBuffer.constantBuffer, 0, 0, cBuffer.localBuffer, 0, 0);
                break;
            }
        }
    }

    static auto findRessource( D3DProgram& prg, std::string& attribute ) -> D3DTexture* {
        for (auto& texture : prg.textures) {
            if (texture.attribute == attribute) {
                return &texture;
            }
        }
        return nullptr;
    }

    static auto releaseShader(D3DShader& shader) -> void {
        dxRelease(shader.layout)
        dxRelease(shader.vs)
        dxRelease(shader.ps)
        dxRelease(shader.gs)
        dxRelease(shader.reflPS)

        if (shader.constantBuffers) {
            for (unsigned int i = 0; i < shader.constantBufferCount; i++) {
                dxRelease(shader.constantBuffers[i].constantBuffer)
                delete[] shader.constantBuffers[i].localBuffer;
            }
            delete[] shader.constantBuffers;
        }
        shader.constantBufferCount = 0;
        shader.constantBuffers = nullptr;
    }

    static auto releaseTexture(D3DTexture& tex) -> void {
        std::memset(&tex.desc, 0, sizeof(tex.desc));
        dxRelease(tex.view)
        dxRelease(tex.rtView)
        dxRelease(tex.staging)
        dxRelease(tex.ptr)
    }
};

}
