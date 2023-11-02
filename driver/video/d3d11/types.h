
#pragma once

namespace DRIVER {

    struct D3DVertex {
        float position[2];
        float texcoord[2];
        float color[4];
    };

    struct D3DTexture {
        D3D11_TEXTURE2D_DESC desc;
        ID3D11Texture2D* ptr = nullptr;
        ID3D11Texture2D* staging = nullptr;
        ID3D11ShaderResourceView* view = nullptr;
    };

    struct D3DShader {
        ID3D11VertexShader* vs = nullptr;
        ID3D11PixelShader* ps = nullptr;
        ID3D11GeometryShader* gs = nullptr;
        ID3D11InputLayout* layout = nullptr;
        std::string error = "";
    };

    struct D3DProgram {
        D3DShader shader;
        D3DTexture texture;
        CropPass crop;
        std::string ident;
        int filter;
        int wrap;
        DXGI_FORMAT format;
        float relativeWidth = 0;
        float relativeHeight = 0;
        bool mipmap = false;
    };

}
