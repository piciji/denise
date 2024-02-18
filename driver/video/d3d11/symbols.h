
#pragma once

namespace DRIVER {

typedef HRESULT (__stdcall *D3D11Create_t)(IDXGIAdapter*,D3D_DRIVER_TYPE,HMODULE,UINT,const D3D_FEATURE_LEVEL*,
                                           UINT,UINT,ID3D11Device**,D3D_FEATURE_LEVEL*,ID3D11DeviceContext**);

typedef HRESULT (__stdcall *D3DCompile_t)(const void *data, SIZE_T data_size, const char *filename,
                                          const D3D_SHADER_MACRO *defines, ID3DInclude *include, const char *entrypoint,
                                          const char *target, UINT sflags, UINT eflags, ID3DBlob **shader, ID3DBlob **error_messages);

typedef HRESULT (__stdcall *D3DReflect_t)(const void *data, SIZE_T data_size, REFIID riid, void **reflector);

typedef HRESULT (__stdcall *D3DCreateDXGIFactory1_t)(REFIID riid, void **factory);

typedef HRESULT (__stdcall *D3DCreateBlob_t)(SIZE_T Size, ID3DBlob **ppBlob);

struct D3D11Symbols {
    HMODULE library = nullptr;
    HMODULE libraryCompiler = nullptr;
    HMODULE libraryDXGI = nullptr;

    D3D11Create_t D3D11Create = nullptr;
    D3DCompile_t D3DCompile = nullptr;
    D3DReflect_t D3DReflect = nullptr;
    D3DCreateDXGIFactory1_t CreateDXGIFactory1 = nullptr;
    D3DCreateBlob_t D3DCreateBlob = nullptr;

    auto initializeSymbols() -> bool {
        library = LoadLibraryA("d3d11.dll");
        if (!library)
            return false;

        libraryDXGI = LoadLibraryA("dxgi.dll");
        if (!libraryDXGI)
            return false;

        static const char* d3dCompilerLookup[] = {
                "d3dcompiler_47.dll", "d3dcompiler_46.dll", "d3dcompiler_45.dll", "d3dcompiler_44.dll",
                "d3dcompiler_43.dll", "d3dcompiler_42.dll", "d3dcompiler_41.dll", "d3dcompiler_40.dll"};

        for(auto d3dCompiler : d3dCompilerLookup) {
            libraryCompiler = LoadLibraryA(d3dCompiler);
            if (libraryCompiler)
                break;
        }

        if (!libraryCompiler)
            return false;

        D3D11Create = (D3D11Create_t)GetProcAddress(library, "D3D11CreateDevice");
        if (!D3D11Create)
            return false;

        D3DCompile = (D3DCompile_t)GetProcAddress(libraryCompiler, "D3DCompile");
        if (!D3DCompile)
            return false;

        D3DReflect = (D3DReflect_t)GetProcAddress(libraryCompiler, "D3DReflect");
        if (!D3DReflect)
            return false;

        D3DCreateBlob = (D3DCreateBlob_t)GetProcAddress(libraryCompiler, "D3DCreateBlob");
        if (!D3DCreateBlob)
            return false;

        CreateDXGIFactory1 = (D3DCreateDXGIFactory1_t)GetProcAddress(libraryDXGI, "CreateDXGIFactory1");
        if (!CreateDXGIFactory1)
            return false;

        return true;
    }

    virtual ~D3D11Symbols() {
        if (libraryCompiler)
            FreeLibrary(libraryCompiler);
        if (libraryDXGI)
            FreeLibrary(libraryDXGI);
        if (library)
            FreeLibrary(library);
    }
};

}
