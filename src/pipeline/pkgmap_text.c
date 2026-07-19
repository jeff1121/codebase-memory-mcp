#include "pipeline/pkgmap_text.h"

#include "foundation/compat.h"
#include "foundation/constants.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef CBM_USE_RUST_PIPELINE_PKGMAP_TEXT
extern int cbm_rs_pipeline_pkgmap_at_prefix_v1(const char *source, size_t available,
                                               const char *prefix, int prefix_len);
extern int cbm_rs_pipeline_pkgmap_find_line_value_offset_v1(const char *source, int source_len,
                                                            const char *prefix);
extern size_t cbm_rs_pipeline_pkgmap_path_dirname_v1(char *buf, size_t bufsize, const char *path);
extern size_t cbm_rs_pipeline_pkgmap_strip_extension_v1(char *buf, size_t bufsize,
                                                        const char *path);
extern size_t cbm_rs_pipeline_pkgmap_join_and_strip_v1(char *buf, size_t bufsize, const char *dir,
                                                       const char *entry);
extern size_t cbm_rs_pipeline_pkgmap_build_entry_path_v1(char *buf, size_t bufsize,
                                                         const char *rel_path, const char *suffix);
#endif

int cbm_pipeline_pkgmap_at_prefix(const char *source, size_t available, const char *prefix,
                                  int prefix_len) {
#ifdef CBM_USE_RUST_PIPELINE_PKGMAP_TEXT
    return cbm_rs_pipeline_pkgmap_at_prefix_v1(source, available, prefix, prefix_len);
#else
    if (!source || !prefix || prefix_len < 0 || (size_t)prefix_len > available) {
        return 0;
    }
    return memcmp(source, prefix, (size_t)prefix_len) == 0;
#endif
}

const char *cbm_pipeline_pkgmap_find_line_value(const char *source, int source_len,
                                                const char *prefix) {
    if (!source || !prefix || source_len < 0) {
        return NULL;
    }
#ifdef CBM_USE_RUST_PIPELINE_PKGMAP_TEXT
    int offset = cbm_rs_pipeline_pkgmap_find_line_value_offset_v1(source, source_len, prefix);
    return offset < 0 ? NULL : source + offset;
#else
    size_t prefix_len = strlen(prefix);
    const char *cursor = source;
    const char *end = source + source_len;
    while (cursor < end) {
        while (cursor < end && (*cursor == ' ' || *cursor == '\t')) {
            cursor++;
        }
        if ((size_t)(end - cursor) >= prefix_len && memcmp(cursor, prefix, prefix_len) == 0) {
            return cursor + prefix_len;
        }
        while (cursor < end && *cursor != '\n') {
            cursor++;
        }
        if (cursor < end) {
            cursor++;
        }
    }
    return NULL;
#endif
}

#ifdef CBM_USE_RUST_PIPELINE_PKGMAP_TEXT
static char *copy_rust_path_result(size_t (*func)(char *, size_t, const char *), const char *path) {
    size_t length = func(NULL, 0, path);
    if (length == SIZE_MAX) {
        return NULL;
    }
    char *result = malloc(length + SKIP_ONE);
    if (!result) {
        return NULL;
    }
    if (func(result, length + SKIP_ONE, path) != length) {
        free(result);
        return NULL;
    }
    return result;
}
#endif

char *cbm_pipeline_pkgmap_path_dirname(const char *path) {
#ifdef CBM_USE_RUST_PIPELINE_PKGMAP_TEXT
    return copy_rust_path_result(cbm_rs_pipeline_pkgmap_path_dirname_v1, path);
#else
    if (!path) {
        return strdup("");
    }
    const char *last = strrchr(path, '/');
    return last ? cbm_strndup(path, (size_t)(last - path)) : strdup("");
#endif
}

char *cbm_pipeline_pkgmap_strip_extension(const char *path) {
#ifdef CBM_USE_RUST_PIPELINE_PKGMAP_TEXT
    return copy_rust_path_result(cbm_rs_pipeline_pkgmap_strip_extension_v1, path);
#else
    if (!path) {
        return strdup("");
    }
    size_t len = strlen(path);
    for (size_t i = len; i > 0; i--) {
        if (path[i - SKIP_ONE] == '.') {
            return cbm_strndup(path, i - SKIP_ONE);
        }
        if (path[i - SKIP_ONE] == '/') {
            break;
        }
    }
    return strdup(path);
#endif
}

char *cbm_pipeline_pkgmap_join_and_strip(const char *dir, const char *entry) {
#ifdef CBM_USE_RUST_PIPELINE_PKGMAP_TEXT
    size_t length = cbm_rs_pipeline_pkgmap_join_and_strip_v1(NULL, 0, dir, entry);
    if (length == SIZE_MAX) {
        return NULL;
    }
    char *result = malloc(length + SKIP_ONE);
    if (!result) {
        return NULL;
    }
    if (cbm_rs_pipeline_pkgmap_join_and_strip_v1(result, length + SKIP_ONE, dir, entry) != length) {
        free(result);
        return NULL;
    }
    return result;
#else
    if (!entry || entry[0] == '\0') {
        return NULL;
    }
    if (entry[0] == '.' && entry[SKIP_ONE] == '/') {
        entry += PAIR_LEN;
    }
    char buf[CBM_SZ_1K];
    snprintf(buf, sizeof(buf), "%s%s%s", dir && dir[0] ? dir : "", dir && dir[0] ? "/" : "", entry);
    return cbm_pipeline_pkgmap_strip_extension(buf);
#endif
}

char *cbm_pipeline_pkgmap_build_entry_path(const char *rel_path, const char *suffix) {
#ifdef CBM_USE_RUST_PIPELINE_PKGMAP_TEXT
    size_t length = cbm_rs_pipeline_pkgmap_build_entry_path_v1(NULL, 0, rel_path, suffix);
    if (length == SIZE_MAX) {
        return NULL;
    }
    char *result = malloc(length + SKIP_ONE);
    if (!result) {
        return NULL;
    }
    if (cbm_rs_pipeline_pkgmap_build_entry_path_v1(result, length + SKIP_ONE, rel_path, suffix) !=
        length) {
        free(result);
        return NULL;
    }
    return result;
#else
    char *dir = cbm_pipeline_pkgmap_path_dirname(rel_path);
    if (!dir) {
        return NULL;
    }
    char buf[CBM_SZ_1K];
    snprintf(buf, sizeof(buf), "%s%s%s", dir[0] ? dir : "", dir[0] ? "/" : "",
             suffix ? suffix : "");
    free(dir);
    return strdup(buf);
#endif
}
