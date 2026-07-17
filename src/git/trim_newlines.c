#include "git/trim_newlines.h"

#include <string.h>

#if defined(CBM_USE_RUST_GIT_TRIM_NEWLINES)
extern void cbm_rs_git_trim_newlines_v1(char *value);
#endif

void cbm_git_trim_newlines(char *value) {
#if defined(CBM_USE_RUST_GIT_TRIM_NEWLINES)
    cbm_rs_git_trim_newlines_v1(value);
#else
    if (!value) {
        return;
    }
    size_t n = strlen(value);
    while (n > 0 && (value[n - 1] == '\n' || value[n - 1] == '\r')) {
        value[--n] = '\0';
    }
#endif
}
