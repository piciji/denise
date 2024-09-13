
#pragma once

#include "../freetype.h"

namespace DRIVER {

    struct D3d9Freetype {
        LPDIRECT3DTEXTURE9 texture;
        LPDIRECT3DDEVICE9 lpD3DDevice;

        unsigned texStorageWidth;
        unsigned texStorageHeight;

        D3d9Freetype() {
            texture = 0;
            texStorageWidth = 0;
            texStorageHeight = 0;
        }

        ~D3d9Freetype() {
            term();
        }

        Freetype ft;

        auto term() -> void {
            reset();
            ft.term();
        }

        auto setDevice(LPDIRECT3DDEVICE9& _lpD3DDevice) -> void {
            lpD3DDevice = _lpD3DDevice;
        }

        auto reset() -> void {
            dxRelease(texture);
            texStorageWidth = 0;
            texStorageHeight = 0;
        }

        auto init() -> bool {
            if (!ft.init())
                return false;

            return true;
        }

        auto setFontSize(int value) -> void {
            ft.setFontSize(value);
        }

        auto updateCoord(Viewport& viewport, LPDIRECT3DVERTEXBUFFER9& vertexBuffer) -> void {
            LPDIRECT3DVERTEXBUFFER9* vertexPtr;
            d3d9vertex vertex[4];

            unsigned texX = viewport.x;
            unsigned texY = viewport.y;

            if ((viewport.width - ft.totalWidth) > 5)
                texX += viewport.width - ft.totalWidth - 5;

            if ((viewport.height - ft.totalHeight) > 5)
                texY += viewport.height - ft.totalHeight - 5;

            vertex[0].x = vertex[2].x = ((float) texX - 0.5f);
            vertex[1].x = vertex[3].x = ((float) texX + (float) ft.totalWidth - 0.5f);
            vertex[0].y = vertex[1].y = ((float) texY - 0.5f);
            vertex[2].y = vertex[3].y = ((float) texY + (float) ft.totalHeight - 0.5f);

            vertex[0].z = vertex[1].z = 1.0;
            vertex[2].z = vertex[3].z = 1.0;
            vertex[0].rhw = vertex[1].rhw = 1.0;
            vertex[2].rhw = vertex[3].rhw = 1.0;

            vertex[0].u = vertex[2].u = 0.0f;
            vertex[1].u = vertex[3].u = ((float) (ft.totalWidth) - 0.5f) / (float) texStorageWidth;
            vertex[0].v = vertex[1].v = 0.0f;
            vertex[2].v = vertex[3].v = ((float) (ft.totalHeight) - 0.5f) / (float) texStorageHeight;

            vertexBuffer->Lock(0, sizeof (d3d9vertex) * 4, (void**) &vertexPtr, 0);
            std::memcpy(vertexPtr, vertex, sizeof (d3d9vertex) * 4);
            vertexBuffer->Unlock();
        }

        auto buildTexture(std::string& text, bool critical) -> void {
            if (!ft.buildTexture(text))
                return;

            unsigned _texStorageWidth = roundUpPowerOfTwo( ft.totalWidth );
            unsigned _texStorageHeight = roundUpPowerOfTwo( ft.totalHeight );

            if (!texture || (_texStorageWidth != texStorageWidth) || (_texStorageHeight != texStorageHeight)) {
                texStorageWidth = _texStorageWidth;
                texStorageHeight = _texStorageHeight;

                if(texture)
                    texture->Release();

                // D3DCAPS9 d3dcaps;
                // lpD3DDevice->GetDeviceCaps(&d3dcaps);
                //
                // if(d3dcaps.MaxTextureWidth < texStorageWidth)
                //     texStorageWidth = d3dcaps.MaxTextureWidth;
                // if (d3dcaps.MaxTextureHeight < texStorageHeight)
                //     texStorageHeight = d3dcaps.MaxTextureHeight;

                HRESULT hr = lpD3DDevice->CreateTexture( texStorageWidth, texStorageHeight, 1, 0, D3DFMT_A8R8G8B8,
                                            static_cast<D3DPOOL> (D3DPOOL_MANAGED), &texture, nullptr);

                if (!SUCCEEDED(hr))
                    return;
            }

            if (!texture)
                return;

            unsigned color;
            if (critical)
                color = D3DCOLOR_ARGB(0, 155, 0, 0);
            else
                color = D3DCOLOR_ARGB(0, 255, 255, 255);

            LPDIRECT3DSURFACE9 surface;
            texture->GetSurfaceLevel(0, &surface);
            D3DLOCKED_RECT d3dlr;
            if (surface) {
                surface->LockRect(&d3dlr, 0, D3DLOCK_NOSYSLOCK | D3DLOCK_DISCARD);

                for(int h = 0; h < ft.totalHeight; h++) {
                    uint8_t* src = ft.textBuffer + h * ft.totalWidth;
                    uint32_t* dest = (uint32_t*)((uint8_t*)d3dlr.pBits + h * d3dlr.Pitch);

                    if (critical) {
                        for(int w = 0; w < ft.totalWidth; w++)
                            *dest++ = color | (*src++ << 24);
                    } else {
                        for(int w = 0; w < ft.totalWidth; w++)
                            *dest++ = color | (((*src++ * 8) / 10) << 24);
                    }
                }

                surface->UnlockRect();
                dxRelease(surface);
            }
        }
    };
}