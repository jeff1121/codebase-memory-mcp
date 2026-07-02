# Rust 重構計畫

## 摘要

本文件規劃以分階段、可驗證、低風險方式，將原本以 C 為主的 `codebase-memory-mcp` 逐步重構為 Rust。產品預設路徑目前仍是 C，Rust 僅透過明確 opt-in 參與部分 foundation 模組；重點不是一次性全面改寫，而是先建立 Rust 骨架與行為基準，再按模組遷移，確保現有單一靜態執行檔、MCP 介面、SQLite 儲存格式、tree-sitter 支援、CLI 行為與安全稽核流程都能維持相容。

## 目前執行狀態

- Phase 0：已新增 `docs/rust-refactor-baseline.md` 與 `tests/golden/rust-refactor/` 第一批小型 golden fixtures，固定 build/test/lint/security、CLI index/status/list、CLI invocation forms、MCP initialize/tools list pagination/error/Content-Length transcript、alias/schema/indexed-repo tool call transcript、小型 graph histogram、language graph parity、artifact export/bootstrap/import/schema mismatch、小型效能 smoke、手動 repository self-index baseline 與手動大型 synthetic performance baseline。
- Phase 1：已新增 Cargo workspace、`cbm-core` crate、`rust/CBM_FFI.md`、`scripts/rust-check.sh` 與 `Makefile.cbm` 的 `rust-*` targets；預設產品 C binary 仍不連入 Rust，只有明確 opt-in 才連 `cbm-core` staticlib。
- Phase 2：已建立第一批 foundation parity 模組：`dump_verify`、`str_intern`、`str_util` 與窄切 `platform` helpers 的 Rust 實作及單元測試。`dump_verify` 可用 `CBM_USE_RUST_DUMP_VERIFY=1` opt-in，`str_intern` 可用 `CBM_USE_RUST_STR_INTERN=1` opt-in，`str_util` 可用 `CBM_USE_RUST_STR_UTIL=1` opt-in predicate、validator、`json_escape`、路徑 helpers 與 arena-backed 字串 helpers；C wrapper 仍由 `CBMArena` 配置輸出，Rust 只回傳 offset/required length 或寫入呼叫端 buffer。`platform` 路徑正規化、Linux cgroup quota helpers 與 env/dirs helpers 可分別用 `CBM_USE_RUST_PLATFORM_PATH=1`、`CBM_USE_RUST_PLATFORM_CGROUP=1`、`CBM_USE_RUST_PLATFORM_ENV_DIRS=1` opt-in；env/dirs 覆蓋 `cbm_safe_getenv`、home/config/local/cache 優先序、覆寫、截斷與路徑正規化。下一批 foundation 已補 diagnostics deterministic formatter Rust parity/FFI fixture；hash_table 暫定 C-only contract，不導入產品 Rust opt-in。預設仍走原 C implementation。
- Phase 4：已啟動低風險 registry 與 plan 切片。registry 已新增 Rust `pipeline::registry` 純決策模組、`cbm_rs_registry_resolve_once` parity fixture，以及產品用的 opaque registry handle FFI。`CBM_USE_RUST_PIPELINE_REGISTRY=1` 現在會讓 `cbm_registry_add` 同步寫入 Rust handle，`cbm_registry_resolve` 在既有 C per-file cache miss 後先嘗試 Rust resolver，再將 Rust 回傳 QN 映射回 C registry-owned key；`label_of`、`find_by_name`、`fuzzy_resolve` 與預設 C path 仍保留。plan 已新增 Rust `pipeline::plan` parity fixture，固定 full pipeline、sequential、parallel extraction、predump、parallel threshold、incremental extract/resolve 與 incremental dump/persist tail。`CBM_USE_RUST_PIPELINE_PLAN=1` 目前讓 C full/incremental extraction phase 讀 Rust plan 來選擇 parallel 或 sequential，並讓 full pipeline pre-dump 階段依 Rust plan dispatch `decorator_tags`、`configlink`、`route_match`、`similarity`、`semantic_edges` 與 `complexity`；實際 pass function、tree-sitter/extraction/LSP 與 graph-buffer mutation 仍留在 C。language graph parity gate 已固定 Python、TypeScript、Go 與 YAML 代表性 fixture，並比對 default C path 與 Rust registry/plan opt-in path 的 definitions、calls、imports、routes、semantic/LSP edges。
- 驗證：2026-07-02 本機驗證紀錄包含 `scripts/test.sh`、`scripts/rust-check.sh`、`scripts/build.sh`、`scripts/smoke-test.sh build/c/codebase-memory-mcp`、`scripts/smoke-invariants.sh build/c/codebase-memory-mcp`（30 passed / 0 failed）、`make -f Makefile.cbm security`、`make -f Makefile.cbm rust-ci` 與 `make -f Makefile.cbm rust-baseline-fixtures`。Rust gate 包含 `cargo fmt`、`cargo clippy --all-targets --all-features`、38 個 Rust 單元測試、FFI smoke、完整 `rust-foundation-optin-test` matrix，以及已接入 `_security.yml` 的 `rust-dangerous-api` source allowlist 與 `cargo-audit` 必要模式；matrix 已納入 `CBM_USE_RUST_PLATFORM_ENV_DIRS=1` 與全 opt-in 組合，每腳 foundation suite 為 241 passed。registry opt-in 另以 `make -f Makefile.cbm rust-pipeline-registry-optin-test` 驗證 53 個 registry tests；plan opt-in 另以 `make -f Makefile.cbm rust-pipeline-plan-optin-test` 驗證 `CBM_USE_RUST_PIPELINE_PLAN=1` runner 的 pipeline suite 209 passed；language graph parity 另以 `make -f Makefile.cbm rust-language-graph-parity` 驗證 default C path 與 `CBM_USE_RUST_PIPELINE_REGISTRY=1 CBM_USE_RUST_PIPELINE_PLAN=1` graph 完全一致。`scripts/lint.sh --ci` 已於 2026-07-02 通過：以對齊 CI 的 `cppcheck 2.20.0` + `clang-format 20.1.8` 驗證，並為本次重構觸及的 C/H 檔補上 clang-format-20 格式化；`rust-audit` 在缺工具時仍跳過，但 `RUST_AUDIT_REQUIRED=1` 會 fail-closed，本機已用 `cargo-audit v0.22.2` 驗證 `Cargo.lock`。
- CI 接入（2026-07-02）：新增 `.github/workflows/_rust.yml` 專責 Rust gates（lint / unit / FFI / foundation 與 pipeline registry+plan opt-in / language-graph parity），並於 `pr.yml` 納入 `rust` job 與 `ci-ok` 必要脈絡；`_security.yml` 既有的 `rust-dangerous-api` 與 `cargo-audit` fail-closed 維持不變。新增 `rust-ci-tests` make 目標作為 CI 用的測試/parity 子集。
- 剩餘範圍（刻意保留）：Phase 4 的 pipeline orchestration 全面遷移與 Phase 5 的「切換預設為 Rust-backed」尚未執行——Rust 端目前僅承接 registry/plan 決策層，實際 pass/extraction/tree-sitter/LSP 仍在 C；提前切換會破壞產品並違反本計畫「C fallback 需經至少一個 release cycle」的完成標準，詳見 `Tasks.md`。
- 下一批 foundation 評估：`platform` env/dirs 已完成 Rust ABI、C opt-in 與 parity fixture。`hash_table` 已補 C contract/stress tests，但因 borrowed key pointer、`get_key` stored pointer、NULL value 與 foreach callback contract，目前標記為 C-only hot path；若未來試作 Rust 版，先走 test-only `cbm_rs_ht_*` parity fixture。`diagnostics` 已補 deterministic query stats、env parser、path、JSON 與 NDJSON 的 Rust parity/FFI fixture；writer thread、檔案 rotation、system metrics 與 stderr lifecycle 仍留 C。Phase 3 的 store/cypher/MCP 已整理於 `docs/rust-refactor-phase3-plan.md`；`store_compat` contract fixture 已凍結 minimal schema/index existence、`edges.url_path_gen`、user index drop/create symmetry、`open_path_query` no-create/read-only、WAL journal mode、readonly WAL read/write rejection、generated column query plan、URL path API 的 project-scoped substring 行為，以及 `nodes_fts` 手動 rebuild/camelCase tokenization；同時修正 `idx_edges_url_path` 未被 drop 的不對稱。`cypher_contract` 已凍結 parser AST shape、OPTIONAL MATCH、UNION/UNION ALL、edge property、deterministic ordered rows、WITH/post-WHERE AST、explicit `LIMIT 0` 與 exact errors。`mcp` suite 已凍結 BM25 `file_pattern` 與 label boost rank/order contract；MCP golden 已凍結 alias、tool schema 與 indexed repo `query_graph` transcript。

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

目前產品預設核心仍位於 `src/` 的 C 程式碼，包含 `foundation/`、`store/`、`cypher/`、`mcp/`、`pipeline/`、`discover/`、`watcher/`、`cli/` 與 `ui/`。Rust workspace 已存在於 `rust/`，但只在明確 opt-in 下支援部分 foundation parity。tree-sitter 擷取、語言規格與 Hybrid LSP 邏輯位於 `internal/cbm/`。測試集中在 `tests/test_*.c` 與 `tests/repro/`。

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
