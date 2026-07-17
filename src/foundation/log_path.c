#include "foundation/log_path.h"

#if defined(CBM_USE_RUST_LOG_COPY_PATH)
extern void cbm_rs_log_copy_path_without_query_v1(const char *path, char *out, size_t outsz);
#endif

void cbm_log_copy_path_without_query(const char *path, char *out, size_t outsz) {
#if defined(CBM_USE_RUST_LOG_COPY_PATH)
    cbm_rs_log_copy_path_without_query_v1(path, out, outsz);
#else
    if (!out || outsz == 0) {
        return;
    }
    out[0] = '\0';
    if (!path) {
        return;
    }
    size_t n = 0;
    while (n < outsz - 1 && path[n] && path[n] != '?' && path[n] != '#') {
        out[n] = path[n];
        n++;
    }
    out[n] = '\0';
#endif
}
