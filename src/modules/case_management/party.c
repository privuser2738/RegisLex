/**
 * @file party.c
 * @brief Party Management (Clients, Opposing Parties, etc.)
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>

typedef struct {
    regislex_uuid_t id;
    char name[256];
    char party_type[64];      /* plaintiff, defendant, client, witness, etc. */
    char contact_email[256];
    char contact_phone[64];
    char address[512];
    char notes[1024];
} regislex_party_t;

regislex_error_t regislex_party_create(regislex_context_t* ctx,
                                        const char* name,
                                        const char* party_type,
                                        regislex_party_t** party) {
    if (!ctx || !name || !party) return REGISLEX_ERROR_INVALID_ARGUMENT;

    *party = (regislex_party_t*)platform_calloc(1, sizeof(regislex_party_t));
    if (!*party) return REGISLEX_ERROR_OUT_OF_MEMORY;

    regislex_uuid_generate(&(*party)->id);
    strncpy((*party)->name, name, sizeof((*party)->name) - 1);
    if (party_type) strncpy((*party)->party_type, party_type, sizeof((*party)->party_type) - 1);

    return REGISLEX_OK;
}

void regislex_party_free(regislex_party_t* party) {
    platform_free(party);
}

const char* regislex_party_get_id(const regislex_party_t* party) {
    return party ? party->id.value : NULL;
}

const char* regislex_party_get_name(const regislex_party_t* party) {
    return party ? party->name : NULL;
}
