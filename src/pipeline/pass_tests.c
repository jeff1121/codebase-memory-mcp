/*
 * pass_tests.c — Create TESTS edges from test functions to production functions.
 *
 * Scans CALLS edges in the graph buffer: if the source function has
 * is_test=true and the target does not, creates a TESTS edge.
 *
 * Also creates TESTS_FILE edges from test File nodes to the production
 * File nodes they correspond to (naming convention: _test.go → .go, etc.)
 *
 * Depends on: pass_calls having populated CALLS edges
 */
#include "foundation/constants.h"

enum {
    PT_RING = 4,
    PT_RING_MASK = 3,
    PT_TEST_LEN = 4,      /* strlen("Test") */
    PT_DESCRIBE_LEN = 9,  /* strlen("Describe") + 1... actually strlen("describe_") */
    PT_CONTEXT_LEN = 7,   /* strlen("context") */
    PT_SUFFIX_LEN = 8,    /* strlen("_test.go") */
    PT_EXT_PY = 3,        /* strlen(".py") / strlen(".go") */
    PT_PYTEST_PREFIX = 5, /* strlen("test_") */
    PT_SEARCH_COUNT = 2,  /* number of search passes */
    PT_FOUND_SKIP = 5,    /* strlen("_test") + 1 or strlen("_spec") + 1 */
};

#define SLEN(s) (sizeof(s) - 1)
#include "pipeline/pipeline.h"
#include "pipeline/pipeline_internal.h"
#include "pipeline/test_node_is_test.h"
#include "graph_buffer/graph_buffer.h"
#include "foundation/log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(CBM_USE_RUST_PIPELINE_TEST_DETECT) || defined(CBM_USE_RUST_PIPELINE_TEST_DETECT_ONLY)
extern size_t cbm_rs_pipeline_test_to_prod_path_v1(char *buf, size_t bufsize,
                                                   const char *test_path);
#endif
static const char *itoa_log(int val) {
    static char bufs[PT_RING][CBM_SZ_32];
    static int idx = 0;
    int i = idx;
    idx = (idx + SKIP_ONE) & PT_RING_MASK;
    snprintf(bufs[i], sizeof(bufs[i]), "%d", val);
    return bufs[i];
}

/* Assemble dir/name+ext into a heap-allocated path. */
static char *build_path(const char *test_path, size_t dir_len, const char *name, size_t name_len,
                        const char *ext, size_t ext_len) {
    char *result = malloc(dir_len + SKIP_ONE + name_len + ext_len + SKIP_ONE);
    if (dir_len > 0) {
        memcpy(result, test_path, dir_len);
        result[dir_len] = '/';
        memcpy(result + dir_len + SKIP_ONE, name, name_len);
        memcpy(result + dir_len + SKIP_ONE + name_len, ext, ext_len + SKIP_ONE);
    } else {
        memcpy(result, name, name_len);
        memcpy(result + name_len, ext, ext_len + SKIP_ONE);
    }
    return result;
}

/* Try to derive the production file path from a test file path.
 * Returns heap-allocated string or NULL. Caller must free(). */
static char *test_to_prod_path(const char *test_path) {
    if (!test_path) {
        return NULL;
    }

#if defined(CBM_USE_RUST_PIPELINE_TEST_DETECT) || defined(CBM_USE_RUST_PIPELINE_TEST_DETECT_ONLY)
    size_t test_len = strlen(test_path);
    char *rust_path = malloc(test_len + SKIP_ONE);
    if (!rust_path) {
        return NULL;
    }
    size_t rust_len =
        cbm_rs_pipeline_test_to_prod_path_v1(rust_path, test_len + SKIP_ONE, test_path);
    if (rust_len != SIZE_MAX) {
        return rust_path;
    }
    free(rust_path);
#endif

    const char *base = strrchr(test_path, '/');
    size_t dir_len = base ? (size_t)(base - test_path) : 0;
    base = base ? base + SKIP_ONE : test_path;

    /* Go: foo_test.go → foo.go */
    const char *suffix = strstr(base, "_test.go");
    if (suffix && suffix[PT_SUFFIX_LEN] == '\0') {
        return build_path(test_path, dir_len, base, (size_t)(suffix - base), ".go", PT_EXT_PY);
    }

    /* Python: test_foo.py → foo.py */
    if (strncmp(base, "test_", SLEN("test_")) == 0) {
        const char *ext = strstr(base, ".py");
        if (ext && ext[PT_EXT_PY] == '\0') {
            return build_path(test_path, dir_len, base + PT_PYTEST_PREFIX,
                              (size_t)(ext - base - PT_PYTEST_PREFIX), ".py", PT_EXT_PY);
        }
    }

    /* JS/TS: foo.test.ts → foo.ts, foo.spec.ts → foo.ts */
    for (int s = 0; s < PT_SEARCH_COUNT; s++) {
        const char *pat = s == 0 ? ".test." : ".spec.";
        const char *found = strstr(base, pat);
        if (found) {
            const char *ext = found + PT_FOUND_SKIP;
            return build_path(test_path, dir_len, base, (size_t)(found - base), ext, strlen(ext));
        }
    }

    return NULL;
}

/* Create TESTS edges from test functions to production functions via CALLS edges. */
static int create_tests_edges(cbm_pipeline_ctx_t *ctx) {
    int count = 0;
    const cbm_gbuf_edge_t **call_edges = NULL;
    int call_count = 0;
    int rc = cbm_gbuf_find_edges_by_type(ctx->gbuf, "CALLS", &call_edges, &call_count);
    if (rc != 0 || call_count == 0) {
        return 0;
    }

    for (int i = 0; i < call_count; i++) {
        const cbm_gbuf_edge_t *e = call_edges[i];
        const cbm_gbuf_node_t *src = cbm_gbuf_find_by_id(ctx->gbuf, e->source_id);
        const cbm_gbuf_node_t *tgt = cbm_gbuf_find_by_id(ctx->gbuf, e->target_id);
        if (!src || !tgt) {
            continue;
        }

        bool src_is_test = cbm_pipeline_test_node_is_test(src->properties_json) ||
                           (src->file_path && cbm_is_test_path(src->file_path));
        if (!src_is_test) {
            continue;
        }

        bool tgt_is_test = cbm_pipeline_test_node_is_test(tgt->properties_json) ||
                           (tgt->file_path && cbm_is_test_path(tgt->file_path));
        if (tgt_is_test) {
            continue;
        }

        if (!cbm_is_test_func_name(src->name)) {
            continue;
        }

        cbm_gbuf_apply_insert_edge(ctx->gbuf, src->id, tgt->id, "TESTS", "{}");
        count++;
    }
    return count;
}

/* Create TESTS_FILE edges from test File nodes to production File nodes. */
static int create_tests_file_edges(cbm_pipeline_ctx_t *ctx) {
    int count = 0;
    const cbm_gbuf_node_t **file_nodes = NULL;
    int file_node_count = 0;
    int rc = cbm_gbuf_find_by_label(ctx->gbuf, "File", &file_nodes, &file_node_count);
    if (rc != 0) {
        return 0;
    }

    for (int i = 0; i < file_node_count; i++) {
        const cbm_gbuf_node_t *fnode = file_nodes[i];
        if (!fnode->file_path || !cbm_is_test_path(fnode->file_path)) {
            continue;
        }

        char *prod_path = test_to_prod_path(fnode->file_path);
        if (!prod_path) {
            continue;
        }

        char *prod_qn = cbm_pipeline_fqn_compute(ctx->project_name, prod_path, "__file__");
        const cbm_gbuf_node_t *prod_node = cbm_gbuf_find_by_qn(ctx->gbuf, prod_qn);
        free(prod_qn);
        free(prod_path);

        if (prod_node && fnode->id != prod_node->id) {
            cbm_gbuf_apply_insert_edge(ctx->gbuf, fnode->id, prod_node->id, "TESTS_FILE", "{}");
            count++;
        }
    }
    return count;
}

int cbm_pipeline_pass_tests(cbm_pipeline_ctx_t *ctx, const cbm_file_info_t *files, int file_count) {
    cbm_log_info("pass.start", "pass", "tests");
    (void)files;
    (void)file_count;

    int tests_count = create_tests_edges(ctx);
    int tests_file_count = create_tests_file_edges(ctx);

    cbm_log_info("pass.done", "pass", "tests", "tests", itoa_log(tests_count), "tests_file",
                 itoa_log(tests_file_count));
    return 0;
}
