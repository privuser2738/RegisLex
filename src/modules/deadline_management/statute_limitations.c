/**
 * @file statute_limitations.c
 * @brief Statute of Limitations Calculator
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>

typedef struct {
    char claim_type[128];
    char jurisdiction[64];
    int years;
    int months;
    int days;
    char notes[512];
} statute_rule_t;

/* Common statute of limitations rules (simplified) */
static statute_rule_t statute_rules[] = {
    {"personal_injury", "federal", 2, 0, 0, "Federal tort claims"},
    {"personal_injury", "CA", 2, 0, 0, "California personal injury"},
    {"personal_injury", "NY", 3, 0, 0, "New York personal injury"},
    {"personal_injury", "TX", 2, 0, 0, "Texas personal injury"},
    {"contract_written", "federal", 4, 0, 0, "Written contracts"},
    {"contract_written", "CA", 4, 0, 0, "California written contracts"},
    {"contract_oral", "CA", 2, 0, 0, "California oral contracts"},
    {"malpractice_medical", "CA", 3, 0, 0, "California medical malpractice"},
    {"malpractice_legal", "CA", 4, 0, 0, "California legal malpractice"},
    {"fraud", "CA", 3, 0, 0, "California fraud claims"},
    {"property_damage", "CA", 3, 0, 0, "California property damage"},
    {"wrongful_death", "CA", 2, 0, 0, "California wrongful death"},
    {NULL, NULL, 0, 0, 0, NULL}
};

regislex_error_t regislex_sol_calculate(const char* claim_type,
                                         const char* jurisdiction,
                                         const regislex_datetime_t* incident_date,
                                         regislex_datetime_t* deadline) {
    if (!claim_type || !jurisdiction || !incident_date || !deadline) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    /* Find matching rule */
    const statute_rule_t* rule = NULL;
    for (int i = 0; statute_rules[i].claim_type[0] != '\0'; i++) {
        if (strcmp(statute_rules[i].claim_type, claim_type) == 0 &&
            strcmp(statute_rules[i].jurisdiction, jurisdiction) == 0) {
            rule = &statute_rules[i];
            break;
        }
    }

    if (!rule) {
        /* Try federal/default */
        for (int i = 0; statute_rules[i].claim_type[0] != '\0'; i++) {
            if (strcmp(statute_rules[i].claim_type, claim_type) == 0 &&
                strcmp(statute_rules[i].jurisdiction, "federal") == 0) {
                rule = &statute_rules[i];
                break;
            }
        }
    }

    if (!rule) {
        return REGISLEX_ERROR_NOT_FOUND;
    }

    /* Calculate deadline */
    memcpy(deadline, incident_date, sizeof(regislex_datetime_t));

    if (rule->years > 0) {
        deadline->year += rule->years;
    }
    if (rule->months > 0) {
        regislex_datetime_add_months(deadline, rule->months);
    }
    if (rule->days > 0) {
        regislex_datetime_add_days(deadline, rule->days);
    }

    return REGISLEX_OK;
}

regislex_error_t regislex_sol_get_period(const char* claim_type,
                                          const char* jurisdiction,
                                          int* years, int* months, int* days) {
    if (!claim_type || !jurisdiction) return REGISLEX_ERROR_INVALID_ARGUMENT;

    for (int i = 0; statute_rules[i].claim_type[0] != '\0'; i++) {
        if (strcmp(statute_rules[i].claim_type, claim_type) == 0 &&
            strcmp(statute_rules[i].jurisdiction, jurisdiction) == 0) {
            if (years) *years = statute_rules[i].years;
            if (months) *months = statute_rules[i].months;
            if (days) *days = statute_rules[i].days;
            return REGISLEX_OK;
        }
    }

    return REGISLEX_ERROR_NOT_FOUND;
}

int regislex_sol_days_remaining(const char* claim_type,
                                 const char* jurisdiction,
                                 const regislex_datetime_t* incident_date) {
    regislex_datetime_t deadline;
    if (regislex_sol_calculate(claim_type, jurisdiction, incident_date, &deadline) != REGISLEX_OK) {
        return -1;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);

    return regislex_datetime_diff_days(&now, &deadline);
}
