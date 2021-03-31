
#include <winsock2.h>
#include <http.h>
#include <cstring>
#include "socket.h"

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

namespace Emulator {

    Socket::~Socket() {
        close();
    }

    auto Socket::establishUnixDomain(std::string address) -> bool {

        struct addrinfo hints;
        struct addrinfo* result = nullptr;
        std::memset( &hints, 0, sizeof(hints) );

        hints.ai_family = PF_UNIX;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = 0;

        if (getaddrinfo( address.c_str(), nullptr, &hints, &result ) != 0)
            return false;

        bool ret = _connect( result );

        freeaddrinfo( result );

        return ret;
    }

    auto Socket::establish(std::string address, std::string port) -> bool {

        struct addrinfo hints;
        struct addrinfo* result = nullptr;
        std::memset( &hints, 0, sizeof(hints) );

        hints.ai_family = PF_UNSPEC; // find out if V4 or V6 automatically
        // hints.ai_flags = AI_NUMERICHOST;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        if (getaddrinfo( address.c_str(), !port.empty() ? port.c_str() : nullptr, &hints, &result ) != 0)
            return false;

        bool ret = _connect( result );

        freeaddrinfo( result );

        return ret;
    }

    auto Socket::sendData( const char* data, unsigned size ) -> bool {
        if (!connected())
            return false;

        if (send( handle, data, size, MSG_NOSIGNAL ) == SOCKET_ERROR)
            return false;

        return true;
    }

    auto Socket::receiveData( char* data, unsigned size ) -> bool {
        if (!connected())
            return false;

        if (recv( handle, data, size, MSG_NOSIGNAL ) == SOCKET_ERROR)
            return false;

        return true;
    }

    auto Socket::poll() -> bool {
        struct fd_set fds;
        struct timeval timeout;
        timeout.tv_sec = 0; // no blocking
        timeout.tv_usec = 0;

        FD_ZERO(&fds);

        FD_SET(handle, &fds);

        return select( 0, &fds, nullptr, nullptr, &timeout ) > 0;
    }

    auto Socket::_connect( addrinfo* result ) -> bool {
        char optval;

        init();

        handle = (int)socket( result->ai_family, result->ai_socktype, result->ai_protocol);

        if (handle == -1)
            return false;

        setsockopt(handle, result->ai_protocol, TCP_NODELAY, &optval, sizeof(optval));

        if ( connect( handle, result->ai_addr, (int)result->ai_addrlen ) != 0 ) {

            closesocket( handle );

            return false;
        }

        return true;
    }

    auto Socket::close() -> void {

        if (handle != -1) {
            closesocket( handle );

            handle = -1;
        }
    }

    auto Socket::init() -> void {

        static bool init = false;

        if (init)
            return;

#if defined(_WIN32)
        WORD wVersionRequested = MAKEWORD(2, 2);
        WSADATA wsaData;
        WSAStartup(wVersionRequested, &wsaData);
#endif

        init = true;
    }

    auto Socket::clean() -> void {
#if defined(_WIN32)
        WSACleanup();
#endif
    }

}