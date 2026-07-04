# Rust 重構計畫

## 摘要

本文件規劃以分階段、可驗證、低風險方式，將原本以 C 為主的 `codebase-memory-mcp` 逐步重構為 Rust。產品預設路徑目前仍是 C，Rust 僅透過明確 opt-in 參與部分 foundation 與 pipeline 決策/adapter 切片；重點不是一次性全面改寫，而是先建立 Rust 骨架與行為基準，再按模組遷移，確保現有單一靜態執行檔、MCP 介面、SQLite 儲存格式、tree-sitter 支援、CLI 行為與安全稽核流程都能維持相容。

## 目前執行狀態

- Phase 0：已新增 `docs/rust-refactor-baseline.md` 與 `tests/golden/rust-refactor/` 第一批小型 golden fixtures，固定 build/test/lint/security、CLI index/status/list、CLI invocation forms、MCP initialize/tools list pagination/error/Content-Length transcript、alias/schema/indexed-repo tool call transcript、小型 graph histogram、language graph parity、artifact export/bootstrap/import/schema mismatch、小型效能 smoke、手動 repository self-index baseline 與手動大型 synthetic performance baseline。
- Phase 1：已新增 Cargo workspace、`cbm-core` crate、`rust/CBM_FFI.md`、`scripts/rust-check.sh` 與 `Makefile.cbm` 的 `rust-*` targets；預設產品 C binary 仍不連入 Rust，只有明確 opt-in 才連 `cbm-core` staticlib。
- Phase 2：已建立第一批 foundation parity 模組：`dump_verify`、`str_intern`、`str_util` 與窄切 `platform` helpers 的 Rust 實作及單元測試。`dump_verify` 可用 `CBM_USE_RUST_DUMP_VERIFY=1` opt-in，`str_intern` 可用 `CBM_USE_RUST_STR_INTERN=1` opt-in，`str_util` 可用 `CBM_USE_RUST_STR_UTIL=1` opt-in predicate、validator、`json_escape`、路徑 helpers 與 arena-backed 字串 helpers；C wrapper 仍由 `CBMArena` 配置輸出，Rust 只回傳 offset/required length 或寫入呼叫端 buffer。`platform` 路徑正規化、Linux cgroup quota helpers 與 env/dirs helpers 可分別用 `CBM_USE_RUST_PLATFORM_PATH=1`、`CBM_USE_RUST_PLATFORM_CGROUP=1`、`CBM_USE_RUST_PLATFORM_ENV_DIRS=1` opt-in；env/dirs 覆蓋 `cbm_safe_getenv`、home/config/local/cache 優先序、覆寫、截斷與路徑正規化。下一批 foundation 已補 diagnostics deterministic formatter Rust parity/FFI fixture；hash_table 暫定 C-only contract，不導入產品 Rust opt-in。預設仍走原 C implementation。
- Phase 4：已啟動低風險 registry 與 plan 切片。registry 已新增 Rust `pipeline::registry` 純決策模組、`cbm_rs_registry_resolve_once` parity fixture，以及產品用的 opaque registry handle FFI。`CBM_USE_RUST_PIPELINE_REGISTRY=1` 現在會讓 `cbm_registry_add` 同步寫入 Rust handle，`cbm_registry_resolve` 在既有 C per-file cache miss 後先嘗試 Rust resolver，再將 Rust 回傳 QN 映射回 C registry-owned key；`label_of`、`find_by_name`、`fuzzy_resolve` 與預設 C path 仍保留。plan 已新增 Rust `pipeline::plan` parity fixture，固定 full pipeline、sequential、parallel extraction、predump、parallel threshold、incremental extract/resolve 與 incremental post/tail typed order。`CBM_USE_RUST_PIPELINE_PLAN=1` 目前讓 C full/incremental extraction phase 讀 Rust plan 來選擇 parallel 或 sequential，並在驗證 Rust typed v2 metadata 後 dispatch incremental extract/resolve：sequential 執行 `definitions`、`calls`、`usages`、`semantic`，parallel 執行 `incr_extract`、`incr_registry`、`incr_resolve`；讓 sequential extraction dispatch 依 Rust typed v2 metadata 執行 `definitions`、`k8s`、`lsp_cross`、`calls`、`usages` 與 `semantic`，讓 parallel extraction 外層 metadata 固定 `parallel_extract`、`registry_build`、`lsp_cross_prepare`、`parallel_resolve`、`infra_routes`、`infra_bindings` 與 `k8s` 的順序與 policy trace，讓 full pipeline pre-dump 階段依 Rust typed v2 predump metadata dispatch `decorator_tags`、`configlink`、`route_match`、`similarity`、`semantic_edges` 與 `complexity`，並讓 typed incremental post steps dispatch `k8s`、`incr_tests`、`incr_decorator_tags`、`incr_configlink`、`incr_similarity`、`incr_semantic_edges`、`edge_relink`、`incremental_dump`、`persist_hashes` 與 `artifact_export`；C 端仍固定驗證 sequential/predump/parallel/incremental extraction `kind/phase/policy/gate_flags/requires_mask/effect_flags` 與 incremental tail 順序/policy，並執行既有 C pass、cache/shared id、cross-LSP cleanup/skip、infra tail、dump、hash persist、FTS rebuild 與 artifact export side effects。inline pass trace contract 會固定 sequential/predump/parallel/incremental extraction dispatch log、`source=typed_v2` 與 pass 順序；incremental dump failure 會原樣傳回錯誤，並跳過後續 `persist_hashes` / `artifact_export` tail。graph-buffer mutation command 邊界已新增 Rust test-only metadata/validation ABI，並新增 C-side `graph_buffer_mutation` adapter，讓 full structure、incremental purge/file-upsert、incremental inbound restore、`TESTS`/`TESTS_FILE`、`pass_usages.c` usage/throws/read-write edges、`pass_configlink.c` configlink edges、`pass_similarity.c` `SIMILAR_TO`、`pass_semantic_edges.c` `SEMANTICALLY_RELATED` 與 `pass_enrichment.c` decorator-tag node upsert 的代表性呼叫先走 command apply 邊界；Rust 只固定 `UpsertNode`、`InsertEdge` 與 `DeleteByFile` 的欄位、結果型別、effect flags 與 null/empty string contract，實際 pass function、tree-sitter/extraction/LSP、graph-buffer storage/ownership 與所有寫入仍留在 C。language graph parity gate 已固定 Python、TypeScript、Go 與 YAML 代表性 fixture，並比對 default C path 與 Rust registry/plan opt-in path 的 definitions、calls、imports、routes、semantic/LSP edges。
- 驗證（2026-07-02 baseline）：本機曾完整通過 `scripts/test.sh`、`scripts/rust-check.sh`、`scripts/build.sh`、`scripts/smoke-test.sh build/c/codebase-memory-mcp`、`scripts/smoke-invariants.sh build/c/codebase-memory-mcp`（30 passed / 0 failed）、`make -f Makefile.cbm security`、`make -f Makefile.cbm rust-ci` 與 `make -f Makefile.cbm rust-baseline-fixtures`；`scripts/lint.sh --ci` 也以 CI 對齊的 `cppcheck 2.20.0` + `clang-format 20.1.8` 通過，`RUST_AUDIT_REQUIRED=1` 已用 `cargo-audit v0.22.2` 驗證 `Cargo.lock`。
- 驗證（2026-07-04 Phase 4 targeted gates）：本輪重跑 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::plan --locked`（11 passed）、`cargo test -p cbm-core pipeline::graph_mutation --locked`（3 passed）、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner graph_buffer`（51 passed）、`build/c/test-runner pipeline`（default C path，217 passed）、`make -f Makefile.cbm rust-pipeline-plan-optin-test`（217 passed）、`make -f Makefile.cbm rust-language-graph-parity`、`make -f Makefile.cbm lint-format`、`git diff --check` 與 `git diff --exit-code -- tests/golden/rust-refactor`。完整 `scripts/test.sh` / `scripts/lint.sh --ci` / `make -f Makefile.cbm security` 尚未在本輪 targeted 修補後重跑，仍以 2026-07-02 baseline 為最近完整紀錄。
- 驗證（2026-07-05 Phase 4 targeted gates）：本輪新增 `SIMILAR_TO`、`SEMANTICALLY_RELATED` 與 decorator-tag upsert 的 C-side adapter callpoint，並重跑 `make -f Makefile.cbm clean-c build/c/test-runner`、`build/c/test-runner graph_buffer pipeline`（268 passed）、`make -f Makefile.cbm rust-pipeline-plan-optin-test`（217 passed）、`make -f Makefile.cbm rust-language-graph-parity`、`scripts/lint.sh --ci`、`cargo fmt --all -- --check`、`make -f Makefile.cbm lint-format`、`git diff --check` 與 `git diff --exit-code -- tests/golden/rust-refactor`。同日稍早 `scripts/test.sh` 曾啟動但依使用者要求中斷，不視為新的完整通過；`make -f Makefile.cbm security` 也尚未在此批 adapter 變更後重跑。
- CI 接入（2026-07-02）：新增 `.github/workflows/_rust.yml` 專責 Rust gates（lint / unit / FFI / foundation 與 pipeline registry+plan opt-in / language-graph parity），並於 `pr.yml` 納入 `rust` job 與 `ci-ok` 必要脈絡；`_security.yml` 既有的 `rust-dangerous-api` 與 `cargo-audit` fail-closed 維持不變。新增 `rust-ci-tests` make 目標作為 CI 用的測試/parity 子集。
- 剩餘範圍（刻意保留）：Phase 4 的 pipeline orchestration 全面遷移與 Phase 5 的「切換預設為 Rust-backed」尚未執行——Rust 端目前承接 registry/plan 決策層與 graph-buffer mutation command contract，C 端已有少量同步 command adapter 呼叫點；實際 pass/extraction/tree-sitter/LSP 與 graph-buffer 寫入仍在 C；提前切換會破壞產品並違反本計畫「C fallback 需經至少一個 release cycle」的完成標準，詳見 `Tasks.md`。
- 下一批 foundation 評估：`platform` env/dirs 已完成 Rust ABI、C opt-in 與 parity fixture。`hash_table` 已補 C contract/stress tests，但因 borrowed key pointer、`get_key` stored pointer、NULL value 與 foreach callback contract，目前標記為 C-only hot path；若未來試作 Rust 版，先走 test-only `cbm_rs_ht_*` parity fixture。`diagnostics` 已補 deterministic query stats、env parser、path、JSON 與 NDJSON 的 Rust parity/FFI fixture；writer thread、檔案 rotation、system metrics 與 stderr lifecycle 仍留 C。`hash_table` 已補 test-only `cbm_rs_ht_*` parity fixture（`foundation::hash_table` 純安全 Rust module + FFI + `tests/test_rust_ffi.c` parity），固定 borrowed key pointer、`get_key` stored pointer、NULL value 與 foreach callback contract；仍維持 C-only hot path，不導入產品 opt-in。Phase 3 的 store/cypher/MCP 已整理於 `docs/rust-refactor-phase3-plan.md`；`store_compat` contract fixture 已凍結 minimal schema/index existence、`edges.url_path_gen`、user index drop/create symmetry、`open_path_query` no-create/read-only、WAL journal mode、readonly WAL read/write rejection、generated column query plan、URL path API 的 project-scoped substring 行為，以及 `nodes_fts` 手動 rebuild/camelCase tokenization；同時修正 `idx_edges_url_path` 未被 drop 的不對稱。`cypher_contract` 已凍結 parser AST shape、OPTIONAL MATCH、UNION/UNION ALL、edge property、deterministic ordered rows、WITH/post-WHERE AST、explicit `LIMIT 0` 與 exact errors。`mcp` suite 已凍結 BM25 `file_pattern` 與 label boost rank/order contract；MCP golden 已凍結 alias、tool schema 與 indexed repo `query_graph` transcript。

## 目標與非目標

目標：

- 提升記憶體安全、模組邊界清晰度與測試可維護性。
- 保留既有 CLI、MCP JSON-RPC stdio 行為、SQLite schema 與圖資料模型。
- 讓每個遷移階段都能獨立驗證、回滾與比較行為。

非目標：

- 第一階段不變更 MCP 工具 API。
- 不更換圖資料模型或儲存格式。
- 不重寫 `graph-ui`。
- 不移除既有 C workflow，直到 Rust 實作通過等價驗證。

## 現況與約束

目前產品預設核心仍位於 `src/` 的 C 程式碼，包含 `foundation/`、`store/`、`cypher/`、`mcp/`、`pipeline/`、`discover/`、`watcher/`、`cli/` 與 `ui/`。Rust workspace 已存在於 `rust/`，但只在明確 opt-in 下支援部分 foundation parity、pipeline registry/plan 決策與 test-only graph-buffer command contract。tree-sitter 擷取、語言規格與 Hybrid LSP 邏輯位於 `internal/cbm/`。測試集中在 `tests/test_*.c` 與 `tests/repro/`。

現有主要驗證流程：

- `scripts/build.sh`：建置 production binary。
- `scripts/test.sh`：ASan/UBSan 測試與迴歸驗證。
- `scripts/lint.sh`：clang-tidy、cppcheck、clang-format 與 no-skip 檢查。
- `make -f Makefile.cbm security`：安全稽核、fuzz、binary string 與 vendored dependency 檢查。

Rust 重構必須保留以下約束：

- 單一跨平台執行檔，支援 macOS、Linux 與 Windows。
- 不破壞安裝腳本、package wrappers 與 release artifact 命名。
- 不破壞 vendored dependencies、tree-sitter grammars、SQLite WAL/FTS5 行為。
- stdout 保留給 MCP JSON-RPC，log 與診斷輸出維持在 stderr 或既有位置。

## 分階段重構計畫

### Phase 0：建立基準

- 固定目前 C 實作的 CLI、MCP tool、SQLite、索引輸出與安全測試基準。
- 建立 golden fixtures：CLI 輸出、JSON-RPC request/response、代表性 repository 的 graph 統計。
- 記錄效能基準：索引時間、查詢延遲、記憶體峰值、binary size。
- 所有後續 Rust 變更都必須與 Phase 0 基準比對。

### Phase 1：加入 Rust 骨架

- 新增 Rust workspace 與最小 crate，但不改變產品行為。
- 建立 C/Rust 邊界規範：錯誤碼、字串所有權、buffer ownership、allocator 規則。
- 新增 Rust 檢查流程，例如 `cargo test`、`cargo clippy`、`cargo fmt --check`。
- 將 Rust 驗證接入 CI 或本地 wrapper，但不取代既有 C 驗證。

### Phase 2：遷移低風險工具模組

- 優先遷移邊界清楚、狀態少的 foundation 類工具，例如字串、路徑、hash、診斷輔助。
- 每個模組先保留 C 測試，再新增 Rust 單元測試與 FFI 等價測試。
- 對外 C header 與呼叫端行為保持不變，直到所有相依模組完成遷移。

### Phase 3：遷移核心服務邊界

- 逐步遷移 `store`、`cypher`、`mcp` 等子系統。
- `store` 必須以 SQLite fixture 驗證 schema、索引、查詢結果與 WAL 行為相容。
- `cypher` 必須使用 parser AST、query result 與 exact error golden tests 驗證相容性；現行架構沒有 SQL emit API，若未來新增 SQL backend，SQL output golden 應作為獨立 contract。
- `mcp` 必須使用 JSON-RPC fixtures 驗證所有 tool 的輸入、輸出、錯誤碼與 stdout/stderr 行為。

### Phase 4：遷移 pipeline 與 extraction 周邊

- 遷移 pipeline orchestration、pass registry 與資料流管理。
- tree-sitter grammar 綁定與 `internal/cbm/` 語言行為先維持相容，再逐步替換周邊 glue code。
- 對代表性語言執行 graph 統計比對，包含 definitions、calls、imports、routes、semantic/LSP edges；後續 extraction、tree-sitter 與 LSP glue 遷移必須持續擴充此 gate。

### Phase 5：切換預設建置與清理

- 當 Rust 實作達到等價門檻後，將預設 build path 切換為 Rust-backed binary。
- 保留 C fallback 至少一個 release cycle。
- 移除已取代 C 模組前，先確認 packaging、install/update/uninstall、UI variant、security suite 全部通過。
- 更新 `README.md`、`CONTRIBUTING.md`、release workflow 與 contributor guide。

## 驗證策略

必要測試：

- 單元測試：Rust `cargo test` 與保留的 C test runner。
- 整合測試：`scripts/test.sh` 或其 Rust-era 等價流程。
- CLI golden tests：子命令、參數、stdin、`--args-file`、錯誤訊息。
- MCP 相容測試：JSON-RPC request/response、tool schema、錯誤碼、stdout/stderr 分離。
- SQLite 相容測試：既有 DB 匯入、schema migration、FTS5 搜尋、WAL 行為。
- Graph 等價測試：小型 histogram、代表性 repository 的 node/edge count、route linking、call graph、semantic edges，以及 language graph parity gate 中的 definitions、calls、imports、routes、semantic/LSP edges。

安全驗證：

- 保留 `make -f Makefile.cbm security` 或提供等價 Rust-era security wrapper。
- 新增 Rust dependency 檢查，例如 `cargo audit` 與 `cargo deny`。
- 對 FFI 邊界加入 sanitizer、Miri 或 fuzz coverage。
- 新增危險 API 審查規則，涵蓋 process spawning、filesystem writes、network calls 與 unsafe blocks。

效能驗證：

- 比較索引時間、查詢延遲、記憶體峰值與 binary size。
- 完整效能基準的目標門檻：效能退化不超過 10%，binary size 退化不超過 20%；第一批小型 smoke baseline 暫以較寬鬆的 wall-time guard 偵測明顯退化。
- 超過門檻時必須記錄原因、影響範圍與後續優化計畫。

跨平台驗證：

- macOS arm64/x86_64、Linux x86_64/arm64、Windows x86_64。
- 分別驗證標準 binary 與 `--with-ui` variant。
- 驗證 npm、PyPI、Homebrew、installer scripts 與 agent config 行為。

## 風險與緩解

- FFI 所有權錯誤：每個 FFI API 必須明確標示 caller/callee ownership，並以測試覆蓋 null、empty、large buffer。
- SQLite 行為差異：以 fixture DB 與 golden query 驗證，不接受隱性 schema 或 pragma 變更。
- tree-sitter binding 差異：先保留現有 C grammar shim，再逐步遷移周邊 glue。
- 靜態連結與跨平台包裝：每個階段都必須產出可安裝 artifact，不只通過本機測試。
- 效能退化：每個子系統遷移都要跑 baseline comparison，不等到最後才量測。
- 安全模型漂移：任何新增 unsafe、process、network、filesystem write 都要納入 allowlist 與安全審查。

## 完成標準

- 所有 MCP tool 的輸入輸出與錯誤行為維持相容。
- 既有 CLI 子命令、install/update/uninstall/config 行為維持相容。
- SQLite 儲存格式、graph artifact 與既有資料庫可讀寫。
- `scripts/test.sh`、`scripts/lint.sh --ci`、`make -f Makefile.cbm security` 有對應 Rust-era 驗證並通過。
- 主要效能指標符合門檻，或有明確記錄與核准的例外。
- C fallback 已經過至少一個 release cycle 驗證後，才能移除被取代的 C 模組。

## 完整移除 C 的路線圖（依子系統排序）

> 目前位置：預設產品 binary 仍是 **100% 純 C、零 Rust**。`rust/cbm-core/src` 約 4.6K 行 / 13 檔，仍只佔整體自有程式碼少數，且所有 production Rust 皆由 `CBM_USE_RUST_*` 明確 opt-in 啟用；C 原始碼與預設路徑仍完整保留。已完成的是 foundation 部分 parity、pipeline registry/plan 的決策層 parity，以及 test-only graph-buffer command contract；離「可移除 C」仍差完整子系統取代、預設 build 切換、至少一個 release cycle 的 C fallback 驗證與最終 packaging/install/UI/security/perf gate。

每個切片一律維持既有方法論：先凍結 contract/golden fixture（預設 C 必須通過）→ 新增 Rust pure helper 與 FFI smoke（不改預設行為）→ 新增單一 `CBM_USE_RUST_*` opt-in → 跑 default / 單一 opt-in / 全 opt-in matrix → 接 `scripts/test.sh`、`scripts/smoke-test.sh`、`scripts/smoke-invariants.sh`、`make -f Makefile.cbm security` 與對應 parity target。每個 PR 維持單一議題、建議 < 500 行。

### 1. Foundation 剩餘葉模組（最低風險，但含 allocator hot path）

- **範圍**：`arena`、`compat_fs`、`compat_regex`、`compat_thread`、`compat`、`hash_table`、`log`、`mem`、`profile`、`slab_alloc`、`vmem`、`yaml`。
- **建議切片順序**：
  1. `yaml`：純解析、狀態少，可作下一個 production opt-in 候選。
  2. `log` / `profile`：先固定 stderr/file I/O、rotation、timestamp 與錯誤輸出 contract，再做 wrapper parity。
  3. `compat_*`：依平台切片，先 path/fs，再 regex/thread；Windows 行為必須先補 fixture。
  4. `hash_table`：test-only `cbm_rs_ht_*` parity fixture 起步**已完成**（`foundation::hash_table` 純安全 module + FFI + `tests/test_rust_ffi.c` parity）；仍不承諾 production opt-in，需在 hot-path pointer contract 可證明安全前維持 C-only。
  5. `arena` / `slab_alloc` / `mem` / `vmem`：最後處理，先只做 contract/stress/fuzz，再評估是否以 Rust ownership 包 C allocator 或整體替換。
- **主要風險**：allocator hot path 與 unsafe 邊界、借用指標生命週期、跨平台 filesystem/thread 差異、log/profile I/O 副作用、效能與 RSS 退化。
- **前置 gate**：每個模組先有預設 C contract/stress tests；allocator 類需 sanitizer/fuzz 或 stress fixture；`hash_table` 需維持 C-only hot path 直到 pointer contract 可證明安全。
- **驗證方式**：`make -f Makefile.cbm test-foundation`、單一 `CBM_USE_RUST_*` foundation opt-in、全 foundation opt-in、`scripts/rust-check.sh`、`make -f Makefile.cbm rust-ffi-test`、`scripts/test.sh`、`scripts/lint.sh --ci`。

### 2. Pipeline orchestration 全面化

- **範圍**：從目前已完成的 registry resolve 與 plan 決策層，擴大到 pass registry、full/incremental orchestration、資料流管理、錯誤傳遞與 graph-buffer mutation 的調度；實際 extraction/tree-sitter/LSP 仍在後續階段。
- **建議切片順序**：
  1. 完整 incremental post/tail adapter 已由 typed steps dispatch；incremental extract/resolve typed v2 dispatch、full/fast predump typed v2 metadata、pass trace golden 與 incremental dump failure side-effect contract 已固定。
  2. 將 pass registry metadata、gating、依賴順序與 parallel/sequential dispatch 持續移到 Rust；sequential extraction dispatch typed v2、parallel extraction 外層 typed v2 metadata/trace、incremental extract/resolve typed v2 dispatch/trace，以及 graph-buffer mutation command contract 已完成。
  3. C-side command adapter 已接入 full structure、incremental purge/file-upsert/inbound restore、`TESTS`/`TESTS_FILE`、usage edges、configlink edges、similarity/semantic edges 與 decorator-tag node upsert；後續才分批收斂更多 C pass 呼叫點，不得一次搬移所有 pass 或 graph-buffer storage。
  4. 擴充 language graph parity，覆蓋更多語言、incremental、fast-mode、semantic/LSP edges。
- **主要風險**：pass order 漂移、parallel race、incremental 與 full 結果不一致、graph-buffer ownership、效能退化。
- **前置 gate**：現有 registry/plan parity 維持全綠；每新增 dispatch 切片前，先固定 default C graph output 與 pass trace golden。
- **驗證方式**：`make -f Makefile.cbm rust-pipeline-registry-optin-test`、`make -f Makefile.cbm rust-pipeline-plan-optin-test`、`make -f Makefile.cbm rust-language-graph-parity`、`build/c/test-runner pipeline`、`scripts/test.sh`、`scripts/smoke-invariants.sh`。

### 3. Store：SQLite 圖儲存

- **範圍**：`src/store/` 的 schema、pragma、authorizer、UDF、FTS、CRUD、search、BFS、ADR、architecture、vector search、artifact import/export 與 readonly/bulk/checkpoint 行為。
- **建議切片順序**：
  1. 補完整 `sqlite_schema`、`table_xinfo`、index layout、FTS shadow object、checkpoint/bulk side effect、artifact import/export fixtures。
  2. Rust schema/index manifest helper，先只做只讀比對與 FFI smoke。
  3. Rust open/readonly/WAL/pragma helper，以 `CBM_USE_RUST_STORE_OPEN=1` 類旗標單獨 opt-in。
  4. CRUD/search/FTS/BM25 分開切；vector tables、graph_buffer direct writer 與 sqlite_writer 最後。
  5. artifact import/export 與 migration path 完成後，才評估 store 預設切換。
- **主要風險**：SQLite pragma/WAL 副作用、readonly 不得建立 DB、FTS/BM25 rank 漂移、UDF/authorizer 行為差異、vector schema 與既有資料庫相容性。
- **前置 gate**：`tests/test_store_compat.c` 擴充到完整 schema/side-effect golden；預設 C path 必須先通過。
- **驗證方式**：`build/c/test-runner store_compat`、`tests/test_store_*.c`、artifact bootstrap/import fixture、`scripts/test.sh`、`scripts/smoke-test.sh build/c/codebase-memory-mcp`、`make -f Makefile.cbm security`。

### 4. Cypher：查詢引擎

- **範圍**：lexer/parser、AST、query executor、aggregation、OPTIONAL MATCH、UNION、computed properties、error reporting，以及 MCP `query_graph` 依賴的結果格式。
- **建議切片順序**：
  1. 補 CALL、EXISTS pattern、malformed query、aggregation edge cases 與 exact error golden。
  2. Rust lexer/parser 或 normalized AST helper，先以 test-only FFI 比對 C AST shape。
  3. `CBM_USE_RUST_CYPHER_PARSER=1` production opt-in，只替換 parser/normalizer，不碰 executor。
  4. 運算式 evaluator、WHERE/filter、projection、aggregation 分段移轉。
  5. executor 最後移轉，並持續以固定 store fixture 比對 deterministic `columns` / `rows`。
- **主要風險**：parser precedence 漂移、錯誤訊息變動、query result ordering、store API side effect、aggregation 與 NULL/optional semantics。
- **前置 gate**：`tests/test_cypher_contract.c` 足夠覆蓋公開語意；若未來新增 SQL backend，SQL output golden 必須是獨立 contract，不可混入現行直接 executor 假設。
- **驗證方式**：`build/c/test-runner cypher cypher_contract`、MCP `query_graph` transcript、store fixture、`scripts/test.sh`、`scripts/smoke-invariants.sh`。

### 5. MCP：JSON-RPC 14 工具

- **範圍**：JSON-RPC codec/envelope、stdio framing、id preservation、tool schema、14 個 tool handler、store/pipeline/cypher 呼叫邊界、stdout/stderr 分離。
- **建議切片順序**：
  1. 補 `initialize` string id、完整 `tools/list` schema、invalid params、每個 tool 的成功/錯誤 transcript。
  2. `CBM_USE_RUST_MCP_CODEC=1`：只替換 JSON-RPC parse/serialize/envelope/framing。
  3. tool schema 產生器移到 Rust，但 handler 仍呼叫 C。
  4. 依副作用由低到高遷移 handler：read-only/query 類 → search/index 狀態類 → install/config 類。
  5. store/cypher/pipeline Rust-backed 穩定後，才移除 C handler fallback。
- **主要風險**：stdout 污染 MCP、JSON id/type preservation、pagination cursor、錯誤碼漂移、tool schema breaking change、長輸入與 invalid JSON robustness。
- **前置 gate**：golden transcript 覆蓋 14 tools、錯誤路徑與 framing；任何 tool API 變更需另開設計議題。
- **驗證方式**：`build/c/test-runner mcp`、`scripts/smoke-test.sh build/c/codebase-memory-mcp`、`scripts/smoke-invariants.sh build/c/codebase-memory-mcp`、`make -f Makefile.cbm rust-baseline-fixtures`、`make -f Makefile.cbm security`。

### 6. `internal/cbm`：tree-sitter + Hybrid LSP（最大、最高風險）

- **範圍**：158 語言 tree-sitter 擷取、vendored grammar binding、`lang_specs`、各語言 extractor、semantic edges、Hybrid LSP、route/import/definition/call extraction 與 language-specific regression。
- **建議切片順序**：
  1. 擴充 language graph parity：先覆蓋高流量語言（TypeScript/JavaScript、Python、Go、Rust、Java、C/C++、YAML/K8s），再分批加入其餘語言。
  2. 保留現有 C grammar shim，先將 language spec table 與 extraction rule metadata 正規化到 Rust。
  3. 逐語言遷移低副作用 extractor helper；每批只含少數語言與 fixtures。
  4. route/import/definition/call/semantic edges 分軸驗證，不混在同一 PR。
  5. Hybrid LSP 最後處理，需獨立 fixture 固定 timeout、fallback、semantic edge 與跨平台 process/IO 行為。
- **主要風險**：tree-sitter grammar binding 與 ABI 差異、語言覆蓋面巨大、LSP process/timeout 跨平台差異、semantic edge 漂移、效能與記憶體退化、Windows toolchain。
- **前置 gate**：每批語言都有 default C graph golden；vendored grammar 完整性與安全 allowlist 維持；LSP 需先凍結 fallback/timeout contract。
- **驗證方式**：`make -f Makefile.cbm rust-language-graph-parity`、`tests/test_extraction.c`、`tests/test_pipeline.c`、代表性 repository self-index baseline、large performance baseline、`scripts/test.sh`、跨平台 CI。

### 7. Phase 5：預設 build 切換為 Rust-backed

- **範圍**：當 foundation、pipeline、store、cypher、mcp、internal/cbm 的 Rust-backed opt-in matrix 都達等價門檻後，才把預設 `scripts/build.sh` / production binary 切到 Rust-backed；C fallback 仍保留。
- **建議切片順序**：
  1. 建立「全 Rust-backed opt-in」release candidate binary，與 default C binary 做完整基準比對。
  2. 將 CI/release workflow 加入 Rust-backed candidate，但不先改預設。
  3. 連續通過 default C、單一 opt-in、全 opt-in、Rust-backed candidate matrix。
  4. 宣告一個 release 的相容期，文件標明 C fallback 與回滾方式。
  5. 下一個 release 才把預設切到 Rust-backed。
- **主要風險**：靜態連結與 binary size、package wrapper、installer、Windows/macOS/Linux 差異、UI variant、效能退化、release artifact 命名或簽章漂移。
- **前置 gate**：所有子系統 opt-in 已可獨立與組合通過；效能退化 ≤10%、binary size 退化 ≤20%，或有維護者核准例外。
- **驗證方式**：`scripts/build.sh`、`scripts/build.sh --with-ui`、`scripts/test.sh`、`scripts/lint.sh --ci`、`scripts/smoke-test.sh`、`scripts/smoke-invariants.sh`、`make -f Makefile.cbm security`、packaging/install/update/uninstall 測試、跨平台 CI。

### 8. 一個 release cycle 後移除 C

- **範圍**：只移除已被 Rust-backed 預設完整取代、且 C fallback 已經過至少一個 release cycle 驗證的 C 模組、build flag、tests 與文件；不移除仍作為 vendored grammar 或必要 ABI shim 的 C。
- **建議切片順序**：
  1. 逐子系統刪除 C fallback 與對應 `CBM_USE_RUST_*` 反向旗標，先從 foundation/pipeline 開始。
  2. store/cypher/mcp 分別清理；每次刪除後重跑完整 artifact/DB/MCP golden。
  3. `internal/cbm` 最後刪除，保留必要 vendored grammar C 檔時需明確標示「非產品邏輯 fallback」。
  4. 更新 README、CONTRIBUTING、release notes、packaging 與安全 allowlist。
- **主要風險**：誤刪仍需的 C shim、release fallback 消失、安裝/升級路徑破壞、文件宣稱與實際 build 不一致。
- **前置 gate**：Rust-backed 預設已穩定一個 release cycle；所有使用者可見行為與 package artifacts 已驗證。
- **驗證方式**：完整 release checklist、`git grep CBM_USE_RUST_` 確認殘留語意、全測試/安全/packaging/UI/cross-platform gate。

### 移除 C 的最終檢核表

- [ ] 預設 Rust-backed binary 已等價通過，且不再需要 C fallback 才能完成任何產品功能。
- [ ] C fallback 已保留並驗證至少一個 release cycle。
- [ ] `scripts/build.sh` 與 `scripts/build.sh --with-ui` 皆通過，artifact 命名、簽章與安裝位置相容。
- [ ] install / update / uninstall / config / package wrappers（npm、PyPI、Homebrew、installer scripts）皆通過。
- [ ] `graph-ui` 內嵌 variant 與 non-UI variant 皆通過 smoke 與 release packaging。
- [ ] `scripts/test.sh`、`scripts/lint.sh --ci`、`scripts/smoke-test.sh`、`scripts/smoke-invariants.sh`、`make -f Makefile.cbm security` 全部通過。
- [ ] macOS arm64/x86_64、Linux x86_64/arm64、Windows x86_64 皆通過核心測試與安裝測試。
- [ ] 效能基準符合索引/查詢/RSS 退化 ≤10%，binary size 退化 ≤20%；例外需明確記錄與核准。
- [ ] SQLite 既有 DB、artifact import/export、FTS/WAL/readonly、MCP 14 tools、CLI 子命令、language graph parity 全部相容。
- [ ] Rust unsafe/process/network/filesystem allowlist、dependency audit、vendored integrity 與 security suite 均無未處理 finding。
- [ ] 文件、release notes、CONTRIBUTING、README 與 migration notes 均已更新，且不再宣稱預設為純 C。
