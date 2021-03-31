
#pragma once

#include <string>

namespace Emulator {

    class Socket {

        ~Socket();

        int handle = -1;

        static auto init() -> void;

        static auto clean() -> void;

        auto establish(std::string address, std::string port = "") -> bool;

        auto establishUnixDomain(std::string address) -> bool;

        auto sendData( const char* data, unsigned size ) -> bool;
        auto receiveData( char* data, unsigned size ) -> bool;
        auto poll() -> bool;

        auto close() -> void;
        auto connected() -> bool { return handle != -1; }

        auto _connect( addrinfo* result ) -> bool;
    };

}