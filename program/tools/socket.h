
#pragma once

#include <string>

struct addrinfo;

struct Socket {

    ~Socket();

    int handle = -1;

    static auto init() -> void;
    static auto clean() -> void;

    auto establish(const std::string& uri) -> bool;
    auto establishServer(const std::string& uri) -> bool;

    auto disconnect() -> void;
    auto connected() const -> bool { return handle != -1; }
    auto getLastError() -> int;

    auto sendData( const char* data, unsigned size ) -> int;
    auto poll(bool& error) -> bool;
    auto receiveData( char* data, unsigned size ) -> int;
    auto acceptClient() -> Socket*;

    auto parseAddr(const std::string& uri) -> addrinfo*;
    static auto splitAddressAndPort(const std::string& uri) -> std::pair<std::string, std::string>;
};
