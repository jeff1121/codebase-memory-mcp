#include "foundation/constants.h"
#include "store/store.h"

#include <string.h>

#ifdef CBM_USE_RUST_STORE_IMMUTABLE_URI
extern size_t cbm_rs_store_build_immutable_uri_v1(char *buf, size_t bufsize, const char *path);
#endif

bool cbm_store_build_immutable_uri(const char *path, char *out, size_t out_sz) {
#ifdef CBM_USE_RUST_STORE_IMMUTABLE_URI
    size_t len = cbm_rs_store_build_immutable_uri_v1(out, out_sz, path);
    return len != SIZE_MAX && len < out_sz;
#else
    static const char PREFIX[] = "file://";
    static const char SUFFIX[] = "?immutable=1";
    static const char HEX[] = "0123456789ABCDEF";
    size_t prefix_len = sizeof(PREFIX) - 1;
    size_t suffix_len = sizeof(SUFFIX) - 1;
    if (!path || !out || prefix_len + 1 > out_sz) {
        return false;
    }
    memcpy(out, PREFIX, prefix_len);
    size_t pos = prefix_len;

    if (path[0] != '/') {
        if (pos + 1 >= out_sz) {
            return false;
        }
        out[pos++] = '/';
    }

    for (const unsigned char *p = (const unsigned char *)path; *p != '\0'; p++) {
        unsigned char c = *p;
        if (c == '\\') {
            c = '/';
        }
        bool safe = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
                    c == '/' || c == '.' || c == '-' || c == '_' || c == '~' || c == ':';
        if (safe) {
            if (pos + 1 >= out_sz) {
                return false;
            }
            out[pos++] = (char)c;
        } else {
            if (pos + 3 >= out_sz) {
                return false;
            }
            out[pos++] = '%';
            out[pos++] = HEX[(c >> 4) & 0xF];
            out[pos++] = HEX[c & 0xF];
        }
    }

    if (pos + suffix_len + 1 > out_sz) {
        return false;
    }
    memcpy(out + pos, SUFFIX, suffix_len);
    pos += suffix_len;
    out[pos] = '\0';
    return true;
#endif
}
