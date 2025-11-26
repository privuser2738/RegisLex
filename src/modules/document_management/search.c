/**
 * @file search.c
 * @brief Document Search Implementation
 */

#include "regislex/regislex.h"
#include "database/database.h"
#include "platform/platform.h"
#include <string.h>

typedef struct {
    char query[256];
    char filters[1024];
    int offset;
    int limit;
} doc_search_params_t;

typedef struct {
    regislex_uuid_t doc_id;
    char title[256];
    float relevance;
} doc_search_result_t;

regislex_error_t regislex_doc_search(regislex_db_context_t* db, const doc_search_params_t* params,
                                      doc_search_result_t** results, int* count) {
    (void)db; (void)params;
    if (results) *results = NULL;
    if (count) *count = 0;
    return REGISLEX_OK;
}

regislex_error_t regislex_doc_search_by_case(regislex_db_context_t* db, const char* case_id,
                                              doc_search_result_t** results, int* count) {
    (void)db; (void)case_id;
    if (results) *results = NULL;
    if (count) *count = 0;
    return REGISLEX_OK;
}

void regislex_doc_search_results_free(doc_search_result_t* results) { platform_free(results); }
