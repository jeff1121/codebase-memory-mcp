/*
 * test_rust_ffi.c - 階段性 Rust staticlib export 的 C smoke tests。
 *
 * 這個檔案刻意使用自己的 main，不連 tests/test_main.c，讓 Rust ABI gate
 * 可以不依賴完整 C test runner。
 */
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pipeline/pipeline.h"
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
        PLAN_SEQUENTIAL = 0,
        PLAN_PREDUMP = 1,
        PLAN_EXTRACTION_CHOICE = 2,
        PLAN_INCREMENTAL_EXTRACT_RESOLVE = 3,
        PLAN_INCREMENTAL_POST = 4,
        PLAN_PARALLEL_EXTRACTION = 5,
        PLAN_FULL_PIPELINE = 6,
        MODE_FULL = CBM_MODE_FULL,
        MODE_MODERATE = CBM_MODE_MODERATE,
        MODE_FAST = CBM_MODE_FAST,
    };
    char out[1024];
    char small[8];

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
    test_pipeline_plan_exports();
    test_registry_import_map_and_bare();
    test_registry_qualified_suffix();
    test_registry_suffix_candidate_selection();
    test_registry_candidate_cap();
    test_registry_import_reachability_penalty();
    test_registry_handle_lifecycle_and_persistent_resolve();
    test_registry_handle_import_map_suffix_and_truncation();
    return 0;
}
