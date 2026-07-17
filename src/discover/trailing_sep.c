#include "discover/trailing_sep.h"

#include <string.h>

#if defined(CBM_USE_RUST_DISCOVER_TRAILING_SEP)
extern int cbm_rs_discover_has_trailing_sep_v1(const char *value);
#endif

bool cbm_discover_has_trailing_sep(const char *value) {
#if defined(CBM_USE_RUST_DISCOVER_TRAILING_SEP)
    return cbm_rs_discover_has_trailing_sep_v1(value) != 0;
#else
    if (!value) {
        return false;
    }
    size_t len = strlen(value);
    return len > 0 && (value[len - 1] == '/' || value[len - 1] == '\\');
#endif
}
