/**
 * @file vendor.c
 * @brief Legal Vendor Management
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>

typedef struct {
    regislex_uuid_t id;
    char name[256];
    char vendor_type[64];
    char contact_email[256];
    char contact_phone[64];
    char address[512];
    char tax_id[32];
    double rating;
    bool is_preferred;
    bool is_active;
} legal_vendor_t;

regislex_error_t regislex_vendor_create(const char* name, legal_vendor_t** vendor) {
    if (!name || !vendor) return REGISLEX_ERROR_INVALID_ARGUMENT;
    *vendor = (legal_vendor_t*)platform_calloc(1, sizeof(legal_vendor_t));
    if (!*vendor) return REGISLEX_ERROR_OUT_OF_MEMORY;
    regislex_uuid_generate(&(*vendor)->id);
    strncpy((*vendor)->name, name, sizeof((*vendor)->name) - 1);
    (*vendor)->is_active = true;
    return REGISLEX_OK;
}

void regislex_vendor_free(legal_vendor_t* vendor) { platform_free(vendor); }
const char* regislex_vendor_get_id(const legal_vendor_t* vendor) { return vendor ? vendor->id.value : NULL; }
const char* regislex_vendor_get_name(const legal_vendor_t* vendor) { return vendor ? vendor->name : NULL; }
