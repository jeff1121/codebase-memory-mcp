#include "foundation/constants.h"

#include <stdbool.h>
#include <string.h>

#ifdef CBM_USE_RUST_PIPELINE_ROUTE_ARGS
extern int cbm_rs_pipeline_is_path_keyword_v1(const char *keyword);
extern int cbm_rs_pipeline_normalize_url_arg_v1(char *buf, size_t bufsize, const char *input);
#endif

int cbm_pipeline_is_path_keyword(const char *keyword) {
    if (!keyword) {
        return 0;
    }
#ifdef CBM_USE_RUST_PIPELINE_ROUTE_ARGS
    return cbm_rs_pipeline_is_path_keyword_v1(keyword);
#else
    static const char *path_keywords[] = {"prefix",     "path",     "route", "pattern",
                                          "url",        "endpoint", "rule",  "mount_path",
                                          "route_path", "url_path", NULL};
    for (const char **kw = path_keywords; *kw; kw++) {
        if (strcmp(keyword, *kw) == 0) {
            return 1;
        }
    }
    return 0;
#endif
}

#ifndef CBM_USE_RUST_PIPELINE_ROUTE_ARGS
static bool is_junk_url(const char *value) {
    for (int i = 0; value[i]; i++) {
        char ch = value[i];
        if (ch == '\\' || ch == '^' || ch == '$' || ch == '*' || ch == '+' || ch == '(' ||
            ch == ')' || ch == '[' || ch == ']' || ch == '|' || ch == ' ') {
            return true;
        }
        if (ch == '/' && i > 0 && value[i - SKIP_ONE] == '/') {
            return true;
        }
    }
    return false;
}
#endif

int cbm_pipeline_normalize_url_arg(const char *url, char *norm, int norm_sz) {
    if (!url || !norm || norm_sz < PAIR_LEN) {
        return 0;
    }
#ifdef CBM_USE_RUST_PIPELINE_ROUTE_ARGS
    return cbm_rs_pipeline_normalize_url_arg_v1(norm, (size_t)norm_sz, url);
#else
    int ni = 0;
    const char *p = url;
    if (*p == '`' || *p == '"' || *p == '\'') {
        p++;
    }
    if (*p != '/') {
        return 0;
    }
    while (*p && ni < norm_sz - PAIR_LEN) {
        if (*p == '$' && *(p + SKIP_ONE) == '{') {
            norm[ni++] = ':';
            p += PAIR_LEN;
            while (*p && *p != '}' && ni < norm_sz - PAIR_LEN) {
                norm[ni++] = *p++;
            }
            if (*p == '}') {
                p++;
            }
        } else if (*p == '`' || *p == '"' || *p == '\'' || *p == '?') {
            break;
        } else {
            norm[ni++] = *p++;
        }
    }
    norm[ni] = '\0';
    enum { MIN_URL_LEN = 4 };
    if (ni < MIN_URL_LEN || !strchr(norm + SKIP_ONE, '/')) {
        return 0;
    }
    return is_junk_url(norm) ? 0 : 1;
#endif
}
