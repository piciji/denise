
#pragma once

#define D3D9VERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)

struct d3d9vertex {
    float x, y, z, rhw; //screen coords
    unsigned color; //diffuse color
    float u, v; //texture coords
};

namespace DRIVER {

typedef IDirect3D9*(__stdcall *D3D9Create_t)(UINT);

struct D3D9Symbols {
    HMODULE library = nullptr;

    D3D9Create_t D3D9Create = nullptr;

    auto initializeSymbols() -> bool {
        library = LoadLibraryA("d3d9.dll");        

        if (!library)
            return false;

        D3D9Create = (D3D9Create_t)GetProcAddress(library, "Direct3DCreate9");

        if (!D3D9Create)
            return false;

        return true;
    }

    virtual ~D3D9Symbols() {
        if (library)
            FreeLibrary(library);
    }
};

}
