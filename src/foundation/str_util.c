/*
 * str_util.c — Safe string operations (arena-allocated).
 */
#include "str_util.h"
#include "arena.h" // CBMArena, cbm_arena_alloc/strdup/strndup
#include "foundation/constants.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <limits.h>

#ifdef CBM_USE_RUST_STR_UTIL
extern size_t cbm_rs_path_join(char *buf, size_t bufsize, const char *base, const char *name);
extern size_t cbm_rs_path_join_n(char *buf, size_t bufsize, const char **parts, int n);
extern const char *cbm_rs_path_ext(const char *path);
extern const char *cbm_rs_path_base(const char *path);
extern size_t cbm_rs_path_dir(char *buf, size_t bufsize, const char *path);
extern int cbm_rs_str_starts_with(const char *s, const char *prefix);
extern int cbm_rs_str_ends_with(const char *s, const char *suffix);
extern int cbm_rs_str_contains(const char *s, const char *sub);
extern size_t cbm_rs_str_tolower(char *buf, size_t bufsize, const char *s);
extern size_t cbm_rs_str_replace_char(char *buf, size_t bufsize, const char *s, char from, char to);
extern size_t cbm_rs_str_strip_ext(char *buf, size_t bufsize, const char *path);
extern size_t cbm_rs_str_split_count(const char *s, char delim);
extern size_t cbm_rs_str_split_part(char *buf, size_t bufsize, const char *s, char delim,
                                    size_t index);
extern int cbm_rs_validate_shell_arg(const char *s);
extern int cbm_rs_validate_project_name(const char *name);
extern int cbm_rs_json_escape(char *buf, int bufsize, const char *src);

static char *cbm_rs_arena_string(CBMArena *a, size_t needed) {
    if (needed == (size_t)-1) {
        return NULL;
    }
    char *result = (char *)cbm_arena_alloc(a, needed + 1);
    if (result) {
        result[0] = '\0';
    }
    return result;
}
#endif

enum {
    JSON_ESC_LEN = 2,       /* escaped char takes 2 bytes (backslash + char) */
    JSON_NUL_RESERVE = 1,   /* reserve 1 byte for NUL terminator */
    JSON_CTRL_LIMIT = 0x20, /* ASCII control character upper bound */
};

char *cbm_path_join(CBMArena *a, const char *base, const char *name) {
#ifdef CBM_USE_RUST_STR_UTIL
    size_t needed = cbm_rs_path_join(NULL, 0, base, name);
    char *result = cbm_rs_arena_string(a, needed);
    if (!result) {
        return NULL;
    }
    cbm_rs_path_join(result, needed + 1, base, name);
    return result;
#else
    if (!base || !name) {
        return NULL;
    }
    size_t blen = strlen(base);
    size_t nlen = strlen(name);

    /* Handle empty components */
    if (blen == 0) {
        return cbm_arena_strdup(a, name);
    }
    if (nlen == 0) {
        return cbm_arena_strdup(a, base);
    }

    /* Strip trailing slash from base */
    while (blen > 0 && base[blen - SKIP_ONE] == '/') {
        blen--;
    }
    /* Strip leading slash from name */
    while (nlen > 0 && *name == '/') {
        name++;
        nlen--;
    }

    if (blen == 0) {
        return cbm_arena_strndup(a, name, nlen);
    }
    if (nlen == 0) {
        return cbm_arena_strndup(a, base, blen);
    }

    char *result = (char *)cbm_arena_alloc(a, blen + SKIP_ONE + nlen + SKIP_ONE);
    if (!result) {
        return NULL;
    }
    memcpy(result, base, blen);
    result[blen] = '/';
    memcpy(result + blen + SKIP_ONE, name, nlen);
    result[blen + SKIP_ONE + nlen] = '\0';
    return result;
#endif
}

char *cbm_path_join_n(CBMArena *a, const char **parts, int n) {
#ifdef CBM_USE_RUST_STR_UTIL
    size_t needed = cbm_rs_path_join_n(NULL, 0, parts, n);
    char *result = cbm_rs_arena_string(a, needed);
    if (!result) {
        return NULL;
    }
    cbm_rs_path_join_n(result, needed + 1, parts, n);
    return result;
#else
    if (n <= 0 || !parts) {
        return cbm_arena_strdup(a, "");
    }
    if (n == SKIP_ONE) {
        return cbm_arena_strdup(a, parts[0]);
    }

    char *result = cbm_arena_strdup(a, parts[0]);
    for (int i = SKIP_ONE; i < n; i++) {
        result = cbm_path_join(a, result, parts[i]);
    }
    return result;
#endif
}

const char *cbm_path_ext(const char *path) {
#ifdef CBM_USE_RUST_STR_UTIL
    return cbm_rs_path_ext(path);
#else
    if (!path) {
        return "";
    }
    const char *dot = NULL;
    const char *slash = NULL;
    for (const char *p = path; *p; p++) {
        if (*p == '.') {
            dot = p;
        }
        if (*p == '/') {
            slash = p;
        }
    }
    /* dot must be after last slash and not at start of basename */
    if (!dot) {
        return "";
    }
    if (slash && dot < slash) {
        return "";
    }
    return dot + SKIP_ONE;
#endif
}

const char *cbm_path_base(const char *path) {
#ifdef CBM_USE_RUST_STR_UTIL
    return cbm_rs_path_base(path);
#else
    if (!path) {
        return "";
    }
    const char *last_slash = NULL;
    for (const char *p = path; *p; p++) {
        if (*p == '/') {
            last_slash = p;
        }
    }
    return last_slash ? last_slash + SKIP_ONE : path;
#endif
}

char *cbm_path_dir(CBMArena *a, const char *path) {
#ifdef CBM_USE_RUST_STR_UTIL
    size_t needed = cbm_rs_path_dir(NULL, 0, path);
    char *result = cbm_rs_arena_string(a, needed);
    if (!result) {
        return NULL;
    }
    cbm_rs_path_dir(result, needed + 1, path);
    return result;
#else
    if (!path) {
        return cbm_arena_strdup(a, ".");
    }
    const char *last_slash = NULL;
    for (const char *p = path; *p; p++) {
        if (*p == '/') {
            last_slash = p;
        }
    }
    if (!last_slash) {
        return cbm_arena_strdup(a, ".");
    }
    return cbm_arena_strndup(a, path, (size_t)(last_slash - path));
#endif
}

bool cbm_str_starts_with(const char *s, const char *prefix) {
#ifdef CBM_USE_RUST_STR_UTIL
    return cbm_rs_str_starts_with(s, prefix) != 0;
#else
    if (!s || !prefix) {
        return false;
    }
    size_t plen = strlen(prefix);
    return strncmp(s, prefix, plen) == 0;
#endif
}

bool cbm_str_ends_with(const char *s, const char *suffix) {
#ifdef CBM_USE_RUST_STR_UTIL
    return cbm_rs_str_ends_with(s, suffix) != 0;
#else
    if (!s || !suffix) {
        return false;
    }
    size_t slen = strlen(s);
    size_t xlen = strlen(suffix);
    if (xlen > slen) {
        return false;
    }
    return strcmp(s + slen - xlen, suffix) == 0;
#endif
}

bool cbm_str_contains(const char *s, const char *sub) {
#ifdef CBM_USE_RUST_STR_UTIL
    return cbm_rs_str_contains(s, sub) != 0;
#else
    if (!s || !sub) {
        return false;
    }
    if (sub[0] == '\0') {
        return true;
    }
    return strstr(s, sub) != NULL;
#endif
}

char *cbm_str_tolower(CBMArena *a, const char *s) {
#ifdef CBM_USE_RUST_STR_UTIL
    size_t needed = cbm_rs_str_tolower(NULL, 0, s);
    char *result = cbm_rs_arena_string(a, needed);
    if (!result) {
        return NULL;
    }
    cbm_rs_str_tolower(result, needed + 1, s);
    return result;
#else
    if (!s) {
        return NULL;
    }
    size_t len = strlen(s);
    char *result = (char *)cbm_arena_alloc(a, len + SKIP_ONE);
    if (!result) {
        return NULL;
    }
    for (size_t i = 0; i < len; i++) {
        result[i] = (char)tolower((unsigned char)s[i]);
    }
    result[len] = '\0';
    return result;
#endif
}

char *cbm_str_replace_char(CBMArena *a, const char *s, char from, char to) {
#ifdef CBM_USE_RUST_STR_UTIL
    size_t needed = cbm_rs_str_replace_char(NULL, 0, s, from, to);
    char *result = cbm_rs_arena_string(a, needed);
    if (!result) {
        return NULL;
    }
    cbm_rs_str_replace_char(result, needed + 1, s, from, to);
    return result;
#else
    if (!s) {
        return NULL;
    }
    size_t len = strlen(s);
    char *result = (char *)cbm_arena_alloc(a, len + SKIP_ONE);
    if (!result) {
        return NULL;
    }
    for (size_t i = 0; i < len; i++) {
        result[i] = (s[i] == from) ? to : s[i];
    }
    result[len] = '\0';
    return result;
#endif
}

char *cbm_str_strip_ext(CBMArena *a, const char *path) {
#ifdef CBM_USE_RUST_STR_UTIL
    size_t needed = cbm_rs_str_strip_ext(NULL, 0, path);
    char *result = cbm_rs_arena_string(a, needed);
    if (!result) {
        return NULL;
    }
    cbm_rs_str_strip_ext(result, needed + 1, path);
    return result;
#else
    if (!path) {
        return NULL;
    }
    const char *dot = NULL;
    const char *slash = NULL;
    for (const char *p = path; *p; p++) {
        if (*p == '.') {
            dot = p;
        }
        if (*p == '/') {
            slash = p;
        }
    }
    if (!dot || (slash && dot < slash)) {
        return cbm_arena_strdup(a, path);
    }
    return cbm_arena_strndup(a, path, (size_t)(dot - path));
#endif
}

char **cbm_str_split(CBMArena *a, const char *s, char delim, int *out_count) {
#ifdef CBM_USE_RUST_STR_UTIL
    if (!out_count) {
        return NULL;
    }
    size_t count = cbm_rs_str_split_count(s, delim);
    if (count == (size_t)-1 || count > (size_t)INT_MAX ||
        count > ((size_t)-1 / sizeof(char *)) - 1) {
        return NULL;
    }

    char **result = (char **)cbm_arena_alloc(a, (count + 1) * sizeof(char *));
    if (!result) {
        return NULL;
    }
    for (size_t i = 0; i < count; i++) {
        size_t part_len = cbm_rs_str_split_part(NULL, 0, s, delim, i);
        if (part_len == (size_t)-1) {
            return NULL;
        }
        result[i] = cbm_rs_arena_string(a, part_len);
        if (!result[i]) {
            return NULL;
        }
        cbm_rs_str_split_part(result[i], part_len + 1, s, delim, i);
    }
    result[count] = NULL;
    *out_count = (int)count;
    return result;
#else
    if (!s || !out_count) {
        return NULL;
    }

    /* Count parts */
    int count = SKIP_ONE;
    for (const char *p = s; *p; p++) {
        if (*p == delim) {
            count++;
        }
    }

    char **result = (char **)cbm_arena_alloc(a, (size_t)(count + SKIP_ONE) * sizeof(char *));
    if (!result) {
        return NULL;
    }

    int idx = 0;
    const char *start = s;
    for (const char *p = s;; p++) {
        if (*p == delim || *p == '\0') {
            size_t part_len = (size_t)(p - start);
            result[idx++] = cbm_arena_strndup(a, start, part_len);
            if (*p == '\0') {
                break;
            }
            start = p + SKIP_ONE;
        }
    }

    result[idx] = NULL;
    *out_count = count;
    return result;
#endif
}

bool cbm_validate_shell_arg(const char *s) {
#ifdef CBM_USE_RUST_STR_UTIL
    return cbm_rs_validate_shell_arg(s) != 0;
#else
    if (!s) {
        return false;
    }
    for (const char *p = s; *p; p++) {
        switch (*p) {
        case '\'':
        case '"':
        case ';':
        case '|':
        case '&':
        case '$':
        case '`':
        case '<':
        case '>':
        case '\n':
        case '\r':
#ifndef _WIN32
        case '\\':
#endif
            return false;
        default:
            break;
        }
    }
    return true;
#endif
}

bool cbm_validate_project_name(const char *name) {
#ifdef CBM_USE_RUST_STR_UTIL
    return cbm_rs_validate_project_name(name) != 0;
#else
    if (!name || !*name)
        return false;
    /* Reject directory traversal */
    if (strcmp(name, "..") == 0 || strstr(name, "..") != NULL)
        return false;
    /* Reject path separators */
    if (strchr(name, '/') || strchr(name, '\\'))
        return false;
    /* Reject leading dot (hidden files / relative refs) */
    if (name[0] == '.')
        return false;
    /* Allow only alphanumeric, dash, underscore, dot */
    for (const char *p = name; *p; p++) {
        if (!(((*p >= 'a') && (*p <= 'z')) || ((*p >= 'A') && (*p <= 'Z')) ||
              ((*p >= '0') && (*p <= '9')) || *p == '-' || *p == '_' || *p == '.')) {
            return false;
        }
    }
    return true;
#endif
}

int cbm_json_escape(char *buf, int bufsize, const char *src) {
#ifdef CBM_USE_RUST_STR_UTIL
    return cbm_rs_json_escape(buf, bufsize, src);
#else
    if (!buf || bufsize <= 0) {
        return 0;
    }
    if (!src) {
        buf[0] = '\0';
        return 0;
    }
    int pos = 0;
    for (int i = 0; src[i] && pos < bufsize - JSON_NUL_RESERVE; i++) {
        unsigned char c = (unsigned char)src[i];
        if (c == '"' || c == '\\') {
            if (pos + JSON_ESC_LEN > bufsize - JSON_NUL_RESERVE) {
                break;
            }
            buf[pos++] = '\\';
            buf[pos++] = (char)c;
        } else if (c == '\n') {
            if (pos + JSON_ESC_LEN > bufsize - JSON_NUL_RESERVE) {
                break;
            }
            buf[pos++] = '\\';
            buf[pos++] = 'n';
        } else if (c == '\r') {
            if (pos + JSON_ESC_LEN > bufsize - JSON_NUL_RESERVE) {
                break;
            }
            buf[pos++] = '\\';
            buf[pos++] = 'r';
        } else if (c == '\t') {
            if (pos + JSON_ESC_LEN > bufsize - JSON_NUL_RESERVE) {
                break;
            }
            buf[pos++] = '\\';
            buf[pos++] = 't';
        } else if (c < JSON_CTRL_LIMIT) {
            /* Other control chars: escape as \u00XX */
            if (pos + 6 > bufsize - JSON_NUL_RESERVE) {
                break;
            }
            pos += snprintf(buf + pos, 7, "\\u%04x", c);
        } else {
            buf[pos++] = (char)c;
        }
    }
    buf[pos] = '\0';
    return pos;
#endif
}
