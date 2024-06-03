
#pragma once

#include <string>
#include <vector>

namespace Emulator {

struct DLLoader {

    DLLoader() {
        this->pathToLib = "";
        handle = nullptr;
        markInitialized = false;
    }

    std::string pathToLib;
    void* handle = nullptr;
    bool markInitialized = false; // free to use

    struct Call {
        unsigned id;
        std::string ident;
        void* ptr;
    };
    std::vector<Call*> calls;

    auto open() -> bool;

    auto hasOpened() -> bool { return handle != nullptr; }

    auto close() -> void;

    auto load(unsigned id, std::string ident) -> bool;
    
    auto getPtr(unsigned id) -> void*;

    auto getCall(unsigned id) -> Call*;

    auto setPath(std::string pathToLib) -> void {
        this->pathToLib = pathToLib;
    }
};

}
