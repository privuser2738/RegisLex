/**
 * @file billing.c
 * @brief Legal Billing (LEDES support)
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>
#include <stdio.h>

typedef struct {
    regislex_uuid_t id;
    regislex_uuid_t matter_id;
    regislex_uuid_t vendor_id;
    char invoice_number[64];
    regislex_datetime_t invoice_date;
    double total_amount;
    double tax_amount;
    char currency[4];
    char status[32];
    char ledes_format[16];
} legal_invoice_t;

regislex_error_t regislex_invoice_create(const char* matter_id, legal_invoice_t** inv) {
    if (!matter_id || !inv) return REGISLEX_ERROR_INVALID_ARGUMENT;
    *inv = (legal_invoice_t*)platform_calloc(1, sizeof(legal_invoice_t));
    if (!*inv) return REGISLEX_ERROR_OUT_OF_MEMORY;
    regislex_uuid_generate(&(*inv)->id);
    regislex_uuid_parse(matter_id, &(*inv)->matter_id);
    regislex_datetime_now(&(*inv)->invoice_date);
    strcpy((*inv)->currency, "USD");
    strcpy((*inv)->status, "draft");
    strcpy((*inv)->ledes_format, "LEDES98B");
    return REGISLEX_OK;
}

void regislex_invoice_free(legal_invoice_t* inv) { platform_free(inv); }

regislex_error_t regislex_invoice_export_ledes(const legal_invoice_t* inv, char* buffer, size_t size) {
    if (!inv || !buffer) return REGISLEX_ERROR_INVALID_ARGUMENT;
    snprintf(buffer, size, "LEDES98B[]|[]|\n");
    return REGISLEX_OK;
}
