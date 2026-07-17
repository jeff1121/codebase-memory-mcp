#include "git/json_escaped_len.h"

#include <limits.h>

#if defined(CBM_USE_RUST_GIT_JSON_ESCAPED_LEN)
extern int cbm_rs_git_json_escaped_len_v1(const char *src);
#endif

int cbm_git_json_escaped_len(const char *src) {
#if defined(CBM_USE_RUST_GIT_JSON_ESCAPED_LEN)
    return cbm_rs_git_json_escaped_len_v1(src);
#else
    if (!src) {
        return 0;
    }

    int len = 0;
    for (const unsigned char *p = (const unsigned char *)src; *p; p++) {
        int width;
        if (*p == '"' || *p == '\\' || *p == '\n' || *p == '\r' || *p == '\t') {
            width = 2;
        } else if (*p < 0x20) {
            width = 6;
        } else {
            width = 1;
        }
        if (len > INT_MAX - 1 - width) {
            return -1;
        }
        len += width;
    }
    return len;
#endif
}
