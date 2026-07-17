#include "pipeline/trackable_file.h"

#include "foundation/constants.h"

#include <string.h>

#define SLEN(s) (sizeof(s) - 1)

#if defined(CBM_USE_RUST_PIPELINE_GITHISTORY)
extern int cbm_rs_pipeline_is_trackable_file_v1(const char *path);
#endif

#if !defined(CBM_USE_RUST_PIPELINE_GITHISTORY)
static bool ends_with(const char *s, size_t slen, const char *suffix) {
    size_t sflen = strlen(suffix);
    return slen >= sflen && strcmp(s + slen - sflen, suffix) == 0;
}
#endif

bool cbm_is_trackable_file(const char *path) {
#if defined(CBM_USE_RUST_PIPELINE_GITHISTORY)
    return cbm_rs_pipeline_is_trackable_file_v1(path) != 0;
#else
    if (!path) {
        return false;
    }
    /* Skip directory prefixes */
#define LEN_NODE_MODULES_SLASH 13 /* strlen("node_modules/") */
    if (strncmp(path, ".git/", SLEN(".git/")) == 0 ||
        strncmp(path, "node_modules/", LEN_NODE_MODULES_SLASH) == 0 ||
        strncmp(path, "vendor/", SLEN("vendor/")) == 0 ||
        strncmp(path, "__pycache__/", SLEN("__pycache__/")) == 0 ||
        strncmp(path, ".cache/", SLEN(".cache/")) == 0) {
        return false;
    }
    /* Skip lock/generated file names */
    const char *base = strrchr(path, '/');
    base = base ? base + SKIP_ONE : path;
    if (strcmp(base, "package-lock.json") == 0 || strcmp(base, "yarn.lock") == 0 ||
        strcmp(base, "pnpm-lock.yaml") == 0 || strcmp(base, "Cargo.lock") == 0 ||
        strcmp(base, "poetry.lock") == 0 || strcmp(base, "composer.lock") == 0 ||
        strcmp(base, "Gemfile.lock") == 0 || strcmp(base, "Pipfile.lock") == 0) {
        return false;
    }
    /* Skip non-source file extensions */
    size_t len = strlen(path);
    if (ends_with(path, len, ".lock") || ends_with(path, len, ".sum") ||
        ends_with(path, len, ".min.js") || ends_with(path, len, ".min.css") ||
        ends_with(path, len, ".map") || ends_with(path, len, ".wasm") ||
        ends_with(path, len, ".png") || ends_with(path, len, ".jpg") ||
        ends_with(path, len, ".gif") || ends_with(path, len, ".ico") ||
        ends_with(path, len, ".svg")) {
        return false;
    }
    return true;
#endif
}
