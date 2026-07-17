#include "simhash/minhash.h"

#include "foundation/constants.h"

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

/* Hex encoding constants (local to this CU) */
enum { HEX_CHARS_PER_U32 = 8, HEX_BASE = 16 };

#if defined(CBM_USE_RUST_SIMHASH_JACCARD)
extern double cbm_rs_simhash_jaccard_v1(const uint32_t *a, const uint32_t *b);
extern void cbm_rs_simhash_to_hex_v1(const uint32_t *fp, char *buf, int bufsize);
extern int cbm_rs_simhash_from_hex_v1(const char *hex, uint32_t *out);
#endif

double cbm_minhash_jaccard(const cbm_minhash_t *a, const cbm_minhash_t *b) {
#if defined(CBM_USE_RUST_SIMHASH_JACCARD)
    return cbm_rs_simhash_jaccard_v1(a ? a->values : NULL, b ? b->values : NULL);
#else
    if (!a || !b) {
        return 0.0;
    }
    int matching = 0;
    for (int i = 0; i < CBM_MINHASH_K; i++) {
        if (a->values[i] == b->values[i]) {
            matching++;
        }
    }
    return (double)matching / (double)CBM_MINHASH_K;
#endif
}

void cbm_minhash_to_hex(const cbm_minhash_t *fp, char *buf, int bufsize) {
#if defined(CBM_USE_RUST_SIMHASH_JACCARD)
    cbm_rs_simhash_to_hex_v1(fp ? fp->values : NULL, buf, bufsize);
#else
    if (!fp || !buf || bufsize < CBM_MINHASH_HEX_BUF) {
        if (buf && bufsize > 0) {
            buf[0] = '\0';
        }
        return;
    }
    int pos = 0;
    for (int i = 0; i < CBM_MINHASH_K; i++) {
        pos += snprintf(buf + pos, (size_t)(bufsize - pos), "%08x", fp->values[i]);
    }
#endif
}

bool cbm_minhash_from_hex(const char *hex, cbm_minhash_t *out) {
#if defined(CBM_USE_RUST_SIMHASH_JACCARD)
    return cbm_rs_simhash_from_hex_v1(hex, out ? out->values : NULL) != 0;
#else
    if (!hex || !out) {
        return false;
    }
    size_t len = strlen(hex);
    if ((int)len != CBM_MINHASH_HEX_LEN) {
        return false;
    }
    for (int i = 0; i < CBM_MINHASH_K; i++) {
        char chunk[HEX_CHARS_PER_U32 + SKIP_ONE];
        ptrdiff_t offset = (ptrdiff_t)i * HEX_CHARS_PER_U32;
        memcpy(chunk, hex + offset, HEX_CHARS_PER_U32);
        chunk[HEX_CHARS_PER_U32] = '\0';
        unsigned long val = strtoul(chunk, NULL, HEX_BASE);
        out->values[i] = (uint32_t)val;
    }
    return true;
#endif
}
