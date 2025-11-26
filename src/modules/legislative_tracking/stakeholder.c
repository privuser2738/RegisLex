/**
 * @file stakeholder.c
 * @brief Legislative Stakeholder Management
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>

typedef struct {
    regislex_uuid_t id;
    char name[256];
    char role[64];
    char contact_email[256];
    char jurisdiction[64];
    char notes[1024];
} legislative_stakeholder_t;

regislex_error_t regislex_stakeholder_create(const char* name, legislative_stakeholder_t** sh) {
    if (!name || !sh) return REGISLEX_ERROR_INVALID_ARGUMENT;
    *sh = (legislative_stakeholder_t*)platform_calloc(1, sizeof(legislative_stakeholder_t));
    if (!*sh) return REGISLEX_ERROR_OUT_OF_MEMORY;
    regislex_uuid_generate(&(*sh)->id);
    strncpy((*sh)->name, name, sizeof((*sh)->name) - 1);
    return REGISLEX_OK;
}

void regislex_stakeholder_free(legislative_stakeholder_t* sh) { platform_free(sh); }
