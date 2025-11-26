/**
 * @file elm_core.c
 * @brief Enterprise Legal Management Core
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>

typedef struct {
    bool initialized;
    char organization_name[256];
    char fiscal_year_start[16];
} elm_context_t;

static elm_context_t elm_ctx = {0};

regislex_error_t regislex_elm_init(const char* org_name) {
    if (!org_name) return REGISLEX_ERROR_INVALID_ARGUMENT;
    strncpy(elm_ctx.organization_name, org_name, sizeof(elm_ctx.organization_name) - 1);
    strcpy(elm_ctx.fiscal_year_start, "01-01");
    elm_ctx.initialized = true;
    return REGISLEX_OK;
}

void regislex_elm_shutdown(void) {
    memset(&elm_ctx, 0, sizeof(elm_ctx));
}

bool regislex_elm_is_initialized(void) { return elm_ctx.initialized; }
const char* regislex_elm_get_org_name(void) { return elm_ctx.organization_name; }
