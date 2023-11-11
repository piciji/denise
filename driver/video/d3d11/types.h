
#pragma once

namespace DRIVER {

    struct SwapChain {
        UINT flags = 0;
        HANDLE frameLatency = nullptr;
        IDXGISwapChain2* ptr = nullptr;
    };

    struct D3DVertex {
        float position[2];
        float texcoord[2];
        float color[4];
    };

    struct D3DShaderVariable {
        unsigned int byteOffset;
        unsigned int size;
        unsigned int constantBufferIndex;
        std::string name;
    };

    struct D3DConstantBuffer {
        std::string name;
        unsigned int size;
        unsigned int bindIndex;
        ID3D11Buffer* constantBuffer;
        unsigned char* localBuffer;
        std::vector<D3DShaderVariable> variables;
    };

    struct D3DTextureBind {
        int index = -1;
        int indexSampler = -1;
    };

    struct D3DTexture {
        std::string attribute;
        D3D11_TEXTURE2D_DESC desc;
        ID3D11Texture2D* ptr = nullptr;
        ID3D11Texture2D* staging = nullptr;
        ID3D11ShaderResourceView* view = nullptr;
        ID3D11RenderTargetView* rtView = nullptr;
        ID3D11SamplerState* sampler = nullptr;
        D3DTextureBind bind;
    };

    struct D3DShader {
        ID3D11VertexShader* vs = nullptr;
        ID3D11PixelShader* ps = nullptr;
        ID3D11GeometryShader* gs = nullptr;
        ID3D11InputLayout* layout = nullptr;
        std::string error = "";
        ID3D11ShaderReflection* reflPS = nullptr;

        D3DConstantBuffer* constantBuffers = nullptr;
        int constantBufferCount = 0;
    };

    struct D3DProgram {
        D3DShader shader;
        CropPass crop;
        std::string ident;
        int filter;
        int wrap;
        DXGI_FORMAT format;
        float relativeWidth = 0;
        float relativeHeight = 0;
        bool mipmap = false;
        std::vector<D3DTexture> textures;
        D3DTextureBind bindTexture;
        D3DTextureBind bindPrevTexture;
        D3DTexture renderTarget;
    };

}
