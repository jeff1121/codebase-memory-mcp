#include "pipeline/calls_json.h"

#include "foundation/compat.h"
#include "foundation/constants.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(CBM_USE_RUST_PIPELINE_CALLS_JSON)
extern size_t cbm_rs_pipeline_calls_extract_local_name_v1(char *buf, size_t bufsize,
                                                          const char *json);
#endif

char *cbm_pipeline_calls_extract_local_name(const char *props_json) {
#if defined(CBM_USE_RUST_PIPELINE_CALLS_JSON)
    size_t length = cbm_rs_pipeline_calls_extract_local_name_v1(NULL, 0, props_json);
    if (length == SIZE_MAX) {
        return NULL;
    }
    char *result = malloc(length + SKIP_ONE);
    if (!result) {
        return NULL;
    }
    if (cbm_rs_pipeline_calls_extract_local_name_v1(result, length + SKIP_ONE, props_json) !=
        length) {
        free(result);
        return NULL;
    }
    return result;
#else
    if (!props_json) {
        return NULL;
    }
    const char *start = strstr(props_json, "\"local_name\":\"");
    if (!start) {
        return NULL;
    }
    start += strlen("\"local_name\":\"");
    const char *end = strchr(start, '"');
    if (!end || end <= start) {
        return NULL;
    }
    return cbm_strndup(start, (size_t)(end - start));
#endif
}
