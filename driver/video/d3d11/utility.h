
#pragma once

namespace DRIVER {

struct D3D11Utility {

    static auto buildProgram2(D3D11Symbols& symbols, D3D_FEATURE_LEVEL& featureLevel, ID3D11Device* device, D3DProgram& program) -> bool {

        static const D3D11_INPUT_ELEMENT_DESC desc[] = {
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(D3DVertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(D3DVertex, texcoord), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        if (!createShader(symbols, featureLevel, device, program.src, "PS", "VS", "", desc, countof(desc), &program.shader))
            return false;

        if (!createPSConsts(device, program)) {
            return false;
        }

        return true;
    }

    static auto resolveParams(ShaderPreset* preset, std::vector<D3DProgram*>& programs, D3DTexture* orig, float* seed) -> void {
        for(auto& param : preset->params) {
            for(auto p : programs) {
                for(int  c = 0; c < p->shader.constantBufferCount; c++) {
                    D3DConstantBuffer* cBuffer = &p->shader.constantBuffers[c];
                    for(auto& v : cBuffer->variables) {
                        if (v.name == param.id) {
                            v.value = (void*)&param.value;
                            break;
                        }
                    }
                }
            }
        }

        D3DTexture* texture = orig;
        for(auto p : programs) {
            for (unsigned int i = 0; i < p->shader.constantBufferCount; i++) {
                auto& cBuffer = p->shader.constantBuffers[i];

                if (cBuffer.name != "Scene")
                    continue;

                for(auto& v : cBuffer.variables) {
                    if (v.name == "SourceSize") {
                        v.value = (void*)&texture->size;
                    } else if (v.name == "OutputSize") {
                        v.value = (void*)&p->renderTarget.size;
                    } else if (v.name == "OriginalSize") {
                        v.value = (void*)&orig->size;
                    } else if (v.name == "Seed") {
                        v.value = (void*)seed;
                    }
                }
            }
            texture = p->crop.active ? &p->cropTarget : &p->renderTarget;
        }
    }

    static auto createShader( D3D11Symbols& symbols, D3D_FEATURE_LEVEL& featureLevel, ID3D11Device* device, const std::string& data, const std::string psEntry, const std::string vsEntry, const std::string gsEntry,
                       const D3D11_INPUT_ELEMENT_DESC* inputElementDescs, unsigned numElements, D3DShader* out) -> bool {

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
            else
                symbols.D3DReflect( psCode->GetBufferPointer(), psCode->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&out->reflPS);
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

        if (!shader.reflPS) {
            shader.error = "reflection error";
            return false;
        }

        D3D11_SHADER_DESC shaderDesc;
        if (FAILED(shader.reflPS->GetDesc(&shaderDesc))) {
            shader.error = "reflection error";
            return false;
        }

        int resourceCount = shaderDesc.BoundResources;
        for (int r = 0; r < resourceCount; r++) {
            D3D11_SHADER_INPUT_BIND_DESC resourceDesc;
            shader.reflPS->GetResourceBindingDesc(r, &resourceDesc);
            bool found = false;

            switch (resourceDesc.Type) {
                case D3D_SIT_TEXTURE: {
                    std::string ident = resourceDesc.Name;
                    for(auto& bind : prg.bindTextures) {
                        if (bind.ident == ident) {
                            bind.index = resourceDesc.BindPoint;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        D3DTextureBind bind;
                        bind.ident = ident;
                        bind.index = resourceDesc.BindPoint;
                        bind.texture = nullptr;
                        prg.bindTextures.push_back(bind);
                    }
                } break;

                case D3D_SIT_SAMPLER: {
                    std::string ident = resourceDesc.Name;
                    ident.pop_back(); // remove "S"

                    for(auto& bind : prg.bindTextures) {
                        if (bind.ident == ident) {
                            bind.indexSampler = resourceDesc.BindPoint;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        D3DTextureBind bind;
                        bind.ident = ident;
                        bind.indexSampler = resourceDesc.BindPoint;
                        bind.texture = nullptr;
                        prg.bindTextures.push_back(bind);
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
            newBuffDesc.Usage = D3D11_USAGE_DYNAMIC;
            newBuffDesc.ByteWidth = bufferDesc.Size;
            newBuffDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            newBuffDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            newBuffDesc.MiscFlags = 0;
            newBuffDesc.StructureByteStride = 0;
            device->CreateBuffer(&newBuffDesc, 0, &shader.constantBuffers[i].constantBuffer);
            shader.constantBuffers[i].size = bufferDesc.Size;

            for (int v = 0; v < bufferDesc.Variables; v++) {
                ID3D11ShaderReflectionVariable* var = cb->GetVariableByIndex(v);

                D3D11_SHADER_VARIABLE_DESC varDesc;
                var->GetDesc(&varDesc);

                D3DShaderVariable varStruct;
                varStruct.byteOffset = varDesc.StartOffset;
                varStruct.size = varDesc.Size;
                varStruct.name = std::string(varDesc.Name);
                shader.constantBuffers[i].variables.push_back(varStruct);
            }
        }

        dxRelease( shader.reflPS )
        return true;
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
