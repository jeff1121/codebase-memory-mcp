> **歷史快照警示（2026-07-16）**：本檔是早期計畫快照，不是目前交接來源。請讀
> [rust-refactor-current-handoff.md](rust-refactor-current-handoff.md) 與根目錄的
> [Rust-Refactor.md](../Rust-Refactor.md)；近期 source selector、SvelteKit file-kind 與 Discover trailing
> separator 的狀態以權威快照為準。

## 2026-07-13：Pipeline plan direct-only 切片

- 新增 `CBM_USE_RUST_PIPELINE_PLAN_ONLY=1`，讓 `rust_plan.h`、`pipeline.c` 與 `pipeline_incremental.c` 在 direct 變體使用既有 Rust `top_v1`、typed `v2` 與 incremental typed plan。
- `PLAN_ONLY` 下停用 predump、sequential、parallel、full-pipeline top-level、incremental extract/resolve 與 incremental post 的 C planner fallback；Rust plan 不可用時回報錯誤，不靜默改走 C planner。
- 實際 pass 執行與 graph side effects 仍由 C pass 函式承擔，這不是整個 pipeline orchestration 的 Rust replacement。
- 新增 `rust-pipeline-plan-only-test`，覆蓋 default/direct FFI smoke、pipeline suite、production link 與版本啟動；並將 direct flag 納入 `rust-all-optin-test`。
- focused 結果：default pipeline `221 passed`、`PLAN_ONLY` pipeline `221 passed`、FFI smoke 與 production link 成功。
- 全矩陣結果：wrapper `5869 passed`；wrapper/direct production link 與 `--version` 均成功。


## 2026-07-13：Arena direct source exclusion

- 新增 `CBM_USE_RUST_ARENA_ONLY=1`；direct foundation source list 排除 `src/foundation/arena.c`，改由既有 Rust `cbm_rs_arena_*` ABI 實作配置、成長、reset、destroy、字串複製與統計。
- 新增 `src/foundation/arena_sprintf.c`，只保留公開 `cbm_arena_*` C ABI forwarding 與 variadic `cbm_arena_sprintf` compatibility shim；stable Rust 不直接實作 C variadic function。
- 新增 `rust-arena-only-test`，並納入全 opt-in direct matrix。
- focused 結果：default/direct foundation 各 `325 passed`、Rust FFI smoke、default/direct production link 與版本啟動成功；全 opt-in wrapper/direct matrix 成功。

## 2026-07-13：graph buffer mutation direct-only

`graph_buffer_mutation.c` 的三個 mutation dispatch ABI 已加入 Rust direct-only slice。Rust 只負責 command shape 的 `reserved0`/kind 分派，實際 graph buffer ownership、節點與邊的配置、dedup、cascade delete 與 SQLite dump 仍由 `graph_buffer.c` 保留。`CBM_USE_RUST_GRAPH_BUFFER_MUTATION_ONLY=1` 時，Makefile 不再編譯 `src/graph_buffer/graph_buffer_mutation.c`，改由 `cbm-core` staticlib 匯出相同 ABI；default/direct graph buffer suite 各 52 項、Rust FFI smoke 與 production link 均通過。

## 2026-07-13：pipeline FQN direct-only

`src/pipeline/fqn.c` 的完整純函式 API 已由 Rust 承接：FQN compute/module/module-dir/folder 與 Python/JavaScript relative import resolver。Rust 以 byte-level path algorithm 保留 separator、extension、`__init__`/`index`、dot segment 與 1024-byte C path buffer 行為；FFI 回傳字串使用 C `malloc`，既有 C 呼叫端仍以 `free()` 負責釋放。`CBM_USE_RUST_PIPELINE_FQN_ONLY=1` 時不再編譯 `fqn.c`；default/direct FQN suite 各 77 項、Rust unit 236 項、FFI 與 production link 均通過。

## 2026-07-13：pipeline registry direct-only

- 新增 `pipeline-registry-only` feature 與 `CBM_USE_RUST_PIPELINE_REGISTRY_ONLY=1`。
- Rust handle 直接承擔 registry 的 QN 去重、label、simple-name index、resolve、fuzzy resolve、ending lookup、import reachability、Perl builtin guard 與 confidence band API。
- direct-only 建置排除 `src/pipeline/registry.c`，維持既有 opaque `cbm_registry_t*` 的 C ABI；QN、label 與 name index 回傳的字串由 Rust handle 持有至 registry 釋放。
- `cbm_registry_find_ending_with` 的指標陣列以 C `malloc` 配置，符合既有呼叫端以 `free` 釋放陣列、但不釋放字串的契約。
- 三組 per-file cache lifecycle symbol 目前以 Rust ABI 相容 no-op 提供；resolve 結果仍保證指向 handle 內穩定字串，但尚未恢復 C 版的 thread-local 效能快取。
- `rust-pipeline-registry-only-test` 涵蓋 default/direct registry suite、Rust FFI smoke、production link 與版本 gate。

## `system_info.c` direct-only 遷移（2026-07-13）

- 新增 `rust/cbm-core/src/ffi_system_info_only.rs`，以 C ABI 保留 `cbm_system_info`、`cbm_default_worker_count`、`cbm_detect_cgroup_cpus` 與 `cbm_detect_cgroup_mem`。
- 覆蓋 macOS、BSD、Linux、Windows 的核心數、效能核心數與記憶體偵測，以及 Linux cgroup v1/v2 CPU 與記憶體限制。
- `CBM_WORKERS` 的有效範圍、fallback 與 invalid warning 行為已與 C 版一致；Rust 透過 `log.c` 的固定 arity bridge 發出 warning。
- `CBM_USE_RUST_SYSTEM_INFO_ONLY=1` 會從 production/test source list 移除 `src/foundation/system_info.c`，並啟用 `system-info-only` Cargo feature。
- 驗證結果：Rust `236 passed`、clippy `-D warnings` 通過；C/Rust 對照各 `36 passed`；全 opt-in wrapper/direct 各 `5869 passed`；兩套 production link 與版本輸出均通過。
- 整體 C→Rust 遷移尚未完成；graph buffer 核心、SQLite store 與 pipeline orchestration 仍需後續切片與 direct replacement gate。

## Pipeline worker pool direct-only slice（2026-07-13）

`src/pipeline/worker_pool.c` 的 `cbm_parallel_for` 已由 `rust/cbm-core/src/ffi_pipeline_worker_pool_only.rs` 直接實作。Rust 版本保留 serial fallback、`max_workers` 與 `count` 邊界、主執行緒參與工作分派、8 MiB worker stack、thread 建立失敗時的 serial fallback，以及 C ABI callback/context 傳遞；`force_pthreads` 仍保留為相容欄位。

`CBM_USE_RUST_PIPELINE_WORKER_POOL_ONLY=1` 會從 pipeline C source list 移除 `worker_pool.c`，啟用 `pipeline-worker-pool-only` feature，並把 `log.c`、`platform.c`、`system_info.c` 作為 direct FFI 支援來源。worker pool direct gate 已加入 all-optin direct matrix。

驗證結果：default/direct worker pool 對照 target 各 `40 passed`，Rust 全 feature 測試 `236 passed`，clippy `-D warnings` 通過，FFI smoke、production link、`--version` 與全 opt-in wrapper/direct matrix 均成功。全專案仍非 C 到 Rust 的完整遷移；尚有未切出的 C 模組與 fallback 路徑。

## Slab allocator direct-only slice（2026-07-13）

`src/foundation/slab_alloc.c` 的 thread-local 64-byte slab allocator已由 `rust/cbm-core/src/ffi_slab_alloc_only.rs` 直接實作。Rust 版本保留 64 KiB page、1024 個 64-byte chunks、free-list reuse、`malloc` heap fallback、calloc overflow/zeroing、slab-to-heap realloc promotion、thread-local reset/destroy/reclaim，以及 tree-sitter `ts_set_allocator` callback ABI。頁面配置仍透過 C `malloc/free`，因此正式版的 mimalloc override 行為不變。

`CBM_USE_RUST_SLAB_ALLOC_ONLY=1` 會從 foundation C source list 移除 `slab_alloc.c`，啟用 `slab-alloc-only` feature；direct FFI smoke 另外連結 vendored Tree-sitter `alloc.c` 以提供 `ts_set_allocator` symbol。此 gate 已納入 all-optin direct matrix，C fallback 仍保留。

驗證結果：Rust 全 feature 測試 `238 passed`，clippy `-D warnings` 通過；default/direct `mem` suite 各 `31 passed`，包含 tree-sitter parallel extraction；FFI smoke、production link、`--version` 與全 opt-in wrapper/direct matrix 均成功。全專案仍非 C 到 Rust 的完整遷移。

## Profiling runtime direct-only slice（2026-07-13）

`src/foundation/profile.c` 已由 `rust/cbm-core/src/ffi_profile_only.rs` 直接替換。Rust slice 保留 `cbm_profile_active` 的 C ABI、`CBM_PROFILE` 環境變數閘門、單調時鐘取樣、elapsed/rate 計算，以及透過固定參數 bridge 輸出的 profiling log。`CBM_USE_RUST_PROFILE_ONLY=1` 會移除 C source、啟用 `profile-only` feature，並納入 all-optin direct matrix；與 worker pool 同時啟用時，`log.c` FFI support source 只加入一次。

驗證：Rust workspace `cargo test -p cbm-core --all-features --locked` 共 240 項通過；`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings` 通過；profile default/direct 專用 gate 各 2 項通過，包含 FFI smoke 與 production link/version；修正 log bridge 去重後，all-optin wrapper/direct 完整測試與 combined release link 均成功。

本 slice 不代表專案已完成 C 到 Rust 的完整遷移；其餘 C 模組仍保留 fallback，後續切片仍須逐一完成相同的測試、FFI、連結與文件 gate。

## AST profile codec direct-only slice（2026-07-13）

`src/semantic/ast_profile.c` 的 AST/Tree-sitter traversal 仍保留 C fallback；其中純資料 codec 元件已由 `rust/cbm-core/src/ffi_ast_profile_codec_only.rs` 直接替換：`cbm_ast_profile_to_str`、`cbm_ast_profile_from_str` 與 `cbm_ast_profile_to_vector`。Rust 以 `repr(C)` 保留 25 個 `uint16_t` 欄位的 ABI、原有 comma-separated field order、截斷/NUL 行為與 normalization 上限；`CBM_USE_RUST_AST_PROFILE_CODEC_ONLY=1` 只排除 C codec 定義，不排除 traversal。

驗證：Rust all-features `cargo test` 共 244 項通過，clippy `-D warnings` 通過；codec 專用 default/direct runner 各 5868 項通過，direct FFI smoke 與 production link/version 通過；新增 flag 納入 all-optin wrapper/direct 完整測試及 combined release link，整體成功。

本 slice 是 semantic AST profile 的元件級替換，不代表該模組或專案已完成 C 到 Rust 的完整遷移。

## Semantic dense-vector direct-only slice（2026-07-13）

`src/semantic/semantic.c` 的三個純 dense-vector primitive 已由 `rust/cbm-core/src/ffi_semantic_vector_only.rs` 直接替換：`cbm_sem_cosine`、`cbm_sem_normalize` 與 `cbm_sem_vec_add_scaled`。Rust 保留 `cbm_sem_vec_t` 的 `repr(C)` 及 768 維 `float` layout、零向量 epsilon policy、逐元素累加與 C ABI；pretrained lookup、random indexing、corpus、scoring orchestration 與 graph diffusion 仍由 C fallback 提供。

驗證：Rust all-features `cargo test` 共 247 項通過，clippy `-D warnings` 通過；semantic-vector default/direct runner 各 5868 項通過，FFI smoke 與 production link/version 通過；新增 flag 納入 all-optin wrapper/direct 完整測試與 combined release link，整體成功。

本 slice 僅替換 semantic dense-vector primitives，不代表整個 semantic corpus 或專案已完成 C 到 Rust 遷移。

## Semantic proximity direct-only slice（2026-07-13）

`src/semantic/semantic.c` 的 exported `cbm_sem_proximity` 純 path scoring helper 已由 `rust/cbm-core/src/ffi_semantic_proximity_only.rs` 直接替換。Rust 以原始 bytes 比對 NUL 結尾 C strings，保留共同 prefix slash count、最大目錄深度、無目錄時回傳 `1.0`、共同目錄 boost 與 NULL policy；semantic vectors、corpus、scoring orchestration 與 graph processing 仍由 C fallback 提供。

驗證：Rust all-features `cargo test` 共 250 項通過，clippy `-D warnings` 通過；proximity default/direct runner 各 5868 項通過，FFI smoke 與 production link/version 通過；新增 flag 納入 all-optin wrapper/direct 完整測試與 combined release link，整體成功。

本 slice 僅替換 module-proximity helper，不代表整個 semantic module 或專案已完成 C 到 Rust 遷移。

## Semantic tokenizer direct-only slice（2026-07-13）

`src/semantic/semantic.c` 的 `cbm_sem_tokenize` 已由 `rust/cbm-core/src/ffi_semantic_tokenize_only.rs` 直接替換。Rust 保留 ASCII delimiter/camel-case 分割、128-byte buffer、lowercase、完整 abbreviation expansion table 與 output-capacity 行為；每個 token 使用 C `malloc` 配置並由既有 C caller 以 `free` 回收，避免跨 allocator ownership 問題。

驗證：Rust all-features `cargo test` 共 253 項通過，clippy `-D warnings` 通過；tokenize default/direct runner 各 5868 項通過，FFI smoke 與 production link/version 通過；新增 flag 納入 all-optin wrapper/direct 完整測試與 combined release link，整體成功。

本 slice 僅替換 semantic token extraction，不代表 semantic corpus 或專案已完成 C 到 Rust 遷移。

## Semantic config/env direct-only（2026-07-13）

新增 `rust/cbm-core/src/ffi_semantic_config_only.rs`，以 `repr(C)` 保留 `cbm_sem_config_t` 的九個 `float` 欄位與 `max_edges` ABI，直接替換 `cbm_sem_get_config()` 和 `cbm_sem_is_enabled()`。Rust 實作保留 C 的預設權重、`CBM_SEMANTIC_THRESHOLD` 的 `strtod` 解析與範圍檢查，以及 `CBM_SEMANTIC_ENABLED` 首字元為 `1` 的啟用規則。

`CBM_USE_RUST_SEMANTIC_CONFIG_ONLY` 已接入 `Makefile.cbm`、Rust feature、FFI smoke、production link 與 `rust-all-optin-test`；C fallback 仍保留。Focused default/direct C runner 通過 `5868 passed`；Rust all-features 通過 `255 passed`，clippy `-D warnings` 通過，wrapper/direct all-optin 與 production link/version 也通過。

這仍是 semantic config/env 的 direct-only 切片，不代表 `src/semantic/semantic.c` 已完成 Rust 全量遷移。

## Discover language direct-only（2026-07-14）

新增 `rust/cbm-core/src/ffi_language_only.rs`，完整替換 `src/discover/language.c` 的四個 exported API：`cbm_language_for_extension()`、`cbm_language_for_filename()`、`cbm_language_name()` 與 `cbm_disambiguate_m()`。內建 159 個 `CBMLanguage` enum 值的副檔名、特殊檔名與名稱表由既有 C contract 轉成 Rust 靜態表；`.env.*`、`.blade.php`、userconfig compound extension 與 `.m` 的 Objective-C/Magma/MATLAB 判斷均保留。

`CBM_USE_RUST_DISCOVER_LANGUAGE_ONLY` 會從 production `DISCOVER_SRCS` 移除 `language.c`，並加入 `userconfig_bridge.c`；FFI smoke 使用無狀態 stub，正式 binary 仍連接既有 `userconfig.c`。驗證：Rust all-features `257 passed`、clippy `-D warnings` 通過；default/direct C runner 各 `5868 passed`；all-optin wrapper/direct、combined Cargo features、production link 與 version check 通過。

這仍是 discover language 的 direct-only 切片；`discover.c`、`gitignore.c`、`userconfig.c` 與其他 C pipeline/store/semantic 功能尚未全量 Rust 化。

## 2026-07-14：discover 語言判定與 gitignore 直接替換

本輪完成兩個可獨立切換的 Rust slice：

- `discover-language-only`：以 `rust/cbm-core/src/ffi_language_only.rs` 取代 `src/discover/language.c` 的語言列舉、副檔名與檔名判定、語言名稱，以及 `.m` 檔案消歧。
- `discover-gitignore-only`：以 `rust/cbm-core/src/ffi_gitignore_only.rs` 取代 `src/discover/gitignore.c` 的 glob、規則解析、載入、比對、釋放與合併。

語言 slice 保留現有 `userconfig` 行為，透過 `src/discover/userconfig_bridge.c` 接回 C 設定查詢；FFI smoke 使用獨立的 `userconfig_bridge_stub.c`，避免將測試相依性混入正式連結。gitignore 合併仍保留測試用複製 hook 與配置失敗時的原子性。

驗證結果：

- `cargo test -p cbm-core --all-features --locked`：259 項通過。
- `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`：通過。
- `make -f Makefile.cbm rust-discover-gitignore-only-test`：default/direct 全套 C 測試各 5868 項通過，direct 正式執行檔可啟動並回報 `codebase-memory-mcp dev`。
- `make -f Makefile.cbm rust-all-optin-test`：wrapper/direct 全選用 Rust gate 通過，direct 正式執行檔可啟動並回報 `codebase-memory-mcp dev`。

`language.c`、`gitignore.c` 的 C 實作仍保留為未啟用 Rust flag 時的 fallback；`discover.c`、`userconfig.c` 與其他尚未完成切片的 C 模組仍未宣稱替換完成。

## 2026-07-14：discover userconfig 直接替換

本輪完成 `discover-userconfig-only`：`rust/cbm-core/src/ffi_userconfig_only.rs` 取代 `src/discover/userconfig.c` 的 process-global 設定指標、global/project JSON 設定載入、`extra_extensions` 解析、語言名稱解析、project-over-global 去重、lookup 與釋放 ABI。

Rust parser 保留現有行為：缺少檔案、空檔案、超過 65536 bytes、invalid JSON、非 object root、非 object `extra_extensions` 與非字串值皆 fail-open；extension 必須以 `.` 開頭，語言名稱沿用既有 `cbm_language_name` ABI，並保留 C API 的 `CBM_LANG_COUNT` sentinel。配置中的字串由 Rust allocation 管理，僅能透過 `cbm_userconfig_free` 釋放，避免跨 allocator 直接 `free`。

FFI smoke 的 `userconfig_app_config_ffi_stub.c` 與 `userconfig_ffi_stub.c` 只在獨立 smoke 且缺少對應 C 依賴時加入；正式 production 仍使用 `platform.c` 的 `cbm_app_config_dir`，language direct 時則使用 Rust `cbm_language_name`。兩個 stub 不會進入 production link。

驗證結果：

- `cargo test -p cbm-core --features discover-userconfig-only --locked`：238 項通過。
- `cargo clippy -p cbm-core --features discover-userconfig-only --all-targets --locked -- -D warnings`：通過。
- `cargo test -p cbm-core --all-features --locked`：262 項通過。
- `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`：通過。
- `make -f Makefile.cbm rust-discover-userconfig-only-test`：default/direct 完整 C runner 各 5868 項通過，direct FFI smoke 與 production `--version` 通過。
- `make -f Makefile.cbm rust-all-optin-test`：wrapper/direct 全選用 Rust gate 通過，direct production `--version` 回報 `codebase-memory-mcp dev`。

`userconfig.c` 仍保留為未啟用 flag 時的 C fallback；JSON 設定與 pipeline runtime 的其餘 orchestration 尚未宣稱完成 Rust 替換。

## 2026-07-15：discover filters direct-only

補上 `CBM_USE_RUST_DISCOVER_FILTERS` 的 direct-only gate，直接替換 `src/discover/discover.c` 中四個 filter helper：
`cbm_should_skip_dir`、`cbm_has_ignored_suffix`、`cbm_should_skip_filename`、`cbm_matches_fast_pattern`。

`tests/test_rust_ffi.c` 加入 `test_discover_filter_exports()`，覆核：
- `MODE_FULL` 與 `MODE_MODERATE`/`MODE_FAST` 的 skip 邏輯邊界；
- full/full-like 模式下 `has_ignored_suffix` 與 `matches_fast_pattern` 對應輸出；
- `discover-filters-only` 目標是否能正常完成 default + direct smoke、`codebase-memory-mcp --version`。

`Makefile.cbm` 新增 `rust-discover-filters-only-test`，包含 default + direct `test-rust-ffi`、`test-runner`，以及 `CBM_USE_RUST_DISCOVER_FILTERS=1` 的 direct `cbm` 驗證流程，並已納入 `rust-ci-tests` 全量測試門。

目前此 gate 僅針對 discover filter helper；`discover.c` 其他路徑仍保留 C fallback，Rust gate 僅在啟用 flag 時接管對應 helper ABI。

## Foundation mem 直替換切片

- `src/foundation/mem.c` 已由 `CBM_USE_RUST_MEM_ONLY` guard 保留為 fallback；正式 Rust 路徑改由 `rust/cbm-core/src/ffi_mem_only.rs` 輸出既有 `cbm_mem_*` ABI。
- `src/foundation/mem_rust_bridge.c` 僅承接 mimalloc、OS RSS/peak、回收與既有 log 行為；`tests/test_rust_ffi.c` 的 smoke 使用獨立 stub，不依賴 production bridge。
- 保留 `CBM_MEM_BUDGET_MB` 優先順序、strict positive 解析、worker budget、pressure hysteresis 與無限初始化語意。
- 驗證證據：`cargo test -p cbm-core --features mem-only --locked` 通過 237 項；mem-only clippy 以 `-D warnings` 通過；`rust-mem-only-test` 的預設與 Rust 直連全測試各通過 5868 項，FFI smoke、production link 與 `--version` 均通過。
- 本切片只替換記憶體管理 ABI；整個 repository 尚未完成 C 全面移除，後續仍須依序處理可安全直替換的 C 模組。

### Foundation diagnostics Rust 替換（`CBM_USE_RUST_DIAGNOSTICS_ONLY`）

- `rust/cbm-core/src/ffi_diagnostics_only.rs` 現在完整承接 `src/foundation/diagnostics.c` 的查詢統計、snapshot/NDJSON 格式化、週期 writer、atomic rename、trajectory rotation 與 start/stop 生命週期。
- 僅保留 `src/foundation/diagnostics_rust_bridge.c` 作為 mimalloc `mi_process_info` 的薄 C bridge；`diagnostics_ffi_stub.c` 只供 direct FFI smoke 使用。
- `CbmQueryStats`、`g_query_stats` 與所有 `cbm_diag_*` 匯出維持既有 C ABI；與既有 `diagnostics-format-only` feature 同時啟用時，formatter ABI 由既有 Rust formatter 模組提供，避免重複符號。
- 驗證：`cargo test -p cbm-core --all-features --locked` 通過 266 tests；all-feature strict clippy 通過；`rust-all-optin-test` 的 wrapper/direct C runner 各通過 5869 tests，direct FFI smoke 實際建立並保留 trajectory，兩條 production link 與 `--version` 均成功。
- 目前仍不是整個儲存庫的 C-to-Rust 完成宣告；Tree-sitter traversal、store、MCP/server 與 pipeline runtime 等未列入本切片的 C production 邏輯仍保留。

## 2026-07-15：pipeline path_alias parser 缺陷修正

本輪補上 `pipeline path_alias` 的 Unicode JSON escape 邊界。`cbm_load_path_aliases` 讀取 `compilerOptions` 字串時，`\\uXXXX` 解碼邏輯會在高位 surrogate 前導時誤移動 `pos`，造成高位/低位配對錯位；已修正 `self.pos` 位移為 `+5`，並在 surrogate 成功時再前進 `+6`。

同時補上 Rust unit test：`parser_accepts_unicode_escape_in_alias_prefix`，使用 `"\\u0040/*"` 路徑別名 key 驗證 parser 可正確還原 `@` 並完成 resolve。

本 slice 僅為 parser 邊界修正，未改變既有 path alias C/Rust 對照或門禁矩陣；下一步請繼續盤點並切換下一個 production-only C ABI。

### Pipeline test node predicate

新增 `CBM_USE_RUST_PIPELINE_TEST_NODE_IS_TEST` opt-in gate，將 `"is_test":true` properties 判斷移至 Rust；C 僅保留節點生命週期與 TESTS edge 寫入。

### UI layout pure helpers

新增 `CBM_USE_RUST_UI_LAYOUT_HELPERS` opt-in gate，移植 `layout3d.c` 的 degree 顏色與 label 尺寸映射；layout simulation 與資料生命週期不變。

同一 gate 亦涵蓋 layout 的 FNV-1a seed 與 deterministic jitter；不改變 layout simulation 的 C ownership。

同一 UI layout gate 亦移植 octree 的 octant 與 child-center 純計算，不移植 octree 的配置或遞迴生命週期。

UI layout gate 亦移植 node-id map 的二分搜尋，透過 `#[repr(C)]` 保持 C map ABI，不接管 map 生命週期。

UI layout max-node clamp 已移植為 Rust predicate，環境變數與 UI 設定檔 I/O 仍由 C 處理。

### UI HTTP parser helpers

新增 `CBM_USE_RUST_UI_HTTPD_HELPERS` opt-in gate，移植 header name 與 bounded value copy；不移植 socket、連線生命週期或 route handler。

### AST profile classifiers

新增 `CBM_USE_RUST_AST_PROFILE_CLASSIFIERS` opt-in gate，以單一 bitmask ABI 移植 AST node-kind predicates；不移植 Tree-sitter traversal。

同一 AST profile gate 亦移植固定容量 Halstead hash insert，不移植 set storage 或 traversal。

AST profile classifier gate 亦移植 parameter-name membership，保留 C source 與參數陣列生命週期。

### Pipeline usages JSON helper

新增 `CBM_USE_RUST_PIPELINE_USAGES_JSON` opt-in gate，移植 IMPORTS edge `local_name` bounded extraction；graph lookup 與 edge mutation 維持 C。

### Semantic token boundary helpers

新增 `CBM_USE_RUST_SEMANTIC_TOKEN_CLASSIFIERS` opt-in gate，移植 delimiter 與 camelCase boundary predicate；token allocation 維持 C。
