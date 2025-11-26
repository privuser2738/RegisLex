/**
 * @file xml.c
 * @brief XML Parser/Writer
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>
#include <stdio.h>

regislex_error_t regislex_xml_escape(const char* input, char* output, size_t size) {
    if (!input || !output) return REGISLEX_ERROR_INVALID_ARGUMENT;
    size_t j = 0;
    for (size_t i = 0; input[i] && j < size - 6; i++) {
        switch (input[i]) {
            case '<': strcpy(output + j, "&lt;"); j += 4; break;
            case '>': strcpy(output + j, "&gt;"); j += 4; break;
            case '&': strcpy(output + j, "&amp;"); j += 5; break;
            case '"': strcpy(output + j, "&quot;"); j += 6; break;
            case '\'': strcpy(output + j, "&apos;"); j += 6; break;
            default: output[j++] = input[i];
        }
    }
    output[j] = '\0';
    return REGISLEX_OK;
}

regislex_error_t regislex_xml_element(char* buffer, size_t size, const char* tag, const char* content) {
    if (!buffer || !tag) return REGISLEX_ERROR_INVALID_ARGUMENT;
    char escaped[4096] = "";
    if (content) regislex_xml_escape(content, escaped, sizeof(escaped));
    snprintf(buffer, size, "<%s>%s</%s>", tag, escaped, tag);
    return REGISLEX_OK;
}
