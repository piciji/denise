
#pragma once

namespace DRIVER {
    struct Matrix4x4 {
        float data[16];
    };

    struct Float4 {
        float x;
        float y;
        float z;
        float w;
    };

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
        std::string name;
        void* value;
    };

    struct D3DConstantBuffer {
        std::string name;
        unsigned int size;
        unsigned int bindIndex;
        ID3D11Buffer* constantBuffer;
        std::vector<D3DShaderVariable> variables;
    };

    struct D3DTexture {
        std::string ident;
        D3D11_TEXTURE2D_DESC desc;
        ID3D11Texture2D* ptr = nullptr;
        ID3D11Texture2D* staging = nullptr;
        ID3D11ShaderResourceView* view = nullptr;
        ID3D11RenderTargetView* rtView = nullptr;
        Float4 size;
    };

    struct D3DTextureBind {
        std::string ident;
        int index = -1;
        int indexSampler = -1;
        D3DTexture* texture = nullptr;
        ID3D11SamplerState* sampler;
        ShaderPreset::Filter filter;
        ShaderPreset::WrapMode wrap;
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
        std::string ident;
        DXGI_FORMAT format;
        bool mipmap = false;
        std::vector<D3DTextureBind> bindTextures;
        D3DTexture renderTarget;
        D3DTexture cropTarget;
        std::string src;
        unsigned passId;

        ShaderPreset::ScaleType scaleTypeX;
        ShaderPreset::ScaleType scaleTypeY;
        float scaleX;
        float scaleY;
        unsigned absX;
        unsigned absY;
        ShaderPreset::Filter filter;
        ShaderPreset::WrapMode wrap;
        CropPass crop;
        D3D11_BOX cropBox;
    };

}
