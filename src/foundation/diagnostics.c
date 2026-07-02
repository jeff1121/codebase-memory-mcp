/*
 * diagnostics.c — Periodic diagnostics file writer.
 *
 * Writes JSON to /tmp/cbm-diagnostics-<pid>.json every 5 seconds.
 * Atomic: writes .tmp then renames to avoid partial reads.
 */
#include "foundation/constants.h"
#include "foundation/diagnostics.h"
#include <stdatomic.h>
#include "foundation/mem.h"
#include "foundation/compat.h"
#include "foundation/compat_thread.h"
#include "foundation/compat_fs.h"
#include "foundation/platform.h"

#include <mimalloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <dirent.h>
#include <unistd.h>
#endif

/* ── Globals ─────────────────────────────────────────────────────── */

cbm_query_stats_t g_query_stats = {0};
static atomic_int g_diag_stop = 0;
static cbm_thread_t g_diag_thread;
static bool g_diag_started = false;
static time_t g_start_time = 0;
static char g_diag_path[CBM_SZ_256] = "";
/* Persistent NDJSON time-series (the memory TRAJECTORY users send us to diagnose
 * slow leaks like #581). Unlike g_diag_path (a latest-snapshot file overwritten
 * every interval and deleted on stop), this is appended-to and KEPT on exit. */
static char g_diag_ndjson_path[CBM_SZ_256] = "";
static size_t g_diag_ndjson_size = 0;
#define DIAG_NDJSON_CAP_BYTES (8u * 1024u * 1024u) /* rotate to .1 past this */

/* ── 純函式輔助工具 ────────────────────────────────────────────── */

static int diag_format_result(int n, size_t buf_sz) {
    if (n < 0 || (size_t)n >= buf_sz) {
        return CBM_NOT_FOUND;
    }
    return n;
}

bool cbm_diag_env_enabled_value(const char *value) {
    return value != NULL && (strcmp(value, "1") == 0 || strcmp(value, "true") == 0);
}

void cbm_diag_query_stats_snapshot(cbm_query_stats_t *stats, cbm_diag_query_snapshot_t *out) {
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    if (stats == NULL) {
        return;
    }

    out->count = atomic_load(&stats->count);
    out->errors = atomic_load(&stats->errors);
    out->total_us = atomic_load(&stats->time_us);
    out->max_us = atomic_load(&stats->max_us);
    out->avg_us = out->count > 0 ? out->total_us / out->count : 0;
}

int cbm_diag_format_path(char *buf, size_t buf_sz, const char *tmpdir, int pid, const char *ext) {
    if (buf == NULL || buf_sz == 0 || tmpdir == NULL || ext == NULL) {
        return CBM_NOT_FOUND;
    }
    int n = snprintf(buf, buf_sz, "%s/cbm-diagnostics-%d.%s", tmpdir, pid, ext);
    return diag_format_result(n, buf_sz);
}

int cbm_diag_format_json(char *buf, size_t buf_sz, const cbm_diag_snapshot_t *snapshot) {
    if (buf == NULL || buf_sz == 0 || snapshot == NULL) {
        return CBM_NOT_FOUND;
    }
    const cbm_diag_query_snapshot_t *q = &snapshot->queries;
    int n = snprintf(buf, buf_sz,
                     "{\n"
                     "  \"uptime_s\": %ld,\n"
                     "  \"rss_bytes\": %zu,\n"
                     "  \"peak_rss_bytes\": %zu,\n"
                     "  \"heap_committed_bytes\": %zu,\n"
                     "  \"peak_committed_bytes\": %zu,\n"
                     "  \"page_faults\": %zu,\n"
                     "  \"fd_count\": %d,\n"
                     "  \"query_count\": %d,\n"
                     "  \"query_errors\": %d,\n"
                     "  \"query_total_us\": %lld,\n"
                     "  \"query_avg_us\": %lld,\n"
                     "  \"query_max_us\": %lld,\n"
                     "  \"pid\": %d\n"
                     "}\n",
                     snapshot->uptime_s, snapshot->rss_bytes, snapshot->peak_rss_bytes,
                     snapshot->heap_committed_bytes, snapshot->peak_committed_bytes,
                     snapshot->page_faults, snapshot->fd_count, q->count, q->errors, q->total_us,
                     q->avg_us, q->max_us, snapshot->pid);
    return diag_format_result(n, buf_sz);
}

int cbm_diag_format_ndjson(char *buf, size_t buf_sz, const cbm_diag_snapshot_t *snapshot) {
    if (buf == NULL || buf_sz == 0 || snapshot == NULL) {
        return CBM_NOT_FOUND;
    }
    int n = snprintf(buf, buf_sz,
                     "{\"uptime_s\":%ld,\"rss\":%zu,\"peak_rss\":%zu,\"committed\":%zu,"
                     "\"peak_committed\":%zu,\"page_faults\":%zu,\"fd\":%d,\"queries\":%d}\n",
                     snapshot->uptime_s, snapshot->rss_bytes, snapshot->peak_rss_bytes,
                     snapshot->heap_committed_bytes, snapshot->peak_committed_bytes,
                     snapshot->page_faults, snapshot->fd_count, snapshot->queries.count);
    return diag_format_result(n, buf_sz);
}

/* ── Query stats ─────────────────────────────────────────────────── */

void cbm_diag_record_query(long long duration_us, bool is_error) {
    atomic_fetch_add(&g_query_stats.count, 1);
    atomic_fetch_add(&g_query_stats.time_us, duration_us);
    if (is_error) {
        atomic_fetch_add(&g_query_stats.errors, 1);
    }
    /* Update max (lock-free CAS loop) */
    long long old_max = atomic_load(&g_query_stats.max_us);
    while (duration_us > old_max) {
        if (atomic_compare_exchange_weak(&g_query_stats.max_us, &old_max, duration_us)) {
            break;
        }
    }
}

/* ── FD count (platform-specific) ────────────────────────────────── */

static int count_open_fds(void) {
#ifdef __linux__
    struct dirent **entries = NULL;
    int n = scandir("/proc/self/fd", &entries, NULL, NULL);
    if (n < 0) {
        return CBM_NOT_FOUND;
    }
    for (int i = 0; i < n; i++) {
        free(entries[i]);
    }
    free(entries);
    return n - PAIR_LEN; /* . and .. */
#elif defined(__APPLE__)
    /* Count via /dev/fd using scandir (MT-safe) */
    struct dirent **entries = NULL;
    int n = scandir("/dev/fd", &entries, NULL, NULL);
    if (n < 0) {
        return CBM_NOT_FOUND;
    }
    for (int i = 0; i < n; i++) {
        free(entries[i]);
    }
    free(entries);
    return n - PAIR_LEN; /* . and .. */
#else
    return CBM_NOT_FOUND; /* Not available on Windows */
#endif
}

/* ── Writer ──────────────────────────────────────────────────────── */

#define DIAG_INTERVAL_S 5
#define DIAG_PATH_EXTRA 24 /* ".tmp" + safety margin */

/* Append one compact JSON line to the persistent NDJSON trajectory, rotating to
 * <path>.1 once it passes the cap. Best-effort: a failed append never disrupts
 * the server. The trajectory (a monotonic rss/committed climb over hours) is
 * what reveals a slow leak like #581 — the single latest-snapshot file cannot. */
static void append_trajectory(const cbm_diag_snapshot_t *snapshot) {
    if (g_diag_ndjson_path[0] == '\0') {
        return;
    }
    if (g_diag_ndjson_size > DIAG_NDJSON_CAP_BYTES) {
        char rot[sizeof(g_diag_ndjson_path) + DIAG_PATH_EXTRA];
        snprintf(rot, sizeof(rot), "%s.1", g_diag_ndjson_path);
        (void)rename(g_diag_ndjson_path, rot); /* keep one previous generation */
        g_diag_ndjson_size = 0;
    }
    FILE *f = fopen(g_diag_ndjson_path, "a");
    if (!f) {
        return;
    }
    char line[CBM_SZ_512];
    int n = cbm_diag_format_ndjson(line, sizeof(line), snapshot);
    if (n >= 0 && fputs(line, f) != EOF) {
        g_diag_ndjson_size += (size_t)n;
    }
    (void)fclose(f);
}

static void write_diagnostics(void) {
    /* Collect mimalloc stats */
    size_t elapsed_ms = 0;
    size_t user_ms = 0;
    size_t sys_ms = 0;
    size_t current_rss = 0;
    size_t peak_rss = 0;
    size_t current_commit = 0;
    size_t peak_commit = 0;
    size_t page_faults = 0;
    mi_process_info(&elapsed_ms, &user_ms, &sys_ms, &current_rss, &peak_rss, &current_commit,
                    &peak_commit, &page_faults);

    /* Fallback RSS for ASan builds */
    if (current_rss == 0) {
        current_rss = cbm_mem_rss();
    }

    int fds = count_open_fds();
    time_t now = time(NULL);
    long uptime = (long)(now - g_start_time);

    cbm_diag_snapshot_t snapshot = {
        .uptime_s = uptime,
        .rss_bytes = current_rss,
        .peak_rss_bytes = peak_rss,
        .heap_committed_bytes = current_commit,
        .peak_committed_bytes = peak_commit,
        .page_faults = page_faults,
        .fd_count = fds,
        .pid = (int)getpid(),
    };
    cbm_diag_query_stats_snapshot(&g_query_stats, &snapshot.queries);

    /* Write to .tmp then rename (atomic) */
    char tmp_path[sizeof(g_diag_path) + DIAG_PATH_EXTRA];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", g_diag_path);

    FILE *f = fopen(tmp_path, "w");
    if (!f) {
        return;
    }

    char json[CBM_SZ_2K];
    if (cbm_diag_format_json(json, sizeof(json), &snapshot) < 0 || fputs(json, f) == EOF) {
        (void)fclose(f);
        return;
    }
    if (fclose(f) != 0) {
        return;
    }
    (void)rename(tmp_path, g_diag_path);

    /* Also append to the persistent trajectory (kept on exit for users to send). */
    append_trajectory(&snapshot);
}

static void *diag_thread_fn(void *arg) {
    (void)arg;
    while (!atomic_load(&g_diag_stop)) {
        write_diagnostics();
        struct timespec ts = {DIAG_INTERVAL_S, 0};
        cbm_nanosleep(&ts, NULL);
    }
    /* Final write before exit */
    write_diagnostics();
    return NULL;
}

/* ── Public API ──────────────────────────────────────────────────── */

bool cbm_diag_start(void) {
    char env_buf[CBM_SZ_32] = "";
    cbm_safe_getenv("CBM_DIAGNOSTICS", env_buf, sizeof(env_buf), NULL);
    if (!cbm_diag_env_enabled_value(env_buf)) {
        return false;
    }

    g_start_time = time(NULL);
    atomic_store(&g_diag_stop, 0);

    if (cbm_diag_format_path(g_diag_path, sizeof(g_diag_path), cbm_tmpdir(), (int)getpid(),
                             "json") < 0 ||
        cbm_diag_format_path(g_diag_ndjson_path, sizeof(g_diag_ndjson_path), cbm_tmpdir(),
                             (int)getpid(), "ndjson") < 0) {
        return false;
    }
    g_diag_ndjson_size = 0;

    if (cbm_thread_create(&g_diag_thread, 0, diag_thread_fn, NULL) != 0) {
        return false;
    }

    g_diag_started = true;
    (void)fprintf(stderr,
                  "level=info msg=diagnostics.start snapshot=%s trajectory=%s interval=%ds\n",
                  g_diag_path, g_diag_ndjson_path, DIAG_INTERVAL_S);
    return true;
}

void cbm_diag_stop(void) {
    if (!g_diag_started) {
        return;
    }
    atomic_store(&g_diag_stop, 1);
    cbm_thread_join(&g_diag_thread);
    g_diag_started = false;

    /* Remove the live latest-snapshot file (and its .tmp), but KEEP the
     * trajectory NDJSON so the user can send it after the server exits. */
    cbm_unlink(g_diag_path);
    char tmp_path[sizeof(g_diag_path) + DIAG_PATH_EXTRA];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", g_diag_path);
    cbm_unlink(tmp_path);
    if (g_diag_ndjson_path[0] != '\0') {
        (void)fprintf(stderr, "level=info msg=diagnostics.trajectory_kept path=%s\n",
                      g_diag_ndjson_path);
    }
}
