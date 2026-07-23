/*
 * cross_repo_json.c — JSON property extraction & builder for cross-repo intelligence.
 */
#include "pipeline/cross_repo_json.h"

#include <stdio.h>
#include <string.h>

#include "foundation/constants.h"

#if defined(CBM_USE_RUST_PIPELINE_CROSS_REPO_JSON) || defined(CBM_USE_RUST_PIPELINE_CROSS_REPO_JSON_ONLY)
extern int cbm_rs_pipeline_cross_repo_json_str_prop_v1(const char *json, const char *key, char *buf,
                                                       size_t bufsz);
extern size_t cbm_rs_pipeline_cross_repo_build_props_v1(
    char *buf, size_t bufsz, const char *target_project, const char *target_function,
    const char *target_file, const char *url_or_channel, const char *extra_key,
    const char *extra_val);
#endif

const char *cbm_pipeline_cross_repo_json_str_prop(const char *json, const char *key, char *buf,
                                                  size_t bufsz) {
#if defined(CBM_USE_RUST_PIPELINE_CROSS_REPO_JSON) || defined(CBM_USE_RUST_PIPELINE_CROSS_REPO_JSON_ONLY)
    return cbm_rs_pipeline_cross_repo_json_str_prop_v1(json, key, buf, bufsz) != 0 ? buf : NULL;
#else
    if (!json || !key || !buf || bufsz == 0) {
        return NULL;
    }
    char pat[CBM_SZ_128];
    snprintf(pat, sizeof(pat), "\"%s\":\"", key);
    const char *start = strstr(json, pat);
    if (!start) {
        return NULL;
    }
    start += strlen(pat);
    const char *end = strchr(start, '"');
    if (!end) {
        return NULL;
    }
    size_t len = (size_t)(end - start);
    if (len >= bufsz) {
        len = bufsz - SKIP_ONE;
    }
    memcpy(buf, start, len);
    buf[len] = '\0';
    return buf;
#endif
}

void cbm_pipeline_cross_repo_build_props(char *buf, size_t bufsz, const char *target_project,
                                         const char *target_function, const char *target_file,
                                         const char *url_or_channel, const char *extra_key,
                                         const char *extra_val) {
#if defined(CBM_USE_RUST_PIPELINE_CROSS_REPO_JSON) || defined(CBM_USE_RUST_PIPELINE_CROSS_REPO_JSON_ONLY)
    (void)cbm_rs_pipeline_cross_repo_build_props_v1(buf, bufsz, target_project, target_function,
                                                    target_file, url_or_channel, extra_key,
                                                    extra_val);
#else
    if (!buf || bufsz == 0) {
        return;
    }
    int n = snprintf(buf, bufsz,
                     "{\"target_project\":\"%s\",\"target_function\":\"%s\","
                     "\"target_file\":\"%s\"",
                     target_project ? target_project : "", target_function ? target_function : "",
                     target_file ? target_file : "");
    if (url_or_channel && url_or_channel[0]) {
        n += snprintf(buf + n, bufsz - (size_t)n, ",\"%s\":\"%s\"",
                      extra_key ? extra_key : "url_path", url_or_channel);
    }
    if (extra_val && extra_val[0]) {
        n += snprintf(buf + n, bufsz - (size_t)n, ",\"%s\":\"%s\"",
                      extra_key ? "transport" : "method", extra_val);
    }
    snprintf(buf + n, bufsz - (size_t)n, "}");
#endif
}
