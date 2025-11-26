/**
 * @file network.c
 * @brief Platform Network Implementation
 */

#include "platform/platform.h"
#include <stdlib.h>
#include <string.h>

#ifdef REGISLEX_PLATFORM_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

struct platform_socket {
#ifdef REGISLEX_PLATFORM_WINDOWS
    SOCKET fd;
#else
    int fd;
#endif
    platform_socket_type_t type;
};

static bool g_net_initialized = false;

platform_error_t platform_net_init(void) {
    if (g_net_initialized) return PLATFORM_OK;
#ifdef REGISLEX_PLATFORM_WINDOWS
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return PLATFORM_ERROR_IO;
    }
#endif
    g_net_initialized = true;
    return PLATFORM_OK;
}

void platform_net_cleanup(void) {
    if (!g_net_initialized) return;
#ifdef REGISLEX_PLATFORM_WINDOWS
    WSACleanup();
#endif
    g_net_initialized = false;
}

platform_error_t platform_socket_create(platform_socket_type_t type, platform_socket_t** sock) {
    if (!sock) return PLATFORM_ERROR_INVALID_ARGUMENT;

    *sock = (platform_socket_t*)platform_calloc(1, sizeof(platform_socket_t));
    if (!*sock) return PLATFORM_ERROR_OUT_OF_MEMORY;

    (*sock)->type = type;
    int sock_type = (type == PLATFORM_SOCKET_TCP) ? SOCK_STREAM : SOCK_DGRAM;

#ifdef REGISLEX_PLATFORM_WINDOWS
    (*sock)->fd = socket(AF_INET, sock_type, 0);
    if ((*sock)->fd == INVALID_SOCKET) {
        platform_free(*sock);
        *sock = NULL;
        return PLATFORM_ERROR_IO;
    }
#else
    (*sock)->fd = socket(AF_INET, sock_type, 0);
    if ((*sock)->fd < 0) {
        platform_free(*sock);
        *sock = NULL;
        return PLATFORM_ERROR_IO;
    }
#endif

    return PLATFORM_OK;
}

void platform_socket_close(platform_socket_t* sock) {
    if (!sock) return;
#ifdef REGISLEX_PLATFORM_WINDOWS
    if (sock->fd != INVALID_SOCKET) {
        closesocket(sock->fd);
    }
#else
    if (sock->fd >= 0) {
        close(sock->fd);
    }
#endif
    platform_free(sock);
}

platform_error_t platform_socket_connect(platform_socket_t* sock, const char* host, int port) {
    if (!sock || !host) return PLATFORM_ERROR_INVALID_ARGUMENT;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);

    struct hostent* he = gethostbyname(host);
    if (!he) return PLATFORM_ERROR_NOT_FOUND;

    memcpy(&addr.sin_addr, he->h_addr_list[0], (size_t)he->h_length);

    if (connect(sock->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        return PLATFORM_ERROR_IO;
    }

    return PLATFORM_OK;
}

platform_error_t platform_socket_bind(platform_socket_t* sock, const char* host, int port) {
    if (!sock) return PLATFORM_ERROR_INVALID_ARGUMENT;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.s_addr = host ? inet_addr(host) : INADDR_ANY;

    if (bind(sock->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        return PLATFORM_ERROR_IO;
    }

    return PLATFORM_OK;
}

platform_error_t platform_socket_listen(platform_socket_t* sock, int backlog) {
    if (!sock) return PLATFORM_ERROR_INVALID_ARGUMENT;

    if (listen(sock->fd, backlog) < 0) {
        return PLATFORM_ERROR_IO;
    }

    return PLATFORM_OK;
}

platform_error_t platform_socket_accept(platform_socket_t* sock, platform_socket_t** client) {
    if (!sock || !client) return PLATFORM_ERROR_INVALID_ARGUMENT;

    *client = (platform_socket_t*)platform_calloc(1, sizeof(platform_socket_t));
    if (!*client) return PLATFORM_ERROR_OUT_OF_MEMORY;

    struct sockaddr_in addr;
#ifdef REGISLEX_PLATFORM_WINDOWS
    int addrlen = sizeof(addr);
    (*client)->fd = accept(sock->fd, (struct sockaddr*)&addr, &addrlen);
    if ((*client)->fd == INVALID_SOCKET) {
        platform_free(*client);
        *client = NULL;
        return PLATFORM_ERROR_IO;
    }
#else
    socklen_t addrlen = sizeof(addr);
    (*client)->fd = accept(sock->fd, (struct sockaddr*)&addr, &addrlen);
    if ((*client)->fd < 0) {
        platform_free(*client);
        *client = NULL;
        return PLATFORM_ERROR_IO;
    }
#endif

    (*client)->type = sock->type;
    return PLATFORM_OK;
}

platform_error_t platform_socket_send(platform_socket_t* sock, const void* data, size_t size, size_t* sent) {
    if (!sock || !data) return PLATFORM_ERROR_INVALID_ARGUMENT;

#ifdef REGISLEX_PLATFORM_WINDOWS
    int n = send(sock->fd, (const char*)data, (int)size, 0);
    if (n == SOCKET_ERROR) return PLATFORM_ERROR_IO;
#else
    ssize_t n = send(sock->fd, data, size, 0);
    if (n < 0) return PLATFORM_ERROR_IO;
#endif

    if (sent) *sent = (size_t)n;
    return PLATFORM_OK;
}

platform_error_t platform_socket_recv(platform_socket_t* sock, void* buffer, size_t size, size_t* received) {
    if (!sock || !buffer) return PLATFORM_ERROR_INVALID_ARGUMENT;

#ifdef REGISLEX_PLATFORM_WINDOWS
    int n = recv(sock->fd, (char*)buffer, (int)size, 0);
    if (n == SOCKET_ERROR) return PLATFORM_ERROR_IO;
#else
    ssize_t n = recv(sock->fd, buffer, size, 0);
    if (n < 0) return PLATFORM_ERROR_IO;
#endif

    if (received) *received = (size_t)n;
    return PLATFORM_OK;
}

platform_error_t platform_socket_set_timeout(platform_socket_t* sock, int send_timeout_ms, int recv_timeout_ms) {
    if (!sock) return PLATFORM_ERROR_INVALID_ARGUMENT;

#ifdef REGISLEX_PLATFORM_WINDOWS
    DWORD send_tv = (DWORD)send_timeout_ms;
    DWORD recv_tv = (DWORD)recv_timeout_ms;
    setsockopt(sock->fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&send_tv, sizeof(send_tv));
    setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recv_tv, sizeof(recv_tv));
#else
    struct timeval send_tv;
    send_tv.tv_sec = send_timeout_ms / 1000;
    send_tv.tv_usec = (send_timeout_ms % 1000) * 1000;
    setsockopt(sock->fd, SOL_SOCKET, SO_SNDTIMEO, &send_tv, sizeof(send_tv));

    struct timeval recv_tv;
    recv_tv.tv_sec = recv_timeout_ms / 1000;
    recv_tv.tv_usec = (recv_timeout_ms % 1000) * 1000;
    setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO, &recv_tv, sizeof(recv_tv));
#endif

    return PLATFORM_OK;
}

platform_error_t platform_socket_set_nonblocking(platform_socket_t* sock, bool non_blocking) {
    if (!sock) return PLATFORM_ERROR_INVALID_ARGUMENT;

#ifdef REGISLEX_PLATFORM_WINDOWS
    u_long mode = non_blocking ? 1 : 0;
    if (ioctlsocket(sock->fd, FIONBIO, &mode) != 0) {
        return PLATFORM_ERROR_IO;
    }
#else
    int flags = fcntl(sock->fd, F_GETFL, 0);
    if (flags < 0) return PLATFORM_ERROR_IO;

    if (non_blocking) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }

    if (fcntl(sock->fd, F_SETFL, flags) < 0) {
        return PLATFORM_ERROR_IO;
    }
#endif

    return PLATFORM_OK;
}
