/**
 * @file risk.c
 * @brief Legal Risk Management
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>

typedef enum { RISK_LOW, RISK_MEDIUM, RISK_HIGH, RISK_CRITICAL } risk_level_t;

typedef struct {
    regislex_uuid_t id;
    regislex_uuid_t entity_id;
    char entity_type[32];
    char title[256];
    char description[2048];
    risk_level_t level;
    double probability;
    double impact;
    char mitigation[2048];
    char status[32];
} legal_risk_t;

regislex_error_t regislex_risk_create(const char* title, legal_risk_t** risk) {
    if (!title || !risk) return REGISLEX_ERROR_INVALID_ARGUMENT;
    *risk = (legal_risk_t*)platform_calloc(1, sizeof(legal_risk_t));
    if (!*risk) return REGISLEX_ERROR_OUT_OF_MEMORY;
    regislex_uuid_generate(&(*risk)->id);
    strncpy((*risk)->title, title, sizeof((*risk)->title) - 1);
    (*risk)->level = RISK_MEDIUM;
    strcpy((*risk)->status, "open");
    return REGISLEX_OK;
}

void regislex_risk_free(legal_risk_t* risk) { platform_free(risk); }

double regislex_risk_score(const legal_risk_t* risk) {
    if (!risk) return 0;
    return risk->probability * risk->impact;
}
