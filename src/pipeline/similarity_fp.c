#include "pipeline/similarity_fp.h"

#include "simhash/minhash.h"

#include <string.h>

#ifdef CBM_USE_RUST_PIPELINE_SIMILARITY_FP
extern int cbm_rs_pipeline_similarity_parse_fp_v1(const char *props_json, uint32_t *output);
#else

enum { FP_KEY_PREFIX_LEN = 6 };

#endif

int cbm_pipeline_similarity_parse_fp(const char *props_json, uint32_t *output) {
#ifdef CBM_USE_RUST_PIPELINE_SIMILARITY_FP
    return cbm_rs_pipeline_similarity_parse_fp_v1(props_json, output);
#else
    if (!props_json || !output) {
        return 0;
    }
    const char *fp_key = strstr(props_json, "\"fp\":\"");
    if (!fp_key) {
        return 0;
    }
    const char *hex_start = fp_key + FP_KEY_PREFIX_LEN;
    const char *hex_end = strchr(hex_start, '"');
    if (!hex_end) {
        return 0;
    }
    int hex_len = (int)(hex_end - hex_start);
    if (hex_len != CBM_MINHASH_HEX_LEN) {
        return 0;
    }
    char hex_buf[CBM_MINHASH_HEX_BUF];
    memcpy(hex_buf, hex_start, (size_t)hex_len);
    hex_buf[hex_len] = '\0';

    cbm_minhash_t fingerprint;
    if (!cbm_minhash_from_hex(hex_buf, &fingerprint)) {
        return 0;
    }
    memcpy(output, fingerprint.values, sizeof(fingerprint.values));
    return 1;
#endif
}
