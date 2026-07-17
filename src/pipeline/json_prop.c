#include "pipeline/json_prop.h"

#include "foundation/constants.h"

#include <stdio.h>
#include <string.h>

#if defined(CBM_USE_RUST_PIPELINE_JSON_PROP)
extern int cbm_rs_pipeline_extract_json_prop_v1(const char *json, const char *key, char *buf,
                                                int bufsz);
#endif

bool cbm_pipeline_extract_json_prop(const char *json, const char *key, char *buf, int bufsz) {
#if defined(CBM_USE_RUST_PIPELINE_JSON_PROP)
    return cbm_rs_pipeline_extract_json_prop_v1(json, key, buf, bufsz) != 0;
#else
    if (!json || !key || !buf || bufsz <= 0) {
        return false;
    }
    char pattern[CBM_SZ_64];
    size_t key_len = strlen(key);
    if (key_len > sizeof(pattern) - 5) {
        return false;
    }
    int pattern_len = snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);
    if (pattern_len < 0 || (size_t)pattern_len >= sizeof(pattern)) {
        return false;
    }
    const char *p = strstr(json, pattern);
    if (!p) {
        return false;
    }
    p += strlen(pattern);
    const char *end = strchr(p, '"');
    if (!end || end <= p) {
        return false;
    }
    size_t len = (size_t)(end - p);
    if (len >= (size_t)bufsz) {
        return false;
    }
    memcpy(buf, p, len);
    buf[len] = '\0';
    return true;
#endif
}
