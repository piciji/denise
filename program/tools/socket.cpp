
#include "../../guikit/api.h"
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
#include "error.h"

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#ifndef IPPROTO_TCP
#define IPPROTO_TCP 6
#endif


Socket::~Socket() {
    disconnect();
}

auto Socket::establishServer(const std::string& uri) -> bool {
    init();

    auto addrInfo = parseAddr( uri );
    if (!addrInfo)
        return false;

    int optval = 1;

    try {
        handle = (int)socket( addrInfo->ai_family, addrInfo->ai_socktype, addrInfo->ai_protocol);

        if (handle == -1) {
            throw Error("can't create socket for " + uri + ", error code %i");
        }

#if defined(SO_REUSEADDR)
        setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval));
#endif

        // don't accumulate small packets, send each packet immediately
        setsockopt(handle, addrInfo->ai_protocol, TCP_NODELAY, (const char*)&optval, sizeof(optval));

        if (bind(handle, addrInfo->ai_addr, (int)addrInfo->ai_addrlen) < 0) {
            throw Error("can't bind to " + uri + ", error code %i");
        }

        if (listen(handle, 2) < 0) {
            throw Error("can't listen to " + uri + ", error code %i");
        }

    } catch (Error& e) {
        disconnect();
        fprintf(stderr, e.what(), getLastError());
    }

    freeaddrinfo( addrInfo );

    return connected();
}

auto Socket::establish(const std::string& uri) -> bool {
    init();

    auto addrInfo = parseAddr( uri );
    if (!addrInfo)
        return false;

    int optval = 1;

    try {
        handle = (int)socket( addrInfo->ai_family, addrInfo->ai_socktype, addrInfo->ai_protocol);

        if (handle == -1) {
            throw Error("can't create socket for " + uri + ", error code %i");
        }

        setsockopt(handle, addrInfo->ai_protocol, TCP_NODELAY, (const char*)&optval, sizeof(optval));

        if ( connect( handle, addrInfo->ai_addr, (int)addrInfo->ai_addrlen ) != 0 ) {
            throw Error("can't connect to " + uri + ", error code %i");
        }

    } catch (Error& e) {
        disconnect();
        fprintf(stderr, e.what(), getLastError());
    }

    freeaddrinfo(addrInfo);

    return connected();
}

auto Socket::acceptClient() -> Socket* {
    sockaddr_in clientAddr{};
    socklen_t clientLen = sizeof(clientAddr);

    int clientHandle = accept( handle, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);

    if (clientHandle < 0) {
        fprintf(stderr, "server %i can not accept client, error code %i", handle, getLastError());
        return nullptr;
    }

    Socket* client = new Socket;
    client->handle = clientHandle;
    return client;
}

auto Socket::sendData( const char* data, unsigned size ) -> int {
    if (!connected())
        return SOCKET_ERROR;

    int sended = send( handle, data, size, MSG_NOSIGNAL );

    if (sended == SOCKET_ERROR) {
        fprintf(stderr, "can't send data to %i, error code %i", handle, getLastError());
    }

    return sended;
}

auto Socket::receiveData( char* data, unsigned size ) -> int {
    if (!connected())
        return SOCKET_ERROR;

    int received = recv( handle, data, size, 0 );

    if (received == SOCKET_ERROR) {
        fprintf(stderr, "can't receive data from %i, error code %i", handle, getLastError());
    }

    return received;
}

auto Socket::poll(bool& error) -> bool {
    error = true;
    if (!connected())
        return false;

    timeval timeout = { 0, 0 }; // non-blocking
    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(handle, &fds);

    // first parameter is ignored on windows
    int res = select( handle + 1, &fds, nullptr, nullptr, &timeout );

    error = res < 0;
    if (error) {
        fprintf(stderr, "can't poll %i, error code %i", handle, getLastError());
    }

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

auto Socket::parseAddr(const std::string& uri) -> addrinfo* {
    if (uri.empty())
        return nullptr;

    auto login = splitAddressAndPort( uri );
    std::string& address = login.first;
    std::string& port = login.second;

    struct addrinfo hints;
    struct addrinfo* addrInfo = nullptr;
    std::memset( &hints, 0, sizeof(hints) );

    hints.ai_family = PF_UNSPEC; // find out if V4 or V6 automatically
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo( address.c_str(), !port.empty() ? port.c_str() : nullptr, &hints, &addrInfo ) != 0) {
        std::string _msg = "can't parse address " + uri + ", error code %i";
        fprintf(stderr, _msg.c_str(), getLastError());
    }

    return addrInfo;
}

auto Socket::splitAddressAndPort(const std::string& uri) -> std::pair<std::string, std::string> {
    std::string address;
    std::string port;

    auto parts = GUIKIT::String::split( uri, ':' );

    if (parts.size() > 1) {

        port = parts.back();

        parts.pop_back();

        address = GUIKIT::String::unsplit( parts, ":" );
    } else {
        address = uri;

        port = "";
    }

    return {address, port };
}
