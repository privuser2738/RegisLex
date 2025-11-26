/**
 * @file handler.c
 * @brief HTTP Request Handlers
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>
#include <stdio.h>

typedef struct {
    char method[8];
    char path[512];
    char headers[4096];
    char body[65536];
    size_t body_length;
} http_request_t;

typedef struct {
    int status_code;
    char status_text[64];
    char headers[2048];
    char body[65536];
    size_t body_length;
} http_response_t;

void regislex_handler_health(void* req, void* resp) {
    (void)req;
    http_response_t* r = (http_response_t*)resp;
    r->status_code = 200;
    strcpy(r->status_text, "OK");
    strcpy(r->body, "{\"status\":\"healthy\",\"version\":\"" REGISLEX_VERSION_STRING "\"}");
    r->body_length = strlen(r->body);
}

void regislex_handler_not_found(void* req, void* resp) {
    (void)req;
    http_response_t* r = (http_response_t*)resp;
    r->status_code = 404;
    strcpy(r->status_text, "Not Found");
    strcpy(r->body, "{\"error\":\"Not Found\"}");
    r->body_length = strlen(r->body);
}

void regislex_handler_error(void* req, void* resp, int code, const char* message) {
    (void)req;
    http_response_t* r = (http_response_t*)resp;
    r->status_code = code;
    snprintf(r->body, sizeof(r->body), "{\"error\":\"%s\"}", message ? message : "Error");
    r->body_length = strlen(r->body);
}
