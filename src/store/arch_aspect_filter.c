#include "store/arch_aspect_filter.h"

#include <string.h>

#if !defined(CBM_USE_RUST_STORE_ARCH_ASPECT_FILTER) && \
    !defined(CBM_USE_RUST_STORE_ARCH_ASPECT_FILTER_ONLY)
static bool want_aspect(const char *const *aspects, int aspect_count, const char *name) {
    if (!aspects || aspect_count == 0) {
        return true;
    }
    for (int i = 0; i < aspect_count; i++) {
        if (strcmp(aspects[i], "all") == 0) {
            return true;
        }
        if (strcmp(aspects[i], name) == 0) {
            return true;
        }
    }
    return false;
}
#endif

#if !defined(CBM_USE_RUST_STORE_ARCH_ASPECT_FILTER_ONLY)
bool cbm_store_arch_wants_aspect(const char *const *aspects, int aspect_count,
                                 const char *name) {
#ifdef CBM_USE_RUST_STORE_ARCH_ASPECT_FILTER
    return cbm_rs_store_arch_wants_aspect_v1(aspects, aspect_count, name);
#else
    return want_aspect(aspects, aspect_count, name);
#endif
}
#endif
