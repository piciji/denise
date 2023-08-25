
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <cstdint>

#include "DLLoader.h"

#ifdef _WIN32
#define PROCCALL __cdecl
#else
#define PROCCALL
#endif

typedef signed long (PROCCALL* PROCHOOKN)(...);

namespace Emulator {

auto DLLoader::open() -> bool {
    if (handle)
        return true;

    if (!pathToLib.size()) {
        lastError = ERROR_PATH;
        return false;
    }

#ifdef _WIN32
    handle = LoadLibrary( pathToLib.c_str() );
#else
    handle = dlopen( pathToLib.c_str(), RTLD_NOW | RTLD_LAZY);
#endif
    if (!handle) {
        lastError = ERROR_OPEN;
        return false;
    }

    lastError = OK;
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
        if (!call->ptr) {
            lastError = ERROR_LOAD;
            return false;
        }
    }

    lastError = OK;
    return true;
}

auto DLLoader::execute(unsigned id) -> int {
    Call* call;
    if ((call = getCall(id)) == nullptr)
        return -1;

    if (!call->ptr)
        return -1;

    return PROCHOOKN(call->ptr)();
}

template<typename... Args>
auto DLLoader::execute(unsigned id, Args... args) -> int {
    Call* call;
    if ((call = getCall(id)) == nullptr)
        return -1;

    if (!call->ptr)
        return -1;

    return PROCHOOKN(call->ptr)(args...);
}

auto DLLoader::getCall(unsigned id) -> Call* {
    for (auto call: calls) {
        if (call->id == id)
            return call;
    }
    return nullptr;
}


template auto DLLoader::execute<int>(unsigned id, int) -> int;
template auto DLLoader::execute<int, int>(unsigned id, int, int) -> int;
template auto DLLoader::execute<int, uint8_t*, unsigned, unsigned>(unsigned id, int, uint8_t*, unsigned, unsigned ) -> int;
template auto DLLoader::execute<void*, int>(unsigned id, void*, int) -> int;
template auto DLLoader::execute<void*, int, int, int, int>(unsigned id, void*, int, int, int, int) -> int;



}
