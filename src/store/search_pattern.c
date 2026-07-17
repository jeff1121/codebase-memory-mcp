/*
 * search_pattern.c — Store search pattern heap helpers.
 *
 * 只處理 glob/regex literal 的 byte-level 轉換；SQLite 查詢生命週期仍由 store.c 擁有。
 */
#include "foundation/constants.h"
#include "store/store.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { ST_GROWTH = 2, ST_GLOB_MIN_LEN = 3, ST_GLOB_SKIP = 2 };

#ifdef CBM_USE_RUST_STORE_SEARCH_PATTERN
extern size_t cbm_rs_store_glob_to_like_v1(char *buf, size_t bufsize, const char *pattern);
extern int cbm_rs_store_like_hint_count_v1(const char *pattern, int max_out);
extern size_t cbm_rs_store_like_hint_v1(char *buf, size_t bufsize, const char *pattern, int max_out,
                                        int index);
#endif

/* Convert a glob pattern to SQL LIKE pattern. */
char *cbm_glob_to_like(const char *pattern) {
#ifdef CBM_USE_RUST_STORE_SEARCH_PATTERN
    size_t len = cbm_rs_store_glob_to_like_v1(NULL, 0, pattern);
    if (len == SIZE_MAX) {
        return NULL;
    }
    char *out = malloc(len + SKIP_ONE);
    if (!out) {
        return NULL;
    }
    size_t written = cbm_rs_store_glob_to_like_v1(out, len + SKIP_ONE, pattern);
    if (written == SIZE_MAX) {
        free(out);
        return NULL;
    }
    return out;
#else
    if (!pattern) {
        return NULL;
    }
    size_t len = strlen(pattern);
    char *out = malloc((len * ST_GROWTH) + SKIP_ONE);
    size_t j = 0;

    for (size_t i = 0; i < len; i++) {
        if (pattern[i] == '*' && i + SKIP_ONE < len && pattern[i + SKIP_ONE] == '*') {
            /* Remove leading / from output if present (handles glob dir-star) */
            if (j > 0 && out[j - SKIP_ONE] == '/') {
                j--;
            }
            out[j++] = '%';
            i++; /* skip second * */
            if (i + SKIP_ONE < len && pattern[i + SKIP_ONE] == '/') {
                i++; /* skip trailing / */
            }
        } else if (pattern[i] == '*') {
            out[j++] = '%';
        } else if (pattern[i] == '?') {
            out[j++] = '_';
        } else {
            out[j++] = pattern[i];
        }
    }
    out[j] = '\0';
    return out;
#endif
}

/* ── extractLikeHints ─────────────────────────────────────────── */

int cbm_extract_like_hints(const char *pattern, char **out, int max_out) {
#ifdef CBM_USE_RUST_STORE_SEARCH_PATTERN
    if (!pattern || !out || max_out <= 0) {
        return 0;
    }
    int nhints = cbm_rs_store_like_hint_count_v1(pattern, max_out);
    if (nhints <= 0) {
        return 0;
    }
    int count = 0;
    for (int i = 0; i < nhints && i < max_out; i++) {
        size_t len = cbm_rs_store_like_hint_v1(NULL, 0, pattern, max_out, i);
        if (len == SIZE_MAX) {
            break;
        }
        char *hint = malloc(len + SKIP_ONE);
        if (!hint) {
            break;
        }
        size_t written = cbm_rs_store_like_hint_v1(hint, len + SKIP_ONE, pattern, max_out, i);
        if (written == SIZE_MAX) {
            free(hint);
            break;
        }
        out[count++] = hint;
    }
    return count;
#else
    if (!pattern || !out || max_out <= 0) {
        return 0;
    }

    /* Bail on alternation — can't convert OR regex to AND LIKE */
    for (const char *p = pattern; *p; p++) {
        if (*p == '|') {
            return 0;
        }
    }

    int count = 0;
    char buf[CBM_SZ_256];
    int blen = 0;

    int i = 0;
    while (pattern[i]) {
        char ch = pattern[i];
        switch (ch) {
        case '\\':
            /* Escaped char — the next char is literal */
            if (pattern[i + SKIP_ONE]) {
                if (blen < (int)sizeof(buf) - SKIP_ONE) {
                    buf[blen++] = pattern[i + SKIP_ONE];
                }
                i += ST_GLOB_SKIP;
            } else {
                i++;
            }
            break;
        case '.':
        case '*':
        case '+':
        case '?':
        case '^':
        case '$':
        case '(':
        case ')':
        case '[':
        case ']':
        case '{':
        case '}':
            /* Meta character — flush current literal segment */
            if (blen >= ST_GLOB_MIN_LEN && count < max_out) {
                buf[blen] = '\0';
                out[count++] = strdup(buf);
            }
            blen = 0;
            i++;
            break;
        default:
            if (blen < (int)sizeof(buf) - SKIP_ONE) {
                buf[blen++] = ch;
            }
            i++;
            break;
        }
    }
    /* Flush trailing segment */
    if (blen >= ST_GLOB_MIN_LEN && count < max_out) {
        buf[blen] = '\0';
        out[count++] = strdup(buf);
    }
    return count;
#endif
}

/* ── ensureCaseInsensitive / stripCaseFlag ────────────────────── */
