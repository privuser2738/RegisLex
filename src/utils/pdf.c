/**
 * @file pdf.c
 * @brief PDF Generation (stub)
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>
#include <stdio.h>

typedef struct {
    FILE* fp;
    int object_count;
    long* object_offsets;
    int offset_capacity;
} pdf_writer_t;

regislex_error_t regislex_pdf_create(const char* path, pdf_writer_t** writer) {
    if (!path || !writer) return REGISLEX_ERROR_INVALID_ARGUMENT;
    *writer = (pdf_writer_t*)platform_calloc(1, sizeof(pdf_writer_t));
    if (!*writer) return REGISLEX_ERROR_OUT_OF_MEMORY;
    (*writer)->fp = fopen(path, "wb");
    if (!(*writer)->fp) { platform_free(*writer); return REGISLEX_ERROR_IO; }
    fprintf((*writer)->fp, "%%PDF-1.4\n");
    return REGISLEX_OK;
}

void regislex_pdf_close(pdf_writer_t* writer) {
    if (writer) {
        if (writer->fp) fclose(writer->fp);
        platform_free(writer->object_offsets);
        platform_free(writer);
    }
}

regislex_error_t regislex_pdf_add_text(pdf_writer_t* writer, const char* text, int x, int y) {
    (void)writer; (void)text; (void)x; (void)y;
    return REGISLEX_ERROR_UNSUPPORTED;
}
