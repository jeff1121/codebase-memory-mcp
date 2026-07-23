#include "cypher/cypher_string_func.h"

#include <stddef.h>

#if defined(CBM_USE_RUST_CYPHER_STRING_FUNC) || defined(CBM_USE_RUST_CYPHER_STRING_FUNC_ONLY)
extern int cbm_rs_cypher_string_func_index_v1(int token_kind);
#endif

const char *cbm_cypher_str_func_name(cbm_token_type_t t) {
    static const char *const names[] = {"toLower", "toUpper", "toString", NULL};
#if defined(CBM_USE_RUST_CYPHER_STRING_FUNC) || defined(CBM_USE_RUST_CYPHER_STRING_FUNC_ONLY)
    int idx = cbm_rs_cypher_string_func_index_v1((int)t);
    return idx < 0 ? "" : names[idx];
#else
    switch (t) {
    case TOK_TOLOWER:
        return names[0];
    case TOK_TOUPPER:
        return names[1];
    case TOK_TOSTRING:
        return names[2];
    default:
        return "";
    }
#endif
}
