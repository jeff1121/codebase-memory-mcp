# C/Rust FFI 邊界

本文件定義 Rust 重構期間新增 Rust 程式碼的初始 FFI 規則。預設 build path 仍保留 C implementation；只有明確 opt-in 的 foundation 或 pipeline 切片會透過 C wrapper 呼叫 Rust staticlib。

## 所有權

- C 傳入 Rust 的指標只在該次呼叫期間借用。
- Rust 不可在回傳後保存借用的 C 指標。
- 若 Rust 配置 buffer 給 C 使用，必須提供明確的 Rust-side free function。
- C 配置的 buffer 仍由既有 C owner 釋放，通常是 arena-backed code。

## 字串與 Buffer

- FFI 邊界的 C string 視為 nullable、NUL-terminated byte sequence。
- Rust API 必須明確表示 nullable C input，通常使用 `Option`。
- 模擬 C byte-oriented 行為時，若涉及截斷或 escape，Rust 必須操作 bytes，而不是 Unicode scalar values。
- stdout 保留給 MCP JSON-RPC；Rust 診斷輸出必須走 stderr 或既有 logging path。

## 錯誤

- FFI function 應回傳穩定的 integer status code 或 nullable output pointer，並符合被替換 C 模組的慣例。
- Human-readable error text 必須是選用行為，除非該 migration 明確允許，否則不可改變公開 MCP 或 CLI 輸出。

## Unsafe Code

- Workspace Rust code 預設 deny unsafe。
- `ffi.rs` 是目前唯一允許 expose `no_mangle` 等 unsafe linkage surface 的模組。
- 未來若新增需要 unsafe 的 FFI module，必須隔離在小型 boundary module，並文件化 pointer validity assumptions、ownership transfer、null handling、allocation/free pairing，以及 invalid/empty input 測試覆蓋。

## 目前匯出

- `cbm_rs_dump_verify_is_degraded`：value-only ABI，由 `tests/test_rust_ffi.c` 驗證；回傳 integer `0`/`1`。
- `cbm_rs_intern_create/free/n/count/bytes`：給 `CBM_USE_RUST_STR_INTERN=1` 使用的 pool-owned string interning ABI。pool 由 Rust 配置與釋放，回傳的字串指標只保證在 pool free 前有效。
- `cbm_rs_intern_n` 接受 raw bytes 與 `len`，支援 embedded NUL；若 `pool` 或 `value` 為 null、`len > UINT32_MAX`、或 `len > isize::MAX`，會在讀取 input 前回傳 null。
- `cbm_rs_path_join`、`cbm_rs_path_join_n`、`cbm_rs_path_dir`、`cbm_rs_str_tolower`、`cbm_rs_str_replace_char`、`cbm_rs_str_strip_ext`、`cbm_rs_str_split_part`：寫入 caller-provided buffer，回傳完整輸出長度；若對應 C contract 應回傳 `NULL`，則回傳 `SIZE_MAX`。C wrapper 先查詢長度、用 `CBMArena` 配置，再呼叫 Rust 寫入。
- `cbm_rs_path_ext`、`cbm_rs_path_base`：回傳 borrowed pointer，指向 input C string 內部或 Rust static empty string；不得由 caller free。
- `cbm_rs_str_split_count`：回傳 split token 數；若 input 為 null 則回傳 `SIZE_MAX`。array 與 substring allocation 仍由 C wrapper 使用 `CBMArena` 完成。
- `cbm_rs_str_starts_with`、`cbm_rs_str_ends_with`、`cbm_rs_str_contains`、`cbm_rs_validate_shell_arg`、`cbm_rs_validate_project_name`：接受 nullable NUL-terminated C string，回傳 integer `0`/`1`。
- `cbm_rs_json_escape`：寫入 caller-provided buffer，遵守 `bufsize`，且永遠在可寫入時 NUL-terminate；不配置 Rust-owned output buffer。
- `cbm_rs_normalize_path_sep`：接受 nullable、可寫 NUL-terminated C string，原地將 backslash 轉成 slash，並 canonicalize drive letter；回傳原 input pointer。
- `cbm_rs_safe_getenv`：給 `CBM_USE_RUST_PLATFORM_ENV_DIRS=1` 使用；env 值以位元組處理，寫入呼叫端提供的 buffer，依呼叫端 buffer 大小截斷，並回傳完整輸出長度；缺值或無效參數以 `SIZE_MAX` 表示。
- `cbm_rs_get_home_dir`、`cbm_rs_app_config_dir`、`cbm_rs_app_local_dir`、`cbm_rs_resolve_cache_dir`：給 `CBM_USE_RUST_PLATFORM_ENV_DIRS=1` 使用；寫入呼叫端提供的 buffer，回傳完整輸出長度；若對應 C contract 應回傳 `NULL`，則回傳 `SIZE_MAX`。這些目錄 helpers 保留 C helper 的 255-byte 暫存值截斷與路徑分隔符正規化。Rust 不配置需要 C 釋放的 buffer，C wrapper 保留呼叫端 buffer 與靜態 buffer 所有權。
- `cbm_rs_detect_cgroup_cpus`、`cbm_rs_detect_cgroup_mem`：接受 nullable cgroup root C string，讀取 cgroup v2/v1 quota 檔案，僅回傳 scalar；null 或無有效 limit 時分別回傳 `-1`、`0`。
- `cbm_rs_diag_env_enabled_value`、`cbm_rs_diag_query_snapshot_values`、`cbm_rs_diag_format_path`、`cbm_rs_diag_format_json`、`cbm_rs_diag_format_ndjson`：test-only diagnostics parity ABI。Rust 僅處理 deterministic env/parser、query scalar snapshot 與 caller-provided buffer formatter；writer thread、atomic global stats、system metrics、file write/rename、NDJSON rotation 與 stderr lifecycle 仍保留在 C。這批 API 目前不接產品 opt-in，未來若需要只應以 `CBM_USE_RUST_DIAGNOSTICS_FORMAT=1` 這類窄範圍 flag 包住 formatter。
- `cbm_rs_pipeline_plan_describe`：pipeline pass plan parity ABI。Rust 只回傳 full pipeline、sequential、parallel extraction、predump、parallel threshold、incremental extract/resolve 與 incremental post/tail 的 plan 字串；不接受或共用 `CbmRsRegistryHandle`，不保存 C 指標，不執行任何 tree-sitter/extraction/LSP/store code。
  - `kind=0` sequential pass table；字串 ABI 仍包含 `infra_routes:after_success` 與 `infra_bindings:after_success` tail。typed v2 metadata 目前只描述前段 6 個 sequential dispatch pass，tail side effects 仍由 C 端既有條件控制。
  - `kind=1` predump pass table；此字串 ABI 保持相容，typed v2 metadata 另由 `cbm_rs_pipeline_plan_steps_v2` 提供。
  - `kind=2` extraction choice，依 `worker_count > 1 && file_count > 50` 回傳 `parallel` 或 `sequential`。
  - `kind=3` incremental extract/resolve plan；typed v2 metadata 另由 `cbm_rs_pipeline_plan_steps_v2` 提供，C 端驗證 typed steps 後逐步 dispatch，實際 extract/resolve 仍呼叫 C 實作。
  - `kind=4` incremental post/tail typed plan；產品 opt-in 透過 typed step FFI dispatch k8s/postpass prefix 與 `edge_relink -> incremental_dump -> persist_hashes -> artifact_export` tail，C 端固定驗證順序與 policy。
  - `kind=5` parallel extraction pass order。
  - `kind=6` full pipeline outer orchestration。
  - `CBM_USE_RUST_PIPELINE_PLAN=1` 使用 `kind=2` 選擇 full/incremental extraction phase 的 parallel 或 sequential path，使用 typed v2 metadata 驅動 sequential extraction dispatch、incremental extract/resolve dispatch 與 full pre-dump pass dispatch/trace，使用 typed v2 metadata 驗證並記錄 parallel extraction 外層順序，並使用 typed incremental post steps 驅動完整 incremental post/tail dispatch；graph-buffer mutation command contract 由 test-only FFI 固定，C-side adapter 已可同步 dispatch structure、incremental purge/file-upsert/inbound restore、`TESTS`/`TESTS_FILE`、usage 與 configlink 等代表性呼叫點，實際 pass 執行、parallel cache/cross-LSP cleanup、dump、hash persist、FTS rebuild、artifact export、infra tail、graph-buffer 寫入、storage 與 ownership 仍由 C 負責。
- `cbm_rs_pipeline_plan_step_count_v2` / `cbm_rs_pipeline_plan_steps_v2`：additive typed plan metadata ABI。目前支援 `kind=0` sequential dispatch、`kind=1` predump、`kind=3` incremental extract/resolve 與 `kind=5` parallel extraction；其他 valid kind 回傳 `-1`，保留未來擴充空間。每個 step 回傳 stable `kind`、`phase`、`policy`、`gate_flags`、`requires_mask` 與 `effect_flags`；C adapter 必須逐欄比對本地 dispatch table，任何缺漏、重複、錯序、未知 bit 或 mode gate 不符都不可採用 typed path。
  - sequential kind：`16=definitions`、`17=k8s`、`18=lsp_cross`、`19=calls`、`20=usages`、`21=semantic`。這批 metadata 只覆蓋 sequential extraction dispatch loop，不包含 `infra_routes` / `infra_bindings` after-success tail。
  - predump kind：`1=decorator_tags`、`2=configlink`、`3=route_match`、`4=similarity`、`5=semantic_edges`、`6=complexity`。
  - incremental extract/resolve kind：`48=definitions`、`49=calls`、`50=usages`、`51=semantic` 用於 sequential incremental path；`52=incr_extract`、`53=incr_registry`、`54=incr_resolve` 用於 parallel incremental path。這批 metadata 會被 C adapter 驗證後用於 dispatch；C 仍負責 extraction cache、shared id、registry build、resolve、cross-LSP prebuild skip 與所有副作用。
  - parallel extraction kind：`32=parallel_extract`、`33=registry_build`、`34=lsp_cross_prepare`、`35=parallel_resolve`、`36=infra_routes`、`37=infra_bindings`、`38=k8s`。這批 metadata 只固定 parallel path 的外層順序與 trace；C 仍負責 result cache、shared id、cross-LSP resources、infra 執行時機、cleanup 與 error propagation。
  - `phase=1` 表示 predump；`phase=2` 表示 sequential extraction dispatch；`phase=3` 表示 parallel extraction outer dispatch；`phase=4` 表示 incremental extract/resolve outer dispatch。`policy=0` 是 required，`policy=1` 是 ignore_err，`policy=4` 是 env_optional；sequential 的 `k8s` 與 `lsp_cross` 使用 ignore_err，incremental extract/resolve 全部使用 ignore_err，parallel 的 `lsp_cross_prepare` 使用 env_optional，其餘 dispatch pass 使用 required。
  - `gate_flags=1` 表示只在 `CBM_MODE_FAST` 跳過；目前只適用於 predump `similarity` 與 `semantic_edges`。`gate_flags=2` 表示 pass 依賴 `ctx->result_cache`；目前只適用於 sequential `lsp_cross`。`gate_flags=4` 表示 incremental parallel resolve 刻意不做 cross-file LSP prebuild。非 FAST（含 advanced mode）仍回傳 6 個 predump steps。
  - `requires_mask` 使用 stable kind 的 bitset 表達已驗證的前置 pass；C 端以本地順序精確比對，不用 Rust metadata 推導可否執行。
  - `effect_flags=1` 表示該 pass mutates graph；未知 effect 必須視為 typed metadata 不可信。parallel 的 `parallel_extract`、`registry_build`、`parallel_resolve`、`infra_routes`、`infra_bindings` 與 `k8s` 皆標示此 effect，`lsp_cross_prepare` 保持 0。
- `cbm_rs_pipeline_incremental_post_step_count` / `cbm_rs_pipeline_incremental_post_steps`：給 `CBM_USE_RUST_PIPELINE_PLAN=1` 使用的 typed incremental post plan ABI。Rust 回傳 stable numeric `kind` / `policy` pairs，C 端必須驗證順序與 policy：post-pass prefix 為 `ignore_err`，`edge_relink` / `incremental_dump` / `persist_hashes` 為 `best_effort`，`artifact_export` 為 `optional_existing_artifact`。驗證通過後，C adapter 會依 typed steps 呼叫既有 C handlers；Rust 不保存 C 指標，也不搬移 dump、hash persist、FTS rebuild 或 artifact export 的 side effects。Artifact policy 仍描述既有 artifact 自動更新分支；C handler 另外會依 `pipeline.persistence` 對 explicit export 使用 full-path 錯誤語意。若 `incremental_dump` 失敗，C handler 會傳回實際 rc，並跳過後續 persist/artifact tail；這屬 C side-effect contract，不由 Rust ABI 決定。
- `cbm_rs_gbuf_mutation_command_count_v1` / `cbm_rs_gbuf_mutation_commands_v1` / `cbm_rs_gbuf_mutation_validate_v1`：test-only graph-buffer mutation command boundary ABI。Rust 回傳 stable command metadata，並只驗證 command shape；目前固定 `1=UpsertNode`、`2=InsertEdge`、`3=DeleteByFile`，以及 required/optional field bitset、result kind、effect flags、reserved bits、empty string 與 null C string contract。此 ABI 的 command struct/constants 由 `src/graph_buffer/graph_buffer.h` 與 FFI smoke 共用；C string 只在呼叫期間借用，底層 graph-buffer 會複製需要保存的值。數值欄位（例如 source/target id 與行號）是固定 ABI scalar slot，Rust validation 不推斷 presence、不做 endpoint lookup，也不解析 JSON；effect flags 表示可能副作用，`InsertEdge` dedup 時可能回傳既有 edge id。此 ABI 不接產品 opt-in，不接受 `cbm_gbuf_t*`，也不執行任何 graph-buffer mutation。C-side `cbm_gbuf_apply_mutation` / `cbm_gbuf_apply_*` adapter 只是同步 normalization/dispatch fixture：它可讓既有 C 呼叫走 command apply 邊界，但仍呼叫 C `cbm_gbuf_*` API，不代表 Rust-backed graph-buffer；除非另有明確 production opt-in flag、C fallback、default/opt-in matrix 與 language graph parity gate，文件不得稱為 graph-buffer migration。
- `cbm_rs_registry_resolve_once`：一次性 registry parity fixture；每次呼叫從 C entry 陣列建立暫時 registry，只供 `tests/test_rust_ffi.c` smoke 使用，不接產品 hot path。
- `cbm_rs_registry_create/free/add/resolve`：給 `CBM_USE_RUST_PIPELINE_REGISTRY=1` 使用的 opaque registry handle ABI。handle 由 Rust 配置與釋放，`add` 複製 QN bytes；`resolve` 只在呼叫期間借用 callee/import 指標，將 QN 與 strategy 寫入 caller-provided buffers。C wrapper 會用回傳 QN 查回 C registry-owned key，維持 `cbm_resolution_t.qualified_name` 的 borrowed-from-registry contract。
- 同一 pool 不保證 thread-safe；這維持目前 C interner 的使用模型。
