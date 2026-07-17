#include "discover/path_join.h"

#include "discover/trailing_sep.h"
#include "foundation/platform.h"

#include <stdio.h>

#if defined(CBM_USE_RUST_DISCOVER_PATH_JOIN)
extern void cbm_rs_discover_path_join_v1(const char *base, const char *rel, char *out,
                                         size_t out_sz);
#endif

void cbm_discover_path_join(const char *base, const char *rel, char *out, size_t out_sz) {
#if defined(CBM_USE_RUST_DISCOVER_PATH_JOIN)
    cbm_rs_discover_path_join_v1(base, rel, out, out_sz);
#else
    if (!out || out_sz == 0) {
        return;
    }
    if (!base || base[0] == '\0') {
        snprintf(out, out_sz, "%s", rel ? rel : "");
    } else if (!rel || rel[0] == '\0') {
        snprintf(out, out_sz, "%s", base);
    } else if (cbm_discover_has_trailing_sep(base)) {
        snprintf(out, out_sz, "%s%s", base, rel);
    } else {
        snprintf(out, out_sz, "%s/%s", base, rel);
    }
    cbm_normalize_path_sep(out);
#endif
}
