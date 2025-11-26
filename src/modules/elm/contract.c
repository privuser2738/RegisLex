/**
 * @file contract.c
 * @brief Contract Management
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>

typedef struct {
    regislex_uuid_t id;
    regislex_uuid_t vendor_id;
    char contract_number[64];
    char title[256];
    char contract_type[64];
    regislex_datetime_t start_date;
    regislex_datetime_t end_date;
    double value;
    char status[32];
} legal_contract_t;

regislex_error_t regislex_contract_create(const char* title, legal_contract_t** contract) {
    if (!title || !contract) return REGISLEX_ERROR_INVALID_ARGUMENT;
    *contract = (legal_contract_t*)platform_calloc(1, sizeof(legal_contract_t));
    if (!*contract) return REGISLEX_ERROR_OUT_OF_MEMORY;
    regislex_uuid_generate(&(*contract)->id);
    strncpy((*contract)->title, title, sizeof((*contract)->title) - 1);
    strcpy((*contract)->status, "draft");
    return REGISLEX_OK;
}

void regislex_contract_free(legal_contract_t* contract) { platform_free(contract); }

bool regislex_contract_is_active(const legal_contract_t* contract) {
    if (!contract) return false;
    regislex_datetime_t now;
    regislex_datetime_now(&now);
    return regislex_datetime_compare(&now, &contract->start_date) >= 0 &&
           regislex_datetime_compare(&now, &contract->end_date) <= 0;
}
