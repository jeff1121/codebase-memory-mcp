#include "foundation/constants.h"
#include "pipeline/infrascan_json.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#if defined(CBM_USE_RUST_PIPELINE_INFRASCAN_JSON) || \
    defined(CBM_USE_RUST_PIPELINE_INFRASCAN_JSON_ONLY)
extern void cbm_rs_pipeline_clean_json_brackets_v1(const char *s, char *out, size_t out_sz);
#endif

#if !defined(CBM_USE_RUST_PIPELINE_INFRASCAN_JSON) && \
    !defined(CBM_USE_RUST_PIPELINE_INFRASCAN_JSON_ONLY)
static void clean_json_array_inner(const char *s, size_t len, char *out, size_t out_sz) {
    size_t pos = 0;
    bool in_space = false;
    for (size_t i = SKIP_ONE; i < len - SKIP_ONE && pos < out_sz - SKIP_ONE; i++) {
        char c = s[i];
        if (c == '"') {
            continue;
        }
        if (c == ',') {
            c = ' ';
        }
        if (c == ' ' || c == '\t') {
            if (!in_space && pos > 0) {
                out[pos++] = ' ';
                in_space = true;
            }
        } else {
            out[pos++] = c;
            in_space = false;
        }
    }
    while (pos > 0 && out[pos - SKIP_ONE] == ' ') {
        pos--;
    }
    out[pos] = '\0';
}
#endif

void cbm_clean_json_brackets(const char *s, char *out, size_t out_sz) {
    if (!s || !out || out_sz == 0) {
        return;
    }

#if defined(CBM_USE_RUST_PIPELINE_INFRASCAN_JSON) || \
    defined(CBM_USE_RUST_PIPELINE_INFRASCAN_JSON_ONLY)
    cbm_rs_pipeline_clean_json_brackets_v1(s, out, out_sz);
#else
    size_t len = strlen(s);
    if (len >= PAIR_LEN && s[0] == '[' && s[len - SKIP_ONE] == ']') {
        clean_json_array_inner(s, len, out, out_sz);
    } else {
        snprintf(out, out_sz, "%s", s);
    }
#endif
}
