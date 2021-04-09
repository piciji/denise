
#pragma once

#include <string>

namespace Emulator {

    struct Socket {

        ~Socket();

        int handle = -1;

        static auto init() -> void;
        static auto clean() -> void;

        auto establish(std::string address, std::string port = "") -> bool;
        auto disconnect() -> void;
        auto connected() -> bool { return handle != -1; }
        auto getLastError() -> int;

        auto sendData( const char* data, unsigned size ) -> bool;
        auto poll(bool& error) -> bool;
        auto receiveData( char* data, unsigned size ) -> bool;
    };

}
