#include <stddef.h>

#include "store/safe_props.h"

#ifdef CBM_USE_RUST_STORE_SAFE_PROPS
extern const char *cbm_rs_store_safe_props_v1(const char *s);
#endif

static const char safe_props_empty[] = "{}";

static const char *safe_props_c(const char *s) {
    if (s == NULL || s[0] == '\0') {
        return safe_props_empty;
    }
    return s;
}

const char *cbm_store_safe_props(const char *s) {
#ifdef CBM_USE_RUST_STORE_SAFE_PROPS
    const char *result = cbm_rs_store_safe_props_v1(s);
    if (result != NULL) {
        return result;
    }
#endif
    return safe_props_c(s);
}
