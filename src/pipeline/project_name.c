#include "pipeline/pipeline.h"

#include "foundation/constants.h"
#include "foundation/platform.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef CBM_USE_RUST_PIPELINE_PROJECT_NAME
extern size_t cbm_rs_pipeline_project_name_from_path(char *buf, size_t bufsize,
                                                     const char *abs_path);
#endif

char *cbm_project_name_from_path(const char *abs_path) {
#ifdef CBM_USE_RUST_PIPELINE_PROJECT_NAME
    size_t rust_len = cbm_rs_pipeline_project_name_from_path(NULL, 0, abs_path);
    if (rust_len != SIZE_MAX) {
        char *rust_name = malloc(rust_len + SKIP_ONE);
        if (rust_name) {
            size_t wrote =
                cbm_rs_pipeline_project_name_from_path(rust_name, rust_len + SKIP_ONE, abs_path);
            if (wrote == rust_len) {
                return rust_name;
            }
            free(rust_name);
        }
    }
#endif
    if (!abs_path || !abs_path[0]) {
        return strdup("root");
    }

    char *path = strdup(abs_path);
    size_t len = strlen(path);
    cbm_normalize_path_sep(path);

    /* 僅保留跨平台資料庫名稱允許的字元；非 ASCII byte 以十六進位保留資訊。 */
    static const char hex_digits[] = "0123456789abcdef";
    char *mapped = malloc(len * 2 + SKIP_ONE);
    if (!mapped) {
        free(path);
        return strdup("root");
    }
    size_t mlen = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)path[i];
        bool safe = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                    c == '.' || c == '_' || c == '-';
        if (safe) {
            mapped[mlen++] = (char)c;
        } else if (c >= 0x80) {
            mapped[mlen++] = hex_digits[(c >> 4) & 0xF];
            mapped[mlen++] = hex_digits[c & 0xF];
        } else {
            mapped[mlen++] = '-';
        }
    }
    mapped[mlen] = '\0';
    free(path);
    path = mapped;
    len = mlen;

    char *dst = path;
    char prev = 0;
    for (size_t i = 0; i < len; i++) {
        if ((path[i] == '-' && prev == '-') || (path[i] == '.' && prev == '.')) {
            continue;
        }
        *dst++ = path[i];
        prev = path[i];
    }
    *dst = '\0';

    char *start = path;
    while (*start == '-' || *start == '.') {
        start++;
    }

    size_t slen = strlen(start);
    while (slen > 0 && start[slen - SKIP_ONE] == '-') {
        start[--slen] = '\0';
    }

    if (*start == '\0') {
        free(path);
        return strdup("root");
    }

    char *result = strdup(start);
    free(path);
    return result;
}
