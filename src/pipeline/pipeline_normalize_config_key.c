#include "pipeline/pipeline_normalize_config_key.h"
#include "cbm.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>

#if defined(CBM_USE_RUST_PIPELINE_NORMALIZE_CONFIG_KEY) || \
    defined(CBM_USE_RUST_PIPELINE_NORMALIZE_CONFIG_KEY_ONLY)
extern int cbm_rs_pipeline_normalize_config_key_v1(const char *key, char *norm_out, size_t norm_sz);
#endif

#if !defined(CBM_USE_RUST_PIPELINE_NORMALIZE_CONFIG_KEY) && \
    !defined(CBM_USE_RUST_PIPELINE_NORMALIZE_CONFIG_KEY_ONLY)
static void emit_camel_words(const char *part, size_t plen, char *norm_out, size_t norm_sz,
                             size_t *out_pos, int *token_count) {
    size_t start = 0;
    for (size_t i = 1; i < plen; i++) {
        if (part[i] >= 'A' && part[i] <= 'Z' && part[i - 1] >= 'a' && part[i - 1] <= 'z') {
            if (*out_pos > 0 && *out_pos < norm_sz - 1) {
                norm_out[(*out_pos)++] = '_';
            }
            for (size_t j = start; j < i && *out_pos < norm_sz - 1; j++) {
                norm_out[(*out_pos)++] = (char)tolower((unsigned char)part[j]);
            }
            (*token_count)++;
            start = i;
        }
    }
    if (*out_pos > 0 && *out_pos < norm_sz - 1) {
        norm_out[(*out_pos)++] = '_';
    }
    for (size_t j = start; j < plen && *out_pos < norm_sz - 1; j++) {
        norm_out[(*out_pos)++] = (char)tolower((unsigned char)part[j]);
    }
    (*token_count)++;
}
#endif

int cbm_normalize_config_key(const char *key, char *norm_out, size_t norm_sz) {
#if defined(CBM_USE_RUST_PIPELINE_NORMALIZE_CONFIG_KEY) || \
    defined(CBM_USE_RUST_PIPELINE_NORMALIZE_CONFIG_KEY_ONLY)
    return cbm_rs_pipeline_normalize_config_key_v1(key, norm_out, norm_sz);
#else
    if (!key || !norm_out || norm_sz == 0) {
        return 0;
    }
    norm_out[0] = '\0';

    char buf[512];
    size_t klen = strlen(key);
    if (klen >= sizeof(buf)) {
        klen = sizeof(buf) - 1;
    }
    memcpy(buf, key, klen);
    buf[klen] = '\0';

    for (size_t i = 0; i < klen; i++) {
        if (buf[i] == '_' || buf[i] == '-' || buf[i] == '.') {
            buf[i] = ' ';
        }
    }

    int token_count = 0;
    size_t out_pos = 0;

    char *saveptr = NULL;
    char *part = strtok_r(buf, " ", &saveptr);
    while (part) {
        emit_camel_words(part, strlen(part), norm_out, norm_sz, &out_pos, &token_count);
        part = strtok_r(NULL, " ", &saveptr);
    }

    norm_out[out_pos] = '\0';
    return token_count;
#endif
}
