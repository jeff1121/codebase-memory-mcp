#include "semantic/token_delim.h"

#if defined(CBM_USE_RUST_SEMANTIC_TOKEN_CLASSIFIERS) || \
    defined(CBM_USE_RUST_SEMANTIC_TOKEN_DELIM_ONLY)
extern int cbm_rs_semantic_is_token_delim_v1(int byte);
#endif

bool cbm_semantic_is_token_delim(unsigned char byte) {
#if defined(CBM_USE_RUST_SEMANTIC_TOKEN_CLASSIFIERS) || \
    defined(CBM_USE_RUST_SEMANTIC_TOKEN_DELIM_ONLY)
    return cbm_rs_semantic_is_token_delim_v1((int)byte) != 0;
#else
    return byte == '.' || byte == '/' || byte == '_' || byte == '-' || byte == ' ' || byte == '(' ||
           byte == ')' || byte == ',' || byte == ':';
#endif
}
