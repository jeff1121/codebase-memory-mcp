#include "cypher/cypher_agg_func.h"

#include <stddef.h>

#if defined(CBM_USE_RUST_CYPHER_AGG_FUNC) || defined(CBM_USE_RUST_CYPHER_AGG_FUNC_ONLY)
extern int cbm_rs_cypher_aggregate_func_index_v1(int token_kind);
#endif

const char *cbm_cypher_agg_func_name(cbm_token_type_t t) {
    static const char *const names[] = {"COUNT", "SUM", "AVG", "MIN", "MAX", "COLLECT", NULL};
#if defined(CBM_USE_RUST_CYPHER_AGG_FUNC) || defined(CBM_USE_RUST_CYPHER_AGG_FUNC_ONLY)
    int idx = cbm_rs_cypher_aggregate_func_index_v1((int)t);
    return idx < 0 ? "COUNT" : names[idx];
#else
    switch (t) {
    case TOK_COUNT:
        return names[0];
    case TOK_SUM:
        return names[1];
    case TOK_AVG:
        return names[2];
    case TOK_MIN_KW:
        return names[3];
    case TOK_MAX_KW:
        return names[4];
    case TOK_COLLECT:
        return names[5];
    default:
        return "COUNT";
    }
#endif
}
