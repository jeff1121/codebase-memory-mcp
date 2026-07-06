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
#include "cypher/cypher.h"
#include "foundation/arena.h"
#include "graph_buffer/graph_buffer.h"
#include "pipeline/pipeline.h"
#include "pipeline/rust_plan.h"
#ifndef _WIN32
#include <sys/stat.h>
#include <unistd.h>
#endif

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

extern int cbm_rs_dump_verify_is_degraded(int committed_nodes, int persisted_nodes, double ratio,
                                          int min_floor);

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
extern int cbm_rs_cypher_lex_token_count_v1(const unsigned char *input, int len);
extern int cbm_rs_cypher_lex_tokens_v1(const unsigned char *input, int len, CbmRsCypherTokenV1 *out,
                                       int cap);
extern size_t cbm_rs_cypher_lex_summary_v1(char *buf, size_t bufsize, const unsigned char *input,
                                           int len);
extern size_t cbm_rs_cypher_parse_summary_v1(char *buf, size_t bufsize, const unsigned char *input,
                                             int len);
extern size_t cbm_rs_mcp_jsonrpc_request_summary_v1(char *buf, size_t bufsize,
                                                    const unsigned char *input, int len);
extern int cbm_rs_mcp_jsonrpc_parse_v1(const unsigned char *input, int len,
                                       CbmRsMcpJsonRpcParseOutV1 *out, char *jsonrpc_buf,
                                       size_t jsonrpc_bufsize, char *method_buf,
                                       size_t method_bufsize, char *id_str_buf,
                                       size_t id_str_bufsize, char *params_buf,
                                       size_t params_bufsize);
extern int cbm_rs_mcp_tools_cursor_offset_v1(const char *params_json, int tool_count);
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
extern size_t cbm_rs_store_like_hint_v1(char *buf, size_t bufsize, const char *pattern,
                                        int max_out, int index);
extern size_t cbm_rs_store_qn_to_package_v1(char *buf, size_t bufsize, const char *qn);
extern size_t cbm_rs_store_qn_to_top_package_v1(char *buf, size_t bufsize, const char *qn);
extern size_t cbm_rs_store_normalize_arch_path_v1(char *norm_out, size_t norm_sz, const char *path);
extern int cbm_rs_store_is_test_file_path_v1(const char *path);
extern int cbm_rs_store_hop_to_risk_v1(int hop);
extern size_t cbm_rs_store_risk_label_v1(char *buf, size_t bufsize, int level);
extern size_t cbm_rs_store_camel_split_v1(char *buf, size_t bufsize, const char *input);
extern int64_t cbm_rs_store_resolve_mmap_size_value_v1(const char *value);
extern int cbm_rs_store_connection_manifest_entry_count_v1(void);
extern int cbm_rs_store_connection_manifest_write_pragma_count_v1(void);
extern int cbm_rs_store_connection_manifest_entries_v1(CbmRsStoreConnectionManifestEntryV1 *out,
                                                       int cap);

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
    check_size("store_glob_question",
               cbm_rs_store_glob_to_like_v1(buf, sizeof(buf), "f???.txt"),
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

    check_size("store_case_strip_null", cbm_rs_store_strip_case_flag_v1(buf, sizeof(buf), NULL),
               0);
    check_str("store_case_strip_null_value", buf, "");
    check_size("store_case_strip_prefixed",
               cbm_rs_store_strip_case_flag_v1(buf, sizeof(buf), "(?i)handler"),
               strlen("handler"));
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
    check_int("store_like_escaped_pipe_count", cbm_rs_store_like_hint_count_v1("foo\\|bar", 16),
              0);
    check_int("store_like_handler_count", cbm_rs_store_like_hint_count_v1(".*handler.*", 16), 1);
    check_size("store_like_handler_len",
               cbm_rs_store_like_hint_v1(NULL, 0, ".*handler.*", 16, 0), strlen("handler"));
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

    check_int("store_like_escaped_dot_count", cbm_rs_store_like_hint_count_v1("foo\\.bar", 16),
              1);
    check_size("store_like_escaped_dot_len",
               cbm_rs_store_like_hint_v1(buf, sizeof(buf), "foo\\.bar", 16, 0),
               strlen("foo.bar"));
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
    check_size("store_arch_path_null",
               cbm_rs_store_normalize_arch_path_v1(buf, sizeof(buf), NULL), SIZE_MAX);
    check_size("store_arch_path_empty",
               cbm_rs_store_normalize_arch_path_v1(buf, sizeof(buf), ""), SIZE_MAX);
    check_size("store_arch_path_ws_only",
               cbm_rs_store_normalize_arch_path_v1(buf, sizeof(buf), "   "), SIZE_MAX);
    check_size("store_arch_path_dotslash_only",
               cbm_rs_store_normalize_arch_path_v1(buf, sizeof(buf), "./"), SIZE_MAX);
    check_size("store_arch_path_slashes_only",
               cbm_rs_store_normalize_arch_path_v1(buf, sizeof(buf), "///"), SIZE_MAX);

    /* norm_sz==0 / NULL out → SIZE_MAX */
    check_size("store_arch_path_zero_sz",
               cbm_rs_store_normalize_arch_path_v1(buf, 0, "apps/foo"), SIZE_MAX);
    check_size("store_arch_path_null_out",
               cbm_rs_store_normalize_arch_path_v1(NULL, sizeof(buf), "apps/foo"), SIZE_MAX);

    /* 截斷先於 strip/collapse（對齊 C strncpy）*/
    char small[5];
    memset(small, 'Z', sizeof(small));
    check_size("store_arch_path_trunc",
               cbm_rs_store_normalize_arch_path_v1(small, sizeof(small), "abcdef"),
               strlen("abcd"));
    check_str("store_arch_path_trunc_val", small, "abcd");
    memset(small, 'Z', sizeof(small));
    check_size("store_arch_path_trunc_slash",
               cbm_rs_store_normalize_arch_path_v1(small, sizeof(small), "abc/def"),
               strlen("abc"));
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
    check_size("store_qn_pkg_no_dot",
               cbm_rs_store_qn_to_package_v1(buf, sizeof(buf), "standalone"), 0);
    check_str("store_qn_pkg_no_dot_value", buf, "");
    check_size("store_qn_pkg_two",
               cbm_rs_store_qn_to_package_v1(buf, sizeof(buf), "project.cmd"),
               strlen("cmd"));
    check_str("store_qn_pkg_two_value", buf, "cmd");
    check_size("store_qn_pkg_three",
               cbm_rs_store_qn_to_package_v1(buf, sizeof(buf), "project.main.foo"),
               strlen("main"));
    check_str("store_qn_pkg_three_value", buf, "main");
    check_size("store_qn_pkg_many",
               cbm_rs_store_qn_to_package_v1(buf, sizeof(buf),
                                             "project.internal.store.search.Search"),
               strlen("store"));
    check_str("store_qn_pkg_many_value", buf, "store");
    check_size("store_qn_pkg_len_only",
               cbm_rs_store_qn_to_package_v1(NULL, 0, "project.internal.store.search.Search"), strlen("store"));
    check_size("store_qn_pkg_buf_zero",
               cbm_rs_store_qn_to_package_v1(marker, 0, "project.internal.store.search.Search"), strlen("store"));
    check_str("store_qn_pkg_buf_zero_value", marker, "ABC");
    check_size("store_qn_pkg_consecutive",
               cbm_rs_store_qn_to_package_v1(buf, sizeof(buf), "project.dir..Func"),
               strlen("dir"));
    check_str("store_qn_pkg_consecutive_value", buf, "dir");
    memset(small, 'Z', sizeof(small));
    check_size("store_qn_pkg_short",
               cbm_rs_store_qn_to_package_v1(small, sizeof(small),
                                             "project.internal.store.search.Search"),
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
    check_size("store_qn_pkg_256_fallback_len",
               cbm_rs_store_qn_to_package_v1(buf, sizeof(buf), qn), strlen("dir"));
    check_str("store_qn_pkg_256_fallback_value", buf, "dir");

    check_size("store_qn_top_null", cbm_rs_store_qn_to_top_package_v1(buf, sizeof(buf), NULL),
               0);
    check_str("store_qn_top_null_value", buf, "");
    check_size("store_qn_top_no_dot",
               cbm_rs_store_qn_to_top_package_v1(buf, sizeof(buf), "standalone"), 0);
    check_str("store_qn_top_no_dot_value", buf, "");
    check_size("store_qn_top_two",
               cbm_rs_store_qn_to_top_package_v1(buf, sizeof(buf), "project.cmd"),
               strlen("cmd"));
    check_str("store_qn_top_two_value", buf, "cmd");
    check_size("store_qn_top_many",
               cbm_rs_store_qn_to_top_package_v1(buf, sizeof(buf),
                                                 "project.internal.store.search.Search"),
               strlen("internal"));
    check_str("store_qn_top_many_value", buf, "internal");
    check_size("store_qn_top_len_only",
               cbm_rs_store_qn_to_top_package_v1(NULL, 0, "project.internal.store.search.Search"), strlen("internal"));
    check_size("store_qn_top_short", cbm_rs_store_qn_to_top_package_v1(top_short, sizeof(top_short),
                                                                   "project.internal.store.search.Search"),
               strlen("internal"));
    check_str("store_qn_top_short_value", top_short, "int");
    snprintf(qn, sizeof(qn), "project.%s.Func", ok_segment);
    check_size("store_qn_top_255_len", cbm_rs_store_qn_to_top_package_v1(buf, sizeof(buf), qn),
               255);
    snprintf(qn, sizeof(qn), "project.%s.Func", too_long_segment);
    check_size("store_qn_top_256_len", cbm_rs_store_qn_to_top_package_v1(buf, sizeof(buf), qn),
               0);
    check_str("store_qn_top_256_value", buf, "");

    check_int("store_test_file_null", cbm_rs_store_is_test_file_path_v1(NULL), 0);
    check_int("store_test_file_empty", cbm_rs_store_is_test_file_path_v1(""), 0);
    check_int("store_test_file_prefix", cbm_rs_store_is_test_file_path_v1("test_handler.py"), 1);
    check_int("store_test_file_dir",
              cbm_rs_store_is_test_file_path_v1("src/__tests__/handler.js"), 1);
    check_int("store_test_file_testdata",
              cbm_rs_store_is_test_file_path_v1("testdata/fixture.json"), 1);
    check_int("store_test_file_substring", cbm_rs_store_is_test_file_path_v1("contest/file.c"),
              1);
    check_int("store_test_file_upper", cbm_rs_store_is_test_file_path_v1("TestOnly.java"), 0);
    check_int("store_test_file_spec", cbm_rs_store_is_test_file_path_v1("handler.spec.ts"), 0);

    check_int("store_hop_risk_1", cbm_rs_store_hop_to_risk_v1(1), 0);
    check_int("store_hop_risk_2", cbm_rs_store_hop_to_risk_v1(2), 1);
    check_int("store_hop_risk_3", cbm_rs_store_hop_to_risk_v1(3), 2);
    check_int("store_hop_risk_0", cbm_rs_store_hop_to_risk_v1(0), 3);
    check_int("store_hop_risk_negative", cbm_rs_store_hop_to_risk_v1(-1), 3);
    check_size("store_risk_label_critical",
               cbm_rs_store_risk_label_v1(buf, sizeof(buf), 0), strlen("CRITICAL"));
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

    CbmRsStoreConnectionManifestEntryV1 conn_entries[16];
    CbmRsStoreConnectionManifestEntryV1 short_conn_entries[15];
    check_int("store_connection_entry_count", cbm_rs_store_connection_manifest_entry_count_v1(),
              16);
    check_int("store_connection_write_pragma_count",
              cbm_rs_store_connection_manifest_write_pragma_count_v1(), 4);
    check_int("store_connection_entries_len",
              cbm_rs_store_connection_manifest_entries_v1(conn_entries, 16), 16);
    check_int("store_connection_entries_null",
              cbm_rs_store_connection_manifest_entries_v1(NULL, 16), -1);
    check_int("store_connection_entries_short",
              cbm_rs_store_connection_manifest_entries_v1(short_conn_entries, 15), -1);
    check_int("store_connection_entries_negative",
              cbm_rs_store_connection_manifest_entries_v1(conn_entries, -1), -1);

    const uint32_t conn_required = CBM_RS_STORE_CONN_FLAG_REQUIRED;
    check_store_connection_entry(
        conn_entries, 16, CBM_RS_STORE_CONN_KIND_OPEN_MODE, "open.path_query",
        conn_required | CBM_RS_STORE_CONN_FLAG_READ_ONLY | CBM_RS_STORE_CONN_FLAG_NO_CREATE,
        CBM_RS_STORE_CONN_MODE_READ_ONLY, 0, 0, "SQLITE_OPEN_READONLY", "");
    check_store_connection_entry(
        conn_entries, 16, CBM_RS_STORE_CONN_KIND_READ_PROBE, "open.path_query.probe",
        conn_required | CBM_RS_STORE_CONN_FLAG_READ_ONLY, CBM_RS_STORE_CONN_MODE_READ_ONLY, 1, 0,
        "SELECT 1 FROM sqlite_master LIMIT 1;", "");
    check_store_connection_entry(conn_entries, 16, CBM_RS_STORE_CONN_KIND_FALLBACK,
                                 "open.path_query.immutable_fallback",
                                 conn_required | CBM_RS_STORE_CONN_FLAG_READ_ONLY |
                                     CBM_RS_STORE_CONN_FLAG_NO_CREATE | CBM_RS_STORE_CONN_FLAG_URI,
                                 CBM_RS_STORE_CONN_MODE_READ_ONLY, 2, 0,
                                 "SQLITE_OPEN_READONLY|SQLITE_OPEN_URI;immutable=1", "");
    check_store_connection_entry(
        conn_entries, 16, CBM_RS_STORE_CONN_KIND_PRAGMA, "pragma.journal_mode.wal",
        conn_required | CBM_RS_STORE_CONN_FLAG_WRITES_DB_OR_WAL, CBM_RS_STORE_CONN_MODE_WRITE, 40,
        0, "PRAGMA journal_mode = WAL;", "");
    check_store_connection_entry(
        conn_entries, 16, CBM_RS_STORE_CONN_KIND_PRAGMA, "pragma.wal_checkpoint.passive",
        conn_required | CBM_RS_STORE_CONN_FLAG_WRITES_DB_OR_WAL |
            CBM_RS_STORE_CONN_FLAG_BEST_EFFORT,
        CBM_RS_STORE_CONN_MODE_WRITE, 50, 0, "PRAGMA wal_checkpoint(PASSIVE)", "");
    check_store_connection_entry(
        conn_entries, 16, CBM_RS_STORE_CONN_KIND_PRAGMA, "pragma.mmap_size",
        conn_required | CBM_RS_STORE_CONN_FLAG_ENV_BACKED,
        CBM_RS_STORE_CONN_MODE_WRITE | CBM_RS_STORE_CONN_MODE_READ_ONLY, 70, 67108864,
        "PRAGMA mmap_size = <resolved>;", "CBM_SQLITE_MMAP_SIZE");
    check_store_connection_entry(conn_entries, 16, CBM_RS_STORE_CONN_KIND_PRAGMA,
                                 "pragma.bulk.cache_size.begin", conn_required,
                                 CBM_RS_STORE_CONN_MODE_BULK_BEGIN, 80, -65536,
                                 "PRAGMA cache_size = -65536;", "");
    check_store_connection_entry(conn_entries, 16, CBM_RS_STORE_CONN_KIND_PRAGMA,
                                 "pragma.bulk.cache_size.end", conn_required,
                                 CBM_RS_STORE_CONN_MODE_BULK_END, 90, -2000,
                                 "PRAGMA cache_size = -2000;", "");
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
    check_int("mcp_cursor_null_val", cbm_rs_mcp_tools_cursor_offset_v1("{\"cursor\":null}", tc), tc);
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
    check_int("mcp_cursor_clamp", cbm_rs_mcp_tools_cursor_offset_v1("{\"cursor\":\"100\"}", tc), tc);
    check_int("mcp_cursor_dup_first",
              cbm_rs_mcp_tools_cursor_offset_v1("{\"cursor\":\"2\",\"cursor\":\"9\"}", tc), 2);
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

int main(void) {
    test_mem_exports();
    test_dump_verify_exports();
    test_arena_exports();
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
    test_compat_exports();
    test_log_profile_exports();
    test_hash_table_exports();
    test_yaml_exports();
    test_cypher_lexer_exports();
    test_cypher_parse_summary_export();
    test_mcp_jsonrpc_summary_export();
    test_mcp_jsonrpc_parse_export();
    test_mcp_tools_cursor_offset_exports();
    test_pipeline_project_name_exports();
    test_pipeline_plan_exports();
    test_gbuf_mutation_metadata_exports();
    test_store_camel_split_exports();
    test_store_immutable_uri_exports();
    test_store_search_pattern_exports();
    test_store_arch_helper_exports();
    test_store_arch_path_scope_exports();
    test_store_mmap_resolver_exports();
    test_store_schema_manifest_exports();
    test_registry_import_map_and_bare();
    test_registry_qualified_suffix();
    test_registry_suffix_candidate_selection();
    test_registry_candidate_cap();
    test_registry_import_reachability_penalty();
    test_registry_handle_lifecycle_and_persistent_resolve();
    test_registry_handle_import_map_suffix_and_truncation();
    return 0;
}
