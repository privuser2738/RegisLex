/**
 * @file router.c
 * @brief HTTP Request Router
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>

typedef void (*route_handler_t)(void* request, void* response);

typedef struct {
    char method[8];
    char path[256];
    route_handler_t handler;
} route_t;

#define MAX_ROUTES 128
static route_t routes[MAX_ROUTES];
static int route_count = 0;

regislex_error_t regislex_router_add(const char* method, const char* path, route_handler_t handler) {
    if (!method || !path || !handler || route_count >= MAX_ROUTES) return REGISLEX_ERROR_INVALID_ARGUMENT;
    strncpy(routes[route_count].method, method, sizeof(routes[route_count].method) - 1);
    strncpy(routes[route_count].path, path, sizeof(routes[route_count].path) - 1);
    routes[route_count].handler = handler;
    route_count++;
    return REGISLEX_OK;
}

route_handler_t regislex_router_match(const char* method, const char* path) {
    if (!method || !path) return NULL;
    for (int i = 0; i < route_count; i++) {
        if (strcmp(routes[i].method, method) == 0 && strcmp(routes[i].path, path) == 0) {
            return routes[i].handler;
        }
    }
    return NULL;
}

void regislex_router_clear(void) { route_count = 0; }
