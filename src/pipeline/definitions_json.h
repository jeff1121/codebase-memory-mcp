#ifndef CBM_PIPELINE_DEFINITIONS_JSON_H
#define CBM_PIPELINE_DEFINITIONS_JSON_H

#include <stddef.h>

int cbm_pipeline_definitions_json_escape_char(char *buf, size_t avail, int byte);
size_t cbm_pipeline_definitions_json_escaped_len(const char *value);

#endif
