#include "cypher/label_alt_match.h"

#include <string.h>

#if defined(CBM_USE_RUST_CYPHER_LABEL_ALT_MATCH)
extern bool cbm_rs_cypher_label_alt_matches_v1(const char *actual, const char *pat);
#endif

#if !defined(CBM_USE_RUST_CYPHER_LABEL_ALT_MATCH_ONLY) && \
    !defined(CBM_USE_RUST_CYPHER_LABEL_ALT_MATCH)
static bool label_alt_matches_fallback(const char *actual, const char *pat) {
    if (!pat) {
        return true;
    }
    if (!actual) {
        return false;
    }
    if (!strchr(pat, '|')) {
        return strcmp(actual, pat) == 0;
    }
    size_t actual_len = strlen(actual);
    const char *segment = pat;
    while (*segment) {
        const char *bar = strchr(segment, '|');
        size_t segment_len = bar ? (size_t)(bar - segment) : strlen(segment);
        if (segment_len == actual_len && strncmp(segment, actual, segment_len) == 0) {
            return true;
        }
        if (!bar) {
            break;
        }
        segment = bar + 1;
    }
    return false;
}
#endif

#if !defined(CBM_USE_RUST_CYPHER_LABEL_ALT_MATCH_ONLY)
bool cbm_cypher_label_alt_matches(const char *actual, const char *pat) {
#if defined(CBM_USE_RUST_CYPHER_LABEL_ALT_MATCH)
    return cbm_rs_cypher_label_alt_matches_v1(actual, pat);
#else
    return label_alt_matches_fallback(actual, pat);
#endif
}
#endif
