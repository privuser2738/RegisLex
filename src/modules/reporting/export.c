/**
 * @file export.c
 * @brief Report Export Implementation
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <stdio.h>
#include <string.h>

typedef enum {
    EXPORT_FORMAT_CSV,
    EXPORT_FORMAT_JSON,
    EXPORT_FORMAT_HTML,
    EXPORT_FORMAT_PDF,
    EXPORT_FORMAT_XLSX
} export_format_t;

regislex_error_t regislex_export_to_csv(const void* data, int row_count, int col_count,
                                         const char** headers, const char* output_path) {
    if (!data || !output_path) return REGISLEX_ERROR_INVALID_ARGUMENT;

    FILE* fp = fopen(output_path, "w");
    if (!fp) return REGISLEX_ERROR_IO;

    /* Write headers */
    if (headers) {
        for (int c = 0; c < col_count; c++) {
            if (c > 0) fprintf(fp, ",");
            fprintf(fp, "\"%s\"", headers[c] ? headers[c] : "");
        }
        fprintf(fp, "\n");
    }

    /* Write data rows */
    const char** rows = (const char**)data;
    for (int r = 0; r < row_count; r++) {
        for (int c = 0; c < col_count; c++) {
            if (c > 0) fprintf(fp, ",");
            const char* val = rows[r * col_count + c];
            if (val) {
                /* Escape quotes and wrap in quotes */
                fprintf(fp, "\"");
                for (const char* p = val; *p; p++) {
                    if (*p == '"') fprintf(fp, "\"\"");
                    else fputc(*p, fp);
                }
                fprintf(fp, "\"");
            }
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
    return REGISLEX_OK;
}

regislex_error_t regislex_export_to_json(const void* data, int row_count, int col_count,
                                          const char** headers, const char* output_path) {
    if (!data || !output_path) return REGISLEX_ERROR_INVALID_ARGUMENT;

    FILE* fp = fopen(output_path, "w");
    if (!fp) return REGISLEX_ERROR_IO;

    fprintf(fp, "[\n");

    const char** rows = (const char**)data;
    for (int r = 0; r < row_count; r++) {
        fprintf(fp, "  {");
        for (int c = 0; c < col_count; c++) {
            if (c > 0) fprintf(fp, ", ");
            const char* key = headers ? headers[c] : "col";
            const char* val = rows[r * col_count + c];
            fprintf(fp, "\"%s\": \"%s\"", key, val ? val : "");
        }
        fprintf(fp, "}%s\n", r < row_count - 1 ? "," : "");
    }

    fprintf(fp, "]\n");
    fclose(fp);
    return REGISLEX_OK;
}

regislex_error_t regislex_export_to_html(const void* data, int row_count, int col_count,
                                          const char** headers, const char* output_path,
                                          const char* title) {
    if (!data || !output_path) return REGISLEX_ERROR_INVALID_ARGUMENT;

    FILE* fp = fopen(output_path, "w");
    if (!fp) return REGISLEX_ERROR_IO;

    fprintf(fp, "<!DOCTYPE html>\n<html>\n<head>\n");
    fprintf(fp, "<title>%s</title>\n", title ? title : "Report");
    fprintf(fp, "<style>table{border-collapse:collapse;width:100%%;}th,td{border:1px solid #ddd;padding:8px;text-align:left;}th{background:#f4f4f4;}</style>\n");
    fprintf(fp, "</head>\n<body>\n");
    fprintf(fp, "<h1>%s</h1>\n", title ? title : "Report");
    fprintf(fp, "<table>\n");

    if (headers) {
        fprintf(fp, "<tr>");
        for (int c = 0; c < col_count; c++) {
            fprintf(fp, "<th>%s</th>", headers[c] ? headers[c] : "");
        }
        fprintf(fp, "</tr>\n");
    }

    const char** rows = (const char**)data;
    for (int r = 0; r < row_count; r++) {
        fprintf(fp, "<tr>");
        for (int c = 0; c < col_count; c++) {
            fprintf(fp, "<td>%s</td>", rows[r * col_count + c] ? rows[r * col_count + c] : "");
        }
        fprintf(fp, "</tr>\n");
    }

    fprintf(fp, "</table>\n</body>\n</html>\n");
    fclose(fp);
    return REGISLEX_OK;
}
