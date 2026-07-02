/*
 * test_platform.c — RED phase tests for foundation/platform.
 */
#include "test_framework.h"
#include "../src/foundation/compat.h" /* cbm_setenv / cbm_unsetenv (Windows-portable) */
#include "../src/foundation/platform.h"
#include "../src/foundation/system_info_internal.h"
#include <stdlib.h>
#include <unistd.h>

typedef struct {
    const char *name;
    char *value;
    bool was_set;
} platform_env_snapshot_t;

static void platform_env_save(platform_env_snapshot_t *snapshot, const char *name) {
    const char *value = getenv(name);
    snapshot->name = name;
    snapshot->was_set = value != NULL;
    snapshot->value = value ? cbm_strdup(value) : NULL;
}

static void platform_env_restore(platform_env_snapshot_t *snapshot) {
    if (snapshot->was_set) {
        (void)cbm_setenv(snapshot->name, snapshot->value ? snapshot->value : "", 1);
    } else {
        (void)cbm_unsetenv(snapshot->name);
    }
    free(snapshot->value);
    snapshot->value = NULL;
}

static void platform_env_save_many(platform_env_snapshot_t *snapshots, const char **names,
                                   size_t count) {
    for (size_t i = 0; i < count; i++) {
        platform_env_save(&snapshots[i], names[i]);
    }
}

static void platform_env_restore_many(platform_env_snapshot_t *snapshots, size_t count) {
    for (size_t i = 0; i < count; i++) {
        platform_env_restore(&snapshots[i]);
    }
}

static void platform_fill_repeated(char *buf, size_t buf_sz, char ch) {
    if (buf_sz == 0) {
        return;
    }
    memset(buf, ch, buf_sz - 1);
    buf[buf_sz - 1] = '\0';
}

#ifdef __linux__
/* Linux-only cgroup tests need stdio for FILE*, stdlib for mkdtemp,
 * string for strncpy/strchr, sys/stat for mkdir, dirent for the
 * shell-free recursive teardown. */
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#endif

TEST(platform_now_ns) {
    uint64_t t1 = cbm_now_ns();
    ASSERT_GT(t1, 0);
    /* Busy-wait a tiny bit */
    for (volatile int i = 0; i < 100000; i++) {}
    uint64_t t2 = cbm_now_ns();
    ASSERT_GT(t2, t1);
    PASS();
}

TEST(platform_now_ms) {
    uint64_t t1 = cbm_now_ms();
    ASSERT_GT(t1, 0);
    PASS();
}

TEST(platform_nprocs) {
    int n = cbm_nprocs();
    ASSERT_GT(n, 0);
    ASSERT_LT(n, 10000); /* sanity */
    PASS();
}

TEST(platform_file_exists) {
    /* This test file should exist */
    ASSERT_TRUE(cbm_file_exists("tests/test_platform.c"));
    ASSERT_FALSE(cbm_file_exists("nonexistent_file_xyz.txt"));
    PASS();
}

TEST(platform_is_dir) {
    ASSERT_TRUE(cbm_is_dir("tests"));
    ASSERT_FALSE(cbm_is_dir("tests/test_platform.c"));
    ASSERT_FALSE(cbm_is_dir("nonexistent_dir"));
    PASS();
}

TEST(platform_file_size) {
    int64_t sz = cbm_file_size("tests/test_platform.c");
    ASSERT_GT(sz, 0);
    ASSERT_EQ(cbm_file_size("nonexistent_file_xyz.txt"), -1);
    PASS();
}

TEST(platform_mmap) {
    /* mmap this test file and verify first bytes */
    size_t sz = 0;
    void *data = cbm_mmap_read("tests/test_platform.c", &sz);
    ASSERT_NOT_NULL(data);
    ASSERT_GT(sz, 0);
    /* First line should be the comment */
    ASSERT(memcmp(data, "/*", 2) == 0);
    cbm_munmap(data, sz);
    PASS();
}

TEST(platform_mmap_nonexistent) {
    size_t sz = 0;
    void *data = cbm_mmap_read("nonexistent_xyz.txt", &sz);
    ASSERT_NULL(data);
    PASS();
}

TEST(platform_normalize_path_sep) {
    char win_path[] = "c:\\Users\\test";
    ASSERT(cbm_normalize_path_sep(win_path) == win_path);
    ASSERT_STR_EQ(win_path, "C:/Users/test");

    char bare_drive[] = "z:";
    ASSERT(cbm_normalize_path_sep(bare_drive) == bare_drive);
    ASSERT_STR_EQ(bare_drive, "Z:");

    char non_drive[] = "abc:def\\x";
    ASSERT(cbm_normalize_path_sep(non_drive) == non_drive);
    ASSERT_STR_EQ(non_drive, "abc:def/x");

    ASSERT_NULL(cbm_normalize_path_sep(NULL));
    PASS();
}

TEST(platform_safe_getenv_contract) {
    const char *name = "CBM_TEST_SAFE_GETENV";
    platform_env_snapshot_t saved;
    platform_env_save(&saved, name);

    char buf[32];
    ASSERT_EQ(cbm_setenv(name, "fixture-value", 1), 0);
    memset(buf, '?', sizeof(buf));
    const char *got = cbm_safe_getenv(name, buf, sizeof(buf), "fallback");
    ASSERT(got == buf);
    ASSERT_STR_EQ(buf, "fixture-value");

    ASSERT_EQ(cbm_unsetenv(name), 0);
    memset(buf, '?', sizeof(buf));
    got = cbm_safe_getenv(name, buf, sizeof(buf), "fallback");
    ASSERT(got == buf);
    ASSERT_STR_EQ(buf, "fallback");

    memset(buf, '?', sizeof(buf));
    got = cbm_safe_getenv(name, buf, sizeof(buf), NULL);
    ASSERT_NULL(got);
    ASSERT_STR_EQ(buf, "");

    ASSERT_EQ(cbm_setenv(name, "abcdef", 1), 0);
    char small[4];
    memset(small, '?', sizeof(small));
    got = cbm_safe_getenv(name, small, sizeof(small), NULL);
    ASSERT(got == small);
    ASSERT_STR_EQ(small, "abc");

    char sentinel = 'Z';
    ASSERT_NULL(cbm_safe_getenv(name, NULL, 0, "fallback"));
    ASSERT_NULL(cbm_safe_getenv(name, &sentinel, 0, "fallback"));
    ASSERT_EQ(sentinel, 'Z');

    memset(buf, '?', sizeof(buf));
    ASSERT_NULL(cbm_safe_getenv(NULL, buf, sizeof(buf), "fallback"));
    ASSERT_STR_EQ(buf, "");

    platform_env_restore(&saved);
    PASS();
}

TEST(platform_home_dir_precedence_and_normalization) {
    const char *names[] = {"HOME", "USERPROFILE"};
    platform_env_snapshot_t saved[sizeof(names) / sizeof(names[0])];
    platform_env_save_many(saved, names, sizeof(names) / sizeof(names[0]));

    ASSERT_EQ(cbm_setenv("HOME", "c:\\Home\\Primary", 1), 0);
    ASSERT_EQ(cbm_setenv("USERPROFILE", "d:\\Profile", 1), 0);
    ASSERT_STR_EQ(cbm_get_home_dir(), "C:/Home/Primary");

    ASSERT_EQ(cbm_unsetenv("HOME"), 0);
    ASSERT_STR_EQ(cbm_get_home_dir(), "D:/Profile");

    ASSERT_EQ(cbm_unsetenv("USERPROFILE"), 0);
    ASSERT_NULL(cbm_get_home_dir());

    platform_env_restore_many(saved, sizeof(saved) / sizeof(saved[0]));
    PASS();
}

TEST(platform_app_dirs_precedence_and_fallback) {
    const char *names[] = {"HOME", "USERPROFILE", "XDG_CONFIG_HOME", "APPDATA", "LOCALAPPDATA"};
    platform_env_snapshot_t saved[sizeof(names) / sizeof(names[0])];
    platform_env_save_many(saved, names, sizeof(names) / sizeof(names[0]));

    ASSERT_EQ(cbm_setenv("HOME", "c:\\Home\\Primary", 1), 0);
    ASSERT_EQ(cbm_unsetenv("USERPROFILE"), 0);

#ifdef _WIN32
    ASSERT_EQ(cbm_setenv("APPDATA", "e:\\Config\\Roaming", 1), 0);
    ASSERT_EQ(cbm_setenv("LOCALAPPDATA", "f:\\Config\\Local", 1), 0);
    ASSERT_STR_EQ(cbm_app_config_dir(), "E:/Config/Roaming");
    ASSERT_STR_EQ(cbm_app_local_dir(), "F:/Config/Local");

    ASSERT_EQ(cbm_unsetenv("APPDATA"), 0);
    ASSERT_EQ(cbm_unsetenv("LOCALAPPDATA"), 0);
    ASSERT_STR_EQ(cbm_app_config_dir(), "C:/Home/Primary/AppData/Roaming");
    ASSERT_STR_EQ(cbm_app_local_dir(), "C:/Home/Primary/AppData/Local");
#else
    ASSERT_EQ(cbm_setenv("XDG_CONFIG_HOME", "/tmp/cbm-config", 1), 0);
    ASSERT_STR_EQ(cbm_app_config_dir(), "/tmp/cbm-config");
    ASSERT_STR_EQ(cbm_app_local_dir(), "/tmp/cbm-config");

    ASSERT_EQ(cbm_unsetenv("XDG_CONFIG_HOME"), 0);
    ASSERT_STR_EQ(cbm_app_config_dir(), "C:/Home/Primary/.config");
    ASSERT_STR_EQ(cbm_app_local_dir(), "C:/Home/Primary/.config");
#endif

    ASSERT_EQ(cbm_unsetenv("HOME"), 0);
    ASSERT_NULL(cbm_app_config_dir());
    ASSERT_NULL(cbm_app_local_dir());

    platform_env_restore_many(saved, sizeof(saved) / sizeof(saved[0]));
    PASS();
}

TEST(platform_cache_dir_override_and_fallback) {
    const char *names[] = {"HOME", "USERPROFILE", "CBM_CACHE_DIR"};
    platform_env_snapshot_t saved[sizeof(names) / sizeof(names[0])];
    platform_env_save_many(saved, names, sizeof(names) / sizeof(names[0]));

    ASSERT_EQ(cbm_setenv("HOME", "c:\\Home\\Primary", 1), 0);
    ASSERT_EQ(cbm_unsetenv("USERPROFILE"), 0);
    ASSERT_EQ(cbm_setenv("CBM_CACHE_DIR", "g:\\Cache\\Root", 1), 0);
    ASSERT_STR_EQ(cbm_resolve_cache_dir(), "G:/Cache/Root");

    ASSERT_EQ(cbm_unsetenv("CBM_CACHE_DIR"), 0);
    ASSERT_STR_EQ(cbm_resolve_cache_dir(), "C:/Home/Primary/.cache/codebase-memory-mcp");

    ASSERT_EQ(cbm_unsetenv("HOME"), 0);
    ASSERT_NULL(cbm_resolve_cache_dir());

    platform_env_restore_many(saved, sizeof(saved) / sizeof(saved[0]));
    PASS();
}

TEST(platform_env_dir_truncation_contract) {
    const char *names[] = {"HOME",    "USERPROFILE",  "XDG_CONFIG_HOME",
                           "APPDATA", "LOCALAPPDATA", "CBM_CACHE_DIR"};
    platform_env_snapshot_t saved[sizeof(names) / sizeof(names[0])];
    platform_env_save_many(saved, names, sizeof(names) / sizeof(names[0]));

    char long_home[301];
    char long_cache[301];
    platform_fill_repeated(long_home, sizeof(long_home), 'a');
    platform_fill_repeated(long_cache, sizeof(long_cache), 'b');

    ASSERT_EQ(cbm_setenv("HOME", long_home, 1), 0);
    ASSERT_EQ(cbm_unsetenv("USERPROFILE"), 0);
    ASSERT_EQ(cbm_unsetenv("XDG_CONFIG_HOME"), 0);
    ASSERT_EQ(cbm_unsetenv("APPDATA"), 0);
    ASSERT_EQ(cbm_unsetenv("LOCALAPPDATA"), 0);
    ASSERT_EQ(cbm_unsetenv("CBM_CACHE_DIR"), 0);

    ASSERT_NOT_NULL(cbm_get_home_dir());
    ASSERT_EQ(strlen(cbm_get_home_dir()), 255);
#ifdef _WIN32
    ASSERT_EQ(strlen(cbm_app_config_dir()), 255 + strlen("/AppData/Roaming"));
    ASSERT_EQ(strlen(cbm_app_local_dir()), 255 + strlen("/AppData/Local"));
#else
    ASSERT_EQ(strlen(cbm_app_config_dir()), 255 + strlen("/.config"));
    ASSERT_EQ(strlen(cbm_app_local_dir()), 255 + strlen("/.config"));
#endif

    ASSERT_EQ(cbm_setenv("CBM_CACHE_DIR", long_cache, 1), 0);
    ASSERT_NOT_NULL(cbm_resolve_cache_dir());
    ASSERT_EQ(strlen(cbm_resolve_cache_dir()), 255);

    platform_env_restore_many(saved, sizeof(saved) / sizeof(saved[0]));
    PASS();
}

/*
 * CBM_WORKERS env override for cbm_default_worker_count.
 *
 * Containers running cbm on a host with more CPUs than the cgroup's
 * effective quota currently see ~host_cpu workers spawned because
 * sysconf(_SC_NPROCESSORS_ONLN) is not cgroup-aware (see GitHub
 * issue for the cgroup-detection ask). CBM_WORKERS is the smaller,
 * explicit-override path that ships independently.
 */
TEST(platform_default_workers_env_override) {
    cbm_setenv("CBM_WORKERS", "4", 1);
    int n = cbm_default_worker_count(true);
    ASSERT_EQ(n, 4);
    /* initial=false should also honor the explicit override. */
    int m = cbm_default_worker_count(false);
    ASSERT_EQ(m, 4);
    cbm_unsetenv("CBM_WORKERS");
    PASS();
}

TEST(platform_default_workers_env_invalid) {
    /* Out-of-range values (< 1 or > 256) and non-numeric strings
     * fall back to the sysconf-derived default. */
    int baseline = cbm_default_worker_count(true);
    ASSERT_GT(baseline, 0);

    cbm_setenv("CBM_WORKERS", "0", 1);
    ASSERT_EQ(cbm_default_worker_count(true), baseline);

    cbm_setenv("CBM_WORKERS", "-1", 1);
    ASSERT_EQ(cbm_default_worker_count(true), baseline);

    cbm_setenv("CBM_WORKERS", "9999", 1);
    ASSERT_EQ(cbm_default_worker_count(true), baseline);

    cbm_setenv("CBM_WORKERS", "not-a-number", 1);
    ASSERT_EQ(cbm_default_worker_count(true), baseline);

    cbm_unsetenv("CBM_WORKERS");
    PASS();
}

TEST(platform_default_workers_env_unset) {
    /* When CBM_WORKERS is unset the result matches today's behaviour
     * (info.total_cores for initial=true, perf_cores-1 for false). */
    cbm_unsetenv("CBM_WORKERS");
    cbm_system_info_t info = cbm_system_info();
    ASSERT_EQ(cbm_default_worker_count(true), info.total_cores);
    PASS();
}

/* ── cgroup-aware detection (Linux only) ─────────────────────────── */

#ifdef __linux__

/* Create a unique tmp directory the caller will own; returns 0 on success. */
static int cgroup_test_setup(char *root, size_t root_sz) {
    strncpy(root, "/tmp/cbm_cgroup_test_XXXXXX", root_sz);
    return mkdtemp(root) != NULL ? 0 : -1;
}

/* Write `content` to "<root>/<relpath>". Creates parent subdir if needed.
 * Returns 0 on success, -1 on any failure. */
static int cgroup_test_write(const char *root, const char *relpath, const char *content) {
    char path[1024];
    const char *slash = strchr(relpath, '/');
    if (slash != NULL) {
        char subdir[1024];
        size_t n = (size_t)(slash - relpath);
        if (n >= sizeof(subdir)) {
            return -1;
        }
        memcpy(subdir, relpath, n);
        subdir[n] = '\0';
        snprintf(path, sizeof(path), "%s/%s", root, subdir);
        (void)mkdir(path, S_IRWXU);
    }
    snprintf(path, sizeof(path), "%s/%s", root, relpath);
    FILE *fp = fopen(path, "we");
    if (fp == NULL) {
        return -1;
    }
    size_t n = strlen(content);
    int rc = (fwrite(content, 1, n, fp) == n) ? 0 : -1;
    fclose(fp);
    return rc;
}

/* Recursively remove a tmp dir created by cgroup_test_setup. Best-effort.
 * Uses opendir/unlink/rmdir rather than system("rm -rf ...") to avoid
 * spawning a shell from the test binary. */
static void cgroup_test_teardown(const char *root) {
    DIR *d = opendir(root);
    if (d != NULL) {
        struct dirent *ent;
        while ((ent = readdir(d)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                continue;
            }
            char child[1024];
            snprintf(child, sizeof(child), "%s/%s", root, ent->d_name);
            struct stat st;
            if (stat(child, &st) == 0 && S_ISDIR(st.st_mode)) {
                cgroup_test_teardown(child); /* recurse into subdir */
            } else {
                (void)unlink(child);
            }
        }
        closedir(d);
    }
    (void)rmdir(root);
}

TEST(cgroup_v2_cpu_quota) {
    char root[64];
    ASSERT_EQ(cgroup_test_setup(root, sizeof(root)), 0);
    /* 200ms quota in a 100ms period → 2 effective CPUs. */
    ASSERT_EQ(cgroup_test_write(root, "cpu.max", "200000 100000\n"), 0);
    ASSERT_EQ(cbm_detect_cgroup_cpus(root), 2);
    cgroup_test_teardown(root);
    PASS();
}

TEST(cgroup_v2_cpu_quota_rounds_up) {
    char root[64];
    ASSERT_EQ(cgroup_test_setup(root, sizeof(root)), 0);
    /* 150ms quota / 100ms period = 1.5 → ceil = 2. */
    ASSERT_EQ(cgroup_test_write(root, "cpu.max", "150000 100000\n"), 0);
    ASSERT_EQ(cbm_detect_cgroup_cpus(root), 2);
    cgroup_test_teardown(root);
    PASS();
}

TEST(cgroup_v2_cpu_unlimited) {
    char root[64];
    ASSERT_EQ(cgroup_test_setup(root, sizeof(root)), 0);
    ASSERT_EQ(cgroup_test_write(root, "cpu.max", "max 100000\n"), 0);
    ASSERT_EQ(cbm_detect_cgroup_cpus(root), -1);
    cgroup_test_teardown(root);
    PASS();
}

TEST(cgroup_v1_cpu_quota) {
    char root[64];
    ASSERT_EQ(cgroup_test_setup(root, sizeof(root)), 0);
    ASSERT_EQ(cgroup_test_write(root, "cpu/cpu.cfs_quota_us", "200000"), 0);
    ASSERT_EQ(cgroup_test_write(root, "cpu/cpu.cfs_period_us", "100000"), 0);
    ASSERT_EQ(cbm_detect_cgroup_cpus(root), 2);
    cgroup_test_teardown(root);
    PASS();
}

TEST(cgroup_v1_cpu_unlimited) {
    char root[64];
    ASSERT_EQ(cgroup_test_setup(root, sizeof(root)), 0);
    /* quota=-1 is the cgroup-v1 sentinel for "no quota". */
    ASSERT_EQ(cgroup_test_write(root, "cpu/cpu.cfs_quota_us", "-1"), 0);
    ASSERT_EQ(cgroup_test_write(root, "cpu/cpu.cfs_period_us", "100000"), 0);
    ASSERT_EQ(cbm_detect_cgroup_cpus(root), -1);
    cgroup_test_teardown(root);
    PASS();
}

TEST(cgroup_no_cpu_files) {
    char root[64];
    ASSERT_EQ(cgroup_test_setup(root, sizeof(root)), 0);
    /* Empty tmp dir: no v2 file, no v1 file → fall through to sysconf. */
    ASSERT_EQ(cbm_detect_cgroup_cpus(root), -1);
    cgroup_test_teardown(root);
    PASS();
}

TEST(cgroup_v2_mem) {
    char root[64];
    ASSERT_EQ(cgroup_test_setup(root, sizeof(root)), 0);
    /* 2 GiB. */
    ASSERT_EQ(cgroup_test_write(root, "memory.max", "2147483648\n"), 0);
    ASSERT_EQ(cbm_detect_cgroup_mem(root), (size_t)2147483648UL);
    cgroup_test_teardown(root);
    PASS();
}

TEST(cgroup_v2_mem_unlimited) {
    char root[64];
    ASSERT_EQ(cgroup_test_setup(root, sizeof(root)), 0);
    ASSERT_EQ(cgroup_test_write(root, "memory.max", "max\n"), 0);
    ASSERT_EQ(cbm_detect_cgroup_mem(root), (size_t)0);
    cgroup_test_teardown(root);
    PASS();
}

TEST(cgroup_v1_mem) {
    char root[64];
    ASSERT_EQ(cgroup_test_setup(root, sizeof(root)), 0);
    /* 1 GiB. */
    ASSERT_EQ(cgroup_test_write(root, "memory/memory.limit_in_bytes", "1073741824"), 0);
    ASSERT_EQ(cbm_detect_cgroup_mem(root), (size_t)1073741824UL);
    cgroup_test_teardown(root);
    PASS();
}

TEST(cgroup_v1_mem_unlimited_sentinel) {
    char root[64];
    ASSERT_EQ(cgroup_test_setup(root, sizeof(root)), 0);
    /* cgroup v1 reports a huge near-ULLONG_MAX value when unlimited
     * (PAGE_COUNTER_MAX). Our parser treats anything >= ULLONG_MAX/2
     * as effectively unlimited. */
    ASSERT_EQ(cgroup_test_write(root, "memory/memory.limit_in_bytes", "9223372036854775807"), 0);
    ASSERT_EQ(cbm_detect_cgroup_mem(root), (size_t)0);
    cgroup_test_teardown(root);
    PASS();
}

TEST(cgroup_no_mem_files) {
    char root[64];
    ASSERT_EQ(cgroup_test_setup(root, sizeof(root)), 0);
    ASSERT_EQ(cbm_detect_cgroup_mem(root), (size_t)0);
    cgroup_test_teardown(root);
    PASS();
}

#endif /* __linux__ */

SUITE(platform) {
    RUN_TEST(platform_now_ns);
    RUN_TEST(platform_now_ms);
    RUN_TEST(platform_nprocs);
    RUN_TEST(platform_file_exists);
    RUN_TEST(platform_is_dir);
    RUN_TEST(platform_file_size);
    RUN_TEST(platform_mmap);
    RUN_TEST(platform_mmap_nonexistent);
    RUN_TEST(platform_normalize_path_sep);
    RUN_TEST(platform_safe_getenv_contract);
    RUN_TEST(platform_home_dir_precedence_and_normalization);
    RUN_TEST(platform_app_dirs_precedence_and_fallback);
    RUN_TEST(platform_cache_dir_override_and_fallback);
    RUN_TEST(platform_env_dir_truncation_contract);
    RUN_TEST(platform_default_workers_env_override);
    RUN_TEST(platform_default_workers_env_invalid);
    RUN_TEST(platform_default_workers_env_unset);
#ifdef __linux__
    RUN_TEST(cgroup_v2_cpu_quota);
    RUN_TEST(cgroup_v2_cpu_quota_rounds_up);
    RUN_TEST(cgroup_v2_cpu_unlimited);
    RUN_TEST(cgroup_v1_cpu_quota);
    RUN_TEST(cgroup_v1_cpu_unlimited);
    RUN_TEST(cgroup_no_cpu_files);
    RUN_TEST(cgroup_v2_mem);
    RUN_TEST(cgroup_v2_mem_unlimited);
    RUN_TEST(cgroup_v1_mem);
    RUN_TEST(cgroup_v1_mem_unlimited_sentinel);
    RUN_TEST(cgroup_no_mem_files);
#endif
}
