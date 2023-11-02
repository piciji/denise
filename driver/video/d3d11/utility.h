
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

    static auto _format(const std::string& format) -> DXGI_FORMAT {
        if(format == "r32i"   ) return DXGI_FORMAT_R32_SINT;
        if(format == "r32ui"  ) return DXGI_FORMAT_R32_UINT;
        if(format == "rgba8"  ) return DXGI_FORMAT_R8G8B8A8_UNORM;
        if(format == "rgb10a2") return DXGI_FORMAT_R10G10B10A2_UNORM;
        if(format == "rgba16" ) return DXGI_FORMAT_R16G16B16A16_UINT;
        if(format == "rgba16f") return DXGI_FORMAT_R16G16B16A16_FLOAT;
        if(format == "rgba32f") return DXGI_FORMAT_R32G32B32A32_FLOAT;
        if(format == "rgba32i") return DXGI_FORMAT_R32G32B32A32_SINT;
        if(format == "rgb32f") return DXGI_FORMAT_R32G32B32_FLOAT;
        if(format == "rgb32i") return DXGI_FORMAT_R32G32B32_SINT;
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    }

    static auto buildProgram(ID3D11Device* device, D3DProgram& program, ShaderPass& pass) -> bool {
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

        D3D11_INPUT_ELEMENT_DESC descShader[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(D3DVertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(D3DVertex, texcoord), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(D3DVertex, color), D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        if (!createShader(device, pass.fragment, "PS", "VS", "", descShader, countof(descShader), &program.shader)) {
            pass.error = program.shader.error;
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

        if (psCode)
            if (FAILED(device->CreatePixelShader( psCode->GetBufferPointer(), psCode->GetBufferSize(), nullptr, &out->ps)))
                msg = (const char*)-1;

        if (vsCode) {
            LPVOID bufPtr  = vsCode->GetBufferPointer();
            SIZE_T bufSize = vsCode->GetBufferSize();
            if (FAILED(device->CreateVertexShader(bufPtr, bufSize, nullptr, &out->vs)))
                msg = (const char*)-2;

            if (inputElementDescs)
                if (FAILED(device->CreateInputLayout(inputElementDescs, numElements, bufPtr, bufSize, &out->layout)))
                    msg = (const char*)-3;
        }

        if (gsCode)
            if (FAILED(device->CreateGeometryShader(gsCode->GetBufferPointer(), gsCode->GetBufferSize(), nullptr, &out->gs)))
                msg = (const char*)-4;

        out->error = msg ? msg : "";
        dxRelease(vsCode)
        dxRelease(psCode)
        dxRelease(gsCode)
        dxRelease(error)

        return msg == nullptr;
    }

    static auto releaseShader(D3DShader& shader) -> void {
        dxRelease(shader.layout)
        dxRelease(shader.vs)
        dxRelease(shader.ps)
        dxRelease(shader.gs)
    }

    static auto releaseTexture(D3DTexture& tex) -> void {
        std::memset(&tex.desc, 0, sizeof(tex.desc));
        dxRelease(tex.view)
        dxRelease(tex.staging)
        dxRelease(tex.ptr)
    }
};

}
