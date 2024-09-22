
#pragma once

#include "../freetype.h"

namespace DRIVER {

    struct D3d9ScreenText : Freetype {
        LPDIRECT3DTEXTURE9 texture;
        LPDIRECT3DDEVICE9 lpD3DDevice;
        LPDIRECT3DVERTEXBUFFER9 vertexBuffer;
        bool linear;

        unsigned texStorageWidth;
        unsigned texStorageHeight;

        D3d9ScreenText() {
            texture = 0;
            linear = false;
            texStorageWidth = 0;
            texStorageHeight = 0;
        }

        ~D3d9ScreenText() {
            term();
        }

        auto init(LPDIRECT3DDEVICE9 _lpD3DDevice, LPDIRECT3DVERTEXBUFFER9 _vertexBuffer) -> void {
            term();
            lpD3DDevice = _lpD3DDevice;
            vertexBuffer = _vertexBuffer;
            ftUpdated |= 8;
            ftInitialized = true;
        }

        auto term() -> void {
            dxRelease(texture);
            texStorageWidth = 0;
            texStorageHeight = 0;
            ftInitialized = false;
        }

        auto showText(Viewport& viewport) -> void {
            if (ftUpdated)
                ftProcessUpdates(viewport);

            if (!ftTextBuffer || !texture)
                return;

            if (ftTs) // animations
                if (ftHandleAnimation(viewport))
                    return;

            lpD3DDevice->SetTexture(0, texture);
            lpD3DDevice->SetStreamSource(0, vertexBuffer, 0, sizeof (d3d9vertex));

            if (linear) {
                lpD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
                lpD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
                lpD3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
                lpD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
                lpD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
            } else {
                lpD3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
            }
        }

        auto ftCalcCoords(Viewport& viewport, float fade = 0.0) -> void {
            LPDIRECT3DVERTEXBUFFER9* vertexPtr;
            d3d9vertex vertex[4];

            unsigned texX = viewport.x;
            unsigned texY = viewport.y;

            unsigned marginHorizontal = (float)viewport.width * ftMarginHorizontal * 0.5 + 0.5f;
            unsigned marginVertical;

            if (ftMarginVertical < 0.0f)
                marginVertical = marginHorizontal;
            else
                marginVertical = (float)viewport.height * ftMarginVertical * 0.5 + 0.5f;

            if (viewport.width >= ftTotalWidth) {
                if (ftPosition & 1) { // center
                    texX += (viewport.width - ftTotalWidth) / 2;
                } else if(ftPosition & 2) { // right
                    texX += viewport.width - ftTotalWidth - marginHorizontal;
                } else
                    texX += marginHorizontal;
            } else
                texX = 0;

            if (viewport.height >= ftTotalHeight) {
                int _fade = (float)viewport.height * fade * 0.5;

                if (ftPosition & 4) { // bottom
                    texY += viewport.height - ftTotalHeight - marginVertical - _fade;
                } else
                    texY += marginVertical + _fade;
            } else
                texY = 0;

            vertex[0].x = vertex[2].x = ((float) texX - 0.5f);
            vertex[1].x = vertex[3].x = ((float) texX + (float) ftTotalWidth - 0.5f);
            vertex[0].y = vertex[1].y = ((float) texY - 0.5f);
            vertex[2].y = vertex[3].y = ((float) texY + (float) ftTotalHeight - 0.5f);

            vertex[0].z = vertex[1].z = 1.0;
            vertex[2].z = vertex[3].z = 1.0;
            vertex[0].rhw = vertex[1].rhw = 1.0;
            vertex[2].rhw = vertex[3].rhw = 1.0;

            vertex[0].u = vertex[2].u = 0.0f;
            vertex[1].u = vertex[3].u = ((float) (ftTotalWidth) - 0.5f) / (float) texStorageWidth;
            vertex[0].v = vertex[1].v = 0.0f;
            vertex[2].v = vertex[3].v = ((float) (ftTotalHeight) - 0.5f) / (float) texStorageHeight;

            vertexBuffer->Lock(0, sizeof (d3d9vertex) * 4, (void**) &vertexPtr, 0);
            std::memcpy(vertexPtr, vertex, sizeof (d3d9vertex) * 4);
            vertexBuffer->Unlock();
        }

        auto ftBuildTexture(std::string text, bool keepOldSize = false) -> void {
            if (!ftBuildText(text, keepOldSize))
                return;

            if (!texture || !keepOldSize) {
                unsigned _texStorageWidth = roundUpPowerOfTwo( ftTotalWidth );
                unsigned _texStorageHeight = roundUpPowerOfTwo( ftTotalHeight );

                if (!texture || (_texStorageWidth != texStorageWidth) || (_texStorageHeight != texStorageHeight)) {
                    texStorageWidth = _texStorageWidth;
                    texStorageHeight = _texStorageHeight;

                    if(texture)
                        texture->Release();

                    lpD3DDevice->CreateTexture( texStorageWidth, texStorageHeight, 1, 0, D3DFMT_A8R8G8B8,
                        static_cast<D3DPOOL> (D3DPOOL_MANAGED), &texture, nullptr);
                }
            }
        }

        auto ftSetColor(FtColNorm& _colNorm, FtColNorm& _colBgNorm) -> void {
            if (!texture)
                return;
            LPDIRECT3DSURFACE9 surface;
            texture->GetSurfaceLevel(0, &surface);
            D3DLOCKED_RECT d3dlr;
            uint8_t font;

            if (surface) {
                surface->LockRect(&d3dlr, 0, D3DLOCK_NOSYSLOCK | D3DLOCK_DISCARD);
                unsigned rgb = ((uint8_t)(_colNorm.r * 255.0f) << 16) | ((uint8_t)(_colNorm.g * 255.0f) << 8) | ((uint8_t)(_colNorm.b * 255.0f) << 0);

                for(int h = 0; h < ftTotalHeight; h++) {
                    uint8_t* src = ftTextBuffer + h * ftTotalWidth;
                    uint32_t* dest = (uint32_t*)((uint8_t*)d3dlr.pBits + h * d3dlr.Pitch);

                    if (_colBgNorm.a == 0.0) {
                        for(int w = 0; w < ftTotalWidth; w++) {
                            font = *src++;
                            *dest++ = rgb | ((uint8_t)((float)font * _colNorm.a) << 24);
                        }
                    } else {
                        for(int w = 0; w < ftTotalWidth; w++)
                            *dest++ = lerp(_colNorm, _colBgNorm, *src++);
                    }
                }

                surface->UnlockRect();
                dxRelease(surface);
            }
        }

        auto lerp(FtColNorm& fg, FtColNorm& bg, uint8_t alpha) -> unsigned {
            FtColNorm result;
            float a = (float)alpha / 255.0f;

            result.r = fg.r * a + bg.r * (1.0f - a);
            result.g = fg.g * a + bg.g * (1.0f - a);
            result.b = fg.b * a + bg.b * (1.0f - a);

            return ((uint8_t)(bg.a * 255.0f) << 24) | ((uint8_t)(result.r * 255.0f) << 16) | ((uint8_t)(result.g * 255.0f) << 8) | ((uint8_t)(result.b * 255.0f) << 0);
        }
    };
}