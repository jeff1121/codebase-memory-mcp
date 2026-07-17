#ifndef CBM_PIPELINE_PARALLEL_JSON_H
#define CBM_PIPELINE_PARALLEL_JSON_H

#include <stddef.h>

int cbm_pipeline_parallel_json_escape_char(char *output, size_t available, int byte);
size_t cbm_pipeline_parallel_json_escaped_len(const char *value);

#endif
