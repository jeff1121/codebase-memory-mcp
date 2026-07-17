#include "foundation/constants.h"
#include "foundation/compat.h"

#include <string.h>

#ifdef CBM_USE_RUST_STORE_FILE_EXT
extern size_t cbm_rs_store_file_ext_lower_v1(char *buf, size_t bufsize, const char *path);
#endif

const char *cbm_store_file_ext(const char *path) {
#ifdef CBM_USE_RUST_STORE_FILE_EXT
    static CBM_TLS char buf[CBM_SZ_16];
    size_t len = cbm_rs_store_file_ext_lower_v1(buf, sizeof(buf), path);
    return len == SIZE_MAX ? NULL : buf;
#else
    if (!path) {
        return NULL;
    }
    const char *dot = strrchr(path, '.');
    if (!dot) {
        return NULL;
    }
    static CBM_TLS char buf[CBM_SZ_16];
    int len = (int)strlen(dot);
    if (len >= (int)sizeof(buf)) {
        return NULL;
    }
    for (int i = 0; i < len; i++) {
        buf[i] = (char)((dot[i] >= 'A' && dot[i] <= 'Z') ? dot[i] + CBM_SZ_32 : dot[i]);
    }
    buf[len] = '\0';
    return buf;
#endif
}
