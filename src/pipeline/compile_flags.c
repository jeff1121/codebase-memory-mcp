/*
 * compile_flags.c — C fallback for extracting include & define compile flags.
 */
#include "pipeline/compile_flags.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "foundation/constants.h"
#include "pipeline/pipeline_internal.h"

enum { CC_FLAG_IDX = 1, CC_FLAG_SKIP = 2 };

#if defined(CBM_USE_RUST_PIPELINE_COMPILE_FLAGS) || defined(CBM_USE_RUST_PIPELINE_COMPILE_FLAGS_ONLY)
extern bool cbm_rs_pipeline_try_include_flag_v1(cbm_compile_flags_t *f, const char **args, int argc,
                                                int *i, const char *directory);
extern bool cbm_rs_pipeline_try_define_flag_v1(cbm_compile_flags_t *f, const char **args, int argc,
                                               int *i);
#endif

#if !defined(CBM_USE_RUST_PIPELINE_COMPILE_FLAGS) && !defined(CBM_USE_RUST_PIPELINE_COMPILE_FLAGS_ONLY)
#ifdef CBM_USE_RUST_PIPELINE_SPLIT_COMMAND
extern size_t cbm_rs_pipeline_resolve_compile_path_v1(char *buf, size_t bufsize, const char *path,
                                                      const char *directory);
#endif

static char *resolve_path(const char *path, const char *directory) {
#ifdef CBM_USE_RUST_PIPELINE_SPLIT_COMMAND
    size_t length = cbm_rs_pipeline_resolve_compile_path_v1(NULL, 0, path, directory);
    if (length == (size_t)-1) {
        return NULL;
    }
    char *result = malloc(length + SKIP_ONE);
    if (!result) {
        return NULL;
    }
    if (cbm_rs_pipeline_resolve_compile_path_v1(result, length + SKIP_ONE, path, directory) !=
        length) {
        free(result);
        return NULL;
    }
    return result;
#else
    if (!path) {
        return NULL;
    }

    if (path[0] == '/') {
        return strdup(path);
    }

    if (directory && directory[0]) {
        char buf[CBM_SZ_4K];
        snprintf(buf, sizeof(buf), "%s/%s", directory, path);
        return strdup(buf);
    }

    return strdup(path);
#endif
}
#endif

bool cbm_pipeline_try_include_flag(cbm_compile_flags_t *f, const char **args, int argc, int *i,
                                   const char *directory) {
#if defined(CBM_USE_RUST_PIPELINE_COMPILE_FLAGS) || defined(CBM_USE_RUST_PIPELINE_COMPILE_FLAGS_ONLY)
    return cbm_rs_pipeline_try_include_flag_v1(f, args, argc, i, directory);
#else
    if (!f || !args || !i || *i < 0 || *i >= argc) {
        return false;
    }
    const char *arg = args[*i];
    if (!arg) {
        return false;
    }
    if (arg[0] == '-' && arg[CC_FLAG_IDX] == 'I') {
        const char *path = arg + CC_FLAG_SKIP;
        if (*path == '\0' && *i + SKIP_ONE < argc) {
            (*i)++;
            path = args[*i];
        }
        if (path && *path) {
            f->include_paths[f->include_count++] = resolve_path(path, directory);
        }
        return true;
    }
    if (strcmp(arg, "-isystem") == 0 && *i + SKIP_ONE < argc) {
        (*i)++;
        f->include_paths[f->include_count++] = resolve_path(args[*i], directory);
        return true;
    }
    return false;
#endif
}

bool cbm_pipeline_try_define_flag(cbm_compile_flags_t *f, const char **args, int argc, int *i) {
#if defined(CBM_USE_RUST_PIPELINE_COMPILE_FLAGS) || defined(CBM_USE_RUST_PIPELINE_COMPILE_FLAGS_ONLY)
    return cbm_rs_pipeline_try_define_flag_v1(f, args, argc, i);
#else
    if (!f || !args || !i || *i < 0 || *i >= argc) {
        return false;
    }
    const char *arg = args[*i];
    if (!arg || arg[0] != '-' || arg[CC_FLAG_IDX] != 'D') {
        return false;
    }
    const char *define = arg + CC_FLAG_SKIP;
    if (*define == '\0' && *i + SKIP_ONE < argc) {
        (*i)++;
        define = args[*i];
    }
    if (define && *define) {
        f->defines[f->define_count++] = strdup(define);
    }
    return true;
#endif
}
