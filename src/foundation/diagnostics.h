/*
 * diagnostics.h — Periodic diagnostics file writer.
 *
 * When CBM_DIAGNOSTICS=1, writes /tmp/cbm-diagnostics-<pid>.json every 5s.
 * Soak tests read this file to track memory, FDs, query stats over time.
 */
#ifndef CBM_DIAGNOSTICS_H
#define CBM_DIAGNOSTICS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>

/* Global query stats — updated by the MCP server on each tool call. */
typedef struct {
    atomic_int count;     /* total tool calls */
    atomic_int errors;    /* tool calls that returned isError=true */
    atomic_llong time_us; /* cumulative wall-clock time (microseconds) */
    atomic_llong max_us;  /* max single call time (microseconds) */
} cbm_query_stats_t;

/* Singleton query stats — MCP server increments these. */
extern cbm_query_stats_t g_query_stats;

/* diagnostics 純邏輯用的查詢統計快照；供 writer 與測試共用。 */
typedef struct {
    int count;
    int errors;
    long long total_us;
    long long avg_us;
    long long max_us;
} cbm_diag_query_snapshot_t;

/* diagnostics JSON/NDJSON formatter 的 deterministic 輸入。 */
typedef struct {
    long uptime_s;
    size_t rss_bytes;
    size_t peak_rss_bytes;
    size_t heap_committed_bytes;
    size_t peak_committed_bytes;
    size_t page_faults;
    int fd_count;
    cbm_diag_query_snapshot_t queries;
    int pid;
} cbm_diag_snapshot_t;

/* 判斷 CBM_DIAGNOSTICS 的單一 env 值是否啟用。 */
bool cbm_diag_env_enabled_value(const char *value);

/* 從 atomic 查詢統計建立 deterministic 快照。 */
void cbm_diag_query_stats_snapshot(cbm_query_stats_t *stats, cbm_diag_query_snapshot_t *out);

/* 組出 diagnostics 檔案路徑；ext 例如 "json" 或 "ndjson"。 */
int cbm_diag_format_path(char *buf, size_t buf_sz, const char *tmpdir, int pid, const char *ext);

/* 將快照格式化為最新狀態 JSON。
 * 成功回傳寫入字元數，緩衝區不足回傳 -1。 */
int cbm_diag_format_json(char *buf, size_t buf_sz, const cbm_diag_snapshot_t *snapshot);

/* 將快照格式化為 trajectory NDJSON 單行。
 * 成功回傳寫入字元數，緩衝區不足回傳 -1。 */
int cbm_diag_format_ndjson(char *buf, size_t buf_sz, const cbm_diag_snapshot_t *snapshot);

/* Record a completed tool call. */
void cbm_diag_record_query(long long duration_us, bool is_error);

/* Start the diagnostics writer thread (if CBM_DIAGNOSTICS env is set).
 * Call once from main(). Returns true if started. */
bool cbm_diag_start(void);

/* Stop the writer thread and delete the diagnostics file. */
void cbm_diag_stop(void);

#endif /* CBM_DIAGNOSTICS_H */
