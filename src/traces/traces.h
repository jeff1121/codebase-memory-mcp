/*
 * traces.h - OTLP trace 內容處理 helper。
 *
 * 這些函式維持既有對外契約；`CBM_USE_RUST_TRACES=1` 時，C 包裝器會委派到
 * Rust ABI；`CBM_USE_RUST_TRACES_ONLY=1` 時，C build 不編譯本模組，Rust staticlib
 * 直接匯出以下符號。`ingest_traces` handler 與其他 MCP 行為仍由 C 控制。
 */
#ifndef CBM_TRACES_H
#define CBM_TRACES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ── 屬性鍵值對 ─────────────────────────────────────────────────── */

typedef struct {
    const char *key;
    const char *string_value;
} cbm_trace_attr_t;

/* ── Resource（service-level info）───────────────────────────────── */

typedef struct {
    cbm_trace_attr_t *attributes;
    int attr_count;
} cbm_trace_resource_t;

/* ── Span ─────────────────────────────────────────────────────────── */

typedef struct {
    int kind; /* 1=internal, 2=server, 3=client */
    cbm_trace_attr_t *attributes;
    int attr_count;
    const char *start_time; /* nanosecond timestamp string */
    const char *end_time;   /* nanosecond timestamp string */
} cbm_trace_span_t;

/* ── HTTP span 資訊（extractHTTPInfo 的結果）────────────────────── */

typedef struct {
    const char *service_name;
    const char *method;
    const char *path;
    const char *status_code;
    int span_kind;
    int64_t duration_ns;
} cbm_http_span_info_t;

/* ── Helper 函式 ─────────────────────────────────────────────────── */

/* 從 resource attributes 取出 "service.name"，找不到就回傳 ""。 */
const char *cbm_extract_service_name(const cbm_trace_resource_t *r);

/* 從 span 取出 HTTP 資訊。若不是 HTTP span 則回傳 false。
 * 會填入 method、path、status、duration；字串指標仍借用 span 內部資料，
 * 不是 heap 配置。 */
bool cbm_extract_http_info(const cbm_trace_span_t *span, const char *service_name,
                           cbm_http_span_info_t *out);

/* 從完整 URL 取出 path。
 * 例如 "https://example.com/api/orders?q=1" → "/api/orders"
 * 會寫入 buf（最多 buf_sz bytes）。若不是有效 URL，回傳 ""。 */
const char *cbm_extract_path_from_url(const char *url, char *buf, size_t buf_sz);

/* 解析奈秒時間戳字串並回傳 duration。 */
int64_t cbm_parse_duration(const char *start_nano, const char *end_nano);

/* 從 int64_t 陣列計算 P99。
 * 會原地排序；空陣列回傳 0。 */
int64_t cbm_calculate_p99(int64_t *values, int count);

#endif /* CBM_TRACES_H */
