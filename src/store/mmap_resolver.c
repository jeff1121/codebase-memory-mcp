#include "foundation/constants.h"
#include "foundation/platform.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef CBM_USE_RUST_STORE_MMAP_RESOLVER
extern int64_t cbm_rs_store_resolve_mmap_size_value_v1(const char *value);
#endif

int64_t cbm_store_resolve_mmap_size(void) {
    enum { MMAP_DEFAULT = 67108864, BASE_10 = 10 };
    char buf[CBM_SZ_64];
    const char *value = cbm_safe_getenv("CBM_SQLITE_MMAP_SIZE", buf, sizeof(buf), NULL);
#ifdef CBM_USE_RUST_STORE_MMAP_RESOLVER
    return cbm_rs_store_resolve_mmap_size_value_v1(value);
#endif
    if (value == NULL) {
        return (int64_t)MMAP_DEFAULT;
    }
    char *end = NULL;
    errno = 0;
    long long parsed = strtoll(buf, &end, BASE_10);
    if (errno == ERANGE || end == buf || *end != '\0') {
        return (int64_t)MMAP_DEFAULT;
    }
    if (parsed < 0) {
        return 0;
    }
    return (int64_t)parsed;
}
