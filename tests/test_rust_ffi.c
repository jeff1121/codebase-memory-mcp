/*
 * test_rust_ffi.c - 階段性 Rust staticlib 匯出的 C smoke tests。
 *
 * 這個檔案刻意使用自己的 main，不連 tests/test_main.c，讓 Rust ABI gate
 * 可以不依賴完整 C test runner。
 */
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pipeline/artifact.h"
#include "cypher/cypher.h"
#include "foundation/arena.h"
#include "foundation/dump_verify.h"
#include "graph_buffer/graph_buffer.h"
#include "pipeline/pipeline.h"
#include "pipeline/pipeline_internal.h"
#include "pipeline/json_prop.h"
#include "pipeline/lsp_cross_classifiers.h"
#include "pipeline/split_command.h"
#include "pipeline/route_node_classifiers.h"
#include "pipeline/rust_plan.h"
#include "discover/discover.h"
#ifdef CBM_USE_RUST_PIPELINE_CHECKED_EXCEPTION_ONLY
extern int cbm_pipeline_is_checked_exception(const char *name);
#endif
#ifdef CBM_USE_RUST_DISCOVER_USERCONFIG_ONLY
#include "discover/userconfig.h"
#endif
#if defined(CBM_USE_RUST_DISCOVER_LANGUAGE_NAME) || \
    defined(CBM_USE_RUST_DISCOVER_LANGUAGE_NAME_ONLY)
extern const char *cbm_rs_discover_language_name_v1(int language);
#endif
extern int cbm_rs_discover_str_in_list_v1(const char *value, const char *const *list);
extern int cbm_rs_discover_ends_with_v1(const char *value, const char *suffix);
extern int cbm_rs_discover_str_contains_v1(const char *value, const char *substring);
extern int cbm_rs_discover_ascii_ieq_v1(const char *left, const char *right);
extern int cbm_rs_discover_has_trailing_sep_v1(const char *value);
extern void cbm_rs_discover_path_join_v1(const char *base, const char *relative, char *buf,
                                         size_t bufsize);
extern size_t cbm_rs_discover_local_rel_path_offset_v1(const char *rel_path,
                                                        const char *local_prefix);
extern size_t cbm_discover_local_rel_path_offset(const char *rel_path, const char *local_prefix);
extern int cbm_rs_watcher_poll_interval_ms_v1(int file_count);
extern int cbm_rs_pipeline_sveltekit_file_kind_v1(const char *file_path);
extern const char *cbm_rs_pipeline_sveltekit_server_method_v1(const char *name);
extern int cbm_rs_pipeline_extract_json_prop_v1(const char *json, const char *key, char *buf,
                                                int bufsz);
extern const char *cbm_rs_pipeline_url_path_v1(const char *url);
extern int cbm_rs_cli_hook_extract_token_v1(const char *pattern, char *buf, size_t bufsize);
extern int cbm_rs_git_path_is_absolute_v1(const char *path);
extern int cbm_rs_git_json_escaped_len_v1(const char *source);
extern void cbm_rs_git_trim_newlines_v1(char *value);
extern void cbm_rs_log_copy_path_without_query_v1(const char *path, char *buf, size_t bufsize);
#ifdef CBM_USE_RUST_DISCOVER_STRING_HELPERS_ONLY
extern bool cbm_discover_str_in_list(const char *value, const char *const *list);
extern bool cbm_discover_ends_with(const char *value, const char *suffix);
extern bool cbm_discover_str_contains(const char *value, const char *substring);
extern bool cbm_discover_ascii_ieq(const char *left, const char *right);
#endif
#ifdef CBM_USE_RUST_DISCOVER_TRAILING_SEP_ONLY
extern bool cbm_discover_has_trailing_sep(const char *value);
#endif
#ifdef CBM_USE_RUST_DISCOVER_PATH_JOIN_ONLY
extern void cbm_discover_path_join(const char *base, const char *relative, char *buf, size_t bufsize);
#endif
#ifdef CBM_USE_RUST_PIPELINE_INCREMENTAL_EDGE_ONLY
extern bool cbm_pipeline_incremental_edge_type_is_recomputed(const char *edge_type);
#endif
#ifdef CBM_USE_RUST_PIPELINE_SEMANTIC_FP_SUFFIX_ONLY
extern bool cbm_pipeline_semantic_fp_ends_with(const char *file_path, const char *suffix);
#endif
#ifdef CBM_USE_RUST_PIPELINE_INCREMENTAL_REGISTRY_LABEL_ONLY
extern bool cbm_pipeline_incremental_label_is_registry_symbol(const char *label);
#endif
#ifdef CBM_USE_RUST_MCP_EDGE_TYPE_ONLY
extern bool cbm_mcp_edge_type_valid(const char *edge_type);
#endif
#ifdef CBM_USE_RUST_WATCHER_POLL_INTERVAL_ONLY
extern int cbm_watcher_poll_interval_ms(int file_count);
#endif
#ifdef CBM_USE_RUST_PIPELINE_SVELTEKIT_FILE_KIND_ONLY
extern int cbm_pipeline_sveltekit_file_kind(const char *file_path);
#endif
#ifdef CBM_USE_RUST_PIPELINE_SVELTEKIT_SERVER_METHOD_ONLY
extern const char *cbm_pipeline_sveltekit_server_method(const char *name);
#endif
#ifdef CBM_USE_RUST_PIPELINE_URL_PATH_ONLY
extern const char *cbm_pipeline_url_path(const char *url);
#endif
#ifdef CBM_USE_RUST_CLI_HOOK_TOKEN_ONLY
extern bool cbm_cli_hook_extract_token(const char *pattern, char *buf, size_t bufsize);
#endif
#ifdef CBM_USE_RUST_GIT_PATH_ABSOLUTE_ONLY
extern bool cbm_git_path_is_absolute(const char *path);
#endif
#ifdef CBM_USE_RUST_GIT_JSON_ESCAPED_LEN_ONLY
extern int cbm_git_json_escaped_len(const char *source);
#endif
#ifdef CBM_USE_RUST_GIT_TRIM_NEWLINES_ONLY
extern void cbm_git_trim_newlines(char *value);
#endif
#ifdef CBM_USE_RUST_LOG_COPY_PATH_ONLY
extern void cbm_log_copy_path_without_query(const char *path, char *buf, size_t bufsize);
#endif
#ifdef CBM_USE_RUST_MEM_ONLY
#include "foundation/mem.h"
#endif
#ifdef CBM_USE_RUST_DIAGNOSTICS_ONLY
#include "foundation/diagnostics.h"
#endif
#ifdef CBM_USE_RUST_PIPELINE_PATH_ALIAS_ONLY
#include "pipeline/path_alias.h"
#endif
#ifndef _WIN32
#include <sys/stat.h>
#include <unistd.h>
#endif

_Static_assert(sizeof(CBMLanguage) == sizeof(int),
               "Rust LSP classifier ABI expects CBMLanguage to use int");
_Static_assert(CBM_ARENA_MAX_BLOCKS == 256, "Rust arena ABI expects CBM_ARENA_MAX_BLOCKS=256");
_Static_assert(CBM_ARENA_DEFAULT_BLOCK_SIZE == ((size_t)64 * 1024),
               "Rust arena ABI expects CBM_ARENA_DEFAULT_BLOCK_SIZE=64KiB");
_Static_assert(offsetof(CBMArena, blocks) == 0, "CBMArena.blocks offset drift");
_Static_assert(offsetof(CBMArena, block_sizes) == sizeof(((CBMArena *)0)->blocks),
               "CBMArena.block_sizes offset drift");
_Static_assert(offsetof(CBMArena, nblocks) ==
                   sizeof(((CBMArena *)0)->blocks) + sizeof(((CBMArena *)0)->block_sizes),
               "CBMArena.nblocks offset drift");
_Static_assert(offsetof(CBMArena, used) == offsetof(CBMArena, block_size) + sizeof(size_t),
               "CBMArena.used offset drift");
_Static_assert(offsetof(CBMArena, total_alloc) == offsetof(CBMArena, used) + sizeof(size_t),
               "CBMArena.total_alloc offset drift");
_Static_assert(sizeof(CBMArena) >= offsetof(CBMArena, total_alloc) + sizeof(size_t),
               "CBMArena size drift");
_Static_assert(CBM_MODE_FULL == 0, "Rust plan ABI expects CBM_MODE_FULL=0");
_Static_assert(CBM_MODE_MODERATE == 1, "Rust plan ABI expects CBM_MODE_MODERATE=1");
_Static_assert(CBM_MODE_FAST == 2, "Rust plan ABI expects CBM_MODE_FAST=2");

typedef struct {
    int start;
    int end;
    int has_next;
    int next_cursor;
} CbmRsMcpToolsPageBoundsV1;

_Static_assert(offsetof(CbmRsMcpToolsPageBoundsV1, start) == 0,
               "McpToolsPageBoundsV1.start offset drift");
_Static_assert(offsetof(CbmRsMcpToolsPageBoundsV1, end) == 4,
               "McpToolsPageBoundsV1.end offset drift");
_Static_assert(offsetof(CbmRsMcpToolsPageBoundsV1, has_next) == 8,
               "McpToolsPageBoundsV1.has_next offset drift");
_Static_assert(offsetof(CbmRsMcpToolsPageBoundsV1, next_cursor) == 12,
               "McpToolsPageBoundsV1.next_cursor offset drift");
_Static_assert(sizeof(CbmRsMcpToolsPageBoundsV1) == 16, "McpToolsPageBoundsV1 ABI size drift");

extern int cbm_rs_dump_verify_is_degraded(int committed_nodes, int persisted_nodes, double ratio,
                                          int min_floor);
extern double cbm_rs_dump_verify_parse_min_ratio_v1(const char *value, int *valid_out);
extern int cbm_rs_cli_compare_versions_v1(const char *left, const char *right);
#ifdef CBM_USE_RUST_CLI_VERSION_ONLY
extern int cbm_cli_compare_versions_v1(const char *left, const char *right);
#endif

extern void cbm_rs_arena_init(CBMArena *arena);
extern void cbm_rs_arena_init_sized(CBMArena *arena, size_t block_size);
extern void *cbm_rs_arena_alloc(CBMArena *arena, size_t n);
extern void *cbm_rs_arena_calloc(CBMArena *arena, size_t n);
extern char *cbm_rs_arena_strdup(CBMArena *arena, const char *value);
extern char *cbm_rs_arena_strndup(CBMArena *arena, const unsigned char *value, size_t len);
extern void cbm_rs_arena_reset(CBMArena *arena);
extern void cbm_rs_arena_destroy(CBMArena *arena);
extern size_t cbm_rs_arena_total(const CBMArena *arena);

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
extern void cbm_rs_mem_init(double ram_fraction);
extern size_t cbm_rs_mem_rss(void);
extern size_t cbm_rs_mem_peak_rss(void);
extern size_t cbm_rs_mem_budget(void);
extern bool cbm_rs_mem_over_budget(void);
extern size_t cbm_rs_mem_worker_budget(int num_workers);
extern void cbm_rs_mem_collect(void);

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

/* yaml：test-only parser/query 契約 parity（對齊 src/foundation/yaml.c） */
typedef struct CBMRustYamlNode CBMRustYamlNode;
extern CBMRustYamlNode *cbm_rs_yaml_parse(const unsigned char *text, int len);
extern void cbm_rs_yaml_free(CBMRustYamlNode *root);
extern const char *cbm_rs_yaml_get_str(const CBMRustYamlNode *root, const char *path);
extern double cbm_rs_yaml_get_float(const CBMRustYamlNode *root, const char *path,
                                    double default_val);
extern bool cbm_rs_yaml_get_bool(const CBMRustYamlNode *root, const char *path, bool default_val);
extern int cbm_rs_yaml_get_str_list(const CBMRustYamlNode *root, const char *path, const char **out,
                                    int max_out);
extern bool cbm_rs_yaml_has(const CBMRustYamlNode *root, const char *path);

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
extern int cbm_rs_compat_regex_known_flags(int flags);
extern int cbm_rs_compat_regex_match_cap(int nmatch, int has_matches);
extern int cbm_rs_compat_regex_status(int matched);
extern size_t cbm_rs_compat_thread_effective_stack_size(size_t stack_size);
extern bool cbm_rs_compat_aligned_alloc_precheck(size_t alignment, size_t size,
                                                 size_t pointer_size);
extern int cbm_rs_log_parse_level_value(const char *value, int current);
extern int cbm_rs_log_parse_format_value(const char *value, int current);
extern int cbm_rs_log_sanitize_text_atom(char *buf, size_t bufsize, const char *value);
extern int cbm_rs_log_json_string(char *buf, size_t bufsize, const char *value);
extern int cbm_rs_log_http_path_without_query(char *buf, size_t bufsize, const char *path);
extern int cbm_rs_log_http_status_level(int status);
extern bool cbm_rs_profile_env_enabled(const char *value);
extern int64_t cbm_rs_profile_elapsed_us(int64_t start_sec, int64_t start_nsec, int64_t now_sec,
                                         int64_t now_nsec);
extern int64_t cbm_rs_profile_elapsed_ms(int64_t us);
extern int64_t cbm_rs_profile_rate_per_s(int64_t items, int64_t us);
extern size_t cbm_rs_pipeline_project_name_from_path(char *buf, size_t bufsize,
                                                     const char *abs_path);
extern int cbm_rs_pipeline_is_env_var_name_v1(const char *value);
extern int cbm_rs_pipeline_normalize_config_key_v1(const char *key, char *norm_out,
                                                   size_t norm_sz);
extern int cbm_rs_pipeline_has_config_extension_v1(const char *path);
extern int cbm_rs_pipeline_is_dockerfile_v1(const char *name);
extern int cbm_rs_pipeline_is_compose_file_v1(const char *name);
extern int cbm_rs_pipeline_is_env_file_v1(const char *name);
extern int cbm_rs_pipeline_is_cloudbuild_file_v1(const char *name);
extern int cbm_rs_pipeline_is_kustomize_file_v1(const char *name);
extern int cbm_rs_pipeline_is_shell_script_v1(const char *name, const char *ext);
extern int cbm_rs_pipeline_is_helm_chart_file_v1(const char *name);
extern int cbm_rs_pipeline_is_gomod_file_v1(const char *name);
extern int cbm_rs_pipeline_is_requirements_file_v1(const char *name);
extern int cbm_rs_pipeline_is_trackable_file_v1(const char *path);
extern void cbm_rs_pipeline_parse_range_v1(const char *input, int *out_start, int *out_count);
extern int cbm_rs_pipeline_parse_name_status_v1(const char *input, void *out, int max_out);
extern int cbm_rs_pipeline_parse_hunks_v1(const char *input, void *out, int max_out);
extern int cbm_rs_pipeline_is_module_dir_v1(int language);
extern int cbm_rs_pipeline_plan_describe(int kind, int mode, int worker_count, int file_count,
                                         char *buf, size_t bufsize);
typedef cbm_rs_pipeline_plan_step_t CbmRsPipelinePlanStep;
typedef cbm_rs_pipeline_plan_step_v2_t CbmRsPipelinePlanStepV2;
typedef cbm_rs_pipeline_top_step_v1_t CbmRsPipelineTopStepV1;

typedef struct {
    int kind;
    int pos;
    int text_len;
    int reserved0;
} CbmRsCypherTokenV1;

typedef struct {
    const char *key;
    const char *string_value;
} CbmRsTraceAttrV1;

typedef struct {
    const CbmRsTraceAttrV1 *attributes;
    int attr_count;
} CbmRsTraceResourceV1;

typedef struct {
    int kind;
    const CbmRsTraceAttrV1 *attributes;
    int attr_count;
    const char *start_time;
    const char *end_time;
} CbmRsTraceSpanV1;

typedef struct {
    const char *service_name;
    const char *method;
    const char *path;
    const char *status_code;
    int span_kind;
    int64_t duration_ns;
} CbmRsHttpSpanInfoV1;

_Static_assert(sizeof(CbmRsTraceAttrV1) == 16, "TraceAttrV1 ABI size drift");
_Static_assert(offsetof(CbmRsTraceAttrV1, key) == 0, "TraceAttrV1.key offset drift");
_Static_assert(offsetof(CbmRsTraceAttrV1, string_value) == 8,
               "TraceAttrV1.string_value offset drift");
_Static_assert(sizeof(CbmRsTraceResourceV1) == 16, "TraceResourceV1 ABI size drift");
_Static_assert(offsetof(CbmRsTraceResourceV1, attributes) == 0,
               "TraceResourceV1.attributes offset drift");
_Static_assert(offsetof(CbmRsTraceResourceV1, attr_count) == 8,
               "TraceResourceV1.attr_count offset drift");
_Static_assert(sizeof(CbmRsTraceSpanV1) == 40, "TraceSpanV1 ABI size drift");
_Static_assert(offsetof(CbmRsTraceSpanV1, kind) == 0, "TraceSpanV1.kind offset drift");
_Static_assert(offsetof(CbmRsTraceSpanV1, attributes) == 8,
               "TraceSpanV1.attributes offset drift");
_Static_assert(offsetof(CbmRsTraceSpanV1, attr_count) == 16,
               "TraceSpanV1.attr_count offset drift");
_Static_assert(offsetof(CbmRsTraceSpanV1, start_time) == 24,
               "TraceSpanV1.start_time offset drift");
_Static_assert(offsetof(CbmRsTraceSpanV1, end_time) == 32,
               "TraceSpanV1.end_time offset drift");
_Static_assert(sizeof(CbmRsHttpSpanInfoV1) == 48, "HttpSpanInfoV1 ABI size drift");
_Static_assert(offsetof(CbmRsHttpSpanInfoV1, service_name) == 0,
               "HttpSpanInfoV1.service_name offset drift");
_Static_assert(offsetof(CbmRsHttpSpanInfoV1, method) == 8, "HttpSpanInfoV1.method offset drift");
_Static_assert(offsetof(CbmRsHttpSpanInfoV1, path) == 16, "HttpSpanInfoV1.path offset drift");
_Static_assert(offsetof(CbmRsHttpSpanInfoV1, status_code) == 24,
               "HttpSpanInfoV1.status_code offset drift");
_Static_assert(offsetof(CbmRsHttpSpanInfoV1, span_kind) == 32,
               "HttpSpanInfoV1.span_kind offset drift");
_Static_assert(offsetof(CbmRsHttpSpanInfoV1, duration_ns) == 40,
               "HttpSpanInfoV1.duration_ns offset drift");

extern const char *cbm_rs_traces_extract_service_name_v1(const CbmRsTraceResourceV1 *r);
extern bool cbm_rs_traces_extract_http_info_v1(const CbmRsTraceSpanV1 *span,
                                               const char *service_name,
                                               CbmRsHttpSpanInfoV1 *out);
extern const char *cbm_rs_traces_extract_path_from_url_v1(const char *url, char *buf,
                                                          size_t buf_sz);
extern int64_t cbm_rs_traces_parse_duration_v1(const char *start_nano, const char *end_nano);
extern int64_t cbm_rs_traces_calculate_p99_v1(int64_t *values, int count);

typedef struct {
    int64_t id;
    int has_id;
    int id_kind;
    int has_params;
    size_t jsonrpc_len;
    size_t method_len;
    size_t id_str_len;
    size_t params_len;
} CbmRsMcpJsonRpcParseOutV1;

enum {
    CBM_RS_MCP_ID_NONE = 0,
    CBM_RS_MCP_ID_INT = 1,
    CBM_RS_MCP_ID_STRING = 2,
    CBM_RS_MCP_ID_OTHER = 3,
};

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
_Static_assert(sizeof(CbmRsPipelineTopStepV1) == 32, "PipelineTopStepV1 ABI size drift");
_Static_assert(offsetof(CbmRsPipelineTopStepV1, kind) == 0, "PipelineTopStepV1.kind offset drift");
_Static_assert(offsetof(CbmRsPipelineTopStepV1, phase) == 4,
               "PipelineTopStepV1.phase offset drift");
_Static_assert(offsetof(CbmRsPipelineTopStepV1, policy) == 8,
               "PipelineTopStepV1.policy offset drift");
_Static_assert(offsetof(CbmRsPipelineTopStepV1, gate_flags) == 12,
               "PipelineTopStepV1.gate_flags offset drift");
_Static_assert(offsetof(CbmRsPipelineTopStepV1, requires_mask) == 16,
               "PipelineTopStepV1.requires_mask offset drift");
_Static_assert(offsetof(CbmRsPipelineTopStepV1, effect_flags) == 24,
               "PipelineTopStepV1.effect_flags offset drift");
_Static_assert(offsetof(CbmRsPipelineTopStepV1, nested_plan_kind) == 28,
               "PipelineTopStepV1.nested_plan_kind offset drift");
_Static_assert(TOK_MATCH == 0, "Rust Cypher lexer ABI expects TOK_MATCH=0");
_Static_assert(TOK_OPTIONAL == 45, "Rust Cypher lexer ABI expects TOK_OPTIONAL=45");
_Static_assert(TOK_CALL == 47, "Rust Cypher lexer ABI expects TOK_CALL=47");
_Static_assert(TOK_EXISTS == 51, "Rust Cypher lexer ABI expects TOK_EXISTS=51");
_Static_assert(TOK_LPAREN == 66, "Rust Cypher lexer ABI expects TOK_LPAREN=66");
_Static_assert(TOK_IDENT == 85, "Rust Cypher lexer ABI expects TOK_IDENT=85");
_Static_assert(TOK_STRING == 86, "Rust Cypher lexer ABI expects TOK_STRING=86");
_Static_assert(TOK_NUMBER == 87, "Rust Cypher lexer ABI expects TOK_NUMBER=87");
_Static_assert(TOK_EOF == 88, "Rust Cypher lexer ABI expects TOK_EOF=88");
_Static_assert(sizeof(CbmRsCypherTokenV1) == 16, "CypherTokenV1 ABI size drift");
_Static_assert(offsetof(CbmRsCypherTokenV1, kind) == 0, "CypherTokenV1.kind offset drift");
_Static_assert(offsetof(CbmRsCypherTokenV1, pos) == 4, "CypherTokenV1.pos offset drift");
_Static_assert(offsetof(CbmRsCypherTokenV1, text_len) == 8, "CypherTokenV1.text_len offset drift");
_Static_assert(offsetof(CbmRsCypherTokenV1, reserved0) == 12,
               "CypherTokenV1.reserved0 offset drift");
_Static_assert(offsetof(CbmRsMcpJsonRpcParseOutV1, id) == 0,
               "McpJsonRpcParseOutV1.id offset drift");
_Static_assert(offsetof(CbmRsMcpJsonRpcParseOutV1, has_id) == 8,
               "McpJsonRpcParseOutV1.has_id offset drift");
_Static_assert(offsetof(CbmRsMcpJsonRpcParseOutV1, id_kind) == 12,
               "McpJsonRpcParseOutV1.id_kind offset drift");
_Static_assert(offsetof(CbmRsMcpJsonRpcParseOutV1, has_params) == 16,
               "McpJsonRpcParseOutV1.has_params offset drift");
_Static_assert(offsetof(CbmRsMcpJsonRpcParseOutV1, jsonrpc_len) == 24,
               "McpJsonRpcParseOutV1.jsonrpc_len offset drift");
_Static_assert(offsetof(CbmRsMcpJsonRpcParseOutV1, method_len) == 24 + sizeof(size_t),
               "McpJsonRpcParseOutV1.method_len offset drift");
_Static_assert(offsetof(CbmRsMcpJsonRpcParseOutV1, id_str_len) == 24 + 2 * sizeof(size_t),
               "McpJsonRpcParseOutV1.id_str_len offset drift");
_Static_assert(offsetof(CbmRsMcpJsonRpcParseOutV1, params_len) == 24 + 3 * sizeof(size_t),
               "McpJsonRpcParseOutV1.params_len offset drift");
_Static_assert(sizeof(CbmRsMcpJsonRpcParseOutV1) == 24 + 4 * sizeof(size_t),
               "McpJsonRpcParseOutV1 ABI size drift");

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

typedef struct {
    int kind;
    uint32_t flags;
    const char *name;
    const char *table_name;
    const char *column_name;
} CbmRsStoreSchemaManifestEntryV1;

typedef struct {
    int kind;
    uint32_t flags;
    int ordinal;
    int hidden;
    const char *name;
    const char *table_name;
    const char *column_name;
    const char *column_type;
    const char *default_sql;
    const char *columns_csv;
    const char *sql_fragment;
} CbmRsStoreSchemaManifestEntryV2;

typedef struct {
    int kind;
    uint32_t flags;
    uint32_t mode_mask;
    int ordinal;
    int64_t value_i64;
    const char *name;
    const char *sql;
    const char *env_name;
} CbmRsStoreConnectionManifestEntryV1;

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
_Static_assert(offsetof(CbmRsStoreSchemaManifestEntryV1, kind) == 0,
               "StoreSchemaManifestEntryV1.kind offset drift");
_Static_assert(offsetof(CbmRsStoreSchemaManifestEntryV1, flags) == 4,
               "StoreSchemaManifestEntryV1.flags offset drift");
_Static_assert(offsetof(CbmRsStoreSchemaManifestEntryV1, name) == 8,
               "StoreSchemaManifestEntryV1.name offset drift");
_Static_assert(offsetof(CbmRsStoreSchemaManifestEntryV1, table_name) == 8 + sizeof(const char *),
               "StoreSchemaManifestEntryV1.table_name offset drift");
_Static_assert(offsetof(CbmRsStoreSchemaManifestEntryV1, column_name) ==
                   8 + 2 * sizeof(const char *),
               "StoreSchemaManifestEntryV1.column_name offset drift");
_Static_assert(sizeof(CbmRsStoreSchemaManifestEntryV1) == 8 + 3 * sizeof(const char *),
               "StoreSchemaManifestEntryV1 ABI size drift");
_Static_assert(offsetof(CbmRsStoreSchemaManifestEntryV2, kind) == 0,
               "StoreSchemaManifestEntryV2.kind offset drift");
_Static_assert(offsetof(CbmRsStoreSchemaManifestEntryV2, flags) == 4,
               "StoreSchemaManifestEntryV2.flags offset drift");
_Static_assert(offsetof(CbmRsStoreSchemaManifestEntryV2, ordinal) == 8,
               "StoreSchemaManifestEntryV2.ordinal offset drift");
_Static_assert(offsetof(CbmRsStoreSchemaManifestEntryV2, hidden) == 12,
               "StoreSchemaManifestEntryV2.hidden offset drift");
_Static_assert(offsetof(CbmRsStoreSchemaManifestEntryV2, name) == 16,
               "StoreSchemaManifestEntryV2.name offset drift");
_Static_assert(offsetof(CbmRsStoreSchemaManifestEntryV2, table_name) == 16 + sizeof(const char *),
               "StoreSchemaManifestEntryV2.table_name offset drift");
_Static_assert(offsetof(CbmRsStoreSchemaManifestEntryV2, column_name) ==
                   16 + 2 * sizeof(const char *),
               "StoreSchemaManifestEntryV2.column_name offset drift");
_Static_assert(offsetof(CbmRsStoreSchemaManifestEntryV2, column_type) ==
                   16 + 3 * sizeof(const char *),
               "StoreSchemaManifestEntryV2.column_type offset drift");
_Static_assert(offsetof(CbmRsStoreSchemaManifestEntryV2, default_sql) ==
                   16 + 4 * sizeof(const char *),
               "StoreSchemaManifestEntryV2.default_sql offset drift");
_Static_assert(offsetof(CbmRsStoreSchemaManifestEntryV2, columns_csv) ==
                   16 + 5 * sizeof(const char *),
               "StoreSchemaManifestEntryV2.columns_csv offset drift");
_Static_assert(offsetof(CbmRsStoreSchemaManifestEntryV2, sql_fragment) ==
                   16 + 6 * sizeof(const char *),
               "StoreSchemaManifestEntryV2.sql_fragment offset drift");
_Static_assert(sizeof(CbmRsStoreSchemaManifestEntryV2) == 16 + 7 * sizeof(const char *),
               "StoreSchemaManifestEntryV2 ABI size drift");
_Static_assert(offsetof(CbmRsStoreConnectionManifestEntryV1, kind) == 0,
               "StoreConnectionManifestEntryV1.kind offset drift");
_Static_assert(offsetof(CbmRsStoreConnectionManifestEntryV1, flags) == 4,
               "StoreConnectionManifestEntryV1.flags offset drift");
_Static_assert(offsetof(CbmRsStoreConnectionManifestEntryV1, mode_mask) == 8,
               "StoreConnectionManifestEntryV1.mode_mask offset drift");
_Static_assert(offsetof(CbmRsStoreConnectionManifestEntryV1, ordinal) == 12,
               "StoreConnectionManifestEntryV1.ordinal offset drift");
_Static_assert(offsetof(CbmRsStoreConnectionManifestEntryV1, value_i64) == 16,
               "StoreConnectionManifestEntryV1.value_i64 offset drift");
_Static_assert(offsetof(CbmRsStoreConnectionManifestEntryV1, name) == 24,
               "StoreConnectionManifestEntryV1.name offset drift");
_Static_assert(offsetof(CbmRsStoreConnectionManifestEntryV1, sql) == 24 + sizeof(const char *),
               "StoreConnectionManifestEntryV1.sql offset drift");
_Static_assert(offsetof(CbmRsStoreConnectionManifestEntryV1, env_name) ==
                   24 + 2 * sizeof(const char *),
               "StoreConnectionManifestEntryV1.env_name offset drift");
_Static_assert(sizeof(CbmRsStoreConnectionManifestEntryV1) == 24 + 3 * sizeof(const char *),
               "StoreConnectionManifestEntryV1 ABI size drift");

extern int cbm_rs_pipeline_incremental_post_step_count(int mode);
extern int cbm_rs_pipeline_incremental_post_steps(int mode, CbmRsPipelinePlanStep *out, int cap);
extern int cbm_rs_pipeline_plan_step_count_v2(int kind, int mode, int worker_count, int file_count);
extern int cbm_rs_pipeline_plan_steps_v2(int kind, int mode, int worker_count, int file_count,
                                         CbmRsPipelinePlanStepV2 *out, int cap);
extern int cbm_rs_pipeline_full_plan_step_count_v1(int mode, int worker_count, int file_count);
extern int cbm_rs_pipeline_full_plan_steps_v1(int mode, int worker_count, int file_count,
                                              CbmRsPipelineTopStepV1 *out, int cap);
extern size_t cbm_rs_pipeline_route_canon_path_v1(char *buf, size_t bufsize, const char *input);
extern int cbm_rs_pipeline_is_path_keyword_v1(const char *keyword);
extern int cbm_rs_pipeline_normalize_url_arg_v1(char *buf, size_t bufsize, const char *input);
extern int cbm_rs_pipeline_is_hash_segment_v1(const unsigned char *segment, size_t len);
extern int cbm_rs_pipeline_is_broker_route_v1(const char *qn);
#ifdef CBM_USE_RUST_PIPELINE_ROUTE_ARGS_ONLY
extern int cbm_pipeline_is_path_keyword(const char *keyword);
extern int cbm_pipeline_normalize_url_arg(const char *url, char *norm, int norm_sz);
#endif
extern int cbm_rs_pipeline_is_test_path_v1(const char *path);
extern int cbm_rs_pipeline_is_test_func_name_v1(const char *name);
extern size_t cbm_rs_pipeline_test_to_prod_path_v1(char *buf, size_t bufsize,
                                                   const char *test_path);
extern int cbm_rs_pipeline_is_checked_exception_v1(const char *name);
extern size_t cbm_rs_pipeline_artifact_path_v1(char *buf, size_t bufsize, const char *repo_path,
                                               const char *name);
extern bool cbm_pipeline_artifact_path(char *buf, size_t bufsz, const char *repo_path,
                                       const char *name);
extern int cbm_rs_cypher_lex_token_count_v1(const unsigned char *input, int len);
extern int cbm_rs_cypher_lex_tokens_v1(const unsigned char *input, int len, CbmRsCypherTokenV1 *out,
                                       int cap);
extern size_t cbm_rs_cypher_lex_summary_v1(char *buf, size_t bufsize, const unsigned char *input,
                                           int len);
extern size_t cbm_rs_cypher_parse_summary_v1(char *buf, size_t bufsize, const unsigned char *input,
                                             int len);
extern int cbm_rs_cypher_scalar_func_index_v1(const char *input);
extern int cbm_rs_cypher_multiarg_func_index_v1(const char *input);
extern int cbm_rs_cypher_aggregate_func_index_v1(int token_kind);
extern int cbm_rs_cypher_string_func_index_v1(int token_kind);
extern int cbm_rs_cypher_single_char_kind_v1(int input);
extern int cbm_rs_cypher_two_char_kind_v1(int first, int second);
extern size_t cbm_rs_mcp_jsonrpc_request_summary_v1(char *buf, size_t bufsize,
                                                    const unsigned char *input, int len);
extern size_t cbm_rs_mcp_initialize_protocol_version_v1(char *buf, size_t bufsize,
                                                        const unsigned char *input, int len);
extern size_t cbm_rs_mcp_tools_call_name_v1(char *buf, size_t bufsize, const unsigned char *input,
                                            int len);
extern size_t cbm_rs_mcp_tools_call_arguments_v1(char *buf, size_t bufsize,
                                                 const unsigned char *input, int len);
extern size_t cbm_rs_mcp_get_string_arg_v1(char *buf, size_t bufsize, const unsigned char *input,
                                           int len, const char *key);
extern int cbm_rs_mcp_get_int_arg_v1(const unsigned char *input, int len, const char *key,
                                     int default_value);
extern int cbm_rs_mcp_get_bool_arg_v1(const unsigned char *input, int len, const char *key);
extern int cbm_rs_mcp_edge_type_valid_v1(const char *input);
extern int cbm_rs_mcp_search_path_arg_valid_v1(const char *input);
extern int cbm_rs_mcp_search_args_valid_v1(const char *root_path, const char *file_pattern);
extern size_t cbm_rs_mcp_strip_root_prefix_offset_v1(const char *path, const char *root,
                                                     size_t root_len);
extern int cbm_rs_mcp_search_mode_v1(const char *input);
extern int cbm_rs_mcp_index_mode_v1(const char *input);
extern uint32_t cbm_rs_mcp_trace_mode_edge_mask_v1(const char *input);
extern void cbm_rs_mcp_sanitize_ascii_in_place_v1(char *input);
extern int cbm_rs_mcp_search_code_score_v1(const char *label, const char *file, int in_degree);
extern int cbm_rs_mcp_search_score_cmp_v1(int left_score, int right_score);
extern size_t cbm_rs_mcp_search_top_dir_v1(char *buf, size_t bufsize, const char *file);
extern int cbm_rs_mcp_detect_changes_wants_symbols_v1(const char *scope);
extern int cbm_rs_mcp_detect_changes_impacted_label_v1(const char *label);
extern size_t cbm_rs_mcp_detect_changes_status_path_offset_v1(const char *line);
extern int cbm_rs_mcp_search_line_match_span_v1(int start_line, int end_line, int line);
extern int cbm_rs_mcp_search_pick_resolved_index_v1(const long *scores, int count,
                                                    int *ambiguous_out);
extern int cbm_rs_mcp_search_pick_tightest_index_v1(const int *spans, int count);
extern int cbm_rs_mcp_utf8_is_cont_v1(int byte);
extern long cbm_rs_mcp_node_resolution_score_v1(const char *label, int start_line, int end_line);
extern int cbm_rs_mcp_adr_mode_v1(const char *input);
extern size_t cbm_rs_mcp_adr_sections_json_v1(char *buf, size_t bufsize, const char *content);
extern int cbm_rs_mcp_bm25_build_match_v1(char *buf, size_t bufsize, const char *input);
extern size_t cbm_rs_mcp_bm25_file_pattern_like_v1(char *buf, size_t bufsize, const char *input);
extern size_t cbm_rs_mcp_sanitize_utf8_lossy_v1(char *buf, size_t bufsize, const char *input);
extern int cbm_rs_mcp_architecture_aspect_wanted_v1(const char *input, const char *name);
extern int cbm_rs_mcp_trace_is_test_file_v1(const char *input);
extern int cbm_rs_mcp_project_db_file_name_v1(const char *input);
extern int cbm_rs_mcp_cancel_request_matches_v1(const char *params_json, int64_t active_id,
                                                const char *active_id_str);
extern size_t cbm_rs_mcp_jsonrpc_format_error_v1(char *buf, size_t bufsize, int64_t id, int code,
                                                 const char *message);
extern size_t cbm_rs_mcp_jsonrpc_format_response_v1(char *buf, size_t bufsize, int64_t id,
                                                    const char *id_str, const char *result_json,
                                                    const char *error_json);
extern size_t cbm_rs_mcp_text_result_v1(char *buf, size_t bufsize, const char *text, int is_error);
extern int cbm_rs_mcp_jsonrpc_parse_v1(const unsigned char *input, int len,
                                       CbmRsMcpJsonRpcParseOutV1 *out, char *jsonrpc_buf,
                                       size_t jsonrpc_bufsize, char *method_buf,
                                       size_t method_bufsize, char *id_str_buf,
                                       size_t id_str_bufsize, char *params_buf,
                                       size_t params_bufsize);
extern int cbm_rs_mcp_tools_cursor_offset_v1(const char *params_json, int tool_count);
extern int cbm_rs_mcp_tool_index_v1(const char *name);
extern int cbm_rs_mcp_tool_dispatch_index_v1(const char *name);
extern int cbm_rs_mcp_tool_count_v1(void);
extern size_t cbm_rs_mcp_tool_name_v1(char *buf, size_t bufsize, int index);
extern size_t cbm_rs_mcp_tool_title_v1(char *buf, size_t bufsize, int index);
extern size_t cbm_rs_mcp_tool_description_v1(char *buf, size_t bufsize, int index);
extern size_t cbm_rs_mcp_tool_input_schema_v1(char *buf, size_t bufsize, int index);
extern size_t cbm_rs_mcp_tool_output_schema_v1(char *buf, size_t bufsize);
extern size_t cbm_rs_mcp_tools_list_json_v1(char *buf, size_t bufsize, int offset, int limit,
                                            int include_next_cursor);
extern int cbm_rs_mcp_content_length_v1(const char *line, int max_len);
extern int cbm_rs_mcp_content_length_header_is_blank_v1(const char *line);
extern int cbm_rs_mcp_content_length_header_matches_v1(const char *line);
extern long long cbm_rs_mcp_content_length_raw_v1(const char *line);
extern size_t cbm_rs_mcp_content_length_response_v1(char *buf, size_t bufsize, const char *resp);
extern size_t cbm_rs_mcp_parse_file_uri_v1(char *buf, size_t bufsize, const char *uri);
extern int cbm_rs_mcp_method_kind_v1(const char *method);
extern size_t cbm_rs_mcp_method_not_found_error_v1(char *buf, size_t bufsize);
extern size_t cbm_rs_mcp_parse_error_message_v1(char *buf, size_t bufsize);
extern size_t cbm_rs_mcp_ping_result_v1(char *buf, size_t bufsize);
extern size_t cbm_rs_mcp_tools_call_default_arguments_v1(char *buf, size_t bufsize);
extern size_t cbm_rs_mcp_missing_tool_name_message_v1(char *buf, size_t bufsize);
extern size_t cbm_rs_mcp_missing_project_error_v1(char *buf, size_t bufsize);
extern size_t cbm_rs_mcp_project_not_found_message_v1(char *buf, size_t bufsize);
extern size_t cbm_rs_mcp_project_list_error_v1(char *buf, size_t bufsize, const char *reason,
                                               const char *projects_csv, int count);
extern size_t cbm_rs_mcp_unknown_tool_message_v1(char *buf, size_t bufsize, const char *tool_name);
extern size_t cbm_rs_mcp_initialize_response_v1(char *buf, size_t bufsize,
                                                const unsigned char *input, int len);
extern int cbm_rs_mcp_tools_page_bounds_v1(int offset, int limit, int include_next_cursor,
                                           int tool_count, CbmRsMcpToolsPageBoundsV1 *out);
extern int cbm_rs_gbuf_mutation_command_count_v1(void);
extern int cbm_rs_gbuf_mutation_commands_v1(CbmRsGbufMutationMetaV1 *out, int cap);
extern int cbm_rs_gbuf_mutation_validate_v1(const CbmRsGbufMutationCmdV1 *cmd,
                                            CbmRsGbufMutationValidationV1 *out);
extern int cbm_rs_store_schema_manifest_entry_count_v1(void);
extern int cbm_rs_store_schema_manifest_user_index_count_v1(void);
extern int cbm_rs_store_schema_manifest_entries_v1(CbmRsStoreSchemaManifestEntryV1 *out, int cap);
extern int cbm_rs_store_schema_manifest_entry_count_v2(void);
extern int cbm_rs_store_schema_manifest_table_column_count_v2(void);
extern int cbm_rs_store_schema_manifest_fts_shadow_count_v2(void);
extern int cbm_rs_store_schema_manifest_entries_v2(CbmRsStoreSchemaManifestEntryV2 *out, int cap);
extern size_t cbm_rs_store_build_immutable_uri_v1(char *buf, size_t bufsize, const char *path);
extern size_t cbm_rs_store_glob_to_like_v1(char *buf, size_t bufsize, const char *pattern);
extern size_t cbm_rs_store_ensure_case_insensitive_v1(char *buf, size_t bufsize,
                                                      const char *pattern);
extern size_t cbm_rs_store_strip_case_flag_v1(char *buf, size_t bufsize, const char *pattern);
extern int cbm_rs_store_like_hint_count_v1(const char *pattern, int max_out);
extern size_t cbm_rs_store_like_hint_v1(char *buf, size_t bufsize, const char *pattern, int max_out,
                                        int index);
extern size_t cbm_rs_store_qn_to_package_v1(char *buf, size_t bufsize, const char *qn);
extern size_t cbm_rs_store_qn_to_top_package_v1(char *buf, size_t bufsize, const char *qn);
extern size_t cbm_rs_store_normalize_arch_path_v1(char *norm_out, size_t norm_sz, const char *path);
extern size_t cbm_rs_store_file_ext_lower_v1(char *buf, size_t bufsize, const char *path);
extern int cbm_rs_store_ext_lang_kind_v1(const char *ext);
extern int cbm_rs_store_is_test_file_path_v1(const char *path);
extern int cbm_rs_store_hop_to_risk_v1(int hop);
extern size_t cbm_rs_store_risk_label_v1(char *buf, size_t bufsize, int level);
extern size_t cbm_rs_store_camel_split_v1(char *buf, size_t bufsize, const char *input);
extern int64_t cbm_rs_store_resolve_mmap_size_value_v1(const char *value);
extern int cbm_rs_store_connection_manifest_entry_count_v1(void);
extern int cbm_rs_store_connection_manifest_write_pragma_count_v1(void);
extern int cbm_rs_store_connection_manifest_entry_count_for_mode_v1(uint32_t mode_mask);
extern int cbm_rs_store_connection_manifest_write_entry_count_for_mode_v1(uint32_t mode_mask);
extern int cbm_rs_store_connection_manifest_entries_v1(CbmRsStoreConnectionManifestEntryV1 *out,
                                                       int cap);
extern int cbm_rs_store_connection_manifest_entries_for_mode_v1(
    uint32_t mode_mask, CbmRsStoreConnectionManifestEntryV1 *out, int cap);

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
extern int cbm_rs_registry_is_test_qn_v1(const char *qn);
extern bool cbm_pipeline_registry_is_test_qn(const char *qn);

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

static void check_i64(const char *name, int64_t actual, int64_t expected) {
    if (actual != expected) {
        fprintf(stderr, "%s: expected %" PRId64 ", got %" PRId64 "\n", name, expected, actual);
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

    int valid = -1;
    check_double_near("ratio_parse_null", cbm_rs_dump_verify_parse_min_ratio_v1(NULL, &valid),
                      CBM_DUMP_VERIFY_DEFAULT_RATIO);
    check_bool("ratio_parse_null_valid", valid, false);

    valid = -1;
    check_double_near("ratio_parse_zero", cbm_rs_dump_verify_parse_min_ratio_v1("0", &valid), 0.0);
    check_bool("ratio_parse_zero_valid", valid, true);

    valid = -1;
    check_double_near("ratio_parse_decimal", cbm_rs_dump_verify_parse_min_ratio_v1("0.25", &valid),
                      0.25);
    check_bool("ratio_parse_decimal_valid", valid, true);

    valid = -1;
    check_double_near("ratio_parse_prefix", cbm_rs_dump_verify_parse_min_ratio_v1("0.5x", &valid),
                      0.5);
    check_bool("ratio_parse_prefix_valid", valid, true);

    valid = -1;
    check_double_near("ratio_parse_hex", cbm_rs_dump_verify_parse_min_ratio_v1("0x1p-1", &valid),
                      0.5);
    check_bool("ratio_parse_hex_valid", valid, true);

    valid = -1;
    check_double_near("ratio_parse_inf", cbm_rs_dump_verify_parse_min_ratio_v1("inf", &valid),
                      CBM_DUMP_VERIFY_DEFAULT_RATIO);
    check_bool("ratio_parse_inf_valid", valid, false);

    valid = -1;
    check_double_near("ratio_parse_nan", cbm_rs_dump_verify_parse_min_ratio_v1("nan", &valid),
                      CBM_DUMP_VERIFY_DEFAULT_RATIO);
    check_bool("ratio_parse_nan_valid", valid, false);
}

static void test_traces_exports(void) {
    CbmRsTraceAttrV1 service_attrs[] = {
        {.key = "deployment.environment", .string_value = "prod"},
        {.key = "service.name", .string_value = "order-service"},
    };
    CbmRsTraceResourceV1 resource = {.attributes = service_attrs, .attr_count = 2};
    const char *service_name = cbm_rs_traces_extract_service_name_v1(&resource);
    check_str("traces_service_name", service_name, "order-service");
    check_str("traces_service_name_null", cbm_rs_traces_extract_service_name_v1(NULL), "");

    CbmRsTraceAttrV1 http_attrs[] = {
        {.key = "http.method", .string_value = "GET"},
        {.key = "url.full", .string_value = "https://example.com/api/orders?page=1"},
        {.key = "http.status_code", .string_value = "200"},
    };
    CbmRsTraceSpanV1 span = {
        .kind = 2,
        .attributes = http_attrs,
        .attr_count = 3,
        .start_time = "1000000000",
        .end_time = "1050000000",
    };
    CbmRsHttpSpanInfoV1 info = {0};
    check_bool("traces_http_info",
               cbm_rs_traces_extract_http_info_v1(&span, "svc", &info), true);
    check_str("traces_http_info_method", info.method, "GET");
    check_str("traces_http_info_path", info.path, "/api/orders");
    check_str("traces_http_info_status", info.status_code, "200");
    check_str("traces_http_info_service_name", info.service_name, "svc");
    check_int("traces_http_info_kind", info.span_kind, 2);
    check_i64("traces_http_info_duration", info.duration_ns, 50000000);
    check_bool("traces_http_info_non_http",
               cbm_rs_traces_extract_http_info_v1(NULL, "svc", &info), false);

    char buf[256];
    check_str("traces_path_from_url",
              cbm_rs_traces_extract_path_from_url_v1(
                  "http://localhost:8080/health?check=true", buf, sizeof(buf)),
              "/health");
    check_str("traces_path_from_url_invalid",
              cbm_rs_traces_extract_path_from_url_v1("not-a-url", buf, sizeof(buf)), "");
    check_str("traces_path_from_url_null",
              cbm_rs_traces_extract_path_from_url_v1(NULL, buf, sizeof(buf)), "");
    check_i64("traces_parse_duration",
              cbm_rs_traces_parse_duration_v1("1000000000", "1050000000"), 50000000);
    check_i64("traces_parse_duration_zero", cbm_rs_traces_parse_duration_v1("1000", "500"), 0);
    check_i64("traces_parse_duration_null",
              cbm_rs_traces_parse_duration_v1(NULL, "1000"), 0);

    int64_t values[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
    check_i64("traces_calculate_p99", cbm_rs_traces_calculate_p99_v1(values, 10), 100);
    int64_t single[] = {42};
    check_i64("traces_calculate_p99_single", cbm_rs_traces_calculate_p99_v1(single, 1), 42);
    check_i64("traces_calculate_p99_empty", cbm_rs_traces_calculate_p99_v1(NULL, 0), 0);
}

static void test_arena_exports(void) {
    enum {
        ARENA_DEFAULT_BLOCK_SIZE = 64 * 1024,
        ARENA_MIN_BLOCK_SIZE = 64,
    };

    check_null("arena_alloc_null", cbm_rs_arena_alloc(NULL, 16));
    check_null("arena_calloc_null", cbm_rs_arena_calloc(NULL, 16));
    check_null("arena_strdup_null_arena", cbm_rs_arena_strdup(NULL, "hello"));
    check_null("arena_strndup_null_arena",
               cbm_rs_arena_strndup(NULL, (const unsigned char *)"hello", 5));
    check_size("arena_total_null", cbm_rs_arena_total(NULL), 0);
    cbm_rs_arena_reset(NULL);
    cbm_rs_arena_destroy(NULL);

    CBMArena arena;
    cbm_rs_arena_init(&arena);
    check_size("arena_default_count", (size_t)arena.nblocks, 1);
    check_size("arena_default_block_size", arena.block_size, ARENA_DEFAULT_BLOCK_SIZE);
    check_size("arena_default_first_size", arena.block_sizes[0], ARENA_DEFAULT_BLOCK_SIZE);
    check_size("arena_default_used", arena.used, 0);
    check_size("arena_default_total", arena.total_alloc, 0);
    check_null("arena_alloc_zero", cbm_rs_arena_alloc(&arena, 0));
    cbm_rs_arena_destroy(&arena);
    check_size("arena_destroy_count", (size_t)arena.nblocks, 0);
    check_size("arena_destroy_total", arena.total_alloc, 0);
    cbm_rs_arena_destroy(&arena);
    check_size("arena_double_destroy_count", (size_t)arena.nblocks, 0);

    cbm_rs_arena_init_sized(&arena, 32);
    check_size("arena_sized_clamp", arena.block_size, ARENA_MIN_BLOCK_SIZE);
    check_size("arena_sized_first_size", arena.block_sizes[0], ARENA_MIN_BLOCK_SIZE);
    void *p1 = cbm_rs_arena_alloc(&arena, 1);
    void *p2 = cbm_rs_arena_alloc(&arena, 1);
    check_not_null("arena_alloc_p1", p1);
    check_not_null("arena_alloc_p2", p2);
    check_size("arena_alloc_p1_align", (uintptr_t)p1 % 8, 0);
    check_size("arena_alloc_p2_align", (uintptr_t)p2 % 8, 0);
    check_size("arena_alloc_delta", (size_t)((char *)p2 - (char *)p1), 8);
    check_size("arena_used_after_two_allocs", arena.used, 16);
    check_size("arena_total_after_two_allocs", cbm_rs_arena_total(&arena), 16);

    unsigned char *zero = (unsigned char *)cbm_rs_arena_calloc(&arena, 16);
    check_not_null("arena_calloc", zero);
    for (int i = 0; i < 16; i++) {
        if (zero[i] != 0) {
            fprintf(stderr, "arena_calloc_zero failed at %d\n", i);
            exit(1);
        }
    }

    char *hello = cbm_rs_arena_strdup(&arena, "hello world");
    check_not_null("arena_strdup", hello);
    check_str("arena_strdup_value", hello, "hello world");
    check_null("arena_strdup_null_value", cbm_rs_arena_strdup(&arena, NULL));

    char *prefix =
        cbm_rs_arena_strndup(&arena, (const unsigned char *)"hello world", strlen("hello"));
    check_not_null("arena_strndup", prefix);
    check_str("arena_strndup_value", prefix, "hello");
    const unsigned char raw[] = {'a', '\0', 'b'};
    char *raw_copy = cbm_rs_arena_strndup(&arena, raw, sizeof(raw));
    check_not_null("arena_strndup_raw", raw_copy);
    check_int("arena_strndup_raw_0", raw_copy[0], 'a');
    check_int("arena_strndup_raw_1", raw_copy[1], '\0');
    check_int("arena_strndup_raw_2", raw_copy[2], 'b');
    check_int("arena_strndup_raw_3", raw_copy[3], '\0');

    cbm_rs_arena_destroy(&arena);

    cbm_rs_arena_init_sized(&arena, 64);
    check_not_null("arena_growth_1", cbm_rs_arena_alloc(&arena, 48));
    check_size("arena_growth_count_1", (size_t)arena.nblocks, 1);
    check_not_null("arena_growth_2", cbm_rs_arena_alloc(&arena, 48));
    check_size("arena_growth_count_2", (size_t)arena.nblocks, 2);
    check_size("arena_growth_block_size", arena.block_size, 128);
    cbm_rs_arena_reset(&arena);
    check_size("arena_reset_count", (size_t)arena.nblocks, 1);
    check_size("arena_reset_block_size", arena.block_size, arena.block_sizes[0]);
    check_size("arena_reset_first_size", arena.block_sizes[0], 64);
    check_size("arena_reset_used", arena.used, 0);
    check_size("arena_reset_total", arena.total_alloc, 0);
    check_not_null("arena_reuse_after_reset", cbm_rs_arena_alloc(&arena, 16));
    cbm_rs_arena_destroy(&arena);

    memset(&arena, 0, sizeof(arena));
    check_null("arena_corrupt_zero_nblocks", cbm_rs_arena_alloc(&arena, 16));
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

static void test_discover_string_helpers_exports(void) {
    const char *const names[] = {"src", "docs", NULL};
    const char *const empty[] = {NULL};
    const char *const stopped[] = {"src", NULL, "docs"};
    check_bool("discover_str_in_list", cbm_rs_discover_str_in_list_v1("docs", names), true);
    check_bool("discover_str_in_list_missing", cbm_rs_discover_str_in_list_v1("tests", names),
               false);
    check_bool("discover_str_in_list_null_value", cbm_rs_discover_str_in_list_v1(NULL, names),
               false);
    check_bool("discover_str_in_list_null_list", cbm_rs_discover_str_in_list_v1("docs", NULL),
               false);
    check_bool("discover_str_in_list_empty", cbm_rs_discover_str_in_list_v1("docs", empty), false);
    check_bool("discover_str_in_list_stopped", cbm_rs_discover_str_in_list_v1("docs", stopped),
               false);
    check_bool("discover_ends_with", cbm_rs_discover_ends_with_v1("module.pyc", ".pyc"), true);
    check_bool("discover_ends_with_empty", cbm_rs_discover_ends_with_v1("module.pyc", ""), true);
    check_bool("discover_ends_with_too_long", cbm_rs_discover_ends_with_v1("module.pyc", ".sqlite"),
               false);
    check_bool("discover_ends_with_null", cbm_rs_discover_ends_with_v1(NULL, ".pyc"), false);
    check_bool("discover_str_contains", cbm_rs_discover_str_contains_v1("service.pb.go", ".pb."),
               true);
    check_bool("discover_str_contains_missing",
               cbm_rs_discover_str_contains_v1("service.pb.go", "mock"), false);
    check_bool("discover_str_contains_empty", cbm_rs_discover_str_contains_v1("service.pb.go", ""),
               true);
    check_bool("discover_str_contains_null", cbm_rs_discover_str_contains_v1(NULL, ".pb."), false);
    check_bool("discover_ascii_ieq", cbm_rs_discover_ascii_ieq_v1("ExcludesFile", "excludesfile"),
               true);
    check_bool("discover_ascii_ieq_non_ascii", cbm_rs_discover_ascii_ieq_v1("\xC0", "\xE0"), false);
    check_bool("discover_ascii_ieq_null", cbm_rs_discover_ascii_ieq_v1(NULL, "core"), false);

#ifdef CBM_USE_RUST_DISCOVER_STRING_HELPERS_ONLY
    check_bool("discover_direct_str_in_list", cbm_discover_str_in_list("docs", names), true);
    check_bool("discover_direct_str_in_list_null", cbm_discover_str_in_list(NULL, names), false);
    check_bool("discover_direct_str_in_list_stopped", cbm_discover_str_in_list("docs", stopped),
               false);
    check_bool("discover_direct_ends_with", cbm_discover_ends_with("module.pyc", ".pyc"), true);
    check_bool("discover_direct_ends_with_null", cbm_discover_ends_with(NULL, ".pyc"), false);
    check_bool("discover_direct_str_contains", cbm_discover_str_contains("service.pb.go", ".pb."),
               true);
    check_bool("discover_direct_str_contains_null", cbm_discover_str_contains(NULL, ".pb."), false);
    check_bool("discover_direct_ascii_ieq", cbm_discover_ascii_ieq("ExcludesFile", "excludesfile"),
               true);
    check_bool("discover_direct_ascii_ieq_non_ascii", cbm_discover_ascii_ieq("\xC0", "\xE0"),
               false);
#endif
}

static void test_discover_trailing_sep_export(void) {
    check_bool("discover_trailing_sep_slash",
               cbm_rs_discover_has_trailing_sep_v1("src/"), true);
    check_bool("discover_trailing_sep_backslash",
               cbm_rs_discover_has_trailing_sep_v1("src\\"), true);
    check_bool("discover_trailing_sep_plain",
               cbm_rs_discover_has_trailing_sep_v1("src"), false);
    check_bool("discover_trailing_sep_empty",
               cbm_rs_discover_has_trailing_sep_v1(""), false);
    check_bool("discover_trailing_sep_null",
               cbm_rs_discover_has_trailing_sep_v1(NULL), false);

#ifdef CBM_USE_RUST_DISCOVER_TRAILING_SEP_ONLY
    check_bool("discover_trailing_sep_direct_slash",
               cbm_discover_has_trailing_sep("direct/"), true);
    check_bool("discover_trailing_sep_direct_backslash",
               cbm_discover_has_trailing_sep("direct\\"), true);
    check_bool("discover_trailing_sep_direct_plain",
               cbm_discover_has_trailing_sep("direct"), false);
    check_bool("discover_trailing_sep_direct_empty",
               cbm_discover_has_trailing_sep(""), false);
    check_bool("discover_trailing_sep_direct_null",
               cbm_discover_has_trailing_sep(NULL), false);
#endif
}

static void test_discover_path_join_export(void) {
    char out[64];
    cbm_rs_discover_path_join_v1("/tmp", "file", out, sizeof(out));
    check_str("discover_path_join_basic", out, "/tmp/file");
    cbm_rs_discover_path_join_v1("/tmp/", "file", out, sizeof(out));
    check_str("discover_path_join_trailing", out, "/tmp/file");
    cbm_rs_discover_path_join_v1("C:\\tmp", "file", out, sizeof(out));
    check_str("discover_path_join_normalized", out, "C:/tmp/file");
    cbm_rs_discover_path_join_v1("/tmp", "filename", out, 5);
    check_str("discover_path_join_truncated", out, "/tmp");
    cbm_rs_discover_path_join_v1(NULL, NULL, out, sizeof(out));
    check_str("discover_path_join_empty", out, "");
    cbm_rs_discover_path_join_v1(NULL, "file", out, sizeof(out));
    check_str("discover_path_join_null_base", out, "file");
    out[0] = 'X';
    out[1] = 'Y';
    cbm_rs_discover_path_join_v1("/tmp", "file", out, 0);
    check_int("discover_path_join_zero_size0", (unsigned char)out[0], 'X');
    check_int("discover_path_join_zero_size1", (unsigned char)out[1], 'Y');

#ifdef CBM_USE_RUST_DISCOVER_PATH_JOIN_ONLY
    cbm_discover_path_join("/direct", "file", out, sizeof(out));
    check_str("discover_path_join_direct", out, "/direct/file");
    cbm_discover_path_join("/direct/", "file", out, sizeof(out));
    check_str("discover_path_join_direct_trailing", out, "/direct/file");
    cbm_discover_path_join(NULL, "file", out, sizeof(out));
    check_str("discover_path_join_direct_null_base", out, "file");
#endif
}

static void test_discover_local_rel_path_export(void) {
    check_size("discover_local_rel_path_v1_strip",
               cbm_rs_discover_local_rel_path_offset_v1("local/src/main.c", "local"),
               6);
    check_size("discover_local_rel_path_v1_boundary",
               cbm_rs_discover_local_rel_path_offset_v1("locality/src/main.c", "local"),
               0);
    check_size("discover_local_rel_path_v1_null",
               cbm_rs_discover_local_rel_path_offset_v1(NULL, "local"),
               0);

    check_size("discover_local_rel_path_bridge_strip",
               cbm_discover_local_rel_path_offset("local/src/main.c", "local"),
               6);
    check_size("discover_local_rel_path_bridge_boundary",
               cbm_discover_local_rel_path_offset("locality/src/main.c", "local"),
               0);
    check_size("discover_local_rel_path_bridge_null",
               cbm_discover_local_rel_path_offset(NULL, "local"),
               0);

#ifdef CBM_USE_RUST_DISCOVER_LOCAL_REL_PATH_ONLY
    check_size("discover_local_rel_path_direct_strip",
               cbm_discover_local_rel_path_offset("local/src/main.c", "local"),
               6);
    check_size("discover_local_rel_path_direct_boundary",
               cbm_discover_local_rel_path_offset("locality/src/main.c", "local"),
               0);
    check_size("discover_local_rel_path_direct_null",
               cbm_discover_local_rel_path_offset(NULL, "local"),
               0);
#endif
}

static void test_watcher_poll_interval_export(void) {
    check_int("watcher_poll_interval_base", cbm_rs_watcher_poll_interval_ms_v1(0), 5000);
    check_int("watcher_poll_interval_step", cbm_rs_watcher_poll_interval_ms_v1(500), 6000);
    check_int("watcher_poll_interval_cap", cbm_rs_watcher_poll_interval_ms_v1(100000), 60000);
    check_int("watcher_poll_interval_negative", cbm_rs_watcher_poll_interval_ms_v1(-1), 5000);

#ifdef CBM_USE_RUST_WATCHER_POLL_INTERVAL_ONLY
    check_int("watcher_poll_interval_direct", cbm_watcher_poll_interval_ms(5000), 15000);
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

static void test_mem_exports(void) {
    EnvSnapshot snapshots[1];
    env_snapshot_save(&snapshots[0], "CBM_MEM_BUDGET_MB");

    env_set_checked("CBM_MEM_BUDGET_MB", "1536");
    cbm_rs_mem_init(0.5);

    check_size("mem_budget_override", cbm_rs_mem_budget(), (size_t)1536 * 1024 * 1024);
    check_size("mem_worker_budget_zero", cbm_rs_mem_worker_budget(0), cbm_rs_mem_budget());
    check_size("mem_worker_budget_neg", cbm_rs_mem_worker_budget(-3), cbm_rs_mem_budget());
    check_size("mem_worker_budget_div4", cbm_rs_mem_worker_budget(4), cbm_rs_mem_budget() / 4);
    check_bool("mem_over_budget_contract", cbm_rs_mem_over_budget(),
               cbm_rs_mem_rss() > cbm_rs_mem_budget());
    check_bool("mem_collect_no_crash", (cbm_rs_mem_collect(), true), true);
    check_bool("mem_peak_non_zero", cbm_rs_mem_peak_rss() >= cbm_rs_mem_rss(), true);

    env_snapshot_restore(snapshots, 1);
    cbm_rs_mem_collect();
}

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

static void test_compat_exports(void) {
    enum {
        REG_EXTENDED = 1,
        REG_ICASE = 2,
        REG_NOSUB = 4,
        REG_NEWLINE = 8,
        REG_KNOWN_MASK = 15,
        REG_OK = 0,
        REG_NOMATCH = -1,
        REG_MATCH_CAP = 32,
    };

    check_int("compat_regex_known_flags",
              cbm_rs_compat_regex_known_flags(REG_EXTENDED | REG_ICASE | 0x80),
              REG_EXTENDED | REG_ICASE);
    check_int("compat_regex_match_cap_negative", cbm_rs_compat_regex_match_cap(-1, 1), 0);
    check_int("compat_regex_match_cap_no_matches", cbm_rs_compat_regex_match_cap(4, 0), 0);
    check_int("compat_regex_match_cap_small", cbm_rs_compat_regex_match_cap(4, 1), 4);
    check_int("compat_regex_match_cap_large", cbm_rs_compat_regex_match_cap(40, 1), REG_MATCH_CAP);
    check_int("compat_regex_status_match", cbm_rs_compat_regex_status(1), REG_OK);
    check_int("compat_regex_status_nomatch", cbm_rs_compat_regex_status(0), REG_NOMATCH);
    check_int("compat_regex_all_flags", cbm_rs_compat_regex_known_flags(0xff), REG_KNOWN_MASK);

    check_size("compat_thread_default_stack", cbm_rs_compat_thread_effective_stack_size(0),
               (size_t)8 * 1024 * 1024);
    check_size("compat_thread_explicit_stack", cbm_rs_compat_thread_effective_stack_size(65536),
               65536);
    check_bool("compat_aligned_precheck_ok",
               cbm_rs_compat_aligned_alloc_precheck(16, 128, sizeof(void *)), true);
    check_bool("compat_aligned_precheck_small_alignment",
               cbm_rs_compat_aligned_alloc_precheck(4, 128, sizeof(void *)), false);
    check_bool("compat_aligned_precheck_non_power_two",
               cbm_rs_compat_aligned_alloc_precheck(24, 128, sizeof(void *)), false);
    check_bool("compat_aligned_precheck_zero_size",
               cbm_rs_compat_aligned_alloc_precheck(16, 0, sizeof(void *)), false);
}

static void test_log_profile_exports(void) {
    enum {
        LOG_DEBUG = 0,
        LOG_INFO = 1,
        LOG_WARN = 2,
        LOG_ERROR = 3,
        LOG_NONE = 4,
        LOG_FORMAT_TEXT = 0,
        LOG_FORMAT_JSON = 1,
    };
    char buf[128];
    char small[4];

    check_int("log_level_warn_text", cbm_rs_log_parse_level_value("WARN", LOG_INFO), LOG_WARN);
    check_int("log_level_numeric", cbm_rs_log_parse_level_value("3", LOG_INFO), LOG_ERROR);
    check_int("log_level_leading_space_plus", cbm_rs_log_parse_level_value(" +3", LOG_INFO),
              LOG_ERROR);
    check_int("log_level_trailing_space_keeps_current",
              cbm_rs_log_parse_level_value("3 ", LOG_WARN), LOG_WARN);
    check_int("log_level_none", cbm_rs_log_parse_level_value("none", LOG_INFO), LOG_NONE);
    check_int("log_level_invalid_keeps_current", cbm_rs_log_parse_level_value("verbose", LOG_WARN),
              LOG_WARN);
    check_int("log_level_empty_keeps_current", cbm_rs_log_parse_level_value("", LOG_WARN),
              LOG_WARN);
    check_int("log_level_null_keeps_current", cbm_rs_log_parse_level_value(NULL, LOG_WARN),
              LOG_WARN);

    check_int("log_format_json", cbm_rs_log_parse_format_value("json", LOG_FORMAT_TEXT),
              LOG_FORMAT_JSON);
    check_int("log_format_text", cbm_rs_log_parse_format_value("TEXT", LOG_FORMAT_JSON),
              LOG_FORMAT_TEXT);
    check_int("log_format_invalid_keeps_current",
              cbm_rs_log_parse_format_value("structured", LOG_FORMAT_JSON), LOG_FORMAT_JSON);
    check_int("log_format_leading_space_keeps_current",
              cbm_rs_log_parse_format_value(" json", LOG_FORMAT_JSON), LOG_FORMAT_JSON);

    check_int("log_text_atom_len",
              cbm_rs_log_sanitize_text_atom(buf, sizeof(buf), "line\r\nbreak\tvalue"), 17);
    check_str("log_text_atom", buf, "line__break_value");
    check_int("log_text_atom_small", cbm_rs_log_sanitize_text_atom(small, sizeof(small), "abcdef"),
              -1);
    check_int("log_text_atom_null_buf", cbm_rs_log_sanitize_text_atom(NULL, sizeof(buf), "abcdef"),
              -1);

    check_int("log_json_string_len",
              cbm_rs_log_json_string(buf, sizeof(buf), "line\n\"quote\"\\tail"), 23);
    check_str("log_json_string", buf, "\"line\\n\\\"quote\\\"\\\\tail\"");
    check_int("log_json_string_small", cbm_rs_log_json_string(small, sizeof(small), "abcdef"), -1);

    check_int("log_http_path_len",
              cbm_rs_log_http_path_without_query(buf, sizeof(buf), "/api/layout?token=x#frag"), 11);
    check_str("log_http_path", buf, "/api/layout");
    check_int("log_http_path_fragment",
              cbm_rs_log_http_path_without_query(buf, sizeof(buf), "/api/layout#frag"), 11);
    check_str("log_http_path_fragment_value", buf, "/api/layout");
    check_int("log_http_level_info", cbm_rs_log_http_status_level(200), LOG_INFO);
    check_int("log_http_level_warn", cbm_rs_log_http_status_level(404), LOG_WARN);
    check_int("log_http_level_error", cbm_rs_log_http_status_level(500), LOG_ERROR);

    check_bool("profile_env_null", cbm_rs_profile_env_enabled(NULL), false);
    check_bool("profile_env_empty", cbm_rs_profile_env_enabled(""), false);
    check_bool("profile_env_zero", cbm_rs_profile_env_enabled("0"), false);
    check_bool("profile_env_double_zero", cbm_rs_profile_env_enabled("00"), false);
    check_bool("profile_env_one", cbm_rs_profile_env_enabled("1"), true);
    check_bool("profile_env_false_string", cbm_rs_profile_env_enabled("false"), true);
    check_i64("profile_elapsed_us", cbm_rs_profile_elapsed_us(10, 900000000, 12, 100250000),
              1200250);
    check_i64("profile_elapsed_ms", cbm_rs_profile_elapsed_ms(1200250), 1200);
    check_i64("profile_rate", cbm_rs_profile_rate_per_s(12, 3000), 4000);
    check_i64("profile_rate_zero_us", cbm_rs_profile_rate_per_s(12, 0), -1);
    check_i64("profile_rate_zero_items", cbm_rs_profile_rate_per_s(0, 3000), -1);
}

static void test_pipeline_project_name_exports(void) {
    char out[128];

    check_size("project_name_null_len",
               cbm_rs_pipeline_project_name_from_path(out, sizeof(out), NULL), strlen("root"));
    check_str("project_name_null_out", out, "root");
    check_size("project_name_empty_len",
               cbm_rs_pipeline_project_name_from_path(out, sizeof(out), ""), strlen("root"));
    check_str("project_name_empty_out", out, "root");
    check_size("project_name_slashes_len",
               cbm_rs_pipeline_project_name_from_path(out, sizeof(out), "///"), strlen("root"));
    check_str("project_name_slashes_out", out, "root");

    check_size("project_name_unix_len",
               cbm_rs_pipeline_project_name_from_path(out, sizeof(out), "/Users/dev/my-project"),
               strlen("Users-dev-my-project"));
    check_str("project_name_unix_out", out, "Users-dev-my-project");
    check_size("project_name_windows_len",
               cbm_rs_pipeline_project_name_from_path(out, sizeof(out), "C:\\Users\\dev\\project"),
               strlen("C-Users-dev-project"));
    check_str("project_name_windows_out", out, "C-Users-dev-project");
    check_size("project_name_drive_canon_len",
               cbm_rs_pipeline_project_name_from_path(out, sizeof(out), "c:/WEBDEV/Cardio-Cloud"),
               strlen("C-WEBDEV-Cardio-Cloud"));
    check_str("project_name_drive_canon_out", out, "C-WEBDEV-Cardio-Cloud");
    check_size("project_name_unsafe_len",
               cbm_rs_pipeline_project_name_from_path(out, sizeof(out), "/x/a..b/my project"),
               strlen("x-a.b-my-project"));
    check_str("project_name_unsafe_out", out, "x-a.b-my-project");
    check_size("project_name_unicode_len",
               cbm_rs_pipeline_project_name_from_path(out, sizeof(out), "/tmp/\xe5\xbc\x80"),
               strlen("tmp-e5bc80"));
    check_str("project_name_unicode_out", out, "tmp-e5bc80");

    char small[6];
    memset(small, 'Z', sizeof(small));
    check_size(
        "project_name_trunc_len",
        cbm_rs_pipeline_project_name_from_path(small, sizeof(small), "/Users/dev/my-project"),
        strlen("Users-dev-my-project"));
    check_str("project_name_trunc_out", small, "Users");
    check_size("project_name_len_only",
               cbm_rs_pipeline_project_name_from_path(NULL, 0, "/Users/dev/my-project"),
               strlen("Users-dev-my-project"));
}

static void test_pipeline_configures_exports(void) {
    char out[128];
    char small[8];

    check_int("configures_env_upper", cbm_rs_pipeline_is_env_var_name_v1("DATABASE_URL"), 1);
    check_int("configures_env_digit", cbm_rs_pipeline_is_env_var_name_v1("DB_2"), 1);
    check_int("configures_env_lower", cbm_rs_pipeline_is_env_var_name_v1("apiKey"), 0);
    check_int("configures_env_short", cbm_rs_pipeline_is_env_var_name_v1("A"), 0);
    check_int("configures_env_null", cbm_rs_pipeline_is_env_var_name_v1(NULL), 0);

    check_int("configures_normalize_max_connections",
              cbm_rs_pipeline_normalize_config_key_v1("max_connections", out, sizeof(out)), 2);
    check_str("configures_normalize_max_connections_out", out, "max_connections");
    check_int("configures_normalize_camel",
              cbm_rs_pipeline_normalize_config_key_v1("maxRetryCount", out, sizeof(out)), 3);
    check_str("configures_normalize_camel_out", out, "max_retry_count");
    check_int("configures_normalize_dot",
              cbm_rs_pipeline_normalize_config_key_v1("database.host", out, sizeof(out)), 2);
    check_str("configures_normalize_dot_out", out, "database_host");
    memset(small, 'Z', sizeof(small));
    check_int("configures_normalize_trunc",
              cbm_rs_pipeline_normalize_config_key_v1("databaseHost", small, sizeof(small)), 2);
    check_str("configures_normalize_trunc_out", small, "databas");
    memset(out, 'Z', sizeof(out));
    check_int("configures_normalize_null",
              cbm_rs_pipeline_normalize_config_key_v1(NULL, out, sizeof(out)), 0);
    check_int("configures_normalize_null_unchanged", out[0], 'Z');

    check_int("configures_ext_toml", cbm_rs_pipeline_has_config_extension_v1("config.toml"), 1);
    check_int("configures_ext_env", cbm_rs_pipeline_has_config_extension_v1(".env"), 1);
    check_int("configures_ext_go", cbm_rs_pipeline_has_config_extension_v1("main.go"), 0);
    check_int("configures_ext_null", cbm_rs_pipeline_has_config_extension_v1(NULL), 0);
}

#ifdef CBM_USE_RUST_PIPELINE_CONFIGURES_ONLY
static void test_pipeline_configures_direct_abi_exports(void) {
    char out[128];
    char small[8];
    char long_key[601];
    char long_out[512];

    check_int("configures_direct_env_upper", cbm_is_env_var_name("DATABASE_URL"), 1);
    check_int("configures_direct_env_null", cbm_is_env_var_name(NULL), 0);
    check_int("configures_direct_ext_toml", cbm_has_config_extension("config.toml"), 1);
    check_int("configures_direct_ext_null", cbm_has_config_extension(NULL), 0);

    check_int("configures_direct_normalize_camel",
              cbm_normalize_config_key("maxRetryCount", out, sizeof(out)), 3);
    check_str("configures_direct_normalize_camel_out", out, "max_retry_count");

    memset(small, 'Z', sizeof(small));
    check_int("configures_direct_normalize_short",
              cbm_normalize_config_key("databaseHost", small, sizeof(small)), 2);
    check_str("configures_direct_normalize_short_out", small, "databas");
    check_int("configures_direct_normalize_short_nul", small[sizeof(small) - 1], 0);

    memset(out, 'Z', sizeof(out));
    check_int("configures_direct_normalize_null_key",
              cbm_normalize_config_key(NULL, out, sizeof(out)), 0);
    check_int("configures_direct_normalize_null_key_unchanged", out[0], 'Z');
    check_int("configures_direct_normalize_null_out",
              cbm_normalize_config_key("maxRetryCount", NULL, sizeof(out)), 0);
    memset(out, 'Z', sizeof(out));
    check_int("configures_direct_normalize_zero_size",
              cbm_normalize_config_key("maxRetryCount", out, 0), 0);
    check_int("configures_direct_normalize_zero_size_unchanged", out[0], 'Z');

    memset(long_key, 'a', sizeof(long_key) - 1);
    long_key[sizeof(long_key) - 1] = '\0';
    check_int("configures_direct_normalize_key_trunc",
              cbm_normalize_config_key(long_key, long_out, sizeof(long_out)), 1);
    check_size("configures_direct_normalize_key_trunc_len", strlen(long_out), 511);
    check_int("configures_direct_normalize_key_trunc_nul", long_out[sizeof(long_out) - 1], 0);
}
#endif

static void test_pipeline_infrascan_exports(void) {
    check_int("infrascan_dockerfile_name",
              cbm_rs_pipeline_is_dockerfile_v1("Dockerfile"), 1);
    check_int("infrascan_dockerfile_lower",
              cbm_rs_pipeline_is_dockerfile_v1("dockerfile"), 1);
    check_int("infrascan_dockerfile_prod",
              cbm_rs_pipeline_is_dockerfile_v1("Dockerfile.prod"), 1);
    check_int("infrascan_dockerfile_suffix",
              cbm_rs_pipeline_is_dockerfile_v1("app.dockerfile"), 1);
    check_int("infrascan_dockerfile_bare_suffix",
              cbm_rs_pipeline_is_dockerfile_v1(".dockerfile"), 0);
    check_int("infrascan_dockerfile_false",
              cbm_rs_pipeline_is_dockerfile_v1("docker-compose.yml"), 0);
    check_int("infrascan_dockerfile_null", cbm_rs_pipeline_is_dockerfile_v1(NULL), 0);

    check_int("infrascan_helm_chart_yaml", cbm_rs_pipeline_is_helm_chart_file_v1("Chart.yaml"), 1);
    check_int("infrascan_helm_chart_yml", cbm_rs_pipeline_is_helm_chart_file_v1("Chart.yml"), 1);
    check_int("infrascan_helm_chart_lower", cbm_rs_pipeline_is_helm_chart_file_v1("chart.yaml"), 0);
    check_int("infrascan_gomod", cbm_rs_pipeline_is_gomod_file_v1("go.mod"), 1);
    check_int("infrascan_gomod_sum", cbm_rs_pipeline_is_gomod_file_v1("go.sum"), 0);
    check_int("infrascan_requirements", cbm_rs_pipeline_is_requirements_file_v1("requirements.txt"), 1);
    check_int("infrascan_requirements_null", cbm_rs_pipeline_is_requirements_file_v1(NULL), 0);

    check_int("infrascan_compose_yml", cbm_rs_pipeline_is_compose_file_v1("docker-compose.yml"),
              1);
    check_int("infrascan_compose_yaml", cbm_rs_pipeline_is_compose_file_v1("docker-compose.yaml"),
              1);
    check_int("infrascan_compose_prod",
              cbm_rs_pipeline_is_compose_file_v1("docker-compose.prod.yml"), 1);
    check_int("infrascan_compose_short", cbm_rs_pipeline_is_compose_file_v1("compose.yml"), 1);
    check_int("infrascan_compose_case", cbm_rs_pipeline_is_compose_file_v1("compose.YAML"), 1);
    check_int("infrascan_compose_false", cbm_rs_pipeline_is_compose_file_v1("mycompose.yml"), 0);
    check_int("infrascan_compose_txt", cbm_rs_pipeline_is_compose_file_v1("docker-compose.txt"),
              0);
    check_int("infrascan_compose_null", cbm_rs_pipeline_is_compose_file_v1(NULL), 0);

    check_int("infrascan_env_exact", cbm_rs_pipeline_is_env_file_v1(".env"), 1);
    check_int("infrascan_env_prefix", cbm_rs_pipeline_is_env_file_v1(".env.production"), 1);
    check_int("infrascan_env_suffix", cbm_rs_pipeline_is_env_file_v1("foo.env"), 1);
    check_int("infrascan_env_case", cbm_rs_pipeline_is_env_file_v1("FOO.ENV"), 1);
    check_int("infrascan_env_false", cbm_rs_pipeline_is_env_file_v1("env"), 0);
    check_int("infrascan_env_bak", cbm_rs_pipeline_is_env_file_v1("foo.env.bak"), 0);
    check_int("infrascan_env_null", cbm_rs_pipeline_is_env_file_v1(NULL), 0);

    check_int("infrascan_cloudbuild_yaml", cbm_rs_pipeline_is_cloudbuild_file_v1("cloudbuild.yaml"),
              1);
    check_int("infrascan_cloudbuild_yml", cbm_rs_pipeline_is_cloudbuild_file_v1("cloudbuild.yml"),
              1);
    check_int("infrascan_cloudbuild_prod",
              cbm_rs_pipeline_is_cloudbuild_file_v1("cloudbuild-prod.yaml"), 1);
    check_int("infrascan_cloudbuild_case",
              cbm_rs_pipeline_is_cloudbuild_file_v1("Cloudbuild.yml"), 1);
    check_int("infrascan_cloudbuild_prefix_false",
              cbm_rs_pipeline_is_cloudbuild_file_v1("mycloudbuild.yaml"), 0);
    check_int("infrascan_cloudbuild_no_ext", cbm_rs_pipeline_is_cloudbuild_file_v1("cloudbuild"),
              0);
    check_int("infrascan_cloudbuild_null", cbm_rs_pipeline_is_cloudbuild_file_v1(NULL), 0);

    check_int("infrascan_kustomize_yaml",
              cbm_rs_pipeline_is_kustomize_file_v1("kustomization.yaml"), 1);
    check_int("infrascan_kustomize_yml",
              cbm_rs_pipeline_is_kustomize_file_v1("kustomization.yml"), 1);
    check_int("infrascan_kustomize_case",
              cbm_rs_pipeline_is_kustomize_file_v1("KUSTOMIZATION.YAML"), 1);
    check_int("infrascan_kustomize_false",
              cbm_rs_pipeline_is_kustomize_file_v1("deployment.yaml"), 0);
    check_int("infrascan_kustomize_suffix_false",
              cbm_rs_pipeline_is_kustomize_file_v1("kustomization.yml.bak"), 0);
    check_int("infrascan_kustomize_null", cbm_rs_pipeline_is_kustomize_file_v1(NULL), 0);

    check_int("infrascan_shell_sh",
              cbm_rs_pipeline_is_shell_script_v1("run.sh", ".sh"), 1);
    check_int("infrascan_shell_bash",
              cbm_rs_pipeline_is_shell_script_v1("deploy.bash", ".bash"), 1);
    check_int("infrascan_shell_zsh",
              cbm_rs_pipeline_is_shell_script_v1("init.zsh", ".zsh"), 1);
    check_int("infrascan_shell_false_py",
              cbm_rs_pipeline_is_shell_script_v1("main.py", ".py"), 0);
    check_int("infrascan_shell_false_empty",
              cbm_rs_pipeline_is_shell_script_v1("Dockerfile", ""), 0);
    check_int("infrascan_shell_false_null_ext",
              cbm_rs_pipeline_is_shell_script_v1("run.sh", NULL), 0);
}

static void test_pipeline_githistory_export(void) {
    check_int("githistory_trackable_source", cbm_rs_pipeline_is_trackable_file_v1("main.go"), 1);
    check_int("githistory_trackable_readme", cbm_rs_pipeline_is_trackable_file_v1("README.md"),
              1);
    check_int("githistory_skip_node_modules",
              cbm_rs_pipeline_is_trackable_file_v1("node_modules/pkg/index.js"), 0);
    check_int("githistory_skip_vendor", cbm_rs_pipeline_is_trackable_file_v1("vendor/lib.go"),
              0);
    check_int("githistory_skip_lock_basename",
              cbm_rs_pipeline_is_trackable_file_v1("package-lock.json"), 0);
    check_int("githistory_skip_sum", cbm_rs_pipeline_is_trackable_file_v1("go.sum"), 0);
    check_int("githistory_skip_image", cbm_rs_pipeline_is_trackable_file_v1("image.png"), 0);
    check_int("githistory_skip_minified",
              cbm_rs_pipeline_is_trackable_file_v1("src/app.min.js"), 0);
    check_int("githistory_null", cbm_rs_pipeline_is_trackable_file_v1(NULL), 0);
}

static void test_pipeline_gitdiff_range_export(void) {
    int start = -1;
    int count = -1;

    cbm_rs_pipeline_parse_range_v1("10,5", &start, &count);
    check_int("gitdiff_range_start", start, 10);
    check_int("gitdiff_range_count", count, 5);
    cbm_rs_pipeline_parse_range_v1("42", &start, &count);
    check_int("gitdiff_range_default_count_start", start, 42);
    check_int("gitdiff_range_default_count", count, 1);
    cbm_rs_pipeline_parse_range_v1("1,0", &start, &count);
    check_int("gitdiff_range_zero_count_start", start, 1);
    check_int("gitdiff_range_zero_count", count, 0);
    cbm_rs_pipeline_parse_range_v1("1, 1", &start, &count);
    check_int("gitdiff_range_space_start", start, 1);
    check_int("gitdiff_range_space_count", count, 1);
    cbm_rs_pipeline_parse_range_v1("x,5", &start, &count);
    check_int("gitdiff_range_invalid_start", start, 0);
    check_int("gitdiff_range_invalid_count", count, 5);
    cbm_rs_pipeline_parse_range_v1(NULL, &start, &count);
    check_int("gitdiff_range_null_start", start, 0);
    check_int("gitdiff_range_null_count", count, 1);
}

static void test_pipeline_gitdiff_name_status_export(void) {
    struct {
        char status[4];
        char path[512];
        char old_path[512];
    } files[16];
    int n = cbm_rs_pipeline_parse_name_status_v1(
        "M\tinternal/store/nodes.go\nA\tpackage-lock.json\nR100\tsrc/old.go\tsrc/new.go\n",
        files, 16);
    check_int("gitdiff_name_status_count", n, 2);
    check_str("gitdiff_name_status_first_status", files[0].status, "M");
    check_str("gitdiff_name_status_first_path", files[0].path, "internal/store/nodes.go");
    check_str("gitdiff_name_status_rename_status", files[1].status, "R");
    check_str("gitdiff_name_status_rename_path", files[1].path, "src/new.go");
    check_str("gitdiff_name_status_rename_old_path", files[1].old_path, "src/old.go");
    check_int("gitdiff_name_status_null", cbm_rs_pipeline_parse_name_status_v1(NULL, files, 16), 0);
    check_int("gitdiff_name_status_zero_limit",
              cbm_rs_pipeline_parse_name_status_v1("M\tmain.go\n", files, 0), 0);

    char long_input[528];
    memset(long_input, 'a', sizeof(long_input));
    long_input[0] = 'M';
    long_input[1] = '\t';
    memcpy(long_input + 510, ".png\n", 6);
    n = cbm_rs_pipeline_parse_name_status_v1(long_input, files, 16);
    check_int("gitdiff_name_status_truncated_count", n, 1);
    check_int("gitdiff_name_status_truncated_len", (int)strlen(files[0].path), 511);
    check_str("gitdiff_name_status_truncated_suffix", files[0].path + 508, ".pn");
}

static void test_pipeline_gitdiff_hunks_export(void) {
    struct {
        char path[512];
        int start_line;
        int end_line;
    } hunks[16];
    int n = cbm_rs_pipeline_parse_hunks_v1(
        "+++ b/main.go\n@@ -10,3 +10,5 @@ func main() {\n"
        "+++ b/binary.png\n@@ -1 +1 @@ ignored\n"
        "+++ b/utils.go\n@@ -50,0 +52,2 @@ func helper() {\n",
        hunks, 16);
    check_int("gitdiff_hunks_count", n, 2);
    check_str("gitdiff_hunks_first_path", hunks[0].path, "main.go");
    check_int("gitdiff_hunks_first_start", hunks[0].start_line, 10);
    check_int("gitdiff_hunks_first_end", hunks[0].end_line, 14);
    check_str("gitdiff_hunks_second_path", hunks[1].path, "utils.go");
    check_int("gitdiff_hunks_second_start", hunks[1].start_line, 52);
    check_int("gitdiff_hunks_second_end", hunks[1].end_line, 53);
    check_int("gitdiff_hunks_null", cbm_rs_pipeline_parse_hunks_v1(NULL, hunks, 16), 0);
    check_int("gitdiff_hunks_zero_limit",
              cbm_rs_pipeline_parse_hunks_v1("+++ b/main.go\n@@ -1 +1 @@\n", hunks, 0), 0);
}

static void test_pipeline_module_dir_export(void) {
    check_int("module_dir_go", cbm_rs_pipeline_is_module_dir_v1(0), 1);
    check_int("module_dir_java", cbm_rs_pipeline_is_module_dir_v1(6), 1);
    check_int("module_dir_python", cbm_rs_pipeline_is_module_dir_v1(1), 0);
    check_int("module_dir_unknown", cbm_rs_pipeline_is_module_dir_v1(999), 0);
}

static void test_pipeline_route_canon_export(void) {
    char out[128];

    check_size("route_canon_null_len", cbm_rs_pipeline_route_canon_path_v1(out, sizeof(out), NULL),
               0);
    check_str("route_canon_null_out", out, "");
    check_size("route_canon_empty_len", cbm_rs_pipeline_route_canon_path_v1(out, sizeof(out), ""),
               0);
    check_str("route_canon_empty_out", out, "");
    check_size("route_canon_static_len",
               cbm_rs_pipeline_route_canon_path_v1(out, sizeof(out), "/products/categories"),
               strlen("/products/categories"));
    check_str("route_canon_static_out", out, "/products/categories");

    check_size("route_canon_colon_len",
               cbm_rs_pipeline_route_canon_path_v1(out, sizeof(out), "/players/:id"),
               strlen("/players/{}"));
    check_str("route_canon_colon_out", out, "/players/{}");
    check_size("route_canon_brace_len",
               cbm_rs_pipeline_route_canon_path_v1(out, sizeof(out), "/players/{id}"),
               strlen("/players/{}"));
    check_str("route_canon_brace_out", out, "/players/{}");
    check_size("route_canon_angle_len",
               cbm_rs_pipeline_route_canon_path_v1(out, sizeof(out), "/users/<int:id>"),
               strlen("/users/{}"));
    check_str("route_canon_angle_out", out, "/users/{}");
    check_size("route_canon_template_len",
               cbm_rs_pipeline_route_canon_path_v1(out, sizeof(out), "/players/${playerId}"),
               strlen("/players/{}"));
    check_str("route_canon_template_out", out, "/players/{}");
    check_size(
        "route_canon_multi_len",
        cbm_rs_pipeline_route_canon_path_v1(out, sizeof(out), "/orders/{id}/items/{itemIndex}"),
        strlen("/orders/{}/items/{}"));
    check_str("route_canon_multi_out", out, "/orders/{}/items/{}");

    check_size("route_canon_mid_colon_len",
               cbm_rs_pipeline_route_canon_path_v1(out, sizeof(out), "/a/b:c"), strlen("/a/b:c"));
    check_str("route_canon_mid_colon_out", out, "/a/b:c");
    check_size("route_canon_unclosed_brace_len",
               cbm_rs_pipeline_route_canon_path_v1(out, sizeof(out), "/a/{id/next"),
               strlen("/a/{}/next"));
    check_str("route_canon_unclosed_brace_out", out, "/a/{}/next");

    char short_buf[6];
    memset(short_buf, 'Z', sizeof(short_buf));
    check_size("route_canon_short_len",
               cbm_rs_pipeline_route_canon_path_v1(short_buf, sizeof(short_buf), "/players/:id"),
               strlen("/players/{}"));
    check_str("route_canon_short_out", short_buf, "/play");
    check_size("route_canon_len_only", cbm_rs_pipeline_route_canon_path_v1(NULL, 0, "/players/:id"),
               strlen("/players/{}"));
}

static void test_pipeline_route_args_export(void) {
    char out[128];

    check_int("route_args_prefix", cbm_rs_pipeline_is_path_keyword_v1("prefix"), 1);
    check_int("route_args_route_path", cbm_rs_pipeline_is_path_keyword_v1("route_path"), 1);
    check_int("route_args_handler", cbm_rs_pipeline_is_path_keyword_v1("handler"), 0);
    check_int("route_args_null_keyword", cbm_rs_pipeline_is_path_keyword_v1(NULL), 0);
    check_int("route_args_normalize", cbm_rs_pipeline_normalize_url_arg_v1(
                                         out, sizeof(out), "`/users/${id}/posts`"), 1);
    check_str("route_args_normalized_out", out, "/users/:id/posts");
    check_int("route_args_reject_relative",
              cbm_rs_pipeline_normalize_url_arg_v1(out, sizeof(out), "users/${id}"), 0);
    check_int("route_args_reject_short", cbm_rs_pipeline_normalize_url_arg_v1(out, sizeof(out), "/api"),
              0);
    check_int("route_args_null_input", cbm_rs_pipeline_normalize_url_arg_v1(out, sizeof(out), NULL), 0);
}

#ifdef CBM_USE_RUST_PIPELINE_ROUTE_ARGS_ONLY
static void test_pipeline_route_args_direct_exports(void) {
    char out[128];

    check_int("route_args_direct_prefix", cbm_pipeline_is_path_keyword("prefix"), 1);
    check_int("route_args_direct_route_path", cbm_pipeline_is_path_keyword("route_path"), 1);
    check_int("route_args_direct_handler", cbm_pipeline_is_path_keyword("handler"), 0);
    check_int("route_args_direct_null_keyword", cbm_pipeline_is_path_keyword(NULL), 0);
    check_int("route_args_direct_normalize",
              cbm_pipeline_normalize_url_arg("`/users/${id}/posts`", out, (int)sizeof(out)), 1);
    check_str("route_args_direct_normalized_out", out, "/users/:id/posts");
    check_int("route_args_direct_reject_relative",
              cbm_pipeline_normalize_url_arg("users/${id}", out, (int)sizeof(out)), 0);
    check_int("route_args_direct_reject_short",
              cbm_pipeline_normalize_url_arg("/api", out, (int)sizeof(out)), 0);
    check_int("route_args_direct_null_input", cbm_pipeline_normalize_url_arg(NULL, out, (int)sizeof(out)),
              0);
}
#endif

static void test_pipeline_route_node_classifiers_export(void) {
    static const unsigned char max_hash[] = "012345678901";
    static const unsigned char too_long_hash[] = "0123456789012";

    check_int("route_node_hash_digits",
              cbm_rs_pipeline_is_hash_segment_v1((const unsigned char *)"12345", 5), 1);
    check_int("route_node_hash_short_word",
              cbm_rs_pipeline_is_hash_segment_v1((const unsigned char *)"a1", 2), 1);
    check_int("route_node_hash_api",
              cbm_rs_pipeline_is_hash_segment_v1((const unsigned char *)"api", 3), 0);
    check_int("route_node_hash_upper",
              cbm_rs_pipeline_is_hash_segment_v1((const unsigned char *)"ABC", 3), 0);
    check_int("route_node_hash_max_len",
              cbm_rs_pipeline_is_hash_segment_v1(max_hash, sizeof(max_hash) - 1), 1);
    check_int("route_node_hash_too_long",
              cbm_rs_pipeline_is_hash_segment_v1(too_long_hash, sizeof(too_long_hash) - 1), 0);
    check_int("route_node_hash_empty", cbm_rs_pipeline_is_hash_segment_v1((const unsigned char *)"", 0),
              0);
    check_int("route_node_hash_null", cbm_rs_pipeline_is_hash_segment_v1(NULL, 1), 0);
    check_int("route_node_broker_kafka",
              cbm_rs_pipeline_is_broker_route_v1("__route__kafka__topic"), 1);
    check_int("route_node_broker_case",
              cbm_rs_pipeline_is_broker_route_v1("__route__KAFKA__topic"), 0);
    check_int("route_node_broker_http", cbm_rs_pipeline_is_broker_route_v1("__route__GET__/api"), 0);
    check_int("route_node_broker_null", cbm_rs_pipeline_is_broker_route_v1(NULL), 0);

    check_int("route_node_bridge_hash_digits",
              cbm_pipeline_is_hash_segment((const unsigned char *)"12345", 5), 1);
    check_int("route_node_bridge_hash_max_len",
              cbm_pipeline_is_hash_segment(max_hash, sizeof(max_hash) - 1), 1);
    check_int("route_node_bridge_hash_too_long",
              cbm_pipeline_is_hash_segment(too_long_hash, sizeof(too_long_hash) - 1), 0);
    check_int("route_node_bridge_hash_empty",
              cbm_pipeline_is_hash_segment((const unsigned char *)"", 0), 0);
    check_int("route_node_bridge_hash_null", cbm_pipeline_is_hash_segment(NULL, 1), 0);
    check_int("route_node_bridge_broker_kafka",
              cbm_pipeline_is_broker_route("__route__kafka__topic"), 1);
    check_int("route_node_bridge_broker_case",
              cbm_pipeline_is_broker_route("__route__KAFKA__topic"), 0);
    check_int("route_node_bridge_broker_http", cbm_pipeline_is_broker_route("__route__GET__/api"),
              0);
    check_int("route_node_bridge_broker_null", cbm_pipeline_is_broker_route(NULL), 0);
}

static void test_pipeline_sveltekit_file_kind_export(void) {
    check_int("sveltekit_server_ts",
              cbm_rs_pipeline_sveltekit_file_kind_v1("apps/x/src/routes/+server.ts"), 1);
    check_int("sveltekit_server_js",
              cbm_rs_pipeline_sveltekit_file_kind_v1("apps/x/src/routes/foo/+server.js"), 1);
    check_int("sveltekit_page_js",
              cbm_rs_pipeline_sveltekit_file_kind_v1("apps/x/src/routes/foo/+page.server.js"), 2);
    check_int("sveltekit_page_ts",
              cbm_rs_pipeline_sveltekit_file_kind_v1("apps/x/src/routes/foo/+page.server.ts"), 2);
    check_int("sveltekit_layout_ts",
              cbm_rs_pipeline_sveltekit_file_kind_v1("apps/x/src/routes/foo/+layout.server.ts"), 3);
    check_int("sveltekit_layout_js",
              cbm_rs_pipeline_sveltekit_file_kind_v1("apps/x/src/routes/foo/+layout.server.js"), 3);
    check_int("sveltekit_missing_routes",
              cbm_rs_pipeline_sveltekit_file_kind_v1("apps/x/src/+server.ts"), 0);
    check_int("sveltekit_wrong_extension",
              cbm_rs_pipeline_sveltekit_file_kind_v1("apps/x/src/routes/+server.jsx"), 0);
    check_int("sveltekit_null", cbm_rs_pipeline_sveltekit_file_kind_v1(NULL), 0);

#ifdef CBM_USE_RUST_PIPELINE_SVELTEKIT_FILE_KIND_ONLY
    check_int("sveltekit_direct_server",
              cbm_pipeline_sveltekit_file_kind("apps/x/src/routes/+server.ts"), 1);
    check_int("sveltekit_direct_missing_routes",
              cbm_pipeline_sveltekit_file_kind("apps/x/src/+server.ts"), 0);
    check_int("sveltekit_direct_null", cbm_pipeline_sveltekit_file_kind(NULL), 0);
#endif
}

static void test_pipeline_sveltekit_server_method_export(void) {
    check_str("sveltekit_method_get", cbm_rs_pipeline_sveltekit_server_method_v1("GET"), "GET");
    check_str("sveltekit_method_fallback", cbm_rs_pipeline_sveltekit_server_method_v1("fallback"),
              "ANY");
    check_null("sveltekit_method_unknown", cbm_rs_pipeline_sveltekit_server_method_v1("CONNECT"));
    check_null("sveltekit_method_null", cbm_rs_pipeline_sveltekit_server_method_v1(NULL));

#ifdef CBM_USE_RUST_PIPELINE_SVELTEKIT_SERVER_METHOD_ONLY
    check_str("sveltekit_method_direct_post", cbm_pipeline_sveltekit_server_method("POST"), "POST");
    check_str("sveltekit_method_direct_fallback", cbm_pipeline_sveltekit_server_method("fallback"),
              "ANY");
    check_null("sveltekit_method_direct_unknown", cbm_pipeline_sveltekit_server_method("CONNECT"));
    check_null("sveltekit_method_direct_null", cbm_pipeline_sveltekit_server_method(NULL));
#endif
}

static void test_pipeline_extract_json_prop_export(void) {
    static const char nul_truncated_json[] = "{\"other\":\"x\"}\0{\"route_path\":\"after\"}";
    const char *json = "{\"route_path\":\"/api/items\",\"route_method\":\"GET\"}";
    const char *empty_key_json = "{\"\":\"empty\"}";
    const char *empty_value_json = "{\"route_path\":\"\"}";
    const char *short_value_json = "{\"route_path\":\"GET\"}";
    char v1_buf[16] = {'V', '1', '!', '!', '!', '!', '!', '!', '\0'};
    char bridge_buf[16] = {'B', 'R', '!', '!', '!', '!', '!', '!', '\0'};
    char v1_exact[4] = {'V', '1', '!', '\0'};
    char bridge_exact[4] = {'B', 'R', '!', '\0'};
    char v1_short[3] = {'V', '1', '!'};
    char bridge_short[3] = {'B', 'R', '!'};
    char key_59[60];
    char key_60[61];
    char json_59[80];

    check_int("pipeline_json_prop_v1_null_json",
              cbm_rs_pipeline_extract_json_prop_v1(NULL, "route_path", v1_buf,
                                                    (int)sizeof(v1_buf)),
              0);
    check_int("pipeline_json_prop_v1_null_json_sentinel", (unsigned char)v1_buf[0], 'V');
    check_int("pipeline_json_prop_v1_null_key",
              cbm_rs_pipeline_extract_json_prop_v1(json, NULL, v1_buf, (int)sizeof(v1_buf)), 0);
    check_int("pipeline_json_prop_v1_null_key_sentinel", (unsigned char)v1_buf[0], 'V');
    check_int("pipeline_json_prop_v1_null_buf",
              cbm_rs_pipeline_extract_json_prop_v1(json, "route_path", NULL, (int)sizeof(v1_buf)),
              0);

    check_bool("pipeline_json_prop_bridge_null_json",
               cbm_pipeline_extract_json_prop(NULL, "route_path", bridge_buf,
                                              (int)sizeof(bridge_buf)),
               false);
    check_int("pipeline_json_prop_bridge_null_json_sentinel", (unsigned char)bridge_buf[0], 'B');
    check_bool("pipeline_json_prop_bridge_null_key",
               cbm_pipeline_extract_json_prop(json, NULL, bridge_buf, (int)sizeof(bridge_buf)),
               false);
    check_int("pipeline_json_prop_bridge_null_key_sentinel", (unsigned char)bridge_buf[0], 'B');
    check_bool("pipeline_json_prop_bridge_null_buf",
               cbm_pipeline_extract_json_prop(json, "route_path", NULL, (int)sizeof(bridge_buf)),
               false);

    check_int("pipeline_json_prop_v1_bufsz_zero",
              cbm_rs_pipeline_extract_json_prop_v1(json, "route_path", v1_buf, 0), 0);
    check_int("pipeline_json_prop_v1_bufsz_zero_sentinel", (unsigned char)v1_buf[0], 'V');
    check_int("pipeline_json_prop_v1_bufsz_negative",
              cbm_rs_pipeline_extract_json_prop_v1(json, "route_path", v1_buf, -1), 0);
    check_int("pipeline_json_prop_v1_bufsz_negative_sentinel", (unsigned char)v1_buf[0], 'V');
    check_int("pipeline_json_prop_v1_bufsz_int_min",
              cbm_rs_pipeline_extract_json_prop_v1(json, "route_path", v1_buf, INT_MIN), 0);
    check_int("pipeline_json_prop_v1_bufsz_int_min_sentinel", (unsigned char)v1_buf[0], 'V');

    check_bool("pipeline_json_prop_bridge_bufsz_zero",
               cbm_pipeline_extract_json_prop(json, "route_path", bridge_buf, 0), false);
    check_int("pipeline_json_prop_bridge_bufsz_zero_sentinel", (unsigned char)bridge_buf[0], 'B');
    check_bool("pipeline_json_prop_bridge_bufsz_negative",
               cbm_pipeline_extract_json_prop(json, "route_path", bridge_buf, -1), false);
    check_int("pipeline_json_prop_bridge_bufsz_negative_sentinel", (unsigned char)bridge_buf[0], 'B');
    check_bool("pipeline_json_prop_bridge_bufsz_int_min",
               cbm_pipeline_extract_json_prop(json, "route_path", bridge_buf, INT_MIN), false);
    check_int("pipeline_json_prop_bridge_bufsz_int_min_sentinel", (unsigned char)bridge_buf[0], 'B');

    check_int("pipeline_json_prop_v1_empty_key",
              cbm_rs_pipeline_extract_json_prop_v1(empty_key_json, "", v1_buf,
                                                    (int)sizeof(v1_buf)),
              1);
    check_str("pipeline_json_prop_v1_empty_key_value", v1_buf, "empty");
    check_bool("pipeline_json_prop_bridge_empty_key",
               cbm_pipeline_extract_json_prop(empty_key_json, "", bridge_buf,
                                              (int)sizeof(bridge_buf)),
               true);
    check_str("pipeline_json_prop_bridge_empty_key_value", bridge_buf, "empty");

    v1_buf[0] = 'V';
    bridge_buf[0] = 'B';
    check_int("pipeline_json_prop_v1_empty_value",
              cbm_rs_pipeline_extract_json_prop_v1(empty_value_json, "route_path", v1_buf,
                                                    (int)sizeof(v1_buf)),
              0);
    check_int("pipeline_json_prop_v1_empty_value_sentinel", (unsigned char)v1_buf[0], 'V');
    check_bool("pipeline_json_prop_bridge_empty_value",
               cbm_pipeline_extract_json_prop(empty_value_json, "route_path", bridge_buf,
                                              (int)sizeof(bridge_buf)),
               false);
    check_int("pipeline_json_prop_bridge_empty_value_sentinel", (unsigned char)bridge_buf[0], 'B');

    check_int("pipeline_json_prop_v1_exact_buffer",
              cbm_rs_pipeline_extract_json_prop_v1(short_value_json, "route_path", v1_exact,
                                                    (int)sizeof(v1_exact)),
              1);
    check_str("pipeline_json_prop_v1_exact_buffer_value", v1_exact, "GET");
    check_bool("pipeline_json_prop_bridge_exact_buffer",
               cbm_pipeline_extract_json_prop(short_value_json, "route_path", bridge_exact,
                                              (int)sizeof(bridge_exact)),
               true);
    check_str("pipeline_json_prop_bridge_exact_buffer_value", bridge_exact, "GET");

    check_int("pipeline_json_prop_v1_short_buffer",
              cbm_rs_pipeline_extract_json_prop_v1(short_value_json, "route_path", v1_short,
                                                    (int)sizeof(v1_short)),
              0);
    check_int("pipeline_json_prop_v1_short_buffer_sentinel", (unsigned char)v1_short[0], 'V');
    check_bool("pipeline_json_prop_bridge_short_buffer",
               cbm_pipeline_extract_json_prop(short_value_json, "route_path", bridge_short,
                                              (int)sizeof(bridge_short)),
               false);
    check_int("pipeline_json_prop_bridge_short_buffer_sentinel", (unsigned char)bridge_short[0], 'B');

    memset(key_59, 'k', sizeof(key_59) - 1);
    key_59[sizeof(key_59) - 1] = '\0';
    memset(key_60, 'k', sizeof(key_60) - 1);
    key_60[sizeof(key_60) - 1] = '\0';
    (void)snprintf(json_59, sizeof(json_59), "{\"%s\":\"v\"}", key_59);
    check_int("pipeline_json_prop_v1_key_59",
              cbm_rs_pipeline_extract_json_prop_v1(json_59, key_59, v1_buf, (int)sizeof(v1_buf)),
              1);
    check_str("pipeline_json_prop_v1_key_59_value", v1_buf, "v");
    check_bool("pipeline_json_prop_bridge_key_59",
               cbm_pipeline_extract_json_prop(json_59, key_59, bridge_buf, (int)sizeof(bridge_buf)),
               true);
    check_str("pipeline_json_prop_bridge_key_59_value", bridge_buf, "v");

    v1_buf[0] = 'V';
    bridge_buf[0] = 'B';
    check_int("pipeline_json_prop_v1_key_60",
              cbm_rs_pipeline_extract_json_prop_v1(json, key_60, v1_buf, (int)sizeof(v1_buf)), 0);
    check_int("pipeline_json_prop_v1_key_60_sentinel", (unsigned char)v1_buf[0], 'V');
    check_bool("pipeline_json_prop_bridge_key_60",
               cbm_pipeline_extract_json_prop(json, key_60, bridge_buf, (int)sizeof(bridge_buf)),
               false);
    check_int("pipeline_json_prop_bridge_key_60_sentinel", (unsigned char)bridge_buf[0], 'B');

    v1_buf[0] = 'V';
    bridge_buf[0] = 'B';
    check_int("pipeline_json_prop_v1_nul_truncation",
              cbm_rs_pipeline_extract_json_prop_v1(nul_truncated_json, "route_path", v1_buf,
                                                    (int)sizeof(v1_buf)),
              0);
    check_int("pipeline_json_prop_v1_nul_truncation_sentinel", (unsigned char)v1_buf[0], 'V');
    check_bool("pipeline_json_prop_bridge_nul_truncation",
               cbm_pipeline_extract_json_prop(nul_truncated_json, "route_path", bridge_buf,
                                              (int)sizeof(bridge_buf)),
               false);
    check_int("pipeline_json_prop_bridge_nul_truncation_sentinel", (unsigned char)bridge_buf[0], 'B');
}

static void test_pipeline_url_path_export(void) {
    check_str("pipeline_url_path_full",
              cbm_rs_pipeline_url_path_v1("https://example.test/api/items"), "/api/items");
    check_str("pipeline_url_path_root", cbm_rs_pipeline_url_path_v1("https://example.test"), "/");
    check_str("pipeline_url_path_plain", cbm_rs_pipeline_url_path_v1("/api/items"), "/api/items");
    check_str("pipeline_url_path_query_slash",
              cbm_rs_pipeline_url_path_v1("https://example.test?next=/query"), "/query");
    check_str("pipeline_url_path_malformed",
              cbm_rs_pipeline_url_path_v1("https:/example.test/path"), "https:/example.test/path");
    check_str("pipeline_url_path_empty_scheme", cbm_rs_pipeline_url_path_v1("://example.test/path"),
              "/path");
    check_str("pipeline_url_path_empty_authority", cbm_rs_pipeline_url_path_v1("http:///path"),
              "/path");

    char plain_root[] = "/";
    check_ptr_eq("pipeline_url_path_plain_root_pointer", cbm_rs_pipeline_url_path_v1(plain_root),
                 plain_root);
    char url_root[] = "https://example.test/";
    check_ptr_eq("pipeline_url_path_url_root_pointer", cbm_rs_pipeline_url_path_v1(url_root),
                 strrchr(url_root, '/'));
    check_null("pipeline_url_path_null", cbm_rs_pipeline_url_path_v1(NULL));

#ifdef CBM_USE_RUST_PIPELINE_URL_PATH_ONLY
    check_str("pipeline_url_path_direct", cbm_pipeline_url_path("http://host/direct"), "/direct");
    char direct_root[] = "http://host/";
    check_ptr_eq("pipeline_url_path_direct_root_pointer", cbm_pipeline_url_path(direct_root),
                 strrchr(direct_root, '/'));
#endif
}

#ifdef CBM_USE_RUST_PIPELINE_ROUTE_NODE_CLASSIFIERS_ONLY
static void test_pipeline_route_node_classifiers_direct_exports(void) {
    static const unsigned char max_hash[] = "012345678901";
    static const unsigned char too_long_hash[] = "0123456789012";

    check_int("route_node_direct_hash_digits",
              cbm_pipeline_is_hash_segment((const unsigned char *)"12345", 5), 1);
    check_int("route_node_direct_hash_max_len",
              cbm_pipeline_is_hash_segment(max_hash, sizeof(max_hash) - 1), 1);
    check_int("route_node_direct_hash_too_long",
              cbm_pipeline_is_hash_segment(too_long_hash, sizeof(too_long_hash) - 1), 0);
    check_int("route_node_direct_hash_empty", cbm_pipeline_is_hash_segment((const unsigned char *)"", 0),
              0);
    check_int("route_node_direct_hash_null", cbm_pipeline_is_hash_segment(NULL, 1), 0);
    check_int("route_node_direct_broker_kafka",
              cbm_pipeline_is_broker_route("__route__kafka__topic"), 1);
    check_int("route_node_direct_broker_case",
              cbm_pipeline_is_broker_route("__route__KAFKA__topic"), 0);
    check_int("route_node_direct_broker_http", cbm_pipeline_is_broker_route("__route__GET__/api"),
              0);
    check_int("route_node_direct_broker_null", cbm_pipeline_is_broker_route(NULL), 0);
}
#endif

static void test_pipeline_test_detect_export(void) {
    check_int("pipeline_test_path_null", cbm_rs_pipeline_is_test_path_v1(NULL), 0);
    check_int("pipeline_test_path_empty", cbm_rs_pipeline_is_test_path_v1(""), 0);
    check_int("pipeline_test_path_go_suffix", cbm_rs_pipeline_is_test_path_v1("foo_test.go"), 1);
    check_int("pipeline_test_path_py_prefix", cbm_rs_pipeline_is_test_path_v1("test_handler.py"),
              1);
    check_int("pipeline_test_path_js_dot", cbm_rs_pipeline_is_test_path_v1("handler.test.js"), 1);
    check_int("pipeline_test_path_ts_spec", cbm_rs_pipeline_is_test_path_v1("handler.spec.ts"), 1);
    check_int("pipeline_test_path_tsx_substring",
              cbm_rs_pipeline_is_test_path_v1("handler.test.tsx"), 1);
    check_int("pipeline_test_path_ts_backup",
              cbm_rs_pipeline_is_test_path_v1("handler.test.ts.bak"), 1);
    check_int("pipeline_test_path_java_suffix", cbm_rs_pipeline_is_test_path_v1("OrderTest.java"),
              1);
    check_int("pipeline_test_path_ruby_spec",
              cbm_rs_pipeline_is_test_path_v1("spec/handler_spec.rb"), 1);
    check_int("pipeline_test_path_dir_tests", cbm_rs_pipeline_is_test_path_v1("src/tests/foo.c"),
              1);
    check_int("pipeline_test_path_dir_mytests",
              cbm_rs_pipeline_is_test_path_v1("src/mytests/foo.c"), 0);
    check_int("pipeline_test_path_contest", cbm_rs_pipeline_is_test_path_v1("contest/file.c"), 0);
    check_int("pipeline_test_path_testdata",
              cbm_rs_pipeline_is_test_path_v1("testdata/fixture.json"), 0);
    check_int("pipeline_test_path_windows_separator",
              cbm_rs_pipeline_is_test_path_v1("C:\\repo\\tests\\handler.py"), 0);
    check_int("pipeline_test_path_plain", cbm_rs_pipeline_is_test_path_v1("handler.ts"), 0);

    check_int("pipeline_test_func_null", cbm_rs_pipeline_is_test_func_name_v1(NULL), 0);
    check_int("pipeline_test_func_empty", cbm_rs_pipeline_is_test_func_name_v1(""), 0);
    check_int("pipeline_test_func_test_alone", cbm_rs_pipeline_is_test_func_name_v1("Test"), 1);
    check_int("pipeline_test_func_test_upper",
              cbm_rs_pipeline_is_test_func_name_v1("TestHTTPHandler"), 1);
    check_int("pipeline_test_func_testable", cbm_rs_pipeline_is_test_func_name_v1("Testable"), 0);
    check_int("pipeline_test_func_benchmark", cbm_rs_pipeline_is_test_func_name_v1("Benchmark"), 1);
    check_int("pipeline_test_func_benchmarkable",
              cbm_rs_pipeline_is_test_func_name_v1("Benchmarkable"), 0);
    check_int("pipeline_test_func_example", cbm_rs_pipeline_is_test_func_name_v1("Example"), 1);
    check_int("pipeline_test_func_example_lower",
              cbm_rs_pipeline_is_test_func_name_v1("Examplefoo"), 0);
    check_int("pipeline_test_func_test_lower", cbm_rs_pipeline_is_test_func_name_v1("testa"), 0);
    check_int("pipeline_test_func_test_upper_lowercase_prefix",
              cbm_rs_pipeline_is_test_func_name_v1("testA"), 1);
    check_int("pipeline_test_func_test_underscore", cbm_rs_pipeline_is_test_func_name_v1("test_"),
              1);
    check_int("pipeline_test_func_before_each", cbm_rs_pipeline_is_test_func_name_v1("beforeEach"),
              1);
    check_int("pipeline_test_func_at_test", cbm_rs_pipeline_is_test_func_name_v1("@test"), 1);
    check_int("pipeline_test_func_create", cbm_rs_pipeline_is_test_func_name_v1("create"), 0);
}

static void test_pipeline_test_to_prod_path_export(void) {
    char out[128];
    char small[6];

    memset(out, 'Z', sizeof(out));
    check_size("pipeline_test_to_prod_null",
               cbm_rs_pipeline_test_to_prod_path_v1(out, sizeof(out), NULL), SIZE_MAX);
    check_int("pipeline_test_to_prod_null_unchanged", out[0], 'Z');

    check_size("pipeline_test_to_prod_go",
               cbm_rs_pipeline_test_to_prod_path_v1(out, sizeof(out), "foo_test.go"),
               strlen("foo.go"));
    check_str("pipeline_test_to_prod_go_out", out, "foo.go");

    check_size("pipeline_test_to_prod_py",
               cbm_rs_pipeline_test_to_prod_path_v1(out, sizeof(out), "nested/test_handler.py"),
               strlen("nested/handler.py"));
    check_str("pipeline_test_to_prod_py_out", out, "nested/handler.py");

    check_size("pipeline_test_to_prod_ts",
               cbm_rs_pipeline_test_to_prod_path_v1(out, sizeof(out), "handler.test.ts"),
               strlen("handler.ts"));
    check_str("pipeline_test_to_prod_ts_out", out, "handler.ts");

    check_size("pipeline_test_to_prod_spec",
               cbm_rs_pipeline_test_to_prod_path_v1(out, sizeof(out), "handler.spec.tsx"),
               strlen("handler.tsx"));
    check_str("pipeline_test_to_prod_spec_out", out, "handler.tsx");

    check_size("pipeline_test_to_prod_backup",
               cbm_rs_pipeline_test_to_prod_path_v1(out, sizeof(out), "handler.test.ts.bak"),
               strlen("handler.ts.bak"));
    check_str("pipeline_test_to_prod_backup_out", out, "handler.ts.bak");

    memset(small, 'Z', sizeof(small));
    check_size("pipeline_test_to_prod_short",
               cbm_rs_pipeline_test_to_prod_path_v1(small, sizeof(small), "foo_test.go"),
               SIZE_MAX);
    check_int("pipeline_test_to_prod_short_unchanged", small[0], 'Z');
}

static void test_pipeline_checked_exception_export(void) {
    check_int("pipeline_checked_exception_null", cbm_rs_pipeline_is_checked_exception_v1(NULL), 0);
    check_int("pipeline_checked_exception_empty", cbm_rs_pipeline_is_checked_exception_v1(""), 1);
    check_int("pipeline_checked_exception_checked",
              cbm_rs_pipeline_is_checked_exception_v1("NotFoundException"), 1);
    check_int("pipeline_checked_exception_error",
              cbm_rs_pipeline_is_checked_exception_v1("NotFoundError"), 0);
    check_int("pipeline_checked_exception_panic", cbm_rs_pipeline_is_checked_exception_v1("panic"),
              0);
    check_int("pipeline_checked_exception_client_panic",
              cbm_rs_pipeline_is_checked_exception_v1("ClientPanic"), 0);
    check_int("pipeline_checked_exception_lower_error",
              cbm_rs_pipeline_is_checked_exception_v1("custom error"), 0);
    check_int("pipeline_checked_exception_checked_thing",
              cbm_rs_pipeline_is_checked_exception_v1("CheckedThing"), 1);
}

#ifdef CBM_USE_RUST_PIPELINE_CHECKED_EXCEPTION_ONLY
static void test_pipeline_checked_exception_direct_exports(void) {
    check_int("pipeline_checked_exception_direct_null", cbm_pipeline_is_checked_exception(NULL), 0);
    check_int("pipeline_checked_exception_direct_empty", cbm_pipeline_is_checked_exception(""), 1);
    check_int("pipeline_checked_exception_direct_checked",
              cbm_pipeline_is_checked_exception("NotFoundException"), 1);
    check_int("pipeline_checked_exception_direct_error",
              cbm_pipeline_is_checked_exception("NotFoundError"), 0);
    check_int("pipeline_checked_exception_direct_panic", cbm_pipeline_is_checked_exception("panic"), 0);
    check_int("pipeline_checked_exception_direct_client_panic",
              cbm_pipeline_is_checked_exception("ClientPanic"), 0);
    check_int("pipeline_checked_exception_direct_lower_error",
              cbm_pipeline_is_checked_exception("custom error"), 0);
    check_int("pipeline_checked_exception_direct_checked_thing",
              cbm_pipeline_is_checked_exception("CheckedThing"), 1);
}
#endif

static void test_pipeline_artifact_path_export(void) {
    char out[128];
    char short_buf[8];

    memset(out, 'Z', sizeof(out));
    check_size("pipeline_artifact_path_null_repo_len",
               cbm_rs_pipeline_artifact_path_v1(out, sizeof(out), NULL, CBM_ARTIFACT_FILENAME),
               SIZE_MAX);
    check_int("pipeline_artifact_path_null_repo_unchanged", out[0], 'Z');

    memset(out, 'Z', sizeof(out));
    check_size("pipeline_artifact_path_null_name_len",
               cbm_rs_pipeline_artifact_path_v1(out, sizeof(out), "/tmp/repo", NULL), SIZE_MAX);
    check_int("pipeline_artifact_path_null_name_unchanged", out[0], 'Z');

    check_size(
        "pipeline_artifact_path_null_buf_len",
        cbm_rs_pipeline_artifact_path_v1(NULL, sizeof(out), "/tmp/repo", CBM_ARTIFACT_FILENAME),
        SIZE_MAX);

    check_size("pipeline_artifact_path_zero_bufsize",
               cbm_rs_pipeline_artifact_path_v1(out, 0, "/tmp/repo", CBM_ARTIFACT_FILENAME),
               SIZE_MAX);

    check_size(
        "pipeline_artifact_path_repo_len",
        cbm_rs_pipeline_artifact_path_v1(out, sizeof(out), "/tmp/repo", CBM_ARTIFACT_FILENAME),
        strlen("/tmp/repo/.codebase-memory/" CBM_ARTIFACT_FILENAME));
    check_str("pipeline_artifact_path_repo_out", out,
              "/tmp/repo/.codebase-memory/" CBM_ARTIFACT_FILENAME);

    check_size("pipeline_artifact_path_meta_len",
               cbm_rs_pipeline_artifact_path_v1(out, sizeof(out), "/tmp/repo", CBM_ARTIFACT_META),
               strlen("/tmp/repo/.codebase-memory/" CBM_ARTIFACT_META));
    check_str("pipeline_artifact_path_meta_out", out,
              "/tmp/repo/.codebase-memory/" CBM_ARTIFACT_META);

    const char repo_non_utf8[] = "/tmp/\xff";
    const char name_non_utf8[] = "art\x80ifact.json";
    const char expected_non_utf8[] = "/tmp/\xff/.codebase-memory/art\x80ifact.json";
    check_size("pipeline_artifact_path_non_utf8_len",
               cbm_rs_pipeline_artifact_path_v1(out, sizeof(out), repo_non_utf8, name_non_utf8),
               sizeof(expected_non_utf8) - 1);
    check_int("pipeline_artifact_path_non_utf8_out",
              memcmp(out, expected_non_utf8, sizeof(expected_non_utf8) - 1), 0);

    memset(short_buf, 'Z', sizeof(short_buf));
    check_size("pipeline_artifact_path_short_len",
               cbm_rs_pipeline_artifact_path_v1(short_buf, sizeof(short_buf), "/tmp/repo",
                                                CBM_ARTIFACT_FILENAME),
               SIZE_MAX);
    check_str("pipeline_artifact_path_short_out", short_buf, "/tmp/re");
    check_int("pipeline_artifact_path_short_nul", short_buf[sizeof(short_buf) - 1], '\0');

    memset(out, 'Z', sizeof(out));
    check_int("pipeline_artifact_path_bridge_full_ok",
              cbm_pipeline_artifact_path(out, sizeof(out), "/tmp/repo", CBM_ARTIFACT_FILENAME),
              1);
    check_str("pipeline_artifact_path_bridge_full_out", out,
              "/tmp/repo/.codebase-memory/" CBM_ARTIFACT_FILENAME);

    memset(short_buf, 'Z', sizeof(short_buf));
    check_int("pipeline_artifact_path_bridge_short_ok",
              cbm_pipeline_artifact_path(short_buf, sizeof(short_buf), "/tmp/repo",
                                         CBM_ARTIFACT_FILENAME),
              0);
    check_str("pipeline_artifact_path_bridge_short_out", short_buf, "/tmp/re");
    check_int("pipeline_artifact_path_bridge_short_nul", short_buf[sizeof(short_buf) - 1], '\0');

    check_int("pipeline_artifact_path_bridge_null_buf",
              cbm_pipeline_artifact_path(NULL, sizeof(out), "/tmp/repo", CBM_ARTIFACT_FILENAME),
              0);

    memset(out, 'Z', sizeof(out));
    check_int("pipeline_artifact_path_bridge_zero_bufsize",
              cbm_pipeline_artifact_path(out, 0, "/tmp/repo", CBM_ARTIFACT_FILENAME), 0);
    check_int("pipeline_artifact_path_bridge_zero_bufsize_unchanged", out[0], 'Z');

    memset(out, 'Z', sizeof(out));
    check_int("pipeline_artifact_path_bridge_non_utf8_ok",
              cbm_pipeline_artifact_path(out, sizeof(out), repo_non_utf8, name_non_utf8), 1);
    check_int("pipeline_artifact_path_bridge_non_utf8_out",
              memcmp(out, expected_non_utf8, sizeof(expected_non_utf8) - 1), 0);
    check_int("pipeline_artifact_path_bridge_non_utf8_nul", out[sizeof(expected_non_utf8) - 1],
              '\0');
}

static uint64_t plan_top_step_bit(int kind) {
    return UINT64_C(1) << (kind - 1);
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
        PLAN_TOP_MACRO_EXTRACTION = CBM_RS_PLAN_TOP_MACRO_EXTRACTION,
        PLAN_TOP_USERCONFIG_LOAD = CBM_RS_PLAN_TOP_USERCONFIG_LOAD,
        PLAN_TOP_DISCOVER = CBM_RS_PLAN_TOP_DISCOVER,
        PLAN_TOP_TRY_INCREMENTAL_OR_DELETE_DB = CBM_RS_PLAN_TOP_TRY_INCREMENTAL_OR_DELETE_DB,
        PLAN_TOP_STRUCTURE = CBM_RS_PLAN_TOP_STRUCTURE,
        PLAN_TOP_EXTRACTION_DISPATCH = CBM_RS_PLAN_TOP_EXTRACTION_DISPATCH,
        PLAN_TOP_TESTS = CBM_RS_PLAN_TOP_TESTS,
        PLAN_TOP_GITHISTORY = CBM_RS_PLAN_TOP_GITHISTORY,
        PLAN_TOP_PREDUMP = CBM_RS_PLAN_TOP_PREDUMP,
        PLAN_TOP_DUMP = CBM_RS_PLAN_TOP_DUMP,
        PLAN_TOP_PERSIST_HASHES = CBM_RS_PLAN_TOP_PERSIST_HASHES,
        PLAN_TOP_ARTIFACT_EXPORT = CBM_RS_PLAN_TOP_ARTIFACT_EXPORT,
        PLAN_TOP_PHASE_FULL_PIPELINE = CBM_RS_PLAN_TOP_PHASE_FULL_PIPELINE,
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
        PLAN_TOP_POLICY_REQUIRED = CBM_RS_PLAN_TOP_POLICY_REQUIRED,
        PLAN_TOP_POLICY_BEST_EFFORT = CBM_RS_PLAN_TOP_POLICY_BEST_EFFORT,
        PLAN_TOP_POLICY_FULL_MODE_ONLY = CBM_RS_PLAN_TOP_POLICY_FULL_MODE_ONLY,
        PLAN_TOP_POLICY_FAIL_OPEN = CBM_RS_PLAN_TOP_POLICY_FAIL_OPEN,
        PLAN_TOP_POLICY_EXISTING_DB_ONLY = CBM_RS_PLAN_TOP_POLICY_EXISTING_DB_ONLY,
        PLAN_TOP_POLICY_OPTIONAL_PERSISTENCE = CBM_RS_PLAN_TOP_POLICY_OPTIONAL_PERSISTENCE,
        PLAN_TOP_GATE_SKIP_FAST = CBM_RS_PLAN_TOP_GATE_SKIP_FAST,
        PLAN_TOP_GATE_MAY_SHORT_CIRCUIT = CBM_RS_PLAN_TOP_GATE_MAY_SHORT_CIRCUIT,
        PLAN_TOP_EFFECT_MUTATES_GRAPH = CBM_RS_PLAN_TOP_EFFECT_MUTATES_GRAPH,
        PLAN_TOP_EFFECT_WRITES_STORE_OR_ARTIFACT = CBM_RS_PLAN_TOP_EFFECT_WRITES_STORE_OR_ARTIFACT,
        PLAN_TOP_NO_NESTED_PLAN = CBM_RS_PLAN_TOP_NO_NESTED_PLAN,
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
    CbmRsPipelineTopStepV1 top_steps[CBM_RS_PLAN_TOP_MAX_STEPS];
    CbmRsPipelineTopStepV1 short_top_steps[CBM_RS_PLAN_TOP_MAX_STEPS - 1];

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

    check_int("plan_full_top_count_parallel",
              cbm_rs_pipeline_full_plan_step_count_v1(MODE_FULL, 2, 51), CBM_RS_PLAN_TOP_MAX_STEPS);
    check_int(
        "plan_full_top_steps_parallel_len",
        cbm_rs_pipeline_full_plan_steps_v1(MODE_FULL, 2, 51, top_steps, CBM_RS_PLAN_TOP_MAX_STEPS),
        CBM_RS_PLAN_TOP_MAX_STEPS);
    check_int("plan_full_top_step0_kind", top_steps[0].kind, PLAN_TOP_MACRO_EXTRACTION);
    check_int("plan_full_top_step0_phase", top_steps[0].phase, PLAN_TOP_PHASE_FULL_PIPELINE);
    check_int("plan_full_top_step0_policy", top_steps[0].policy, PLAN_TOP_POLICY_FULL_MODE_ONLY);
    check_int("plan_full_top_step0_nested", top_steps[0].nested_plan_kind, PLAN_TOP_NO_NESTED_PLAN);
    check_int("plan_full_top_step1_kind", top_steps[1].kind, PLAN_TOP_USERCONFIG_LOAD);
    check_int("plan_full_top_step1_policy", top_steps[1].policy, PLAN_TOP_POLICY_FAIL_OPEN);
    check_int("plan_full_top_step3_kind", top_steps[3].kind, PLAN_TOP_TRY_INCREMENTAL_OR_DELETE_DB);
    check_int("plan_full_top_step3_policy", top_steps[3].policy, PLAN_TOP_POLICY_EXISTING_DB_ONLY);
    check_u32("plan_full_top_step3_gate", top_steps[3].gate_flags, PLAN_TOP_GATE_MAY_SHORT_CIRCUIT);
    check_u32("plan_full_top_step3_effect", top_steps[3].effect_flags,
              PLAN_TOP_EFFECT_WRITES_STORE_OR_ARTIFACT);
    check_int("plan_full_top_step5_kind", top_steps[5].kind, PLAN_TOP_EXTRACTION_DISPATCH);
    check_int("plan_full_top_step5_nested", top_steps[5].nested_plan_kind,
              PLAN_PARALLEL_EXTRACTION);
    check_u64("plan_full_top_step5_requires", top_steps[5].requires_mask,
              plan_top_step_bit(PLAN_TOP_MACRO_EXTRACTION) |
                  plan_top_step_bit(PLAN_TOP_USERCONFIG_LOAD) |
                  plan_top_step_bit(PLAN_TOP_DISCOVER) |
                  plan_top_step_bit(PLAN_TOP_TRY_INCREMENTAL_OR_DELETE_DB) |
                  plan_top_step_bit(PLAN_TOP_STRUCTURE));
    check_u32("plan_full_top_step5_effect", top_steps[5].effect_flags,
              PLAN_TOP_EFFECT_MUTATES_GRAPH);
    check_int("plan_full_top_step7_kind", top_steps[7].kind, PLAN_TOP_GITHISTORY);
    check_u32("plan_full_top_step7_gate", top_steps[7].gate_flags, PLAN_TOP_GATE_SKIP_FAST);
    check_int("plan_full_top_step8_kind", top_steps[8].kind, PLAN_TOP_PREDUMP);
    check_int("plan_full_top_step8_nested", top_steps[8].nested_plan_kind, PLAN_PREDUMP);
    check_u64("plan_full_top_step8_requires", top_steps[8].requires_mask,
              plan_top_step_bit(PLAN_TOP_MACRO_EXTRACTION) |
                  plan_top_step_bit(PLAN_TOP_USERCONFIG_LOAD) |
                  plan_top_step_bit(PLAN_TOP_DISCOVER) |
                  plan_top_step_bit(PLAN_TOP_TRY_INCREMENTAL_OR_DELETE_DB) |
                  plan_top_step_bit(PLAN_TOP_STRUCTURE) |
                  plan_top_step_bit(PLAN_TOP_EXTRACTION_DISPATCH) |
                  plan_top_step_bit(PLAN_TOP_TESTS) | plan_top_step_bit(PLAN_TOP_GITHISTORY));
    check_int("plan_full_top_step9_kind", top_steps[9].kind, PLAN_TOP_DUMP);
    check_u32("plan_full_top_step9_effect", top_steps[9].effect_flags,
              PLAN_TOP_EFFECT_WRITES_STORE_OR_ARTIFACT);
    check_int("plan_full_top_step10_kind", top_steps[10].kind, PLAN_TOP_PERSIST_HASHES);
    check_int("plan_full_top_step10_policy", top_steps[10].policy, PLAN_TOP_POLICY_BEST_EFFORT);
    check_int("plan_full_top_step11_kind", top_steps[11].kind, PLAN_TOP_ARTIFACT_EXPORT);
    check_int("plan_full_top_step11_policy", top_steps[11].policy,
              PLAN_TOP_POLICY_OPTIONAL_PERSISTENCE);

    check_int("plan_full_top_count_fast", cbm_rs_pipeline_full_plan_step_count_v1(MODE_FAST, 2, 51),
              11);
    check_int(
        "plan_full_top_steps_fast_len",
        cbm_rs_pipeline_full_plan_steps_v1(MODE_FAST, 2, 51, top_steps, CBM_RS_PLAN_TOP_MAX_STEPS),
        11);
    check_int("plan_full_top_fast_no_githistory_slot", top_steps[7].kind, PLAN_TOP_PREDUMP);
    check_u64(
        "plan_full_top_fast_predump_requires", top_steps[7].requires_mask,
        plan_top_step_bit(PLAN_TOP_MACRO_EXTRACTION) | plan_top_step_bit(PLAN_TOP_USERCONFIG_LOAD) |
            plan_top_step_bit(PLAN_TOP_DISCOVER) |
            plan_top_step_bit(PLAN_TOP_TRY_INCREMENTAL_OR_DELETE_DB) |
            plan_top_step_bit(PLAN_TOP_STRUCTURE) |
            plan_top_step_bit(PLAN_TOP_EXTRACTION_DISPATCH) | plan_top_step_bit(PLAN_TOP_TESTS));
    check_int(
        "plan_full_top_steps_sequential_len",
        cbm_rs_pipeline_full_plan_steps_v1(MODE_FULL, 1, 99, top_steps, CBM_RS_PLAN_TOP_MAX_STEPS),
        CBM_RS_PLAN_TOP_MAX_STEPS);
    check_int("plan_full_top_step5_nested_seq", top_steps[5].nested_plan_kind, PLAN_SEQUENTIAL);
    check_int("plan_full_top_steps_null",
              cbm_rs_pipeline_full_plan_steps_v1(MODE_FULL, 2, 51, NULL, CBM_RS_PLAN_TOP_MAX_STEPS),
              -1);
    check_int("plan_full_top_steps_short",
              cbm_rs_pipeline_full_plan_steps_v1(MODE_FULL, 2, 51, short_top_steps,
                                                 CBM_RS_PLAN_TOP_MAX_STEPS - 1),
              -1);
    check_int("plan_full_top_steps_negative",
              cbm_rs_pipeline_full_plan_steps_v1(MODE_FULL, 2, 51, top_steps, -1), -1);

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

#define CBM_RS_STORE_SCHEMA_KIND_CORE_TABLE 1
#define CBM_RS_STORE_SCHEMA_KIND_FTS_TABLE 2
#define CBM_RS_STORE_SCHEMA_KIND_USER_INDEX 3
#define CBM_RS_STORE_SCHEMA_KIND_GENERATED_COLUMN 4
#define CBM_RS_STORE_SCHEMA_KIND_TABLE_COLUMN 5
#define CBM_RS_STORE_SCHEMA_KIND_FTS_SHADOW_TABLE 6

#define CBM_RS_STORE_SCHEMA_FLAG_REQUIRED (UINT32_C(1) << 0)
#define CBM_RS_STORE_SCHEMA_FLAG_DROPPABLE_USER_INDEX (UINT32_C(1) << 1)
#define CBM_RS_STORE_SCHEMA_FLAG_GENERATED (UINT32_C(1) << 2)
#define CBM_RS_STORE_SCHEMA_FLAG_FTS (UINT32_C(1) << 3)
#define CBM_RS_STORE_SCHEMA_FLAG_NOT_NULL (UINT32_C(1) << 4)
#define CBM_RS_STORE_SCHEMA_FLAG_PRIMARY_KEY (UINT32_C(1) << 5)
#define CBM_RS_STORE_SCHEMA_FLAG_UNIQUE (UINT32_C(1) << 6)
#define CBM_RS_STORE_SCHEMA_FLAG_AUTOINCREMENT (UINT32_C(1) << 7)
#define CBM_RS_STORE_SCHEMA_FLAG_FK_CASCADE (UINT32_C(1) << 8)

#define CBM_RS_STORE_CONN_KIND_OPEN_MODE 1
#define CBM_RS_STORE_CONN_KIND_PRAGMA 2
#define CBM_RS_STORE_CONN_KIND_READ_PROBE 3
#define CBM_RS_STORE_CONN_KIND_FALLBACK 4

#define CBM_RS_STORE_CONN_MODE_MEMORY (UINT32_C(1) << 0)
#define CBM_RS_STORE_CONN_MODE_WRITE (UINT32_C(1) << 1)
#define CBM_RS_STORE_CONN_MODE_READ_ONLY (UINT32_C(1) << 2)
#define CBM_RS_STORE_CONN_MODE_BULK_BEGIN (UINT32_C(1) << 3)
#define CBM_RS_STORE_CONN_MODE_BULK_END (UINT32_C(1) << 4)
#define CBM_RS_STORE_CONN_MODE_CHECKPOINT (UINT32_C(1) << 5)

#define CBM_RS_STORE_CONN_FLAG_REQUIRED (UINT32_C(1) << 0)
#define CBM_RS_STORE_CONN_FLAG_NO_CREATE (UINT32_C(1) << 1)
#define CBM_RS_STORE_CONN_FLAG_READ_ONLY (UINT32_C(1) << 2)
#define CBM_RS_STORE_CONN_FLAG_URI (UINT32_C(1) << 3)
#define CBM_RS_STORE_CONN_FLAG_WRITES_DB_OR_WAL (UINT32_C(1) << 4)
#define CBM_RS_STORE_CONN_FLAG_BEST_EFFORT (UINT32_C(1) << 5)
#define CBM_RS_STORE_CONN_FLAG_ENV_BACKED (UINT32_C(1) << 6)

static const CbmRsStoreSchemaManifestEntryV1 *find_store_schema_entry(
    const CbmRsStoreSchemaManifestEntryV1 *entries, int count, int kind, const char *name) {
    for (int i = 0; i < count; i++) {
        if (entries[i].kind == kind && entries[i].name && strcmp(entries[i].name, name) == 0) {
            return &entries[i];
        }
    }
    return NULL;
}

static void check_store_schema_entry(const CbmRsStoreSchemaManifestEntryV1 *entries, int count,
                                     int kind, const char *name, const char *table_name,
                                     const char *column_name, uint32_t flags) {
    const CbmRsStoreSchemaManifestEntryV1 *entry =
        find_store_schema_entry(entries, count, kind, name);
    if (!entry) {
        fprintf(stderr, "store_schema_missing: kind=%d name=%s\n", kind, name);
        exit(1);
    }
    check_str("store_schema_table", entry->table_name, table_name);
    check_str("store_schema_column", entry->column_name, column_name);
    check_u32("store_schema_flags", entry->flags, flags);
}

static const CbmRsStoreSchemaManifestEntryV2 *find_store_schema_entry_v2(
    const CbmRsStoreSchemaManifestEntryV2 *entries, int count, int kind, const char *name) {
    for (int i = 0; i < count; i++) {
        if (entries[i].kind == kind && entries[i].name && strcmp(entries[i].name, name) == 0) {
            return &entries[i];
        }
    }
    return NULL;
}

static void check_store_schema_entry_v2(const CbmRsStoreSchemaManifestEntryV2 *entries, int count,
                                        int kind, const char *name, const char *table_name,
                                        const char *column_name, const char *column_type,
                                        const char *default_sql, const char *columns_csv,
                                        const char *sql_fragment, int ordinal, int hidden,
                                        uint32_t flags) {
    const CbmRsStoreSchemaManifestEntryV2 *entry =
        find_store_schema_entry_v2(entries, count, kind, name);
    if (!entry) {
        fprintf(stderr, "store_schema_v2_missing: kind=%d name=%s\n", kind, name);
        exit(1);
    }
    check_str("store_schema_v2_table", entry->table_name, table_name);
    check_str("store_schema_v2_column", entry->column_name, column_name);
    check_str("store_schema_v2_type", entry->column_type, column_type);
    check_str("store_schema_v2_default", entry->default_sql, default_sql);
    check_str("store_schema_v2_columns", entry->columns_csv, columns_csv);
    check_str("store_schema_v2_sql", entry->sql_fragment, sql_fragment);
    check_int("store_schema_v2_ordinal", entry->ordinal, ordinal);
    check_int("store_schema_v2_hidden", entry->hidden, hidden);
    check_u32("store_schema_v2_flags", entry->flags, flags);
}

static const CbmRsStoreConnectionManifestEntryV1 *find_store_connection_entry(
    const CbmRsStoreConnectionManifestEntryV1 *entries, int count, int kind, const char *name) {
    for (int i = 0; i < count; i++) {
        if (entries[i].kind == kind && entries[i].name && strcmp(entries[i].name, name) == 0) {
            return &entries[i];
        }
    }
    return NULL;
}

static void check_store_connection_entry(const CbmRsStoreConnectionManifestEntryV1 *entries,
                                         int count, int kind, const char *name, uint32_t flags,
                                         uint32_t mode_mask, int ordinal, int64_t value_i64,
                                         const char *sql, const char *env_name) {
    const CbmRsStoreConnectionManifestEntryV1 *entry =
        find_store_connection_entry(entries, count, kind, name);
    if (!entry) {
        fprintf(stderr, "store_connection_missing: kind=%d name=%s\n", kind, name);
        exit(1);
    }
    check_u32("store_connection_flags", entry->flags, flags);
    check_u32("store_connection_mode", entry->mode_mask, mode_mask);
    check_int("store_connection_ordinal", entry->ordinal, ordinal);
    check_u64("store_connection_value", (uint64_t)entry->value_i64, (uint64_t)value_i64);
    check_str("store_connection_sql", entry->sql, sql);
    check_str("store_connection_env", entry->env_name, env_name);
}

static void test_store_camel_split_exports(void) {
    char buf[4096];
    char short_buf[8];

    check_size("store_camel_null_len", cbm_rs_store_camel_split_v1(buf, sizeof(buf), NULL), 0);
    check_str("store_camel_null", buf, "");
    check_size("store_camel_empty_len", cbm_rs_store_camel_split_v1(buf, sizeof(buf), ""), 0);
    check_str("store_camel_empty", buf, "");

    const char *camel_expected = "updateCloudClient update Cloud Client";
    check_size("store_camel_update_len",
               cbm_rs_store_camel_split_v1(buf, sizeof(buf), "updateCloudClient"),
               strlen(camel_expected));
    check_str("store_camel_update", buf, camel_expected);
    check_size("store_camel_xml_len", cbm_rs_store_camel_split_v1(buf, sizeof(buf), "XMLParser"),
               strlen("XMLParser XML Parser"));
    check_str("store_camel_xml", buf, "XMLParser XML Parser");
    check_size("store_camel_snake_len", cbm_rs_store_camel_split_v1(buf, sizeof(buf), "snake_case"),
               strlen("snake_case snake_case"));
    check_str("store_camel_snake", buf, "snake_case snake_case");

    const char non_ascii[] = "caf\303\251HTTP";
    const char non_ascii_expected[] = "caf\303\251HTTP caf\303\251HTTP";
    check_size("store_camel_non_ascii_len",
               cbm_rs_store_camel_split_v1(buf, sizeof(buf), non_ascii),
               strlen(non_ascii_expected));
    check_str("store_camel_non_ascii", buf, non_ascii_expected);

    memset(buf, 0, sizeof(buf));
    memset(short_buf, 0, sizeof(short_buf));
    check_size("store_camel_short_len",
               cbm_rs_store_camel_split_v1(short_buf, sizeof(short_buf), "updateCloud"),
               strlen("updateCloud update Cloud"));
    check_str("store_camel_short_value", short_buf, "updateC");

    char fallback[2048];
    memset(fallback, 'a', sizeof(fallback) - 1);
    fallback[sizeof(fallback) - 1] = '\0';
    check_size("store_camel_fallback_len", cbm_rs_store_camel_split_v1(buf, sizeof(buf), fallback),
               sizeof(fallback) - 1);
    check_size("store_camel_fallback_cmp", (size_t)memcmp(buf, fallback, sizeof(fallback)), 0);

    char near_limit[2047];
    memset(near_limit, 'a', sizeof(near_limit) - 1);
    near_limit[sizeof(near_limit) - 1] = '\0';
    check_size("store_camel_near_limit_len",
               cbm_rs_store_camel_split_v1(buf, sizeof(buf), near_limit), 2047);
    check_int("store_camel_near_limit_space", buf[sizeof(near_limit) - 1], ' ');
    check_int("store_camel_near_limit_nul", buf[2047], '\0');
}

static void test_store_immutable_uri_exports(void) {
    char buf[256];
    char small[16];

    const char *posix_expected = "file:///tmp/codebase%20memory/graph.db?immutable=1";
    check_size(
        "store_immutable_posix_len",
        cbm_rs_store_build_immutable_uri_v1(buf, sizeof(buf), "/tmp/codebase memory/graph.db"),
        strlen(posix_expected));
    check_str("store_immutable_posix", buf, posix_expected);

    check_size("store_immutable_relative_len",
               cbm_rs_store_build_immutable_uri_v1(buf, sizeof(buf), "tmp/a.db"),
               strlen("file:///tmp/a.db?immutable=1"));
    check_str("store_immutable_relative", buf, "file:///tmp/a.db?immutable=1");

    check_size("store_immutable_root_len",
               cbm_rs_store_build_immutable_uri_v1(buf, sizeof(buf), "/"),
               strlen("file:///?immutable=1"));
    check_str("store_immutable_root", buf, "file:///?immutable=1");

    const char *windows_expected = "file:///C:/Users/dev/graph.db?immutable=1";
    check_size("store_immutable_windows_len",
               cbm_rs_store_build_immutable_uri_v1(buf, sizeof(buf), "C:\\Users\\dev\\graph.db"),
               strlen(windows_expected));
    check_str("store_immutable_windows", buf, windows_expected);
    check_size("store_immutable_windows_forward_len",
               cbm_rs_store_build_immutable_uri_v1(buf, sizeof(buf), "C:/Users/dev/graph.db"),
               strlen(windows_expected));
    check_str("store_immutable_windows_forward", buf, windows_expected);

    const char *unc_expected = "file://///server/share/db.sqlite?immutable=1";
    check_size(
        "store_immutable_unc_len",
        cbm_rs_store_build_immutable_uri_v1(buf, sizeof(buf), "\\\\server\\share\\db.sqlite"),
        strlen(unc_expected));
    check_str("store_immutable_unc", buf, unc_expected);

    const char *safe_expected = "file:///repo/.safe-_:~?immutable=1";
    check_size("store_immutable_safe_len",
               cbm_rs_store_build_immutable_uri_v1(buf, sizeof(buf), "repo/.safe-_:~"),
               strlen(safe_expected));
    check_str("store_immutable_safe", buf, safe_expected);

    const char *escaped_expected = "file:///tmp/a%3Fb%23c%26d%3De.db?immutable=1";
    check_size("store_immutable_escape_len",
               cbm_rs_store_build_immutable_uri_v1(buf, sizeof(buf), "/tmp/a?b#c&d=e.db"),
               strlen(escaped_expected));
    check_str("store_immutable_escape", buf, escaped_expected);

    check_size("store_immutable_percent_len",
               cbm_rs_store_build_immutable_uri_v1(buf, sizeof(buf), "/tmp/a%20b.db"),
               strlen("file:///tmp/a%2520b.db?immutable=1"));
    check_str("store_immutable_percent", buf, "file:///tmp/a%2520b.db?immutable=1");

    const char non_ascii[] = "/tmp/\xe5\xbc\x80.db";
    const char *non_ascii_expected = "file:///tmp/%E5%BC%80.db?immutable=1";
    check_size("store_immutable_non_ascii_len",
               cbm_rs_store_build_immutable_uri_v1(buf, sizeof(buf), non_ascii),
               strlen(non_ascii_expected));
    check_str("store_immutable_non_ascii", buf, non_ascii_expected);

    check_size("store_immutable_empty_len",
               cbm_rs_store_build_immutable_uri_v1(buf, sizeof(buf), ""),
               strlen("file:///?immutable=1"));
    check_str("store_immutable_empty", buf, "file:///?immutable=1");

    check_size("store_immutable_null", cbm_rs_store_build_immutable_uri_v1(buf, sizeof(buf), NULL),
               SIZE_MAX);
    check_size("store_immutable_length_only",
               cbm_rs_store_build_immutable_uri_v1(NULL, 0, "/tmp/a b.db"),
               strlen("file:///tmp/a%20b.db?immutable=1"));

    memset(small, 'Z', sizeof(small));
    check_size("store_immutable_short_len",
               cbm_rs_store_build_immutable_uri_v1(small, sizeof(small), "/tmp/a b.db"),
               strlen("file:///tmp/a%20b.db?immutable=1"));
    check_str("store_immutable_short_value", small, "file:///tmp/a%2");
}

static void test_store_search_pattern_exports(void) {
    char buf[512];
    char small[4];

    check_size("store_glob_null", cbm_rs_store_glob_to_like_v1(buf, sizeof(buf), NULL), SIZE_MAX);
    check_size("store_glob_len_only", cbm_rs_store_glob_to_like_v1(NULL, 0, "**/*.py"),
               strlen("%%.py"));
    check_size("store_glob_doublestar", cbm_rs_store_glob_to_like_v1(buf, sizeof(buf), "**/*.py"),
               strlen("%%.py"));
    check_str("store_glob_doublestar_value", buf, "%%.py");
    check_size("store_glob_empty", cbm_rs_store_glob_to_like_v1(buf, sizeof(buf), ""), 0);
    check_str("store_glob_empty_value", buf, "");
    check_size("store_glob_brackets",
               cbm_rs_store_glob_to_like_v1(buf, sizeof(buf), "src/[abc]/*.ts"),
               strlen("src/[abc]/%.ts"));
    check_str("store_glob_brackets_value", buf, "src/[abc]/%.ts");
    check_size("store_glob_question", cbm_rs_store_glob_to_like_v1(buf, sizeof(buf), "f???.txt"),
               strlen("f___.txt"));
    check_str("store_glob_question_value", buf, "f___.txt");
    memset(small, 'Z', sizeof(small));
    check_size("store_glob_short", cbm_rs_store_glob_to_like_v1(small, sizeof(small), "**/*.py"),
               strlen("%%.py"));
    check_str("store_glob_short_value", small, "%%.");

    check_size("store_case_ensure_null",
               cbm_rs_store_ensure_case_insensitive_v1(buf, sizeof(buf), NULL), 0);
    check_str("store_case_ensure_null_value", buf, "");
    check_size("store_case_ensure_len_only",
               cbm_rs_store_ensure_case_insensitive_v1(NULL, 0, "handler"), strlen("(?i)handler"));
    check_size("store_case_ensure_plain",
               cbm_rs_store_ensure_case_insensitive_v1(buf, sizeof(buf), "handler"),
               strlen("(?i)handler"));
    check_str("store_case_ensure_plain_value", buf, "(?i)handler");
    check_size("store_case_ensure_prefixed",
               cbm_rs_store_ensure_case_insensitive_v1(buf, sizeof(buf), "(?i)handler"),
               strlen("(?i)handler"));
    check_str("store_case_ensure_prefixed_value", buf, "(?i)handler");
    check_size("store_case_ensure_empty",
               cbm_rs_store_ensure_case_insensitive_v1(buf, sizeof(buf), ""), strlen("(?i)"));
    check_str("store_case_ensure_empty_value", buf, "(?i)");
    memset(small, 'Z', sizeof(small));
    check_size("store_case_ensure_short",
               cbm_rs_store_ensure_case_insensitive_v1(small, sizeof(small), "handler"),
               strlen("(?i)handler"));
    check_str("store_case_ensure_short_value", small, "(?i");

    check_size("store_case_strip_null", cbm_rs_store_strip_case_flag_v1(buf, sizeof(buf), NULL), 0);
    check_str("store_case_strip_null_value", buf, "");
    check_size("store_case_strip_prefixed",
               cbm_rs_store_strip_case_flag_v1(buf, sizeof(buf), "(?i)handler"), strlen("handler"));
    check_str("store_case_strip_prefixed_value", buf, "handler");
    check_size("store_case_strip_plain",
               cbm_rs_store_strip_case_flag_v1(buf, sizeof(buf), "handler"), strlen("handler"));
    check_str("store_case_strip_plain_value", buf, "handler");
    check_size("store_case_strip_double",
               cbm_rs_store_strip_case_flag_v1(buf, sizeof(buf), "(?i)(?i)double"),
               strlen("(?i)double"));
    check_str("store_case_strip_double_value", buf, "(?i)double");

    check_int("store_like_null_count", cbm_rs_store_like_hint_count_v1(NULL, 16), 0);
    check_int("store_like_zero_count", cbm_rs_store_like_hint_count_v1(".*handler.*", 0), 0);
    check_int("store_like_negative_count", cbm_rs_store_like_hint_count_v1(".*handler.*", -1), 0);
    check_int("store_like_alt_count", cbm_rs_store_like_hint_count_v1(".*foo|.*bar", 16), 0);
    check_int("store_like_escaped_pipe_count", cbm_rs_store_like_hint_count_v1("foo\\|bar", 16), 0);
    check_int("store_like_handler_count", cbm_rs_store_like_hint_count_v1(".*handler.*", 16), 1);
    check_size("store_like_handler_len", cbm_rs_store_like_hint_v1(NULL, 0, ".*handler.*", 16, 0),
               strlen("handler"));
    check_size("store_like_handler_value_len",
               cbm_rs_store_like_hint_v1(buf, sizeof(buf), ".*handler.*", 16, 0),
               strlen("handler"));
    check_str("store_like_handler_value", buf, "handler");
    memset(small, 'Z', sizeof(small));
    check_size("store_like_short_len",
               cbm_rs_store_like_hint_v1(small, sizeof(small), ".*handler.*", 16, 0),
               strlen("handler"));
    check_str("store_like_short_value", small, "han");
    check_size("store_like_missing_index",
               cbm_rs_store_like_hint_v1(buf, sizeof(buf), ".*handler.*", 16, 1), SIZE_MAX);
    check_size("store_like_negative_index",
               cbm_rs_store_like_hint_v1(buf, sizeof(buf), ".*handler.*", 16, -1), SIZE_MAX);

    check_int("store_like_multi_count", cbm_rs_store_like_hint_count_v1(".*Order.*Handler.*", 16),
              2);
    check_size("store_like_multi_first",
               cbm_rs_store_like_hint_v1(buf, sizeof(buf), ".*Order.*Handler.*", 16, 0),
               strlen("Order"));
    check_str("store_like_multi_first_value", buf, "Order");
    check_size("store_like_multi_second",
               cbm_rs_store_like_hint_v1(buf, sizeof(buf), ".*Order.*Handler.*", 16, 1),
               strlen("Handler"));
    check_str("store_like_multi_second_value", buf, "Handler");

    check_int("store_like_escaped_dot_count", cbm_rs_store_like_hint_count_v1("foo\\.bar", 16), 1);
    check_size("store_like_escaped_dot_len",
               cbm_rs_store_like_hint_v1(buf, sizeof(buf), "foo\\.bar", 16, 0), strlen("foo.bar"));
    check_str("store_like_escaped_dot_value", buf, "foo.bar");
    check_int("store_like_escaped_meta_count",
              cbm_rs_store_like_hint_count_v1("foo\\*bar\\?baz", 16), 1);
    check_size("store_like_escaped_meta_len",
               cbm_rs_store_like_hint_v1(buf, sizeof(buf), "foo\\*bar\\?baz", 16, 0),
               strlen("foo*bar?baz"));
    check_str("store_like_escaped_meta_value", buf, "foo*bar?baz");

    check_int("store_like_max_count", cbm_rs_store_like_hint_count_v1(".*aaa.*bbb.*ccc.*ddd.*", 2),
              2);
    check_size("store_like_max_missing",
               cbm_rs_store_like_hint_v1(buf, sizeof(buf), ".*aaa.*bbb.*ccc.*ddd.*", 2, 2),
               SIZE_MAX);

    char long_pattern[301];
    memset(long_pattern, 'a', sizeof(long_pattern) - 1);
    long_pattern[sizeof(long_pattern) - 1] = '\0';
    check_int("store_like_long_count", cbm_rs_store_like_hint_count_v1(long_pattern, 16), 1);
    check_size("store_like_long_len",
               cbm_rs_store_like_hint_v1(buf, sizeof(buf), long_pattern, 16, 0), 255);
    check_size("store_like_long_strlen", strlen(buf), 255);
    for (size_t i = 0; i < 255; i++) {
        if (buf[i] != 'a') {
            fail("store_like_long_byte", "unexpected byte");
        }
    }
    check_int("store_like_long_nul", buf[255], '\0');
}

static void test_store_file_ext_lower_exports(void) {
    char buf[16];
    check_size("store_ext_py", cbm_rs_store_file_ext_lower_v1(buf, sizeof(buf), "main.py"),
               strlen(".py"));
    check_str("store_ext_py_val", buf, ".py");
    check_size("store_ext_upper", cbm_rs_store_file_ext_lower_v1(buf, sizeof(buf), "App.JAVA"),
               strlen(".java"));
    check_str("store_ext_upper_val", buf, ".java");
    check_size("store_ext_last_dot", cbm_rs_store_file_ext_lower_v1(buf, sizeof(buf), "a.TAR.GZ"),
               strlen(".gz"));
    check_str("store_ext_last_dot_val", buf, ".gz");
    check_size("store_ext_dotfile", cbm_rs_store_file_ext_lower_v1(buf, sizeof(buf), ".gitignore"),
               strlen(".gitignore"));
    check_str("store_ext_dotfile_val", buf, ".gitignore");
    check_size("store_ext_trailing_dot", cbm_rs_store_file_ext_lower_v1(buf, sizeof(buf), "noext."),
               strlen("."));
    check_str("store_ext_trailing_dot_val", buf, ".");
    /* 無 . / null / null out / bufsize==0 → SIZE_MAX */
    check_size("store_ext_none", cbm_rs_store_file_ext_lower_v1(buf, sizeof(buf), "Makefile"),
               SIZE_MAX);
    check_size("store_ext_null", cbm_rs_store_file_ext_lower_v1(buf, sizeof(buf), NULL), SIZE_MAX);
    check_size("store_ext_null_out", cbm_rs_store_file_ext_lower_v1(NULL, sizeof(buf), "a.py"),
               SIZE_MAX);
    check_size("store_ext_zero_sz", cbm_rs_store_file_ext_lower_v1(buf, 0, "a.py"), SIZE_MAX);
    /* 副檔名長度 >= bufsize 拒絕（對齊 C len >= sizeof(buf)）*/
    check_size("store_ext_too_long",
               cbm_rs_store_file_ext_lower_v1(buf, sizeof(buf), "a.abcdefghijklmno"), SIZE_MAX);
    check_size("store_ext_fits",
               cbm_rs_store_file_ext_lower_v1(buf, sizeof(buf), "a.abcdefghijklmn"),
               strlen(".abcdefghijklmn"));
    check_str("store_ext_fits_val", buf, ".abcdefghijklmn");
}

static void test_store_ext_lang_kind_exports(void) {
    check_int("store_ext_lang_py", cbm_rs_store_ext_lang_kind_v1(".py"), 0);
    check_int("store_ext_lang_cpp", cbm_rs_store_ext_lang_kind_v1(".cpp"), 8);
    check_int("store_ext_lang_cc", cbm_rs_store_ext_lang_kind_v1(".cc"), 9);
    check_int("store_ext_lang_tsx", cbm_rs_store_ext_lang_kind_v1(".tsx"), 5);
    check_int("store_ext_lang_yaml", cbm_rs_store_ext_lang_kind_v1(".yaml"), 29);
    check_int("store_ext_lang_yml", cbm_rs_store_ext_lang_kind_v1(".yml"), 30);
    check_int("store_ext_lang_svelte", cbm_rs_store_ext_lang_kind_v1(".svelte"), 43);
    check_int("store_ext_lang_unknown", cbm_rs_store_ext_lang_kind_v1(".txt"), -1);
    check_int("store_ext_lang_upper", cbm_rs_store_ext_lang_kind_v1(".PY"), -1);
    check_int("store_ext_lang_null", cbm_rs_store_ext_lang_kind_v1(NULL), -1);
}

static void test_store_arch_path_scope_exports(void) {
    char buf[512];

    check_size("store_arch_path_basic",
               cbm_rs_store_normalize_arch_path_v1(buf, sizeof(buf), "apps/foo"),
               strlen("apps/foo"));
    check_str("store_arch_path_basic_val", buf, "apps/foo");

    check_size("store_arch_path_dotslash",
               cbm_rs_store_normalize_arch_path_v1(buf, sizeof(buf), "./apps/foo"),
               strlen("apps/foo"));
    check_str("store_arch_path_dotslash_val", buf, "apps/foo");

    check_size("store_arch_path_leadslash",
               cbm_rs_store_normalize_arch_path_v1(buf, sizeof(buf), "/apps/foo"),
               strlen("apps/foo"));
    check_str("store_arch_path_leadslash_val", buf, "apps/foo");

    check_size("store_arch_path_ws",
               cbm_rs_store_normalize_arch_path_v1(buf, sizeof(buf), "  \tapps/foo"),
               strlen("apps/foo"));
    check_str("store_arch_path_ws_val", buf, "apps/foo");

    check_size("store_arch_path_trailing",
               cbm_rs_store_normalize_arch_path_v1(buf, sizeof(buf), "apps/foo/ "),
               strlen("apps/foo"));
    check_str("store_arch_path_trailing_val", buf, "apps/foo");

    check_size("store_arch_path_collapse",
               cbm_rs_store_normalize_arch_path_v1(buf, sizeof(buf), "apps///foo//bar"),
               strlen("apps/foo/bar"));
    check_str("store_arch_path_collapse_val", buf, "apps/foo/bar");

    /* 未設定 / 正規化後為空 → SIZE_MAX（對應 C arch_path_prepare 回傳 false）*/
    check_size("store_arch_path_null", cbm_rs_store_normalize_arch_path_v1(buf, sizeof(buf), NULL),
               SIZE_MAX);
    check_size("store_arch_path_empty", cbm_rs_store_normalize_arch_path_v1(buf, sizeof(buf), ""),
               SIZE_MAX);
    check_size("store_arch_path_ws_only",
               cbm_rs_store_normalize_arch_path_v1(buf, sizeof(buf), "   "), SIZE_MAX);
    check_size("store_arch_path_dotslash_only",
               cbm_rs_store_normalize_arch_path_v1(buf, sizeof(buf), "./"), SIZE_MAX);
    check_size("store_arch_path_slashes_only",
               cbm_rs_store_normalize_arch_path_v1(buf, sizeof(buf), "///"), SIZE_MAX);

    /* norm_sz==0 / NULL out → SIZE_MAX */
    check_size("store_arch_path_zero_sz", cbm_rs_store_normalize_arch_path_v1(buf, 0, "apps/foo"),
               SIZE_MAX);
    check_size("store_arch_path_null_out",
               cbm_rs_store_normalize_arch_path_v1(NULL, sizeof(buf), "apps/foo"), SIZE_MAX);

    /* 截斷先於 strip/collapse（對齊 C strncpy）*/
    char small[5];
    memset(small, 'Z', sizeof(small));
    check_size("store_arch_path_trunc",
               cbm_rs_store_normalize_arch_path_v1(small, sizeof(small), "abcdef"), strlen("abcd"));
    check_str("store_arch_path_trunc_val", small, "abcd");
    memset(small, 'Z', sizeof(small));
    check_size("store_arch_path_trunc_slash",
               cbm_rs_store_normalize_arch_path_v1(small, sizeof(small), "abc/def"), strlen("abc"));
    check_str("store_arch_path_trunc_slash_val", small, "abc");
}

static void test_store_arch_helper_exports(void) {
    char buf[512];
    char small[4];
    char marker[4] = "ABC";
    char top_short[4] = "ZZZ";

    check_size("store_qn_pkg_null", cbm_rs_store_qn_to_package_v1(buf, sizeof(buf), NULL), 0);
    check_str("store_qn_pkg_null_value", buf, "");
    check_size("store_qn_pkg_empty", cbm_rs_store_qn_to_package_v1(buf, sizeof(buf), ""), 0);
    check_str("store_qn_pkg_empty_value", buf, "");
    check_size("store_qn_pkg_no_dot", cbm_rs_store_qn_to_package_v1(buf, sizeof(buf), "standalone"),
               0);
    check_str("store_qn_pkg_no_dot_value", buf, "");
    check_size("store_qn_pkg_two", cbm_rs_store_qn_to_package_v1(buf, sizeof(buf), "project.cmd"),
               strlen("cmd"));
    check_str("store_qn_pkg_two_value", buf, "cmd");
    check_size("store_qn_pkg_three",
               cbm_rs_store_qn_to_package_v1(buf, sizeof(buf), "project.main.foo"), strlen("main"));
    check_str("store_qn_pkg_three_value", buf, "main");
    check_size(
        "store_qn_pkg_many",
        cbm_rs_store_qn_to_package_v1(buf, sizeof(buf), "project.internal.store.search.Search"),
        strlen("store"));
    check_str("store_qn_pkg_many_value", buf, "store");
    check_size("store_qn_pkg_len_only",
               cbm_rs_store_qn_to_package_v1(NULL, 0, "project.internal.store.search.Search"),
               strlen("store"));
    check_size("store_qn_pkg_buf_zero",
               cbm_rs_store_qn_to_package_v1(marker, 0, "project.internal.store.search.Search"),
               strlen("store"));
    check_str("store_qn_pkg_buf_zero_value", marker, "ABC");
    check_size("store_qn_pkg_consecutive",
               cbm_rs_store_qn_to_package_v1(buf, sizeof(buf), "project.dir..Func"), strlen("dir"));
    check_str("store_qn_pkg_consecutive_value", buf, "dir");
    memset(small, 'Z', sizeof(small));
    check_size(
        "store_qn_pkg_short",
        cbm_rs_store_qn_to_package_v1(small, sizeof(small), "project.internal.store.search.Search"),
        strlen("store"));
    check_str("store_qn_pkg_short_value", small, "sto");

    char ok_segment[256];
    memset(ok_segment, 'a', sizeof(ok_segment) - 1);
    ok_segment[sizeof(ok_segment) - 1] = '\0';
    char qn[320];
    snprintf(qn, sizeof(qn), "project.dir.%s.Func", ok_segment);
    check_size("store_qn_pkg_255_len", cbm_rs_store_qn_to_package_v1(buf, sizeof(buf), qn), 255);
    check_size("store_qn_pkg_255_strlen", strlen(buf), 255);
    for (size_t i = 0; i < 255; i++) {
        if (buf[i] != 'a') {
            fail("store_qn_pkg_255_byte", "unexpected byte");
        }
    }
    check_int("store_qn_pkg_255_nul", buf[255], '\0');

    char too_long_segment[257];
    memset(too_long_segment, 'b', sizeof(too_long_segment) - 1);
    too_long_segment[sizeof(too_long_segment) - 1] = '\0';
    snprintf(qn, sizeof(qn), "project.dir.%s.Func", too_long_segment);
    check_size("store_qn_pkg_256_fallback_len", cbm_rs_store_qn_to_package_v1(buf, sizeof(buf), qn),
               strlen("dir"));
    check_str("store_qn_pkg_256_fallback_value", buf, "dir");

    check_size("store_qn_top_null", cbm_rs_store_qn_to_top_package_v1(buf, sizeof(buf), NULL), 0);
    check_str("store_qn_top_null_value", buf, "");
    check_size("store_qn_top_no_dot",
               cbm_rs_store_qn_to_top_package_v1(buf, sizeof(buf), "standalone"), 0);
    check_str("store_qn_top_no_dot_value", buf, "");
    check_size("store_qn_top_two",
               cbm_rs_store_qn_to_top_package_v1(buf, sizeof(buf), "project.cmd"), strlen("cmd"));
    check_str("store_qn_top_two_value", buf, "cmd");
    check_size(
        "store_qn_top_many",
        cbm_rs_store_qn_to_top_package_v1(buf, sizeof(buf), "project.internal.store.search.Search"),
        strlen("internal"));
    check_str("store_qn_top_many_value", buf, "internal");
    check_size("store_qn_top_len_only",
               cbm_rs_store_qn_to_top_package_v1(NULL, 0, "project.internal.store.search.Search"),
               strlen("internal"));
    check_size("store_qn_top_short",
               cbm_rs_store_qn_to_top_package_v1(top_short, sizeof(top_short),
                                                 "project.internal.store.search.Search"),
               strlen("internal"));
    check_str("store_qn_top_short_value", top_short, "int");
    snprintf(qn, sizeof(qn), "project.%s.Func", ok_segment);
    check_size("store_qn_top_255_len", cbm_rs_store_qn_to_top_package_v1(buf, sizeof(buf), qn),
               255);
    snprintf(qn, sizeof(qn), "project.%s.Func", too_long_segment);
    check_size("store_qn_top_256_len", cbm_rs_store_qn_to_top_package_v1(buf, sizeof(buf), qn), 0);
    check_str("store_qn_top_256_value", buf, "");

    check_int("store_test_file_null", cbm_rs_store_is_test_file_path_v1(NULL), 0);
    check_int("store_test_file_empty", cbm_rs_store_is_test_file_path_v1(""), 0);
    check_int("store_test_file_prefix", cbm_rs_store_is_test_file_path_v1("test_handler.py"), 1);
    check_int("store_test_file_dir", cbm_rs_store_is_test_file_path_v1("src/__tests__/handler.js"),
              1);
    check_int("store_test_file_testdata",
              cbm_rs_store_is_test_file_path_v1("testdata/fixture.json"), 1);
    check_int("store_test_file_substring", cbm_rs_store_is_test_file_path_v1("contest/file.c"), 1);
    check_int("store_test_file_upper", cbm_rs_store_is_test_file_path_v1("TestOnly.java"), 0);
    check_int("store_test_file_spec", cbm_rs_store_is_test_file_path_v1("handler.spec.ts"), 0);

    check_int("store_hop_risk_1", cbm_rs_store_hop_to_risk_v1(1), 0);
    check_int("store_hop_risk_2", cbm_rs_store_hop_to_risk_v1(2), 1);
    check_int("store_hop_risk_3", cbm_rs_store_hop_to_risk_v1(3), 2);
    check_int("store_hop_risk_0", cbm_rs_store_hop_to_risk_v1(0), 3);
    check_int("store_hop_risk_negative", cbm_rs_store_hop_to_risk_v1(-1), 3);
    check_size("store_risk_label_critical", cbm_rs_store_risk_label_v1(buf, sizeof(buf), 0),
               strlen("CRITICAL"));
    check_str("store_risk_label_critical_value", buf, "CRITICAL");
    check_size("store_risk_label_high", cbm_rs_store_risk_label_v1(buf, sizeof(buf), 1),
               strlen("HIGH"));
    check_str("store_risk_label_high_value", buf, "HIGH");
    check_size("store_risk_label_medium", cbm_rs_store_risk_label_v1(buf, sizeof(buf), 2),
               strlen("MEDIUM"));
    check_str("store_risk_label_medium_value", buf, "MEDIUM");
    check_size("store_risk_label_low", cbm_rs_store_risk_label_v1(buf, sizeof(buf), 3),
               strlen("LOW"));
    check_str("store_risk_label_low_value", buf, "LOW");
    check_size("store_risk_label_unknown", cbm_rs_store_risk_label_v1(buf, sizeof(buf), 99),
               strlen("LOW"));
    check_str("store_risk_label_unknown_value", buf, "LOW");
    memset(small, 'Z', sizeof(small));
    check_size("store_risk_label_short", cbm_rs_store_risk_label_v1(small, sizeof(small), 0),
               strlen("CRITICAL"));
    check_str("store_risk_label_short_value", small, "CRI");
}

static void test_store_mmap_resolver_exports(void) {
    check_i64("store_mmap_null", cbm_rs_store_resolve_mmap_size_value_v1(NULL), 67108864);
    check_i64("store_mmap_empty", cbm_rs_store_resolve_mmap_size_value_v1(""), 67108864);
    check_i64("store_mmap_garbage", cbm_rs_store_resolve_mmap_size_value_v1("not-a-number"),
              67108864);
    check_i64("store_mmap_partial", cbm_rs_store_resolve_mmap_size_value_v1("123abc"), 67108864);
    check_i64("store_mmap_trailing_space", cbm_rs_store_resolve_mmap_size_value_v1("4096 "),
              67108864);
    check_i64("store_mmap_overflow", cbm_rs_store_resolve_mmap_size_value_v1("9223372036854775808"),
              67108864);
    check_i64("store_mmap_zero", cbm_rs_store_resolve_mmap_size_value_v1("0"), 0);
    check_i64("store_mmap_negative", cbm_rs_store_resolve_mmap_size_value_v1("-1"), 0);
    check_i64("store_mmap_min_negative",
              cbm_rs_store_resolve_mmap_size_value_v1("-9223372036854775808"), 0);
    check_i64("store_mmap_explicit", cbm_rs_store_resolve_mmap_size_value_v1("1048576"), 1048576);
    check_i64("store_mmap_leading_space_plus", cbm_rs_store_resolve_mmap_size_value_v1(" \t+4096"),
              4096);
    check_i64("store_mmap_i64_max", cbm_rs_store_resolve_mmap_size_value_v1("9223372036854775807"),
              INT64_MAX);
}

static void test_store_schema_manifest_exports(void) {
    CbmRsStoreSchemaManifestEntryV1 entries[16];
    CbmRsStoreSchemaManifestEntryV1 short_entries[15];

    check_int("store_schema_entry_count", cbm_rs_store_schema_manifest_entry_count_v1(), 16);
    check_int("store_schema_user_index_count", cbm_rs_store_schema_manifest_user_index_count_v1(),
              9);
    check_int("store_schema_entries_len", cbm_rs_store_schema_manifest_entries_v1(entries, 16), 16);
    check_int("store_schema_entries_null", cbm_rs_store_schema_manifest_entries_v1(NULL, 16), -1);
    check_int("store_schema_entries_short",
              cbm_rs_store_schema_manifest_entries_v1(short_entries, 15), -1);
    check_int("store_schema_entries_negative", cbm_rs_store_schema_manifest_entries_v1(entries, -1),
              -1);

    const uint32_t required = CBM_RS_STORE_SCHEMA_FLAG_REQUIRED;
    check_store_schema_entry(entries, 16, CBM_RS_STORE_SCHEMA_KIND_CORE_TABLE, "projects", "", "",
                             required);
    check_store_schema_entry(entries, 16, CBM_RS_STORE_SCHEMA_KIND_CORE_TABLE, "file_hashes", "",
                             "", required);
    check_store_schema_entry(entries, 16, CBM_RS_STORE_SCHEMA_KIND_CORE_TABLE, "nodes", "", "",
                             required);
    check_store_schema_entry(entries, 16, CBM_RS_STORE_SCHEMA_KIND_CORE_TABLE, "edges", "", "",
                             required);
    check_store_schema_entry(entries, 16, CBM_RS_STORE_SCHEMA_KIND_CORE_TABLE, "project_summaries",
                             "", "", required);
    check_store_schema_entry(entries, 16, CBM_RS_STORE_SCHEMA_KIND_FTS_TABLE, "nodes_fts", "", "",
                             required | CBM_RS_STORE_SCHEMA_FLAG_FTS);
    check_store_schema_entry(entries, 16, CBM_RS_STORE_SCHEMA_KIND_GENERATED_COLUMN,
                             "edges.url_path_gen", "edges", "url_path_gen",
                             required | CBM_RS_STORE_SCHEMA_FLAG_GENERATED);

    const uint32_t user_index_flags = required | CBM_RS_STORE_SCHEMA_FLAG_DROPPABLE_USER_INDEX;
    check_store_schema_entry(entries, 16, CBM_RS_STORE_SCHEMA_KIND_USER_INDEX, "idx_nodes_label",
                             "nodes", "", user_index_flags);
    check_store_schema_entry(entries, 16, CBM_RS_STORE_SCHEMA_KIND_USER_INDEX, "idx_nodes_name",
                             "nodes", "", user_index_flags);
    check_store_schema_entry(entries, 16, CBM_RS_STORE_SCHEMA_KIND_USER_INDEX, "idx_nodes_file",
                             "nodes", "", user_index_flags);
    check_store_schema_entry(entries, 16, CBM_RS_STORE_SCHEMA_KIND_USER_INDEX, "idx_edges_source",
                             "edges", "", user_index_flags);
    check_store_schema_entry(entries, 16, CBM_RS_STORE_SCHEMA_KIND_USER_INDEX, "idx_edges_target",
                             "edges", "", user_index_flags);
    check_store_schema_entry(entries, 16, CBM_RS_STORE_SCHEMA_KIND_USER_INDEX, "idx_edges_type",
                             "edges", "", user_index_flags);
    check_store_schema_entry(entries, 16, CBM_RS_STORE_SCHEMA_KIND_USER_INDEX,
                             "idx_edges_target_type", "edges", "", user_index_flags);
    check_store_schema_entry(entries, 16, CBM_RS_STORE_SCHEMA_KIND_USER_INDEX,
                             "idx_edges_source_type", "edges", "", user_index_flags);
    check_store_schema_entry(entries, 16, CBM_RS_STORE_SCHEMA_KIND_USER_INDEX, "idx_edges_url_path",
                             "edges", "", user_index_flags);

    CbmRsStoreSchemaManifestEntryV2 entries_v2[48];
    CbmRsStoreSchemaManifestEntryV2 short_entries_v2[47];
    check_int("store_schema_entry_count_v2", cbm_rs_store_schema_manifest_entry_count_v2(), 48);
    check_int("store_schema_table_column_count_v2",
              cbm_rs_store_schema_manifest_table_column_count_v2(), 29);
    check_int("store_schema_fts_shadow_count_v2",
              cbm_rs_store_schema_manifest_fts_shadow_count_v2(), 4);
    check_int("store_schema_entries_len_v2",
              cbm_rs_store_schema_manifest_entries_v2(entries_v2, 48), 48);
    check_int("store_schema_entries_null_v2", cbm_rs_store_schema_manifest_entries_v2(NULL, 48),
              -1);
    check_int("store_schema_entries_short_v2",
              cbm_rs_store_schema_manifest_entries_v2(short_entries_v2, 47), -1);
    check_int("store_schema_entries_negative_v2",
              cbm_rs_store_schema_manifest_entries_v2(entries_v2, -1), -1);

    check_store_schema_entry_v2(entries_v2, 48, CBM_RS_STORE_SCHEMA_KIND_CORE_TABLE, "nodes", "",
                                "", "", "",
                                "id,project,label,name,qualified_name,file_path,"
                                "start_line,end_line,properties",
                                "UNIQUE(project,qualified_name);FOREIGN KEY(project) REFERENCES "
                                "projects(name) ON DELETE CASCADE",
                                -1, 0, required);
    check_store_schema_entry_v2(entries_v2, 48, CBM_RS_STORE_SCHEMA_KIND_TABLE_COLUMN, "nodes.id",
                                "nodes", "id", "INTEGER", "", "", "", 0, 0,
                                required | CBM_RS_STORE_SCHEMA_FLAG_PRIMARY_KEY |
                                    CBM_RS_STORE_SCHEMA_FLAG_AUTOINCREMENT);
    check_store_schema_entry_v2(
        entries_v2, 48, CBM_RS_STORE_SCHEMA_KIND_TABLE_COLUMN, "nodes.qualified_name", "nodes",
        "qualified_name", "TEXT", "", "", "", 4, 0,
        required | CBM_RS_STORE_SCHEMA_FLAG_NOT_NULL | CBM_RS_STORE_SCHEMA_FLAG_UNIQUE);
    check_store_schema_entry_v2(entries_v2, 48, CBM_RS_STORE_SCHEMA_KIND_TABLE_COLUMN,
                                "nodes.properties", "nodes", "properties", "TEXT", "'{}'", "", "",
                                8, 0, required);
    check_store_schema_entry_v2(entries_v2, 48, CBM_RS_STORE_SCHEMA_KIND_GENERATED_COLUMN,
                                "edges.url_path_gen", "edges", "url_path_gen", "TEXT", "", "",
                                "json_extract(properties,'$.url_path')", 6, 2,
                                required | CBM_RS_STORE_SCHEMA_FLAG_GENERATED);
    check_store_schema_entry_v2(entries_v2, 48, CBM_RS_STORE_SCHEMA_KIND_USER_INDEX,
                                "idx_edges_url_path", "edges", "", "", "", "project,url_path_gen",
                                "", -1, 0, user_index_flags);
    check_store_schema_entry_v2(entries_v2, 48, CBM_RS_STORE_SCHEMA_KIND_FTS_TABLE, "nodes_fts", "",
                                "", "", "", "name,qualified_name,label,file_path",
                                "content='';tokenize='unicode61 remove_diacritics 2'", -1, 0,
                                required | CBM_RS_STORE_SCHEMA_FLAG_FTS);
    check_store_schema_entry_v2(entries_v2, 48, CBM_RS_STORE_SCHEMA_KIND_FTS_SHADOW_TABLE,
                                "nodes_fts_idx", "nodes_fts", "", "", "", "", "", -1, 0,
                                required | CBM_RS_STORE_SCHEMA_FLAG_FTS);

    CbmRsStoreConnectionManifestEntryV1 conn_entries[18];
    CbmRsStoreConnectionManifestEntryV1 short_conn_entries[17];
    check_int("store_connection_entry_count", cbm_rs_store_connection_manifest_entry_count_v1(),
              18);
    check_int("store_connection_write_pragma_count",
              cbm_rs_store_connection_manifest_write_pragma_count_v1(), 5);
    check_int("store_connection_entries_len",
              cbm_rs_store_connection_manifest_entries_v1(conn_entries, 18), 18);
    check_int("store_connection_entries_null",
              cbm_rs_store_connection_manifest_entries_v1(NULL, 18), -1);
    check_int("store_connection_entries_short",
              cbm_rs_store_connection_manifest_entries_v1(short_conn_entries, 17), -1);
    check_int("store_connection_entries_negative",
              cbm_rs_store_connection_manifest_entries_v1(conn_entries, -1), -1);

    const uint32_t conn_required = CBM_RS_STORE_CONN_FLAG_REQUIRED;
    check_store_connection_entry(
        conn_entries, 18, CBM_RS_STORE_CONN_KIND_OPEN_MODE, "open.path_query",
        conn_required | CBM_RS_STORE_CONN_FLAG_READ_ONLY | CBM_RS_STORE_CONN_FLAG_NO_CREATE,
        CBM_RS_STORE_CONN_MODE_READ_ONLY, 0, 0, "SQLITE_OPEN_READONLY", "");
    check_store_connection_entry(
        conn_entries, 18, CBM_RS_STORE_CONN_KIND_READ_PROBE, "open.path_query.probe",
        conn_required | CBM_RS_STORE_CONN_FLAG_READ_ONLY, CBM_RS_STORE_CONN_MODE_READ_ONLY, 1, 0,
        "SELECT 1 FROM sqlite_master LIMIT 1;", "");
    check_store_connection_entry(conn_entries, 18, CBM_RS_STORE_CONN_KIND_FALLBACK,
                                 "open.path_query.immutable_fallback",
                                 conn_required | CBM_RS_STORE_CONN_FLAG_READ_ONLY |
                                     CBM_RS_STORE_CONN_FLAG_NO_CREATE | CBM_RS_STORE_CONN_FLAG_URI,
                                 CBM_RS_STORE_CONN_MODE_READ_ONLY, 2, 0,
                                 "SQLITE_OPEN_READONLY|SQLITE_OPEN_URI;immutable=1", "");
    check_store_connection_entry(
        conn_entries, 18, CBM_RS_STORE_CONN_KIND_PRAGMA, "pragma.journal_mode.wal",
        conn_required | CBM_RS_STORE_CONN_FLAG_WRITES_DB_OR_WAL, CBM_RS_STORE_CONN_MODE_WRITE, 40,
        0, "PRAGMA journal_mode = WAL;", "");
    check_store_connection_entry(
        conn_entries, 18, CBM_RS_STORE_CONN_KIND_PRAGMA, "pragma.wal_checkpoint.passive",
        conn_required | CBM_RS_STORE_CONN_FLAG_WRITES_DB_OR_WAL |
            CBM_RS_STORE_CONN_FLAG_BEST_EFFORT,
        CBM_RS_STORE_CONN_MODE_WRITE, 50, 0, "PRAGMA wal_checkpoint(PASSIVE)", "");
    check_store_connection_entry(
        conn_entries, 18, CBM_RS_STORE_CONN_KIND_PRAGMA, "pragma.mmap_size",
        conn_required | CBM_RS_STORE_CONN_FLAG_ENV_BACKED,
        CBM_RS_STORE_CONN_MODE_WRITE | CBM_RS_STORE_CONN_MODE_READ_ONLY, 70, 67108864,
        "PRAGMA mmap_size = <resolved>;", "CBM_SQLITE_MMAP_SIZE");
    check_store_connection_entry(conn_entries, 18, CBM_RS_STORE_CONN_KIND_PRAGMA,
                                 "pragma.bulk.cache_size.begin", conn_required,
                                 CBM_RS_STORE_CONN_MODE_BULK_BEGIN, 80, -65536,
                                 "PRAGMA cache_size = -65536;", "");
    check_store_connection_entry(conn_entries, 18, CBM_RS_STORE_CONN_KIND_PRAGMA,
                                 "pragma.bulk.cache_size.end", conn_required,
                                 CBM_RS_STORE_CONN_MODE_BULK_END, 90, -2000,
                                 "PRAGMA cache_size = -2000;", "");
    check_store_connection_entry(
        conn_entries, 18, CBM_RS_STORE_CONN_KIND_PRAGMA, "checkpoint.passive.api",
        conn_required | CBM_RS_STORE_CONN_FLAG_WRITES_DB_OR_WAL, CBM_RS_STORE_CONN_MODE_CHECKPOINT,
        10, 0, "SQLITE_CHECKPOINT_PASSIVE", "");
    check_store_connection_entry(conn_entries, 18, CBM_RS_STORE_CONN_KIND_PRAGMA,
                                 "checkpoint.optimize", conn_required,
                                 CBM_RS_STORE_CONN_MODE_CHECKPOINT, 20, 0, "PRAGMA optimize;", "");

    CbmRsStoreConnectionManifestEntryV1 mode_entries[9];
    CbmRsStoreConnectionManifestEntryV1 short_mode_entries[1];
    check_int(
        "store_connection_memory_count",
        cbm_rs_store_connection_manifest_entry_count_for_mode_v1(CBM_RS_STORE_CONN_MODE_MEMORY), 4);
    check_int(
        "store_connection_readonly_count",
        cbm_rs_store_connection_manifest_entry_count_for_mode_v1(CBM_RS_STORE_CONN_MODE_READ_ONLY),
        7);
    check_int("store_connection_readonly_write_count",
              cbm_rs_store_connection_manifest_write_entry_count_for_mode_v1(
                  CBM_RS_STORE_CONN_MODE_READ_ONLY),
              0);
    check_int(
        "store_connection_bulk_begin_count",
        cbm_rs_store_connection_manifest_entry_count_for_mode_v1(CBM_RS_STORE_CONN_MODE_BULK_BEGIN),
        2);
    check_int(
        "store_connection_bulk_end_count",
        cbm_rs_store_connection_manifest_entry_count_for_mode_v1(CBM_RS_STORE_CONN_MODE_BULK_END),
        2);
    check_int(
        "store_connection_checkpoint_count",
        cbm_rs_store_connection_manifest_entry_count_for_mode_v1(CBM_RS_STORE_CONN_MODE_CHECKPOINT),
        2);
    check_int("store_connection_checkpoint_write_count",
              cbm_rs_store_connection_manifest_write_entry_count_for_mode_v1(
                  CBM_RS_STORE_CONN_MODE_CHECKPOINT),
              1);
    check_int("store_connection_unknown_count",
              cbm_rs_store_connection_manifest_entry_count_for_mode_v1(UINT32_C(1) << 31), 0);

    check_int("store_connection_entries_for_mode_null",
              cbm_rs_store_connection_manifest_entries_for_mode_v1(
                  CBM_RS_STORE_CONN_MODE_CHECKPOINT, NULL, 2),
              -1);
    check_int("store_connection_entries_for_mode_short",
              cbm_rs_store_connection_manifest_entries_for_mode_v1(
                  CBM_RS_STORE_CONN_MODE_CHECKPOINT, short_mode_entries, 1),
              -1);
    check_int("store_connection_entries_for_mode_negative",
              cbm_rs_store_connection_manifest_entries_for_mode_v1(
                  CBM_RS_STORE_CONN_MODE_CHECKPOINT, mode_entries, -1),
              -1);
    check_int(
        "store_connection_entries_for_mode_empty",
        cbm_rs_store_connection_manifest_entries_for_mode_v1(UINT32_C(1) << 31, mode_entries, 0),
        0);
    check_int("store_connection_entries_for_checkpoint",
              cbm_rs_store_connection_manifest_entries_for_mode_v1(
                  CBM_RS_STORE_CONN_MODE_CHECKPOINT, mode_entries, 2),
              2);
    check_str("store_connection_checkpoint_first", mode_entries[0].name, "checkpoint.passive.api");
    check_str("store_connection_checkpoint_second", mode_entries[1].name, "checkpoint.optimize");
    check_u32("store_connection_checkpoint_first_flags", mode_entries[0].flags,
              conn_required | CBM_RS_STORE_CONN_FLAG_WRITES_DB_OR_WAL);

    check_int("store_connection_entries_for_readonly",
              cbm_rs_store_connection_manifest_entries_for_mode_v1(CBM_RS_STORE_CONN_MODE_READ_ONLY,
                                                                   mode_entries, 9),
              7);
    for (int i = 0; i < 7; i++) {
        if ((mode_entries[i].flags & CBM_RS_STORE_CONN_FLAG_WRITES_DB_OR_WAL) != 0) {
            fail("store_connection_readonly_nonwriting", "readonly plan contains write pragma");
        }
    }
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

static void test_registry_is_test_qn_export(void) {
    check_int("registry_is_test_qn_null", cbm_rs_registry_is_test_qn_v1(NULL), 0);
    check_int("registry_is_test_qn_empty", cbm_rs_registry_is_test_qn_v1(""), 0);
    check_int("registry_is_test_qn_Test", cbm_rs_registry_is_test_qn_v1("pkg.Test.Anchor"), 1);
    check_int("registry_is_test_qn_test", cbm_rs_registry_is_test_qn_v1("pkg.test.Anchor"), 1);
    check_int("registry_is_test_qn_Mock", cbm_rs_registry_is_test_qn_v1("pkg.Mock.Anchor"), 1);
    check_int("registry_is_test_qn_mock", cbm_rs_registry_is_test_qn_v1("pkg.mock.Anchor"), 1);
    check_int("registry_is_test_qn_Stub", cbm_rs_registry_is_test_qn_v1("pkg.Stub.Anchor"), 1);
    check_int("registry_is_test_qn_stub", cbm_rs_registry_is_test_qn_v1("pkg.stub.Anchor"), 1);
    check_int("registry_is_test_qn_Fake", cbm_rs_registry_is_test_qn_v1("pkg.Fake.Anchor"), 1);
    check_int("registry_is_test_qn_fake", cbm_rs_registry_is_test_qn_v1("pkg.fake.Anchor"), 1);
    check_int("registry_is_test_qn_Fixture", cbm_rs_registry_is_test_qn_v1("pkg.Fixture.Anchor"),
              1);
    check_int("registry_is_test_qn_spec", cbm_rs_registry_is_test_qn_v1("pkg.spec.Anchor"), 1);
    check_int("registry_is_test_qn_upper_TEST", cbm_rs_registry_is_test_qn_v1("pkg.TEST.Anchor"),
              0);
    check_int("registry_is_test_qn_lower_fixture",
              cbm_rs_registry_is_test_qn_v1("pkg.fixture.Anchor"), 0);
    check_int("registry_is_test_qn_upper_SPEC", cbm_rs_registry_is_test_qn_v1("pkg.SPEC.Anchor"),
              0);

    const char non_utf8_with_spec[] = "pkg.\xff"
                                      "spec.Anchor";
    const char non_utf8_without_needle[] = "pkg.\xff"
                                           "plain.Anchor";
    check_int("registry_is_test_qn_non_utf8_spec",
              cbm_rs_registry_is_test_qn_v1(non_utf8_with_spec), 1);
    check_int("registry_is_test_qn_non_utf8_plain",
              cbm_rs_registry_is_test_qn_v1(non_utf8_without_needle), 0);
}

static void test_registry_is_test_qn_bridge(void) {
    const char *positives[] = {"Test", "test", "Mock", "mock", "Stub", "stub", "Fake",
                               "fake", "Fixture", "spec"};
    for (size_t i = 0; i < sizeof(positives) / sizeof(positives[0]); i++) {
        check_int("registry_test_qn_bridge_needle",
                  cbm_pipeline_registry_is_test_qn(positives[i]) ? 1 : 0, 1);
    }

    check_int("registry_test_qn_bridge_null", cbm_pipeline_registry_is_test_qn(NULL) ? 1 : 0, 0);
    check_int("registry_test_qn_bridge_empty", cbm_pipeline_registry_is_test_qn("") ? 1 : 0, 0);
    check_int("registry_test_qn_bridge_case", cbm_pipeline_registry_is_test_qn("TEST") ? 1 : 0,
              0);
    check_int("registry_test_qn_bridge_fixture_case",
              cbm_pipeline_registry_is_test_qn("fixture") ? 1 : 0, 0);
    check_int("registry_test_qn_bridge_substring", cbm_pipeline_registry_is_test_qn("contest") ? 1 : 0,
              1);

    const char non_utf8_with_spec[] = "pkg.\xff"
                                      "spec.Anchor";
    const char non_utf8_without_needle[] = "pkg.\xff"
                                         "plain.Anchor";
    check_int("registry_test_qn_bridge_non_utf8_spec",
              cbm_pipeline_registry_is_test_qn(non_utf8_with_spec) ? 1 : 0, 1);
    check_int("registry_test_qn_bridge_non_utf8_plain",
              cbm_pipeline_registry_is_test_qn(non_utf8_without_needle) ? 1 : 0, 0);
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

static void test_yaml_exports(void) {
    cbm_rs_yaml_free(NULL);
    check_null("yaml_get_str_null_root", cbm_rs_yaml_get_str(NULL, "key"));
    check_bool("yaml_has_null_root", cbm_rs_yaml_has(NULL, "key"), false);
    check_double_near("yaml_get_float_null_root", cbm_rs_yaml_get_float(NULL, "key", 7.5), 7.5);
    check_bool("yaml_get_bool_null_root", cbm_rs_yaml_get_bool(NULL, "key", true), true);
    const char *null_items[2] = {0};
    check_int("yaml_get_list_null_root", cbm_rs_yaml_get_str_list(NULL, "key", null_items, 2), 0);

    CBMRustYamlNode *empty = cbm_rs_yaml_parse(NULL, 42);
    check_not_null("yaml_parse_null_input", empty);
    check_bool("yaml_empty_missing", cbm_rs_yaml_has(empty, "anything"), false);
    cbm_rs_yaml_free(empty);

    const unsigned char partial[] = "key: value\n";
    CBMRustYamlNode *partial_root = cbm_rs_yaml_parse(partial, 7);
    check_not_null("yaml_parse_partial", partial_root);
    check_str("yaml_partial_value", cbm_rs_yaml_get_str(partial_root, "key"), "va");
    cbm_rs_yaml_free(partial_root);

    const unsigned char yaml[] = "project:\n"
                                 "  name: myapp # inline comment\n"
                                 "  tags:\n"
                                 "    - web\n"
                                 "    - api\n"
                                 "  version: 2.0\n"
                                 "  enabled: TRUE\n"
                                 "  rate: -3.14\n"
                                 "channel: #general\n"
                                 "quoted: \"#ff0000\"\n"
                                 "key: first\n"
                                 "key: second\n";
    CBMRustYamlNode *root = cbm_rs_yaml_parse(yaml, (int)strlen((const char *)yaml));
    check_not_null("yaml_parse_nested", root);
    check_str("yaml_get_nested_str", cbm_rs_yaml_get_str(root, "project.name"), "myapp");
    check_str("yaml_inline_hash_no_space", cbm_rs_yaml_get_str(root, "channel"), "#general");
    check_str("yaml_quoted_hash", cbm_rs_yaml_get_str(root, "quoted"), "\"#ff0000\"");
    check_str("yaml_duplicate_first", cbm_rs_yaml_get_str(root, "key"), "first");
    check_null("yaml_map_not_scalar", cbm_rs_yaml_get_str(root, "project"));
    check_bool("yaml_has_map_node", cbm_rs_yaml_has(root, "project"), true);
    check_bool("yaml_missing_nested", cbm_rs_yaml_has(root, "project.missing"), false);
    check_bool("yaml_bool_true", cbm_rs_yaml_get_bool(root, "project.enabled", false), true);
    check_bool("yaml_bool_default", cbm_rs_yaml_get_bool(root, "project.unknown", true), true);
    check_double_near("yaml_float", cbm_rs_yaml_get_float(root, "project.rate", 0.0), -3.14);
    check_double_near("yaml_float_default", cbm_rs_yaml_get_float(root, "project.name", 99.0),
                      99.0);

    const char *items[4] = {0};
    check_int("yaml_list_count", cbm_rs_yaml_get_str_list(root, "project.tags", items, 4), 2);
    check_str("yaml_list_0", items[0], "web");
    check_str("yaml_list_1", items[1], "api");
    const char *limited[1] = {0};
    check_int("yaml_list_limit", cbm_rs_yaml_get_str_list(root, "project.tags", limited, 1), 1);
    check_str("yaml_list_limit_0", limited[0], "web");

    char long_path[300];
    memset(long_path, 'a', 260);
    long_path[260] = '\0';
    check_null("yaml_path_segment_overflow", cbm_rs_yaml_get_str(root, long_path));
    check_bool("yaml_has_path_segment_overflow", cbm_rs_yaml_has(root, long_path), false);
    check_null("yaml_empty_path", cbm_rs_yaml_get_str(root, ""));
    check_null("yaml_null_path", cbm_rs_yaml_get_str(root, NULL));
    cbm_rs_yaml_free(root);

    const unsigned char top_list[] = "colors:\n"
                                     "- red\n"
                                     "- green\n";
    CBMRustYamlNode *top = cbm_rs_yaml_parse(top_list, (int)strlen((const char *)top_list));
    check_not_null("yaml_top_list_parse", top);
    const char *colors[3] = {0};
    check_int("yaml_top_list_count", cbm_rs_yaml_get_str_list(top, "colors", colors, 3), 2);
    check_str("yaml_top_list_0", colors[0], "red");
    check_str("yaml_top_list_1", colors[1], "green");
    cbm_rs_yaml_free(top);
}

static void test_cypher_scalar_func_index_exports(void) {
    /* 符合（ASCII 大小寫不敏感），索引對齊 C scalar_func_canonical 的 names[] 順序 */
    check_int("cypher_scalar_labels", cbm_rs_cypher_scalar_func_index_v1("labels"), 0);
    check_int("cypher_scalar_labels_upper", cbm_rs_cypher_scalar_func_index_v1("LABELS"), 0);
    check_int("cypher_scalar_type", cbm_rs_cypher_scalar_func_index_v1("Type"), 1);
    check_int("cypher_scalar_tointeger", cbm_rs_cypher_scalar_func_index_v1("toInteger"), 5);
    check_int("cypher_scalar_tointeger_upper", cbm_rs_cypher_scalar_func_index_v1("TOINTEGER"), 5);
    check_int("cypher_scalar_toboolean", cbm_rs_cypher_scalar_func_index_v1("toBoolean"), 7);
    check_int("cypher_scalar_reverse", cbm_rs_cypher_scalar_func_index_v1("reverse"), 13);
    /* 不符合 → -1 */
    check_int("cypher_scalar_null", cbm_rs_cypher_scalar_func_index_v1(NULL), -1);
    check_int("cypher_scalar_empty", cbm_rs_cypher_scalar_func_index_v1(""), -1);
    check_int("cypher_scalar_unknown", cbm_rs_cypher_scalar_func_index_v1("unknown"), -1);
    check_int("cypher_scalar_prefix", cbm_rs_cypher_scalar_func_index_v1("label"), -1);
    check_int("cypher_scalar_suffix", cbm_rs_cypher_scalar_func_index_v1("labelsX"), -1);
    check_int("cypher_scalar_tolower", cbm_rs_cypher_scalar_func_index_v1("toLower"), -1);
    check_int("cypher_scalar_tostring", cbm_rs_cypher_scalar_func_index_v1("toString"), -1);
}

static void test_cypher_single_char_kind_export(void) {
    check_int("cypher_single_char_lparen", cbm_rs_cypher_single_char_kind_v1('('), 66);
    check_int("cypher_single_char_equals", cbm_rs_cypher_single_char_kind_v1('='), 79);
    check_int("cypher_single_char_pipe", cbm_rs_cypher_single_char_kind_v1('|'), 83);
    check_int("cypher_single_char_unknown", cbm_rs_cypher_single_char_kind_v1('@'), 88);
    check_int("cypher_single_char_negative", cbm_rs_cypher_single_char_kind_v1(-1), -1);
    check_int("cypher_single_char_overflow", cbm_rs_cypher_single_char_kind_v1(256), -1);
}

static void test_cypher_two_char_kind_export(void) {
    check_int("cypher_two_char_neq_bang", cbm_rs_cypher_two_char_kind_v1('!', '='), 17);
    check_int("cypher_two_char_neq_angle", cbm_rs_cypher_two_char_kind_v1('<', '>'), 17);
    check_int("cypher_two_char_eqtilde", cbm_rs_cypher_two_char_kind_v1('=', '~'), 80);
    check_int("cypher_two_char_gte", cbm_rs_cypher_two_char_kind_v1('>', '='), 81);
    check_int("cypher_two_char_lte", cbm_rs_cypher_two_char_kind_v1('<', '='), 82);
    check_int("cypher_two_char_dotdot", cbm_rs_cypher_two_char_kind_v1('.', '.'), 84);
    check_int("cypher_two_char_unknown", cbm_rs_cypher_two_char_kind_v1('@', '!'), 88);
    check_int("cypher_two_char_negative", cbm_rs_cypher_two_char_kind_v1(-1, '='), -1);
    check_int("cypher_two_char_overflow", cbm_rs_cypher_two_char_kind_v1(256, '='), -1);
}

static void test_cypher_multiarg_func_index_exports(void) {
    /* 符合（ASCII 大小寫不敏感），索引對齊 C multiarg_func_canonical 的 names[] 順序 */
    check_int("cypher_multiarg_coalesce", cbm_rs_cypher_multiarg_func_index_v1("coalesce"), 0);
    check_int("cypher_multiarg_coalesce_upper", cbm_rs_cypher_multiarg_func_index_v1("COALESCE"),
              0);
    check_int("cypher_multiarg_substring", cbm_rs_cypher_multiarg_func_index_v1("substring"), 1);
    check_int("cypher_multiarg_replace", cbm_rs_cypher_multiarg_func_index_v1("Replace"), 2);
    check_int("cypher_multiarg_left", cbm_rs_cypher_multiarg_func_index_v1("LEFT"), 3);
    check_int("cypher_multiarg_right", cbm_rs_cypher_multiarg_func_index_v1("right"), 4);
    /* 不符合 → -1 */
    check_int("cypher_multiarg_null", cbm_rs_cypher_multiarg_func_index_v1(NULL), -1);
    check_int("cypher_multiarg_empty", cbm_rs_cypher_multiarg_func_index_v1(""), -1);
    check_int("cypher_multiarg_prefix", cbm_rs_cypher_multiarg_func_index_v1("coalesc"), -1);
    check_int("cypher_multiarg_suffix", cbm_rs_cypher_multiarg_func_index_v1("coalesceX"), -1);
    check_int("cypher_multiarg_count", cbm_rs_cypher_multiarg_func_index_v1("count"), -1);
    check_int("cypher_multiarg_tolower", cbm_rs_cypher_multiarg_func_index_v1("toLower"), -1);
    check_int("cypher_multiarg_labels", cbm_rs_cypher_multiarg_func_index_v1("labels"), -1);
    check_int("cypher_multiarg_split", cbm_rs_cypher_multiarg_func_index_v1("split"), -1);
    check_int("cypher_multiarg_non_ascii_suffix",
              cbm_rs_cypher_multiarg_func_index_v1("right\xC3\xA9"), -1);
}

static void test_cypher_aggregate_func_index_exports(void) {
    check_int("cypher_agg_count", cbm_rs_cypher_aggregate_func_index_v1(10), 0);
    check_int("cypher_agg_sum", cbm_rs_cypher_aggregate_func_index_v1(26), 1);
    check_int("cypher_agg_avg", cbm_rs_cypher_aggregate_func_index_v1(27), 2);
    check_int("cypher_agg_min", cbm_rs_cypher_aggregate_func_index_v1(28), 3);
    check_int("cypher_agg_max", cbm_rs_cypher_aggregate_func_index_v1(29), 4);
    check_int("cypher_agg_collect", cbm_rs_cypher_aggregate_func_index_v1(30), 5);
    check_int("cypher_agg_tolower", cbm_rs_cypher_aggregate_func_index_v1(31), -1);
    check_int("cypher_agg_ident", cbm_rs_cypher_aggregate_func_index_v1(85), -1);
    check_int("cypher_agg_negative", cbm_rs_cypher_aggregate_func_index_v1(-1), -1);
    check_int("cypher_agg_eof", cbm_rs_cypher_aggregate_func_index_v1(88), -1);
}

static void test_cypher_string_func_index_exports(void) {
    check_int("cypher_string_tolower", cbm_rs_cypher_string_func_index_v1(31), 0);
    check_int("cypher_string_toupper", cbm_rs_cypher_string_func_index_v1(32), 1);
    check_int("cypher_string_tostring", cbm_rs_cypher_string_func_index_v1(33), 2);
    check_int("cypher_string_count", cbm_rs_cypher_string_func_index_v1(10), -1);
    check_int("cypher_string_ident", cbm_rs_cypher_string_func_index_v1(85), -1);
    check_int("cypher_string_negative", cbm_rs_cypher_string_func_index_v1(-1), -1);
    check_int("cypher_string_eof", cbm_rs_cypher_string_func_index_v1(88), -1);
}

static void test_cypher_lexer_exports(void) {
    check_int("cypher_lex_count_null", cbm_rs_cypher_lex_token_count_v1(NULL, 0), -1);
    char invalid_buf[8] = "x";
    check_size("cypher_lex_summary_null",
               cbm_rs_cypher_lex_summary_v1(invalid_buf, sizeof(invalid_buf), NULL, 0), SIZE_MAX);

    const unsigned char query[] = "MATCH (f:Function)-[r:CALLS|DEFINES*1..2]->(g) "
                                  "WHERE f.name =~ \"Al\\npha\" RETURN COUNT(g)";
    const char *expected = "MATCH@0:MATCH|LPAREN@6:(|IDENT@7:f|COLON@8:\\:|IDENT@9:Function|"
                           "RPAREN@17:)|DASH@18:-|LBRACKET@19:[|IDENT@20:r|COLON@21:\\:|"
                           "IDENT@22:CALLS|PIPE@27:\\||IDENT@28:DEFINES|STAR@35:*|NUMBER@36:1|"
                           "DOTDOT@37:..|NUMBER@39:2|RBRACKET@40:]|DASH@41:-|GT@42:>|"
                           "LPAREN@43:(|IDENT@44:g|RPAREN@45:)|WHERE@47:WHERE|IDENT@53:f|"
                           "DOT@54:.|IDENT@55:name|EQTILDE@60:=~|STRING@63:Al\\npha|"
                           "RETURN@73:RETURN|COUNT@80:COUNT|LPAREN@85:(|IDENT@86:g|"
                           "RPAREN@87:)|EOF@88:";
    char summary[1024];
    size_t summary_len = cbm_rs_cypher_lex_summary_v1(summary, sizeof(summary), query,
                                                      (int)strlen((const char *)query));
    check_size("cypher_lex_summary_len", summary_len, strlen(expected));
    check_str("cypher_lex_summary", summary, expected);
    char short_buf[12];
    check_size("cypher_lex_summary_short_len",
               cbm_rs_cypher_lex_summary_v1(short_buf, sizeof(short_buf), query,
                                            (int)strlen((const char *)query)),
               strlen(expected));
    check_str("cypher_lex_summary_short", short_buf, "MATCH@0:MAT");

    int count = cbm_rs_cypher_lex_token_count_v1(query, (int)strlen((const char *)query));
    check_int("cypher_lex_token_count", count, 35);
    CbmRsCypherTokenV1 tokens[40] = {0};
    check_int("cypher_lex_tokens_null",
              cbm_rs_cypher_lex_tokens_v1(query, (int)strlen((const char *)query), NULL, 40), -1);
    check_int("cypher_lex_tokens_short",
              cbm_rs_cypher_lex_tokens_v1(query, (int)strlen((const char *)query), tokens, 2), -1);
    check_int("cypher_lex_tokens",
              cbm_rs_cypher_lex_tokens_v1(query, (int)strlen((const char *)query), tokens, 40), 35);
    check_int("cypher_tok_match_kind", tokens[0].kind, TOK_MATCH);
    check_int("cypher_tok_match_pos", tokens[0].pos, 0);
    check_int("cypher_tok_dotdot_kind", tokens[15].kind, TOK_DOTDOT);
    check_int("cypher_tok_dotdot_pos", tokens[15].pos, 37);
    check_int("cypher_tok_string_kind", tokens[28].kind, TOK_STRING);
    check_int("cypher_tok_string_pos", tokens[28].pos, 63);
    check_int("cypher_tok_string_len", tokens[28].text_len, 6);
    check_int("cypher_tok_eof_kind", tokens[34].kind, TOK_EOF);
    check_int("cypher_tok_eof_pos", tokens[34].pos, 88);

    const unsigned char comments[] = "// one\nMATCH /* block */ (n) -- two\nRETURN 'a\\tb\\'c'";
    const char *expected_comments =
        "MATCH@7:MATCH|LPAREN@25:(|IDENT@26:n|RPAREN@27:)|RETURN@36:RETURN|"
        "STRING@43:a\\tb'c|EOF@52:";
    check_size("cypher_lex_comments_len",
               cbm_rs_cypher_lex_summary_v1(summary, sizeof(summary), comments,
                                            (int)strlen((const char *)comments)),
               strlen(expected_comments));
    check_str("cypher_lex_comments", summary, expected_comments);
}

static void test_cypher_parse_summary_export(void) {
    char invalid_buf[8] = "keep";
    check_size("cypher_parse_summary_null",
               cbm_rs_cypher_parse_summary_v1(invalid_buf, sizeof(invalid_buf), NULL, 0), SIZE_MAX);
    check_str("cypher_parse_summary_null_buf", invalid_buf, "keep");

    const unsigned char malformed[] = "MATCH (f:Function WHERE f.name = \"x\"";
    check_size("cypher_parse_summary_malformed",
               cbm_rs_cypher_parse_summary_v1(invalid_buf, sizeof(invalid_buf), malformed,
                                              (int)strlen((const char *)malformed)),
               SIZE_MAX);

    const unsigned char parser_shape[] =
        "MATCH (n:Function|Module)-[r:CALLS|DEFINES*1..2]->(m:Function) "
        "WHERE n.name = \"main\" OR n.name = \"Alpha\" AND m.name = \"Beta\" RETURN n.name";
    const char *expected_parser_shape =
        "patterns=1;P0=req;nodes=n:Function\\|Module,m:Function;"
        "rels=r:CALLS|DEFINES:outbound:1..2;"
        "where=OR(COND(n.name,=,main),AND(COND(n.name,=,Alpha),COND(m.name,=,Beta)));"
        "with=-;post_where=-;return=n.name";
    char summary[1024];
    check_size("cypher_parse_summary_len_only",
               cbm_rs_cypher_parse_summary_v1(NULL, 0, parser_shape,
                                              (int)strlen((const char *)parser_shape)),
               strlen(expected_parser_shape));
    check_size("cypher_parse_summary_parser_shape_len",
               cbm_rs_cypher_parse_summary_v1(summary, sizeof(summary), parser_shape,
                                              (int)strlen((const char *)parser_shape)),
               strlen(expected_parser_shape));
    check_str("cypher_parse_summary_parser_shape", summary, expected_parser_shape);
    char short_buf[12];
    check_size("cypher_parse_summary_short_len",
               cbm_rs_cypher_parse_summary_v1(short_buf, sizeof(short_buf), parser_shape,
                                              (int)strlen((const char *)parser_shape)),
               strlen(expected_parser_shape));
    check_str("cypher_parse_summary_short", short_buf, "patterns=1;");
    check_size("cypher_parse_summary_negative_len",
               cbm_rs_cypher_parse_summary_v1(summary, sizeof(summary), parser_shape, -1),
               SIZE_MAX);

    const unsigned char hop_single_number[] =
        "MATCH (a:Function)-[:CALLS*2]->(b:Function) RETURN b.name";
    const char *expected_hop_single_number =
        "patterns=1;P0=req;nodes=a:Function,b:Function;"
        "rels=:CALLS:outbound:1..2;where=-;with=-;post_where=-;return=b.name";
    check_size("cypher_parse_summary_hop_single_number_len",
               cbm_rs_cypher_parse_summary_v1(summary, sizeof(summary), hop_single_number,
                                              (int)strlen((const char *)hop_single_number)),
               strlen(expected_hop_single_number));
    check_str("cypher_parse_summary_hop_single_number", summary, expected_hop_single_number);

    const unsigned char hop_unbounded[] =
        "MATCH (a:Function)-[:CALLS*]->(b:Function) RETURN b.name";
    const char *expected_hop_unbounded =
        "patterns=1;P0=req;nodes=a:Function,b:Function;"
        "rels=:CALLS:outbound:1..0;where=-;with=-;post_where=-;return=b.name";
    check_size("cypher_parse_summary_hop_unbounded_len",
               cbm_rs_cypher_parse_summary_v1(summary, sizeof(summary), hop_unbounded,
                                              (int)strlen((const char *)hop_unbounded)),
               strlen(expected_hop_unbounded));
    check_str("cypher_parse_summary_hop_unbounded", summary, expected_hop_unbounded);

    const unsigned char hop_omitted_min[] =
        "MATCH (a:Function)-[:CALLS*..3]->(b:Function) RETURN b.name";
    const char *expected_hop_omitted_min =
        "patterns=1;P0=req;nodes=a:Function,b:Function;"
        "rels=:CALLS:outbound:1..3;where=-;with=-;post_where=-;return=b.name";
    check_size("cypher_parse_summary_hop_omitted_min_len",
               cbm_rs_cypher_parse_summary_v1(summary, sizeof(summary), hop_omitted_min,
                                              (int)strlen((const char *)hop_omitted_min)),
               strlen(expected_hop_omitted_min));
    check_str("cypher_parse_summary_hop_omitted_min", summary, expected_hop_omitted_min);

    const unsigned char optional_match[] =
        "MATCH (f:Function) OPTIONAL MATCH (f)-[:CALLS]->(g:Function) RETURN f.name, g.name";
    const char *expected_optional_match =
        "patterns=2;P0=req;nodes=f:Function;rels=-;"
        "P1=optional;nodes=f:,g:Function;rels=:CALLS:outbound:1..1;"
        "where=-;with=-;post_where=-;return=f.name,g.name";
    check_size("cypher_parse_summary_optional_len",
               cbm_rs_cypher_parse_summary_v1(summary, sizeof(summary), optional_match,
                                              (int)strlen((const char *)optional_match)),
               strlen(expected_optional_match));
    check_str("cypher_parse_summary_optional", summary, expected_optional_match);

    const unsigned char union_all[] =
        "MATCH (f:Function) RETURN f.name UNION ALL MATCH (g:Function) RETURN g.name";
    const char *expected_union_all = "union=all;left={patterns=1;P0=req;nodes=f:Function;rels=-;"
                                     "where=-;with=-;post_where=-;return=f.name};"
                                     "right={patterns=1;P0=req;nodes=g:Function;rels=-;"
                                     "where=-;with=-;post_where=-;return=g.name}";

    const char *expected_union_distinct =
        "union=distinct;left={patterns=1;P0=req;nodes=f:Function;rels=-;"
        "where=-;with=-;post_where=-;return=f.name};"
        "right={patterns=1;P0=req;nodes=g:Function;rels=-;"
        "where=-;with=-;post_where=-;return=g.name}";
    check_size("cypher_parse_summary_union_len",
               cbm_rs_cypher_parse_summary_v1(summary, sizeof(summary), union_all,
                                              (int)strlen((const char *)union_all)),
               strlen(expected_union_all));
    check_str("cypher_parse_summary_union", summary, expected_union_all);

    const unsigned char union_distinct[] =
        "MATCH (f:Function) RETURN f.name UNION MATCH (g:Function) RETURN g.name";
    check_size("cypher_parse_summary_union_distinct_len",
               cbm_rs_cypher_parse_summary_v1(summary, sizeof(summary), union_distinct,
                                              (int)strlen((const char *)union_distinct)),
               strlen(expected_union_distinct));
    check_str("cypher_parse_summary_union_distinct", summary, expected_union_distinct);

    const unsigned char exists_query[] =
        "MATCH (f:Function) WHERE NOT EXISTS { (f)<-[:CALLS]-() } RETURN f.name";
    const char *expected_exists =
        "patterns=1;P0=req;nodes=f:Function;rels=-;"
        "where=NOT(EXISTS(f,inbound,CALLS));with=-;post_where=-;return=f.name";
    check_size("cypher_parse_summary_exists_len",
               cbm_rs_cypher_parse_summary_v1(summary, sizeof(summary), exists_query,
                                              (int)strlen((const char *)exists_query)),
               strlen(expected_exists));
    check_str("cypher_parse_summary_exists", summary, expected_exists);

    const unsigned char with_post_where[] =
        "MATCH (f:Function)-[:CALLS]->(g:Function) "
        "WITH f.name AS caller, COUNT(g) AS cnt WHERE cnt > \"1\" RETURN caller";
    const char *expected_with =
        "patterns=1;P0=req;nodes=f:Function,g:Function;rels=:CALLS:outbound:1..1;"
        "where=-;with=f.name:caller,COUNT(g):cnt;post_where=COND(cnt,>,1);return=caller";
    check_size("cypher_parse_summary_with_len",
               cbm_rs_cypher_parse_summary_v1(summary, sizeof(summary), with_post_where,
                                              (int)strlen((const char *)with_post_where)),
               strlen(expected_with));
    check_str("cypher_parse_summary_with", summary, expected_with);

    const unsigned char exists_identifier_query[] =
        "MATCH (f:Function) WHERE EXISTS (f) RETURN f.name";
    check_size("cypher_parse_summary_exists_identifier_len",
               cbm_rs_cypher_parse_summary_v1(summary, sizeof(summary), exists_identifier_query,
                                              (int)strlen((const char *)exists_identifier_query)),
               SIZE_MAX);
    check_size("cypher_parse_summary_exists_identifier_len_only",
               cbm_rs_cypher_parse_summary_v1(NULL, 0, exists_identifier_query,
                                              (int)strlen((const char *)exists_identifier_query)),
               SIZE_MAX);

    const unsigned char with_distinct[] =
        "MATCH (f:Function) WITH DISTINCT f.name AS caller WHERE caller = \"x\" RETURN caller";
    const char *expected_with_distinct = "patterns=1;P0=req;nodes=f:Function;rels=-;"
                                         "where=-;with=DISTINCT f.name:caller;"
                                         "post_where=COND(caller,=,x);return=caller";
    check_size("cypher_parse_summary_with_distinct_len",
               cbm_rs_cypher_parse_summary_v1(summary, sizeof(summary), with_distinct,
                                              (int)strlen((const char *)with_distinct)),
               strlen(expected_with_distinct));
    check_str("cypher_parse_summary_with_distinct", summary, expected_with_distinct);

    const unsigned char return_distinct[] =
        "MATCH (f:Function) RETURN DISTINCT f.name ORDER BY f.name LIMIT 1 SKIP 0";
    const char *expected_return_distinct = "patterns=1;P0=req;nodes=f:Function;rels=-;"
                                           "where=-;with=-;post_where=-;return=DISTINCT f.name";
    check_size("cypher_parse_summary_return_distinct_len",
               cbm_rs_cypher_parse_summary_v1(summary, sizeof(summary), return_distinct,
                                              (int)strlen((const char *)return_distinct)),
               strlen(expected_return_distinct));
    check_str("cypher_parse_summary_return_distinct", summary, expected_return_distinct);

    const unsigned char return_distinct_count[] =
        "MATCH (f:Function) RETURN DISTINCT COUNT(DISTINCT f.name) AS name_count";
    const char *expected_return_distinct_count =
        "patterns=1;P0=req;nodes=f:Function;rels=-;"
        "where=-;with=-;post_where=-;return=DISTINCT COUNT(DISTINCT f.name):name_count";
    check_size("cypher_parse_summary_return_distinct_count_len",
               cbm_rs_cypher_parse_summary_v1(summary, sizeof(summary), return_distinct_count,
                                              (int)strlen((const char *)return_distinct_count)),
               strlen(expected_return_distinct_count));
    check_str("cypher_parse_summary_return_distinct_count", summary,
              expected_return_distinct_count);
}

static void test_mcp_tools_cursor_offset_exports(void) {
    const int tc = 14;
    /* null / malformed JSON / 尾端殘留 → 0 */
    check_int("mcp_cursor_null", cbm_rs_mcp_tools_cursor_offset_v1(NULL, tc), 0);
    check_int("mcp_cursor_empty", cbm_rs_mcp_tools_cursor_offset_v1("", tc), 0);
    check_int("mcp_cursor_open_brace", cbm_rs_mcp_tools_cursor_offset_v1("{", tc), 0);
    check_int("mcp_cursor_bad_value", cbm_rs_mcp_tools_cursor_offset_v1("{\"cursor\":}", tc), 0);
    check_int("mcp_cursor_not_json", cbm_rs_mcp_tools_cursor_offset_v1("not json", tc), 0);
    check_int("mcp_cursor_trailing", cbm_rs_mcp_tools_cursor_offset_v1("{} trailing", tc), 0);
    /* 合法 JSON 但 root 非物件或無 cursor → 0 */
    check_int("mcp_cursor_empty_obj", cbm_rs_mcp_tools_cursor_offset_v1("{}", tc), 0);
    check_int("mcp_cursor_no_cursor", cbm_rs_mcp_tools_cursor_offset_v1("{\"limit\":5}", tc), 0);
    check_int("mcp_cursor_array", cbm_rs_mcp_tools_cursor_offset_v1("[]", tc), 0);
    check_int("mcp_cursor_number_root", cbm_rs_mcp_tools_cursor_offset_v1("5", tc), 0);
    /* cursor 存在但非有效非負整數字串 → tool_count */
    check_int("mcp_cursor_empty_str", cbm_rs_mcp_tools_cursor_offset_v1("{\"cursor\":\"\"}", tc),
              tc);
    check_int("mcp_cursor_abc", cbm_rs_mcp_tools_cursor_offset_v1("{\"cursor\":\"abc\"}", tc), tc);
    check_int("mcp_cursor_number", cbm_rs_mcp_tools_cursor_offset_v1("{\"cursor\":5}", tc), tc);
    check_int("mcp_cursor_null_val", cbm_rs_mcp_tools_cursor_offset_v1("{\"cursor\":null}", tc),
              tc);
    check_int("mcp_cursor_negative", cbm_rs_mcp_tools_cursor_offset_v1("{\"cursor\":\"-1\"}", tc),
              tc);
    check_int("mcp_cursor_trailing_space",
              cbm_rs_mcp_tools_cursor_offset_v1("{\"cursor\":\"5 \"}", tc), tc);
    check_int("mcp_cursor_hex", cbm_rs_mcp_tools_cursor_offset_v1("{\"cursor\":\"0x5\"}", tc), tc);
    check_int("mcp_cursor_overflow",
              cbm_rs_mcp_tools_cursor_offset_v1("{\"cursor\":\"99999999999999999999\"}", tc), tc);
    /* 有效 cursor → clamp 到 tool_count */
    check_int("mcp_cursor_zero", cbm_rs_mcp_tools_cursor_offset_v1("{\"cursor\":\"0\"}", tc), 0);
    check_int("mcp_cursor_three", cbm_rs_mcp_tools_cursor_offset_v1("{\"cursor\":\"3\"}", tc), 3);
    check_int("mcp_cursor_lead_space", cbm_rs_mcp_tools_cursor_offset_v1("{\"cursor\":\" 4\"}", tc),
              4);
    check_int("mcp_cursor_plus", cbm_rs_mcp_tools_cursor_offset_v1("{\"cursor\":\"+6\"}", tc), 6);
    check_int("mcp_cursor_neg_zero", cbm_rs_mcp_tools_cursor_offset_v1("{\"cursor\":\"-0\"}", tc),
              0);
    check_int("mcp_cursor_clamp", cbm_rs_mcp_tools_cursor_offset_v1("{\"cursor\":\"100\"}", tc),
              tc);
    check_int("mcp_cursor_dup_first",
              cbm_rs_mcp_tools_cursor_offset_v1("{\"cursor\":\"2\",\"cursor\":\"9\"}", tc), 2);
}

static void test_mcp_tool_index_export(void) {
    const char *names[] = {
        "index_repository", "search_graph",     "query_graph", "trace_path",    "get_code_snippet",
        "get_graph_schema", "get_architecture", "search_code", "list_projects", "delete_project",
        "index_status",     "detect_changes",   "manage_adr",  "ingest_traces",
    };
    for (int i = 0; i < 14; i++) {
        char label[64];
        snprintf(label, sizeof(label), "mcp_tool_index_%s", names[i]);
        check_int(label, cbm_rs_mcp_tool_index_v1(names[i]), i);
    }
    check_int("mcp_tool_index_null", cbm_rs_mcp_tool_index_v1(NULL), -1);
    check_int("mcp_tool_index_empty", cbm_rs_mcp_tool_index_v1(""), -1);
    check_int("mcp_tool_index_case_sensitive", cbm_rs_mcp_tool_index_v1("Search_Graph"), -1);
    check_int("mcp_tool_index_unknown", cbm_rs_mcp_tool_index_v1("unknown_tool"), -1);
}

static void test_mcp_tool_dispatch_index_export(void) {
    check_int("mcp_tool_dispatch_index_trace_path", cbm_rs_mcp_tool_dispatch_index_v1("trace_path"),
              3);
    check_int("mcp_tool_dispatch_index_trace_call_path_alias",
              cbm_rs_mcp_tool_dispatch_index_v1("trace_call_path"), 3);
    check_int("mcp_tool_dispatch_index_search_graph",
              cbm_rs_mcp_tool_dispatch_index_v1("search_graph"), 1);
    check_int("mcp_tool_dispatch_index_null", cbm_rs_mcp_tool_dispatch_index_v1(NULL), -1);
    check_int("mcp_tool_dispatch_index_empty", cbm_rs_mcp_tool_dispatch_index_v1(""), -1);
    check_int("mcp_tool_dispatch_index_case_sensitive",
              cbm_rs_mcp_tool_dispatch_index_v1("Trace_Call_Path"), -1);
    check_int("mcp_tool_dispatch_index_unknown", cbm_rs_mcp_tool_dispatch_index_v1("unknown_tool"),
              -1);
}

static void test_mcp_tool_name_export(void) {
    check_int("mcp_tool_count", cbm_rs_mcp_tool_count_v1(), 14);

    char name[32];
    check_size("mcp_tool_name_first_len", cbm_rs_mcp_tool_name_v1(name, sizeof(name), 0),
               strlen("index_repository"));
    check_str("mcp_tool_name_first", name, "index_repository");
    check_size("mcp_tool_name_mid_len", cbm_rs_mcp_tool_name_v1(name, sizeof(name), 7),
               strlen("search_code"));
    check_str("mcp_tool_name_mid", name, "search_code");
    check_size("mcp_tool_name_last_len", cbm_rs_mcp_tool_name_v1(name, sizeof(name), 13),
               strlen("ingest_traces"));
    check_str("mcp_tool_name_last", name, "ingest_traces");
    check_size("mcp_tool_name_len_only", cbm_rs_mcp_tool_name_v1(NULL, 0, 13),
               strlen("ingest_traces"));

    char short_buf[7];
    check_size("mcp_tool_name_short_len", cbm_rs_mcp_tool_name_v1(short_buf, sizeof(short_buf), 0),
               strlen("index_repository"));
    check_str("mcp_tool_name_short", short_buf, "index_");

    char invalid_buf[8] = "keep";
    check_size("mcp_tool_name_negative",
               cbm_rs_mcp_tool_name_v1(invalid_buf, sizeof(invalid_buf), -1), SIZE_MAX);
    check_str("mcp_tool_name_negative_buf", invalid_buf, "keep");
    check_size("mcp_tool_name_over", cbm_rs_mcp_tool_name_v1(invalid_buf, sizeof(invalid_buf), 14),
               SIZE_MAX);
    check_str("mcp_tool_name_over_buf", invalid_buf, "keep");
}

static void test_mcp_tool_title_export(void) {
    char title[32];
    check_size("mcp_tool_title_first_len", cbm_rs_mcp_tool_title_v1(title, sizeof(title), 0),
               strlen("Index repository"));
    check_str("mcp_tool_title_first", title, "Index repository");
    check_size("mcp_tool_title_mid_len", cbm_rs_mcp_tool_title_v1(title, sizeof(title), 7),
               strlen("Search code"));
    check_str("mcp_tool_title_mid", title, "Search code");
    check_size("mcp_tool_title_last_len", cbm_rs_mcp_tool_title_v1(title, sizeof(title), 13),
               strlen("Ingest traces"));
    check_str("mcp_tool_title_last", title, "Ingest traces");
    check_size("mcp_tool_title_len_only", cbm_rs_mcp_tool_title_v1(NULL, 0, 13),
               strlen("Ingest traces"));

    char short_buf[8];
    check_size("mcp_tool_title_short_len",
               cbm_rs_mcp_tool_title_v1(short_buf, sizeof(short_buf), 0),
               strlen("Index repository"));
    check_str("mcp_tool_title_short", short_buf, "Index r");

    char invalid_buf[8] = "keep";
    check_size("mcp_tool_title_negative",
               cbm_rs_mcp_tool_title_v1(invalid_buf, sizeof(invalid_buf), -1), SIZE_MAX);
    check_str("mcp_tool_title_negative_buf", invalid_buf, "keep");
    check_size("mcp_tool_title_over",
               cbm_rs_mcp_tool_title_v1(invalid_buf, sizeof(invalid_buf), 14), SIZE_MAX);
    check_str("mcp_tool_title_over_buf", invalid_buf, "keep");
}

static void test_mcp_tool_description_export(void) {
    const char *index_repository =
        "Index a repository into the knowledge graph. Special mode 'cross-repo-intelligence': skip "
        "extraction, only match Routes/Channels across projects to create CROSS_HTTP_CALLS/"
        "CROSS_ASYNC_CALLS/CROSS_CHANNEL edges. Requires target_projects param. Ensure target "
        "projects have fresh indexes first.";
    char desc[2048];
    check_size("mcp_tool_description_first_len",
               cbm_rs_mcp_tool_description_v1(desc, sizeof(desc), 0), strlen(index_repository));
    check_str("mcp_tool_description_first", desc, index_repository);

    check_size("mcp_tool_description_schema_len",
               cbm_rs_mcp_tool_description_v1(desc, sizeof(desc), 5),
               strlen("Get the schema of the knowledge graph (node labels, edge types)"));
    check_str("mcp_tool_description_schema", desc,
              "Get the schema of the knowledge graph (node labels, edge types)");
    check_size("mcp_tool_description_last_len",
               cbm_rs_mcp_tool_description_v1(desc, sizeof(desc), 13),
               strlen("Ingest runtime traces to enhance the knowledge graph"));
    check_str("mcp_tool_description_last", desc,
              "Ingest runtime traces to enhance the knowledge graph");
    check_size("mcp_tool_description_len_only", cbm_rs_mcp_tool_description_v1(NULL, 0, 13),
               strlen("Ingest runtime traces to enhance the knowledge graph"));

    size_t desc_len = cbm_rs_mcp_tool_description_v1(desc, sizeof(desc), 1);
    check_size("mcp_tool_description_search_len", desc_len, strlen(desc));
    if (!strstr(desc, "Detect truncation with has_more")) {
        fprintf(stderr, "FAIL mcp_tool_description_search_substring\n");
        exit(1);
    }
    desc_len = cbm_rs_mcp_tool_description_v1(desc, sizeof(desc), 2);
    check_size("mcp_tool_description_query_len", desc_len, strlen(desc));
    if (!strstr(desc, "COMPLEXITY / BOTTLENECKS")) {
        fprintf(stderr, "FAIL mcp_tool_description_query_substring\n");
        exit(1);
    }

    char short_buf[12];
    check_size("mcp_tool_description_short_len",
               cbm_rs_mcp_tool_description_v1(short_buf, sizeof(short_buf), 0),
               strlen(index_repository));
    check_str("mcp_tool_description_short", short_buf, "Index a rep");

    char invalid_buf[8] = "keep";
    check_size("mcp_tool_description_negative",
               cbm_rs_mcp_tool_description_v1(invalid_buf, sizeof(invalid_buf), -1), SIZE_MAX);
    check_str("mcp_tool_description_negative_buf", invalid_buf, "keep");
    check_size("mcp_tool_description_over",
               cbm_rs_mcp_tool_description_v1(invalid_buf, sizeof(invalid_buf), 14), SIZE_MAX);
    check_str("mcp_tool_description_over_buf", invalid_buf, "keep");
}

static void test_mcp_tool_input_schema_export(void) {
    char schema[8192];
    size_t schema_len = cbm_rs_mcp_tool_input_schema_v1(schema, sizeof(schema), 0);
    check_size("mcp_tool_input_schema_index_len", schema_len, strlen(schema));
    if (!strstr(schema, "\"required\":[\"repo_path\"]") ||
        !strstr(schema, "cross-repo-intelligence")) {
        fprintf(stderr, "FAIL mcp_tool_input_schema_index_substrings\n");
        exit(1);
    }

    schema_len = cbm_rs_mcp_tool_input_schema_v1(schema, sizeof(schema), 1);
    check_size("mcp_tool_input_schema_search_len", schema_len, strlen(schema));
    if (!strstr(schema, "updateCloudClient") ||
        !strstr(schema, "[\\\"send\\\",\\\"pubsub\\\",\\\"publish\\\"]")) {
        fprintf(stderr, "FAIL mcp_tool_input_schema_search_substrings\n");
        exit(1);
    }

    schema_len = cbm_rs_mcp_tool_input_schema_v1(schema, sizeof(schema), 7);
    check_size("mcp_tool_input_schema_search_code_len", schema_len, strlen(schema));
    if (!strstr(schema, "\\\\.(go|ts)$") || !strstr(schema, "\"default\":10")) {
        fprintf(stderr, "FAIL mcp_tool_input_schema_search_code_substrings\n");
        exit(1);
    }

    check_size("mcp_tool_input_schema_list_projects_len",
               cbm_rs_mcp_tool_input_schema_v1(schema, sizeof(schema), 8),
               strlen("{\"type\":\"object\",\"properties\":{}}"));
    check_str("mcp_tool_input_schema_list_projects", schema,
              "{\"type\":\"object\",\"properties\":{}}");
    check_size("mcp_tool_input_schema_schema_len",
               cbm_rs_mcp_tool_input_schema_v1(schema, sizeof(schema), 5),
               strlen("{\"type\":\"object\",\"properties\":{\"project\":{\"type\":\"string\"}},"
                      "\"required\":[\"project\"]}"));
    check_str("mcp_tool_input_schema_schema", schema,
              "{\"type\":\"object\",\"properties\":{\"project\":{\"type\":\"string\"}},"
              "\"required\":[\"project\"]}");
    check_size("mcp_tool_input_schema_len_only", cbm_rs_mcp_tool_input_schema_v1(NULL, 0, 8),
               strlen("{\"type\":\"object\",\"properties\":{}}"));

    char short_buf[12];
    check_size("mcp_tool_input_schema_short_len",
               cbm_rs_mcp_tool_input_schema_v1(short_buf, sizeof(short_buf), 8),
               strlen("{\"type\":\"object\",\"properties\":{}}"));
    check_str("mcp_tool_input_schema_short", short_buf, "{\"type\":\"ob");

    char invalid_buf[8] = "keep";
    check_size("mcp_tool_input_schema_negative",
               cbm_rs_mcp_tool_input_schema_v1(invalid_buf, sizeof(invalid_buf), -1), SIZE_MAX);
    check_str("mcp_tool_input_schema_negative_buf", invalid_buf, "keep");
    check_size("mcp_tool_input_schema_over",
               cbm_rs_mcp_tool_input_schema_v1(invalid_buf, sizeof(invalid_buf), 14), SIZE_MAX);
    check_str("mcp_tool_input_schema_over_buf", invalid_buf, "keep");
}

static void test_mcp_tool_output_schema_export(void) {
    const char *schema_json = "{\"type\":\"object\",\"additionalProperties\":true}";
    char schema[128];
    check_size("mcp_tool_output_schema_len",
               cbm_rs_mcp_tool_output_schema_v1(schema, sizeof(schema)), strlen(schema_json));
    check_str("mcp_tool_output_schema", schema, schema_json);
    check_size("mcp_tool_output_schema_len_only", cbm_rs_mcp_tool_output_schema_v1(NULL, 0),
               strlen(schema_json));

    char short_buf[12];
    check_size("mcp_tool_output_schema_short_len",
               cbm_rs_mcp_tool_output_schema_v1(short_buf, sizeof(short_buf)), strlen(schema_json));
    check_str("mcp_tool_output_schema_short", short_buf, "{\"type\":\"ob");
}

static void test_mcp_tools_list_json_export(void) {
    char json[65536];
    size_t first_len = cbm_rs_mcp_tools_list_json_v1(json, sizeof(json), 0, 8, 1);
    check_size("mcp_tools_list_json_first_len", first_len, strlen(json));
    if (!strstr(json, "\"tools\":[") || !strstr(json, "\"name\":\"index_repository\"") ||
        !strstr(json, "\"name\":\"search_code\"") || strstr(json, "\"name\":\"list_projects\"") ||
        !strstr(json, "\"inputSchema\":{\"type\":\"object\"") ||
        !strstr(json, "\"outputSchema\":{\"type\":\"object\",\"additionalProperties\":true}") ||
        !strstr(json, "\"nextCursor\":\"8\"")) {
        fprintf(stderr, "FAIL mcp_tools_list_json_first_shape\n");
        exit(1);
    }
    check_size("mcp_tools_list_json_first_len_only",
               cbm_rs_mcp_tools_list_json_v1(NULL, 0, 0, 8, 1), first_len);

    size_t full_len = cbm_rs_mcp_tools_list_json_v1(json, sizeof(json), 0, 14, 0);
    check_size("mcp_tools_list_json_full_len", full_len, strlen(json));
    if (!strstr(json, "\"name\":\"ingest_traces\"") || strstr(json, "nextCursor")) {
        fprintf(stderr, "FAIL mcp_tools_list_json_full_shape\n");
        exit(1);
    }

    check_size("mcp_tools_list_json_empty_len",
               cbm_rs_mcp_tools_list_json_v1(json, sizeof(json), 14, 8, 1),
               strlen("{\"tools\":[]}"));
    check_str("mcp_tools_list_json_empty", json, "{\"tools\":[]}");

    char short_buf[12];
    check_size("mcp_tools_list_json_short_len",
               cbm_rs_mcp_tools_list_json_v1(short_buf, sizeof(short_buf), 14, 8, 1),
               strlen("{\"tools\":[]}"));
    check_str("mcp_tools_list_json_short", short_buf, "{\"tools\":[]");
}

static void test_mcp_content_length_export(void) {
    const int max_len = 10 * 1024 * 1024;
    check_int("mcp_content_length_null", cbm_rs_mcp_content_length_v1(NULL, max_len), 0);
    check_int("mcp_content_length_basic",
              cbm_rs_mcp_content_length_v1("Content-Length: 42", max_len), 42);
    check_int("mcp_content_length_no_space",
              cbm_rs_mcp_content_length_v1("Content-Length:42", max_len), 42);
    check_int("mcp_content_length_plus",
              cbm_rs_mcp_content_length_v1("Content-Length:\t+7", max_len), 7);
    check_int("mcp_content_length_trailing",
              cbm_rs_mcp_content_length_v1("Content-Length:12 trailing", max_len), 12);
    check_int("mcp_content_length_zero", cbm_rs_mcp_content_length_v1("Content-Length:0", max_len),
              0);
    check_int("mcp_content_length_negative",
              cbm_rs_mcp_content_length_v1("Content-Length:-1", max_len), 0);
    check_int("mcp_content_length_missing_digits",
              cbm_rs_mcp_content_length_v1("Content-Length:", max_len), 0);
    check_int("mcp_content_length_bad_prefix",
              cbm_rs_mcp_content_length_v1("content-length:5", max_len), 0);
    check_int("mcp_content_length_max",
              cbm_rs_mcp_content_length_v1("Content-Length:10485760", max_len), max_len);
    check_int("mcp_content_length_over_max",
              cbm_rs_mcp_content_length_v1("Content-Length:10485761", max_len), 0);
}

static void test_mcp_content_length_raw_export(void) {
    check_i64("mcp_content_length_raw_basic", cbm_rs_mcp_content_length_raw_v1("Content-Length: 42"),
              42);
    check_i64("mcp_content_length_raw_no_space", cbm_rs_mcp_content_length_raw_v1("Content-Length:42"),
              42);
    check_i64("mcp_content_length_raw_plus", cbm_rs_mcp_content_length_raw_v1("Content-Length:\t+7"),
              7);
    check_i64("mcp_content_length_raw_trailing", cbm_rs_mcp_content_length_raw_v1("Content-Length:12 trailing"),
              12);
    check_i64("mcp_content_length_raw_zero", cbm_rs_mcp_content_length_raw_v1("Content-Length:0"), 0);
    check_i64("mcp_content_length_raw_negative", cbm_rs_mcp_content_length_raw_v1("Content-Length:-1"),
              -1);
    check_i64("mcp_content_length_raw_missing_digits",
              cbm_rs_mcp_content_length_raw_v1("Content-Length:"), -1);
    check_i64("mcp_content_length_raw_bad_prefix",
              cbm_rs_mcp_content_length_raw_v1("content-length:5"), -1);
    check_i64("mcp_content_length_raw_bad_value",
              cbm_rs_mcp_content_length_raw_v1("Content-Length:abc"), -1);
    check_i64("mcp_content_length_raw_null", cbm_rs_mcp_content_length_raw_v1(NULL), -1);
}

static void test_mcp_content_length_header_matches_export(void) {
    check_int("mcp_content_length_header_matches_basic",
              cbm_rs_mcp_content_length_header_matches_v1("Content-Length: 42"), 1);
    check_int("mcp_content_length_header_matches_no_space",
              cbm_rs_mcp_content_length_header_matches_v1("Content-Length:42"), 1);
    check_int("mcp_content_length_header_matches_empty_value",
              cbm_rs_mcp_content_length_header_matches_v1("Content-Length:"), 1);
    check_int("mcp_content_length_header_matches_bad_value",
              cbm_rs_mcp_content_length_header_matches_v1("Content-Length:abc"), 1);
    check_int("mcp_content_length_header_matches_lowercase",
              cbm_rs_mcp_content_length_header_matches_v1("content-length:5"), 0);
    check_int("mcp_content_length_header_matches_leading_space",
              cbm_rs_mcp_content_length_header_matches_v1(" Content-Length:5"), 0);
    check_int("mcp_content_length_header_matches_missing_colon",
              cbm_rs_mcp_content_length_header_matches_v1("Content-Length"), 0);
    check_int("mcp_content_length_header_matches_other",
              cbm_rs_mcp_content_length_header_matches_v1("Content-Type: application/json"), 0);
    check_int("mcp_content_length_header_matches_empty",
              cbm_rs_mcp_content_length_header_matches_v1(""), 0);
    check_int("mcp_content_length_header_matches_null",
              cbm_rs_mcp_content_length_header_matches_v1(NULL), 0);
}

static void test_mcp_content_length_header_blank_export(void) {
    check_int("mcp_content_length_header_blank_empty",
              cbm_rs_mcp_content_length_header_is_blank_v1(""), 1);
    check_int("mcp_content_length_header_blank_lf",
              cbm_rs_mcp_content_length_header_is_blank_v1("\n"), 1);
    check_int("mcp_content_length_header_blank_crlf",
              cbm_rs_mcp_content_length_header_is_blank_v1("\r\n"), 1);
    check_int("mcp_content_length_header_blank_multi_crlf",
              cbm_rs_mcp_content_length_header_is_blank_v1("\r\r\n"), 1);
    check_int("mcp_content_length_header_blank_space",
              cbm_rs_mcp_content_length_header_is_blank_v1(" "), 0);
    check_int("mcp_content_length_header_blank_space_crlf",
              cbm_rs_mcp_content_length_header_is_blank_v1(" \r\n"), 0);
    check_int("mcp_content_length_header_blank_header",
              cbm_rs_mcp_content_length_header_is_blank_v1("Content-Type: application/json\r\n"),
              0);
    check_int("mcp_content_length_header_blank_null",
              cbm_rs_mcp_content_length_header_is_blank_v1(NULL), 0);
}

static void test_mcp_content_length_response_export(void) {
    const char *expected = "Content-Length: 11\r\n\r\n{\"ok\":true}";
    char framed[64];
    check_size("mcp_content_length_response_len",
               cbm_rs_mcp_content_length_response_v1(framed, sizeof(framed), "{\"ok\":true}"),
               strlen(expected));
    check_str("mcp_content_length_response", framed, expected);
    check_size("mcp_content_length_response_len_only",
               cbm_rs_mcp_content_length_response_v1(NULL, 0, "{\"ok\":true}"), strlen(expected));

    char short_buf[10];
    check_size("mcp_content_length_response_short_len",
               cbm_rs_mcp_content_length_response_v1(short_buf, sizeof(short_buf), "{\"ok\":true}"),
               strlen(expected));
    check_str("mcp_content_length_response_short", short_buf, "Content-L");

    char empty[32];
    check_size("mcp_content_length_response_empty_len",
               cbm_rs_mcp_content_length_response_v1(empty, sizeof(empty), ""),
               strlen("Content-Length: 0\r\n\r\n"));
    check_str("mcp_content_length_response_empty", empty, "Content-Length: 0\r\n\r\n");

    check_size("mcp_content_length_response_null",
               cbm_rs_mcp_content_length_response_v1(framed, sizeof(framed), NULL), SIZE_MAX);
}

static void test_mcp_parse_file_uri_export(void) {
    char path[256];
    check_size("mcp_parse_file_uri_unix_len",
               cbm_rs_mcp_parse_file_uri_v1(path, sizeof(path), "file:///home/user/project"),
               strlen("/home/user/project"));
    check_str("mcp_parse_file_uri_unix", path, "/home/user/project");

    check_size("mcp_parse_file_uri_root_len",
               cbm_rs_mcp_parse_file_uri_v1(path, sizeof(path), "file:///"), strlen("/"));
    check_str("mcp_parse_file_uri_root", path, "/");

    check_size("mcp_parse_file_uri_windows_len",
               cbm_rs_mcp_parse_file_uri_v1(path, sizeof(path), "file:///C:/Users/project"),
               strlen("C:/Users/project"));
    check_str("mcp_parse_file_uri_windows", path, "C:/Users/project");

    check_size("mcp_parse_file_uri_percent_len",
               cbm_rs_mcp_parse_file_uri_v1(path, sizeof(path), "file:///home/user/my%20project"),
               strlen("/home/user/my%20project"));
    check_str("mcp_parse_file_uri_percent", path, "/home/user/my%20project");

    check_size("mcp_parse_file_uri_len_only",
               cbm_rs_mcp_parse_file_uri_v1(NULL, 0, "file:///tmp/test"), strlen("/tmp/test"));

    char short_buf[5];
    check_size("mcp_parse_file_uri_short_len",
               cbm_rs_mcp_parse_file_uri_v1(short_buf, sizeof(short_buf), "file:///usr/local/bin"),
               strlen("/usr/local/bin"));
    check_str("mcp_parse_file_uri_short", short_buf, "/usr");

    check_size("mcp_parse_file_uri_file_empty_len",
               cbm_rs_mcp_parse_file_uri_v1(path, sizeof(path), "file://"), 0);
    check_str("mcp_parse_file_uri_file_empty", path, "");

    check_size("mcp_parse_file_uri_bad_scheme",
               cbm_rs_mcp_parse_file_uri_v1(path, sizeof(path), "https://example.com"), SIZE_MAX);
    check_size("mcp_parse_file_uri_empty", cbm_rs_mcp_parse_file_uri_v1(path, sizeof(path), ""),
               SIZE_MAX);
    check_size("mcp_parse_file_uri_null", cbm_rs_mcp_parse_file_uri_v1(path, sizeof(path), NULL),
               SIZE_MAX);
}

static void test_mcp_method_kind_export(void) {
    check_int("mcp_method_kind_null", cbm_rs_mcp_method_kind_v1(NULL), 0);
    check_int("mcp_method_kind_empty", cbm_rs_mcp_method_kind_v1(""), 0);
    check_int("mcp_method_kind_initialize", cbm_rs_mcp_method_kind_v1("initialize"), 1);
    check_int("mcp_method_kind_ping", cbm_rs_mcp_method_kind_v1("ping"), 2);
    check_int("mcp_method_kind_tools_list", cbm_rs_mcp_method_kind_v1("tools/list"), 3);
    check_int("mcp_method_kind_tools_call", cbm_rs_mcp_method_kind_v1("tools/call"), 4);
    check_int("mcp_method_kind_cancelled", cbm_rs_mcp_method_kind_v1("notifications/cancelled"), 5);
    check_int("mcp_method_kind_case", cbm_rs_mcp_method_kind_v1("Initialize"), 0);
    check_int("mcp_method_kind_space", cbm_rs_mcp_method_kind_v1("tools/list "), 0);
    check_int("mcp_method_kind_unknown", cbm_rs_mcp_method_kind_v1("unknown/method"), 0);
}

static void test_mcp_method_not_found_error_export(void) {
    const char *expected = "{\"code\":-32601,\"message\":\"Method not found\"}";
    char error[64];
    check_size("mcp_method_not_found_error_len",
               cbm_rs_mcp_method_not_found_error_v1(error, sizeof(error)), strlen(expected));
    check_str("mcp_method_not_found_error", error, expected);
    check_size("mcp_method_not_found_error_len_only", cbm_rs_mcp_method_not_found_error_v1(NULL, 0),
               strlen(expected));

    char short_buf[12];
    check_size("mcp_method_not_found_error_short_len",
               cbm_rs_mcp_method_not_found_error_v1(short_buf, sizeof(short_buf)),
               strlen(expected));
    check_str("mcp_method_not_found_error_short", short_buf, "{\"code\":-32");
}

static void test_mcp_parse_error_message_export(void) {
    const char *expected = "Parse error";
    char message[32];
    check_size("mcp_parse_error_message_len",
               cbm_rs_mcp_parse_error_message_v1(message, sizeof(message)), strlen(expected));
    check_str("mcp_parse_error_message", message, expected);
    check_size("mcp_parse_error_message_len_only", cbm_rs_mcp_parse_error_message_v1(NULL, 0),
               strlen(expected));

    char short_buf[6];
    check_size("mcp_parse_error_message_short_len",
               cbm_rs_mcp_parse_error_message_v1(short_buf, sizeof(short_buf)), strlen(expected));
    check_str("mcp_parse_error_message_short", short_buf, "Parse");
}

static void test_mcp_ping_result_export(void) {
    char result[8];
    check_size("mcp_ping_result_len", cbm_rs_mcp_ping_result_v1(result, sizeof(result)),
               strlen("{}"));
    check_str("mcp_ping_result", result, "{}");
    check_size("mcp_ping_result_len_only", cbm_rs_mcp_ping_result_v1(NULL, 0), strlen("{}"));

    char short_buf[2] = "x";
    check_size("mcp_ping_result_short_len", cbm_rs_mcp_ping_result_v1(short_buf, sizeof(short_buf)),
               strlen("{}"));
    check_str("mcp_ping_result_short", short_buf, "{");
}

static void test_mcp_tools_call_default_arguments_export(void) {
    char args[8];
    check_size("mcp_call_default_args_len",
               cbm_rs_mcp_tools_call_default_arguments_v1(args, sizeof(args)), strlen("{}"));
    check_str("mcp_call_default_args", args, "{}");
    check_size("mcp_call_default_args_len_only",
               cbm_rs_mcp_tools_call_default_arguments_v1(NULL, 0), strlen("{}"));

    char short_buf[2] = "x";
    check_size("mcp_call_default_args_short_len",
               cbm_rs_mcp_tools_call_default_arguments_v1(short_buf, sizeof(short_buf)),
               strlen("{}"));
    check_str("mcp_call_default_args_short", short_buf, "{");
}

static void test_mcp_missing_tool_name_message_export(void) {
    const char *expected = "missing tool name";
    char message[32];
    check_size("mcp_missing_tool_name_len",
               cbm_rs_mcp_missing_tool_name_message_v1(message, sizeof(message)), strlen(expected));
    check_str("mcp_missing_tool_name", message, expected);
    check_size("mcp_missing_tool_name_len_only", cbm_rs_mcp_missing_tool_name_message_v1(NULL, 0),
               strlen(expected));

    char short_buf[8];
    check_size("mcp_missing_tool_name_short_len",
               cbm_rs_mcp_missing_tool_name_message_v1(short_buf, sizeof(short_buf)),
               strlen(expected));
    check_str("mcp_missing_tool_name_short", short_buf, "missing");
}

static void test_mcp_missing_project_error_export(void) {
    const char *expected =
        "{\"error\":\"missing required argument: project\",\"hint\":\"Pass the project as the "
        "\\\"project\\\" argument, e.g. {\\\"project\\\":\\\"<name from list_projects>\\\"}. Run "
        "list_projects to see indexed projects.\"}";
    char message[256];
    check_size("mcp_missing_project_error_len",
               cbm_rs_mcp_missing_project_error_v1(message, sizeof(message)), strlen(expected));
    check_str("mcp_missing_project_error", message, expected);
    check_size("mcp_missing_project_error_len_only", cbm_rs_mcp_missing_project_error_v1(NULL, 0),
               strlen(expected));

    char short_buf[16];
    check_size("mcp_missing_project_error_short_len",
               cbm_rs_mcp_missing_project_error_v1(short_buf, sizeof(short_buf)), strlen(expected));
    check_str("mcp_missing_project_error_short", short_buf, "{\"error\":\"missi");
}

static void test_mcp_project_not_found_message_export(void) {
    const char *expected = "project not found";
    char message[32];
    check_size("mcp_project_not_found_len",
               cbm_rs_mcp_project_not_found_message_v1(message, sizeof(message)), strlen(expected));
    check_str("mcp_project_not_found", message, expected);
    check_size("mcp_project_not_found_len_only", cbm_rs_mcp_project_not_found_message_v1(NULL, 0),
               strlen(expected));

    char short_buf[8];
    check_size("mcp_project_not_found_short_len",
               cbm_rs_mcp_project_not_found_message_v1(short_buf, sizeof(short_buf)),
               strlen(expected));
    check_str("mcp_project_not_found_short", short_buf, "project");
}

static void test_mcp_project_list_error_export(void) {
    const char *reason = "project not found or not indexed";
    const char *projects = "\"alpha\",\"beta\"";
    const char *expected_with_projects =
        "{\"error\":\"project not found or not indexed\",\"hint\":\"Use list_projects to see all "
        "indexed projects, then pass it as the \\\"project\\\" "
        "argument.\",\"available_projects\":[\"alpha\",\"beta\"],\"count\":2}";
    const char *expected_empty =
        "{\"error\":\"project not found or not indexed\",\"hint\":\"No projects indexed yet. "
        "Call index_repository first.\"}";

    char message[320];
    check_size("mcp_project_list_error_with_projects_len",
               cbm_rs_mcp_project_list_error_v1(message, sizeof(message), reason, projects, 2),
               strlen(expected_with_projects));
    check_str("mcp_project_list_error_with_projects", message, expected_with_projects);
    check_size("mcp_project_list_error_with_projects_len_only",
               cbm_rs_mcp_project_list_error_v1(NULL, 0, reason, projects, 2),
               strlen(expected_with_projects));

    check_size("mcp_project_list_error_empty_len",
               cbm_rs_mcp_project_list_error_v1(message, sizeof(message), reason, "", 0),
               strlen(expected_empty));
    check_str("mcp_project_list_error_empty", message, expected_empty);

    char short_buf[18];
    check_size("mcp_project_list_error_short_len",
               cbm_rs_mcp_project_list_error_v1(short_buf, sizeof(short_buf), reason, projects, 2),
               strlen(expected_with_projects));
    check_str("mcp_project_list_error_short", short_buf, "{\"error\":\"project");

    check_size("mcp_project_list_error_null_reason",
               cbm_rs_mcp_project_list_error_v1(message, sizeof(message), NULL, projects, 2),
               SIZE_MAX);
    check_size("mcp_project_list_error_null_projects_required",
               cbm_rs_mcp_project_list_error_v1(message, sizeof(message), reason, NULL, 2),
               SIZE_MAX);
    check_size("mcp_project_list_error_null_projects_optional",
               cbm_rs_mcp_project_list_error_v1(message, sizeof(message), reason, NULL, 0),
               strlen(expected_empty));
}

static void test_mcp_unknown_tool_message_export(void) {
    const char *expected = "unknown tool: nonexistent_tool";
    char message[64];
    check_size("mcp_unknown_tool_message_len",
               cbm_rs_mcp_unknown_tool_message_v1(message, sizeof(message), "nonexistent_tool"),
               strlen(expected));
    check_str("mcp_unknown_tool_message", message, expected);
    check_size("mcp_unknown_tool_message_len_only",
               cbm_rs_mcp_unknown_tool_message_v1(NULL, 0, "nonexistent_tool"), strlen(expected));

    char short_buf[14];
    check_size("mcp_unknown_tool_message_short_len",
               cbm_rs_mcp_unknown_tool_message_v1(short_buf, sizeof(short_buf), "nonexistent_tool"),
               strlen(expected));
    check_str("mcp_unknown_tool_message_short", short_buf, "unknown tool:");

    check_size("mcp_unknown_tool_message_null",
               cbm_rs_mcp_unknown_tool_message_v1(message, sizeof(message), NULL), SIZE_MAX);
}

static void check_mcp_page_bounds(const char *label, int offset, int limit, int include_next,
                                  int tool_count, int start, int end, int has_next,
                                  int next_cursor) {
    CbmRsMcpToolsPageBoundsV1 bounds = {-1, -1, -1, -1};
    check_int(label,
              cbm_rs_mcp_tools_page_bounds_v1(offset, limit, include_next, tool_count, &bounds), 0);
    char field[96];
    snprintf(field, sizeof(field), "%s_start", label);
    check_int(field, bounds.start, start);
    snprintf(field, sizeof(field), "%s_end", label);
    check_int(field, bounds.end, end);
    snprintf(field, sizeof(field), "%s_has_next", label);
    check_int(field, bounds.has_next, has_next);
    snprintf(field, sizeof(field), "%s_next_cursor", label);
    check_int(field, bounds.next_cursor, next_cursor);
}

static void test_mcp_tools_page_bounds_export(void) {
    check_int("mcp_page_bounds_null_out", cbm_rs_mcp_tools_page_bounds_v1(0, 8, 1, 14, NULL), -1);
    check_mcp_page_bounds("mcp_page_full", 0, 14, 0, 14, 0, 14, 0, 0);
    check_mcp_page_bounds("mcp_page_first", 0, 8, 1, 14, 0, 8, 1, 8);
    check_mcp_page_bounds("mcp_page_second", 8, 8, 1, 14, 8, 14, 0, 0);
    check_mcp_page_bounds("mcp_page_negative_offset", -5, 3, 1, 14, 0, 3, 1, 3);
    check_mcp_page_bounds("mcp_page_over_offset", 99, 8, 1, 14, 14, 14, 0, 0);
    check_mcp_page_bounds("mcp_page_negative_limit", 2, -1, 0, 14, 2, 14, 0, 0);
    check_mcp_page_bounds("mcp_page_over_limit", 2, 99, 0, 14, 2, 14, 0, 0);
    check_mcp_page_bounds("mcp_page_negative_tool_count", 0, 5, 1, -1, 0, 0, 0, 0);
}

static void test_mcp_jsonrpc_summary_export(void) {
    char invalid_buf[8] = "keep";
    check_size("mcp_jsonrpc_summary_null",
               cbm_rs_mcp_jsonrpc_request_summary_v1(invalid_buf, sizeof(invalid_buf), NULL, 0),
               SIZE_MAX);
    check_str("mcp_jsonrpc_summary_null_buf", invalid_buf, "keep");
    check_size("mcp_jsonrpc_summary_negative_len",
               cbm_rs_mcp_jsonrpc_request_summary_v1(invalid_buf, sizeof(invalid_buf),
                                                     (const unsigned char *)"{}", -1),
               SIZE_MAX);
    check_size("mcp_jsonrpc_summary_not_json",
               cbm_rs_mcp_jsonrpc_request_summary_v1(invalid_buf, sizeof(invalid_buf),
                                                     (const unsigned char *)"not json", 8),
               SIZE_MAX);
    check_size("mcp_jsonrpc_summary_array_root",
               cbm_rs_mcp_jsonrpc_request_summary_v1(invalid_buf, sizeof(invalid_buf),
                                                     (const unsigned char *)"[1,2,3]", 7),
               SIZE_MAX);
    check_size("mcp_jsonrpc_summary_missing_method",
               cbm_rs_mcp_jsonrpc_request_summary_v1(
                   invalid_buf, sizeof(invalid_buf),
                   (const unsigned char *)"{\"jsonrpc\":\"2.0\",\"id\":1}", 24),
               SIZE_MAX);

    const unsigned char initialize[] = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\","
                                       "\"params\":{\"capabilities\":{}}}";
    const char *expected_initialize = "jsonrpc=2.0;method=initialize;id=int:1;params=object";
    char summary[256];
    check_size("mcp_jsonrpc_summary_initialize_len",
               cbm_rs_mcp_jsonrpc_request_summary_v1(summary, sizeof(summary), initialize,
                                                     (int)strlen((const char *)initialize)),
               strlen(expected_initialize));
    check_str("mcp_jsonrpc_summary_initialize", summary, expected_initialize);
    check_size("mcp_jsonrpc_summary_initialize_len_only",
               cbm_rs_mcp_jsonrpc_request_summary_v1(NULL, 0, initialize,
                                                     (int)strlen((const char *)initialize)),
               strlen(expected_initialize));
    char short_buf[12];
    check_size("mcp_jsonrpc_summary_short_len",
               cbm_rs_mcp_jsonrpc_request_summary_v1(short_buf, sizeof(short_buf), initialize,
                                                     (int)strlen((const char *)initialize)),
               strlen(expected_initialize));
    check_str("mcp_jsonrpc_summary_short", short_buf, "jsonrpc=2.0");

    const unsigned char notification[] =
        "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}";
    const char *expected_notification =
        "jsonrpc=2.0;method=notifications/initialized;id=none;params=none";
    check_size("mcp_jsonrpc_summary_notification_len",
               cbm_rs_mcp_jsonrpc_request_summary_v1(summary, sizeof(summary), notification,
                                                     (int)strlen((const char *)notification)),
               strlen(expected_notification));
    check_str("mcp_jsonrpc_summary_notification", summary, expected_notification);

    const unsigned char missing_jsonrpc[] = "{\"id\":1,\"method\":\"initialize\",\"params\":{}}";
    check_size("mcp_jsonrpc_summary_default_jsonrpc_len",
               cbm_rs_mcp_jsonrpc_request_summary_v1(summary, sizeof(summary), missing_jsonrpc,
                                                     (int)strlen((const char *)missing_jsonrpc)),
               strlen(expected_initialize));
    check_str("mcp_jsonrpc_summary_default_jsonrpc", summary, expected_initialize);

    const unsigned char string_id[] =
        "  { \"jsonrpc\" : \"2.0\" , \"id\" : \"99\" , \"method\" : \"tools/list\" }  ";
    const char *expected_string_id = "jsonrpc=2.0;method=tools/list;id=string:99;params=none";
    check_size("mcp_jsonrpc_summary_string_id_len",
               cbm_rs_mcp_jsonrpc_request_summary_v1(summary, sizeof(summary), string_id,
                                                     (int)strlen((const char *)string_id)),
               strlen(expected_string_id));
    check_str("mcp_jsonrpc_summary_string_id", summary, expected_string_id);

    const unsigned char tools_call[] =
        "{\"jsonrpc\":\"2.0\",\"id\":42,\"method\":\"tools/call\","
        "\"params\":{\"name\":\"search_graph\",\"arguments\":{\"limit\":5}}}";
    const char *expected_tools_call = "jsonrpc=2.0;method=tools/call;id=int:42;params=object";
    check_size("mcp_jsonrpc_summary_tools_call_len",
               cbm_rs_mcp_jsonrpc_request_summary_v1(summary, sizeof(summary), tools_call,
                                                     (int)strlen((const char *)tools_call)),
               strlen(expected_tools_call));
    check_str("mcp_jsonrpc_summary_tools_call", summary, expected_tools_call);

    const unsigned char delimiter_escape[] =
        "{\"method\":\"weird\\nmethod\",\"id\":\"a;b=c\\\\d\",\"params\":[]}";
    const char *expected_escape =
        "jsonrpc=2.0;method=weird\\nmethod;id=string:a\\;b\\=c\\\\d;params=array";
    check_size("mcp_jsonrpc_summary_escape_len",
               cbm_rs_mcp_jsonrpc_request_summary_v1(summary, sizeof(summary), delimiter_escape,
                                                     (int)strlen((const char *)delimiter_escape)),
               strlen(expected_escape));
    check_str("mcp_jsonrpc_summary_escape", summary, expected_escape);
}

static void test_mcp_initialize_protocol_version_export(void) {
    const char *latest = "2025-11-25";
    char version[32];

    check_size("mcp_initialize_protocol_null",
               cbm_rs_mcp_initialize_protocol_version_v1(version, sizeof(version), NULL, 0),
               strlen(latest));
    check_str("mcp_initialize_protocol_null_value", version, latest);
    check_size("mcp_initialize_protocol_negative_len",
               cbm_rs_mcp_initialize_protocol_version_v1(version, sizeof(version),
                                                         (const unsigned char *)"{}", -1),
               strlen(latest));
    check_str("mcp_initialize_protocol_negative_len_value", version, latest);

    const unsigned char supported[] = "{\"protocolVersion\":\"2025-06-18\"}";
    check_size("mcp_initialize_protocol_supported",
               cbm_rs_mcp_initialize_protocol_version_v1(version, sizeof(version), supported,
                                                         (int)strlen((const char *)supported)),
               strlen("2025-06-18"));
    check_str("mcp_initialize_protocol_supported_value", version, "2025-06-18");
    check_size("mcp_initialize_protocol_len_only",
               cbm_rs_mcp_initialize_protocol_version_v1(NULL, 0, supported,
                                                         (int)strlen((const char *)supported)),
               strlen("2025-06-18"));

    char short_buf[6];
    check_size("mcp_initialize_protocol_short",
               cbm_rs_mcp_initialize_protocol_version_v1(short_buf, sizeof(short_buf), supported,
                                                         (int)strlen((const char *)supported)),
               strlen("2025-06-18"));
    check_str("mcp_initialize_protocol_short_value", short_buf, "2025-");

    const unsigned char latest_supported[] = "{\"protocolVersion\":\"2025-11-25\"}";
    check_size(
        "mcp_initialize_protocol_latest",
        cbm_rs_mcp_initialize_protocol_version_v1(version, sizeof(version), latest_supported,
                                                  (int)strlen((const char *)latest_supported)),
        strlen(latest));
    check_str("mcp_initialize_protocol_latest_value", version, latest);

    const unsigned char old_supported[] = "{\"protocolVersion\":\"2024-11-05\"}";
    check_size("mcp_initialize_protocol_old",
               cbm_rs_mcp_initialize_protocol_version_v1(version, sizeof(version), old_supported,
                                                         (int)strlen((const char *)old_supported)),
               strlen("2024-11-05"));
    check_str("mcp_initialize_protocol_old_value", version, "2024-11-05");

    const unsigned char missing[] = "{\"capabilities\":{}}";
    check_size("mcp_initialize_protocol_missing",
               cbm_rs_mcp_initialize_protocol_version_v1(version, sizeof(version), missing,
                                                         (int)strlen((const char *)missing)),
               strlen(latest));
    check_str("mcp_initialize_protocol_missing_value", version, latest);

    const unsigned char unknown[] = "{\"protocolVersion\":\"9999-01-01\"}";
    check_size("mcp_initialize_protocol_unknown",
               cbm_rs_mcp_initialize_protocol_version_v1(version, sizeof(version), unknown,
                                                         (int)strlen((const char *)unknown)),
               strlen(latest));
    check_str("mcp_initialize_protocol_unknown_value", version, latest);

    const unsigned char non_string[] = "{\"protocolVersion\":20250618}";
    check_size("mcp_initialize_protocol_non_string",
               cbm_rs_mcp_initialize_protocol_version_v1(version, sizeof(version), non_string,
                                                         (int)strlen((const char *)non_string)),
               strlen(latest));
    check_str("mcp_initialize_protocol_non_string_value", version, latest);

    const unsigned char invalid_json[] = "{\"protocolVersion\":}";
    check_size("mcp_initialize_protocol_invalid",
               cbm_rs_mcp_initialize_protocol_version_v1(version, sizeof(version), invalid_json,
                                                         (int)strlen((const char *)invalid_json)),
               strlen(latest));
    check_str("mcp_initialize_protocol_invalid_value", version, latest);

    const unsigned char root_array[] = "[]";
    check_size("mcp_initialize_protocol_root_array",
               cbm_rs_mcp_initialize_protocol_version_v1(version, sizeof(version), root_array,
                                                         (int)strlen((const char *)root_array)),
               strlen(latest));
    check_str("mcp_initialize_protocol_root_array_value", version, latest);

    const unsigned char duplicate[] =
        "{\"protocolVersion\":\"2024-11-05\",\"protocolVersion\":\"2025-11-25\"}";
    check_size("mcp_initialize_protocol_dup_first",
               cbm_rs_mcp_initialize_protocol_version_v1(version, sizeof(version), duplicate,
                                                         (int)strlen((const char *)duplicate)),
               strlen("2024-11-05"));
    check_str("mcp_initialize_protocol_dup_first_value", version, "2024-11-05");

    const unsigned char duplicate_non_string[] =
        "{\"protocolVersion\":7,\"protocolVersion\":\"2024-11-05\"}";
    check_size(
        "mcp_initialize_protocol_dup_non_string_first",
        cbm_rs_mcp_initialize_protocol_version_v1(version, sizeof(version), duplicate_non_string,
                                                  (int)strlen((const char *)duplicate_non_string)),
        strlen(latest));
    check_str("mcp_initialize_protocol_dup_non_string_first_value", version, latest);

    const unsigned char trailing[] = "{} trailing";
    check_size("mcp_initialize_protocol_trailing",
               cbm_rs_mcp_initialize_protocol_version_v1(version, sizeof(version), trailing,
                                                         (int)strlen((const char *)trailing)),
               strlen(latest));
    check_str("mcp_initialize_protocol_trailing_value", version, latest);
}

static void test_mcp_initialize_response_export(void) {
    const char *latest =
        "{\"protocolVersion\":\"2025-11-25\",\"serverInfo\":{\"name\":\"codebase-memory-mcp\","
        "\"version\":\"0.10.0\"},\"capabilities\":{\"tools\":{\"listChanged\":false}}}";
    char response[256];

    check_size("mcp_initialize_response_null",
               cbm_rs_mcp_initialize_response_v1(response, sizeof(response), NULL, 0),
               strlen(latest));
    check_str("mcp_initialize_response_null_value", response, latest);
    check_size("mcp_initialize_response_len_only",
               cbm_rs_mcp_initialize_response_v1(NULL, 0, NULL, 0), strlen(latest));

    const unsigned char supported[] = "{\"protocolVersion\":\"2024-11-05\"}";
    const char *old =
        "{\"protocolVersion\":\"2024-11-05\",\"serverInfo\":{\"name\":\"codebase-memory-mcp\","
        "\"version\":\"0.10.0\"},\"capabilities\":{\"tools\":{\"listChanged\":false}}}";
    check_size("mcp_initialize_response_supported",
               cbm_rs_mcp_initialize_response_v1(response, sizeof(response), supported,
                                                 (int)strlen((const char *)supported)),
               strlen(old));
    check_str("mcp_initialize_response_supported_value", response, old);

    const unsigned char unknown[] = "{\"protocolVersion\":\"9999-01-01\"}";
    check_size("mcp_initialize_response_unknown",
               cbm_rs_mcp_initialize_response_v1(response, sizeof(response), unknown,
                                                 (int)strlen((const char *)unknown)),
               strlen(latest));
    check_str("mcp_initialize_response_unknown_value", response, latest);

    const unsigned char duplicate[] =
        "{\"protocolVersion\":\"2024-11-05\",\"protocolVersion\":\"2025-11-25\"}";
    check_size("mcp_initialize_response_dup_first",
               cbm_rs_mcp_initialize_response_v1(response, sizeof(response), duplicate,
                                                 (int)strlen((const char *)duplicate)),
               strlen(old));
    check_str("mcp_initialize_response_dup_first_value", response, old);

    char short_buf[20];
    check_size("mcp_initialize_response_short",
               cbm_rs_mcp_initialize_response_v1(short_buf, sizeof(short_buf), NULL, 0),
               strlen(latest));
    check_str("mcp_initialize_response_short_value", short_buf, "{\"protocolVersion\":");

    const unsigned char trailing[] = "{} trailing";
    check_size("mcp_initialize_response_trailing",
               cbm_rs_mcp_initialize_response_v1(response, sizeof(response), trailing,
                                                 (int)strlen((const char *)trailing)),
               strlen(latest));
    check_str("mcp_initialize_response_trailing_value", response, latest);
}

static void test_mcp_tools_call_name_export(void) {
    char invalid_buf[8] = "keep";
    check_size("mcp_call_name_null",
               cbm_rs_mcp_tools_call_name_v1(invalid_buf, sizeof(invalid_buf), NULL, 0), SIZE_MAX);
    check_str("mcp_call_name_null_buf", invalid_buf, "keep");
    check_size("mcp_call_name_negative_len",
               cbm_rs_mcp_tools_call_name_v1(invalid_buf, sizeof(invalid_buf),
                                             (const unsigned char *)"{}", -1),
               SIZE_MAX);
    check_size("mcp_call_name_not_json",
               cbm_rs_mcp_tools_call_name_v1(invalid_buf, sizeof(invalid_buf),
                                             (const unsigned char *)"not json", 8),
               SIZE_MAX);
    check_size("mcp_call_name_array_root",
               cbm_rs_mcp_tools_call_name_v1(invalid_buf, sizeof(invalid_buf),
                                             (const unsigned char *)"[]", 2),
               SIZE_MAX);
    check_size("mcp_call_name_missing",
               cbm_rs_mcp_tools_call_name_v1(invalid_buf, sizeof(invalid_buf),
                                             (const unsigned char *)"{\"arguments\":{}}", 16),
               SIZE_MAX);
    check_size(
        "mcp_call_name_non_string_first",
        cbm_rs_mcp_tools_call_name_v1(invalid_buf, sizeof(invalid_buf),
                                      (const unsigned char *)"{\"name\":7,\"name\":\"late\"}", 24),
        SIZE_MAX);

    const unsigned char params[] = "{\"name\":\"search_graph\",\"arguments\":{\"limit\":5}}";
    char name[64];
    check_size("mcp_call_name_len",
               cbm_rs_mcp_tools_call_name_v1(name, sizeof(name), params,
                                             (int)strlen((const char *)params)),
               strlen("search_graph"));
    check_str("mcp_call_name", name, "search_graph");
    check_size("mcp_call_name_len_only",
               cbm_rs_mcp_tools_call_name_v1(NULL, 0, params, (int)strlen((const char *)params)),
               strlen("search_graph"));
    char short_buf[7];
    check_size("mcp_call_name_short_len",
               cbm_rs_mcp_tools_call_name_v1(short_buf, sizeof(short_buf), params,
                                             (int)strlen((const char *)params)),
               strlen("search_graph"));
    check_str("mcp_call_name_short", short_buf, "search");

    const unsigned char duplicate[] = "{\"name\":\"first\",\"name\":\"second\"}";
    check_size("mcp_call_name_duplicate_len",
               cbm_rs_mcp_tools_call_name_v1(name, sizeof(name), duplicate,
                                             (int)strlen((const char *)duplicate)),
               strlen("first"));
    check_str("mcp_call_name_duplicate", name, "first");

    const unsigned char unicode[] = "{\"name\":\"\\u641c\\u5c0b\"}";
    check_size("mcp_call_name_unicode_len",
               cbm_rs_mcp_tools_call_name_v1(name, sizeof(name), unicode,
                                             (int)strlen((const char *)unicode)),
               strlen("搜尋"));
    check_str("mcp_call_name_unicode", name, "搜尋");
}

static void test_mcp_tools_call_arguments_export(void) {
    char invalid_buf[8] = "keep";
    check_size("mcp_call_args_null",
               cbm_rs_mcp_tools_call_arguments_v1(invalid_buf, sizeof(invalid_buf), NULL, 0),
               SIZE_MAX);
    check_str("mcp_call_args_null_buf", invalid_buf, "keep");
    check_size("mcp_call_args_negative_len",
               cbm_rs_mcp_tools_call_arguments_v1(invalid_buf, sizeof(invalid_buf),
                                                  (const unsigned char *)"{}", -1),
               SIZE_MAX);
    check_size("mcp_call_args_not_json",
               cbm_rs_mcp_tools_call_arguments_v1(invalid_buf, sizeof(invalid_buf),
                                                  (const unsigned char *)"not json", 8),
               SIZE_MAX);
    check_size("mcp_call_args_array_root",
               cbm_rs_mcp_tools_call_arguments_v1(invalid_buf, sizeof(invalid_buf),
                                                  (const unsigned char *)"[]", 2),
               SIZE_MAX);

    const unsigned char missing[] = "{\"name\":\"tool\"}";
    char args[128];
    check_size("mcp_call_args_missing_len",
               cbm_rs_mcp_tools_call_arguments_v1(args, sizeof(args), missing,
                                                  (int)strlen((const char *)missing)),
               strlen("{}"));
    check_str("mcp_call_args_missing", args, "{}");

    const unsigned char params[] = "{\"name\":\"search_graph\",\"arguments\":{\"limit\":5}}";
    check_size("mcp_call_args_len",
               cbm_rs_mcp_tools_call_arguments_v1(args, sizeof(args), params,
                                                  (int)strlen((const char *)params)),
               strlen("{\"limit\":5}"));
    check_str("mcp_call_args", args, "{\"limit\":5}");
    check_size(
        "mcp_call_args_len_only",
        cbm_rs_mcp_tools_call_arguments_v1(NULL, 0, params, (int)strlen((const char *)params)),
        strlen("{\"limit\":5}"));
    char short_buf[8];
    check_size("mcp_call_args_short_len",
               cbm_rs_mcp_tools_call_arguments_v1(short_buf, sizeof(short_buf), params,
                                                  (int)strlen((const char *)params)),
               strlen("{\"limit\":5}"));
    check_str("mcp_call_args_short", short_buf, "{\"limit");

    const unsigned char duplicate[] = "{\"arguments\":{\"first\":1},\"arguments\":{\"second\":2}}";
    check_size("mcp_call_args_duplicate_len",
               cbm_rs_mcp_tools_call_arguments_v1(args, sizeof(args), duplicate,
                                                  (int)strlen((const char *)duplicate)),
               strlen("{\"first\":1}"));
    check_str("mcp_call_args_duplicate", args, "{\"first\":1}");

    const unsigned char array_args[] = "{\"arguments\":[1,true,null]}";
    check_size("mcp_call_args_array_len",
               cbm_rs_mcp_tools_call_arguments_v1(args, sizeof(args), array_args,
                                                  (int)strlen((const char *)array_args)),
               strlen("[1,true,null]"));
    check_str("mcp_call_args_array", args, "[1,true,null]");

    const unsigned char spaced[] = "{\"arguments\" : { \"spaced\" : true } }";
    check_size("mcp_call_args_spaced_len",
               cbm_rs_mcp_tools_call_arguments_v1(args, sizeof(args), spaced,
                                                  (int)strlen((const char *)spaced)),
               strlen("{ \"spaced\" : true }"));
    check_str("mcp_call_args_spaced", args, "{ \"spaced\" : true }");

    const unsigned char invalid_utf8[] = "{\"arguments\":\"bad\xff\"}";
    check_size("mcp_call_args_invalid_utf8",
               cbm_rs_mcp_tools_call_arguments_v1(args, sizeof(args), invalid_utf8,
                                                  (int)strlen((const char *)invalid_utf8)),
               SIZE_MAX);
}

static void test_mcp_get_string_arg_export(void) {
    const unsigned char args[] = "{\"label\":\"Function\",\"name_pattern\":\".*Order.*\"}";
    char value[64];
    check_size("mcp_get_string_arg_label_len",
               cbm_rs_mcp_get_string_arg_v1(value, sizeof(value), args,
                                            (int)strlen((const char *)args), "label"),
               strlen("Function"));
    check_str("mcp_get_string_arg_label", value, "Function");
    check_size("mcp_get_string_arg_pattern_len",
               cbm_rs_mcp_get_string_arg_v1(value, sizeof(value), args,
                                            (int)strlen((const char *)args), "name_pattern"),
               strlen(".*Order.*"));
    check_str("mcp_get_string_arg_pattern", value, ".*Order.*");
    check_size(
        "mcp_get_string_arg_len_only",
        cbm_rs_mcp_get_string_arg_v1(NULL, 0, args, (int)strlen((const char *)args), "label"),
        strlen("Function"));

    char short_buf[5];
    check_size("mcp_get_string_arg_short_len",
               cbm_rs_mcp_get_string_arg_v1(short_buf, sizeof(short_buf), args,
                                            (int)strlen((const char *)args), "label"),
               strlen("Function"));
    check_str("mcp_get_string_arg_short", short_buf, "Func");

    const unsigned char unicode[] = "{\"name\":\"\\u641c\\u5c0b\"}";
    check_size("mcp_get_string_arg_unicode_len",
               cbm_rs_mcp_get_string_arg_v1(value, sizeof(value), unicode,
                                            (int)strlen((const char *)unicode), "name"),
               strlen("搜尋"));
    check_str("mcp_get_string_arg_unicode", value, "搜尋");

    const unsigned char duplicate[] = "{\"name\":\"first\",\"name\":\"second\"}";
    check_size("mcp_get_string_arg_duplicate_len",
               cbm_rs_mcp_get_string_arg_v1(value, sizeof(value), duplicate,
                                            (int)strlen((const char *)duplicate), "name"),
               strlen("first"));
    check_str("mcp_get_string_arg_duplicate", value, "first");

    check_size("mcp_get_string_arg_missing",
               cbm_rs_mcp_get_string_arg_v1(value, sizeof(value), args,
                                            (int)strlen((const char *)args), "missing"),
               SIZE_MAX);
    check_size("mcp_get_string_arg_non_string",
               cbm_rs_mcp_get_string_arg_v1(value, sizeof(value),
                                            (const unsigned char *)"{\"count\":42}", 12, "count"),
               SIZE_MAX);
    check_size("mcp_get_string_arg_first_non_string",
               cbm_rs_mcp_get_string_arg_v1(value, sizeof(value),
                                            (const unsigned char *)"{\"name\":7,\"name\":\"late\"}",
                                            24, "name"),
               SIZE_MAX);
    check_size("mcp_get_string_arg_null_input",
               cbm_rs_mcp_get_string_arg_v1(value, sizeof(value), NULL, 0, "name"), SIZE_MAX);
    check_size("mcp_get_string_arg_null_key",
               cbm_rs_mcp_get_string_arg_v1(value, sizeof(value), args,
                                            (int)strlen((const char *)args), NULL),
               SIZE_MAX);
    check_size("mcp_get_string_arg_empty_key",
               cbm_rs_mcp_get_string_arg_v1(value, sizeof(value), args,
                                            (int)strlen((const char *)args), ""),
               SIZE_MAX);
    check_size("mcp_get_string_arg_not_json",
               cbm_rs_mcp_get_string_arg_v1(value, sizeof(value), (const unsigned char *)"not json",
                                            8, "name"),
               SIZE_MAX);
    check_size("mcp_get_string_arg_trailing",
               cbm_rs_mcp_get_string_arg_v1(value, sizeof(value),
                                            (const unsigned char *)"{\"name\":\"x\"} trailing", 21,
                                            "name"),
               SIZE_MAX);
    const unsigned char invalid_utf8[] = "{\"name\":\"bad\xff\"}";
    check_size("mcp_get_string_arg_invalid_utf8",
               cbm_rs_mcp_get_string_arg_v1(value, sizeof(value), invalid_utf8,
                                            (int)strlen((const char *)invalid_utf8), "name"),
               SIZE_MAX);
}

static void test_mcp_get_int_bool_arg_exports(void) {
    const unsigned char args[] =
        "{\"limit\":10,\"offset\":5,\"include_connected\":true,\"regex\":false}";
    check_int("mcp_get_int_arg_limit",
              cbm_rs_mcp_get_int_arg_v1(args, (int)strlen((const char *)args), "limit", 0), 10);
    check_int("mcp_get_int_arg_offset",
              cbm_rs_mcp_get_int_arg_v1(args, (int)strlen((const char *)args), "offset", 0), 5);
    check_int("mcp_get_int_arg_missing",
              cbm_rs_mcp_get_int_arg_v1(args, (int)strlen((const char *)args), "missing", 42), 42);
    check_int(
        "mcp_get_int_arg_string_default",
        cbm_rs_mcp_get_int_arg_v1((const unsigned char *)"{\"limit\":\"ten\"}", 15, "limit", 5), 5);
    check_int("mcp_get_int_arg_bool_default",
              cbm_rs_mcp_get_int_arg_v1((const unsigned char *)"{\"flag\":true}", 13, "flag", -1),
              -1);
    check_int("mcp_get_int_arg_duplicate_first",
              cbm_rs_mcp_get_int_arg_v1((const unsigned char *)"{\"limit\":7,\"limit\":8}", 21,
                                        "limit", 0),
              7);
    check_int("mcp_get_int_arg_first_non_int",
              cbm_rs_mcp_get_int_arg_v1((const unsigned char *)"{\"limit\":\"ten\",\"limit\":8}",
                                        25, "limit", 5),
              5);
    check_int("mcp_get_int_arg_float_default",
              cbm_rs_mcp_get_int_arg_v1((const unsigned char *)"{\"limit\":7.5}", 13, "limit", 5),
              5);
    check_int("mcp_get_int_arg_null_input", cbm_rs_mcp_get_int_arg_v1(NULL, 0, "limit", 5), 5);
    check_int("mcp_get_int_arg_null_key",
              cbm_rs_mcp_get_int_arg_v1(args, (int)strlen((const char *)args), NULL, 5), 5);
    check_int("mcp_get_int_arg_empty_key",
              cbm_rs_mcp_get_int_arg_v1(args, (int)strlen((const char *)args), "", 5), 5);
    check_int("mcp_get_int_arg_not_json",
              cbm_rs_mcp_get_int_arg_v1((const unsigned char *)"not json", 8, "limit", 5), 5);
    check_int(
        "mcp_get_int_arg_trailing",
        cbm_rs_mcp_get_int_arg_v1((const unsigned char *)"{\"limit\":7} trailing", 20, "limit", 5),
        5);

    check_int(
        "mcp_get_bool_arg_true",
        cbm_rs_mcp_get_bool_arg_v1(args, (int)strlen((const char *)args), "include_connected"), 1);
    check_int("mcp_get_bool_arg_false",
              cbm_rs_mcp_get_bool_arg_v1(args, (int)strlen((const char *)args), "regex"), 0);
    check_int("mcp_get_bool_arg_missing",
              cbm_rs_mcp_get_bool_arg_v1(args, (int)strlen((const char *)args), "missing"), 0);
    check_int("mcp_get_bool_arg_int_false",
              cbm_rs_mcp_get_bool_arg_v1((const unsigned char *)"{\"flag\":1}", 10, "flag"), 0);
    check_int("mcp_get_bool_arg_string_false",
              cbm_rs_mcp_get_bool_arg_v1((const unsigned char *)"{\"flag\":\"true\"}", 15, "flag"),
              0);
    check_int("mcp_get_bool_arg_duplicate_first",
              cbm_rs_mcp_get_bool_arg_v1((const unsigned char *)"{\"flag\":true,\"flag\":false}",
                                         26, "flag"),
              1);
    check_int(
        "mcp_get_bool_arg_first_non_bool",
        cbm_rs_mcp_get_bool_arg_v1((const unsigned char *)"{\"flag\":1,\"flag\":true}", 22, "flag"),
        0);
    check_int("mcp_get_bool_arg_null_input", cbm_rs_mcp_get_bool_arg_v1(NULL, 0, "flag"), 0);
    check_int("mcp_get_bool_arg_null_key",
              cbm_rs_mcp_get_bool_arg_v1(args, (int)strlen((const char *)args), NULL), 0);
    check_int("mcp_get_bool_arg_empty_key",
              cbm_rs_mcp_get_bool_arg_v1(args, (int)strlen((const char *)args), ""), 0);
    check_int("mcp_get_bool_arg_not_json",
              cbm_rs_mcp_get_bool_arg_v1((const unsigned char *)"not json", 8, "flag"), 0);
    check_int(
        "mcp_get_bool_arg_trailing",
        cbm_rs_mcp_get_bool_arg_v1((const unsigned char *)"{\"flag\":true} trailing", 22, "flag"),
        0);
    const unsigned char invalid_utf8[] = "{\"flag\":\"bad\xff\"}";
    check_int(
        "mcp_get_bool_arg_invalid_utf8",
        cbm_rs_mcp_get_bool_arg_v1(invalid_utf8, (int)strlen((const char *)invalid_utf8), "flag"),
        0);
}

static void test_mcp_edge_type_valid_export(void) {
    check_int("mcp_edge_type_null", cbm_rs_mcp_edge_type_valid_v1(NULL), 0);
    check_int("mcp_edge_type_empty", cbm_rs_mcp_edge_type_valid_v1(""), 1);
    check_int("mcp_edge_type_calls", cbm_rs_mcp_edge_type_valid_v1("CALLS"), 1);
    check_int("mcp_edge_type_defines_method", cbm_rs_mcp_edge_type_valid_v1("DEFINES_METHOD"), 1);
    check_int("mcp_edge_type_underscore", cbm_rs_mcp_edge_type_valid_v1("_"), 1);
    check_int("mcp_edge_type_lowercase", cbm_rs_mcp_edge_type_valid_v1("calls"), 0);
    check_int("mcp_edge_type_dash", cbm_rs_mcp_edge_type_valid_v1("CALLS-TO"), 0);
    check_int("mcp_edge_type_digit", cbm_rs_mcp_edge_type_valid_v1("CALLS1"), 0);
    check_int("mcp_edge_type_space", cbm_rs_mcp_edge_type_valid_v1("CALLS "), 0);
    check_int("mcp_edge_type_unicode", cbm_rs_mcp_edge_type_valid_v1("呼叫"), 0);
    check_int("mcp_edge_type_len64",
              cbm_rs_mcp_edge_type_valid_v1(
                  "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKL"),
              1);
    check_int("mcp_edge_type_len65",
              cbm_rs_mcp_edge_type_valid_v1(
                  "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLM"),
              0);
#ifdef CBM_USE_RUST_MCP_EDGE_TYPE_ONLY
    check_bool("mcp_edge_type_direct_calls", cbm_mcp_edge_type_valid("CALLS"), true);
    check_bool("mcp_edge_type_direct_lower", cbm_mcp_edge_type_valid("calls"), false);
    check_bool("mcp_edge_type_direct_null", cbm_mcp_edge_type_valid(NULL), false);
#endif
}

static void test_mcp_search_path_arg_valid_export(void) {
    check_int("mcp_search_path_null", cbm_rs_mcp_search_path_arg_valid_v1(NULL), 0);
    check_int("mcp_search_path_empty", cbm_rs_mcp_search_path_arg_valid_v1(""), 1);
    check_int("mcp_search_path_simple", cbm_rs_mcp_search_path_arg_valid_v1("src/main.go"), 1);
    check_int("mcp_search_path_ampersand", cbm_rs_mcp_search_path_arg_valid_v1("*R&D*.go"), 1);
    check_int("mcp_search_path_space", cbm_rs_mcp_search_path_arg_valid_v1("apps/foo bar/*.ts"), 1);
    check_int("mcp_search_path_single_quote", cbm_rs_mcp_search_path_arg_valid_v1("'"), 0);
    check_int("mcp_search_path_double_quote", cbm_rs_mcp_search_path_arg_valid_v1("\""), 0);
    check_int("mcp_search_path_semicolon", cbm_rs_mcp_search_path_arg_valid_v1(";"), 0);
    check_int("mcp_search_path_pipe", cbm_rs_mcp_search_path_arg_valid_v1("|"), 0);
    check_int("mcp_search_path_dollar", cbm_rs_mcp_search_path_arg_valid_v1("$"), 0);
    check_int("mcp_search_path_backtick", cbm_rs_mcp_search_path_arg_valid_v1("`"), 0);
    check_int("mcp_search_path_lt", cbm_rs_mcp_search_path_arg_valid_v1("<"), 0);
    check_int("mcp_search_path_gt", cbm_rs_mcp_search_path_arg_valid_v1(">"), 0);
    check_int("mcp_search_path_lf", cbm_rs_mcp_search_path_arg_valid_v1("src\nmain.go"), 0);
    check_int("mcp_search_path_cr", cbm_rs_mcp_search_path_arg_valid_v1("src\rmain.go"), 0);
#ifndef _WIN32
    check_int("mcp_search_path_backslash", cbm_rs_mcp_search_path_arg_valid_v1("src\\main.go"), 0);
#else
    check_int("mcp_search_path_backslash", cbm_rs_mcp_search_path_arg_valid_v1("src\\main.go"), 1);
#endif
}

static void test_mcp_search_args_valid_export(void) {
    check_int("mcp_search_args_root_null", cbm_rs_mcp_search_args_valid_v1(NULL, NULL), 0);
    check_int("mcp_search_args_root_empty", cbm_rs_mcp_search_args_valid_v1("", NULL), 1);
    check_int("mcp_search_args_root_only", cbm_rs_mcp_search_args_valid_v1("src/main.go", NULL), 1);
    check_int("mcp_search_args_with_pattern",
              cbm_rs_mcp_search_args_valid_v1("src/main.go", "*.go"), 1);
    check_int("mcp_search_args_ampersand",
              cbm_rs_mcp_search_args_valid_v1("apps/foo bar", "*R&D*.go"), 1);
    check_int("mcp_search_args_bad_root", cbm_rs_mcp_search_args_valid_v1("'", "*.go"), 0);
    check_int("mcp_search_args_bad_pattern", cbm_rs_mcp_search_args_valid_v1("src/main.go", ";"),
              0);
    check_int("mcp_search_args_bad_pattern_lf",
              cbm_rs_mcp_search_args_valid_v1("src/main.go", "bad\npattern"), 0);
}

static void test_mcp_strip_root_prefix_offset_export(void) {
    check_size("mcp_strip_root_null_path",
               cbm_rs_mcp_strip_root_prefix_offset_v1(NULL, "/repo", strlen("/repo")), 0);
    check_size("mcp_strip_root_null_root",
               cbm_rs_mcp_strip_root_prefix_offset_v1("/repo/a.c", NULL, strlen("/repo")), 0);
    check_size("mcp_strip_root_child",
               cbm_rs_mcp_strip_root_prefix_offset_v1("/repo/src/a.c", "/repo", strlen("/repo")),
               strlen("/repo/"));
    check_size("mcp_strip_root_exact",
               cbm_rs_mcp_strip_root_prefix_offset_v1("/repo", "/repo", strlen("/repo")),
               strlen("/repo"));
    check_size("mcp_strip_root_no_component_boundary",
               cbm_rs_mcp_strip_root_prefix_offset_v1("/repo/submarine.c", "/repo/sub",
                                                      strlen("/repo/sub")),
               strlen("/repo/sub"));
    check_size("mcp_strip_root_trailing_slash",
               cbm_rs_mcp_strip_root_prefix_offset_v1("/repo/src/a.c", "/repo/", strlen("/repo/")),
               strlen("/repo/"));
    check_size("mcp_strip_root_no_match",
               cbm_rs_mcp_strip_root_prefix_offset_v1("/other/a.c", "/repo", strlen("/repo")), 0);
    check_size("mcp_strip_root_short_path",
               cbm_rs_mcp_strip_root_prefix_offset_v1("/repo", "/repo-long", strlen("/repo-long")),
               0);
    check_size("mcp_strip_root_zero_len",
               cbm_rs_mcp_strip_root_prefix_offset_v1("/repo/a.c", "/repo", 0), 1);
}

static void test_mcp_search_mode_export(void) {
    check_int("mcp_search_mode_null", cbm_rs_mcp_search_mode_v1(NULL), 0);
    check_int("mcp_search_mode_empty", cbm_rs_mcp_search_mode_v1(""), 0);
    check_int("mcp_search_mode_compact", cbm_rs_mcp_search_mode_v1("compact"), 0);
    check_int("mcp_search_mode_full", cbm_rs_mcp_search_mode_v1("full"), 1);
    check_int("mcp_search_mode_files", cbm_rs_mcp_search_mode_v1("files"), 2);
    check_int("mcp_search_mode_case", cbm_rs_mcp_search_mode_v1("Full"), 0);
    check_int("mcp_search_mode_trailing", cbm_rs_mcp_search_mode_v1("files "), 0);
    check_int("mcp_search_mode_unknown", cbm_rs_mcp_search_mode_v1("unknown"), 0);
}

static void test_mcp_index_mode_export(void) {
    check_int("mcp_index_mode_null", cbm_rs_mcp_index_mode_v1(NULL), CBM_MODE_FULL);
    check_int("mcp_index_mode_empty", cbm_rs_mcp_index_mode_v1(""), CBM_MODE_FULL);
    check_int("mcp_index_mode_full", cbm_rs_mcp_index_mode_v1("full"), CBM_MODE_FULL);
    check_int("mcp_index_mode_moderate", cbm_rs_mcp_index_mode_v1("moderate"), CBM_MODE_MODERATE);
    check_int("mcp_index_mode_fast", cbm_rs_mcp_index_mode_v1("fast"), CBM_MODE_FAST);
    check_int("mcp_index_mode_cross_repo", cbm_rs_mcp_index_mode_v1("cross-repo-intelligence"), 3);
    check_int("mcp_index_mode_case", cbm_rs_mcp_index_mode_v1("Fast"), CBM_MODE_FULL);
    check_int("mcp_index_mode_trailing", cbm_rs_mcp_index_mode_v1("fast "), CBM_MODE_FULL);
    check_int("mcp_index_mode_unknown", cbm_rs_mcp_index_mode_v1("unknown"), CBM_MODE_FULL);
}

static void test_mcp_trace_mode_edge_mask_export(void) {
    enum {
        TRACE_EDGE_CALLS = 1u << 0,
        TRACE_EDGE_DATA_FLOWS = 1u << 1,
        TRACE_EDGE_HTTP_CALLS = 1u << 2,
        TRACE_EDGE_ASYNC_CALLS = 1u << 3,
        TRACE_EDGE_CROSS_HTTP_CALLS = 1u << 4,
        TRACE_EDGE_CROSS_ASYNC_CALLS = 1u << 5,
        TRACE_EDGE_CROSS_CHANNEL = 1u << 6,
        TRACE_EDGE_CROSS_GRPC_CALLS = 1u << 7,
        TRACE_EDGE_CROSS_GRAPHQL_CALLS = 1u << 8,
        TRACE_EDGE_CROSS_TRPC_CALLS = 1u << 9,
    };
    uint32_t cross_service =
        TRACE_EDGE_HTTP_CALLS | TRACE_EDGE_ASYNC_CALLS | TRACE_EDGE_DATA_FLOWS | TRACE_EDGE_CALLS |
        TRACE_EDGE_CROSS_HTTP_CALLS | TRACE_EDGE_CROSS_ASYNC_CALLS | TRACE_EDGE_CROSS_CHANNEL |
        TRACE_EDGE_CROSS_GRPC_CALLS | TRACE_EDGE_CROSS_GRAPHQL_CALLS | TRACE_EDGE_CROSS_TRPC_CALLS;

    check_int("mcp_trace_mode_edges_null", (int)cbm_rs_mcp_trace_mode_edge_mask_v1(NULL),
              TRACE_EDGE_CALLS);
    check_int("mcp_trace_mode_edges_empty", (int)cbm_rs_mcp_trace_mode_edge_mask_v1(""),
              TRACE_EDGE_CALLS);
    check_int("mcp_trace_mode_edges_calls", (int)cbm_rs_mcp_trace_mode_edge_mask_v1("calls"),
              TRACE_EDGE_CALLS);
    check_int("mcp_trace_mode_edges_data_flow",
              (int)cbm_rs_mcp_trace_mode_edge_mask_v1("data_flow"),
              TRACE_EDGE_CALLS | TRACE_EDGE_DATA_FLOWS);
    check_int("mcp_trace_mode_edges_cross_service",
              (int)cbm_rs_mcp_trace_mode_edge_mask_v1("cross_service"), (int)cross_service);
    check_int("mcp_trace_mode_edges_case", (int)cbm_rs_mcp_trace_mode_edge_mask_v1("Data_Flow"),
              TRACE_EDGE_CALLS);
    check_int("mcp_trace_mode_edges_trailing",
              (int)cbm_rs_mcp_trace_mode_edge_mask_v1("data_flow "), TRACE_EDGE_CALLS);
    check_int("mcp_trace_mode_edges_unknown", (int)cbm_rs_mcp_trace_mode_edge_mask_v1("unknown"),
              TRACE_EDGE_CALLS);
}

static void test_mcp_sanitize_ascii_export(void) {
    cbm_rs_mcp_sanitize_ascii_in_place_v1(NULL);

    char ascii[] = "abc XYZ 123 \n\t";
    cbm_rs_mcp_sanitize_ascii_in_place_v1(ascii);
    check_str("mcp_sanitize_ascii_plain", ascii, "abc XYZ 123 \n\t");

    char mixed[] = {'a', (char)0x7f, (char)0x80, (char)0xc3, (char)0xa9, 'z', (char)0xff, 0};
    cbm_rs_mcp_sanitize_ascii_in_place_v1(mixed);
    check_int("mcp_sanitize_ascii_a", mixed[0], 'a');
    check_int("mcp_sanitize_ascii_7f", (unsigned char)mixed[1], 0x7f);
    check_int("mcp_sanitize_ascii_80", mixed[2], '?');
    check_int("mcp_sanitize_ascii_c3", mixed[3], '?');
    check_int("mcp_sanitize_ascii_a9", mixed[4], '?');
    check_int("mcp_sanitize_ascii_z", mixed[5], 'z');
    check_int("mcp_sanitize_ascii_ff", mixed[6], '?');

    char empty[] = "";
    cbm_rs_mcp_sanitize_ascii_in_place_v1(empty);
    check_str("mcp_sanitize_ascii_empty", empty, "");
}

static void test_mcp_search_code_score_export(void) {
    check_int("mcp_search_score_null", cbm_rs_mcp_search_code_score_v1(NULL, NULL, 7), 7);
    check_int("mcp_search_score_function",
              cbm_rs_mcp_search_code_score_v1("Function", "src/app.c", 3), 13);
    check_int("mcp_search_score_method", cbm_rs_mcp_search_code_score_v1("Method", "src/app.c", 3),
              13);
    check_int("mcp_search_score_route", cbm_rs_mcp_search_code_score_v1("Route", "src/app.c", 3),
              18);
    check_int("mcp_search_score_class", cbm_rs_mcp_search_code_score_v1("Class", "src/app.c", 3),
              3);
    check_int("mcp_search_score_case", cbm_rs_mcp_search_code_score_v1("function", "src/app.c", 3),
              3);
    check_int("mcp_search_score_vendored",
              cbm_rs_mcp_search_code_score_v1("Function", "vendored/lib.c", 3), -37);
    check_int("mcp_search_score_vendor",
              cbm_rs_mcp_search_code_score_v1("Function", "vendor/lib.c", 3), -37);
    check_int("mcp_search_score_node_modules",
              cbm_rs_mcp_search_code_score_v1("Function", "node_modules/pkg/index.js", 3), -37);
    check_int("mcp_search_score_test",
              cbm_rs_mcp_search_code_score_v1("Function", "src/test_helper.c", 3), 8);
    check_int("mcp_search_score_spec",
              cbm_rs_mcp_search_code_score_v1("Function", "src/spec/helper.c", 3), 8);
    check_int("mcp_search_score_underscore_test",
              cbm_rs_mcp_search_code_score_v1("Function", "src/foo_test.c", 3), 8);
    check_int("mcp_search_score_vendor_test",
              cbm_rs_mcp_search_code_score_v1("Route", "vendor/test_api.c", 3), -37);
}

static void test_mcp_search_score_cmp_export(void) {
    check_int("mcp_search_cmp_left_higher", cbm_rs_mcp_search_score_cmp_v1(20, 10), -10);
    check_int("mcp_search_cmp_right_higher", cbm_rs_mcp_search_score_cmp_v1(10, 20), 10);
    check_int("mcp_search_cmp_equal", cbm_rs_mcp_search_score_cmp_v1(7, 7), 0);
    check_int("mcp_search_cmp_negative_left", cbm_rs_mcp_search_score_cmp_v1(-5, 5), 10);
    check_int("mcp_search_cmp_negative_right", cbm_rs_mcp_search_score_cmp_v1(5, -5), -10);
}

static void test_mcp_search_top_dir_export(void) {
    char buf[32];
    check_size("mcp_search_top_dir_src_len",
               cbm_rs_mcp_search_top_dir_v1(buf, sizeof(buf), "src/app.c"), strlen("src/"));
    check_str("mcp_search_top_dir_src", buf, "src/");
    check_size("mcp_search_top_dir_nested_len",
               cbm_rs_mcp_search_top_dir_v1(buf, sizeof(buf), "src/nested/app.c"), strlen("src/"));
    check_str("mcp_search_top_dir_nested", buf, "src/");
    check_size("mcp_search_top_dir_file_len",
               cbm_rs_mcp_search_top_dir_v1(buf, sizeof(buf), "app.c"), strlen("app.c"));
    check_str("mcp_search_top_dir_file", buf, "app.c");
    check_size("mcp_search_top_dir_abs_len",
               cbm_rs_mcp_search_top_dir_v1(buf, sizeof(buf), "/abs/app.c"), strlen("/"));
    check_str("mcp_search_top_dir_abs", buf, "/");
    check_size("mcp_search_top_dir_null", cbm_rs_mcp_search_top_dir_v1(buf, sizeof(buf), NULL),
               SIZE_MAX);

    char short_buf[5];
    check_size("mcp_search_top_dir_short_len",
               cbm_rs_mcp_search_top_dir_v1(short_buf, sizeof(short_buf), "verylong/path.c"),
               strlen("verylong/"));
    check_str("mcp_search_top_dir_short", short_buf, "very");
}

static void test_mcp_detect_changes_wants_symbols_export(void) {
    check_int("mcp_detect_scope_null", cbm_rs_mcp_detect_changes_wants_symbols_v1(NULL), 1);
    check_int("mcp_detect_scope_symbols", cbm_rs_mcp_detect_changes_wants_symbols_v1("symbols"), 1);
    check_int("mcp_detect_scope_impact", cbm_rs_mcp_detect_changes_wants_symbols_v1("impact"), 1);
    check_int("mcp_detect_scope_empty", cbm_rs_mcp_detect_changes_wants_symbols_v1(""), 0);
    check_int("mcp_detect_scope_files", cbm_rs_mcp_detect_changes_wants_symbols_v1("files"), 0);
    check_int("mcp_detect_scope_case", cbm_rs_mcp_detect_changes_wants_symbols_v1("Symbols"), 0);
    check_int("mcp_detect_scope_trailing", cbm_rs_mcp_detect_changes_wants_symbols_v1("symbols "),
              0);
    check_int("mcp_detect_scope_unknown", cbm_rs_mcp_detect_changes_wants_symbols_v1("unknown"), 0);
}

static void test_mcp_detect_changes_impacted_label_export(void) {
    check_int("mcp_detect_label_null", cbm_rs_mcp_detect_changes_impacted_label_v1(NULL), 0);
    check_int("mcp_detect_label_file", cbm_rs_mcp_detect_changes_impacted_label_v1("File"), 0);
    check_int("mcp_detect_label_folder", cbm_rs_mcp_detect_changes_impacted_label_v1("Folder"), 0);
    check_int("mcp_detect_label_project", cbm_rs_mcp_detect_changes_impacted_label_v1("Project"),
              0);
    check_int("mcp_detect_label_function", cbm_rs_mcp_detect_changes_impacted_label_v1("Function"),
              1);
    check_int("mcp_detect_label_method", cbm_rs_mcp_detect_changes_impacted_label_v1("Method"), 1);
    check_int("mcp_detect_label_route", cbm_rs_mcp_detect_changes_impacted_label_v1("Route"), 1);
    check_int("mcp_detect_label_empty", cbm_rs_mcp_detect_changes_impacted_label_v1(""), 1);
    check_int("mcp_detect_label_case", cbm_rs_mcp_detect_changes_impacted_label_v1("file"), 1);
    check_int("mcp_detect_label_trailing", cbm_rs_mcp_detect_changes_impacted_label_v1("Project "),
              1);
}

static void test_mcp_detect_changes_status_path_offset_export(void) {
    check_size("mcp_detect_status_path_null", cbm_rs_mcp_detect_changes_status_path_offset_v1(NULL),
               SIZE_MAX);
    check_size("mcp_detect_status_path_empty", cbm_rs_mcp_detect_changes_status_path_offset_v1(""),
               SIZE_MAX);
    check_size("mcp_detect_status_path_plain",
               cbm_rs_mcp_detect_changes_status_path_offset_v1("src/mcp/mcp.c"), 0);
    check_size("mcp_detect_status_path_untracked",
               cbm_rs_mcp_detect_changes_status_path_offset_v1("?? src/new.c"), 3);
    check_size("mcp_detect_status_path_added",
               cbm_rs_mcp_detect_changes_status_path_offset_v1("A  src/a.c"), 3);
    check_size("mcp_detect_status_path_modified",
               cbm_rs_mcp_detect_changes_status_path_offset_v1(" M src/m.c"), 3);
    check_size("mcp_detect_status_path_rename",
               cbm_rs_mcp_detect_changes_status_path_offset_v1("R  old/name.c -> new/name.c"), 17);
    check_size("mcp_detect_status_path_rename_empty",
               cbm_rs_mcp_detect_changes_status_path_offset_v1("R  old -> "), SIZE_MAX);
    check_size("mcp_detect_status_path_unknown_prefix",
               cbm_rs_mcp_detect_changes_status_path_offset_v1("ZZ weird"), 0);
}

static void test_mcp_search_line_match_span_export(void) {
    check_int("mcp_search_line_span_hit", cbm_rs_mcp_search_line_match_span_v1(10, 20, 15), 10);
    check_int("mcp_search_line_span_start", cbm_rs_mcp_search_line_match_span_v1(10, 20, 10), 10);
    check_int("mcp_search_line_span_end", cbm_rs_mcp_search_line_match_span_v1(10, 20, 20), 10);
    check_int("mcp_search_line_span_before", cbm_rs_mcp_search_line_match_span_v1(10, 20, 9), -1);
    check_int("mcp_search_line_span_after", cbm_rs_mcp_search_line_match_span_v1(10, 20, 21), -1);
    check_int("mcp_search_line_span_invalid", cbm_rs_mcp_search_line_match_span_v1(20, 10, 15), -1);
}

static void test_mcp_search_pick_resolved_index_export(void) {
    int ambiguous = 9;
    check_int("mcp_pick_resolved_null_out", cbm_rs_mcp_search_pick_resolved_index_v1(NULL, 0, NULL),
              -1);
    check_int("mcp_pick_resolved_invalid_count",
              cbm_rs_mcp_search_pick_resolved_index_v1(NULL, 0, &ambiguous), -1);
    check_int("mcp_pick_resolved_invalid_count_ambiguous", ambiguous, 0);
    check_int("mcp_pick_resolved_null_scores",
              cbm_rs_mcp_search_pick_resolved_index_v1(NULL, 3, &ambiguous), -1);
    check_int("mcp_pick_resolved_null_scores_ambiguous", ambiguous, 0);

    const long scores_unique[] = {10, 20, 15};
    check_int("mcp_pick_resolved_unique",
              cbm_rs_mcp_search_pick_resolved_index_v1(scores_unique, 3, &ambiguous), 1);
    check_int("mcp_pick_resolved_unique_ambiguous", ambiguous, 0);

    const long scores_tie[] = {20, 10, 20};
    check_int("mcp_pick_resolved_tie",
              cbm_rs_mcp_search_pick_resolved_index_v1(scores_tie, 3, &ambiguous), 0);
    check_int("mcp_pick_resolved_tie_ambiguous", ambiguous, 1);
}

static void test_mcp_search_pick_tightest_index_export(void) {
    check_int("mcp_pick_tightest_invalid_count", cbm_rs_mcp_search_pick_tightest_index_v1(NULL, 0),
              -1);
    check_int("mcp_pick_tightest_null_spans", cbm_rs_mcp_search_pick_tightest_index_v1(NULL, 3),
              -1);

    const int spans_all_miss[] = {-1, -1};
    check_int("mcp_pick_tightest_all_miss",
              cbm_rs_mcp_search_pick_tightest_index_v1(spans_all_miss, 2), -1);

    const int spans_unique[] = {10, 5, 20};
    check_int("mcp_pick_tightest_unique", cbm_rs_mcp_search_pick_tightest_index_v1(spans_unique, 3),
              1);

    const int spans_tie[] = {3, 3, 5};
    check_int("mcp_pick_tightest_tie_first", cbm_rs_mcp_search_pick_tightest_index_v1(spans_tie, 3),
              0);

    const int spans_mixed[] = {-1, 7, -1};
    check_int("mcp_pick_tightest_mixed", cbm_rs_mcp_search_pick_tightest_index_v1(spans_mixed, 3),
              1);
}

static void test_mcp_utf8_is_cont_export(void) {
    check_int("mcp_utf8_is_cont_80", cbm_rs_mcp_utf8_is_cont_v1(0x80), 1);
    check_int("mcp_utf8_is_cont_bf", cbm_rs_mcp_utf8_is_cont_v1(0xBF), 1);
    check_int("mcp_utf8_is_cont_7f", cbm_rs_mcp_utf8_is_cont_v1(0x7F), 0);
    check_int("mcp_utf8_is_cont_c0", cbm_rs_mcp_utf8_is_cont_v1(0xC0), 0);
    check_int("mcp_utf8_is_cont_180", cbm_rs_mcp_utf8_is_cont_v1(0x180), 1);
}

static void test_mcp_node_resolution_score_export(void) {
    check_i64("mcp_node_score_null_label", cbm_rs_mcp_node_resolution_score_v1(NULL, 10, 20), 10);
    check_i64("mcp_node_score_empty_label", cbm_rs_mcp_node_resolution_score_v1("", 10, 20),
              1000010);
    check_i64("mcp_node_score_function", cbm_rs_mcp_node_resolution_score_v1("Function", 10, 20),
              2000010);
    check_i64("mcp_node_score_method", cbm_rs_mcp_node_resolution_score_v1("Method", 10, 20),
              2000010);
    check_i64("mcp_node_score_class", cbm_rs_mcp_node_resolution_score_v1("Class", 10, 20),
              1000010);
    check_i64("mcp_node_score_module", cbm_rs_mcp_node_resolution_score_v1("Module", 10, 20), 10);
    check_i64("mcp_node_score_file", cbm_rs_mcp_node_resolution_score_v1("File", 10, 20), 10);
    check_i64("mcp_node_score_negative_span",
              cbm_rs_mcp_node_resolution_score_v1("Function", 20, 10), 2000000);
    check_i64("mcp_node_score_case_sensitive",
              cbm_rs_mcp_node_resolution_score_v1("function", 10, 20), 1000010);
}

static void test_mcp_adr_mode_export(void) {
    check_int("mcp_adr_mode_null", cbm_rs_mcp_adr_mode_v1(NULL), 0);
    check_int("mcp_adr_mode_empty", cbm_rs_mcp_adr_mode_v1(""), 0);
    check_int("mcp_adr_mode_get", cbm_rs_mcp_adr_mode_v1("get"), 0);
    check_int("mcp_adr_mode_update", cbm_rs_mcp_adr_mode_v1("update"), 1);
    check_int("mcp_adr_mode_store", cbm_rs_mcp_adr_mode_v1("store"), 1);
    check_int("mcp_adr_mode_sections", cbm_rs_mcp_adr_mode_v1("sections"), 2);
    check_int("mcp_adr_mode_case", cbm_rs_mcp_adr_mode_v1("Update"), 0);
    check_int("mcp_adr_mode_trailing", cbm_rs_mcp_adr_mode_v1("update "), 0);
    check_int("mcp_adr_mode_unknown", cbm_rs_mcp_adr_mode_v1("unknown"), 0);
}

static void test_mcp_adr_sections_json_export(void) {
    char buf[128];
    const char *content = "# PURPOSE\r\nbody\n## STACK\nnot header";
    const char *expected = "[\"# PURPOSE\",\"## STACK\"]";
    check_size("mcp_adr_sections_len", cbm_rs_mcp_adr_sections_json_v1(buf, sizeof(buf), content),
               strlen(expected));
    check_str("mcp_adr_sections_json", buf, expected);
    check_size("mcp_adr_sections_len_only", cbm_rs_mcp_adr_sections_json_v1(NULL, 0, content),
               strlen(expected));

    check_size("mcp_adr_sections_null", cbm_rs_mcp_adr_sections_json_v1(buf, sizeof(buf), NULL),
               strlen("[]"));
    check_str("mcp_adr_sections_null_json", buf, "[]");
    check_size("mcp_adr_sections_empty", cbm_rs_mcp_adr_sections_json_v1(buf, sizeof(buf), ""),
               strlen("[]"));
    check_str("mcp_adr_sections_empty_json", buf, "[]");

    const char *escaped = " # ignored\n# quote \"slash\\\"\n";
    const char *escaped_expected = "[\"# quote \\\"slash\\\\\\\"\"]";
    check_size("mcp_adr_sections_escape_len",
               cbm_rs_mcp_adr_sections_json_v1(buf, sizeof(buf), escaped),
               strlen(escaped_expected));
    check_str("mcp_adr_sections_escape", buf, escaped_expected);

    char short_buf[10];
    check_size("mcp_adr_sections_short_len",
               cbm_rs_mcp_adr_sections_json_v1(short_buf, sizeof(short_buf), content),
               strlen(expected));
    check_str("mcp_adr_sections_short", short_buf, "[\"# PURPO");
}

static void check_mcp_bm25_match(const char *name, const char *input, size_t bufsize,
                                 int expected_tokens, const char *expected) {
    char buf[64];
    memset(buf, 'X', sizeof(buf));
    int tokens = cbm_rs_mcp_bm25_build_match_v1(buf, bufsize, input);
    check_int(name, tokens, expected_tokens);
    if (expected) {
        check_str(name, buf, expected);
    }
}

static void test_mcp_bm25_build_match_export(void) {
    char buf[16];
    check_int("mcp_bm25_null_buf", cbm_rs_mcp_bm25_build_match_v1(NULL, sizeof(buf), "abc"), 0);
    memset(buf, 'X', sizeof(buf));
    check_int("mcp_bm25_null_input", cbm_rs_mcp_bm25_build_match_v1(buf, sizeof(buf), NULL), 0);
    check_int("mcp_bm25_null_input_unchanged", buf[0], 'X');
    check_mcp_bm25_match("mcp_bm25_empty", "", sizeof(buf), 0, "");
    check_mcp_bm25_match("mcp_bm25_punct", "!!!", sizeof(buf), 0, "");
    check_mcp_bm25_match("mcp_bm25_words", "update settings", sizeof(buf), 1, "update");
    check_mcp_bm25_match("mcp_bm25_words_large", "update settings", 64, 2, "update OR settings");
    check_mcp_bm25_match("mcp_bm25_sanitize", "foo-bar baz.qux", 64, 4, "foo OR bar OR baz OR qux");
    check_mcp_bm25_match("mcp_bm25_non_ascii", "呼叫 alpha β gamma", 64, 2, "alpha OR gamma");
    check_mcp_bm25_match("mcp_bm25_short_exact", "alpha beta", 10, 1, "alpha");
    check_mcp_bm25_match("mcp_bm25_too_short_token", "abcdef", 7, 0, "");

    memset(buf, 'X', sizeof(buf));
    check_int("mcp_bm25_bufsize_one", cbm_rs_mcp_bm25_build_match_v1(buf, 1, "abc"), 0);
    check_int("mcp_bm25_bufsize_one_unchanged", buf[0], 'X');
}

static void test_mcp_bm25_file_pattern_like_export(void) {
    char buf[32];
    check_size("mcp_bm25_like_null", cbm_rs_mcp_bm25_file_pattern_like_v1(buf, sizeof(buf), NULL),
               SIZE_MAX);
    check_size("mcp_bm25_like_len_only", cbm_rs_mcp_bm25_file_pattern_like_v1(NULL, 0, "src/lib"),
               strlen("%src/lib%"));
    check_size("mcp_bm25_like_contains",
               cbm_rs_mcp_bm25_file_pattern_like_v1(buf, sizeof(buf), "src/lib"),
               strlen("%src/lib%"));
    check_str("mcp_bm25_like_contains_value", buf, "%src/lib%");
    check_size("mcp_bm25_like_empty", cbm_rs_mcp_bm25_file_pattern_like_v1(buf, sizeof(buf), ""),
               strlen("%%"));
    check_str("mcp_bm25_like_empty_value", buf, "%%");
    check_size("mcp_bm25_like_star", cbm_rs_mcp_bm25_file_pattern_like_v1(buf, sizeof(buf), "*.go"),
               strlen("%.go"));
    check_str("mcp_bm25_like_star_value", buf, "%.go");
    check_size("mcp_bm25_like_path_star",
               cbm_rs_mcp_bm25_file_pattern_like_v1(buf, sizeof(buf), "src/api/*"),
               strlen("src/api/%"));
    check_str("mcp_bm25_like_path_star_value", buf, "src/api/%");
    check_size("mcp_bm25_like_question",
               cbm_rs_mcp_bm25_file_pattern_like_v1(buf, sizeof(buf), "f???.txt"),
               strlen("f___.txt"));
    check_str("mcp_bm25_like_question_value", buf, "f___.txt");

    char small[6];
    check_size("mcp_bm25_like_short",
               cbm_rs_mcp_bm25_file_pattern_like_v1(small, sizeof(small), "src/lib"),
               strlen("%src/lib%"));
    check_str("mcp_bm25_like_short_value", small, "%src/");
}

static void test_mcp_sanitize_utf8_lossy_export(void) {
    char buf[64];
    check_size("mcp_sanitize_utf8_null", cbm_rs_mcp_sanitize_utf8_lossy_v1(buf, sizeof(buf), NULL),
               SIZE_MAX);
    check_size("mcp_sanitize_utf8_empty", cbm_rs_mcp_sanitize_utf8_lossy_v1(buf, sizeof(buf), ""),
               0);
    check_str("mcp_sanitize_utf8_empty_val", buf, "");
    check_size("mcp_sanitize_utf8_ascii",
               cbm_rs_mcp_sanitize_utf8_lossy_v1(buf, sizeof(buf), "hello"), 5);
    check_str("mcp_sanitize_utf8_ascii_val", buf, "hello");
    check_size("mcp_sanitize_utf8_invalid",
               cbm_rs_mcp_sanitize_utf8_lossy_v1(buf, sizeof(buf), "hello \xff world"), 15);
    check_str("mcp_sanitize_utf8_invalid_val", buf, "hello \xef\xbf\xbd world");
}

static void test_mcp_architecture_aspect_wanted_export(void) {
    check_int("mcp_arch_aspect_null", cbm_rs_mcp_architecture_aspect_wanted_v1(NULL, "structure"),
              1);
    check_int("mcp_arch_aspect_empty", cbm_rs_mcp_architecture_aspect_wanted_v1("", "structure"),
              1);
    check_int("mcp_arch_aspect_no_aspects",
              cbm_rs_mcp_architecture_aspect_wanted_v1("{\"path\":\"/foo\"}", "structure"), 1);
    check_int("mcp_arch_aspect_not_array",
              cbm_rs_mcp_architecture_aspect_wanted_v1("{\"aspects\":\"structure\"}", "structure"),
              1);
    check_int("mcp_arch_aspect_empty_array",
              cbm_rs_mcp_architecture_aspect_wanted_v1("{\"aspects\":[]}", "structure"), 0);
    check_int("mcp_arch_aspect_all",
              cbm_rs_mcp_architecture_aspect_wanted_v1("{\"aspects\":[\"all\"]}", "structure"), 1);
    check_int(
        "mcp_arch_aspect_match",
        cbm_rs_mcp_architecture_aspect_wanted_v1("{\"aspects\":[\"structure\"]}", "structure"), 1);
    check_int(
        "mcp_arch_aspect_no_match",
        cbm_rs_mcp_architecture_aspect_wanted_v1("{\"aspects\":[\"structure\"]}", "dependencies"),
        0);
    check_int(
        "mcp_arch_aspect_ignore_non_str",
        cbm_rs_mcp_architecture_aspect_wanted_v1("{\"aspects\":[123,\"structure\"]}", "structure"),
        1);
    check_int("mcp_arch_aspect_trailing_invalid",
              cbm_rs_mcp_architecture_aspect_wanted_v1("{\"aspects\":[]} trailing", "structure"),
              1);
}

static void test_mcp_trace_is_test_file_export(void) {
    check_int("mcp_trace_test_file_null", cbm_rs_mcp_trace_is_test_file_v1(NULL), 0);
    check_int("mcp_trace_test_file_empty", cbm_rs_mcp_trace_is_test_file_v1(""), 0);
    check_int("mcp_trace_test_file_testdata",
              cbm_rs_mcp_trace_is_test_file_v1("src/testdata/helper.go"), 1);
    check_int("mcp_trace_test_file_prefix", cbm_rs_mcp_trace_is_test_file_v1("test_handler.py"), 1);
    check_int("mcp_trace_test_file_suffix", cbm_rs_mcp_trace_is_test_file_v1("src/foo_test.go"), 1);
    check_int("mcp_trace_test_file_tests_dir",
              cbm_rs_mcp_trace_is_test_file_v1("src/tests/helper.c"), 1);
    check_int("mcp_trace_test_file_spec_dir",
              cbm_rs_mcp_trace_is_test_file_v1("src/spec/helper.rb"), 1);
    check_int("mcp_trace_test_file_dot_test", cbm_rs_mcp_trace_is_test_file_v1("handler.test.ts"),
              1);
    check_int("mcp_trace_test_file_contest",
              cbm_rs_mcp_trace_is_test_file_v1("src/contest/helper.c"), 0);
    check_int("mcp_trace_test_file_mytests",
              cbm_rs_mcp_trace_is_test_file_v1("src/mytests/helper.c"), 0);
    check_int("mcp_trace_test_file_spec_suffix",
              cbm_rs_mcp_trace_is_test_file_v1("handler.spec.ts"), 0);
    check_int("mcp_trace_test_file_windows",
              cbm_rs_mcp_trace_is_test_file_v1("C:\\repo\\tests\\helper.py"), 0);
}

static void test_mcp_project_db_file_name_export(void) {
    check_int("mcp_project_db_null", cbm_rs_mcp_project_db_file_name_v1(NULL), 0);
    check_int("mcp_project_db_empty", cbm_rs_mcp_project_db_file_name_v1(""), 0);
    check_int("mcp_project_db_dotdb", cbm_rs_mcp_project_db_file_name_v1(".db"), 0);
    check_int("mcp_project_db_min", cbm_rs_mcp_project_db_file_name_v1("x.db"), 1);
    check_int("mcp_project_db_regular", cbm_rs_mcp_project_db_file_name_v1("alpha704.db"), 1);
    check_int("mcp_project_db_tmp", cbm_rs_mcp_project_db_file_name_v1("tmp-bench.db"), 1);
    check_int("mcp_project_db_wal", cbm_rs_mcp_project_db_file_name_v1("alpha704.db-wal"), 0);
    check_int("mcp_project_db_sqlite", cbm_rs_mcp_project_db_file_name_v1("alpha704.sqlite"), 0);
    check_int("mcp_project_db_internal", cbm_rs_mcp_project_db_file_name_v1("_internal.db"), 0);
    check_int("mcp_project_db_memory", cbm_rs_mcp_project_db_file_name_v1(":memory:"), 0);
    check_int("mcp_project_db_memory_db", cbm_rs_mcp_project_db_file_name_v1(":memory:.db"), 0);
}

static void test_mcp_cancel_request_matches_export(void) {
    check_int("mcp_cancel_null", cbm_rs_mcp_cancel_request_matches_v1(NULL, 7, NULL), 0);
    check_int("mcp_cancel_empty", cbm_rs_mcp_cancel_request_matches_v1("", 7, NULL), 0);
    check_int("mcp_cancel_not_json", cbm_rs_mcp_cancel_request_matches_v1("not json", 7, NULL), 0);
    check_int("mcp_cancel_array", cbm_rs_mcp_cancel_request_matches_v1("[]", 7, NULL), 0);
    check_int("mcp_cancel_missing", cbm_rs_mcp_cancel_request_matches_v1("{}", 7, NULL), 0);

    check_int("mcp_cancel_numeric_match",
              cbm_rs_mcp_cancel_request_matches_v1("{\"requestId\":7}", 7, NULL), 1);
    check_int("mcp_cancel_numeric_mismatch",
              cbm_rs_mcp_cancel_request_matches_v1("{\"requestId\":8}", 7, NULL), 0);
    check_int("mcp_cancel_numeric_string_mismatch",
              cbm_rs_mcp_cancel_request_matches_v1("{\"requestId\":\"7\"}", 7, NULL), 0);
    check_int("mcp_cancel_numeric_float_mismatch",
              cbm_rs_mcp_cancel_request_matches_v1("{\"requestId\":7.0}", 7, NULL), 0);
    check_int("mcp_cancel_numeric_dup_first",
              cbm_rs_mcp_cancel_request_matches_v1("{\"requestId\":7,\"requestId\":8}", 7, NULL),
              1);
    check_int("mcp_cancel_numeric_dup_first_mismatch",
              cbm_rs_mcp_cancel_request_matches_v1("{\"requestId\":7,\"requestId\":8}", 8, NULL),
              0);

    check_int("mcp_cancel_string_match",
              cbm_rs_mcp_cancel_request_matches_v1("{\"requestId\":\"call-1\"}", -1, "call-1"), 1);
    check_int("mcp_cancel_string_mismatch",
              cbm_rs_mcp_cancel_request_matches_v1("{\"requestId\":\"call-2\"}", -1, "call-1"), 0);
    check_int("mcp_cancel_string_numeric_mismatch",
              cbm_rs_mcp_cancel_request_matches_v1("{\"requestId\":7}", -1, "7"), 0);
    check_int("mcp_cancel_string_unicode",
              cbm_rs_mcp_cancel_request_matches_v1("{\"requestId\":\"call-\\u4e2d\"}", -1,
                                                   "call-\xE4\xB8\xAD"),
              1);
    check_int(
        "mcp_cancel_string_empty_dup_first",
        cbm_rs_mcp_cancel_request_matches_v1("{\"requestId\":\"\",\"requestId\":\"late\"}", -1, ""),
        1);
    check_int("mcp_cancel_invalid_utf8",
              cbm_rs_mcp_cancel_request_matches_v1("{\"requestId\":\"bad\xff\"}", -1, "bad"), 0);
}

static void test_mcp_jsonrpc_format_error_export(void) {
    char json[160];
    const char *expected =
        "{\"jsonrpc\":\"2.0\",\"id\":5,\"error\":{\"code\":-32600,\"message\":\"Invalid "
        "Request\"}}";
    check_size("mcp_format_error_len",
               cbm_rs_mcp_jsonrpc_format_error_v1(json, sizeof(json), 5, -32600, "Invalid Request"),
               strlen(expected));
    check_str("mcp_format_error_json", json, expected);
    check_size("mcp_format_error_len_only",
               cbm_rs_mcp_jsonrpc_format_error_v1(NULL, 0, 5, -32600, "Invalid Request"),
               strlen(expected));

    char short_buf[10];
    check_size("mcp_format_error_short_len",
               cbm_rs_mcp_jsonrpc_format_error_v1(short_buf, sizeof(short_buf), 5, -32600,
                                                  "Invalid Request"),
               strlen(expected));
    check_str("mcp_format_error_short", short_buf, "{\"jsonrpc");

    const char *escaped =
        "{\"jsonrpc\":\"2.0\",\"id\":-1,\"error\":{\"code\":-32700,\"message\":\"bad "
        "\\\"json\\\" \\\\ input\\n\"}}";
    check_size("mcp_format_error_escape_len",
               cbm_rs_mcp_jsonrpc_format_error_v1(json, sizeof(json), -1, -32700,
                                                  "bad \"json\" \\ input\n"),
               strlen(escaped));
    check_str("mcp_format_error_escape", json, escaped);

    const char *empty = "{\"jsonrpc\":\"2.0\",\"id\":0,\"error\":{\"code\":-1,\"message\":\"\"}}";
    check_size("mcp_format_error_null_message_len",
               cbm_rs_mcp_jsonrpc_format_error_v1(json, sizeof(json), 0, -1, NULL), strlen(empty));
    check_str("mcp_format_error_null_message", json, empty);
}

static void test_mcp_jsonrpc_format_response_export(void) {
    char json[192];
    const char *result =
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"name\":\"codebase-memory-mcp\"}}";
    check_size("mcp_format_response_result_len",
               cbm_rs_mcp_jsonrpc_format_response_v1(json, sizeof(json), 1, NULL,
                                                     "{\"name\":\"codebase-memory-mcp\"}", NULL),
               strlen(result));
    check_str("mcp_format_response_result", json, result);
    check_size("mcp_format_response_result_len_only",
               cbm_rs_mcp_jsonrpc_format_response_v1(NULL, 0, 1, NULL,
                                                     "{\"name\":\"codebase-memory-mcp\"}", NULL),
               strlen(result));

    const char *string_id = "{\"jsonrpc\":\"2.0\",\"id\":\"init-abc\",\"result\":{\"ok\":true}}";
    check_size("mcp_format_response_string_id_len",
               cbm_rs_mcp_jsonrpc_format_response_v1(json, sizeof(json), 0, "init-abc",
                                                     "{ \"ok\" : true }", NULL),
               strlen(string_id));
    check_str("mcp_format_response_string_id", json, string_id);

    char short_buf[12];
    check_size("mcp_format_response_short_len",
               cbm_rs_mcp_jsonrpc_format_response_v1(short_buf, sizeof(short_buf), 0, "init-abc",
                                                     "{ \"ok\" : true }", NULL),
               strlen(string_id));
    check_str("mcp_format_response_short", short_buf, "{\"jsonrpc\":");

    const char *escaped_id = "{\"jsonrpc\":\"2.0\",\"id\":\"a\\\"b\\\\c\",\"result\":null}";
    check_size("mcp_format_response_escaped_id_len",
               cbm_rs_mcp_jsonrpc_format_response_v1(json, sizeof(json), 7, "a\"b\\c", NULL, NULL),
               strlen(escaped_id));
    check_str("mcp_format_response_escaped_id", json, escaped_id);

    const char *error =
        "{\"jsonrpc\":\"2.0\",\"id\":5,\"error\":{\"code\":-32600,\"message\":\"bad\"}}";
    check_size("mcp_format_response_error_len",
               cbm_rs_mcp_jsonrpc_format_response_v1(json, sizeof(json), 5, NULL,
                                                     "{\"ignored\":true}",
                                                     "{\"code\":-32600,\"message\":\"bad\"}"),
               strlen(error));
    check_str("mcp_format_response_error", json, error);

    const char *invalid_result = "{\"jsonrpc\":\"2.0\",\"id\":9}";
    check_size("mcp_format_response_invalid_result_len",
               cbm_rs_mcp_jsonrpc_format_response_v1(json, sizeof(json), 9, NULL, "not json", NULL),
               strlen(invalid_result));
    check_str("mcp_format_response_invalid_result", json, invalid_result);

    check_size("mcp_format_response_invalid_error_len",
               cbm_rs_mcp_jsonrpc_format_response_v1(json, sizeof(json), 9, NULL, "{\"ok\":true}",
                                                     "not json"),
               strlen(invalid_result));
    check_str("mcp_format_response_invalid_error", json, invalid_result);
}

static void test_mcp_text_result_export(void) {
    char json[256];
    const char *structured = "{\"content\":[{\"type\":\"text\",\"text\":\"{\\\"total\\\":5}\"}],"
                             "\"structuredContent\":{\"total\":5},\"isError\":false}";
    check_size("mcp_text_result_structured_len",
               cbm_rs_mcp_text_result_v1(json, sizeof(json), "{\"total\":5}", 0),
               strlen(structured));
    check_str("mcp_text_result_structured", json, structured);
    check_size("mcp_text_result_structured_len_only",
               cbm_rs_mcp_text_result_v1(NULL, 0, "{\"total\":5}", 0), strlen(structured));

    char short_buf[14];
    check_size("mcp_text_result_short_len",
               cbm_rs_mcp_text_result_v1(short_buf, sizeof(short_buf), "{\"total\":5}", 0),
               strlen(structured));
    check_str("mcp_text_result_short", short_buf, "{\"content\":[{");

    const char *plain =
        "{\"content\":[{\"type\":\"text\",\"text\":\"plain text\"}],\"isError\":false}";
    check_size("mcp_text_result_plain_len",
               cbm_rs_mcp_text_result_v1(json, sizeof(json), "plain text", 0), strlen(plain));
    check_str("mcp_text_result_plain", json, plain);

    const char *array_text =
        "{\"content\":[{\"type\":\"text\",\"text\":\"[1,2]\"}],\"isError\":false}";
    check_size("mcp_text_result_array_len",
               cbm_rs_mcp_text_result_v1(json, sizeof(json), "[1,2]", 0), strlen(array_text));
    check_str("mcp_text_result_array", json, array_text);

    const char *err =
        "{\"content\":[{\"type\":\"text\",\"text\":\"bad \\\"json\\\" \\\\ input\\n\"}],"
        "\"isError\":true}";
    check_size("mcp_text_result_error_len",
               cbm_rs_mcp_text_result_v1(json, sizeof(json), "bad \"json\" \\ input\n", 1),
               strlen(err));
    check_str("mcp_text_result_error", json, err);

    const char *null_text = "{\"content\":[{\"type\":\"text\",\"text\":\"\"}],\"isError\":false}";
    check_size("mcp_text_result_null_len", cbm_rs_mcp_text_result_v1(json, sizeof(json), NULL, 0),
               strlen(null_text));
    check_str("mcp_text_result_null", json, null_text);
}

static void test_mcp_jsonrpc_parse_export(void) {
    CbmRsMcpJsonRpcParseOutV1 out = {0};
    char jsonrpc[16];
    char method[32];
    char id_str[16];
    char params[96];

    const unsigned char request[] = " { \"id\" : \"abc\" , \"method\" : \"tools/call\" , "
                                    "\"params\" : { \"name\" : \"search_graph\" } } ";
    check_int("mcp_jsonrpc_parse_len_only_rc",
              cbm_rs_mcp_jsonrpc_parse_v1(request, (int)strlen((const char *)request), &out, NULL,
                                          0, NULL, 0, NULL, 0, NULL, 0),
              0);
    check_i64("mcp_jsonrpc_parse_len_only_id", out.id, -1);
    check_int("mcp_jsonrpc_parse_len_only_has_id", out.has_id, 1);
    check_int("mcp_jsonrpc_parse_len_only_id_kind", out.id_kind, CBM_RS_MCP_ID_STRING);
    check_int("mcp_jsonrpc_parse_len_only_has_params", out.has_params, 1);
    check_size("mcp_jsonrpc_parse_len_only_jsonrpc_len", out.jsonrpc_len, strlen("2.0"));
    check_size("mcp_jsonrpc_parse_len_only_method_len", out.method_len, strlen("tools/call"));
    check_size("mcp_jsonrpc_parse_len_only_id_str_len", out.id_str_len, strlen("abc"));
    check_size("mcp_jsonrpc_parse_len_only_params_len", out.params_len,
               strlen("{ \"name\" : \"search_graph\" }"));

    memset(&out, 0, sizeof(out));
    check_int("mcp_jsonrpc_parse_write_rc",
              cbm_rs_mcp_jsonrpc_parse_v1(request, (int)strlen((const char *)request), &out,
                                          jsonrpc, sizeof(jsonrpc), method, sizeof(method), id_str,
                                          sizeof(id_str), params, sizeof(params)),
              0);
    check_str("mcp_jsonrpc_parse_jsonrpc", jsonrpc, "2.0");
    check_str("mcp_jsonrpc_parse_method", method, "tools/call");
    check_str("mcp_jsonrpc_parse_id_str", id_str, "abc");
    check_str("mcp_jsonrpc_parse_params", params, "{ \"name\" : \"search_graph\" }");

    const unsigned char int_id[] = "{\"jsonrpc\":\"2.0\",\"id\":42,\"method\":\"ping\"}";
    memset(&out, 0, sizeof(out));
    check_int("mcp_jsonrpc_parse_int_id_rc",
              cbm_rs_mcp_jsonrpc_parse_v1(int_id, (int)strlen((const char *)int_id), &out, jsonrpc,
                                          sizeof(jsonrpc), method, sizeof(method), id_str,
                                          sizeof(id_str), params, sizeof(params)),
              0);
    check_i64("mcp_jsonrpc_parse_int_id", out.id, 42);
    check_int("mcp_jsonrpc_parse_int_id_kind", out.id_kind, CBM_RS_MCP_ID_INT);
    check_int("mcp_jsonrpc_parse_int_has_params", out.has_params, 0);
    check_str("mcp_jsonrpc_parse_int_id_str_empty", id_str, "");
    check_str("mcp_jsonrpc_parse_int_params_empty", params, "");

    const unsigned char other_id[] = "{\"jsonrpc\":2,\"id\":true,\"method\":\"ping\"}";
    memset(&out, 0, sizeof(out));
    check_int("mcp_jsonrpc_parse_other_id_rc",
              cbm_rs_mcp_jsonrpc_parse_v1(other_id, (int)strlen((const char *)other_id), &out,
                                          jsonrpc, sizeof(jsonrpc), method, sizeof(method), id_str,
                                          sizeof(id_str), params, sizeof(params)),
              0);
    check_i64("mcp_jsonrpc_parse_other_id", out.id, -1);
    check_int("mcp_jsonrpc_parse_other_has_id", out.has_id, 1);
    check_int("mcp_jsonrpc_parse_other_id_kind", out.id_kind, CBM_RS_MCP_ID_OTHER);
    check_str("mcp_jsonrpc_parse_other_jsonrpc_default", jsonrpc, "2.0");

    const unsigned char notification[] =
        "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}";
    memset(&out, 0, sizeof(out));
    check_int("mcp_jsonrpc_parse_notification_rc",
              cbm_rs_mcp_jsonrpc_parse_v1(notification, (int)strlen((const char *)notification),
                                          &out, jsonrpc, sizeof(jsonrpc), method, sizeof(method),
                                          id_str, sizeof(id_str), params, sizeof(params)),
              0);
    check_int("mcp_jsonrpc_parse_notification_has_id", out.has_id, 0);
    check_int("mcp_jsonrpc_parse_notification_id_kind", out.id_kind, CBM_RS_MCP_ID_NONE);
    check_str("mcp_jsonrpc_parse_notification_method", method, "notifications/initialized");

    const unsigned char duplicate_keys[] =
        "{\"method\":\"ping\",\"method\":\"tools/list\",\"id\":1,\"id\":\"late\","
        "\"params\":{\"a\":1},\"params\":{\"b\":2},\"jsonrpc\":3,\"jsonrpc\":\"late\"}";
    memset(&out, 0, sizeof(out));
    check_int("mcp_jsonrpc_parse_duplicate_rc",
              cbm_rs_mcp_jsonrpc_parse_v1(duplicate_keys, (int)strlen((const char *)duplicate_keys),
                                          &out, jsonrpc, sizeof(jsonrpc), method, sizeof(method),
                                          id_str, sizeof(id_str), params, sizeof(params)),
              0);
    check_str("mcp_jsonrpc_parse_duplicate_jsonrpc", jsonrpc, "2.0");
    check_str("mcp_jsonrpc_parse_duplicate_method", method, "ping");
    check_i64("mcp_jsonrpc_parse_duplicate_id", out.id, 1);
    check_int("mcp_jsonrpc_parse_duplicate_id_kind", out.id_kind, CBM_RS_MCP_ID_INT);
    check_str("mcp_jsonrpc_parse_duplicate_params", params, "{\"a\":1}");

    const unsigned char unicode[] =
        "{\"jsonrpc\":\"2.0\",\"id\":\"\\u4e2d\",\"method\":\"p\\u0069ng\"}";
    memset(&out, 0, sizeof(out));
    check_int("mcp_jsonrpc_parse_unicode_rc",
              cbm_rs_mcp_jsonrpc_parse_v1(unicode, (int)strlen((const char *)unicode), &out,
                                          jsonrpc, sizeof(jsonrpc), method, sizeof(method), id_str,
                                          sizeof(id_str), params, sizeof(params)),
              0);
    check_str("mcp_jsonrpc_parse_unicode_method", method, "ping");
    check_str("mcp_jsonrpc_parse_unicode_id", id_str, "中");

    const unsigned char invalid_utf8[] = "{\"jsonrpc\":\"2.0\",\"method\":\"bad\xff\"}";
    check_int("mcp_jsonrpc_parse_invalid_utf8",
              cbm_rs_mcp_jsonrpc_parse_v1(invalid_utf8, (int)strlen((const char *)invalid_utf8),
                                          &out, NULL, 0, NULL, 0, NULL, 0, NULL, 0),
              -1);

    char short_method[3];
    memset(&out, 0, sizeof(out));
    check_int("mcp_jsonrpc_parse_short_rc",
              cbm_rs_mcp_jsonrpc_parse_v1(int_id, (int)strlen((const char *)int_id), &out, jsonrpc,
                                          sizeof(jsonrpc), short_method, sizeof(short_method), NULL,
                                          0, NULL, 0),
              0);
    check_size("mcp_jsonrpc_parse_short_method_len", out.method_len, strlen("ping"));
    check_str("mcp_jsonrpc_parse_short_method", short_method, "pi");

    check_int("mcp_jsonrpc_parse_null_input",
              cbm_rs_mcp_jsonrpc_parse_v1(NULL, 0, &out, NULL, 0, NULL, 0, NULL, 0, NULL, 0), -1);
    check_int("mcp_jsonrpc_parse_negative_len",
              cbm_rs_mcp_jsonrpc_parse_v1((const unsigned char *)"{}", -1, &out, NULL, 0, NULL, 0,
                                          NULL, 0, NULL, 0),
              -1);
    check_int("mcp_jsonrpc_parse_null_out",
              cbm_rs_mcp_jsonrpc_parse_v1(int_id, (int)strlen((const char *)int_id), NULL, NULL, 0,
                                          NULL, 0, NULL, 0, NULL, 0),
              -1);
    const unsigned char missing_method[] = "{\"jsonrpc\":\"2.0\"}";
    check_int("mcp_jsonrpc_parse_missing_method",
              cbm_rs_mcp_jsonrpc_parse_v1(missing_method, (int)strlen((const char *)missing_method),
                                          &out, NULL, 0, NULL, 0, NULL, 0, NULL, 0),
              -1);
}

#if !defined(CBM_USE_RUST_DISCOVER_USERCONFIG_ONLY) || \
    defined(CBM_USE_RUST_DISCOVER_LANGUAGE_ONLY) || \
    defined(CBM_USE_RUST_DISCOVER_LANGUAGE_NAME) || \
    defined(CBM_USE_RUST_DISCOVER_LANGUAGE_NAME_ONLY)
static void test_discover_language_name_public_exports(void) {
    const char *go = cbm_language_name(CBM_LANG_GO);
    const char *go_again = cbm_language_name(CBM_LANG_GO);
    const char *cfscript = cbm_language_name(CBM_LANG_CFSCRIPT);
    const char *cfml = cbm_language_name(CBM_LANG_CFML);
    const char *negative = cbm_language_name((CBMLanguage)-1);
    const char *count = cbm_language_name(CBM_LANG_COUNT);

    check_int("discover_language_name_public_go", go ? strcmp(go, "Go") : -1, 0);
    check_int("discover_language_name_public_static", go == go_again, 1);
    check_int("discover_language_name_public_negative",
              negative ? strcmp(negative, "Unknown") : -1,
              0);
    check_int("discover_language_name_public_count", count ? strcmp(count, "Unknown") : -1, 0);
    check_int("discover_language_name_public_cfscript",
              cfscript ? strcmp(cfscript, "CFML") : -1,
              0);
    check_int("discover_language_name_public_cfml", cfml ? strcmp(cfml, "CFML") : -1, 0);
}
#endif

#if defined(CBM_USE_RUST_DISCOVER_LANGUAGE_NAME) || \
    defined(CBM_USE_RUST_DISCOVER_LANGUAGE_NAME_ONLY)
static void test_discover_language_name_v1_exports(void) {
    const char *go = cbm_rs_discover_language_name_v1(CBM_LANG_GO);
    const char *go_again = cbm_rs_discover_language_name_v1(CBM_LANG_GO);
    const char *cfscript = cbm_rs_discover_language_name_v1(CBM_LANG_CFSCRIPT);
    const char *cfml = cbm_rs_discover_language_name_v1(CBM_LANG_CFML);
    const char *negative = cbm_rs_discover_language_name_v1(-1);
    const char *count = cbm_rs_discover_language_name_v1(CBM_LANG_COUNT);

    check_int("discover_language_name_v1_go", go ? strcmp(go, "Go") : -1, 0);
    check_int("discover_language_name_v1_static", go == go_again, 1);
    check_int("discover_language_name_v1_negative", negative ? strcmp(negative, "Unknown") : -1, 0);
    check_int("discover_language_name_v1_count", count ? strcmp(count, "Unknown") : -1, 0);
    check_int("discover_language_name_v1_cfscript",
              cfscript ? strcmp(cfscript, "CFML") : -1,
              0);
    check_int("discover_language_name_v1_cfml", cfml ? strcmp(cfml, "CFML") : -1, 0);
}
#endif

#ifdef CBM_USE_RUST_DISCOVER_USERCONFIG_ONLY
static void test_discover_userconfig_exports(void) {
    const char *dir = "/tmp/cbm-rust-userconfig-smoke";
    const char *path = "/tmp/cbm-rust-userconfig-smoke/.codebase-memory.json";
#ifdef _WIN32
    (void)_mkdir(dir);
#else
    (void)mkdir(dir, 0755);
#endif
    FILE *file = fopen(path, "w");
    if (!file) {
        fprintf(stderr, "userconfig smoke: cannot create fixture\n");
        exit(EXIT_FAILURE);
    }
    fputs("{\"extra_extensions\":{\".blade.php\":\"php\"}}", file);
    fclose(file);

    cbm_userconfig_t *cfg = cbm_userconfig_load(dir);
    if (!cfg || cfg->count != 1 || cbm_userconfig_lookup(cfg, ".blade.php") == CBM_LANG_COUNT) {
        fprintf(stderr, "userconfig smoke: Rust ABI mismatch\n");
        cbm_userconfig_free(cfg);
        remove(path);
        exit(EXIT_FAILURE);
    }
    cbm_userconfig_free(cfg);
    remove(path);
}
#endif

#if defined(CBM_USE_RUST_DISCOVER_USERCONFIG_ONLY) && \
    (defined(CBM_USE_RUST_DISCOVER_LANGUAGE_NAME) || \
     defined(CBM_USE_RUST_DISCOVER_LANGUAGE_NAME_ONLY))
static void test_discover_userconfig_language_name_exports(void) {
    const char *dir = "/tmp/cbm-rust-userconfig-language-name-smoke";
    const char *path = "/tmp/cbm-rust-userconfig-language-name-smoke/.codebase-memory.json";
#ifdef _WIN32
    (void)_mkdir(dir);
#else
    (void)mkdir(dir, 0755);
#endif
    FILE *file = fopen(path, "w");
    if (!file) {
        fprintf(stderr, "userconfig language name smoke: cannot create fixture\n");
        exit(EXIT_FAILURE);
    }
    fputs("{\"extra_extensions\":{\".cfml\":\"cfml\"}}", file);
    fclose(file);

    cbm_userconfig_t *cfg = cbm_userconfig_load(dir);
    if (!cfg || cfg->count != 1 ||
        cbm_userconfig_lookup(cfg, ".cfml") != CBM_LANG_CFSCRIPT) {
        fprintf(stderr, "userconfig language name smoke: Rust ABI mismatch\n");
        cbm_userconfig_free(cfg);
        remove(path);
        exit(EXIT_FAILURE);
    }
    cbm_userconfig_free(cfg);
    remove(path);
}
#endif

static void test_discover_filter_exports(void) {
    check_bool("discover_filters_full_root_dot_git",
              cbm_should_skip_dir(".git", CBM_MODE_FULL),
              true);
    check_bool("discover_filters_full_root_src", cbm_should_skip_dir("src", CBM_MODE_FULL), false);
    check_bool("discover_filters_full_root_docs", cbm_should_skip_dir("docs", CBM_MODE_FULL), false);
    check_bool("discover_filters_moderate_docs",
              cbm_should_skip_dir("docs", CBM_MODE_MODERATE),
              true);
    check_bool("discover_filters_fast_docs", cbm_should_skip_dir("docs", CBM_MODE_FAST), true);
    check_bool("discover_filters_full_null_dir", cbm_should_skip_dir(NULL, CBM_MODE_FULL), false);

    check_bool("discover_filters_suffix_full_pyc",
              cbm_has_ignored_suffix("module.pyc", CBM_MODE_FULL),
              true);
    check_bool("discover_filters_suffix_full_zip",
              cbm_has_ignored_suffix("archive.zip", CBM_MODE_FULL),
              false);
    check_bool("discover_filters_suffix_moderate_zip",
              cbm_has_ignored_suffix("archive.zip", CBM_MODE_MODERATE),
              true);
    check_bool("discover_filters_suffix_fast_min_css",
              cbm_has_ignored_suffix("style.min.css", CBM_MODE_FAST),
              true);
    check_bool("discover_filters_suffix_null", cbm_has_ignored_suffix(NULL, CBM_MODE_FAST), false);

    check_bool("discover_filters_filename_full_license",
              cbm_should_skip_filename("LICENSE", CBM_MODE_FULL),
              false);
    check_bool("discover_filters_filename_moderate_license", cbm_should_skip_filename("LICENSE",
                                                                                    CBM_MODE_MODERATE),
              true);
    check_bool("discover_filters_filename_fast_mock",
              cbm_should_skip_filename("MOCK_", CBM_MODE_FAST),
              false);
    check_bool("discover_filters_filename_null", cbm_should_skip_filename(NULL, CBM_MODE_FAST), false);

    check_bool("discover_filters_pattern_full", cbm_matches_fast_pattern("foo.d.ts", CBM_MODE_FULL),
              false);
    check_bool("discover_filters_pattern_fast", cbm_matches_fast_pattern("foo.d.ts", CBM_MODE_FAST),
              true);
    check_bool("discover_filters_pattern_fast_generated",
              cbm_matches_fast_pattern("foo.generated.rs", CBM_MODE_FAST),
              true);
    check_bool("discover_filters_pattern_null", cbm_matches_fast_pattern(NULL, CBM_MODE_FAST), false);
    check_bool("discover_filters_invalid_dir", cbm_should_skip_dir("src", 99), false);
    check_bool("discover_filters_invalid_suffix", cbm_has_ignored_suffix("plain.txt", 99), false);
    check_bool("discover_filters_invalid_filename", cbm_should_skip_filename("plain.txt", 99), false);
    check_bool("discover_filters_invalid_pattern", cbm_matches_fast_pattern("plain.txt", 99), false);
}

#ifdef CBM_USE_RUST_MEM_ONLY
static void test_mem_direct_exports(void) {
    cbm_mem_init(0.5);
    size_t budget = cbm_mem_budget();
    if (budget == 0 || cbm_mem_rss() == 0 || cbm_mem_peak_rss() < cbm_mem_rss()) {
        fprintf(stderr, "mem smoke: Rust ABI mismatch\n");
        exit(EXIT_FAILURE);
    }
    if (cbm_mem_worker_budget(0) != budget || cbm_mem_worker_budget(-2) != budget ||
        cbm_mem_worker_budget(4) != budget / 4) {
        fprintf(stderr, "mem smoke: worker budget mismatch\n");
        exit(EXIT_FAILURE);
    }
    (void)cbm_mem_over_budget();
    cbm_mem_collect();
}
#endif

#ifdef CBM_USE_RUST_DIAGNOSTICS_ONLY
static void test_diagnostics_direct_exports(void) {
#ifndef _WIN32
    char json_path[CBM_SZ_256];
    char ndjson_path[CBM_SZ_256];
    int pid = (int)getpid();
    if (cbm_diag_format_path(json_path, sizeof(json_path), "/tmp", pid, "json") < 0 ||
        cbm_diag_format_path(ndjson_path, sizeof(ndjson_path), "/tmp", pid, "ndjson") < 0) {
        fprintf(stderr, "diagnostics smoke: path formatter failed\n");
        exit(EXIT_FAILURE);
    }
    (void)remove(json_path);
    (void)remove(ndjson_path);
    (void)remove("/tmp/cbm-diagnostics-smoke-unused");

    cbm_diag_query_snapshot_t snapshot = {0};
    cbm_diag_record_query(100, false);
    cbm_diag_record_query(250, true);
    cbm_diag_query_stats_snapshot(&g_query_stats, &snapshot);
    if (snapshot.count < 2 || snapshot.errors < 1 || snapshot.max_us < 250) {
        fprintf(stderr, "diagnostics smoke: query stats mismatch\n");
        exit(EXIT_FAILURE);
    }

    char json[1024];
    cbm_diag_snapshot_t fixture = {
        .uptime_s = 17,
        .rss_bytes = 1024,
        .peak_rss_bytes = 2048,
        .heap_committed_bytes = 4096,
        .peak_committed_bytes = 8192,
        .page_faults = 23,
        .fd_count = 9,
        .queries = snapshot,
        .pid = 4242,
    };
    if (cbm_diag_format_json(json, sizeof(json), &fixture) < 0 || strstr(json, "uptime_s") == NULL) {
        fprintf(stderr, "diagnostics smoke: JSON formatter failed\n");
        exit(EXIT_FAILURE);
    }

    if (setenv("CBM_DIAGNOSTICS", "1", 1) != 0 || !cbm_diag_start()) {
        fprintf(stderr, "diagnostics smoke: start failed\n");
        exit(EXIT_FAILURE);
    }
    usleep(100000);
    cbm_diag_stop();
    if (access(ndjson_path, F_OK) != 0 || access(json_path, F_OK) == 0) {
        fprintf(stderr, "diagnostics smoke: lifecycle file contract failed\n");
        (void)remove(ndjson_path);
        exit(EXIT_FAILURE);
    }
    (void)remove(ndjson_path);
    (void)unsetenv("CBM_DIAGNOSTICS");
#endif
}
#endif

#ifdef CBM_USE_RUST_PIPELINE_PATH_ALIAS_ONLY
static void test_path_alias_direct_exports(void) {
#ifndef _WIN32
    const char *dir = "/tmp/cbm-rust-path-alias-smoke";
    const char *config = "/tmp/cbm-rust-path-alias-smoke/tsconfig.json";
    (void)mkdir(dir, 0755);
    FILE *file = fopen(config, "w");
    if (!file) {
        fprintf(stderr, "path alias smoke: cannot create config\n");
        exit(EXIT_FAILURE);
    }
    fputs("{\"compilerOptions\":{\"paths\":{\"@/*\":[\"src/*\"]}}}", file);
    fclose(file);

    cbm_path_alias_collection_t *coll = cbm_load_path_aliases(dir);
    if (!coll) {
        fprintf(stderr, "path alias smoke: loader returned NULL\n");
        exit(EXIT_FAILURE);
    }
    const cbm_path_alias_map_t *map = cbm_path_alias_find_for_file(coll, "src/main.ts");
    char *resolved = cbm_path_alias_resolve(map, "@/lib/auth");
    if (!resolved || strcmp(resolved, "src/lib/auth") != 0) {
        fprintf(stderr, "path alias smoke: resolver mismatch\n");
        free(resolved);
        cbm_path_alias_collection_free(coll);
        exit(EXIT_FAILURE);
    }
    free(resolved);
    cbm_path_alias_collection_free(coll);
    (void)remove(config);
    (void)rmdir(dir);
#endif
}
#endif

static void test_cli_version_exports(void) {
    check_int("cli_version_newer", cbm_rs_cli_compare_versions_v1("0.2.1", "0.2.0"), 1);
    check_int("cli_version_equal", cbm_rs_cli_compare_versions_v1("v0.2.1", "0.2.1"), 0);
    check_int("cli_version_prerelease", cbm_rs_cli_compare_versions_v1("0.2.1-dev", "0.2.1"), -1);
    check_int("cli_version_missing_parts", cbm_rs_cli_compare_versions_v1("1", "1.0.0"), 0);
    check_int("cli_version_null_inputs", cbm_rs_cli_compare_versions_v1(NULL, NULL), 0);
}

static void test_cli_hook_token_export(void) {
    char out[97];
    check_int("cli_hook_token_longest",
              cbm_rs_cli_hook_extract_token_v1("**/OrderHandler.py", out, sizeof(out)), 1);
    check_str("cli_hook_token_longest_value", out, "OrderHandler");
    check_int("cli_hook_token_short",
              cbm_rs_cli_hook_extract_token_v1("foo/bar", out, sizeof(out)), 0);
    check_int("cli_hook_token_truncated",
              cbm_rs_cli_hook_extract_token_v1("prefix_abcdefghijklmnopqrstuvwxyz", out, 8), 1);
    check_str("cli_hook_token_truncated_value", out, "prefix_");
    check_int("cli_hook_token_null", cbm_rs_cli_hook_extract_token_v1(NULL, out, sizeof(out)), 0);

#ifdef CBM_USE_RUST_CLI_HOOK_TOKEN_ONLY
    check_int("cli_hook_token_direct",
              cbm_cli_hook_extract_token("longest_identifier", out, sizeof(out)), 1);
    check_str("cli_hook_token_direct_value", out, "longest_identifier");
#endif
}

static void test_git_path_absolute_export(void) {
    check_int("git_path_absolute_empty", cbm_rs_git_path_is_absolute_v1(""), 0);
    check_int("git_path_absolute_slash", cbm_rs_git_path_is_absolute_v1("/"), 1);
    check_int("git_path_absolute_root", cbm_rs_git_path_is_absolute_v1("/tmp/project"), 1);
    check_int("git_path_absolute_network", cbm_rs_git_path_is_absolute_v1("//server"), 1);
    check_int("git_path_absolute_relative", cbm_rs_git_path_is_absolute_v1("relative/project"), 0);
    check_int("git_path_absolute_backslash", cbm_rs_git_path_is_absolute_v1("\\root"), 0);
    check_int("git_path_absolute_null", cbm_rs_git_path_is_absolute_v1(NULL), 0);

#ifdef _WIN32
    check_int("git_path_absolute_drive_only", cbm_rs_git_path_is_absolute_v1("C:"), 1);
    check_int("git_path_absolute_drive_relative", cbm_rs_git_path_is_absolute_v1("C:project"), 1);
    check_int("git_path_absolute_drive_root", cbm_rs_git_path_is_absolute_v1("z:/project"), 1);
    check_int("git_path_absolute_drive_backslash", cbm_rs_git_path_is_absolute_v1("z:\\project"),
              1);
    check_int("git_path_absolute_drive_invalid", cbm_rs_git_path_is_absolute_v1("1:/project"), 0);
    check_int("git_path_absolute_drive_non_ascii", cbm_rs_git_path_is_absolute_v1("\xC4:project"),
              0);
    check_int("git_path_absolute_unc", cbm_rs_git_path_is_absolute_v1("\\\\server\\share"), 0);
#else
    check_int("git_path_absolute_non_windows_drive_only", cbm_rs_git_path_is_absolute_v1("C:"), 0);
    check_int("git_path_absolute_non_windows_drive", cbm_rs_git_path_is_absolute_v1("C:project"),
              0);
#endif

#ifdef CBM_USE_RUST_GIT_PATH_ABSOLUTE_ONLY
    check_bool("git_path_absolute_direct", cbm_git_path_is_absolute("/tmp/project"), true);
    check_bool("git_path_absolute_direct_relative", cbm_git_path_is_absolute("relative/project"),
               false);
    check_bool("git_path_absolute_direct_empty", cbm_git_path_is_absolute(""), false);
    check_bool("git_path_absolute_direct_null", cbm_git_path_is_absolute(NULL), false);
#endif
}

static void test_git_json_escaped_len_export(void) {
    check_int("git_json_escaped_len_plain", cbm_rs_git_json_escaped_len_v1("plain"), 5);
    check_int("git_json_escaped_len_quote", cbm_rs_git_json_escaped_len_v1("quote\\slash"), 12);
    check_int("git_json_escaped_len_newline", cbm_rs_git_json_escaped_len_v1("line\n"), 6);
    check_int("git_json_escaped_len_carriage_return", cbm_rs_git_json_escaped_len_v1("\r"), 2);
    check_int("git_json_escaped_len_tab", cbm_rs_git_json_escaped_len_v1("\t"), 2);
    check_int("git_json_escaped_len_control", cbm_rs_git_json_escaped_len_v1("\x01"), 6);
    check_int("git_json_escaped_len_null", cbm_rs_git_json_escaped_len_v1(NULL), 0);

#ifdef CBM_USE_RUST_GIT_JSON_ESCAPED_LEN_ONLY
    check_int("git_json_escaped_len_direct", cbm_git_json_escaped_len("quote\""), 7);
    check_int("git_json_escaped_len_direct_newline", cbm_git_json_escaped_len("\n"), 2);
    check_int("git_json_escaped_len_direct_carriage_return", cbm_git_json_escaped_len("\r"), 2);
    check_int("git_json_escaped_len_direct_tab", cbm_git_json_escaped_len("\t"), 2);
    check_int("git_json_escaped_len_direct_control", cbm_git_json_escaped_len("\x01"), 6);
    check_int("git_json_escaped_len_direct_null", cbm_git_json_escaped_len(NULL), 0);
#endif
}

static void test_git_trim_newlines_export(void) {
    char value[] = "output\r\n";
    cbm_rs_git_trim_newlines_v1(value);
    check_str("git_trim_newlines_value", value, "output");

    char only_newlines[] = "\r\n";
    cbm_rs_git_trim_newlines_v1(only_newlines);
    check_str("git_trim_newlines_only_newlines", only_newlines, "");

    char empty[] = "";
    cbm_rs_git_trim_newlines_v1(empty);
    check_str("git_trim_newlines_empty", empty, "");
    cbm_rs_git_trim_newlines_v1(NULL);

#ifdef CBM_USE_RUST_GIT_TRIM_NEWLINES_ONLY
    char direct[] = "direct\r";
    cbm_git_trim_newlines(direct);
    check_str("git_trim_newlines_direct", direct, "direct");
    cbm_git_trim_newlines(NULL);
#endif
}

static void test_log_copy_path_without_query_export(void) {
    char out[64];
    cbm_rs_log_copy_path_without_query_v1("/api/items?q=1", out, sizeof(out));
    check_str("log_copy_path_query", out, "/api/items");
    cbm_rs_log_copy_path_without_query_v1("/api/items#section", out, sizeof(out));
    check_str("log_copy_path_fragment", out, "/api/items");
    cbm_rs_log_copy_path_without_query_v1("/api/items", out, sizeof(out));
    check_str("log_copy_path_plain", out, "/api/items");
    cbm_rs_log_copy_path_without_query_v1("/api/items?q=1", out, 5);
    check_str("log_copy_path_truncated", out, "/api");
    cbm_rs_log_copy_path_without_query_v1(NULL, out, sizeof(out));
    check_str("log_copy_path_null", out, "");

    char one[1] = {'x'};
    cbm_rs_log_copy_path_without_query_v1("/api/items", one, sizeof(one));
    check_int("log_copy_path_one_byte", one[0], '\0');

    char untouched[] = "x";
    cbm_rs_log_copy_path_without_query_v1("/api/items", untouched, 0);
    check_str("log_copy_path_zero_bytes", untouched, "x");
    cbm_rs_log_copy_path_without_query_v1("/api/items", NULL, 0);

#ifdef CBM_USE_RUST_LOG_COPY_PATH_ONLY
    cbm_log_copy_path_without_query("/direct#section", out, sizeof(out));
    check_str("log_copy_path_direct", out, "/direct");
#endif
}

#ifdef CBM_USE_RUST_CLI_VERSION_ONLY
static void test_cli_version_direct_exports(void) {
    check_int("cli_version_direct_newer", cbm_cli_compare_versions_v1("0.10.0", "0.2.0"), 8);
    check_int("cli_version_direct_dev", cbm_cli_compare_versions_v1("0.2.1", "0.2.1-dev"), 1);
}
#endif

int main(void) {
    test_cli_version_exports();
#ifdef CBM_USE_RUST_CLI_VERSION_ONLY
    test_cli_version_direct_exports();
#endif
    test_cli_hook_token_export();
    test_git_path_absolute_export();
    test_git_json_escaped_len_export();
    test_git_trim_newlines_export();
    test_log_copy_path_without_query_export();
    test_mem_exports();
    test_dump_verify_exports();
    test_traces_exports();
    test_arena_exports();
    test_intern_null_contracts();
    test_intern_dedup_and_pool_lifetime();
    test_intern_embedded_nul_and_empty_pools();
    test_intern_pointer_stability_after_growth();
    test_intern_rejects_oversized_len_before_reading();
    test_discover_string_helpers_exports();
    test_discover_trailing_sep_export();
    test_discover_path_join_export();
    test_discover_local_rel_path_export();
    test_watcher_poll_interval_export();
    test_str_util_exports();
    test_platform_normalize_export();
    test_platform_env_dirs_exports();
#ifndef _WIN32
    test_platform_cgroup_exports();
#endif
    test_diagnostics_exports();
    test_compat_exports();
    test_log_profile_exports();
    test_hash_table_exports();
    test_yaml_exports();
    test_cypher_lexer_exports();
    test_cypher_parse_summary_export();
    test_cypher_single_char_kind_export();
    test_cypher_two_char_kind_export();
    test_cypher_scalar_func_index_exports();
    test_cypher_multiarg_func_index_exports();
    test_cypher_aggregate_func_index_exports();
    test_cypher_string_func_index_exports();
    test_mcp_jsonrpc_summary_export();
    test_mcp_initialize_protocol_version_export();
    test_mcp_tools_call_name_export();
    test_mcp_tools_call_arguments_export();
    test_mcp_get_string_arg_export();
    test_mcp_get_int_bool_arg_exports();
    test_mcp_edge_type_valid_export();
    test_mcp_search_path_arg_valid_export();
    test_mcp_search_args_valid_export();
    test_mcp_strip_root_prefix_offset_export();
    test_mcp_search_mode_export();
    test_mcp_index_mode_export();
    test_mcp_trace_mode_edge_mask_export();
    test_mcp_sanitize_ascii_export();
    test_mcp_search_code_score_export();
    test_mcp_search_score_cmp_export();
    test_mcp_search_top_dir_export();
    test_mcp_detect_changes_wants_symbols_export();
    test_mcp_detect_changes_impacted_label_export();
    test_mcp_detect_changes_status_path_offset_export();
    test_mcp_search_line_match_span_export();
    test_mcp_search_pick_resolved_index_export();
    test_mcp_search_pick_tightest_index_export();
    test_mcp_utf8_is_cont_export();
    test_mcp_node_resolution_score_export();
    test_mcp_adr_mode_export();
    test_mcp_adr_sections_json_export();
    test_mcp_bm25_build_match_export();
    test_mcp_bm25_file_pattern_like_export();
    test_mcp_sanitize_utf8_lossy_export();
    test_mcp_architecture_aspect_wanted_export();
    test_mcp_trace_is_test_file_export();
    test_mcp_project_db_file_name_export();
    test_mcp_cancel_request_matches_export();
    test_mcp_jsonrpc_format_error_export();
    test_mcp_jsonrpc_format_response_export();
    test_mcp_text_result_export();
    test_mcp_jsonrpc_parse_export();
#if !defined(CBM_USE_RUST_DISCOVER_USERCONFIG_ONLY) || \
    defined(CBM_USE_RUST_DISCOVER_LANGUAGE_ONLY) || \
    defined(CBM_USE_RUST_DISCOVER_LANGUAGE_NAME) || \
    defined(CBM_USE_RUST_DISCOVER_LANGUAGE_NAME_ONLY)
    test_discover_language_name_public_exports();
#endif
#if defined(CBM_USE_RUST_DISCOVER_LANGUAGE_NAME) || \
    defined(CBM_USE_RUST_DISCOVER_LANGUAGE_NAME_ONLY)
    test_discover_language_name_v1_exports();
#endif
#ifdef CBM_USE_RUST_DISCOVER_USERCONFIG_ONLY
    test_discover_userconfig_exports();
#endif
#if defined(CBM_USE_RUST_DISCOVER_USERCONFIG_ONLY) && \
    (defined(CBM_USE_RUST_DISCOVER_LANGUAGE_NAME) || \
     defined(CBM_USE_RUST_DISCOVER_LANGUAGE_NAME_ONLY))
    test_discover_userconfig_language_name_exports();
#endif
    test_discover_filter_exports();
#ifdef CBM_USE_RUST_MEM_ONLY
    test_mem_direct_exports();
#endif
#ifdef CBM_USE_RUST_DIAGNOSTICS_ONLY
    test_diagnostics_direct_exports();
#endif
#ifdef CBM_USE_RUST_PIPELINE_PATH_ALIAS_ONLY
    test_path_alias_direct_exports();
#endif
    test_mcp_tools_cursor_offset_exports();
    test_mcp_tool_index_export();
    test_mcp_tool_dispatch_index_export();
    test_mcp_tool_name_export();
    test_mcp_tool_title_export();
    test_mcp_tool_description_export();
    test_mcp_tool_input_schema_export();
    test_mcp_tool_output_schema_export();
    test_mcp_tools_list_json_export();
    test_mcp_content_length_export();
    test_mcp_content_length_raw_export();
    test_mcp_content_length_header_matches_export();
    test_mcp_content_length_header_blank_export();
    test_mcp_content_length_response_export();
    test_mcp_parse_file_uri_export();
    test_mcp_method_kind_export();
    test_mcp_method_not_found_error_export();
    test_mcp_parse_error_message_export();
    test_mcp_ping_result_export();
    test_mcp_tools_call_default_arguments_export();
    test_mcp_missing_tool_name_message_export();
    test_mcp_missing_project_error_export();
    test_mcp_project_not_found_message_export();
    test_mcp_project_list_error_export();
    test_mcp_unknown_tool_message_export();
    test_mcp_initialize_response_export();
    test_mcp_tools_page_bounds_export();
    test_pipeline_project_name_exports();
    extern void test_rust_simhash_jaccard_exports(void);
    test_rust_simhash_jaccard_exports();
    extern void test_rust_pipeline_complexity_exports(void);
    test_rust_pipeline_complexity_exports();
    extern void test_rust_vmem_round_exports(void);
    test_rust_vmem_round_exports();
    extern void test_rust_pipeline_pkgmap_exports(void);
    test_rust_pipeline_pkgmap_exports();
    extern void test_rust_pipeline_configlink_exports(void);
    test_rust_pipeline_configlink_exports();
    extern void test_rust_split_command_exports(void);
    test_rust_split_command_exports();
    extern void test_pipeline_split_command_public_abi(void);
    test_pipeline_split_command_public_abi();
    extern void test_rust_cross_repo_json_exports(void);
    test_rust_cross_repo_json_exports();
    extern void test_rust_enrichment_exports(void);
    test_rust_enrichment_exports();
    extern void test_rust_calls_json_exports(void);
    test_rust_calls_json_exports();
    extern void test_rust_envscan_exports(void);
    test_rust_envscan_exports();
    extern void test_rust_lsp_classifier_exports(void);
    test_rust_lsp_classifier_exports();
    extern void test_pipeline_lsp_classifier_public_abi(void);
    test_pipeline_lsp_classifier_public_abi();
    extern void test_rust_semantic_json_exports(void);
    test_rust_semantic_json_exports();
    test_pipeline_configures_exports();
#ifdef CBM_USE_RUST_PIPELINE_CONFIGURES_ONLY
    test_pipeline_configures_direct_abi_exports();
#endif
    test_pipeline_infrascan_exports();
    {
        extern void test_pipeline_infrascan_clean_json_export(void);
        test_pipeline_infrascan_clean_json_export();
    }
    {
        extern void test_rust_pipeline_definitions_json_export(void);
        test_rust_pipeline_definitions_json_export();
    }
    {
        extern void test_rust_pipeline_similarity_fp_export(void);
        test_rust_pipeline_similarity_fp_export();
    }
    {
        extern void test_rust_cli_archive_exports(void);
        test_rust_cli_archive_exports();
    }
    {
        extern void test_rust_pipeline_k8s_helpers_export(void);
        test_rust_pipeline_k8s_helpers_export();
    }
    {
        extern void test_rust_pipeline_incremental_export(void);
        test_rust_pipeline_incremental_export();
    }
    {
        extern void test_rust_pipeline_incremental_registry_export(void);
        test_rust_pipeline_incremental_registry_export();
    }
    {
        extern void test_rust_pipeline_parallel_json_export(void);
        test_rust_pipeline_parallel_json_export();
    }
    {
        extern void test_rust_discover_language_markers_export(void);
        test_rust_discover_language_markers_export();
    }
    {
        extern void test_rust_pipeline_semantic_suffix_export(void);
        test_rust_pipeline_semantic_suffix_export();
    }
    {
        extern void test_rust_store_arch_json_prop_export(void);
        test_rust_store_arch_json_prop_export();
    }
    test_pipeline_githistory_export();
    test_pipeline_gitdiff_range_export();
    test_pipeline_gitdiff_name_status_export();
    test_pipeline_gitdiff_hunks_export();
    test_pipeline_module_dir_export();
    test_pipeline_route_canon_export();
    test_pipeline_route_args_export();
#ifdef CBM_USE_RUST_PIPELINE_ROUTE_ARGS_ONLY
    test_pipeline_route_args_direct_exports();
#endif
    test_pipeline_route_node_classifiers_export();
    test_pipeline_sveltekit_file_kind_export();
    test_pipeline_sveltekit_server_method_export();
    test_pipeline_extract_json_prop_export();
    test_pipeline_url_path_export();
#ifdef CBM_USE_RUST_PIPELINE_ROUTE_NODE_CLASSIFIERS_ONLY
    test_pipeline_route_node_classifiers_direct_exports();
#endif
    test_pipeline_test_detect_export();
    extern void test_pipeline_test_node_is_test_export(void);
    test_pipeline_test_node_is_test_export();
    extern void test_rust_ui_layout_helpers_export(void);
    test_rust_ui_layout_helpers_export();
    extern void test_rust_ui_httpd_helpers_export(void);
    test_rust_ui_httpd_helpers_export();
    extern void test_rust_ast_profile_classifiers_export(void);
    test_rust_ast_profile_classifiers_export();
    extern void test_rust_pipeline_usages_json_export(void);
    test_rust_pipeline_usages_json_export();
    extern void test_rust_semantic_token_classifiers_export(void);
    test_rust_semantic_token_classifiers_export();
    test_pipeline_test_to_prod_path_export();
    test_pipeline_checked_exception_export();
#ifdef CBM_USE_RUST_PIPELINE_CHECKED_EXCEPTION_ONLY
    test_pipeline_checked_exception_direct_exports();
#endif
    test_pipeline_artifact_path_export();
    test_pipeline_plan_exports();
    test_gbuf_mutation_metadata_exports();
    test_store_camel_split_exports();
    test_store_immutable_uri_exports();
    test_store_search_pattern_exports();
    test_store_arch_helper_exports();
    test_store_arch_path_scope_exports();
    test_store_file_ext_lower_exports();
    test_store_ext_lang_kind_exports();
    test_store_mmap_resolver_exports();
    test_store_schema_manifest_exports();
    test_registry_import_map_and_bare();
    test_registry_qualified_suffix();
    test_registry_suffix_candidate_selection();
    test_registry_is_test_qn_export();
    test_registry_is_test_qn_bridge();
    test_registry_candidate_cap();
    test_registry_import_reachability_penalty();
    test_registry_handle_lifecycle_and_persistent_resolve();
    test_registry_handle_import_map_suffix_and_truncation();
    return 0;
}

void test_rust_simhash_jaccard_exports(void) {
    extern double cbm_rs_simhash_jaccard_v1(const uint32_t *a, const uint32_t *b);
    extern void cbm_rs_simhash_to_hex_v1(const uint32_t *fp, char *buf, int bufsize);
    extern int cbm_rs_simhash_from_hex_v1(const char *hex, uint32_t *out);
    uint32_t left[64];
    uint32_t right[64];
    for (size_t i = 0; i < 64; i++) {
        left[i] = (uint32_t)(i * 17 + 3);
        right[i] = left[i];
    }
    for (size_t i = 0; i < 16; i++) {
        right[i] += 1;
    }
    double partial = cbm_rs_simhash_jaccard_v1(left, right);
    double null_value = cbm_rs_simhash_jaccard_v1(NULL, right);
    if (partial < 0.749 || partial > 0.751 || null_value != 0.0) {
        fprintf(stderr, "Rust simhash jaccard smoke test failed\n");
        exit(EXIT_FAILURE);
    }

    uint32_t original[64];
    uint32_t decoded[64] = {0};
    for (size_t i = 0; i < 64; i++) {
        original[i] = (uint32_t)(0xdead0000U + i);
    }
    char encoded[513];
    cbm_rs_simhash_to_hex_v1(original, encoded, (int)sizeof(encoded));
    if (strlen(encoded) != 512 || cbm_rs_simhash_from_hex_v1(encoded, decoded) == 0 ||
        memcmp(original, decoded, sizeof(original)) != 0) {
        fprintf(stderr, "Rust simhash hex smoke test failed\n");
        exit(EXIT_FAILURE);
    }
}

void test_rust_pipeline_complexity_exports(void) {
    extern int cbm_rs_pipeline_complexity_json_int_v1(const char *json, const char *key, int dflt);
    extern int cbm_rs_pipeline_complexity_json_bool_v1(const char *json, const char *key);
    const char *json = "{\"loop_depth\": 12, \"self_recursive\":true}";
    if (cbm_rs_pipeline_complexity_json_int_v1(json, "loop_depth", 0) != 12 ||
        cbm_rs_pipeline_complexity_json_int_v1(json, "missing", 7) != 7 ||
        cbm_rs_pipeline_complexity_json_bool_v1(json, "self_recursive") == 0 ||
        cbm_rs_pipeline_complexity_json_bool_v1(json, "missing") != 0 ||
        cbm_rs_pipeline_complexity_json_int_v1(NULL, "loop_depth", 9) != 9) {
        fprintf(stderr, "Rust pipeline complexity JSON smoke test failed\n");
        exit(EXIT_FAILURE);
    }
}

void test_rust_vmem_round_exports(void) {
    extern size_t cbm_rs_vmem_round_to_page_v1(size_t size, size_t page_size);
    if (cbm_rs_vmem_round_to_page_v1(0, 4096) != 0 ||
        cbm_rs_vmem_round_to_page_v1(1, 4096) != 4096 ||
        cbm_rs_vmem_round_to_page_v1(4097, 4096) != 8192 ||
        cbm_rs_vmem_round_to_page_v1(123, 0) != 0) {
        fprintf(stderr, "Rust vmem page-round smoke test failed\n");
        exit(EXIT_FAILURE);
    }
}

void test_rust_pipeline_pkgmap_exports(void) {
    extern int cbm_rs_pipeline_pkgmap_at_prefix_v1(const char *source, size_t available,
                                                   const char *prefix, int prefix_len);
    extern int cbm_rs_pipeline_pkgmap_find_line_value_offset_v1(const char *source, int source_len,
                                                                const char *prefix);
    extern size_t cbm_rs_pipeline_pkgmap_path_dirname_v1(char *buf, size_t bufsize,
                                                         const char *path);
    extern size_t cbm_rs_pipeline_pkgmap_strip_extension_v1(char *buf, size_t bufsize,
                                                            const char *path);
    extern size_t cbm_rs_pipeline_pkgmap_join_and_strip_v1(char *buf, size_t bufsize,
                                                           const char *dir, const char *entry);
    extern size_t cbm_rs_pipeline_pkgmap_build_entry_path_v1(char *buf, size_t bufsize,
                                                              const char *rel_path,
                                                              const char *suffix);
    const char *source = "other\n  module github.com/acme/project\nnext";
    char dirname[64];
    char stem[64];
    char joined[128];
    char built[128];
    if (cbm_rs_pipeline_pkgmap_at_prefix_v1("name=", 5, "name=", 5) == 0 ||
        cbm_rs_pipeline_pkgmap_at_prefix_v1("name", 4, "name=", 5) != 0 ||
        cbm_rs_pipeline_pkgmap_find_line_value_offset_v1(source, (int)strlen(source), "module ") !=
            15 ||
        cbm_rs_pipeline_pkgmap_find_line_value_offset_v1(source, (int)strlen(source), "missing") !=
            -1 ||
        cbm_rs_pipeline_pkgmap_path_dirname_v1(dirname, sizeof(dirname),
                                               "packages/foo/package.json") != strlen("packages/foo") ||
        strcmp(dirname, "packages/foo") != 0 ||
        cbm_rs_pipeline_pkgmap_strip_extension_v1(stem, sizeof(stem), "src/index.ts") !=
            strlen("src/index") ||
        strcmp(stem, "src/index") != 0 ||
        cbm_rs_pipeline_pkgmap_join_and_strip_v1(joined, sizeof(joined), "packages/foo",
                                                 "./src/index.ts") != strlen("packages/foo/src/index") ||
        strcmp(joined, "packages/foo/src/index") != 0 ||
        cbm_rs_pipeline_pkgmap_build_entry_path_v1(built, sizeof(built),
                                                   "packages/foo/package.toml", "src/lib") !=
            strlen("packages/foo/src/lib") ||
        strcmp(built, "packages/foo/src/lib") != 0) {
        fprintf(stderr, "Rust pipeline pkgmap text smoke test failed\n");
        exit(EXIT_FAILURE);
    }
}

void test_rust_pipeline_configlink_exports(void) {
    extern int cbm_rs_pipeline_configlink_is_manifest_file_v1(const char *basename);
    extern int cbm_rs_pipeline_configlink_is_dep_section_v1(const char *value);
    extern int cbm_rs_pipeline_configlink_is_cargo_dep_section_v1(const char *value);
    extern void cbm_rs_pipeline_configlink_lowercase_into_v1(const char *value, char *buf,
                                                             size_t bufsize);
    extern double cbm_rs_pipeline_configlink_match_dep_to_import_v1(const char *target_name,
                                                                     const char *target_qn,
                                                                     const char *dep_lower);
    if (cbm_rs_pipeline_configlink_is_manifest_file_v1("Cargo.toml") == 0 ||
        cbm_rs_pipeline_configlink_is_manifest_file_v1("main.rs") != 0 ||
        cbm_rs_pipeline_configlink_is_dep_section_v1("devDependencies") == 0 ||
        cbm_rs_pipeline_configlink_is_cargo_dep_section_v1("crate.dependencies") == 0 ||
        cbm_rs_pipeline_configlink_match_dep_to_import_v1("serde", NULL, "serde") < 0.949 ||
        cbm_rs_pipeline_configlink_match_dep_to_import_v1("SerdeCore", "crate.serde.core",
                                                          "serde") < 0.799) {
        fprintf(stderr, "Rust pipeline configlink smoke test failed\n");
        exit(EXIT_FAILURE);
    }
    char lower[5];
    cbm_rs_pipeline_configlink_lowercase_into_v1("SerdeCore", lower, sizeof(lower));
    if (strcmp(lower, "serd") != 0) {
        fprintf(stderr, "Rust pipeline configlink lowercase smoke test failed\n");
        exit(EXIT_FAILURE);
    }
}

void test_rust_split_command_exports(void) {
    extern int cbm_rs_pipeline_split_command_v1(const char *cmd, char **out, int max_out);
    extern size_t cbm_rs_pipeline_resolve_compile_path_v1(char *buf, size_t bufsize,
                                                          const char *path, const char *directory);
    char *out[4] = {NULL};
    int count = cbm_rs_pipeline_split_command_v1("cc -I\"include path\" 'file name.c'", out, 4);
    if (count != 3 || strcmp(out[0], "cc") != 0 || strcmp(out[1], "-Iinclude path") != 0 ||
        strcmp(out[2], "file name.c") != 0) {
        fprintf(stderr, "Rust split command smoke test failed\n");
        for (int i = 0; i < count && i < 4; i++) {
            free(out[i]);
        }
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < count; i++) {
        free(out[i]);
    }
    char resolved[64];
    if (cbm_rs_pipeline_resolve_compile_path_v1(resolved, sizeof(resolved), "src/a.c", "/repo") !=
            strlen("/repo/src/a.c") ||
        strcmp(resolved, "/repo/src/a.c") != 0) {
        fprintf(stderr, "Rust compile path smoke test failed\n");
        exit(EXIT_FAILURE);
    }
}

void test_pipeline_split_command_public_abi(void) {
    char sentinel_zero[] = "sentinel-zero";
    char sentinel_one[] = "sentinel-one";
    char sentinel_two[] = "sentinel-two";
    char *sentinels[3] = {sentinel_zero, sentinel_one, sentinel_two};

    if (cbm_split_command(NULL, sentinels, 3) != 0 || sentinels[0] != sentinel_zero ||
        sentinels[1] != sentinel_one || sentinels[2] != sentinel_two ||
        cbm_split_command("cc", NULL, 1) != 0 || cbm_split_command("cc", sentinels, 0) != 0 ||
        sentinels[0] != sentinel_zero || sentinels[1] != sentinel_one ||
        sentinels[2] != sentinel_two || cbm_split_command("cc", sentinels, -1) != 0 ||
        sentinels[0] != sentinel_zero || sentinels[1] != sentinel_one ||
        sentinels[2] != sentinel_two) {
        fprintf(stderr, "Pipeline split command public ABI sentinel test failed\n");
        exit(EXIT_FAILURE);
    }

    char *tokens[4] = {NULL};
    int count = cbm_split_command("cc -I\"include path\" 'file name.c'", tokens, 4);
    if (count != 3 || !tokens[0] || !tokens[1] || !tokens[2] || strcmp(tokens[0], "cc") != 0 ||
        strcmp(tokens[1], "-Iinclude path") != 0 || strcmp(tokens[2], "file name.c") != 0 ||
        tokens[0][strlen("cc")] != '\0' ||
        tokens[1][strlen("-Iinclude path")] != '\0' ||
        tokens[2][strlen("file name.c")] != '\0') {
        fprintf(stderr, "Pipeline split command public ABI quote test failed\n");
        for (int i = 0; i < 4; i++) {
            free(tokens[i]);
        }
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < 4; i++) {
        free(tokens[i]);
    }

    char *limited[2] = {NULL};
    count = cbm_split_command("one two three", limited, 2);
    if (count != 2 || !limited[0] || !limited[1] || strcmp(limited[0], "one") != 0 ||
        strcmp(limited[1], "two") != 0 || limited[0][strlen("one")] != '\0' ||
        limited[1][strlen("two")] != '\0') {
        fprintf(stderr, "Pipeline split command public ABI max-out test failed\n");
        free(limited[0]);
        free(limited[1]);
        exit(EXIT_FAILURE);
    }
    free(limited[0]);
    free(limited[1]);

    const size_t boundary_len = 4095;
    char *boundary_input = malloc(boundary_len + 1);
    if (!boundary_input) {
        fprintf(stderr, "Pipeline split command public ABI boundary setup failed\n");
        exit(EXIT_FAILURE);
    }
    memset(boundary_input, 'x', boundary_len);
    boundary_input[boundary_len] = '\0';

    char *boundary[1] = {NULL};
    count = cbm_split_command(boundary_input, boundary, 1);
    if (count != 1 || !boundary[0] || strlen(boundary[0]) != boundary_len ||
        boundary[0][boundary_len] != '\0') {
        fprintf(stderr, "Pipeline split command public ABI boundary test failed\n");
        free(boundary[0]);
        free(boundary_input);
        exit(EXIT_FAILURE);
    }
    free(boundary[0]);
    free(boundary_input);
}

void test_rust_cross_repo_json_exports(void) {
    extern int cbm_rs_pipeline_cross_repo_json_str_prop_v1(const char *json, const char *key,
                                                           char *buf, size_t bufsz);
    extern size_t cbm_rs_pipeline_cross_repo_build_props_v1(char *buf, size_t bufsz,
                                                            const char *target_project,
                                                            const char *target_function,
                                                            const char *target_file,
                                                            const char *url_or_channel,
                                                            const char *extra_key,
                                                            const char *extra_val);
    char value[32];
    char small[5];
    char props[256];
    const char *json = "{\"url_path\":\"/api/items\",\"method\":\"GET\"}";
    if (cbm_rs_pipeline_cross_repo_json_str_prop_v1(json, "url_path", value, sizeof(value)) == 0 ||
        strcmp(value, "/api/items") != 0 ||
        cbm_rs_pipeline_cross_repo_json_str_prop_v1(json, "url_path", small, sizeof(small)) == 0 ||
        strcmp(small, "/api") != 0 ||
        cbm_rs_pipeline_cross_repo_json_str_prop_v1(json, "missing", value, sizeof(value)) != 0 ||
        cbm_rs_pipeline_cross_repo_build_props_v1(props, sizeof(props), "target", "handler",
                                                  "src.rs", "/api", "url_path", "GET") == 0 ||
        strcmp(props,
               "{\"target_project\":\"target\",\"target_function\":\"handler\",\"target_file\":\"src.rs\",\"url_path\":\"/api\",\"transport\":\"GET\"}") !=
            0) {
        fprintf(stderr, "Rust cross-repo JSON smoke test failed\n");
        exit(EXIT_FAILURE);
    }
}

void test_rust_enrichment_exports(void) {
    extern int cbm_rs_pipeline_split_camel_case_v1(const char *value, char **out, int max_out);
    extern int cbm_rs_pipeline_tokenize_decorator_v1(const char *value, char **out, int max_out);
    char *parts[8] = {NULL};
    char *tokens[8] = {NULL};
    int part_count = cbm_rs_pipeline_split_camel_case_v1("loadHTTPServer", parts, 8);
    int token_count = cbm_rs_pipeline_tokenize_decorator_v1("@router.getUser_config(arg)", tokens, 8);
    if (part_count != 2 || strcmp(parts[0], "load") != 0 || strcmp(parts[1], "HTTPServer") != 0 ||
        token_count != 2 || strcmp(tokens[0], "user") != 0 || strcmp(tokens[1], "config") != 0) {
        fprintf(stderr, "Rust enrichment tokenizer smoke test failed\n");
        for (int i = 0; i < part_count && i < 8; i++) {
            free(parts[i]);
        }
        for (int i = 0; i < token_count && i < 8; i++) {
            free(tokens[i]);
        }
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < part_count; i++) {
        free(parts[i]);
    }
    for (int i = 0; i < token_count; i++) {
        free(tokens[i]);
    }
}

void test_rust_calls_json_exports(void) {
    extern size_t cbm_rs_pipeline_calls_extract_local_name_v1(char *buf, size_t bufsize,
                                                              const char *json);
    char value[32];
    const char *json = "{\"local_name\":\"client\",\"module\":\"pkg\"}";
    if (cbm_rs_pipeline_calls_extract_local_name_v1(value, sizeof(value), json) != strlen("client") ||
        strcmp(value, "client") != 0 ||
        cbm_rs_pipeline_calls_extract_local_name_v1(value, sizeof(value), "{\"local_name\":\"\"}") !=
            SIZE_MAX) {
        fprintf(stderr, "Rust calls JSON smoke test failed\n");
        exit(EXIT_FAILURE);
    }
}

void test_rust_envscan_exports(void) {
    extern int cbm_rs_pipeline_envscan_is_dockerfile_name_v1(const char *name);
    extern int cbm_rs_pipeline_envscan_is_env_file_name_v1(const char *name);
    extern int cbm_rs_pipeline_envscan_is_secret_file_v1(const char *name);
    extern int cbm_rs_pipeline_envscan_is_ignored_dir_v1(const char *name);
    extern int cbm_rs_pipeline_envscan_detect_file_type_v1(const char *name);
    if (cbm_rs_pipeline_envscan_is_dockerfile_name_v1("Dockerfile.prod") == 0 ||
        cbm_rs_pipeline_envscan_is_env_file_name_v1("settings.env") == 0 ||
        cbm_rs_pipeline_envscan_is_secret_file_v1("service_account.json") == 0 ||
        cbm_rs_pipeline_envscan_is_ignored_dir_v1("node_modules") == 0 ||
        cbm_rs_pipeline_envscan_detect_file_type_v1("config.yaml") != 2 ||
        cbm_rs_pipeline_envscan_detect_file_type_v1("main.YML") != 0) {
        fprintf(stderr, "Rust envscan classifier smoke test failed\n");
        exit(EXIT_FAILURE);
    }
}

void test_rust_lsp_classifier_exports(void) {
    extern int cbm_rs_pipeline_lsp_ts_modes_v1(int language, const char *rel_path);
    extern int cbm_rs_pipeline_lsp_has_cross_lsp_v1(int language);
    extern int cbm_rs_pipeline_lsp_map_label_allowed_v1(const char *label);
    if (cbm_rs_pipeline_lsp_ts_modes_v1(2, "src/app.jsx") != 3 ||
        cbm_rs_pipeline_lsp_ts_modes_v1(3, "src/types.d.ts") != 4 ||
        cbm_rs_pipeline_lsp_has_cross_lsp_v1(5) == 0 ||
        cbm_rs_pipeline_lsp_has_cross_lsp_v1(CBM_LANG_C) == 0 ||
        cbm_rs_pipeline_lsp_has_cross_lsp_v1(CBM_LANG_BASH) != 0 ||
        cbm_rs_pipeline_lsp_has_cross_lsp_v1(31) != 0 ||
        cbm_rs_pipeline_lsp_map_label_allowed_v1("Struct") == 0 ||
        cbm_rs_pipeline_lsp_map_label_allowed_v1("Variable") != 0) {
        fprintf(stderr, "Rust LSP classifier smoke test failed\n");
        exit(EXIT_FAILURE);
    }
}

void test_pipeline_lsp_classifier_public_abi(void) {
    static const char lsp_label_struct_nul[] = "Struct\0Variable";
    static const char lsp_label_variable_nul[] = "Variable\0Struct";
    static const char lsp_label_high_byte[] = "\xE9";
    static const char *const allowed_labels[] = {
        "Class", "Struct", "Interface", "Enum", "Type", "Trait", "Protocol", "Function", "Method",
    };
    bool js = false;
    bool jsx = false;
    bool dts = false;

    cbm_pxc_ts_modes(CBM_LANG_JAVASCRIPT, "src/app.jsx", &js, &jsx, &dts);
    check_bool("lsp_public_ts_js", js, true);
    check_bool("lsp_public_ts_jsx_suffix", jsx, true);
    check_bool("lsp_public_ts_jsx_suffix_dts", dts, false);

    js = false;
    jsx = true;
    dts = true;
    cbm_pxc_ts_modes(CBM_LANG_JAVASCRIPT, NULL, &js, &jsx, &dts);
    check_bool("lsp_public_ts_null_path_js", js, true);
    check_bool("lsp_public_ts_null_path_jsx", jsx, false);
    check_bool("lsp_public_ts_null_path_dts", dts, false);

    js = true;
    jsx = true;
    dts = false;
    cbm_pxc_ts_modes(CBM_LANG_TYPESCRIPT, "src/types.d.ts", &js, &jsx, &dts);
    check_bool("lsp_public_ts_dts_js", js, false);
    check_bool("lsp_public_ts_dts_jsx", jsx, false);
    check_bool("lsp_public_ts_dts", dts, true);

    js = true;
    jsx = false;
    dts = true;
    cbm_pxc_ts_modes(CBM_LANG_TSX, "src/component.tsx", &js, &jsx, &dts);
    check_bool("lsp_public_tsx_js", js, false);
    check_bool("lsp_public_tsx_jsx", jsx, true);
    check_bool("lsp_public_tsx_dts", dts, false);

    js = true;
    jsx = true;
    dts = true;
    cbm_pxc_ts_modes(CBM_LANG_BASH, "scripts/run.sh", &js, &jsx, &dts);
    check_bool("lsp_public_ts_unknown_js", js, false);
    check_bool("lsp_public_ts_unknown_jsx", jsx, false);
    check_bool("lsp_public_ts_unknown_dts", dts, false);

    check_bool("lsp_public_has_go", cbm_pxc_has_cross_lsp(CBM_LANG_GO), true);
    check_bool("lsp_public_has_c", cbm_pxc_has_cross_lsp(CBM_LANG_C), true);
    check_bool("lsp_public_has_rust", cbm_pxc_has_cross_lsp(CBM_LANG_RUST), true);
    check_bool("lsp_public_has_bash", cbm_pxc_has_cross_lsp(CBM_LANG_BASH), false);
    check_bool("lsp_public_has_unknown", cbm_pxc_has_cross_lsp((CBMLanguage)9999), false);

    for (size_t i = 0; i < sizeof(allowed_labels) / sizeof(allowed_labels[0]); i++) {
        check_ptr_eq("lsp_public_map_allowed_identity", cbm_pxc_map_label(allowed_labels[i]),
                     allowed_labels[i]);
    }
    check_null("lsp_public_map_null", cbm_pxc_map_label(NULL));
    check_null("lsp_public_map_rejected", cbm_pxc_map_label("Variable"));
    check_null("lsp_public_map_case", cbm_pxc_map_label("struct"));
    check_null("lsp_public_map_high_byte", cbm_pxc_map_label(lsp_label_high_byte));
    check_ptr_eq("lsp_public_map_embedded_nul_allowed", cbm_pxc_map_label(lsp_label_struct_nul),
                 lsp_label_struct_nul);
    check_null("lsp_public_map_embedded_nul_rejected", cbm_pxc_map_label(lsp_label_variable_nul));
}

void test_rust_semantic_json_exports(void) {
    extern int cbm_rs_pipeline_semantic_json_str_value_v1(const char *json, const char *key,
                                                          char *buf, int bufsize);
    extern int cbm_rs_pipeline_semantic_json_str_array_v1(const char *json, const char *key,
                                                          char **out, int max_out);
    char value[16];
    char *items[4] = {NULL};
    const char *json = "{\"bt\":\"alpha\",\"tokens\":[\"one\",\"two\"]}";
    int count = cbm_rs_pipeline_semantic_json_str_array_v1(json, "tokens", items, 4);
    if (cbm_rs_pipeline_semantic_json_str_value_v1(json, "bt", value, sizeof(value)) == 0 ||
        strcmp(value, "alpha") != 0 || count != 2 || strcmp(items[0], "one") != 0 ||
        strcmp(items[1], "two") != 0) {
        fprintf(stderr, "Rust semantic JSON smoke test failed\n");
        for (int i = 0; i < count && i < 4; i++) {
            free(items[i]);
        }
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < count; i++) {
        free(items[i]);
    }
}

extern void cbm_rs_pipeline_clean_json_brackets_v1(const char *s, char *out, size_t out_sz);
extern void cbm_clean_json_brackets(const char *s, char *out, size_t out_sz);

void test_pipeline_infrascan_clean_json_export(void) {
    char buf[64];
    cbm_rs_pipeline_clean_json_brackets_v1("[\"alpha\", \"beta\",\t\"gamma\"]", buf, sizeof(buf));
    check_int("infrascan_clean_json_array", strcmp(buf, "alpha beta gamma"), 0);

    cbm_rs_pipeline_clean_json_brackets_v1("plain value", buf, 7);
    check_int("infrascan_clean_json_plain_truncate", strcmp(buf, "plain "), 0);

    cbm_rs_pipeline_clean_json_brackets_v1("[\"alpha\", \"beta\"]", buf, 7);
    check_int("infrascan_clean_json_array_truncate", strcmp(buf, "alpha"), 0);

    cbm_clean_json_brackets("[\"alpha\", \"beta\",\t\"gamma\"]", buf, sizeof(buf));
    check_int("infrascan_clean_json_bridge_array", strcmp(buf, "alpha beta gamma"), 0);

    cbm_clean_json_brackets("plain value", buf, 7);
    check_int("infrascan_clean_json_bridge_plain_truncate", strcmp(buf, "plain "), 0);

    cbm_clean_json_brackets("[\"alpha\", \"beta\"]", buf, 7);
    check_int("infrascan_clean_json_bridge_array_truncate", strcmp(buf, "alpha"), 0);

    snprintf(buf, sizeof(buf), "%s", "sentinel");
    cbm_clean_json_brackets(NULL, buf, sizeof(buf));
    check_int("infrascan_clean_json_bridge_null_input", strcmp(buf, "sentinel"), 0);

    cbm_clean_json_brackets("value", NULL, sizeof(buf));
    snprintf(buf, sizeof(buf), "%s", "sentinel");
    cbm_clean_json_brackets("value", buf, 0);
    check_int("infrascan_clean_json_bridge_zero_size", strcmp(buf, "sentinel"), 0);
}

extern int cbm_rs_pipeline_definitions_json_escape_char(char *output, size_t available, int byte);
extern size_t cbm_rs_pipeline_definitions_json_escaped_len(const char *value);
extern int cbm_pipeline_definitions_json_escape_char(char *output, size_t available, int byte);
extern size_t cbm_pipeline_definitions_json_escaped_len(const char *value);

void test_rust_pipeline_definitions_json_export(void) {
    char output[2] = {0, 0};
    int length = cbm_rs_pipeline_definitions_json_escape_char(output, sizeof(output), '"');
    check_int("definitions_json_escape_quote_len", length, 2);
    check_int("definitions_json_escape_quote_slash", (unsigned char)output[0], '\\');
    check_int("definitions_json_escape_quote_value", (unsigned char)output[1], '"');

    output[0] = '?';
    output[1] = '!';
    length = cbm_rs_pipeline_definitions_json_escape_char(output, 1, '\n');
    check_int("definitions_json_escape_short_len", length, 2);
    check_int("definitions_json_escape_short_unchanged", (unsigned char)output[0], '?');
    check_int("definitions_json_escape_null_output",
              cbm_rs_pipeline_definitions_json_escape_char(NULL, 2, '\t'), 2);
    length = cbm_rs_pipeline_definitions_json_escape_char(output, sizeof(output), 0xe9);
    check_int("definitions_json_escape_unsigned_len", length, 1);
    check_int("definitions_json_escape_unsigned_byte", (unsigned char)output[0], 0xe9);
    check_int("definitions_json_escape_len", (int)cbm_rs_pipeline_definitions_json_escaped_len("a\"b\\c\nd"),
              10);
    check_int("definitions_json_escape_null_len",
              (int)cbm_rs_pipeline_definitions_json_escaped_len(NULL), 0);

    output[0] = '?';
    output[1] = '!';
    length = cbm_pipeline_definitions_json_escape_char(output, 1, '\n');
    check_int("definitions_json_bridge_short_len", length, 2);
    check_int("definitions_json_bridge_short_unchanged", (unsigned char)output[0], '?');
    check_int("definitions_json_bridge_null_output",
              cbm_pipeline_definitions_json_escape_char(NULL, 2, '\t'), 2);
    length = cbm_pipeline_definitions_json_escape_char(output, sizeof(output), '\f');
    check_int("definitions_json_bridge_control_len", length, 1);
    check_int("definitions_json_bridge_control_byte", (unsigned char)output[0], ' ');
    length = cbm_pipeline_definitions_json_escape_char(output, sizeof(output), 0xe9);
    check_int("definitions_json_bridge_unsigned_len", length, 1);
    check_int("definitions_json_bridge_unsigned_byte", (unsigned char)output[0], 0xe9);
    check_int("definitions_json_bridge_len",
              (int)cbm_pipeline_definitions_json_escaped_len("a\"b\\c\nd"), 10);
    check_int("definitions_json_bridge_null_len",
              (int)cbm_pipeline_definitions_json_escaped_len(NULL), 0);
}

extern int cbm_rs_pipeline_similarity_parse_fp_v1(const char *props_json, uint32_t *out);
extern int cbm_pipeline_similarity_parse_fp(const char *props_json, uint32_t *out);

void test_rust_pipeline_similarity_fp_export(void) {
    char props[521];
    uint32_t output[64] = {0};
    uint32_t bridge_output[64] = {0};
    strcpy(props, "{\"fp\":\"");
    memset(props + strlen(props), '0', 512);
    props[519] = '"';
    props[520] = '\0';
    check_int("similarity_fp_valid", cbm_rs_pipeline_similarity_parse_fp_v1(props, output), 1);
    check_int("similarity_fp_zero", (int)output[0], 0);
    check_int("similarity_fp_short", cbm_rs_pipeline_similarity_parse_fp_v1("{\"fp\":\"00\"}", output),
              0);
    check_int("similarity_fp_null", cbm_rs_pipeline_similarity_parse_fp_v1(NULL, output), 0);
    check_int("similarity_fp_null_output", cbm_rs_pipeline_similarity_parse_fp_v1(props, NULL), 0);

    check_int("similarity_fp_bridge_valid", cbm_pipeline_similarity_parse_fp(props, bridge_output),
              1);
    check_int("similarity_fp_bridge_zero", (int)bridge_output[0], 0);
    bridge_output[0] = 12345;
    check_int("similarity_fp_bridge_short",
              cbm_pipeline_similarity_parse_fp("{\"fp\":\"00\"}", bridge_output), 0);
    check_int("similarity_fp_bridge_short_unchanged", (int)bridge_output[0], 12345);
    check_int("similarity_fp_bridge_null_input",
              cbm_pipeline_similarity_parse_fp(NULL, bridge_output), 0);
    check_int("similarity_fp_bridge_null_output", cbm_pipeline_similarity_parse_fp(props, NULL), 0);
    props[strlen("{\"fp\":\"")] = 'g';
    check_int("similarity_fp_bridge_loose_hex",
              cbm_pipeline_similarity_parse_fp(props, bridge_output), 1);
    check_int("similarity_fp_bridge_loose_hex_zero", (int)bridge_output[0], 0);
}

extern int cbm_rs_cli_archive_tar_end_v1(const unsigned char *header);
extern uint16_t cbm_rs_cli_archive_zip_read_u16_v1(const unsigned char *data, int offset);
extern uint32_t cbm_rs_cli_archive_zip_read_u32_v1(const unsigned char *data, int offset);

void test_rust_cli_archive_exports(void) {
    unsigned char header[512] = {0};
    unsigned char zip_header[] = {0x34, 0x12};
    unsigned char zip_word[] = {0x78, 0x56, 0x34, 0x12};
    check_int("cli_archive_tar_end", cbm_rs_cli_archive_tar_end_v1(header), 1);
    header[0] = 1;
    check_int("cli_archive_tar_nonzero", cbm_rs_cli_archive_tar_end_v1(header), 0);
    check_int("cli_archive_tar_null", cbm_rs_cli_archive_tar_end_v1(NULL), 0);
    check_int("cli_archive_zip_u16", cbm_rs_cli_archive_zip_read_u16_v1(zip_header, 0), 0x1234);
    check_int("cli_archive_zip_u32", (int)cbm_rs_cli_archive_zip_read_u32_v1(zip_word, 0), 0x12345678);
    check_int("cli_archive_zip_negative", cbm_rs_cli_archive_zip_read_u16_v1(zip_header, -1), 0);
}

extern size_t cbm_rs_pipeline_k8s_basename_offset_v1(const char *path);
extern int cbm_rs_pipeline_k8s_indent_v1(const char *line);
extern int cbm_rs_pipeline_k8s_split_kv_v1(const char *text, char *key, size_t key_size,
                                            char *value, size_t value_size);

void test_rust_pipeline_k8s_helpers_export(void) {
    char key[32];
    char value[32];
    check_int("k8s_basename_offset", (int)cbm_rs_pipeline_k8s_basename_offset_v1("apps/deploy.yaml"),
              5);
    check_int("k8s_basename_null", (int)cbm_rs_pipeline_k8s_basename_offset_v1(NULL), 0);
    check_int("k8s_indent", cbm_rs_pipeline_k8s_indent_v1("  name"), 2);
    check_int("k8s_indent_tab", cbm_rs_pipeline_k8s_indent_v1("\tname"), 0);
    check_int("k8s_split_kv", cbm_rs_pipeline_k8s_split_kv_v1("name: 'frontend' # comment", key,
                                                               sizeof(key), value, sizeof(value)),
              1);
    check_str("k8s_split_kv_key", key, "name");
    check_str("k8s_split_kv_value", value, "frontend");
    check_int("k8s_split_kv_comment", cbm_rs_pipeline_k8s_split_kv_v1("# comment", key,
                                                                      sizeof(key), value,
                                                                      sizeof(value)),
              0);
}

extern int cbm_rs_pipeline_incremental_edge_type_is_recomputed_v1(const char *edge_type);

void test_rust_pipeline_incremental_export(void) {
    check_int("incremental_recomputed_similarity",
              cbm_rs_pipeline_incremental_edge_type_is_recomputed_v1("SIMILAR_TO"), 1);
    check_int("incremental_recomputed_semantic",
              cbm_rs_pipeline_incremental_edge_type_is_recomputed_v1("SEMANTICALLY_RELATED"),
              1);
    check_int("incremental_recomputed_file_changes",
              cbm_rs_pipeline_incremental_edge_type_is_recomputed_v1("FILE_CHANGES_WITH"), 1);
    check_int("incremental_recomputed_data_flows",
              cbm_rs_pipeline_incremental_edge_type_is_recomputed_v1("DATA_FLOWS"), 1);
    check_int("incremental_recomputed_calls",
              cbm_rs_pipeline_incremental_edge_type_is_recomputed_v1("CALLS"), 0);
    check_int("incremental_recomputed_empty",
              cbm_rs_pipeline_incremental_edge_type_is_recomputed_v1(""), 0);
    check_int("incremental_recomputed_null",
              cbm_rs_pipeline_incremental_edge_type_is_recomputed_v1(NULL), 0);
#ifdef CBM_USE_RUST_PIPELINE_INCREMENTAL_EDGE_ONLY
    check_bool("incremental_direct_similar",
               cbm_pipeline_incremental_edge_type_is_recomputed("SIMILAR_TO"), true);
    check_bool("incremental_direct_calls",
               cbm_pipeline_incremental_edge_type_is_recomputed("CALLS"), false);
    check_bool("incremental_direct_null",
               cbm_pipeline_incremental_edge_type_is_recomputed(NULL), false);
#endif
}

extern int cbm_rs_pipeline_parallel_json_escape_char(char *output, size_t available, int byte);
extern size_t cbm_rs_pipeline_parallel_json_escaped_len(const char *value);
extern int cbm_pipeline_parallel_json_escape_char(char *output, size_t available, int byte);
extern size_t cbm_pipeline_parallel_json_escaped_len(const char *value);

void test_rust_pipeline_parallel_json_export(void) {
    char output[2] = {0, 0};
    int length = cbm_rs_pipeline_parallel_json_escape_char(output, sizeof(output), '"');
    check_int("parallel_json_escape_quote_len", length, 2);
    check_int("parallel_json_escape_quote_slash", (unsigned char)output[0], '\\');
    check_int("parallel_json_escape_quote_value", (unsigned char)output[1], '"');
    check_int("parallel_json_escape_len",
              (int)cbm_rs_pipeline_parallel_json_escaped_len("a\"b\\c\nd"), 10);
    check_int("parallel_json_escape_null_len",
              (int)cbm_rs_pipeline_parallel_json_escaped_len(NULL), 0);

    output[0] = '?';
    output[1] = '!';
    length = cbm_pipeline_parallel_json_escape_char(output, 1, '\n');
    check_int("parallel_json_bridge_short_len", length, 2);
    check_int("parallel_json_bridge_short_unchanged", (unsigned char)output[0], '?');

    length = cbm_pipeline_parallel_json_escape_char(output, sizeof(output), '\f');
    check_int("parallel_json_bridge_control_len", length, 1);
    check_int("parallel_json_bridge_control_space", (unsigned char)output[0], ' ');
    check_int("parallel_json_bridge_null_output", cbm_pipeline_parallel_json_escape_char(NULL, 2, '\t'), 2);
    check_int("parallel_json_bridge_null_len", (int)cbm_pipeline_parallel_json_escaped_len(NULL), 0);
}

extern int cbm_rs_discover_language_marker_v1(const char *buffer, int kind);
extern int cbm_discover_language_marker(const char *buffer, int kind);

void test_rust_discover_language_markers_export(void) {
    static const char magma_nul_terminated[] = "intrinsic Foo\0(";

    check_int("discover_marker_objc", cbm_rs_discover_language_marker_v1("@implementation Foo", 0), 1);
    check_int("discover_marker_objc_false", cbm_rs_discover_language_marker_v1("function foo", 0), 0);
    check_int("discover_marker_magma_end", cbm_rs_discover_language_marker_v1("end procedure;", 1), 1);
    check_int("discover_marker_magma_callable",
              cbm_rs_discover_language_marker_v1("intrinsic Foo(x)", 2), 1);
    check_int("discover_marker_magma_non_ascii",
              cbm_rs_discover_language_marker_v1("intrinsic \xE9(", 2), 0);
    check_int("discover_marker_magma_ascii_open",
              cbm_rs_discover_language_marker_v1("intrinsic Foo(", 2), 1);
    check_int("discover_marker_magma_empty_name",
              cbm_rs_discover_language_marker_v1("intrinsic (", 2), 0);
    check_int("discover_marker_magma_false",
              cbm_rs_discover_language_marker_v1("intrinsic Foo", 2), 0);
    check_int("discover_marker_magma_nul_terminated",
              cbm_rs_discover_language_marker_v1(magma_nul_terminated, 2), 0);
    check_int("discover_marker_matlab", cbm_rs_discover_language_marker_v1("  classdef Foo", 3), 1);
    check_int("discover_marker_matlab_block", cbm_rs_discover_language_marker_v1("%{ block", 3), 0);
    check_int("discover_marker_null", cbm_rs_discover_language_marker_v1(NULL, 0), 0);
    check_int("discover_marker_invalid_kind", cbm_rs_discover_language_marker_v1("function foo", 99),
              0);

    check_int("discover_marker_bridge_objc", cbm_discover_language_marker("@interface Foo", 0), 1);
    check_int("discover_marker_bridge_magma", cbm_discover_language_marker("intrinsic Foo(x)", 2),
              1);
    check_int("discover_marker_bridge_magma_non_ascii",
              cbm_discover_language_marker("intrinsic \xE9(", 2), 0);
    check_int("discover_marker_bridge_magma_ascii_open",
              cbm_discover_language_marker("intrinsic Foo(", 2), 1);
    check_int("discover_marker_bridge_magma_empty_name",
              cbm_discover_language_marker("intrinsic (", 2), 0);
    check_int("discover_marker_bridge_magma_no_open",
              cbm_discover_language_marker("intrinsic Foo", 2), 0);
    check_int("discover_marker_bridge_magma_nul_terminated",
              cbm_discover_language_marker(magma_nul_terminated, 2), 0);
    check_int("discover_marker_bridge_matlab", cbm_discover_language_marker("%{ block", 3), 0);
    check_int("discover_marker_bridge_unknown", cbm_discover_language_marker("function foo", 99),
              0);
    check_int("discover_marker_bridge_null", cbm_discover_language_marker(NULL, 0), 0);
}

extern int cbm_rs_pipeline_incremental_label_is_registry_symbol_v1(const char *label);

void test_rust_pipeline_incremental_registry_export(void) {
    check_int("incremental_registry_function",
              cbm_rs_pipeline_incremental_label_is_registry_symbol_v1("Function"), 1);
    check_int("incremental_registry_struct",
              cbm_rs_pipeline_incremental_label_is_registry_symbol_v1("Struct"), 1);
    check_int("incremental_registry_trait",
              cbm_rs_pipeline_incremental_label_is_registry_symbol_v1("Trait"), 1);
    check_int("incremental_registry_variable",
              cbm_rs_pipeline_incremental_label_is_registry_symbol_v1("Variable"), 1);
    check_int("incremental_registry_module",
              cbm_rs_pipeline_incremental_label_is_registry_symbol_v1("Module"), 0);
    check_int("incremental_registry_null",
              cbm_rs_pipeline_incremental_label_is_registry_symbol_v1(NULL), 0);

#ifdef CBM_USE_RUST_PIPELINE_INCREMENTAL_REGISTRY_LABEL_ONLY
    check_bool("incremental_registry_direct_fn",
               cbm_pipeline_incremental_label_is_registry_symbol("Function"), true);
    check_bool("incremental_registry_direct_module",
               cbm_pipeline_incremental_label_is_registry_symbol("Module"), false);
    check_bool("incremental_registry_direct_null",
               cbm_pipeline_incremental_label_is_registry_symbol(NULL), false);
#endif
}

extern int cbm_rs_pipeline_semantic_fp_ends_with_v1(const char *file_path, const char *suffix);

void test_rust_pipeline_semantic_suffix_export(void) {
    check_int("semantic_fp_suffix_go", cbm_rs_pipeline_semantic_fp_ends_with_v1("pkg/api.go", ".go"),
              1);
    check_int("semantic_fp_suffix_false",
              cbm_rs_pipeline_semantic_fp_ends_with_v1("pkg/api.py", ".go"), 0);
    check_int("semantic_fp_suffix_empty", cbm_rs_pipeline_semantic_fp_ends_with_v1("pkg/api.go", ""),
              1);
    check_int("semantic_fp_suffix_null", cbm_rs_pipeline_semantic_fp_ends_with_v1(NULL, ".go"), 0);
    check_int("semantic_fp_suffix_case",
              cbm_rs_pipeline_semantic_fp_ends_with_v1("pkg/api.GO", ".go"), 0);
#ifdef CBM_USE_RUST_PIPELINE_SEMANTIC_FP_SUFFIX_ONLY
    check_bool("semantic_direct_go", cbm_pipeline_semantic_fp_ends_with("pkg/api.go", ".go"), true);
    check_bool("semantic_direct_false", cbm_pipeline_semantic_fp_ends_with("pkg/api.py", ".go"),
               false);
    check_bool("semantic_direct_null", cbm_pipeline_semantic_fp_ends_with(NULL, ".go"), false);
#endif
}

extern size_t cbm_rs_store_arch_json_prop_len_v1(const char *json, const char *key, int key_len);
extern size_t cbm_rs_store_arch_json_prop_copy_v1(char *output, size_t output_size,
                                                  const char *json, const char *key, int key_len);

void test_rust_store_arch_json_prop_export(void) {
    const char *json = "{\"method\":\"GET\",\"path\":\"/api\"}";
    const char *method_key = "\"method\"";
    char output[32];
    size_t length = cbm_rs_store_arch_json_prop_len_v1(json, method_key, 8);
    check_int("store_arch_json_prop_len", (int)length, 3);
    check_int("store_arch_json_prop_copy", (int)cbm_rs_store_arch_json_prop_copy_v1(
                                                       output, sizeof(output), json, method_key, 8),
              3);
    check_str("store_arch_json_prop_value", output, "GET");
    check_int("store_arch_json_prop_missing",
              cbm_rs_store_arch_json_prop_len_v1(json, "\"missing\"", 9) == (size_t)-1, 1);
    check_int("store_arch_json_prop_null",
              cbm_rs_store_arch_json_prop_len_v1(NULL, method_key, 8) == (size_t)-1, 1);
}

void test_pipeline_test_node_is_test_export(void) {
    extern int cbm_rs_pipeline_test_node_is_test_v1(const char *properties_json);
    extern bool cbm_pipeline_test_node_is_test(const char *properties_json);

    check_bool("test_node_true", cbm_rs_pipeline_test_node_is_test_v1("{\"is_test\":true}"), true);
    check_bool("test_node_false", cbm_rs_pipeline_test_node_is_test_v1("{\"is_test\":false}"),
               false);
    check_bool("test_node_spacing", cbm_rs_pipeline_test_node_is_test_v1("{\"is_test\": true}"),
               false);
    check_bool("test_node_null", cbm_rs_pipeline_test_node_is_test_v1(NULL), false);
    check_bool("test_node_bridge_true", cbm_pipeline_test_node_is_test("{\"is_test\":true}"), true);
    check_bool("test_node_bridge_false", cbm_pipeline_test_node_is_test("{\"is_test\":false}"),
               false);
    check_bool("test_node_bridge_spacing", cbm_pipeline_test_node_is_test("{\"is_test\": true}"),
               false);
    check_bool("test_node_bridge_null", cbm_pipeline_test_node_is_test(NULL), false);
}

void test_rust_ui_layout_helpers_export(void) {
    extern uint32_t cbm_rs_ui_layout_stellar_color_v1(int degree);
    extern float cbm_rs_ui_layout_size_for_label_v1(const char *label);
    extern uint32_t cbm_rs_ui_layout_fnv1a_v1(const char *value);
    extern float cbm_rs_ui_layout_rand_float_v1(uint32_t *seed);
    extern int cbm_rs_ui_layout_octant_v1(float ox, float oy, float oz, float x, float y,
                                          float z);
    extern void cbm_rs_ui_layout_child_center_v1(float ox, float oy, float oz, float half,
                                                  int child, float *cx, float *cy, float *cz);
    extern int cbm_rs_ui_layout_find_node_index_v1(const void *map, int count, int64_t id);
    extern int cbm_rs_ui_layout_clamp_max_nodes_v1(int requested, int cap);

    check_int("ui_layout_color_low", (int)cbm_rs_ui_layout_stellar_color_v1(1), 0xff6050);
    check_int("ui_layout_color_hub", (int)cbm_rs_ui_layout_stellar_color_v1(51), 0x80a0ff);
    check_double_near("ui_layout_size_project", cbm_rs_ui_layout_size_for_label_v1("Project"),
                      20.0);
    check_double_near("ui_layout_size_function", cbm_rs_ui_layout_size_for_label_v1("Function"),
                      4.0);
    check_double_near("ui_layout_size_null", cbm_rs_ui_layout_size_for_label_v1(NULL), 4.0);
    check_int("ui_layout_fnv1a_abc", (int)cbm_rs_ui_layout_fnv1a_v1("abc"), 0x1a47e90b);
    uint32_t seed = 1;
    float jitter = cbm_rs_ui_layout_rand_float_v1(&seed);
    check_bool("ui_layout_jitter_range", jitter >= -0.5f && jitter < 0.5f, true);
    check_bool("ui_layout_jitter_seed_changed", seed != 1, true);
    check_int("ui_layout_octant", cbm_rs_ui_layout_octant_v1(0.0f, 0.0f, 0.0f, 1.0f, -1.0f,
                                                               1.0f),
              5);
    float cx = 0.0f;
    float cy = 0.0f;
    float cz = 0.0f;
    cbm_rs_ui_layout_child_center_v1(10.0f, 20.0f, 30.0f, 8.0f, 5, &cx, &cy, &cz);
    check_double_near("ui_layout_child_center_x", cx, 14.0);
    check_double_near("ui_layout_child_center_y", cy, 16.0);
    check_double_near("ui_layout_child_center_z", cz, 34.0);
    typedef struct {
        int64_t id;
        int idx;
    } test_node_id_entry_t;
    const test_node_id_entry_t map[] = {{10, 2}, {20, 4}, {30, 6}};
    check_int("ui_layout_node_id_found",
              cbm_rs_ui_layout_find_node_index_v1(map, 3, 20), 4);
    check_int("ui_layout_node_id_missing",
              cbm_rs_ui_layout_find_node_index_v1(map, 3, 25), -1);
    check_int("ui_layout_clamp_default", cbm_rs_ui_layout_clamp_max_nodes_v1(0, 2000), 2000);
    check_int("ui_layout_clamp_requested", cbm_rs_ui_layout_clamp_max_nodes_v1(100, 2000), 100);
}

void test_rust_ui_httpd_helpers_export(void) {
    extern int cbm_rs_ui_httpd_header_name_is_v1(const char *line, size_t name_len,
                                                  const char *name);
    extern int cbm_rs_ui_httpd_copy_header_value_v1(const char *value, size_t value_len, char *out,
                                                     size_t out_size);

    check_bool("ui_httpd_header_origin", cbm_rs_ui_httpd_header_name_is_v1("Origin", 6, "origin"),
               true);
    check_bool("ui_httpd_header_wrong_length",
               cbm_rs_ui_httpd_header_name_is_v1("Origin", 5, "origin"), false);
    char value[16] = {0};
    check_bool("ui_httpd_copy_value",
               cbm_rs_ui_httpd_copy_header_value_v1(" \tvalue\t ", 9, value, sizeof(value)), true);
    check_str("ui_httpd_copy_value_output", value, "value");
    char small[4] = {'X', 'X', 'X', 'X'};
    check_bool("ui_httpd_copy_oversize",
               cbm_rs_ui_httpd_copy_header_value_v1("value", 5, small, sizeof(small)), false);
    check_int("ui_httpd_copy_oversize_zero", small[0], 0);
}

void test_rust_ast_profile_classifiers_export(void) {
    extern uint32_t cbm_rs_ast_profile_kind_flags_v1(const char *kind);
    extern int cbm_rs_ast_profile_halstead_insert_v1(uint32_t *set, const char *key);
    extern int cbm_rs_ast_profile_is_param_name_v1(const char *ident, const char *const *param_names,
                                                    int param_count);
    extern uint32_t cbm_ast_profile_kind_flags(const char *kind);
    extern bool cbm_ast_profile_halstead_insert(uint32_t *set, const char *key);
    extern bool cbm_ast_profile_is_param_name(const char *ident, const char **param_names,
                                              int param_count);

    check_u32("ast_profile_if", cbm_rs_ast_profile_kind_flags_v1("if_statement"), 1u | (1u << 12));
    check_u32("ast_profile_assignment", cbm_rs_ast_profile_kind_flags_v1("assignment_expression"),
              1u << 8 | 1u << 12);
    check_u32("ast_profile_literal", cbm_rs_ast_profile_kind_flags_v1("string_literal"), 1u << 9);
    check_u32("ast_profile_identifier", cbm_rs_ast_profile_kind_flags_v1("identifier"), 1u << 13);
    check_u32("ast_profile_unknown", cbm_rs_ast_profile_kind_flags_v1("punctuation"), 0);
    check_u32("ast_profile_null", cbm_rs_ast_profile_kind_flags_v1(NULL), 0);
    uint32_t set[512] = {0};
    check_bool("ast_profile_halstead_first", cbm_rs_ast_profile_halstead_insert_v1(set, "if"), true);
    check_bool("ast_profile_halstead_duplicate", cbm_rs_ast_profile_halstead_insert_v1(set, "if"),
               false);
    check_bool("ast_profile_halstead_second", cbm_rs_ast_profile_halstead_insert_v1(set, "for"),
               true);
    const char *params[] = {"count", "value"};
    check_bool("ast_profile_param_found",
               cbm_rs_ast_profile_is_param_name_v1("value", params, 2), true);
    check_bool("ast_profile_param_missing",
               cbm_rs_ast_profile_is_param_name_v1("other", params, 2), false);

    check_u32("ast_profile_bridge_boolean_operator",
              cbm_ast_profile_kind_flags("boolean_operator"), (1u << 6) | (1u << 12));
    check_u32("ast_profile_bridge_call", cbm_ast_profile_kind_flags("call_expression"), 1u << 12);
    check_u32("ast_profile_bridge_not_operator", cbm_ast_profile_kind_flags("not_operator"), 0);
    check_u32("ast_profile_bridge_null", cbm_ast_profile_kind_flags(NULL), 0);
    uint32_t bridge_set[512] = {0};
    check_bool("ast_profile_bridge_halstead_first",
               cbm_ast_profile_halstead_insert(bridge_set, "if"), true);
    check_bool("ast_profile_bridge_halstead_duplicate",
               cbm_ast_profile_halstead_insert(bridge_set, "if"), false);
    check_bool("ast_profile_bridge_halstead_empty",
               cbm_ast_profile_halstead_insert(bridge_set, ""), true);
    uint32_t null_key_set[512] = {17};
    check_bool("ast_profile_bridge_halstead_null_set",
               cbm_ast_profile_halstead_insert(NULL, "if"), false);
    check_bool("ast_profile_bridge_halstead_null_key",
               cbm_ast_profile_halstead_insert(null_key_set, NULL), false);
    check_u32("ast_profile_bridge_halstead_null_key_unchanged", null_key_set[0], 17);
    const char *bridge_params[] = {"count", NULL, "value"};
    check_bool("ast_profile_bridge_param_found",
               cbm_ast_profile_is_param_name("value", bridge_params, 3), true);
    check_bool("ast_profile_bridge_param_missing",
               cbm_ast_profile_is_param_name("other", bridge_params, 3), false);
    check_bool("ast_profile_bridge_param_null_ident",
               cbm_ast_profile_is_param_name(NULL, bridge_params, 3), false);
    check_bool("ast_profile_bridge_param_null_names",
               cbm_ast_profile_is_param_name("value", NULL, 3), false);
    check_bool("ast_profile_bridge_param_zero_count",
               cbm_ast_profile_is_param_name("value", bridge_params, 0), false);
    check_bool("ast_profile_bridge_param_negative_count",
               cbm_ast_profile_is_param_name("value", bridge_params, -1), false);
}

void test_rust_pipeline_usages_json_export(void) {
    extern size_t cbm_rs_pipeline_usages_local_name_len_v1(const char *json);
    extern size_t cbm_rs_pipeline_usages_local_name_copy_v1(const char *json, char *out,
                                                             size_t out_size);

    const char *json = "{\"local_name\":\"thing\"}";
    char out[16] = {0};
    check_size("pipeline_usages_local_name_len",
               cbm_rs_pipeline_usages_local_name_len_v1(json), 5);
    check_size("pipeline_usages_local_name_copy",
               cbm_rs_pipeline_usages_local_name_copy_v1(json, out, sizeof(out)), 5);
    check_str("pipeline_usages_local_name_value", out, "thing");
    check_size("pipeline_usages_local_name_missing",
               cbm_rs_pipeline_usages_local_name_len_v1("{}"), SIZE_MAX);
    check_size("pipeline_usages_local_name_null",
               cbm_rs_pipeline_usages_local_name_copy_v1(NULL, out, sizeof(out)), SIZE_MAX);
}

void test_rust_semantic_token_classifiers_export(void) {
    extern int cbm_rs_semantic_is_token_delim_v1(int byte);
    extern int cbm_rs_semantic_is_camel_break_v1(const char *name, int index);
    extern bool cbm_semantic_is_token_delim(unsigned char byte);
    extern bool cbm_semantic_is_camel_break(const char *name, int index);

    check_int("semantic_token_delim_dot", cbm_rs_semantic_is_token_delim_v1('.'), 1);
    check_int("semantic_token_delim_letter", cbm_rs_semantic_is_token_delim_v1('A'), 0);
    check_int("semantic_token_delim_high_byte", cbm_rs_semantic_is_token_delim_v1(0x80), 0);
    check_int("semantic_token_delim_bridge_dot", cbm_semantic_is_token_delim('.'), 1);
    check_int("semantic_token_delim_bridge_letter", cbm_semantic_is_token_delim('A'), 0);
    check_int("semantic_token_delim_bridge_high_byte", cbm_semantic_is_token_delim(0x80), 0);
    check_int("semantic_token_camel_break", cbm_rs_semantic_is_camel_break_v1("fooBar", 3), 1);
    check_int("semantic_token_camel_break_after_upper", cbm_rs_semantic_is_camel_break_v1("FooBar", 3),
              1);
    check_int("semantic_token_negative_index", cbm_rs_semantic_is_camel_break_v1("fooBar", -1), 0);
    check_int("semantic_camel_bridge_foo_bar", cbm_semantic_is_camel_break("fooBar", 3) ? 1 : 0, 1);
    check_int("semantic_camel_bridge_foo2_bar", cbm_semantic_is_camel_break("foo2Bar", 4) ? 1 : 0,
              0);
    check_int("semantic_camel_bridge_symbol", cbm_semantic_is_camel_break("foo$Bar", 4) ? 1 : 0, 0);
    check_int("semantic_camel_bridge_utf8",
              cbm_semantic_is_camel_break("caf\303\251Bar", 5) ? 1 : 0, 0);
    check_int("semantic_camel_bridge_utf8_ascii",
              cbm_semantic_is_camel_break("caf\303\251fooBar", 8) ? 1 : 0, 1);
    check_int("semantic_camel_bridge_negative", cbm_semantic_is_camel_break("fooBar", -1) ? 1 : 0,
              0);
    check_int("semantic_camel_bridge_zero", cbm_semantic_is_camel_break("fooBar", 0) ? 1 : 0, 0);
    check_int("semantic_camel_bridge_null", cbm_semantic_is_camel_break(NULL, 1) ? 1 : 0, 0);
}
#include "cbm.h"
