/**
 * @file rest_api.c
 * @brief REST API Implementation (Stub)
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>

/* REST API initialization - stub */
regislex_error_t regislex_rest_api_init(regislex_context_t* ctx) {
    (void)ctx;
    return REGISLEX_OK;
}

/* REST API shutdown - stub */
void regislex_rest_api_shutdown(regislex_context_t* ctx) {
    (void)ctx;
}

/* Start REST server - stub */
regislex_error_t regislex_rest_api_start(regislex_context_t* ctx, int port) {
    (void)ctx; (void)port;
    return REGISLEX_ERROR_UNSUPPORTED;
}

/* Stop REST server - stub */
void regislex_rest_api_stop(regislex_context_t* ctx) {
    (void)ctx;
}
