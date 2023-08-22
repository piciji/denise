
#pragma once

#include <string>
#include <vector>

namespace Emulator {

struct DLLoader {

    DLLoader() {
        this->pathToLib = "";
        handle = nullptr;
        lastError = OK;
    }

    enum Error {OK, ERROR_PATH, ERROR_OPEN, ERROR_LOAD, ERROR_CALL} lastError = OK;

    std::string pathToLib;
    void* handle;

    struct Call {
        unsigned id;
        std::string ident;
        void* ptr;
    };
    std::vector<Call*> calls;

    auto open() -> bool;

    auto hasOpened() -> bool { return handle != nullptr; }

    auto hasError() -> bool { return lastError != OK; }

    auto markCallError() -> void { lastError = ERROR_CALL; }

    auto close() -> void;

    auto load(unsigned id, std::string ident) -> bool;

    auto execute(unsigned id) -> int;

    template<typename... Args>
    auto execute(unsigned id, Args... args) -> int;

    auto getCall(unsigned id) -> Call*;

    auto setPath(std::string pathToLib) -> void {
        this->pathToLib = pathToLib;
    }
};

}
