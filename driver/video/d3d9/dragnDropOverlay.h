
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

            unsigned _col;
            for(int h = 0; h < texHeight; h++) {
                uint32_t* src = (uint32_t*)(buffer + h * texWidth * 4);
                uint32_t* dest = (uint32_t*)((uint8_t*)d3dlr.pBits + h * d3dlr.Pitch);

                for(int w = 0; w < texWidth; w++) {
                    _col = *src++;
                    *dest++ = (_col & 0xff00ff00) | ((_col >> 16) & 0xff) | ((_col & 0xff) << 16);
                }
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

        updateBuffer();
    }

    auto updateCoord(Viewport& viewport, LPDIRECT3DVERTEXBUFFER9& vertexBuffer) -> void {
        // todo: don't refresh this each frame
        LPDIRECT3DVERTEXBUFFER9* vertexPtr;
        d3d9vertex vertex[4];

        vertex[0].x = vertex[2].x = ((float) (viewport.x + texX) - 0.5f);
        vertex[1].x = vertex[3].x = ((float) (viewport.x + texX) + (float) texWidth - 0.5f);
        vertex[0].y = vertex[1].y = ((float) (viewport.y + texY) - 0.5f);
        vertex[2].y = vertex[3].y = ((float) (viewport.y + texY) + (float) texHeight - 0.5f);

        vertex[0].z = vertex[1].z = 1.0;
        vertex[2].z = vertex[3].z = 1.0;
        vertex[0].rhw = vertex[1].rhw = 1.0;
        vertex[2].rhw = vertex[3].rhw = 1.0;

        vertex[0].u = vertex[2].u = 0.0f;
        vertex[1].u = vertex[3].u = ((float) (texWidth) - 0.5f) / (float) texStorageWidth;
        vertex[0].v = vertex[1].v = 0.0f;
        vertex[2].v = vertex[3].v = ((float) (texHeight) - 0.5f) / (float) texStorageHeight;

        vertexBuffer->Lock(0, sizeof (d3d9vertex) * 4, (void**) &vertexPtr, 0);
        std::memcpy(vertexPtr, vertex, sizeof (d3d9vertex) * 4);
        vertexBuffer->Unlock();
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
};