#include "foundation/constants.h"
#include "store/store.h"

#include <stdio.h>
#include <string.h>

#ifdef CBM_USE_RUST_STORE_ARCH_PATH_SCOPE
extern size_t cbm_rs_store_normalize_arch_path_v1(char *norm_out, size_t norm_sz, const char *path);
#endif

bool cbm_store_arch_path_prepare(const char *path, char *norm_out, size_t norm_sz, char *like_out,
                                 size_t like_sz) {
#ifdef CBM_USE_RUST_STORE_ARCH_PATH_SCOPE
    size_t rust_len = cbm_rs_store_normalize_arch_path_v1(norm_out, norm_sz, path);
    if (rust_len == SIZE_MAX) {
        return false;
    }
    snprintf(like_out, like_sz, "%s/%%", norm_out);
    return true;
#else
    if (!path || !norm_out || norm_sz == 0 || !like_out || like_sz == 0) {
        return false;
    }
    while (*path == ' ' || *path == '\t' || *path == '\n' || *path == '\r') {
        path++;
    }
    if (path[0] == '\0') {
        return false;
    }
    if (strncmp(path, "./", 2) == 0) {
        path += 2;
    }
    while (*path == '/') {
        path++;
    }
    if (path[0] == '\0') {
        return false;
    }
    strncpy(norm_out, path, norm_sz - 1);
    norm_out[norm_sz - 1] = '\0';
    size_t len = strlen(norm_out);
    while (len > 0 &&
           (norm_out[len - 1] == ' ' || norm_out[len - 1] == '\t' || norm_out[len - 1] == '/')) {
        norm_out[--len] = '\0';
    }
    size_t w = 0;
    for (size_t r = 0; norm_out[r] != '\0'; r++) {
        if (norm_out[r] == '/' && w > 0 && norm_out[w - 1] == '/') {
            continue;
        }
        norm_out[w++] = norm_out[r];
    }
    norm_out[w] = '\0';
    if (norm_out[0] == '\0') {
        return false;
    }
    snprintf(like_out, like_sz, "%s/%%", norm_out);
    return true;
#endif
}
