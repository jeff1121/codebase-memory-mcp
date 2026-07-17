> **歷史快照警示（2026-07-16）**：本檔停留於早期工作階段，不能單獨作為目前交接或完成證據。
> 請優先讀 [rust-refactor-current-handoff.md](rust-refactor-current-handoff.md)，再讀根目錄的
> [Handoff.md](../Handoff.md)、[Rust-Refactor.md](../Rust-Refactor.md)、[Tasks.md](../Tasks.md) 與
> [rust/CBM_FFI.md](../rust/CBM_FFI.md)。不要依本檔的
> 歷史 passed 數、flag 數或「已完成」敘述判定目前 dirty worktree 狀態。

## 2026-07-13：Pipeline plan direct-only handoff

本輪完成 `CBM_USE_RUST_PIPELINE_PLAN_ONLY=1`。Rust `top_v1`、typed `v2` 與 incremental typed plan 現在可作為 direct build 的唯一 plan decision source；predump、sequential、parallel、full pipeline top-level、incremental extraction/post 的 C planner fallback 在 direct build 不再使用。C 仍執行實際 pass 與 graph side effects，因此目前狀態是 Rust plan decision layer + C execution layer，不是全 pipeline Rust replacement。

驗證：default 與 direct pipeline 各 `221 passed`；direct FFI smoke、production link、`--version` 成功；全 opt-in wrapper `5869 passed`，wrapper/direct production link 與版本啟動成功。下一階段應優先把 pass execution side effects 移入 Rust ABI，再處理 pipeline registry 完整 resolver/cache/index 與 store/Cypher/MCP 等大型邊界。


## 2026-07-13：Arena direct handoff

`CBM_USE_RUST_ARENA_ONLY=1` 已可在 production direct build 排除 `src/foundation/arena.c`。Rust FFI 提供 arena layout-compatible 的核心生命週期、配置、字串 helper 與統計；`src/foundation/arena_sprintf.c` 僅轉接公開 C symbol 並處理 C variadic formatting。default、focused direct 與全 opt-in wrapper/direct matrix 均成功。後續仍需處理其他 foundation/store/pipeline 邊界，不能將此切片視為整個 C runtime 已移除。

## 2026-07-13：graph buffer mutation slice

本輪已完成 `graph_buffer_mutation.c` 的 Rust direct-only 取代。Rust 對外匯出 `cbm_gbuf_apply_mutation`、`cbm_gbuf_apply_upsert_node`、`cbm_gbuf_apply_insert_edge` 與 `cbm_gbuf_apply_delete_by_file`，並以既有 C graph buffer API 執行真正的資料操作；因此沒有改變指標借用、錯誤回傳或 ownership。驗證結果為 default/direct graph buffer 各 52 passed、FFI smoke passed、direct production link passed。下一個 graph migration 仍必須處理 `graph_buffer.c` 的記憶體模型、merge/dedup、vector storage 與 SQLite dump，不能把本 slice 視為完整 graph buffer Rust 化。

## 2026-07-13：pipeline FQN slice

`pipeline-fqn-only` 已成為可直接使用的 Rust slice。Rust 接管 `fqn.c` 全部公開 path/FQN 函式，並以 C `malloc` 配置結果，避免跨 allocator 釋放錯誤；`project_name.c` 不在本 slice 內，仍由其既有 Rust direct-only flag 控制。驗證為 default/direct FQN 各 77 passed、Rust all-features 236 passed、clippy passed、direct link 排除 `src/pipeline/fqn.c`。後續若調整 resolver，必須保留 C header 的 null、separator、extension、relative import 與 buffer limit 契約。

## 2026-07-13：pipeline registry direct-only 交接

本次加入 `pipeline-registry-only`。啟用 `CBM_USE_RUST_PIPELINE_REGISTRY_ONLY=1` 時，Makefile 不再編譯 `src/pipeline/registry.c`，Rust export 直接提供 `cbm_registry_*` 公開符號。Rust handle 內的 QN 與 label 均使用穩定 heap allocation，避免 C 呼叫端保存 borrowed pointer 時失效；`find_ending_with` 回傳的指標陣列由 C `malloc` 配置。

現況限制：三組 cache lifecycle 函式保留 ABI 但為 no-op，因此功能契約相同，resolve 的穩定指標契約相同，但尚未達到 C 版 per-file thread-local cache 的效能。後續應先補 Rust cache 與 benchmark，再考慮把 direct-only 旗標提升為預設路徑。

## 2026-07-15：pipeline module-directory direct 一致化

已完成 `CBM_USE_RUST_PIPELINE_MODULE_DIR_ONLY=1` 在 `helpers.c` 的共享化。`CBM_LANG_JAVA` 與 `CBM_LANG_GO` 的 module 判定現在由 `cbm_lang_module_is_dir()` 統一掌控，`pass_calls.c`、`pass_parallel.c`、`pass_semantic.c`、`pass_usages.c`、`pass_lsp_cross.c` 全部改為共用。

證據：`rust/CBM_FFI.md` 已更新為跨 pass 一致性接入；`rust-pipeline-module-dir-only-test` 與 all-optin direct matrix 已納入 `make -f Makefile.cbm rust-ci`。仍保留 pass side-effect 與 graph mutation 於 C，未主張整體 pipeline 已完全 Rust 化。

下一步：繼續盤點 `src/pipeline` 的其餘 production ABI 邊界（非 pure helper），以相同 direct-only gate 方式逐步拆解。

## 2026-07-13 handoff：`system_info.c` 已 direct-only

本輪已將 `src/foundation/system_info.c` 替換為 `rust/cbm-core/src/ffi_system_info_only.rs`。預設 build 仍使用 C；只有設定 `CBM_USE_RUST_SYSTEM_INFO_ONLY=1` 時才移除 C source 並連接 Rust staticlib。Rust 保留既有 C layout 與 symbol ABI，並透過 `src/foundation/log.c` 的固定 arity bridge 保持 worker 環境值 warning 行為。

已驗證：`make -f Makefile.cbm rust-system-info-only-test` 成功，C/Rust 對照各 36 項通過；`cargo test -p cbm-core --all-features --locked` 為 236 項通過；`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings` 成功；最終 `rust-all-optin-test` wrapper/direct 各 5869 項通過，兩套 production binary 皆輸出 `codebase-memory-mcp dev`。

下一位執行者不得將此切片宣稱為全庫 Rust 化。優先評估 graph buffer 核心或 pipeline worker pool，並維持每個切片的 C/Rust 對照、direct-only gate、FFI smoke、production link 與全 opt-in 矩陣。

## Worker pool direct slice handoff（2026-07-13）

本輪新增 `CBM_USE_RUST_PIPELINE_WORKER_POOL_ONLY=1`。direct build 不再編譯 `src/pipeline/worker_pool.c`，改連結 `rust/cbm-core` 的 `pipeline-worker-pool-only` feature；未設定旗標時仍保留 C fallback。Rust FFI 依賴的 C 支援來源是 logging、platform 與 system-info（若同時啟用 system-info direct，則由 Rust 提供 system-info symbols）。

已驗證 default/direct worker pool 與 parallel 聚焦測試各 `40 passed`，Rust 全 feature `236 passed`、clippy 無警告、FFI smoke、production binary 與 all-optin wrapper/direct matrix 均成功。後續接手者應維持 direct-only gate、C fallback 與 all-optin 矩陣，不應宣稱整個 C runtime 已完成 Rust 化。

## Slab allocator direct slice handoff（2026-07-13）

本輪新增 `CBM_USE_RUST_SLAB_ALLOC_ONLY=1`。direct build 不再編譯 `src/foundation/slab_alloc.c`，改連結 `rust/cbm-core` 的 `slab-alloc-only` feature；未設定旗標時仍使用原始 C slab allocator。Rust 實作透過 C `malloc/free/realloc` 維持正式版 allocator override，並由 vendored Tree-sitter `alloc.c` 提供 `ts_set_allocator`。

已驗證 Rust 全 feature `238 passed`、clippy 無警告、default/direct `mem` 各 `31 passed`、FFI smoke、production binary 與 all-optin wrapper/direct matrix 均成功。後續接手者應保留 C fallback、direct gate 與 Tree-sitter allocator ABI；不要把本 slice 解讀為整個 runtime 已完成 Rust 化。

## Profiling runtime direct-only 交接（2026-07-13）

本輪將 `src/foundation/profile.c` 替換為 `rust/cbm-core/src/ffi_profile_only.rs`。保留的 ABI 包含 `cbm_profile_active`、`cbm_profile_init`、`cbm_profile_enable`、`cbm_profile_now` 與 `cbm_profile_log_elapsed`；Rust 透過 `cbm_log_profile_elapsed` 固定參數 bridge 呼叫既有 logging 實作。Makefile gate 為 `CBM_USE_RUST_PROFILE_ONLY=1`，Cargo feature 為 `profile-only`。

已完成驗證：Rust 240 項測試與 clippy；profile default/direct 各 2 項；FFI smoke；production link/version；以及包含 system-info、worker-pool、slab allocator、profile 的 all-optin wrapper/direct 完整測試與 combined release link。worker-pool 與 profile 同時啟用時，`log.c` 僅由一個 FFI support gate 提供。

注意：這是 direct-only slice，不是完整 Rust 化。未替換模組仍必須保留 C fallback，後續工作應先讀取目標模組與測試，再建立最小 Rust ABI slice 及其 direct gate。

## AST profile codec direct-only 交接（2026-07-13）

本輪新增 `rust/cbm-core/src/ffi_ast_profile_codec_only.rs`，直接提供 `cbm_ast_profile_to_str`、`cbm_ast_profile_from_str`、`cbm_ast_profile_to_vector`。`CbmAstProfile` 使用 `repr(C)` 與 25 個 `u16` 欄位對齊 `src/semantic/ast_profile.h`；parser 保留十進位欄位、逗號分隔、短 buffer 截斷與 NUL 終止契約。`src/semantic/ast_profile.c` 以 `CBM_USE_RUST_AST_PROFILE_CODEC_ONLY` 排除三個 C codec 定義，但 traversal、Tree-sitter node walk 與 profile 計算仍在 C。

已完成 Rust 244 tests、clippy、default/direct 各 5868 項 C runner、direct FFI smoke、production link/version，以及包含新 flag 的 all-optin wrapper/direct 與 combined release link。下一步若要替換 traversal，必須先固定 `TSNode` layout、`ts_node_*` call boundary 與跨 grammar 行為，不應直接以 opaque struct 猜測 ABI。

## Semantic dense-vector direct-only 交接（2026-07-13）

本輪新增 `rust/cbm-core/src/ffi_semantic_vector_only.rs`，直接提供 `cbm_sem_cosine`、`cbm_sem_normalize`、`cbm_sem_vec_add_scaled`。`CbmSemVec` 使用 `repr(C)` 的 `[f32; 768]` 對齊 `cbm_sem_vec_t`；Rust 保留 NULL safety、`1e-10F` denominator epsilon、normalize 零向量保留與逐維 scaled-add 行為。C source 只在 `CBM_USE_RUST_SEMANTIC_VECTOR_ONLY` 未啟用時提供三個 definitions，其餘 semantic C state 與 map 仍會編譯。

已完成 Rust 247 tests、clippy、default/direct 各 5868 項 C runner、direct FFI smoke、production link/version，以及包含新 flag 的 all-optin wrapper/direct 與 combined release link。曾發現並修正 preprocessor guard 覆蓋 C pretrained-map declaration 的問題；目前 guard 僅包住三個 vector function。

下一步若要替換 random-index 或 corpus，須先固定 pretrained asset、hash ABI、shared state initialization 與 parallel worker 行為，不可把本 slice 推廣成整個 semantic replacement。

## Semantic proximity direct-only 交接（2026-07-13）

本輪新增 `rust/cbm-core/src/ffi_semantic_proximity_only.rs`，直接提供 `cbm_sem_proximity`。Rust 不要求 UTF-8，使用 `CStr::to_bytes()` 保留 C path byte semantics；共同 prefix 的 `/` 數量除以兩路徑 slash count 的最大值，再乘以 `0.10` boost，無目錄或 NULL 時回傳 `1.0`。`src/semantic/semantic.c` 只在 `CBM_USE_RUST_SEMANTIC_PROXIMITY_ONLY` 未啟用時提供 C definition。

已完成 Rust 250 tests、clippy、default/direct 各 5868 項 C runner、direct FFI smoke、production link/version，以及包含新 flag 的 all-optin wrapper/direct 與 combined release link。下一步若要搬移 combined score，須先固定 sparse TF-IDF、MinHash、profile vector 與 clamp 的跨語言浮點契約。

## Semantic tokenizer direct-only 交接（2026-07-13）

本輪新增 `rust/cbm-core/src/ffi_semantic_tokenize_only.rs`，直接提供 `cbm_sem_tokenize`。Rust 依 C `TOKEN_BUF_LEN=128`、delimiter set、lowercase ASCII alnum、lowercase-to-uppercase camel break 與原始 token count snapshot 實作；abbreviation expansion table 保持既有內容與輸出順序。輸出字串透過 C `malloc` 配置，呼叫端可安全以既有 `free` 回收。

已完成 Rust 253 tests、clippy、default/direct 各 5868 項 C runner、direct FFI smoke、production link/version，以及包含新 flag 的 all-optin wrapper/direct 與 combined release link。下一步若替換 semantic corpus，須先處理 token pointer ownership、pretrained map、parallel workers 與 C/Rust allocator 邊界。

## 本回合交接：semantic config/env direct-only（2026-07-13）

本回合完成 `cbm_sem_get_config()` 與 `cbm_sem_is_enabled()` 的 Rust direct-only replacement。實作位於 `rust/cbm-core/src/ffi_semantic_config_only.rs`，C 端只在 `CBM_USE_RUST_SEMANTIC_CONFIG_ONLY` 下停用兩個對應函式；semantic 其餘 C 實作與 fallback 未移除。

驗證結果：focused default/direct C suite 通過 `5868 passed`；`cargo test -p cbm-core --all-features --locked` 通過 `255 passed`；clippy `-D warnings` 通過；`rust-all-optin-test` 的 wrapper/direct FFI、full runner、production link 與 `--version` 均成功。

下一步應繼續從 `src/semantic/semantic.c` 選擇具有明確 ABI 邊界的純函式或資料結構切片，並維持 direct-only gate、C fallback、focused C runner、all-features 與 all-optin 證據鏈。

## 本回合交接：discover/language direct-only（2026-07-14）

完成 `rust/cbm-core/src/ffi_language_only.rs`，以 `repr`/C ABI 相容的 `c_int` 回傳語言 enum，覆蓋副檔名、特殊檔名、語言名稱與 `.m` 內容判斷。Rust 靜態表保留 C 原始表的 159 個 enum mapping；userconfig 仍由 C 維護，透過正式 bridge 傳入 Rust。

FFI smoke 的 `userconfig_bridge_stub.c` 只在沒有 C userconfig object 的 smoke link 使用，正式 `cbm` link 使用 `userconfig_bridge.c` 加既有 `userconfig.c`，避免以測試 stub 取代 production 行為。

驗證：default/direct 完整 C suite 各 `5868 passed`；Rust all-features `257 passed`；clippy `-D warnings`；all-optin wrapper/direct full runner、combined feature build、production binary 與 `--version` 均通過。

下一步應從 `discover/gitignore`、`discover/userconfig` 或其他尚未有 direct gate 的低耦合模組選擇下一個完整邊界，並維持 C fallback 與同等證據鏈。

## 2026-07-14 交接：discover language/gitignore

### 已完成

`rust/cbm-core/src/ffi_language_only.rs` 已直接實作語言判定 ABI，覆蓋 C 的 159 個語言值、extension/filename lookup、名稱查詢與 `.m` 消歧；`userconfig` 查詢透過正式 C bridge 保持既有設定來源。

`rust/cbm-core/src/ffi_gitignore_only.rs` 已直接實作 gitignore pattern parser/matcher、檔案載入、釋放與 merge，包含 rooted pattern、`**`、字元類別、否定規則、directory-only 規則，以及 merge allocation failure 的原子性。

### Gate 與證據

- Cargo all-features：259 tests passed。
- Clippy all-targets/all-features：`-D warnings` 通過。
- gitignore dedicated default/direct gate：完整 C runner 各 5868 tests passed；direct binary version 啟動成功。
- all-optin wrapper/direct gate：通過；direct binary version 啟動成功。

### 後續注意

兩個 C 檔案仍可在未設定對應 `CBM_USE_RUST_DISCOVER_*_ONLY` 時作為 fallback。正式環境的 language bridge 與 FFI smoke bridge 必須維持分離；不可用 weak import 或將測試 stub 帶入 production。下一個切片仍須先盤點 ABI、加入 focused test，再建立 default/direct 與 all-optin gate。整個 repository 尚未完成 C 到 Rust 的全面替換。

## 2026-07-14 交接：discover userconfig

### 已完成

`rust/cbm-core/src/ffi_userconfig_only.rs` 已提供 `cbm_userconfig_load`、`cbm_userconfig_lookup`、`cbm_userconfig_free`、`cbm_set_user_lang_config` 與 `cbm_get_user_lang_config` 的 C ABI。`CbmUserconfig` 與 `CbmUserext` 使用 `repr(C)`；Rust 配置的 `CString` 與 entry vector 只由 Rust free path 回收，C 呼叫端不得自行釋放欄位。

設定讀取保留原有 global `$XDG_CONFIG_HOME/codebase-memory-mcp/config.json`／平台 config dir 與 project `.codebase-memory.json` 路徑，project 同名 extension 覆蓋 global。JSON parser 對 invalid input、缺檔與非預期 root fail-open；語言名稱查詢透過 `cbm_language_name`，避免複製 enum mapping。

### Gate 與證據

- userconfig feature：Rust 238 tests passed，strict Clippy passed。
- all-features：262 tests passed，strict Clippy passed。
- dedicated default/direct：完整 C runner 各 5868 tests passed，direct FFI smoke 與 production version passed。
- all-optin wrapper/direct：passed；direct production version 為 `codebase-memory-mcp dev`。

### 後續注意

`userconfig_ffi_stub.c` 與 `userconfig_app_config_ffi_stub.c` 是獨立 FFI smoke 支援，Makefile 已避免 language Rust 同時連 language-name stub；不可將兩個 stub 加入 production。下一個切片仍須先確認與 userconfig 的 ownership／bridge 邊界，再選擇低風險 pure helper。`discover.c`、pipeline orchestration、SQLite、tree-sitter/LSP 與其餘 C runtime 仍未替換。

## Foundation mem handoff

- 切片：`foundation/mem.c`。
- Rust feature：`mem-only`；C opt-in：`CBM_USE_RUST_MEM_ONLY=1`。
- ABI：Rust 輸出 `cbm_mem_init`、`cbm_mem_rss`、`cbm_mem_peak_rss`、`cbm_mem_budget`、`cbm_mem_over_budget`、`cbm_mem_worker_budget`、`cbm_mem_collect`。
- C bridge 僅處理 mimalloc 與平台記憶體查詢；smoke 以 stub 提供同名 bridge 依賴。`cbm_rs_mem_bridge_collect` 避免與既有 test-only `cbm_rs_mem_collect` 衝突。
- 驗證：mem-only Cargo 237 tests、clippy、預設與直連 C suite 各 5868 passed、FFI smoke、production binary `--version` 全部通過。

### Foundation diagnostics Rust handoff

- Feature：`diagnostics-only`；C opt-in：`CBM_USE_RUST_DIAGNOSTICS_ONLY=1`。
- Rust 實作：`rust/cbm-core/src/ffi_diagnostics_only.rs`。
- C 邊界：`src/foundation/diagnostics_rust_bridge.c` 僅包裝 mimalloc process-info；direct smoke stub 為 `src/foundation/diagnostics_ffi_stub.c`。
- 驗證結果：all-features 266 tests 與 strict clippy 通過；全部 opt-in wrapper/direct 各 5869 tests 通過；diagnostics direct FFI 建立 snapshot/NDJSON 並正常 stop；production `--version` 通過。
- 後續：不要把這個 slice 解讀成整個 C server 已被 Rust 取代；下一階段應從仍由 C 提供、且可形成完整 ABI 邊界的模組繼續。

## 本回合補強：path_alias Unicode escape 解析修正（2026-07-15）

發現 `ffi_pipeline_path_alias_only.rs` 中 `
\uXXXX` 解析邏輯將 `self.pos` 在讀取第一個 hex 前導段時增加 4 而非 5，對含 surrogate 的 escape 會造成偏移。已修正為 `+5` 並維持 surrogate 成對位移 `+6`。

新增 regression 單元測試 `parser_accepts_unicode_escape_in_alias_prefix`，使用 `"\\u0040/*"` 在 JSON alias key 中驗證 `@` 解碼與 alias resolve。

切片行為：path alias 直替換流程維持不變（direct-only source gate、FFI bridge、all-optin 矩陣保持），僅補齊 parser 邊界缺失。

下一步建議：繼續盤點尚未 direct-only 的 production C ABI，優先處理 `discover`/`pipeline` 剩餘純函式邊界。

- Pipeline test node `is_test` predicate 已具備 Rust unit test、C FFI smoke test 與 `CBM_USE_RUST_PIPELINE_TEST_NODE_IS_TEST` opt-in gate；尚未執行本輪建置、測試或 lint。
- UI layout pure helpers 已新增 Rust unit、C FFI smoke 與 `CBM_USE_RUST_UI_LAYOUT_HELPERS` opt-in target；本輪尚未執行建置、測試或 lint。
- UI layout gate 另涵蓋 FNV-1a 與 jitter；Rust/C ABI smoke case 已加入，尚未執行驗證。
- UI octree scalar/vector helpers 已接上 Rust ABI；octree allocation、insert 與 repulsion 仍為 C，尚未執行驗證。
- UI layout node-id binary search 已加入 `#[repr(C)]` ABI 與 smoke case；map ownership 仍在 C，尚未執行驗證。
- UI layout max-node clamp 已接上 Rust ABI；環境變數解析與 UI I/O 未移動，尚未執行驗證。
- UI HTTP parser header helpers 已具備 Rust unit、C FFI smoke 與 opt-in target；socket/handler 仍在 C，尚未執行驗證。
- AST profile classifiers 已加入 Rust unit、C FFI smoke 與 opt-in target；Tree-sitter traversal 仍在 C，尚未執行驗證。
- AST profile gate 亦涵蓋 Halstead insert ABI 與 smoke case；set 仍由 C 配置，尚未執行驗證。
- AST profile parameter-name membership 已接上 Rust ABI；輸入陣列仍由 C 持有，尚未執行驗證。
- Pipeline usages `local_name` helper 已具備 Rust unit、C FFI smoke 與 opt-in target；尚未執行驗證。
- Semantic token boundary helpers 已加入 Rust unit、C FFI smoke 與 opt-in target；尚未執行驗證。
