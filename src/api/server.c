/**
 * @file server.c
 * @brief HTTP Server Implementation
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>
#include <stdio.h>

typedef struct {
    platform_socket_t* socket;
    char host[128];
    int port;
    bool running;
    int max_connections;
} http_server_t;

static http_server_t* server = NULL;

regislex_error_t regislex_server_create(const char* host, int port) {
    if (server) return REGISLEX_ERROR_ALREADY_EXISTS;
    server = (http_server_t*)platform_calloc(1, sizeof(http_server_t));
    if (!server) return REGISLEX_ERROR_OUT_OF_MEMORY;
    strncpy(server->host, host ? host : "0.0.0.0", sizeof(server->host) - 1);
    server->port = port > 0 ? port : 8080;
    server->max_connections = 100;
    return REGISLEX_OK;
}

regislex_error_t regislex_server_start(void) {
    if (!server) return REGISLEX_ERROR_INVALID_ARGUMENT;
    if (platform_socket_create(PLATFORM_SOCKET_TCP, &server->socket) != PLATFORM_OK) return REGISLEX_ERROR_NETWORK;
    if (platform_socket_bind(server->socket, server->host, server->port) != PLATFORM_OK) {
        platform_socket_close(server->socket);
        return REGISLEX_ERROR_NETWORK;
    }
    if (platform_socket_listen(server->socket, server->max_connections) != PLATFORM_OK) {
        platform_socket_close(server->socket);
        return REGISLEX_ERROR_NETWORK;
    }
    server->running = true;
    return REGISLEX_OK;
}

void regislex_server_stop(void) {
    if (server) {
        server->running = false;
        if (server->socket) platform_socket_close(server->socket);
    }
}

void regislex_server_destroy(void) {
    regislex_server_stop();
    platform_free(server);
    server = NULL;
}

bool regislex_server_is_running(void) { return server && server->running; }
