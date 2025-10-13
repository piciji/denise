
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
    };

    struct D3DTexture {
        D3D11_TEXTURE2D_DESC desc;
        ID3D11Texture2D* ptr = nullptr;
        ID3D11Texture2D* staging = nullptr;
        ID3D11ShaderResourceView* view = nullptr;
        ID3D11RenderTargetView* rtView = nullptr;
        Float4 size;
    };

    struct D3DShader {
        ID3D11VertexShader* vs = nullptr;
        ID3D11PixelShader* ps = nullptr;
        ID3D11GeometryShader* gs = nullptr;
        ID3D11InputLayout* layout = nullptr;
        std::string error = "";
        SpirvReflection reflection;

        ID3DBlob* psCode = nullptr;
        ID3DBlob* vsCode = nullptr;
    };

    struct D3DProgram {
        bool inUse;
        D3DShader shader;
        std::string ident;
        DXGI_FORMAT format;

        bool mipmap = false;
        bool feedback = false;
        unsigned frameCount = 0;
        unsigned frameModulo = 0;

        D3DTexture renderTarget;
        D3DTexture feedbackTarget;
        std::string codeFragment;
        std::string codeVertex;
        std::vector<SemanticTexture> semanticTextures;
        SemanticBuffer semanticBuffer[2];
        ID3D11Buffer* buffers[2] = {nullptr};

        ShaderPreset::ScaleType scaleTypeX;
        ShaderPreset::ScaleType scaleTypeY;
        float scaleX;
        float scaleY;
        unsigned absX;
        unsigned absY;
        ShaderPreset::Filter filter;
        ShaderPreset::WrapMode wrap;
    };

}
