
#pragma once

typedef int WINBOOL;

namespace DRIVER {

typedef IDirect3D9*(__stdcall *D3D9Create_t)(UINT);
typedef HRESULT (__stdcall *D3D9CreateFont_t)(struct IDirect3DDevice9 *device, INT height, UINT width, UINT weight,
                                              UINT miplevels, WINBOOL italic, DWORD charset, DWORD precision, DWORD quality, DWORD pitchandfamily,
                                              const WCHAR *facename, struct ID3DXFont **font);

struct D3D9Symbols {
    HMODULE library = nullptr;
    HMODULE libraryX = nullptr;

    D3D9Create_t D3D9Create = nullptr;
    D3D9CreateFont_t D3D9CreateFont = nullptr;

    auto initializeSymbols() -> bool {
        library = LoadLibraryA("d3d9.dll");        

        if (!library)
            return false;

        libraryX = LoadLibraryA("d3dx9_43.dll");

        D3D9Create = (D3D9Create_t)GetProcAddress(library, "Direct3DCreate9");
        if (!D3D9Create)
            return false;

        if (libraryX)
            D3D9CreateFont = (D3D9CreateFont_t)GetProcAddress(libraryX, "D3DXCreateFontW");

        return true;
    }

    virtual ~D3D9Symbols() {
        if (library)
            FreeLibrary(library);
        if (libraryX)
            FreeLibrary(libraryX);
    }
};

}
