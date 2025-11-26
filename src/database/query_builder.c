/**
 * @file query_builder.c
 * @brief SQL Query Builder Implementation
 */

#include "database/database.h"
#include "platform/platform.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define MAX_QUERY_LENGTH 4096
#define MAX_COLUMNS 64
#define MAX_CONDITIONS 32

struct regislex_query_builder {
    char table[128];
    char columns[MAX_COLUMNS][64];
    int column_count;
    char conditions[MAX_CONDITIONS][256];
    int condition_count;
    char order_by[128];
    int limit;
    int offset;
    char query[MAX_QUERY_LENGTH];
};

regislex_query_builder_t* regislex_qb_create(const char* table) {
    if (!table) return NULL;

    regislex_query_builder_t* qb = (regislex_query_builder_t*)platform_calloc(1, sizeof(regislex_query_builder_t));
    if (!qb) return NULL;

    strncpy(qb->table, table, sizeof(qb->table) - 1);
    qb->limit = -1;
    qb->offset = -1;

    return qb;
}

void regislex_qb_destroy(regislex_query_builder_t* qb) {
    platform_free(qb);
}

regislex_query_builder_t* regislex_qb_select(regislex_query_builder_t* qb, const char* columns) {
    if (!qb || !columns) return qb;

    /* Parse comma-separated columns */
    char* copy = platform_strdup(columns);
    if (!copy) return qb;

    char* token = strtok(copy, ",");
    while (token && qb->column_count < MAX_COLUMNS) {
        while (*token == ' ') token++;
        char* end = token + strlen(token) - 1;
        while (end > token && *end == ' ') *end-- = '\0';

        strncpy(qb->columns[qb->column_count++], token, 63);
        token = strtok(NULL, ",");
    }

    platform_free(copy);
    return qb;
}

regislex_query_builder_t* regislex_qb_where(regislex_query_builder_t* qb, const char* condition) {
    if (!qb || !condition || qb->condition_count >= MAX_CONDITIONS) return qb;
    strncpy(qb->conditions[qb->condition_count++], condition, 255);
    return qb;
}

regislex_query_builder_t* regislex_qb_where_eq(regislex_query_builder_t* qb,
                                                const char* column, const char* value) {
    if (!qb || !column || !value || qb->condition_count >= MAX_CONDITIONS) return qb;
    snprintf(qb->conditions[qb->condition_count++], 255, "%s = '%s'", column, value);
    return qb;
}

regislex_query_builder_t* regislex_qb_order_by(regislex_query_builder_t* qb,
                                                const char* column, bool desc) {
    if (!qb || !column) return qb;
    snprintf(qb->order_by, sizeof(qb->order_by), "%s %s", column, desc ? "DESC" : "ASC");
    return qb;
}

regislex_query_builder_t* regislex_qb_limit(regislex_query_builder_t* qb, int limit) {
    if (!qb) return qb;
    qb->limit = limit;
    return qb;
}

regislex_query_builder_t* regislex_qb_offset(regislex_query_builder_t* qb, int offset) {
    if (!qb) return qb;
    qb->offset = offset;
    return qb;
}

const char* regislex_qb_build_select(regislex_query_builder_t* qb) {
    if (!qb) return NULL;

    char columns_str[1024] = "*";
    if (qb->column_count > 0) {
        columns_str[0] = '\0';
        for (int i = 0; i < qb->column_count; i++) {
            if (i > 0) strcat(columns_str, ", ");
            strcat(columns_str, qb->columns[i]);
        }
    }

    int len = snprintf(qb->query, sizeof(qb->query), "SELECT %s FROM %s", columns_str, qb->table);

    if (qb->condition_count > 0) {
        len += snprintf(qb->query + len, sizeof(qb->query) - len, " WHERE ");
        for (int i = 0; i < qb->condition_count; i++) {
            if (i > 0) len += snprintf(qb->query + len, sizeof(qb->query) - len, " AND ");
            len += snprintf(qb->query + len, sizeof(qb->query) - len, "%s", qb->conditions[i]);
        }
    }

    if (qb->order_by[0]) {
        len += snprintf(qb->query + len, sizeof(qb->query) - len, " ORDER BY %s", qb->order_by);
    }

    if (qb->limit >= 0) {
        len += snprintf(qb->query + len, sizeof(qb->query) - len, " LIMIT %d", qb->limit);
    }

    if (qb->offset >= 0) {
        snprintf(qb->query + len, sizeof(qb->query) - len, " OFFSET %d", qb->offset);
    }

    return qb->query;
}

const char* regislex_qb_build_insert(regislex_query_builder_t* qb,
                                      const char** columns, const char** values, int count) {
    if (!qb || !columns || !values || count <= 0) return NULL;

    int len = snprintf(qb->query, sizeof(qb->query), "INSERT INTO %s (", qb->table);

    for (int i = 0; i < count; i++) {
        if (i > 0) len += snprintf(qb->query + len, sizeof(qb->query) - len, ", ");
        len += snprintf(qb->query + len, sizeof(qb->query) - len, "%s", columns[i]);
    }

    len += snprintf(qb->query + len, sizeof(qb->query) - len, ") VALUES (");

    for (int i = 0; i < count; i++) {
        if (i > 0) len += snprintf(qb->query + len, sizeof(qb->query) - len, ", ");
        len += snprintf(qb->query + len, sizeof(qb->query) - len, "'%s'", values[i]);
    }

    snprintf(qb->query + len, sizeof(qb->query) - len, ")");

    return qb->query;
}

const char* regislex_qb_build_update(regislex_query_builder_t* qb,
                                      const char** columns, const char** values, int count) {
    if (!qb || !columns || !values || count <= 0) return NULL;

    int len = snprintf(qb->query, sizeof(qb->query), "UPDATE %s SET ", qb->table);

    for (int i = 0; i < count; i++) {
        if (i > 0) len += snprintf(qb->query + len, sizeof(qb->query) - len, ", ");
        len += snprintf(qb->query + len, sizeof(qb->query) - len, "%s = '%s'", columns[i], values[i]);
    }

    if (qb->condition_count > 0) {
        len += snprintf(qb->query + len, sizeof(qb->query) - len, " WHERE ");
        for (int i = 0; i < qb->condition_count; i++) {
            if (i > 0) len += snprintf(qb->query + len, sizeof(qb->query) - len, " AND ");
            len += snprintf(qb->query + len, sizeof(qb->query) - len, "%s", qb->conditions[i]);
        }
    }

    return qb->query;
}

const char* regislex_qb_build_delete(regislex_query_builder_t* qb) {
    if (!qb) return NULL;

    int len = snprintf(qb->query, sizeof(qb->query), "DELETE FROM %s", qb->table);

    if (qb->condition_count > 0) {
        len += snprintf(qb->query + len, sizeof(qb->query) - len, " WHERE ");
        for (int i = 0; i < qb->condition_count; i++) {
            if (i > 0) len += snprintf(qb->query + len, sizeof(qb->query) - len, " AND ");
            snprintf(qb->query + len, sizeof(qb->query) - len, "%s", qb->conditions[i]);
        }
    }

    return qb->query;
}
