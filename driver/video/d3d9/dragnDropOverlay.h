
struct D3d9DragndropOverlay : DragndropOverlay {

    D3d9DragndropOverlay() : DragndropOverlay() {
        surface = 0;
        texture = 0;
        texStorageWidth = 0;
        texStorageHeight = 0;
    }

    LPDIRECT3DSURFACE9 surface;
    LPDIRECT3DTEXTURE9 texture;
    LPDIRECT3DDEVICE9 lpD3DDevice;

    unsigned texStorageWidth;
    unsigned texStorageHeight;

    auto show(Viewport& _viewport) -> void {
        update(_viewport);

        if (!buffer)
            return;

        updateAlpha();

        lpD3DDevice->SetTexture(0, texture);
    }

    auto updateBuffer() -> void {
        if (!texture)
            return;
        texture->GetSurfaceLevel(0, &surface);
        D3DLOCKED_RECT d3dlr;
        if (surface) {
            surface->LockRect(&d3dlr, 0, D3DLOCK_NOSYSLOCK | D3DLOCK_DISCARD);

            for(int h = 0; h < texHeight; h++) {
                uint32_t* src = (uint32_t*)(buffer + h * texWidth * 4);
                uint32_t* dest = (uint32_t*)((uint8_t*)d3dlr.pBits + h * d3dlr.Pitch);

                for(int w = 0; w < texWidth; w++)
                    *dest++ = *src++;
            }

            surface->UnlockRect();
            dxRelease(surface);
        }
    }

    auto buildTexture(unsigned width, unsigned height) -> void {
        if (!initialized)
            return;

        if(texture)
            texture->Release();

        D3DCAPS9 d3dcaps;
        lpD3DDevice->GetDeviceCaps(&d3dcaps);
        texStorageWidth = roundUpPowerOfTwo( width );
        texStorageHeight = roundUpPowerOfTwo( height );

        if(d3dcaps.MaxTextureWidth < texStorageWidth)
            texStorageWidth = d3dcaps.MaxTextureWidth;
        if (d3dcaps.MaxTextureHeight < texStorageHeight)
            texStorageHeight = d3dcaps.MaxTextureHeight;

        lpD3DDevice->CreateTexture( texStorageWidth, texStorageHeight, 1, 0, D3DFMT_A8R8G8B8,
                                    static_cast<D3DPOOL> (D3DPOOL_MANAGED), &texture, nullptr);

        texWidth = width;
        texHeight = height;

        updateBuffer();
    }

    auto term() -> void {
        dxRelease(surface);
        dxRelease(texture);

        DragndropOverlay::term();
        initialized = false;
    }

    auto init(LPDIRECT3DDEVICE9 lpD3DDevice) -> bool {
        term();

        this->lpD3DDevice = lpD3DDevice;
        viewport.width = 0;
        viewport.height = 0;

        return initialized = true;
    }

    auto setDragnDropOverlay(uint8_t* _data, unsigned _width, unsigned _height, unsigned line) -> void {
        if (line >= MAX_LINES)
            line = 0;

        Image& bitmap = lines[line].bitmap;
        bitmap.setData(_data, _width, _height, true);
    }
};