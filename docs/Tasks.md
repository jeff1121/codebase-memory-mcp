> **歷史快照警示（2026-07-16）**：本檔的勾選狀態只代表早期工作階段，不能作為目前驗證證據。
> 請改讀 [rust-refactor-current-handoff.md](rust-refactor-current-handoff.md) 與根目錄的
> [Tasks.md](../Tasks.md)。新的 gate 結果只可在實際執行後更新。

## 2026-07-13 完成：Pipeline plan direct-only gate

- [x] 新增 `CBM_USE_RUST_PIPELINE_PLAN_ONLY`，使 direct build 的 pipeline plan 決策只接受 Rust ABI 結果。
- [x] 移除 direct plan path 的 C planner fallback；保留 C pass side effects 作為尚未 Rust 化的執行層。
- [x] 新增 default/direct focused gate 與全 opt-in direct matrix wiring。
- [x] 通過 focused pipeline、FFI smoke、production link，以及全 opt-in wrapper/direct matrix。
- [ ] 尚未完成整個 pipeline orchestration 的 Rust replacement；pass side effects、完整 registry resolver/cache/index 與其他 C subsystems 仍待後續切片。


## 2026-07-13 完成：Arena direct replacement

- [x] 以 Rust FFI 取代 direct build 的 arena 核心實作。
- [x] 將 variadic `cbm_arena_sprintf` 限縮為最小 C compatibility shim。
- [x] 新增 default/direct foundation、FFI、production 與全矩陣 gates。
- [x] 通過 foundation `325 passed` 與全 opt-in wrapper/direct matrix。

## 2026-07-13：graph buffer mutation

- [x] 新增 `graph-buffer-mutation-only` Cargo feature 與 Rust FFI dispatch。
- [x] direct-only build 排除 `src/graph_buffer/graph_buffer_mutation.c`。
- [x] 建立 default/direct graph buffer、FFI、production 與版本 gate。
- [ ] 保留 `graph_buffer.c` 的完整儲存實作，後續仍需獨立設計 Rust graph buffer ownership 與 SQLite dump migration。

## 2026-07-13：pipeline FQN

- [x] 將 `fqn_compute`、`fqn_module`、`fqn_module_dir`、`fqn_folder` 移植至 Rust。
- [x] 將 Python/JavaScript relative import resolver 與 C path buffer 邊界移植至 Rust。
- [x] 以 C `malloc` 維持既有 FFI 回傳字串的 ownership 契約。
- [x] direct-only build 排除 `src/pipeline/fqn.c`，並加入 default/direct/FFI/production gate。

## Pipeline registry direct-only

- [x] 以 Rust handle 取代 `registry.c` 的公開 registry ABI。
- [x] 移植 QN、label、simple-name index、resolve 與 fuzzy resolve。
- [x] 移植 ending lookup、import reachability、Perl builtin guard 與 confidence band。
- [x] 加入 C `malloc/free` 相容的 ending lookup 輸出契約。
- [x] 加入 default/direct registry、FFI、production 與版本 gate。
- [ ] 將 per-file thread-local resolve/reach/import-map cache 移植為 Rust 實作，恢復原有效能特性。

## 2026-07-15：pipeline module-directory direct 一致化

- [x] 將 `CBM_LANG_GO`/`CBM_LANG_JAVA` 的 module 判定移到共享 helper `cbm_lang_module_is_dir()`（`internal/cbm/helpers.c`）。
- [x] 將 `pass_calls.c`、`pass_parallel.c`、`pass_semantic.c`、`pass_usages.c`、`pass_lsp_cross.c` 全部改採 helper，移除各 pass 本地判定邏輯。
- [x] 將 `CBM_USE_RUST_PIPELINE_MODULE_DIR`、`CBM_USE_RUST_PIPELINE_MODULE_DIR_ONLY` 與 `pipeline-module-dir-only` 目標接入 Makefile、FFI 文檔與 test gate。
- [x] 補齊 `rust/CBM_FFI.md` 與專案紀錄文件，明確標示跨 pass direct 一致性已完成。
- [ ] 繼續盤點並完成下一個可直替換的 production C ABI 模組。

## 2026-07-13 Rust direct-only 進度

- [x] 完成 `system_info.c` 的 Rust direct-only ABI 與平台偵測實作。
- [x] 完成 cgroup v1/v2、worker policy、`CBM_WORKERS` invalid warning parity。
- [x] 完成 Makefile source gate、Cargo feature、FFI smoke、production link 與 CI direct gate。
- [x] 通過 Rust 236 項測試、clippy 嚴格 lint、全 opt-in wrapper/direct 5869 項測試。
- [ ] 續做尚未 direct-only 的 graph buffer 核心、SQLite store 與 pipeline orchestration。

## 2026-07-13 worker pool direct slice

- [x] 新增 `ffi_pipeline_worker_pool_only`，覆蓋 `cbm_parallel_for` 的 serial/threaded dispatch、worker 數量政策、callback/context ABI 與 fallback 行為。
- [x] 新增 `CBM_USE_RUST_PIPELINE_WORKER_POOL_ONLY` source/feature/link gate，以及獨立 `rust-pipeline-worker-pool-only-test` target。
- [x] 將 worker pool direct replacement 納入 `rust-all-optin-test` direct flag matrix。
- [x] 完成 default/direct `worker_pool parallel`、FFI smoke、Rust tests/clippy、production link 與版本驗證。
- [ ] 仍需依序處理其他未切出的 C 模組；本 slice 不代表全專案 Rust 遷移完成。

## 2026-07-13 slab allocator direct slice

- [x] 新增 `ffi_slab_alloc_only`，覆蓋 thread-local slab/free-list、heap fallback、calloc、realloc promotion、reset/destroy/reclaim 與 tree-sitter allocator callback。
- [x] 新增 `CBM_USE_RUST_SLAB_ALLOC_ONLY` source/feature/link gate、Tree-sitter `alloc.c` FFI 支援與獨立 `rust-slab-alloc-only-test` target。
- [x] 將 slab allocator direct replacement 納入 `rust-all-optin-test` direct flag matrix。
- [x] 完成 Rust tests/clippy、default/direct `mem`、FFI smoke、production link/version 與 all-optin wrapper/direct audit。
- [ ] 仍需依序處理其他未切出的 C 模組；本 slice 不代表全專案 Rust 遷移完成。

## Profiling runtime direct-only（2026-07-13）

- [x] 新增 `ffi_profile_only.rs`：profile active global、環境變數閘門、monotonic clock、elapsed/rate 與固定 log bridge。
- [x] 加入 `CBM_USE_RUST_PROFILE_ONLY` 的 source selector、Cargo feature、CFLAGS、FFI/link gate 與專用 target。
- [x] 以單一 `log.c` FFI support source 支援 profile 與 worker-pool 的組合 direct build，避免重複符號。
- [x] 完成 Rust 240 項測試、clippy、profile default/direct gate，以及 all-optin wrapper/direct 測試與 production link/version。
- [ ] 專案仍非完整 C 到 Rust 遷移；下一個模組須依相同 direct-only gate 評估與落地。

## AST profile codec direct-only（2026-07-13）

- [x] 新增 `ffi_ast_profile_codec_only.rs`，保留 25 欄 `cbm_ast_profile_t` 的 C ABI。
- [x] 直接替換 profile string encode/decode 與 25 維 normalized vector；保留 C Tree-sitter traversal fallback。
- [x] 加入 `CBM_USE_RUST_AST_PROFILE_CODEC_ONLY`、`ast-profile-codec-only` feature、C conditional、link filter 與專用 gate。
- [x] Rust all-features 244 項、clippy、codec default/direct 各 5868 項、FFI、production link/version 與 all-optin wrapper/direct 均通過。
- [ ] `ast_profile` traversal 仍待後續以明確 Tree-sitter ABI 計畫替換；專案仍非完整 C 到 Rust 遷移。

## Semantic dense-vector direct-only（2026-07-13）

- [x] 新增 `ffi_semantic_vector_only.rs`，保留 `cbm_sem_vec_t` 的 768 維 C ABI。
- [x] 直接替換 cosine、normalize、scaled-add；保留 pretrained/random-index、corpus、scoring 與 diffusion C fallback。
- [x] 加入 `CBM_USE_RUST_SEMANTIC_VECTOR_ONLY`、`semantic-vector-only` feature、C conditional、link filter 與專用 gate。
- [x] Rust all-features 247 項、clippy、semantic-vector default/direct 各 5868 項、FFI、production link/version 與 all-optin wrapper/direct 均通過。
- [ ] semantic corpus、random-index 與 scoring orchestration 仍待後續以更大範圍 ABI 計畫替換；專案仍非完整 C 到 Rust 遷移。

## Semantic proximity direct-only（2026-07-13）

- [x] 新增 `ffi_semantic_proximity_only.rs`，以 byte-level C string 契約實作 path proximity multiplier。
- [x] 直接替換 `cbm_sem_proximity`，保留 NULL、無目錄、共同 prefix slash count 與 1.10 上限行為。
- [x] 加入 `CBM_USE_RUST_SEMANTIC_PROXIMITY_ONLY`、`semantic-proximity-only` feature、C conditional、link filter 與專用 gate。
- [x] Rust all-features 250 項、clippy、proximity default/direct 各 5868 項、FFI、production link/version 與 all-optin wrapper/direct 均通過。
- [ ] semantic corpus、random-index、combined score 與 graph diffusion 仍待後續評估；專案仍非完整 C 到 Rust 遷移。

## Semantic tokenizer direct-only（2026-07-13）

- [x] 新增 `ffi_semantic_tokenize_only.rs`，保留 C tokenizer 的 ASCII/camel/delimiter 與 abbreviation 契約。
- [x] 以 C `malloc` 配置輸出 token strings，維持現有 C `free` ownership。
- [x] 加入 `CBM_USE_RUST_SEMANTIC_TOKENIZE_ONLY`、`semantic-tokenize-only` feature、C conditional、link filter 與專用 gate。
- [x] Rust all-features 253 項、clippy、tokenize default/direct 各 5868 項、FFI、production link/version 與 all-optin wrapper/direct 均通過。
- [ ] semantic corpus/random-index/scoring 仍待後續評估；專案仍非完整 C 到 Rust 遷移。

## Rust replacement：semantic config/env（2026-07-13）

- [x] 新增 `ffi_semantic_config_only.rs`，覆蓋 `cbm_sem_get_config()` 與 `cbm_sem_is_enabled()`。
- [x] 保留 `cbm_sem_config_t` 的 C ABI、預設值、threshold 環境變數解析與 enabled gate 行為。
- [x] 接入 `CBM_USE_RUST_SEMANTIC_CONFIG_ONLY` 的 Cargo、Makefile、FFI、production 與 all-optin gates。
- [x] 驗證 focused C runner（`5868 passed`）、Rust all-features（`255 passed`）、clippy 與 wrapper/direct all-optin。
- [ ] `src/semantic/semantic.c` 其餘語意索引、LSH、edge scoring 與 pass 邏輯仍待後續切片；目前不得宣稱 semantic 或整個 C 專案已全量 Rust 化。

## Rust replacement：discover/language（2026-07-14）

- [x] 將 `src/discover/language.c` 四個 exported API 替換為 `ffi_language_only.rs`。
- [x] 保留 159 個語言 enum 的副檔名、特殊檔名、名稱與 `.m` disambiguation contract。
- [x] 保留 C userconfig 覆寫；正式 binary 使用 `userconfig_bridge.c`，FFI smoke 使用 stub。
- [x] 接入 Cargo、Makefile source selection、direct gate、production 與 all-optin。
- [x] 驗證 Rust `257 passed`、clippy、default/direct `5868 passed` 與 wrapper/direct all-optin。
- [ ] `discover.c`、`gitignore.c`、`userconfig.c` 及其他未列入 direct gate 的 C 模組仍待依序替換。

## 2026-07-14 進度：discover 語言與 gitignore

- [x] 完成 `discover-language-only` Rust FFI slice：語言表、extension/filename 判定、語言名稱與 `.m` 消歧。
- [x] 完成 `discover-gitignore-only` Rust FFI slice：glob 規則、解析、載入、比對、釋放與合併。
- [x] 建立正式 `userconfig_bridge.c` 與 FFI smoke 專用 `userconfig_bridge_stub.c`。
- [x] 加入 Cargo feature、Makefile direct/default gate、全選用 Rust gate 與四份交接文件紀錄。
- [x] 驗證 Rust all-features：259 項測試通過，Clippy `-D warnings` 通過。
- [x] 驗證 gitignore default/direct：C 測試各 5868 項通過，正式執行檔可啟動。
- [x] 驗證全選用 Rust wrapper/direct：建置、完整測試與正式執行檔啟動通過。

> 目前仍是增量 opt-in 遷移；不得將上述結果解讀為整個 C 核心已完成 Rust 取代。

## 2026-07-14 進度：discover userconfig

- [x] 完成 `discover-userconfig-only` Rust FFI：load、parse、lookup、free、set/get global config。
- [x] 保留 global/project 設定優先序、invalid JSON fail-open、`CBM_LANG_COUNT` sentinel 與 project dedup 行為。
- [x] 建立獨立 userconfig FFI path/language stubs，並限制為 smoke link；production 使用真實 C/Rust 依賴。
- [x] 加入 Rust unit、`test_rust_ffi.c` smoke、default/direct dedicated gate、all-optin direct flag 與文件紀錄。
- [x] 驗證 userconfig feature：238 tests passed，strict Clippy passed。
- [x] 驗證 all-features：262 tests passed，strict Clippy passed。
- [x] 驗證 dedicated default/direct 與全選用 Rust wrapper/direct gate；完整 C runner 各 5868 項通過。

> `cargo fmt --all -- --check` 仍會報出既有其他 Rust 檔的格式差異；本輪新增 `ffi_userconfig_only.rs` 已單獨通過 `rustfmt --check`，未格式化無關檔案。

## Foundation mem 直替換進度

- [x] 建立 `mem-only` Cargo feature 與 `CBM_USE_RUST_MEM_ONLY` C gate。
- [x] 以 Rust 實作 budget、RSS/peak、worker budget、pressure 與 collect ABI。
- [x] 建立 production mimalloc/OS bridge 與 FFI smoke stubs。
- [x] 通過 mem-only Cargo test、clippy、預設/直連 5868 項 C 全測試、FFI smoke、production link 與版本啟動。
- [ ] 繼續盤點並替換下一個仍由 production C 實作的模組。

### Foundation diagnostics Rust 替換完成

- [x] 以 `CBM_USE_RUST_DIAGNOSTICS_ONLY` 切換完整 diagnostics production implementation。
- [x] 保留 mimalloc process-info 的最小 C bridge，並提供 direct FFI stub。
- [x] 維持 query stats、snapshot/NDJSON、atomic rename、rotation、start/stop 與 C ABI。
- [x] 完成 Rust all-features 266 tests、strict clippy、wrapper/direct all-optin 各 5869 tests 與 production link 驗證。
- [ ] 繼續盤點下一個仍由 C 提供的完整 production 模組；本項不代表全 repository 已完成 Rust 替換。

## 2026-07-15 進度：path alias parser 缺陷修正

- [x] 修正 `ffi_pipeline_path_alias_only.rs` 中 `\uXXXX` 轉義後的位移邏輯，避免 surrogate 前導字元時錯位導致解析失敗。
- [x] 新增 `parser_accepts_unicode_escape_in_alias_prefix` 回歸測試，覆蓋 JSON 路徑別名在 `@` 等字元逃脫的解析。
- [ ] 繼續盤點並完成下一個可直替換的 production C ABI 模組。

- [x] 將 pipeline test node 的 `is_test` JSON 純判斷移至 Rust，保留 C graph mutation 與所有權。
- [x] 將 UI layout 的 degree 顏色與 label 尺寸純 helper 移至 Rust，保留 Barnes-Hut、SQLite 與結果所有權於 C。
- [x] 將 UI layout 的 FNV-1a seed 與 deterministic jitter 納入 Rust helper gate。
- [x] 將 UI octree 的 octant 與 child-center 純計算移至 Rust，保留 C node ownership。
- [x] 將 UI layout node-id map 二分搜尋移至 Rust，保留 C map 配置與排序責任。
- [x] 將 UI layout max-node clamp predicate 移至 Rust，保留 C 環境設定解析。
- [x] 將 UI HTTP parser 的 header name/value 純 helper 移至 Rust，保留 C socket 與 request ownership。
- [x] 將 AST profile node-kind classifiers 移至 Rust bitmask ABI，保留 C traversal 與 profile ownership。
- [x] 將 AST profile Halstead fixed-set insert 移至 Rust，保留 C set ownership。
- [x] 將 AST profile parameter-name membership 移至 Rust，保留 C input lifetime。
- [x] 將 pass_usages 的 IMPORTS `local_name` JSON 純擷取移至 Rust，保留 C graph ownership。
- [x] 將 semantic token delimiter 與 camelCase boundary predicate 移至 Rust，保留 C token ownership。
