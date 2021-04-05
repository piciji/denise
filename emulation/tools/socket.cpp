
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>
#include "socket.h"

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

namespace Emulator {

    Socket::~Socket() {
        close();
    }

    auto Socket::establish(std::string address, std::string port) -> int {
        char optval;
        bool UnixDomain = port == "UD";
        int res;

        if (UnixDomain)
            port = "";

        struct addrinfo hints;
        struct addrinfo* addrInfo = nullptr;
        std::memset( &hints, 0, sizeof(hints) );

        hints.ai_family = UnixDomain ? PF_UNIX : PF_UNSPEC; // find out if V4 or V6 automatically
        // hints.ai_flags = AI_NUMERICHOST;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = UnixDomain ? 0 : IPPROTO_TCP;

        init();

        if ((res = getaddrinfo( address.c_str(), !port.empty() ? port.c_str() : nullptr, &hints, &addrInfo )) != 0)
            return res;

        handle = (int)socket( addrInfo->ai_family, addrInfo->ai_socktype, addrInfo->ai_protocol);

        if (handle == -1)
            return freeaddrinfo( addrInfo ), -1;

        setsockopt(handle, addrInfo->ai_protocol, TCP_NODELAY, &optval, sizeof(optval));

        if ( connect( handle, addrInfo->ai_addr, (int)addrInfo->ai_addrlen ) != 0 ) {

            closesocket( handle );

            handle = -1;

            return freeaddrinfo( addrInfo ), -2;
        }

        freeaddrinfo( addrInfo );

        return 1;
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

    auto Socket::poll() -> int {
        if (!connected())
            return false;

        TIMEVAL timeout = { 0, 0 }; // non blocking
        fd_set fds;
        //timeout.tv_sec = 0; // no blocking
        //timeout.tv_usec = 0;

        FD_ZERO(&fds);
        FD_SET(handle, &fds);

        //return select( 0, &fds, nullptr, nullptr, &timeout ) > 0;

        int res = select( 0, &fds, nullptr, nullptr, &timeout );

        if (res == SOCKET_ERROR) {
            //res = WSAGetLastError();
        }

        return res;
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
