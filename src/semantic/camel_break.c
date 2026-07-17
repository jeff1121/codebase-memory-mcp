#include "semantic/camel_break.h"

#include "foundation/constants.h"

#if defined(CBM_USE_RUST_SEMANTIC_TOKEN_CLASSIFIERS) || \
    defined(CBM_USE_RUST_SEMANTIC_CAMEL_BREAK_ONLY)
extern int cbm_rs_semantic_is_camel_break_v1(const char *name, int index);
#endif

bool cbm_semantic_is_camel_break(const char *name, int index) {
    if (!name || index <= 0) {
        return false;
    }

#if defined(CBM_USE_RUST_SEMANTIC_TOKEN_CLASSIFIERS) || \
    defined(CBM_USE_RUST_SEMANTIC_CAMEL_BREAK_ONLY)
    return cbm_rs_semantic_is_camel_break_v1(name, index) != 0;
#else
    char current = name[index];
    char previous = name[index - SKIP_ONE];
    return current >= 'A' && current <= 'Z' && previous >= 'a' && previous <= 'z';
#endif
}
