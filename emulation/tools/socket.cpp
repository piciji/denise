
#if defined(_WIN32)
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #define closesocket close
    #include <sys/socket.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <unistd.h>
    #include <cerrno>
#endif

#include <cstring>
#include "socket.h"

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#ifndef IPPROTO_TCP
#define IPPROTO_TCP 6
#endif

namespace Emulator {

    Socket::~Socket() {
        disconnect();
    }

    auto Socket::establish(std::string address, std::string port) -> bool {
        int optval = 1;

        if (address.empty())
            return false;

        struct addrinfo hints;
        struct addrinfo* addrInfo = nullptr;
        std::memset( &hints, 0, sizeof(hints) );

        hints.ai_family = PF_UNSPEC; // find out if V4 or V6 automatically
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        init();

        if (getaddrinfo( address.c_str(), !port.empty() ? port.c_str() : nullptr, &hints, &addrInfo ) != 0)
            return false;

        handle = (int)socket( addrInfo->ai_family, addrInfo->ai_socktype, addrInfo->ai_protocol);

        if (handle == -1) {
            freeaddrinfo(addrInfo);
            return false;
        }

        setsockopt(handle, addrInfo->ai_protocol, TCP_NODELAY, (const char*)&optval, sizeof(optval));

        if ( connect( handle, addrInfo->ai_addr, (int)addrInfo->ai_addrlen ) != 0 ) {

            disconnect();

            freeaddrinfo( addrInfo );

            return false;
        }

        freeaddrinfo( addrInfo );

        return true;
    }

    auto Socket::sendData( const char* data, unsigned size ) -> bool {
        if (!connected())
            return false;

        int error = send( handle, data, size, MSG_NOSIGNAL );

        return error != SOCKET_ERROR;
    }

    auto Socket::receiveData( char* data, unsigned size ) -> bool {
        if (!connected())
            return false;

        int error = recv( handle, data, size, MSG_NOSIGNAL );

        return error != SOCKET_ERROR;
    }

    auto Socket::poll(bool& error) -> bool {

        error = true;
        if (!connected())
            return false;

        timeval timeout = { 0, 0 }; // non blocking
        fd_set fds;

        FD_ZERO(&fds);
        FD_SET(handle, &fds);

        // first parameter is ignored on windows
        int res = select( handle + 1, &fds, nullptr, nullptr, &timeout );

        error = res < 0;
        // res == 0 means no data
        return res > 0;
    }

    auto Socket::disconnect() -> void {

        if (handle != -1) {

            closesocket( handle );

            handle = -1;
        }
    }

    auto Socket::getLastError() -> int {

#if defined(_WIN32)
        return WSAGetLastError();
#else
        return errno;
#endif

    }

    auto Socket::init() -> void {
        return;
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
        return;
#if defined(_WIN32)
        WSACleanup();
#endif
    }

}
