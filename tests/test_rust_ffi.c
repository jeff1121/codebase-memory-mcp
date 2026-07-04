/*
 * test_rust_ffi.c - 階段性 Rust staticlib export 的 C smoke tests。
 *
 * 這個檔案刻意使用自己的 main，不連 tests/test_main.c，讓 Rust ABI gate
 * 可以不依賴完整 C test runner。
 */
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graph_buffer/graph_buffer.h"
#include "pipeline/pipeline.h"
#include "pipeline/rust_plan.h"
#ifndef _WIN32
#include <sys/stat.h>
#include <unistd.h>
#endif

_Static_assert(CBM_MODE_FULL == 0, "Rust plan ABI expects CBM_MODE_FULL=0");
_Static_assert(CBM_MODE_MODERATE == 1, "Rust plan ABI expects CBM_MODE_MODERATE=1");
_Static_assert(CBM_MODE_FAST == 2, "Rust plan ABI expects CBM_MODE_FAST=2");

extern int cbm_rs_dump_verify_is_degraded(int committed_nodes, int persisted_nodes, double ratio,
                                          int min_floor);

typedef struct CInternPool CInternPool;

extern CInternPool *cbm_rs_intern_create(void);
extern void cbm_rs_intern_free(CInternPool *pool);
extern const unsigned char *cbm_rs_intern_n(CInternPool *pool, const unsigned char *value,
                                            size_t len);
extern unsigned int cbm_rs_intern_count(const CInternPool *pool);
extern size_t cbm_rs_intern_bytes(const CInternPool *pool);
extern int cbm_rs_str_starts_with(const char *s, const char *prefix);
extern int cbm_rs_str_ends_with(const char *s, const char *suffix);
extern int cbm_rs_str_contains(const char *s, const char *sub);
extern size_t cbm_rs_path_join(char *buf, size_t bufsize, const char *base, const char *name);
extern size_t cbm_rs_path_join_n(char *buf, size_t bufsize, const char **parts, int n);
extern const char *cbm_rs_path_ext(const char *path);
extern const char *cbm_rs_path_base(const char *path);
extern size_t cbm_rs_path_dir(char *buf, size_t bufsize, const char *path);
extern size_t cbm_rs_str_tolower(char *buf, size_t bufsize, const char *s);
extern size_t cbm_rs_str_replace_char(char *buf, size_t bufsize, const char *s, char from, char to);
extern size_t cbm_rs_str_strip_ext(char *buf, size_t bufsize, const char *path);
extern size_t cbm_rs_str_split_count(const char *s, char delim);
extern size_t cbm_rs_str_split_part(char *buf, size_t bufsize, const char *s, char delim,
                                    size_t index);
extern int cbm_rs_validate_shell_arg(const char *s);
extern int cbm_rs_validate_project_name(const char *name);
extern int cbm_rs_json_escape(char *buf, int bufsize, const char *src);
extern char *cbm_rs_normalize_path_sep(char *path);
extern size_t cbm_rs_safe_getenv(char *buf, size_t bufsize, const char *name, const char *fallback);
extern size_t cbm_rs_get_home_dir(char *buf, size_t bufsize);
extern size_t cbm_rs_app_config_dir(char *buf, size_t bufsize);
extern size_t cbm_rs_app_local_dir(char *buf, size_t bufsize);
extern size_t cbm_rs_resolve_cache_dir(char *buf, size_t bufsize);
extern int cbm_rs_detect_cgroup_cpus(const char *cgroup_root);
extern size_t cbm_rs_detect_cgroup_mem(const char *cgroup_root);

/* hash_table：test-only borrowed-pointer 契約 parity（對齊 src/foundation/hash_table.c） */
typedef struct CBMRustHashTable CBMRustHashTable;
typedef void (*cbm_rs_ht_iter_fn)(const char *key, void *value, void *userdata);
extern CBMRustHashTable *cbm_rs_ht_create(void);
extern void cbm_rs_ht_free(CBMRustHashTable *ht);
extern void *cbm_rs_ht_set(CBMRustHashTable *ht, const char *key, void *value);
extern void *cbm_rs_ht_get(const CBMRustHashTable *ht, const char *key);
extern bool cbm_rs_ht_has(const CBMRustHashTable *ht, const char *key);
extern const char *cbm_rs_ht_get_key(const CBMRustHashTable *ht, const char *key);
extern void *cbm_rs_ht_delete(CBMRustHashTable *ht, const char *key);
extern unsigned int cbm_rs_ht_count(const CBMRustHashTable *ht);
extern void cbm_rs_ht_clear(CBMRustHashTable *ht);
extern void cbm_rs_ht_foreach(const CBMRustHashTable *ht, cbm_rs_ht_iter_fn func, void *userdata);

typedef struct {
    int count;
    int errors;
    long long total_us;
    long long avg_us;
    long long max_us;
} CbmRsDiagQuerySnapshot;

typedef struct {
    long uptime_s;
    size_t rss_bytes;
    size_t peak_rss_bytes;
    size_t heap_committed_bytes;
    size_t peak_committed_bytes;
    size_t page_faults;
    int fd_count;
    CbmRsDiagQuerySnapshot queries;
    int pid;
} CbmRsDiagSnapshot;

extern int cbm_rs_diag_env_enabled_value(const char *value);
extern int cbm_rs_diag_query_snapshot_values(int count, int errors, long long total_us,
                                             long long max_us, CbmRsDiagQuerySnapshot *out);
extern int cbm_rs_diag_format_path(char *buf, size_t bufsize, const char *tmpdir, int pid,
                                   const char *ext);
extern int cbm_rs_diag_format_json(char *buf, size_t bufsize, const CbmRsDiagSnapshot *snapshot);
extern int cbm_rs_diag_format_ndjson(char *buf, size_t bufsize, const CbmRsDiagSnapshot *snapshot);
extern int cbm_rs_pipeline_plan_describe(int kind, int mode, int worker_count, int file_count,
                                         char *buf, size_t bufsize);
typedef cbm_rs_pipeline_plan_step_t CbmRsPipelinePlanStep;
typedef cbm_rs_pipeline_plan_step_v2_t CbmRsPipelinePlanStepV2;

_Static_assert(sizeof(CbmRsPipelinePlanStepV2) == 32, "PlanStepV2 ABI size drift");
_Static_assert(offsetof(CbmRsPipelinePlanStepV2, kind) == 0, "PlanStepV2.kind offset drift");
_Static_assert(offsetof(CbmRsPipelinePlanStepV2, phase) == 4, "PlanStepV2.phase offset drift");
_Static_assert(offsetof(CbmRsPipelinePlanStepV2, policy) == 8, "PlanStepV2.policy offset drift");
_Static_assert(offsetof(CbmRsPipelinePlanStepV2, gate_flags) == 12,
               "PlanStepV2.gate_flags offset drift");
_Static_assert(offsetof(CbmRsPipelinePlanStepV2, requires_mask) == 16,
               "PlanStepV2.requires_mask offset drift");
_Static_assert(offsetof(CbmRsPipelinePlanStepV2, effect_flags) == 24,
               "PlanStepV2.effect_flags offset drift");

typedef struct {
    int kind;
    int result_kind;
    uint32_t required_fields;
    uint32_t optional_fields;
    uint32_t effect_flags;
    uint32_t validation_flags;
    uint32_t reserved0;
    uint32_t reserved1;
} CbmRsGbufMutationMetaV1;

typedef cbm_gbuf_mutation_cmd_t CbmRsGbufMutationCmdV1;

typedef struct {
    int ok;
    int error_code;
    uint32_t missing_fields;
    uint32_t invalid_fields;
    uint32_t normalized_flags;
    uint32_t reserved0;
} CbmRsGbufMutationValidationV1;

_Static_assert(sizeof(CbmRsGbufMutationMetaV1) == 32, "GbufMutationMetaV1 ABI size drift");
_Static_assert(offsetof(CbmRsGbufMutationMetaV1, kind) == 0,
               "GbufMutationMetaV1.kind offset drift");
_Static_assert(offsetof(CbmRsGbufMutationMetaV1, result_kind) == 4,
               "GbufMutationMetaV1.result_kind offset drift");
_Static_assert(offsetof(CbmRsGbufMutationMetaV1, required_fields) == 8,
               "GbufMutationMetaV1.required_fields offset drift");
_Static_assert(offsetof(CbmRsGbufMutationMetaV1, reserved1) == 28,
               "GbufMutationMetaV1.reserved1 offset drift");
#if UINTPTR_MAX == UINT64_MAX
_Static_assert(sizeof(CbmRsGbufMutationCmdV1) == 80, "GbufMutationCmdV1 ABI size drift");
_Static_assert(sizeof(CbmRsGbufMutationCmdV1) == sizeof(cbm_gbuf_mutation_cmd_t),
               "GbufMutationCmdV1 C adapter ABI size drift");
_Static_assert(offsetof(CbmRsGbufMutationCmdV1, label) == 8,
               "GbufMutationCmdV1.label offset drift");
_Static_assert(offsetof(CbmRsGbufMutationCmdV1, start_line) == 40,
               "GbufMutationCmdV1.start_line offset drift");
_Static_assert(offsetof(CbmRsGbufMutationCmdV1, source_id) == 48,
               "GbufMutationCmdV1.source_id offset drift");
_Static_assert(offsetof(CbmRsGbufMutationCmdV1, properties_json) == 72,
               "GbufMutationCmdV1.properties_json offset drift");
#endif
_Static_assert(sizeof(CbmRsGbufMutationValidationV1) == 24,
               "GbufMutationValidationV1 ABI size drift");
_Static_assert(offsetof(CbmRsGbufMutationValidationV1, missing_fields) == 8,
               "GbufMutationValidationV1.missing_fields offset drift");

extern int cbm_rs_pipeline_incremental_post_step_count(int mode);
extern int cbm_rs_pipeline_incremental_post_steps(int mode, CbmRsPipelinePlanStep *out, int cap);
extern int cbm_rs_pipeline_plan_step_count_v2(int kind, int mode, int worker_count, int file_count);
extern int cbm_rs_pipeline_plan_steps_v2(int kind, int mode, int worker_count, int file_count,
                                         CbmRsPipelinePlanStepV2 *out, int cap);
extern int cbm_rs_gbuf_mutation_command_count_v1(void);
extern int cbm_rs_gbuf_mutation_commands_v1(CbmRsGbufMutationMetaV1 *out, int cap);
extern int cbm_rs_gbuf_mutation_validate_v1(const CbmRsGbufMutationCmdV1 *cmd,
                                            CbmRsGbufMutationValidationV1 *out);

typedef struct {
    const char *name;
    const char *qualified_name;
    const char *label;
} CbmRsRegistryEntry;

typedef struct {
    double confidence;
    int candidate_count;
    int matched;
} CbmRsRegistryResolveOut;

typedef struct CbmRsRegistryHandle CbmRsRegistryHandle;

extern int cbm_rs_registry_resolve_once(const CbmRsRegistryEntry *entries, int entry_count,
                                        const char *callee_name, const char *module_qn,
                                        const char **import_keys, const char **import_vals,
                                        int import_count, char *qn_buf, size_t qn_buf_size,
                                        char *strategy_buf, size_t strategy_buf_size,
                                        CbmRsRegistryResolveOut *out);
extern CbmRsRegistryHandle *cbm_rs_registry_create(void);
extern void cbm_rs_registry_free(CbmRsRegistryHandle *registry);
extern int cbm_rs_registry_add(CbmRsRegistryHandle *registry, const char *qualified_name);
extern int cbm_rs_registry_resolve(const CbmRsRegistryHandle *registry, const char *callee_name,
                                   const char *module_qn, const char **import_keys,
                                   const char **import_vals, int import_count, char *qn_buf,
                                   size_t qn_buf_size, char *strategy_buf, size_t strategy_buf_size,
                                   CbmRsRegistryResolveOut *out);

static void fail(const char *name, const char *detail) {
    fprintf(stderr, "%s: %s\n", name, detail);
    exit(1);
}

static void check_bool(const char *name, int actual, bool expected) {
    bool actual_bool = actual != 0;
    if (actual_bool != expected) {
        fprintf(stderr, "%s: expected %s, got %s\n", name, expected ? "true" : "false",
                actual_bool ? "true" : "false");
        exit(1);
    }
}

static void check_int(const char *name, int actual, int expected) {
    if (actual != expected) {
        fprintf(stderr, "%s: expected %d, got %d\n", name, expected, actual);
        exit(1);
    }
}

static void check_u32(const char *name, unsigned int actual, unsigned int expected) {
    if (actual != expected) {
        fprintf(stderr, "%s: expected %u, got %u\n", name, expected, actual);
        exit(1);
    }
}

static void check_u64(const char *name, uint64_t actual, uint64_t expected) {
    if (actual != expected) {
        fprintf(stderr, "%s: expected %" PRIu64 ", got %" PRIu64 "\n", name, expected, actual);
        exit(1);
    }
}

static void check_size(const char *name, size_t actual, size_t expected) {
    if (actual != expected) {
        fprintf(stderr, "%s: expected %zu, got %zu\n", name, expected, actual);
        exit(1);
    }
}

static void check_double_near(const char *name, double actual, double expected) {
    if (fabs(actual - expected) > 0.000001) {
        fprintf(stderr, "%s: expected %.6f, got %.6f\n", name, expected, actual);
        exit(1);
    }
}

static void check_str(const char *name, const char *actual, const char *expected) {
    if (!actual || strcmp(actual, expected) != 0) {
        fprintf(stderr, "%s: expected '%s', got '%s'\n", name, expected,
                actual ? actual : "(null)");
        exit(1);
    }
}

static void check_null(const char *name, const void *ptr) {
    if (ptr != NULL) {
        fprintf(stderr, "%s: expected NULL, got %p\n", name, ptr);
        exit(1);
    }
}

static void check_not_null(const char *name, const void *ptr) {
    if (ptr == NULL) {
        fail(name, "expected non-NULL pointer");
    }
}

static void check_ptr_eq(const char *name, const void *actual, const void *expected) {
    if (actual != expected) {
        fprintf(stderr, "%s: expected %p, got %p\n", name, expected, actual);
        exit(1);
    }
}

static void check_ptr_ne(const char *name, const void *actual, const void *unexpected) {
    if (actual == unexpected) {
        fprintf(stderr, "%s: expected pointer different from %p\n", name, unexpected);
        exit(1);
    }
}

typedef struct {
    const char *name;
    char *value;
    bool was_set;
} EnvSnapshot;

static char *dup_cstr(const char *value) {
    size_t len = strlen(value) + 1;
    char *copy = (char *)malloc(len);
    check_not_null("env_snapshot_alloc", copy);
    memcpy(copy, value, len);
    return copy;
}

static int ffi_setenv(const char *name, const char *value) {
#ifdef _WIN32
    return _putenv_s(name, value);
#else
    return setenv(name, value, 1);
#endif
}

static int ffi_unsetenv(const char *name) {
#ifdef _WIN32
    return _putenv_s(name, "");
#else
    return unsetenv(name);
#endif
}

static void env_set_checked(const char *name, const char *value) {
    if (ffi_setenv(name, value) != 0) {
        fail("env_set", name);
    }
}

static void env_unset_checked(const char *name) {
    if (ffi_unsetenv(name) != 0) {
        fail("env_unset", name);
    }
}

static void env_snapshot_save(EnvSnapshot *snapshot, const char *name) {
    const char *value = getenv(name);
    snapshot->name = name;
    snapshot->was_set = value != NULL;
    snapshot->value = value ? dup_cstr(value) : NULL;
}

static void env_snapshot_restore(EnvSnapshot *snapshots, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (snapshots[i].was_set) {
            (void)ffi_setenv(snapshots[i].name, snapshots[i].value ? snapshots[i].value : "");
        } else {
            (void)ffi_unsetenv(snapshots[i].name);
        }
        free(snapshots[i].value);
        snapshots[i].value = NULL;
    }
}

static void fill_repeated(char *buf, size_t bufsize, char ch) {
    if (bufsize == 0) {
        return;
    }
    memset(buf, ch, bufsize - 1);
    buf[bufsize - 1] = '\0';
}

static const unsigned char *intern_stack_value(CInternPool *pool) {
    const unsigned char local[] = {'s', 't', 'a', 'c', 'k'};
    return cbm_rs_intern_n(pool, local, sizeof(local));
}

static void test_dump_verify_exports(void) {
    check_bool("no_baseline", cbm_rs_dump_verify_is_degraded(-1, 500, 0.5, 50), false);
    check_bool("sparse_at_floor", cbm_rs_dump_verify_is_degraded(50, 10, 0.5, 50), false);
    check_bool("custom_floor", cbm_rs_dump_verify_is_degraded(25, 1, 0.5, 24), true);
    check_bool("floor_plus_one_above", cbm_rs_dump_verify_is_degraded(51, 26, 0.5, 50), false);
    check_bool("floor_plus_one_below", cbm_rs_dump_verify_is_degraded(51, 25, 0.5, 50), true);
    check_bool("shortfall", cbm_rs_dump_verify_is_degraded(1000, 400, 0.5, 50), true);
    check_bool("at_ratio", cbm_rs_dump_verify_is_degraded(1000, 500, 0.5, 50), false);
    check_bool("below_ratio", cbm_rs_dump_verify_is_degraded(1000, 499, 0.5, 50), true);
    check_bool("count_error", cbm_rs_dump_verify_is_degraded(1000, -1, 0.5, 50), true);
    check_bool("ratio_zero", cbm_rs_dump_verify_is_degraded(1000, 10, 0.0, 50), false);
    check_bool("ratio_negative", cbm_rs_dump_verify_is_degraded(1000, 10, -0.1, 50), false);
    check_bool("ratio_nan", cbm_rs_dump_verify_is_degraded(1000, 10, NAN, 50), false);
    check_bool("ratio_negative_infinity", cbm_rs_dump_verify_is_degraded(1000, 10, -INFINITY, 50),
               false);
    check_bool("tight_ratio", cbm_rs_dump_verify_is_degraded(1000, 900, 0.95, 50), true);
    check_bool("growth", cbm_rs_dump_verify_is_degraded(500, 750, 0.5, 50), false);
}

static void test_intern_null_contracts(void) {
    const unsigned char abc[] = {'a', 'b', 'c'};

    cbm_rs_intern_free(NULL);
    check_u32("intern_count_null", cbm_rs_intern_count(NULL), 0);
    check_size("intern_bytes_null", cbm_rs_intern_bytes(NULL), 0);
    check_null("intern_null_pool", cbm_rs_intern_n(NULL, abc, sizeof(abc)));

    CInternPool *pool = cbm_rs_intern_create();
    check_not_null("intern_create", pool);
    check_null("intern_null_value_zero_len", cbm_rs_intern_n(pool, NULL, 0));
    check_null("intern_null_value_nonzero_len", cbm_rs_intern_n(pool, NULL, sizeof(abc)));
    check_u32("intern_count_after_nulls", cbm_rs_intern_count(pool), 0);
    check_size("intern_bytes_after_nulls", cbm_rs_intern_bytes(pool), 0);
    cbm_rs_intern_free(pool);
}

static void test_intern_dedup_and_pool_lifetime(void) {
    const unsigned char hello_world[] = "hello world";
    const unsigned char hello[] = "hello";

    CInternPool *pool = cbm_rs_intern_create();
    check_not_null("dedup_create", pool);

    const unsigned char *prefix = cbm_rs_intern_n(pool, hello_world, 5);
    const unsigned char *full = cbm_rs_intern_n(pool, hello, 5);
    const unsigned char *other = cbm_rs_intern_n(pool, (const unsigned char *)"world", 5);
    const unsigned char *stacked = intern_stack_value(pool);

    check_not_null("dedup_prefix", prefix);
    check_ptr_eq("dedup_prefix_full", prefix, full);
    check_ptr_ne("dedup_other", prefix, other);
    check_not_null("dedup_stack", stacked);
    if (memcmp(prefix, hello, 5) != 0 || prefix[5] != '\0') {
        fail("dedup_prefix_contents", "unexpected bytes");
    }
    if (memcmp(stacked, "stack", 5) != 0 || stacked[5] != '\0') {
        fail("dedup_stack_contents", "unexpected bytes after stack source returned");
    }
    check_u32("dedup_count", cbm_rs_intern_count(pool), 3);
    check_size("dedup_bytes", cbm_rs_intern_bytes(pool), 15);

    cbm_rs_intern_free(pool);
}

static void test_intern_embedded_nul_and_empty_pools(void) {
    const unsigned char raw1[] = {'a', '\0', 'b', 'x'};
    const unsigned char raw2[] = {'a', '\0', 'b', 'z', 'z'};

    CInternPool *pool = cbm_rs_intern_create();
    CInternPool *other_pool = cbm_rs_intern_create();
    check_not_null("embedded_create", pool);
    check_not_null("embedded_other_create", other_pool);

    const unsigned char *embedded = cbm_rs_intern_n(pool, raw1, 3);
    const unsigned char *embedded_again = cbm_rs_intern_n(pool, raw2, 3);
    const unsigned char *shorter = cbm_rs_intern_n(pool, raw1, 2);
    const unsigned char *empty = cbm_rs_intern_n(pool, raw1, 0);
    const unsigned char *other_empty = cbm_rs_intern_n(other_pool, raw2, 0);

    check_not_null("embedded", embedded);
    check_ptr_eq("embedded_again", embedded, embedded_again);
    check_ptr_ne("embedded_shorter", embedded, shorter);
    check_ptr_ne("embedded_empty_cross_pool", empty, other_empty);
    if (memcmp(embedded, raw1, 3) != 0 || embedded[3] != '\0') {
        fail("embedded_contents", "unexpected bytes");
    }
    if (memcmp(shorter, raw1, 2) != 0 || shorter[2] != '\0') {
        fail("embedded_shorter_contents", "unexpected bytes");
    }
    check_u32("embedded_count", cbm_rs_intern_count(pool), 3);
    check_size("embedded_bytes", cbm_rs_intern_bytes(pool), 5);

    cbm_rs_intern_free(other_pool);
    cbm_rs_intern_free(pool);
}

static void test_intern_pointer_stability_after_growth(void) {
    CInternPool *pool = cbm_rs_intern_create();
    check_not_null("growth_create", pool);

    const unsigned char *sentinel = cbm_rs_intern_n(pool, (const unsigned char *)"sentinel", 8);
    check_not_null("growth_sentinel", sentinel);

    for (int i = 0; i < 10000; i++) {
        unsigned char filler[32];
        int written = snprintf((char *)filler, sizeof(filler), "filler_%06d", i);
        if (written < 0 || (size_t)written >= sizeof(filler)) {
            fail("growth_filler_format", "snprintf failed");
        }
        check_not_null("growth_filler", cbm_rs_intern_n(pool, filler, (size_t)written));
    }

    const unsigned char *again = cbm_rs_intern_n(pool, (const unsigned char *)"sentinel", 8);
    check_ptr_eq("growth_sentinel_stable", sentinel, again);
    if (memcmp(sentinel, "sentinel", 8) != 0 || sentinel[8] != '\0') {
        fail("growth_sentinel_contents", "unexpected bytes after hash growth");
    }

    cbm_rs_intern_free(pool);
}

static void test_intern_rejects_oversized_len_before_reading(void) {
#if SIZE_MAX > UINT32_MAX
    CInternPool *pool = cbm_rs_intern_create();
    check_not_null("oversized_create", pool);

    const unsigned char one = 'x';
    check_null("oversized_len", cbm_rs_intern_n(pool, &one, (size_t)UINT32_MAX + 1u));
    check_u32("oversized_count", cbm_rs_intern_count(pool), 0);
    check_size("oversized_bytes", cbm_rs_intern_bytes(pool), 0);

    cbm_rs_intern_free(pool);
#endif
}

static void test_str_util_exports(void) {
    check_bool("starts_with_true", cbm_rs_str_starts_with("hello world", "hello"), true);
    check_bool("starts_with_empty", cbm_rs_str_starts_with("hello", ""), true);
    check_bool("starts_with_null", cbm_rs_str_starts_with(NULL, "hello"), false);

    check_bool("ends_with_true", cbm_rs_str_ends_with("hello world", "world"), true);
    check_bool("ends_with_empty", cbm_rs_str_ends_with("hello", ""), true);
    check_bool("ends_with_null", cbm_rs_str_ends_with("hello", NULL), false);

    check_bool("contains_true", cbm_rs_str_contains("hello world", "lo wo"), true);
    check_bool("contains_empty", cbm_rs_str_contains("hello", ""), true);
    check_bool("contains_null", cbm_rs_str_contains(NULL, "hello"), false);

    char out[64];
    check_size("path_join_len", cbm_rs_path_join(out, sizeof(out), "src///", "///main.c"), 10);
    check_str("path_join_out", out, "src/main.c");
    check_size("path_join_trunc_len", cbm_rs_path_join(out, 4, "abcdef", "ghi"), 10);
    check_str("path_join_trunc_out", out, "abc");
    check_size("path_join_null", cbm_rs_path_join(out, sizeof(out), NULL, "main.c"), SIZE_MAX);

    const char *parts[] = {"a", "b", "c.txt"};
    check_size("path_join_n_len", cbm_rs_path_join_n(out, sizeof(out), parts, 3), 9);
    check_str("path_join_n_out", out, "a/b/c.txt");
    check_size("path_join_n_empty", cbm_rs_path_join_n(out, sizeof(out), NULL, 0), 0);
    check_str("path_join_n_empty_out", out, "");

    check_str("path_ext", cbm_rs_path_ext("foo.tar.gz"), "gz");
    check_str("path_ext_none", cbm_rs_path_ext("dir.name/file"), "");
    check_str("path_base", cbm_rs_path_base("/a/b/c"), "c");
    check_str("path_base_null", cbm_rs_path_base(NULL), "");

    check_size("path_dir_len", cbm_rs_path_dir(out, sizeof(out), "src/main.c"), 3);
    check_str("path_dir_out", out, "src");
    check_size("path_dir_null_len", cbm_rs_path_dir(out, sizeof(out), NULL), 1);
    check_str("path_dir_null_out", out, ".");

    check_size("tolower_len", cbm_rs_str_tolower(out, sizeof(out), "Hello"), 5);
    check_str("tolower_out", out, "hello");
    check_size("tolower_null", cbm_rs_str_tolower(out, sizeof(out), NULL), SIZE_MAX);

    check_size("replace_len", cbm_rs_str_replace_char(out, sizeof(out), "a/b/c", '/', '.'), 5);
    check_str("replace_out", out, "a.b.c");
    check_size("strip_len", cbm_rs_str_strip_ext(out, sizeof(out), "foo.tar.gz"), 7);
    check_str("strip_out", out, "foo.tar");

    check_size("split_count", cbm_rs_str_split_count("/a//b/", '/'), 5);
    check_size("split_part_0_len", cbm_rs_str_split_part(out, sizeof(out), "/a//b/", '/', 0), 0);
    check_str("split_part_0_out", out, "");
    check_size("split_part_2_len", cbm_rs_str_split_part(out, sizeof(out), "/a//b/", '/', 2), 0);
    check_str("split_part_2_out", out, "");
    check_size("split_part_3_len", cbm_rs_str_split_part(out, sizeof(out), "/a//b/", '/', 3), 1);
    check_str("split_part_3_out", out, "b");
    check_size("split_part_oob", cbm_rs_str_split_part(out, sizeof(out), "/a//b/", '/', 5),
               SIZE_MAX);

    check_bool("shell_safe", cbm_rs_validate_shell_arg("hello world_123"), true);
    check_bool("shell_quote", cbm_rs_validate_shell_arg("it's bad"), false);
#ifdef _WIN32
    check_bool("shell_backslash", cbm_rs_validate_shell_arg("path\\to\\file"), true);
#else
    check_bool("shell_backslash", cbm_rs_validate_shell_arg("path\\to\\file"), false);
#endif

    check_bool("project_safe", cbm_rs_validate_project_name("project-1.test"), true);
    check_bool("project_leading_dot", cbm_rs_validate_project_name(".hidden"), false);
    check_bool("project_dotdot", cbm_rs_validate_project_name("bad..name"), false);
    check_bool("project_slash", cbm_rs_validate_project_name("bad/name"), false);

    char buf[64];
    int len = cbm_rs_json_escape(buf, (int)sizeof(buf),
                                 "A\x01"
                                 "B\n");
    if (strcmp(buf, "A\\u0001B\\n") != 0 || len != 10) {
        fail("json_escape_control", "unexpected escaped output");
    }
    len = cbm_rs_json_escape(buf, 4, "abcdef");
    if (strcmp(buf, "abc") != 0 || len != 3) {
        fail("json_escape_truncate", "unexpected truncated output");
    }
    len = cbm_rs_json_escape(buf, (int)sizeof(buf), NULL);
    if (strcmp(buf, "") != 0 || len != 0) {
        fail("json_escape_null_src", "unexpected null-src output");
    }
    check_size("json_escape_null_buf", (size_t)cbm_rs_json_escape(NULL, 8, "abc"), 0);
    check_size("json_escape_zero_size", (size_t)cbm_rs_json_escape(buf, 0, "abc"), 0);
}

static void test_platform_normalize_export(void) {
    char win_path[] = "c:\\Users\\test";
    check_ptr_eq("normalize_ptr", cbm_rs_normalize_path_sep(win_path), win_path);
    if (strcmp(win_path, "C:/Users/test") != 0) {
        fail("normalize_win_path", "unexpected normalized path");
    }

    char bare_drive[] = "z:";
    check_ptr_eq("normalize_bare_drive_ptr", cbm_rs_normalize_path_sep(bare_drive), bare_drive);
    if (strcmp(bare_drive, "Z:") != 0) {
        fail("normalize_bare_drive", "unexpected normalized drive");
    }

    char non_drive[] = "abc:def\\x";
    check_ptr_eq("normalize_non_drive_ptr", cbm_rs_normalize_path_sep(non_drive), non_drive);
    if (strcmp(non_drive, "abc:def/x") != 0) {
        fail("normalize_non_drive", "unexpected non-drive path");
    }

    check_null("normalize_null", cbm_rs_normalize_path_sep(NULL));
}

static void test_platform_env_dirs_exports(void) {
    const char *names[] = {"HOME",         "USERPROFILE",   "XDG_CONFIG_HOME", "APPDATA",
                           "LOCALAPPDATA", "CBM_CACHE_DIR", "CBM_RS_FFI_ENV"};
    EnvSnapshot snapshots[sizeof(names) / sizeof(names[0])];
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        env_snapshot_save(&snapshots[i], names[i]);
    }

    char out[64];
    env_unset_checked("CBM_RS_FFI_ENV");
    check_size("safe_getenv_fallback_len",
               cbm_rs_safe_getenv(out, sizeof(out), "CBM_RS_FFI_ENV", "fallback"), 8);
    check_str("safe_getenv_fallback_out", out, "fallback");

    env_set_checked("CBM_RS_FFI_ENV", "abcdef");
    check_size("safe_getenv_len", cbm_rs_safe_getenv(out, 4, "CBM_RS_FFI_ENV", NULL), 6);
    check_str("safe_getenv_trunc_out", out, "abc");
    check_size("safe_getenv_null_buf", cbm_rs_safe_getenv(NULL, 0, "CBM_RS_FFI_ENV", NULL), 6);

    env_unset_checked("CBM_RS_FFI_ENV");
    check_size("safe_getenv_missing", cbm_rs_safe_getenv(out, sizeof(out), "CBM_RS_FFI_ENV", NULL),
               SIZE_MAX);
    check_size("safe_getenv_null_name", cbm_rs_safe_getenv(out, sizeof(out), NULL, "fallback"),
               SIZE_MAX);

    env_set_checked("HOME", "c:\\Home\\Primary");
    env_set_checked("USERPROFILE", "d:\\Profile");
    env_unset_checked("XDG_CONFIG_HOME");
    env_unset_checked("APPDATA");
    env_unset_checked("LOCALAPPDATA");
    env_unset_checked("CBM_CACHE_DIR");

    check_size("home_len", cbm_rs_get_home_dir(out, sizeof(out)), strlen("C:/Home/Primary"));
    check_str("home_out", out, "C:/Home/Primary");
    check_size("home_null_buf", cbm_rs_get_home_dir(NULL, 0), strlen("C:/Home/Primary"));

#ifdef _WIN32
    check_size("config_fallback_len", cbm_rs_app_config_dir(out, sizeof(out)),
               strlen("C:/Home/Primary/AppData/Roaming"));
    check_str("config_fallback_out", out, "C:/Home/Primary/AppData/Roaming");
    check_size("local_fallback_len", cbm_rs_app_local_dir(out, sizeof(out)),
               strlen("C:/Home/Primary/AppData/Local"));
    check_str("local_fallback_out", out, "C:/Home/Primary/AppData/Local");

    env_set_checked("APPDATA", "e:\\Config\\Roaming");
    env_set_checked("LOCALAPPDATA", "f:\\Config\\Local");
    check_size("config_override_len", cbm_rs_app_config_dir(out, sizeof(out)),
               strlen("E:/Config/Roaming"));
    check_str("config_override_out", out, "E:/Config/Roaming");
    check_size("local_override_len", cbm_rs_app_local_dir(out, sizeof(out)),
               strlen("F:/Config/Local"));
    check_str("local_override_out", out, "F:/Config/Local");
#else
    check_size("config_fallback_len", cbm_rs_app_config_dir(out, sizeof(out)),
               strlen("C:/Home/Primary/.config"));
    check_str("config_fallback_out", out, "C:/Home/Primary/.config");
    check_size("local_fallback_len", cbm_rs_app_local_dir(out, sizeof(out)),
               strlen("C:/Home/Primary/.config"));
    check_str("local_fallback_out", out, "C:/Home/Primary/.config");

    env_set_checked("XDG_CONFIG_HOME", "/tmp/cbm-rs-config");
    check_size("config_override_len", cbm_rs_app_config_dir(out, sizeof(out)),
               strlen("/tmp/cbm-rs-config"));
    check_str("config_override_out", out, "/tmp/cbm-rs-config");
    check_size("local_override_len", cbm_rs_app_local_dir(out, sizeof(out)),
               strlen("/tmp/cbm-rs-config"));
    check_str("local_override_out", out, "/tmp/cbm-rs-config");
#endif

    env_set_checked("CBM_CACHE_DIR", "g:\\Cache\\Root");
    check_size("cache_override_len", cbm_rs_resolve_cache_dir(out, sizeof(out)),
               strlen("G:/Cache/Root"));
    check_str("cache_override_out", out, "G:/Cache/Root");

    env_unset_checked("CBM_CACHE_DIR");
    check_size("cache_fallback_len", cbm_rs_resolve_cache_dir(out, sizeof(out)),
               strlen("C:/Home/Primary/.cache/codebase-memory-mcp"));
    check_str("cache_fallback_out", out, "C:/Home/Primary/.cache/codebase-memory-mcp");

    char long_home[301];
    fill_repeated(long_home, sizeof(long_home), 'a');
    env_set_checked("HOME", long_home);
    env_unset_checked("USERPROFILE");
    env_unset_checked("XDG_CONFIG_HOME");
    env_unset_checked("APPDATA");
    env_unset_checked("LOCALAPPDATA");
    check_size("home_trunc_full_len", cbm_rs_get_home_dir(out, sizeof(out)), 255);
    check_size("home_trunc_out_len", strlen(out), sizeof(out) - 1);

    env_unset_checked("HOME");
    check_size("home_missing_len", cbm_rs_get_home_dir(out, sizeof(out)), SIZE_MAX);

    env_snapshot_restore(snapshots, sizeof(snapshots) / sizeof(snapshots[0]));
}

#ifndef _WIN32
static void write_temp_file(const char *root, const char *name, const char *content) {
    char path[512];
    int written = snprintf(path, sizeof(path), "%s/%s", root, name);
    if (written < 0 || (size_t)written >= sizeof(path)) {
        fail("cgroup_path", "path buffer too small");
    }
    FILE *fp = fopen(path, "w");
    check_not_null("cgroup_fopen", fp);
    size_t len = strlen(content);
    if (fwrite(content, 1, len, fp) != len) {
        fclose(fp);
        fail("cgroup_write", "write failed");
    }
    fclose(fp);
}

static void test_platform_cgroup_exports(void) {
    char root[] = "/tmp/cbm_rs_cgroup_XXXXXX";
    check_not_null("cgroup_mkdtemp", mkdtemp(root));

    write_temp_file(root, "cpu.max", "150000 100000\n");
    write_temp_file(root, "memory.max", "2147483648\n");

    check_size("cgroup_cpu", (size_t)cbm_rs_detect_cgroup_cpus(root), 2);
    check_size("cgroup_mem", cbm_rs_detect_cgroup_mem(root), (size_t)2147483648UL);

    char cpu_path[512];
    char mem_path[512];
    snprintf(cpu_path, sizeof(cpu_path), "%s/cpu.max", root);
    snprintf(mem_path, sizeof(mem_path), "%s/memory.max", root);
    (void)unlink(cpu_path);
    (void)unlink(mem_path);
    (void)rmdir(root);
}
#endif

static CbmRsDiagSnapshot diagnostics_fixture_snapshot(void) {
    CbmRsDiagSnapshot snapshot = {
        .uptime_s = 17,
        .rss_bytes = 1024,
        .peak_rss_bytes = 2048,
        .heap_committed_bytes = 4096,
        .peak_committed_bytes = 8192,
        .page_faults = 23,
        .fd_count = 9,
        .queries =
            {
                .count = 3,
                .errors = 1,
                .total_us = 425,
                .avg_us = 141,
                .max_us = 250,
            },
        .pid = 4242,
    };
    return snapshot;
}

static void test_diagnostics_exports(void) {
    check_bool("diag_env_one", cbm_rs_diag_env_enabled_value("1"), true);
    check_bool("diag_env_true", cbm_rs_diag_env_enabled_value("true"), true);
    check_bool("diag_env_null", cbm_rs_diag_env_enabled_value(NULL), false);
    check_bool("diag_env_empty", cbm_rs_diag_env_enabled_value(""), false);
    check_bool("diag_env_false", cbm_rs_diag_env_enabled_value("false"), false);
    check_bool("diag_env_upper", cbm_rs_diag_env_enabled_value("TRUE"), false);
    check_bool("diag_env_space", cbm_rs_diag_env_enabled_value(" true"), false);

    CbmRsDiagQuerySnapshot query = {0};
    check_int("diag_query_snapshot_rc", cbm_rs_diag_query_snapshot_values(3, 1, 425, 250, &query),
              0);
    check_int("diag_query_count", query.count, 3);
    check_int("diag_query_errors", query.errors, 1);
    check_size("diag_query_total", (size_t)query.total_us, 425);
    check_size("diag_query_avg", (size_t)query.avg_us, 141);
    check_size("diag_query_max", (size_t)query.max_us, 250);
    check_int("diag_query_null_out", cbm_rs_diag_query_snapshot_values(3, 1, 425, 250, NULL), -1);

    check_int("diag_query_zero_rc", cbm_rs_diag_query_snapshot_values(0, 0, 425, 250, &query), 0);
    check_size("diag_query_zero_avg", (size_t)query.avg_us, 0);

    char path[128];
    char small[8];
    check_int("diag_path_len", cbm_rs_diag_format_path(path, sizeof(path), "/tmp", 1234, "json"),
              (int)strlen("/tmp/cbm-diagnostics-1234.json"));
    check_str("diag_path_out", path, "/tmp/cbm-diagnostics-1234.json");
    check_int("diag_path_small",
              cbm_rs_diag_format_path(small, sizeof(small), "/tmp", 1234, "ndjson"), -1);
    check_int("diag_path_null", cbm_rs_diag_format_path(NULL, sizeof(path), "/tmp", 1234, "json"),
              -1);

    CbmRsDiagSnapshot snapshot = diagnostics_fixture_snapshot();
    char json[1024];
    const char *expected_json = "{\n"
                                "  \"uptime_s\": 17,\n"
                                "  \"rss_bytes\": 1024,\n"
                                "  \"peak_rss_bytes\": 2048,\n"
                                "  \"heap_committed_bytes\": 4096,\n"
                                "  \"peak_committed_bytes\": 8192,\n"
                                "  \"page_faults\": 23,\n"
                                "  \"fd_count\": 9,\n"
                                "  \"query_count\": 3,\n"
                                "  \"query_errors\": 1,\n"
                                "  \"query_total_us\": 425,\n"
                                "  \"query_avg_us\": 141,\n"
                                "  \"query_max_us\": 250,\n"
                                "  \"pid\": 4242\n"
                                "}\n";
    check_int("diag_json_len", cbm_rs_diag_format_json(json, sizeof(json), &snapshot),
              (int)strlen(expected_json));
    check_str("diag_json_out", json, expected_json);
    check_int("diag_json_small", cbm_rs_diag_format_json(small, sizeof(small), &snapshot), -1);
    check_int("diag_json_null_snapshot", cbm_rs_diag_format_json(json, sizeof(json), NULL), -1);

    char line[256];
    const char *expected_ndjson =
        "{\"uptime_s\":17,\"rss\":1024,\"peak_rss\":2048,\"committed\":4096,"
        "\"peak_committed\":8192,\"page_faults\":23,\"fd\":9,\"queries\":3}\n";
    check_int("diag_ndjson_len", cbm_rs_diag_format_ndjson(line, sizeof(line), &snapshot),
              (int)strlen(expected_ndjson));
    check_str("diag_ndjson_out", line, expected_ndjson);
    check_int("diag_ndjson_small", cbm_rs_diag_format_ndjson(small, sizeof(small), &snapshot), -1);
    check_int("diag_ndjson_null_snapshot", cbm_rs_diag_format_ndjson(line, sizeof(line), NULL), -1);
}

static void test_pipeline_plan_exports(void) {
    enum {
        PLAN_SEQUENTIAL = CBM_RS_PLAN_SEQUENTIAL,
        PLAN_PREDUMP = CBM_RS_PLAN_PREDUMP,
        PLAN_EXTRACTION_CHOICE = CBM_RS_PLAN_EXTRACTION_CHOICE,
        PLAN_INCREMENTAL_EXTRACT_RESOLVE = CBM_RS_PLAN_INCREMENTAL_EXTRACT_RESOLVE,
        PLAN_INCREMENTAL_POST = CBM_RS_PLAN_INCREMENTAL_POST,
        PLAN_PARALLEL_EXTRACTION = CBM_RS_PLAN_PARALLEL_EXTRACTION,
        PLAN_FULL_PIPELINE = CBM_RS_PLAN_FULL_PIPELINE,
        PLAN_STEP_PREDUMP_DECORATOR_TAGS = CBM_RS_PLAN_STEP_PREDUMP_DECORATOR_TAGS,
        PLAN_STEP_PREDUMP_CONFIGLINK = CBM_RS_PLAN_STEP_PREDUMP_CONFIGLINK,
        PLAN_STEP_PREDUMP_ROUTE_MATCH = CBM_RS_PLAN_STEP_PREDUMP_ROUTE_MATCH,
        PLAN_STEP_PREDUMP_SIMILARITY = CBM_RS_PLAN_STEP_PREDUMP_SIMILARITY,
        PLAN_STEP_PREDUMP_SEMANTIC_EDGES = CBM_RS_PLAN_STEP_PREDUMP_SEMANTIC_EDGES,
        PLAN_STEP_PREDUMP_COMPLEXITY = CBM_RS_PLAN_STEP_PREDUMP_COMPLEXITY,
        PLAN_STEP_SEQUENTIAL_DEFINITIONS = CBM_RS_PLAN_STEP_SEQUENTIAL_DEFINITIONS,
        PLAN_STEP_SEQUENTIAL_K8S = CBM_RS_PLAN_STEP_SEQUENTIAL_K8S,
        PLAN_STEP_SEQUENTIAL_LSP_CROSS = CBM_RS_PLAN_STEP_SEQUENTIAL_LSP_CROSS,
        PLAN_STEP_SEQUENTIAL_CALLS = CBM_RS_PLAN_STEP_SEQUENTIAL_CALLS,
        PLAN_STEP_SEQUENTIAL_USAGES = CBM_RS_PLAN_STEP_SEQUENTIAL_USAGES,
        PLAN_STEP_SEQUENTIAL_SEMANTIC = CBM_RS_PLAN_STEP_SEQUENTIAL_SEMANTIC,
        PLAN_STEP_PARALLEL_EXTRACT = CBM_RS_PLAN_STEP_PARALLEL_EXTRACT,
        PLAN_STEP_PARALLEL_REGISTRY_BUILD = CBM_RS_PLAN_STEP_PARALLEL_REGISTRY_BUILD,
        PLAN_STEP_PARALLEL_LSP_CROSS_PREPARE = CBM_RS_PLAN_STEP_PARALLEL_LSP_CROSS_PREPARE,
        PLAN_STEP_PARALLEL_RESOLVE = CBM_RS_PLAN_STEP_PARALLEL_RESOLVE,
        PLAN_STEP_PARALLEL_INFRA_ROUTES = CBM_RS_PLAN_STEP_PARALLEL_INFRA_ROUTES,
        PLAN_STEP_PARALLEL_INFRA_BINDINGS = CBM_RS_PLAN_STEP_PARALLEL_INFRA_BINDINGS,
        PLAN_STEP_PARALLEL_K8S = CBM_RS_PLAN_STEP_PARALLEL_K8S,
        PLAN_STEP_INCREMENTAL_DEFINITIONS = CBM_RS_PLAN_STEP_INCREMENTAL_DEFINITIONS,
        PLAN_STEP_INCREMENTAL_CALLS = CBM_RS_PLAN_STEP_INCREMENTAL_CALLS,
        PLAN_STEP_INCREMENTAL_USAGES = CBM_RS_PLAN_STEP_INCREMENTAL_USAGES,
        PLAN_STEP_INCREMENTAL_SEMANTIC = CBM_RS_PLAN_STEP_INCREMENTAL_SEMANTIC,
        PLAN_STEP_INCREMENTAL_PARALLEL_EXTRACT = CBM_RS_PLAN_STEP_INCREMENTAL_PARALLEL_EXTRACT,
        PLAN_STEP_INCREMENTAL_REGISTRY = CBM_RS_PLAN_STEP_INCREMENTAL_REGISTRY,
        PLAN_STEP_INCREMENTAL_RESOLVE = CBM_RS_PLAN_STEP_INCREMENTAL_RESOLVE,
        PLAN_PHASE_PREDUMP = CBM_RS_PLAN_PHASE_PREDUMP,
        PLAN_PHASE_SEQUENTIAL_EXTRACT = CBM_RS_PLAN_PHASE_SEQUENTIAL_EXTRACT,
        PLAN_PHASE_PARALLEL_EXTRACT = CBM_RS_PLAN_PHASE_PARALLEL_EXTRACT,
        PLAN_PHASE_INCREMENTAL_EXTRACT_RESOLVE = CBM_RS_PLAN_PHASE_INCREMENTAL_EXTRACT_RESOLVE,
        PLAN_POLICY_REQUIRED = CBM_RS_PLAN_POLICY_REQUIRED,
        INCR_POST_K8S = CBM_RS_INCR_POST_K8S,
        INCR_POST_TESTS = CBM_RS_INCR_POST_TESTS,
        INCR_POST_DECORATOR_TAGS = CBM_RS_INCR_POST_DECORATOR_TAGS,
        INCR_POST_CONFIGLINK = CBM_RS_INCR_POST_CONFIGLINK,
        INCR_POST_SIMILARITY = CBM_RS_INCR_POST_SIMILARITY,
        INCR_POST_SEMANTIC_EDGES = CBM_RS_INCR_POST_SEMANTIC_EDGES,
        INCR_POST_EDGE_RELINK = CBM_RS_INCR_POST_EDGE_RELINK,
        INCR_POST_INCREMENTAL_DUMP = CBM_RS_INCR_POST_INCREMENTAL_DUMP,
        INCR_POST_PERSIST_HASHES = CBM_RS_INCR_POST_PERSIST_HASHES,
        INCR_POST_ARTIFACT_EXPORT = CBM_RS_INCR_POST_ARTIFACT_EXPORT,
        PLAN_POLICY_IGNORE_ERR = CBM_RS_PLAN_POLICY_IGNORE_ERR,
        PLAN_POLICY_BEST_EFFORT = CBM_RS_PLAN_POLICY_BEST_EFFORT,
        PLAN_POLICY_OPTIONAL_EXISTING_ARTIFACT = CBM_RS_PLAN_POLICY_OPTIONAL_EXISTING_ARTIFACT,
        PLAN_POLICY_ENV_OPTIONAL = CBM_RS_PLAN_POLICY_ENV_OPTIONAL,
        PLAN_GATE_SKIP_FAST = CBM_RS_PLAN_GATE_SKIP_FAST,
        PLAN_GATE_REQUIRES_RESULT_CACHE = CBM_RS_PLAN_GATE_REQUIRES_RESULT_CACHE,
        PLAN_GATE_NO_CROSS_LSP_PREBUILD = CBM_RS_PLAN_GATE_NO_CROSS_LSP_PREBUILD,
        PLAN_EFFECT_MUTATES_GRAPH = CBM_RS_PLAN_EFFECT_MUTATES_GRAPH,
        MODE_FULL = CBM_MODE_FULL,
        MODE_MODERATE = CBM_MODE_MODERATE,
        MODE_FAST = CBM_MODE_FAST,
    };
    char out[1024];
    char small[8];
    CbmRsPipelinePlanStep steps[10];
    CbmRsPipelinePlanStep short_steps[9];
    CbmRsPipelinePlanStepV2 steps_v2[7];
    CbmRsPipelinePlanStepV2 short_steps_v2[6];

    const char *seq =
        "definitions:required,k8s:ignore_err,lsp_cross:ignore_err:requires_result_cache,"
        "calls:required,usages:required,semantic:required,infra_routes:after_success,"
        "infra_bindings:after_success";
    check_int("plan_seq_len",
              cbm_rs_pipeline_plan_describe(PLAN_SEQUENTIAL, MODE_FULL, 0, 0, out, sizeof(out)),
              (int)strlen(seq));
    check_str("plan_seq_out", out, seq);

    const char *predump_full =
        "decorator_tags,configlink,route_match,similarity,semantic_edges,complexity";
    check_int("plan_predump_full_len",
              cbm_rs_pipeline_plan_describe(PLAN_PREDUMP, MODE_FULL, 0, 0, out, sizeof(out)),
              (int)strlen(predump_full));
    check_str("plan_predump_full_out", out, predump_full);

    const char *predump_fast = "decorator_tags,configlink,route_match,complexity";
    check_int("plan_predump_fast_len",
              cbm_rs_pipeline_plan_describe(PLAN_PREDUMP, MODE_FAST, 0, 0, out, sizeof(out)),
              (int)strlen(predump_fast));
    check_str("plan_predump_fast_out", out, predump_fast);
    check_int("plan_predump_moderate_len",
              cbm_rs_pipeline_plan_describe(PLAN_PREDUMP, MODE_MODERATE, 0, 0, out, sizeof(out)),
              (int)strlen(predump_full));
    check_str("plan_predump_moderate_out", out, predump_full);
    check_int("plan_predump_advanced_len",
              cbm_rs_pipeline_plan_describe(PLAN_PREDUMP, 3, 0, 0, out, sizeof(out)),
              (int)strlen(predump_full));
    check_str("plan_predump_advanced_out", out, predump_full);

    check_int(
        "plan_threshold_at_50",
        cbm_rs_pipeline_plan_describe(PLAN_EXTRACTION_CHOICE, MODE_FULL, 2, 50, out, sizeof(out)),
        (int)strlen("sequential"));
    check_str("plan_threshold_at_50_out", out, "sequential");
    check_int(
        "plan_threshold_51",
        cbm_rs_pipeline_plan_describe(PLAN_EXTRACTION_CHOICE, MODE_FULL, 2, 51, out, sizeof(out)),
        (int)strlen("parallel"));
    check_str("plan_threshold_51_out", out, "parallel");
    check_int(
        "plan_single_worker",
        cbm_rs_pipeline_plan_describe(PLAN_EXTRACTION_CHOICE, MODE_FULL, 1, 51, out, sizeof(out)),
        (int)strlen("sequential"));
    check_str("plan_single_worker_out", out, "sequential");
    check_int(
        "plan_zero_worker",
        cbm_rs_pipeline_plan_describe(PLAN_EXTRACTION_CHOICE, MODE_FULL, 0, 51, out, sizeof(out)),
        (int)strlen("sequential"));
    check_str("plan_zero_worker_out", out, "sequential");
    check_int(
        "plan_negative_worker",
        cbm_rs_pipeline_plan_describe(PLAN_EXTRACTION_CHOICE, MODE_FULL, -1, 51, out, sizeof(out)),
        (int)strlen("sequential"));
    check_str("plan_negative_worker_out", out, "sequential");
    check_int(
        "plan_negative_file_count",
        cbm_rs_pipeline_plan_describe(PLAN_EXTRACTION_CHOICE, MODE_FULL, 2, -1, out, sizeof(out)),
        (int)strlen("sequential"));
    check_str("plan_negative_file_count_out", out, "sequential");

    const char *parallel =
        "parallel_extract:required,registry_build:required,lsp_cross_prepare:env_optional,"
        "parallel_resolve:required,infra_routes:required,infra_bindings:required,k8s:ignore_err";
    check_int(
        "plan_parallel_len",
        cbm_rs_pipeline_plan_describe(PLAN_PARALLEL_EXTRACTION, MODE_FULL, 0, 0, out, sizeof(out)),
        (int)strlen(parallel));
    check_str("plan_parallel_out", out, parallel);

    const char *incr_parallel = "incr_extract:ignore_err,incr_registry:ignore_err,"
                                "incr_resolve:ignore_err:no_cross_lsp_prebuild";
    check_int("plan_incr_parallel_len",
              cbm_rs_pipeline_plan_describe(PLAN_INCREMENTAL_EXTRACT_RESOLVE, MODE_FULL, 2, 51, out,
                                            sizeof(out)),
              (int)strlen(incr_parallel));
    check_str("plan_incr_parallel_out", out, incr_parallel);

    const char *incr_seq =
        "definitions:ignore_err,calls:ignore_err,usages:ignore_err,semantic:ignore_err";
    check_int("plan_incr_seq_len",
              cbm_rs_pipeline_plan_describe(PLAN_INCREMENTAL_EXTRACT_RESOLVE, MODE_FULL, 2, 50, out,
                                            sizeof(out)),
              (int)strlen(incr_seq));
    check_str("plan_incr_seq_out", out, incr_seq);

    const char *incr_post_moderate =
        "k8s:ignore_err,incr_tests:ignore_err,incr_decorator_tags:ignore_err,"
        "incr_configlink:ignore_err,incr_similarity:ignore_err,incr_semantic_edges:ignore_err,"
        "edge_relink:best_effort,incremental_dump:best_effort,persist_hashes:best_effort,"
        "artifact_export:optional_existing_artifact";
    check_int(
        "plan_incr_post_moderate_len",
        cbm_rs_pipeline_plan_describe(PLAN_INCREMENTAL_POST, MODE_MODERATE, 0, 0, out, sizeof(out)),
        (int)strlen(incr_post_moderate));
    check_str("plan_incr_post_moderate_out", out, incr_post_moderate);
    check_int(
        "plan_incr_post_full_len",
        cbm_rs_pipeline_plan_describe(PLAN_INCREMENTAL_POST, MODE_FULL, 0, 0, out, sizeof(out)),
        (int)strlen(incr_post_moderate));
    check_str("plan_incr_post_full_out", out, incr_post_moderate);

    const char *incr_post_fast =
        "k8s:ignore_err,incr_tests:ignore_err,incr_decorator_tags:ignore_err,"
        "incr_configlink:ignore_err,edge_relink:best_effort,incremental_dump:best_effort,"
        "persist_hashes:best_effort,artifact_export:optional_existing_artifact";
    check_int(
        "plan_incr_post_fast_len",
        cbm_rs_pipeline_plan_describe(PLAN_INCREMENTAL_POST, MODE_FAST, 0, 0, out, sizeof(out)),
        (int)strlen(incr_post_fast));
    check_str("plan_incr_post_fast_out", out, incr_post_fast);
    check_int("plan_incr_post_advanced_len",
              cbm_rs_pipeline_plan_describe(PLAN_INCREMENTAL_POST, 3, 0, 0, out, sizeof(out)),
              (int)strlen(incr_post_fast));
    check_str("plan_incr_post_advanced_out", out, incr_post_fast);

    check_int("plan_incr_post_step_count_moderate",
              cbm_rs_pipeline_incremental_post_step_count(MODE_MODERATE), 10);
    check_int("plan_incr_post_steps_moderate_len",
              cbm_rs_pipeline_incremental_post_steps(MODE_MODERATE, steps, 10), 10);
    check_int("plan_incr_post_step_0_kind", steps[0].kind, INCR_POST_K8S);
    check_int("plan_incr_post_step_0_policy", steps[0].policy, PLAN_POLICY_IGNORE_ERR);
    check_int("plan_incr_post_step_5_kind", steps[5].kind, INCR_POST_SEMANTIC_EDGES);
    check_int("plan_incr_post_step_5_policy", steps[5].policy, PLAN_POLICY_IGNORE_ERR);
    check_int("plan_incr_post_step_6_kind", steps[6].kind, INCR_POST_EDGE_RELINK);
    check_int("plan_incr_post_step_6_policy", steps[6].policy, PLAN_POLICY_BEST_EFFORT);
    check_int("plan_incr_post_step_7_kind", steps[7].kind, INCR_POST_INCREMENTAL_DUMP);
    check_int("plan_incr_post_step_8_kind", steps[8].kind, INCR_POST_PERSIST_HASHES);
    check_int("plan_incr_post_step_9_kind", steps[9].kind, INCR_POST_ARTIFACT_EXPORT);
    check_int("plan_incr_post_step_9_policy", steps[9].policy,
              PLAN_POLICY_OPTIONAL_EXISTING_ARTIFACT);

    check_int("plan_incr_post_step_count_fast",
              cbm_rs_pipeline_incremental_post_step_count(MODE_FAST), 8);
    check_int("plan_incr_post_steps_fast_len",
              cbm_rs_pipeline_incremental_post_steps(MODE_FAST, steps, 10), 8);
    check_int("plan_incr_post_fast_tail_0", steps[4].kind, INCR_POST_EDGE_RELINK);
    check_int("plan_incr_post_fast_tail_1", steps[5].kind, INCR_POST_INCREMENTAL_DUMP);
    check_int("plan_incr_post_fast_tail_2", steps[6].kind, INCR_POST_PERSIST_HASHES);
    check_int("plan_incr_post_fast_tail_3", steps[7].kind, INCR_POST_ARTIFACT_EXPORT);
    check_int("plan_incr_post_steps_null",
              cbm_rs_pipeline_incremental_post_steps(MODE_MODERATE, NULL, 10), -1);
    check_int("plan_incr_post_steps_short_cap",
              cbm_rs_pipeline_incremental_post_steps(MODE_MODERATE, short_steps, 9), -1);
    check_int("plan_incr_post_steps_negative_cap",
              cbm_rs_pipeline_incremental_post_steps(MODE_MODERATE, steps, -1), -1);

    check_int(
        "plan_incr_extract_v2_count_sequential",
        cbm_rs_pipeline_plan_step_count_v2(PLAN_INCREMENTAL_EXTRACT_RESOLVE, MODE_FULL, 2, 50), 4);
    check_int("plan_incr_extract_v2_steps_sequential_len",
              cbm_rs_pipeline_plan_steps_v2(PLAN_INCREMENTAL_EXTRACT_RESOLVE, MODE_FULL, 2, 50,
                                            steps_v2, 4),
              4);
    check_int("plan_incr_extract_v2_seq_step0_kind", steps_v2[0].kind,
              PLAN_STEP_INCREMENTAL_DEFINITIONS);
    check_int("plan_incr_extract_v2_seq_step0_phase", steps_v2[0].phase,
              PLAN_PHASE_INCREMENTAL_EXTRACT_RESOLVE);
    check_int("plan_incr_extract_v2_seq_step0_policy", steps_v2[0].policy, PLAN_POLICY_IGNORE_ERR);
    check_u32("plan_incr_extract_v2_seq_step0_gate", steps_v2[0].gate_flags, 0);
    check_u64("plan_incr_extract_v2_seq_step0_requires", steps_v2[0].requires_mask, 0);
    check_u32("plan_incr_extract_v2_seq_step0_effect", steps_v2[0].effect_flags,
              PLAN_EFFECT_MUTATES_GRAPH);
    check_int("plan_incr_extract_v2_seq_step3_kind", steps_v2[3].kind,
              PLAN_STEP_INCREMENTAL_SEMANTIC);
    check_u64("plan_incr_extract_v2_seq_step3_requires", steps_v2[3].requires_mask,
              (UINT64_C(1) << (PLAN_STEP_INCREMENTAL_DEFINITIONS - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_INCREMENTAL_CALLS - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_INCREMENTAL_USAGES - 1)));

    check_int(
        "plan_incr_extract_v2_count_parallel",
        cbm_rs_pipeline_plan_step_count_v2(PLAN_INCREMENTAL_EXTRACT_RESOLVE, MODE_FULL, 2, 51), 3);
    check_int("plan_incr_extract_v2_steps_parallel_len",
              cbm_rs_pipeline_plan_steps_v2(PLAN_INCREMENTAL_EXTRACT_RESOLVE, MODE_FULL, 2, 51,
                                            steps_v2, 3),
              3);
    check_int("plan_incr_extract_v2_parallel_step0_kind", steps_v2[0].kind,
              PLAN_STEP_INCREMENTAL_PARALLEL_EXTRACT);
    check_int("plan_incr_extract_v2_parallel_step0_phase", steps_v2[0].phase,
              PLAN_PHASE_INCREMENTAL_EXTRACT_RESOLVE);
    check_int("plan_incr_extract_v2_parallel_step0_policy", steps_v2[0].policy,
              PLAN_POLICY_IGNORE_ERR);
    check_int("plan_incr_extract_v2_parallel_step2_kind", steps_v2[2].kind,
              PLAN_STEP_INCREMENTAL_RESOLVE);
    check_u32("plan_incr_extract_v2_parallel_step2_gate", steps_v2[2].gate_flags,
              PLAN_GATE_NO_CROSS_LSP_PREBUILD);
    check_u64("plan_incr_extract_v2_parallel_step2_requires", steps_v2[2].requires_mask,
              (UINT64_C(1) << (PLAN_STEP_INCREMENTAL_PARALLEL_EXTRACT - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_INCREMENTAL_REGISTRY - 1)));
    check_u32("plan_incr_extract_v2_parallel_step2_effect", steps_v2[2].effect_flags,
              PLAN_EFFECT_MUTATES_GRAPH);
    check_int("plan_incr_extract_v2_steps_short_cap",
              cbm_rs_pipeline_plan_steps_v2(PLAN_INCREMENTAL_EXTRACT_RESOLVE, MODE_FULL, 2, 51,
                                            short_steps_v2, 2),
              -1);
    check_int(
        "plan_incr_extract_v2_steps_null",
        cbm_rs_pipeline_plan_steps_v2(PLAN_INCREMENTAL_EXTRACT_RESOLVE, MODE_FULL, 2, 51, NULL, 3),
        -1);

    check_int("plan_sequential_v2_count_full",
              cbm_rs_pipeline_plan_step_count_v2(PLAN_SEQUENTIAL, MODE_FULL, 9, 9), 6);
    check_int("plan_sequential_v2_count_fast",
              cbm_rs_pipeline_plan_step_count_v2(PLAN_SEQUENTIAL, MODE_FAST, 0, 0), 6);
    check_int("plan_sequential_v2_steps_full_len",
              cbm_rs_pipeline_plan_steps_v2(PLAN_SEQUENTIAL, MODE_FULL, 9, 9, steps_v2, 6), 6);
    check_int("plan_sequential_v2_step0_kind", steps_v2[0].kind, PLAN_STEP_SEQUENTIAL_DEFINITIONS);
    check_int("plan_sequential_v2_step0_phase", steps_v2[0].phase, PLAN_PHASE_SEQUENTIAL_EXTRACT);
    check_int("plan_sequential_v2_step0_policy", steps_v2[0].policy, PLAN_POLICY_REQUIRED);
    check_int("plan_sequential_v2_step0_gate", (int)steps_v2[0].gate_flags, 0);
    check_u64("plan_sequential_v2_step0_requires", steps_v2[0].requires_mask, 0);
    check_int("plan_sequential_v2_step0_effect", (int)steps_v2[0].effect_flags,
              PLAN_EFFECT_MUTATES_GRAPH);
    check_int("plan_sequential_v2_step1_kind", steps_v2[1].kind, PLAN_STEP_SEQUENTIAL_K8S);
    check_int("plan_sequential_v2_step1_policy", steps_v2[1].policy, PLAN_POLICY_IGNORE_ERR);
    check_u64("plan_sequential_v2_step1_requires", steps_v2[1].requires_mask,
              UINT64_C(1) << (PLAN_STEP_SEQUENTIAL_DEFINITIONS - 1));
    check_int("plan_sequential_v2_step2_kind", steps_v2[2].kind, PLAN_STEP_SEQUENTIAL_LSP_CROSS);
    check_int("plan_sequential_v2_step2_policy", steps_v2[2].policy, PLAN_POLICY_IGNORE_ERR);
    check_int("plan_sequential_v2_step2_gate", (int)steps_v2[2].gate_flags,
              PLAN_GATE_REQUIRES_RESULT_CACHE);
    check_u64("plan_sequential_v2_step2_requires", steps_v2[2].requires_mask,
              (UINT64_C(1) << (PLAN_STEP_SEQUENTIAL_DEFINITIONS - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_SEQUENTIAL_K8S - 1)));
    check_int("plan_sequential_v2_step5_kind", steps_v2[5].kind, PLAN_STEP_SEQUENTIAL_SEMANTIC);
    check_u64("plan_sequential_v2_step5_requires", steps_v2[5].requires_mask,
              (UINT64_C(1) << (PLAN_STEP_SEQUENTIAL_DEFINITIONS - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_SEQUENTIAL_K8S - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_SEQUENTIAL_LSP_CROSS - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_SEQUENTIAL_CALLS - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_SEQUENTIAL_USAGES - 1)));

    check_int("plan_predump_v2_count_full",
              cbm_rs_pipeline_plan_step_count_v2(PLAN_PREDUMP, MODE_FULL, 9, 9), 6);
    check_int("plan_predump_v2_count_moderate",
              cbm_rs_pipeline_plan_step_count_v2(PLAN_PREDUMP, MODE_MODERATE, 0, 0), 6);
    check_int("plan_predump_v2_count_fast",
              cbm_rs_pipeline_plan_step_count_v2(PLAN_PREDUMP, MODE_FAST, 0, 0), 4);
    check_int("plan_predump_v2_count_advanced",
              cbm_rs_pipeline_plan_step_count_v2(PLAN_PREDUMP, 3, 0, 0), 6);
    check_int("plan_predump_v2_steps_full_len",
              cbm_rs_pipeline_plan_steps_v2(PLAN_PREDUMP, MODE_FULL, 9, 9, steps_v2, 6), 6);
    check_int("plan_predump_v2_step0_kind", steps_v2[0].kind, PLAN_STEP_PREDUMP_DECORATOR_TAGS);
    check_int("plan_predump_v2_step0_phase", steps_v2[0].phase, PLAN_PHASE_PREDUMP);
    check_int("plan_predump_v2_step0_policy", steps_v2[0].policy, PLAN_POLICY_REQUIRED);
    check_int("plan_predump_v2_step0_gate", (int)steps_v2[0].gate_flags, 0);
    check_u64("plan_predump_v2_step0_requires", steps_v2[0].requires_mask, 0);
    check_int("plan_predump_v2_step0_effect", (int)steps_v2[0].effect_flags,
              PLAN_EFFECT_MUTATES_GRAPH);
    check_int("plan_predump_v2_step3_kind", steps_v2[3].kind, PLAN_STEP_PREDUMP_SIMILARITY);
    check_int("plan_predump_v2_step3_gate", (int)steps_v2[3].gate_flags, PLAN_GATE_SKIP_FAST);
    check_u64("plan_predump_v2_step3_requires", steps_v2[3].requires_mask,
              (UINT64_C(1) << (PLAN_STEP_PREDUMP_DECORATOR_TAGS - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_PREDUMP_CONFIGLINK - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_PREDUMP_ROUTE_MATCH - 1)));
    check_int("plan_predump_v2_step5_kind", steps_v2[5].kind, PLAN_STEP_PREDUMP_COMPLEXITY);
    check_u64("plan_predump_v2_step5_requires", steps_v2[5].requires_mask,
              (UINT64_C(1) << (PLAN_STEP_PREDUMP_DECORATOR_TAGS - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_PREDUMP_CONFIGLINK - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_PREDUMP_ROUTE_MATCH - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_PREDUMP_SIMILARITY - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_PREDUMP_SEMANTIC_EDGES - 1)));
    check_int("plan_predump_v2_steps_fast_len",
              cbm_rs_pipeline_plan_steps_v2(PLAN_PREDUMP, MODE_FAST, 0, 0, steps_v2, 6), 4);
    check_int("plan_predump_v2_fast_step3_kind", steps_v2[3].kind, PLAN_STEP_PREDUMP_COMPLEXITY);
    check_u64("plan_predump_v2_fast_step3_requires", steps_v2[3].requires_mask,
              (UINT64_C(1) << (PLAN_STEP_PREDUMP_DECORATOR_TAGS - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_PREDUMP_CONFIGLINK - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_PREDUMP_ROUTE_MATCH - 1)));
    check_int("plan_predump_v2_steps_null",
              cbm_rs_pipeline_plan_steps_v2(PLAN_PREDUMP, MODE_FULL, 0, 0, NULL, 6), -1);
    check_int("plan_predump_v2_steps_short_cap",
              cbm_rs_pipeline_plan_steps_v2(PLAN_PREDUMP, MODE_FULL, 0, 0, short_steps_v2, 5), -1);
    check_int("plan_predump_v2_steps_negative_cap",
              cbm_rs_pipeline_plan_steps_v2(PLAN_PREDUMP, MODE_FULL, 0, 0, steps_v2, -1), -1);

    check_int("plan_parallel_v2_count_full",
              cbm_rs_pipeline_plan_step_count_v2(PLAN_PARALLEL_EXTRACTION, MODE_FULL, 2, 51), 7);
    check_int("plan_parallel_v2_count_fast",
              cbm_rs_pipeline_plan_step_count_v2(PLAN_PARALLEL_EXTRACTION, MODE_FAST, 0, 0), 7);
    check_int(
        "plan_parallel_v2_steps_full_len",
        cbm_rs_pipeline_plan_steps_v2(PLAN_PARALLEL_EXTRACTION, MODE_FULL, 2, 51, steps_v2, 7), 7);
    check_int("plan_parallel_v2_step0_kind", steps_v2[0].kind, PLAN_STEP_PARALLEL_EXTRACT);
    check_int("plan_parallel_v2_step0_phase", steps_v2[0].phase, PLAN_PHASE_PARALLEL_EXTRACT);
    check_int("plan_parallel_v2_step0_policy", steps_v2[0].policy, PLAN_POLICY_REQUIRED);
    check_u32("plan_parallel_v2_step0_gate", steps_v2[0].gate_flags, 0);
    check_u64("plan_parallel_v2_step0_requires", steps_v2[0].requires_mask, 0);
    check_u32("plan_parallel_v2_step0_effect", steps_v2[0].effect_flags, PLAN_EFFECT_MUTATES_GRAPH);
    check_int("plan_parallel_v2_step1_kind", steps_v2[1].kind, PLAN_STEP_PARALLEL_REGISTRY_BUILD);
    check_int("plan_parallel_v2_step1_policy", steps_v2[1].policy, PLAN_POLICY_REQUIRED);
    check_u64("plan_parallel_v2_step1_requires", steps_v2[1].requires_mask,
              UINT64_C(1) << (PLAN_STEP_PARALLEL_EXTRACT - 1));
    check_u32("plan_parallel_v2_step1_effect", steps_v2[1].effect_flags, PLAN_EFFECT_MUTATES_GRAPH);
    check_int("plan_parallel_v2_step2_kind", steps_v2[2].kind,
              PLAN_STEP_PARALLEL_LSP_CROSS_PREPARE);
    check_int("plan_parallel_v2_step2_policy", steps_v2[2].policy, PLAN_POLICY_ENV_OPTIONAL);
    check_u32("plan_parallel_v2_step2_gate", steps_v2[2].gate_flags, 0);
    check_u64("plan_parallel_v2_step2_requires", steps_v2[2].requires_mask,
              (UINT64_C(1) << (PLAN_STEP_PARALLEL_EXTRACT - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_PARALLEL_REGISTRY_BUILD - 1)));
    check_u32("plan_parallel_v2_step2_effect", steps_v2[2].effect_flags, 0);
    check_int("plan_parallel_v2_step3_kind", steps_v2[3].kind, PLAN_STEP_PARALLEL_RESOLVE);
    check_int("plan_parallel_v2_step3_policy", steps_v2[3].policy, PLAN_POLICY_REQUIRED);
    check_u64("plan_parallel_v2_step3_requires", steps_v2[3].requires_mask,
              (UINT64_C(1) << (PLAN_STEP_PARALLEL_EXTRACT - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_PARALLEL_REGISTRY_BUILD - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_PARALLEL_LSP_CROSS_PREPARE - 1)));
    check_u32("plan_parallel_v2_step3_effect", steps_v2[3].effect_flags, PLAN_EFFECT_MUTATES_GRAPH);
    check_int("plan_parallel_v2_step6_kind", steps_v2[6].kind, PLAN_STEP_PARALLEL_K8S);
    check_int("plan_parallel_v2_step6_policy", steps_v2[6].policy, PLAN_POLICY_IGNORE_ERR);
    check_u64("plan_parallel_v2_step6_requires", steps_v2[6].requires_mask,
              (UINT64_C(1) << (PLAN_STEP_PARALLEL_EXTRACT - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_PARALLEL_REGISTRY_BUILD - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_PARALLEL_LSP_CROSS_PREPARE - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_PARALLEL_RESOLVE - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_PARALLEL_INFRA_ROUTES - 1)) |
                  (UINT64_C(1) << (PLAN_STEP_PARALLEL_INFRA_BINDINGS - 1)));
    check_u32("plan_parallel_v2_step6_effect", steps_v2[6].effect_flags, PLAN_EFFECT_MUTATES_GRAPH);
    check_int(
        "plan_parallel_v2_steps_short_cap",
        cbm_rs_pipeline_plan_steps_v2(PLAN_PARALLEL_EXTRACTION, MODE_FULL, 0, 0, short_steps_v2, 6),
        -1);
    check_int("plan_v2_count_unsupported",
              cbm_rs_pipeline_plan_step_count_v2(PLAN_FULL_PIPELINE, MODE_FULL, 0, 0), -1);
    check_int("plan_predump_v2_steps_invalid_kind",
              cbm_rs_pipeline_plan_steps_v2(99, MODE_FULL, 0, 0, steps_v2, 6), -1);

    const char *full_fast =
        "macro_extraction:full_mode_only,userconfig_load:fail_open,discover:required,"
        "try_incremental_or_delete_db:existing_db_only,structure:required,"
        "parallel_extract:required,registry_build:required,lsp_cross_prepare:env_optional,"
        "parallel_resolve:required,infra_routes:required,infra_bindings:required,k8s:ignore_err,"
        "tests:required,decorator_tags,configlink,route_match,complexity,dump:required,"
        "persist_hashes:best_effort,artifact_export:optional_persistence";
    check_int("plan_full_fast_len",
              cbm_rs_pipeline_plan_describe(PLAN_FULL_PIPELINE, MODE_FAST, 2, 51, out, sizeof(out)),
              (int)strlen(full_fast));
    check_str("plan_full_fast_out", out, full_fast);

    check_int("plan_invalid_kind",
              cbm_rs_pipeline_plan_describe(99, MODE_FULL, 0, 0, out, sizeof(out)), -1);
    check_int("plan_null_buf",
              cbm_rs_pipeline_plan_describe(PLAN_PREDUMP, MODE_FULL, 0, 0, NULL, sizeof(out)), -1);
    check_int("plan_zero_bufsize",
              cbm_rs_pipeline_plan_describe(PLAN_PREDUMP, MODE_FULL, 0, 0, out, 0), -1);
    memset(out, 'x', sizeof(out));
    check_int(
        "plan_exact_fit",
        cbm_rs_pipeline_plan_describe(PLAN_PREDUMP, MODE_FULL, 0, 0, out, strlen(predump_full) + 1),
        (int)strlen(predump_full));
    check_str("plan_exact_fit_out", out, predump_full);
    memset(out, 'x', sizeof(out));
    check_int(
        "plan_one_byte_short",
        cbm_rs_pipeline_plan_describe(PLAN_PREDUMP, MODE_FULL, 0, 0, out, strlen(predump_full)),
        -1);
    check_bool("plan_one_byte_short_nul", out[strlen(predump_full) - 1] == '\0', true);
    memset(small, 'x', sizeof(small));
    check_int("plan_small_buf",
              cbm_rs_pipeline_plan_describe(PLAN_PREDUMP, MODE_FULL, 0, 0, small, sizeof(small)),
              -1);
    check_bool("plan_small_buf_nul", small[sizeof(small) - 1] == '\0', true);
}

static void test_gbuf_mutation_metadata_exports(void) {
    const uint32_t common_validation = CBM_GBUF_MUTATION_VALIDATION_REQUIRES_NON_NULL_CSTR |
                                       CBM_GBUF_MUTATION_VALIDATION_ALLOWS_EMPTY_CSTR |
                                       CBM_GBUF_MUTATION_VALIDATION_NO_ENDPOINT_LOOKUP |
                                       CBM_GBUF_MUTATION_VALIDATION_NO_JSON_PARSE;
    CbmRsGbufMutationMetaV1 metas[3];
    CbmRsGbufMutationMetaV1 short_metas[2];
    CbmRsGbufMutationValidationV1 validation;

    check_int("gbuf_mut_command_count", cbm_rs_gbuf_mutation_command_count_v1(), 3);
    check_int("gbuf_mut_commands_len", cbm_rs_gbuf_mutation_commands_v1(metas, 3), 3);
    check_int("gbuf_mut_commands_null", cbm_rs_gbuf_mutation_commands_v1(NULL, 3), -1);
    check_int("gbuf_mut_commands_short", cbm_rs_gbuf_mutation_commands_v1(short_metas, 2), -1);
    check_int("gbuf_mut_commands_negative", cbm_rs_gbuf_mutation_commands_v1(metas, -1), -1);

    check_int("gbuf_mut_meta0_kind", metas[0].kind, CBM_GBUF_MUTATION_UPSERT_NODE);
    check_int("gbuf_mut_meta0_result", metas[0].result_kind, CBM_GBUF_MUTATION_RESULT_NODE_ID);
    check_u32("gbuf_mut_meta0_required", metas[0].required_fields,
              CBM_GBUF_MUTATION_FIELD_QUALIFIED_NAME);
    check_u32("gbuf_mut_meta0_optional", metas[0].optional_fields,
              CBM_GBUF_MUTATION_FIELD_LABEL | CBM_GBUF_MUTATION_FIELD_NAME |
                  CBM_GBUF_MUTATION_FIELD_FILE_PATH | CBM_GBUF_MUTATION_FIELD_START_LINE |
                  CBM_GBUF_MUTATION_FIELD_END_LINE | CBM_GBUF_MUTATION_FIELD_PROPERTIES_JSON);
    check_u32("gbuf_mut_meta0_effect", metas[0].effect_flags,
              CBM_GBUF_MUTATION_EFFECT_MUTATES_GRAPH | CBM_GBUF_MUTATION_EFFECT_ALLOCATES_ID |
                  CBM_GBUF_MUTATION_EFFECT_UPDATES_EXISTING);
    check_u32("gbuf_mut_meta0_validation", metas[0].validation_flags, common_validation);
    check_u32("gbuf_mut_meta0_reserved0", metas[0].reserved0, 0);
    check_u32("gbuf_mut_meta0_reserved1", metas[0].reserved1, 0);

    check_int("gbuf_mut_meta1_kind", metas[1].kind, CBM_GBUF_MUTATION_INSERT_EDGE);
    check_int("gbuf_mut_meta1_result", metas[1].result_kind, CBM_GBUF_MUTATION_RESULT_EDGE_ID);
    check_u32("gbuf_mut_meta1_required", metas[1].required_fields,
              CBM_GBUF_MUTATION_FIELD_SOURCE_ID | CBM_GBUF_MUTATION_FIELD_TARGET_ID |
                  CBM_GBUF_MUTATION_FIELD_EDGE_TYPE);
    check_u32("gbuf_mut_meta1_optional", metas[1].optional_fields,
              CBM_GBUF_MUTATION_FIELD_PROPERTIES_JSON);
    check_u32("gbuf_mut_meta1_effect", metas[1].effect_flags,
              CBM_GBUF_MUTATION_EFFECT_MUTATES_GRAPH | CBM_GBUF_MUTATION_EFFECT_ALLOCATES_ID |
                  CBM_GBUF_MUTATION_EFFECT_DEDUPS | CBM_GBUF_MUTATION_EFFECT_UPDATES_EXISTING);

    check_int("gbuf_mut_meta2_kind", metas[2].kind, CBM_GBUF_MUTATION_DELETE_BY_FILE);
    check_int("gbuf_mut_meta2_result", metas[2].result_kind, CBM_GBUF_MUTATION_RESULT_COUNT);
    check_u32("gbuf_mut_meta2_required", metas[2].required_fields,
              CBM_GBUF_MUTATION_FIELD_FILE_PATH);
    check_u32("gbuf_mut_meta2_optional", metas[2].optional_fields, 0);
    check_u32("gbuf_mut_meta2_effect", metas[2].effect_flags,
              CBM_GBUF_MUTATION_EFFECT_MUTATES_GRAPH | CBM_GBUF_MUTATION_EFFECT_CASCADE_EDGES);

    CbmRsGbufMutationCmdV1 upsert_empty_qn = {
        .kind = CBM_GBUF_MUTATION_UPSERT_NODE,
        .qualified_name = "",
    };
    check_int("gbuf_mut_validate_upsert_empty_qn_rc",
              cbm_rs_gbuf_mutation_validate_v1(&upsert_empty_qn, &validation), 0);
    check_int("gbuf_mut_validate_upsert_empty_qn_ok", validation.ok, 1);
    check_int("gbuf_mut_validate_upsert_empty_qn_error", validation.error_code,
              CBM_GBUF_MUTATION_ERROR_OK);
    check_u32("gbuf_mut_validate_upsert_empty_qn_missing", validation.missing_fields, 0);
    check_u32("gbuf_mut_validate_upsert_empty_qn_invalid", validation.invalid_fields, 0);
    check_u32("gbuf_mut_validate_upsert_empty_qn_norm", validation.normalized_flags,
              CBM_GBUF_MUTATION_NORMALIZED_OPTIONAL_NULL_CSTR);
    check_u32("gbuf_mut_validate_upsert_empty_qn_reserved", validation.reserved0, 0);

    CbmRsGbufMutationCmdV1 upsert_missing_qn = {.kind = CBM_GBUF_MUTATION_UPSERT_NODE};
    check_int("gbuf_mut_validate_upsert_missing_qn_rc",
              cbm_rs_gbuf_mutation_validate_v1(&upsert_missing_qn, &validation), 0);
    check_int("gbuf_mut_validate_upsert_missing_qn_ok", validation.ok, 0);
    check_int("gbuf_mut_validate_upsert_missing_qn_error", validation.error_code,
              CBM_GBUF_MUTATION_ERROR_MISSING_FIELD);
    check_u32("gbuf_mut_validate_upsert_missing_qn_missing", validation.missing_fields,
              CBM_GBUF_MUTATION_FIELD_QUALIFIED_NAME);

    CbmRsGbufMutationCmdV1 insert_orphan_edge = {
        .kind = CBM_GBUF_MUTATION_INSERT_EDGE,
        .source_id = 0,
        .target_id = -99,
        .edge_type = "CALLS",
    };
    check_int("gbuf_mut_validate_insert_orphan_rc",
              cbm_rs_gbuf_mutation_validate_v1(&insert_orphan_edge, &validation), 0);
    check_int("gbuf_mut_validate_insert_orphan_ok", validation.ok, 1);
    check_u32("gbuf_mut_validate_insert_orphan_missing", validation.missing_fields, 0);
    check_u32("gbuf_mut_validate_insert_orphan_norm", validation.normalized_flags,
              CBM_GBUF_MUTATION_NORMALIZED_OPTIONAL_NULL_CSTR);

    CbmRsGbufMutationCmdV1 insert_missing_type = {
        .kind = CBM_GBUF_MUTATION_INSERT_EDGE,
        .source_id = 1,
        .target_id = 2,
    };
    check_int("gbuf_mut_validate_insert_missing_type_rc",
              cbm_rs_gbuf_mutation_validate_v1(&insert_missing_type, &validation), 0);
    check_int("gbuf_mut_validate_insert_missing_type_ok", validation.ok, 0);
    check_u32("gbuf_mut_validate_insert_missing_type_missing", validation.missing_fields,
              CBM_GBUF_MUTATION_FIELD_EDGE_TYPE);

    CbmRsGbufMutationCmdV1 delete_file = {
        .kind = CBM_GBUF_MUTATION_DELETE_BY_FILE,
        .file_path = "src/main.c",
    };
    check_int("gbuf_mut_validate_delete_file_rc",
              cbm_rs_gbuf_mutation_validate_v1(&delete_file, &validation), 0);
    check_int("gbuf_mut_validate_delete_file_ok", validation.ok, 1);
    check_int("gbuf_mut_validate_delete_file_error", validation.error_code,
              CBM_GBUF_MUTATION_ERROR_OK);
    check_u32("gbuf_mut_validate_delete_file_norm", validation.normalized_flags, 0);

    CbmRsGbufMutationCmdV1 delete_missing_file = {.kind = CBM_GBUF_MUTATION_DELETE_BY_FILE};
    check_int("gbuf_mut_validate_delete_missing_file_rc",
              cbm_rs_gbuf_mutation_validate_v1(&delete_missing_file, &validation), 0);
    check_int("gbuf_mut_validate_delete_missing_file_ok", validation.ok, 0);
    check_u32("gbuf_mut_validate_delete_missing_file_missing", validation.missing_fields,
              CBM_GBUF_MUTATION_FIELD_FILE_PATH);

    CbmRsGbufMutationCmdV1 unknown = {.kind = 99};
    check_int("gbuf_mut_validate_unknown_rc",
              cbm_rs_gbuf_mutation_validate_v1(&unknown, &validation), 0);
    check_int("gbuf_mut_validate_unknown_ok", validation.ok, 0);
    check_int("gbuf_mut_validate_unknown_error", validation.error_code,
              CBM_GBUF_MUTATION_ERROR_UNKNOWN_KIND);
    check_u32("gbuf_mut_validate_unknown_invalid", validation.invalid_fields,
              CBM_GBUF_MUTATION_FIELD_KIND);

    CbmRsGbufMutationCmdV1 reserved = {
        .kind = CBM_GBUF_MUTATION_DELETE_BY_FILE,
        .reserved0 = 1,
        .file_path = "src/main.c",
    };
    check_int("gbuf_mut_validate_reserved_rc",
              cbm_rs_gbuf_mutation_validate_v1(&reserved, &validation), 0);
    check_int("gbuf_mut_validate_reserved_ok", validation.ok, 0);
    check_int("gbuf_mut_validate_reserved_error", validation.error_code,
              CBM_GBUF_MUTATION_ERROR_INVALID_FIELD);
    check_u32("gbuf_mut_validate_reserved_invalid", validation.invalid_fields,
              CBM_GBUF_MUTATION_FIELD_RESERVED);

    CbmRsGbufMutationCmdV1 unknown_reserved = {
        .kind = 99,
        .reserved0 = 1,
    };
    check_int("gbuf_mut_validate_unknown_reserved_rc",
              cbm_rs_gbuf_mutation_validate_v1(&unknown_reserved, &validation), 0);
    check_int("gbuf_mut_validate_unknown_reserved_ok", validation.ok, 0);
    check_int("gbuf_mut_validate_unknown_reserved_error", validation.error_code,
              CBM_GBUF_MUTATION_ERROR_UNKNOWN_KIND);
    check_u32("gbuf_mut_validate_unknown_reserved_invalid", validation.invalid_fields,
              CBM_GBUF_MUTATION_FIELD_KIND);

    check_int("gbuf_mut_validate_null_cmd", cbm_rs_gbuf_mutation_validate_v1(NULL, &validation),
              -1);
    check_int("gbuf_mut_validate_null_out", cbm_rs_gbuf_mutation_validate_v1(&delete_file, NULL),
              -1);
}

static CbmRsRegistryResolveOut resolve_registry_once(
    const CbmRsRegistryEntry *entries, int entry_count, const char *callee_name,
    const char *module_qn, const char **import_keys, const char **import_vals, int import_count,
    char *qn, size_t qn_size, char *strategy, size_t strategy_size) {
    CbmRsRegistryResolveOut out = {0};
    int rc = cbm_rs_registry_resolve_once(entries, entry_count, callee_name, module_qn, import_keys,
                                          import_vals, import_count, qn, qn_size, strategy,
                                          strategy_size, &out);
    if (rc < 0) {
        fail("registry_resolve_once", "unexpected invalid-argument result");
    }
    return out;
}

static int resolve_registry_handle_rc(const CbmRsRegistryHandle *registry, const char *callee_name,
                                      const char *module_qn, const char **import_keys,
                                      const char **import_vals, int import_count, char *qn,
                                      size_t qn_size, char *strategy, size_t strategy_size,
                                      CbmRsRegistryResolveOut *out) {
    return cbm_rs_registry_resolve(registry, callee_name, module_qn, import_keys, import_vals,
                                   import_count, qn, qn_size, strategy, strategy_size, out);
}

static CbmRsRegistryResolveOut resolve_registry_handle(
    const CbmRsRegistryHandle *registry, const char *callee_name, const char *module_qn,
    const char **import_keys, const char **import_vals, int import_count, char *qn, size_t qn_size,
    char *strategy, size_t strategy_size) {
    CbmRsRegistryResolveOut out = {0};
    int rc = resolve_registry_handle_rc(registry, callee_name, module_qn, import_keys, import_vals,
                                        import_count, qn, qn_size, strategy, strategy_size, &out);
    if (rc < 0) {
        fail("registry_resolve_handle", "unexpected invalid-argument result");
    }
    return out;
}

static void test_registry_handle_lifecycle_and_persistent_resolve(void) {
    cbm_rs_registry_free(NULL);
    check_int("registry_add_null_handle", cbm_rs_registry_add(NULL, "proj.pkg.Foo"), -1);
    check_int(
        "registry_resolve_null_handle",
        cbm_rs_registry_resolve(NULL, "Foo", "proj.pkg", NULL, NULL, 0, NULL, 0, NULL, 0, NULL),
        -1);

    CbmRsRegistryHandle *registry = cbm_rs_registry_create();
    check_not_null("registry_handle_create", registry);
    check_int("registry_add_null_qn", cbm_rs_registry_add(registry, NULL), -1);
    check_int("registry_add_foo", cbm_rs_registry_add(registry, "proj.pkg.Foo"), 1);
    check_int("registry_add_duplicate", cbm_rs_registry_add(registry, "proj.pkg.Foo"), 1);
    check_int("registry_add_bar", cbm_rs_registry_add(registry, "proj.other.Bar"), 1);

    char qn[128];
    char strategy[64];
    CbmRsRegistryResolveOut out = resolve_registry_handle(
        registry, "Foo", "proj.pkg", NULL, NULL, 0, qn, sizeof(qn), strategy, sizeof(strategy));
    check_bool("registry_handle_same_module_matched", out.matched, true);
    check_str("registry_handle_same_module_qn", qn, "proj.pkg.Foo");
    check_str("registry_handle_same_module_strategy", strategy, "same_module");

    out = resolve_registry_handle(registry, "Bar", "proj.pkg", NULL, NULL, 0, qn, sizeof(qn),
                                  strategy, sizeof(strategy));
    check_bool("registry_handle_unique_matched", out.matched, true);
    check_str("registry_handle_unique_qn", qn, "proj.other.Bar");
    check_str("registry_handle_unique_strategy", strategy, "unique_name");

    out = resolve_registry_handle(registry, "Missing", "proj.pkg", NULL, NULL, 0, qn, sizeof(qn),
                                  strategy, sizeof(strategy));
    check_bool("registry_handle_unmatched", out.matched, false);
    check_str("registry_handle_unmatched_qn", qn, "");
    check_str("registry_handle_unmatched_strategy", strategy, "");

    cbm_rs_registry_free(registry);
}

static void test_registry_handle_import_map_suffix_and_truncation(void) {
    CbmRsRegistryHandle *registry = cbm_rs_registry_create();
    check_not_null("registry_handle_create_suffix", registry);
    check_int("registry_add_suffix", cbm_rs_registry_add(registry, "proj.other.sub.Foo"), 1);

    const char *keys[] = {"other"};
    const char *vals[] = {"proj.other"};
    char qn[128];
    char strategy[64];
    CbmRsRegistryResolveOut out =
        resolve_registry_handle(registry, "other.Foo", "proj.pkg", keys, vals, 1, qn, sizeof(qn),
                                strategy, sizeof(strategy));
    check_bool("registry_handle_import_suffix_matched", out.matched, true);
    check_str("registry_handle_import_suffix_qn", qn, "proj.other.sub.Foo");
    check_str("registry_handle_import_suffix_strategy", strategy, "import_map_suffix");

    char small_qn[8];
    char small_strategy[8];
    memset(small_qn, 'x', sizeof(small_qn));
    memset(small_strategy, 'x', sizeof(small_strategy));
    out = (CbmRsRegistryResolveOut){0};
    int rc =
        resolve_registry_handle_rc(registry, "other.Foo", "proj.pkg", keys, vals, 1, small_qn,
                                   sizeof(small_qn), small_strategy, sizeof(small_strategy), &out);
    check_int("registry_handle_trunc_rc", rc, -2);
    check_bool("registry_handle_trunc_qn_nul", small_qn[sizeof(small_qn) - 1] == '\0', true);
    check_bool("registry_handle_trunc_strategy_nul",
               small_strategy[sizeof(small_strategy) - 1] == '\0', true);

    cbm_rs_registry_free(registry);
}

static void test_registry_import_map_and_bare(void) {
    const CbmRsRegistryEntry entries[] = {
        {"Process", "proj.pkg.worker.Process", "Function"},
        {"requireAdmin", "proj.lib.authorization.requireAdmin", "Function"},
        {"requireAdmin", "proj.lib.users.requireAdmin", "Function"},
    };
    const char *keys1[] = {"worker"};
    const char *vals1[] = {"proj.pkg.worker"};
    char qn[128];
    char strategy[64];

    CbmRsRegistryResolveOut out =
        resolve_registry_once(entries, 3, "worker.Process", "proj.cmd.main", keys1, vals1, 1, qn,
                              sizeof(qn), strategy, sizeof(strategy));
    check_bool("registry_import_map_matched", out.matched, true);
    check_str("registry_import_map_qn", qn, "proj.pkg.worker.Process");
    check_str("registry_import_map_strategy", strategy, "import_map");
    check_double_near("registry_import_map_conf", out.confidence, 0.95);

    const char *keys2[] = {"requireAdmin"};
    const char *vals2[] = {"proj.lib.authorization"};
    out = resolve_registry_once(entries, 3, "requireAdmin", "proj.actions.settings", keys2, vals2,
                                1, qn, sizeof(qn), strategy, sizeof(strategy));
    check_bool("registry_import_map_bare_matched", out.matched, true);
    check_str("registry_import_map_bare_qn", qn, "proj.lib.authorization.requireAdmin");
    check_str("registry_import_map_bare_strategy", strategy, "import_map");
}

static void test_registry_qualified_suffix(void) {
    const CbmRsRegistryEntry entries[] = {
        {"save", "proj.lib.App.Alpha.save", "Function"},
        {"save", "proj.lib.App.Beta.save", "Function"},
        {"save", "proj.lib.App.Gamma.save", "Function"},
    };
    char qn[128];
    char strategy[64];
    CbmRsRegistryResolveOut out =
        resolve_registry_once(entries, 3, "App::Beta::save", "proj.lib.App.Caller", NULL, NULL, 0,
                              qn, sizeof(qn), strategy, sizeof(strategy));
    check_bool("registry_qualified_suffix_matched", out.matched, true);
    check_str("registry_qualified_suffix_qn", qn, "proj.lib.App.Beta.save");
    check_str("registry_qualified_suffix_strategy", strategy, "qualified_suffix");
}

static void test_registry_suffix_candidate_selection(void) {
    const CbmRsRegistryEntry entries[] = {
        {"Process", "proj.test.Process", "Function"},
        {"Process", "proj.prod.Process", "Function"},
    };
    char qn[128];
    char strategy[64];
    CbmRsRegistryResolveOut out =
        resolve_registry_once(entries, 2, "Process", "proj.test.caller", NULL, NULL, 0, qn,
                              sizeof(qn), strategy, sizeof(strategy));
    check_bool("registry_suffix_matched", out.matched, true);
    check_str("registry_suffix_qn", qn, "proj.prod.Process");
    check_str("registry_suffix_strategy", strategy, "suffix_match");
}

static void test_registry_candidate_cap(void) {
    enum { ENTRY_COUNT = 257 };
    CbmRsRegistryEntry entries[ENTRY_COUNT];
    char qns[ENTRY_COUNT][64];
    for (int i = 0; i < ENTRY_COUNT; i++) {
        snprintf(qns[i], sizeof(qns[i]), "proj.mod%d.flags", i);
        entries[i] = (CbmRsRegistryEntry){"flags", qns[i], "Variable"};
    }

    char qn[128];
    char strategy[64];
    CbmRsRegistryResolveOut out =
        resolve_registry_once(entries, ENTRY_COUNT, "flags", "proj.other.caller", NULL, NULL, 0, qn,
                              sizeof(qn), strategy, sizeof(strategy));
    check_bool("registry_candidate_cap_unmatched", out.matched, false);
    check_str("registry_candidate_cap_empty_qn", qn, "");

    out = resolve_registry_once(entries, ENTRY_COUNT, "flags", "proj.mod7", NULL, NULL, 0, qn,
                                sizeof(qn), strategy, sizeof(strategy));
    check_bool("registry_candidate_cap_same_module", out.matched, true);
    check_str("registry_candidate_cap_qn", qn, "proj.mod7.flags");
    check_str("registry_candidate_cap_strategy", strategy, "same_module");
}

static void test_registry_import_reachability_penalty(void) {
    const CbmRsRegistryEntry entries[] = {
        {"Helper", "proj.shared.utils.Helper", "Function"},
    };
    const char *keys[] = {"Other"};
    const char *vals[] = {"proj.other"};
    char qn[128];
    char strategy[64];
    CbmRsRegistryResolveOut out =
        resolve_registry_once(entries, 1, "Helper", "proj.caller", keys, vals, 1, qn, sizeof(qn),
                              strategy, sizeof(strategy));
    check_bool("registry_reachability_penalty_matched", out.matched, true);
    check_str("registry_reachability_penalty_qn", qn, "proj.shared.utils.Helper");
    check_str("registry_reachability_penalty_strategy", strategy, "unique_name");
    check_double_near("registry_reachability_penalty_conf", out.confidence, 0.375);
}

/* ── hash_table borrowed-pointer 契約 parity ──────────────────────
 * 對齊 tests/test_hash_table.c 的 C 契約：content-key 比對、get_key 回傳
 * stored 指標（覆寫更新）、null value 為有效存在項、foreach exactly-once。 */
typedef struct {
    void *expected_userdata;
    bool bad_userdata;
    bool duplicate;
    bool unexpected;
    bool key_mismatch;
    bool seen[4];
    int count;
    int sum;
} RsForeachCtx;

static void rs_ht_foreach_cb(const char *key, void *value, void *userdata) {
    RsForeachCtx *ctx = (RsForeachCtx *)userdata;
    if (!ctx) {
        return;
    }
    if (userdata != ctx->expected_userdata) {
        ctx->bad_userdata = true;
        return;
    }
    if (!key || !value) {
        ctx->unexpected = true;
        return;
    }
    int idx = *(int *)value;
    if (idx < 0 || idx >= 4) {
        ctx->unexpected = true;
        return;
    }
    const char *expected_keys[] = {"fa", "fb", "fc", "fd"};
    if (strcmp(key, expected_keys[idx]) != 0) {
        ctx->key_mismatch = true;
    }
    if (ctx->seen[idx]) {
        ctx->duplicate = true;
    }
    ctx->seen[idx] = true;
    ctx->count++;
    ctx->sum += idx;
}

static void test_hash_table_exports(void) {
    /* null-handle 契約：不得 crash，回傳空 */
    int sentinel = 1;
    check_null("ht_set_null_ht", cbm_rs_ht_set(NULL, "k", &sentinel));
    check_null("ht_get_null_ht", cbm_rs_ht_get(NULL, "k"));
    check_bool("ht_has_null_ht", cbm_rs_ht_has(NULL, "k"), false);
    check_null("ht_get_key_null_ht", cbm_rs_ht_get_key(NULL, "k"));
    check_null("ht_delete_null_ht", cbm_rs_ht_delete(NULL, "k"));
    check_u32("ht_count_null_ht", cbm_rs_ht_count(NULL), 0);
    cbm_rs_ht_free(NULL);
    cbm_rs_ht_clear(NULL);
    cbm_rs_ht_foreach(NULL, rs_ht_foreach_cb, NULL);

    CBMRustHashTable *ht = cbm_rs_ht_create();
    check_not_null("ht_create", ht);
    check_u32("ht_count_empty", cbm_rs_ht_count(ht), 0);

    /* null-key 契約 */
    check_null("ht_set_null_key", cbm_rs_ht_set(ht, NULL, &sentinel));
    check_null("ht_get_null_key", cbm_rs_ht_get(ht, NULL));
    check_bool("ht_has_null_key", cbm_rs_ht_has(ht, NULL), false);
    check_null("ht_get_key_null_key", cbm_rs_ht_get_key(ht, NULL));
    check_null("ht_delete_null_key", cbm_rs_ht_delete(ht, NULL));
    check_u32("ht_count_after_null_key", cbm_rs_ht_count(ht), 0);

    /* set/get：首次插入回傳 null prev */
    int val = 42;
    check_null("ht_set_first_prev", cbm_rs_ht_set(ht, "hello", &val));
    check_ptr_eq("ht_get_value", cbm_rs_ht_get(ht, "hello"), &val);
    check_u32("ht_count_one", cbm_rs_ht_count(ht), 1);

    /* content lookup + get_key 回傳 stored 指標（非 lookup buffer） */
    char stored_key[] = "same-content";
    char lookup_key[] = "same-content";
    int cv = 123;
    check_null("ht_set_stored", cbm_rs_ht_set(ht, stored_key, &cv));
    check_bool("ht_has_by_content", cbm_rs_ht_has(ht, lookup_key), true);
    check_ptr_eq("ht_get_by_content", cbm_rs_ht_get(ht, lookup_key), &cv);
    check_ptr_eq("ht_get_key_stored_ptr", cbm_rs_ht_get_key(ht, lookup_key), stored_key);

    /* 覆寫：回傳前值，且 stored key 指標更新為新指標 */
    char second_key[] = "same-content";
    int cv2 = 456;
    check_ptr_eq("ht_overwrite_prev", cbm_rs_ht_set(ht, second_key, &cv2), &cv);
    check_ptr_eq("ht_overwrite_value", cbm_rs_ht_get(ht, stored_key), &cv2);
    check_ptr_eq("ht_overwrite_key_ptr", cbm_rs_ht_get_key(ht, stored_key), second_key);
    check_u32("ht_overwrite_count", cbm_rs_ht_count(ht), 2);

    /* null value 仍是有效存在項 */
    check_null("ht_set_nullval_prev", cbm_rs_ht_set(ht, "nullable", NULL));
    check_null("ht_get_nullval", cbm_rs_ht_get(ht, "nullable"));
    check_bool("ht_has_nullval", cbm_rs_ht_has(ht, "nullable"), true);
    check_str("ht_get_key_nullval", cbm_rs_ht_get_key(ht, "nullable"), "nullable");
    check_u32("ht_count_with_nullval", cbm_rs_ht_count(ht), 3);
    /* 刪除 null-value 項回傳 null（與 C 契約一致：value 即 NULL） */
    check_null("ht_delete_nullval", cbm_rs_ht_delete(ht, "nullable"));
    check_bool("ht_has_after_delete_nullval", cbm_rs_ht_has(ht, "nullable"), false);
    check_u32("ht_count_after_delete_nullval", cbm_rs_ht_count(ht), 2);

    /* 非 UTF-8 高位元 C-string key（content 比對含 raw bytes） */
    char hb_stored[] = {(char)0xff, (char)0x80, 'k', (char)0xfe, '\0'};
    char hb_lookup[] = {(char)0xff, (char)0x80, 'k', (char)0xfe, '\0'};
    int hv = 88;
    check_null("ht_hb_set", cbm_rs_ht_set(ht, hb_stored, &hv));
    check_bool("ht_hb_has", cbm_rs_ht_has(ht, hb_lookup), true);
    check_ptr_eq("ht_hb_get", cbm_rs_ht_get(ht, hb_lookup), &hv);
    check_ptr_eq("ht_hb_get_key", cbm_rs_ht_get_key(ht, hb_lookup), hb_stored);

    /* delete 回傳 value；missing 回傳 null */
    check_ptr_eq("ht_delete_val", cbm_rs_ht_delete(ht, "hello"), &val);
    check_null("ht_delete_missing", cbm_rs_ht_delete(ht, "hello"));
    check_null("ht_get_after_delete", cbm_rs_ht_get(ht, "hello"));

    /* clear */
    cbm_rs_ht_clear(ht);
    check_u32("ht_clear_count", cbm_rs_ht_count(ht), 0);
    check_bool("ht_clear_has", cbm_rs_ht_has(ht, "same-content"), false);
    cbm_rs_ht_free(ht);

    /* 多筆 + 成長（HashMap resize parity） */
    CBMRustHashTable *big = cbm_rs_ht_create();
    check_not_null("ht_big_create", big);
    static char keys[200][16];
    static int vals[200];
    for (int i = 0; i < 200; i++) {
        snprintf(keys[i], sizeof(keys[i]), "key_%03d", i);
        vals[i] = i * 10;
        check_null("ht_big_set", cbm_rs_ht_set(big, keys[i], &vals[i]));
    }
    check_u32("ht_big_count", cbm_rs_ht_count(big), 200);
    bool all_ok = true;
    for (int i = 0; i < 200; i++) {
        void *got = cbm_rs_ht_get(big, keys[i]);
        if (got != &vals[i]) {
            all_ok = false;
        }
    }
    check_bool("ht_big_all_survive_resize", all_ok, true);
    cbm_rs_ht_free(big);

    /* foreach：userdata 傳遞 + exactly-once + null-fn no-op */
    CBMRustHashTable *fe = cbm_rs_ht_create();
    int fv[] = {0, 1, 2, 3};
    cbm_rs_ht_set(fe, "fa", &fv[0]);
    cbm_rs_ht_set(fe, "fb", &fv[1]);
    cbm_rs_ht_set(fe, "fc", &fv[2]);
    cbm_rs_ht_set(fe, "fd", &fv[3]);
    RsForeachCtx ctx = {.expected_userdata = NULL};
    ctx.expected_userdata = &ctx;
    cbm_rs_ht_foreach(fe, rs_ht_foreach_cb, &ctx);
    check_bool("ht_fe_bad_userdata", ctx.bad_userdata, false);
    check_bool("ht_fe_duplicate", ctx.duplicate, false);
    check_bool("ht_fe_unexpected", ctx.unexpected, false);
    check_bool("ht_fe_key_mismatch", ctx.key_mismatch, false);
    check_int("ht_fe_count", ctx.count, 4);
    check_int("ht_fe_sum", ctx.sum, 6);
    cbm_rs_ht_foreach(fe, NULL, &ctx); /* null fn 必須 no-op、不得 crash */
    check_int("ht_fe_count_after_null_fn", ctx.count, 4);
    cbm_rs_ht_free(fe);
}

int main(void) {
    test_dump_verify_exports();
    test_intern_null_contracts();
    test_intern_dedup_and_pool_lifetime();
    test_intern_embedded_nul_and_empty_pools();
    test_intern_pointer_stability_after_growth();
    test_intern_rejects_oversized_len_before_reading();
    test_str_util_exports();
    test_platform_normalize_export();
    test_platform_env_dirs_exports();
#ifndef _WIN32
    test_platform_cgroup_exports();
#endif
    test_diagnostics_exports();
    test_hash_table_exports();
    test_pipeline_plan_exports();
    test_gbuf_mutation_metadata_exports();
    test_registry_import_map_and_bare();
    test_registry_qualified_suffix();
    test_registry_suffix_candidate_selection();
    test_registry_candidate_cap();
    test_registry_import_reachability_penalty();
    test_registry_handle_lifecycle_and_persistent_resolve();
    test_registry_handle_import_map_suffix_and_truncation();
    return 0;
}
