
#ifdef _WIN32
#define UNICODE
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <cstdint>

#include "DLLoader.h"

namespace Emulator {

auto DLLoader::open() -> bool {
    if (handle)
        return true;

    if (!pathToLib.size())
        return false;

#ifdef _WIN32
    std::wstring wpath = std::wstring(pathToLib.begin(), pathToLib.end());
    handle = LoadLibrary( wpath.c_str() );
#else
    handle = dlopen( pathToLib.c_str(), RTLD_NOW | RTLD_LAZY);
#endif
    if (!handle) {
        return false;
    }

    return true;
}

auto DLLoader::close() -> void {
    if (handle) {
#ifdef _WIN32
        FreeLibrary(reinterpret_cast<HMODULE>( handle));
#else
        dlclose(handle);
#endif
        handle = nullptr;
    }
    calls.clear();
    markInitialized = false;
}

auto DLLoader::load(unsigned id, std::string ident) -> bool {
    if (!handle)
        return false;

    Call* call;
    if ((call = getCall(id)) == nullptr) {
        call = new Call;
        call->id = id;
        call->ident = ident;
        call->ptr = nullptr;
        calls.push_back(call);
    }

    if (!call->ptr) {
#ifdef _WIN32
        call->ptr = reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<HMODULE>(handle), call->ident.c_str()));
#else
        call->ptr = dlsym(handle, call->ident.c_str());
#endif
        if (!call->ptr)
            return false;
    }

    return true;
}

auto DLLoader::getPtr(unsigned id) -> void* {
    Call* call = getCall(id);
    if (call)
        return call->ptr;
    return nullptr;
}

auto DLLoader::getCall(unsigned id) -> Call* {
    for (auto call: calls) {
        if (call->id == id)
            return call;
    }
    return nullptr;
}

}
