#include "package_list.h"

#include <string.h>

#if !defined(CBM_USE_RUST_STORE_PACKAGE_LIST) && \
    !defined(CBM_USE_RUST_STORE_PACKAGE_LIST_ONLY)
static bool cbm_store_package_list_contains_c(const char *pkg, char **list, int count) {
    for (int j = 0; j < count; j++) {
        if (strcmp(pkg, list[j]) == 0) {
            return true;
        }
    }
    return false;
}
#endif

#if !defined(CBM_USE_RUST_STORE_PACKAGE_LIST_ONLY)
bool cbm_store_package_list_contains(const char *pkg, char **list, int count) {
#ifdef CBM_USE_RUST_STORE_PACKAGE_LIST
    return cbm_rs_store_package_list_contains_v1(pkg, list, count);
#else
    return cbm_store_package_list_contains_c(pkg, list, count);
#endif
}
#endif
