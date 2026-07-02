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
- `cbm_rs_pipeline_plan_describe`：pipeline pass plan parity ABI。Rust 只回傳 full pipeline、sequential、parallel extraction、predump、parallel threshold、incremental extract/resolve 與 incremental dump/persist tail 的 plan 字串；不接受或共用 `CbmRsRegistryHandle`，不保存 C 指標，不執行任何 tree-sitter/extraction/LSP/store code。
  - `kind=0` sequential pass table。
  - `kind=1` predump pass table。
  - `kind=2` extraction choice，依 `worker_count > 1 && file_count > 50` 回傳 `parallel` 或 `sequential`。
  - `kind=3` incremental extract/resolve plan。
  - `kind=4` incremental k8s/postpass/edge-relink/dump/persist/artifact tail。
  - `kind=5` full parallel extraction pass order。
  - `kind=6` full pipeline outer orchestration。
  - `CBM_USE_RUST_PIPELINE_PLAN=1` 使用 `kind=2` 選擇 full/incremental extraction phase 的 parallel 或 sequential path，並使用 `kind=1` 驅動 full pre-dump pass dispatch；實際 pass 執行與資料 mutation 仍由 C 負責。
- `cbm_rs_registry_resolve_once`：一次性 registry parity fixture；每次呼叫從 C entry 陣列建立暫時 registry，只供 `tests/test_rust_ffi.c` smoke 使用，不接產品 hot path。
- `cbm_rs_registry_create/free/add/resolve`：給 `CBM_USE_RUST_PIPELINE_REGISTRY=1` 使用的 opaque registry handle ABI。handle 由 Rust 配置與釋放，`add` 複製 QN bytes；`resolve` 只在呼叫期間借用 callee/import 指標，將 QN 與 strategy 寫入 caller-provided buffers。C wrapper 會用回傳 QN 查回 C registry-owned key，維持 `cbm_resolution_t.qualified_name` 的 borrowed-from-registry contract。
- 同一 pool 不保證 thread-safe；這維持目前 C interner 的使用模型。
