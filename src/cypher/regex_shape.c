/*
 * regex_shape.c -- Cypher regex-shaped string predicate fallback.
 */
#include "cypher/regex_shape.h"

#include <string.h>

#if defined(CBM_USE_RUST_CYPHER_REGEX_SHAPE)
extern bool cbm_rs_cypher_looks_like_regex_v1(const char *s);
#endif

#if !defined(CBM_USE_RUST_CYPHER_REGEX_SHAPE_ONLY)
bool cbm_cypher_looks_like_regex(const char *s) {
#if defined(CBM_USE_RUST_CYPHER_REGEX_SHAPE)
    return cbm_rs_cypher_looks_like_regex_v1(s);
#else
    if (!s) {
        return false;
    }
    return strstr(s, ".*") || strstr(s, ".+") || strchr(s, '[') || strchr(s, '(') ||
           strchr(s, '|') || strchr(s, '^') || strchr(s, '$');
#endif
}
#endif
