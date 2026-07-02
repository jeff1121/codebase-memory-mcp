# Rust Refactor Tasks

本清單依據 `Rust-Refactor.md` 拆解，用於追蹤 Rust 重構與驗證進度。

## Phase 0：建立基準

- [x] 建立 `docs/rust-refactor-baseline.md`，記錄現有 build/test/lint/security 基準。
- [x] 固定第一批 CLI、MCP、graph 統計、效能與安全驗證方向。
- [x] 補齊第一批可重跑 golden fixtures：CLI index/status/list、MCP initialize/tools list pagination、小型 fixture graph 統計。
- [x] 建立第一批小型 fixture 效能紀錄：binary size、index wall time、query wall time、child peak RSS。
- [x] 擴充第一批 CLI invocation golden：raw JSON args、stdin JSON、`--args-file`、flag form、`--json` envelope、非 `--json` 成功與錯誤輸出摘要。
- [x] 擴充第一批 MCP golden：`notifications/initialized` no-response、`tools/list` 兩頁 pagination、原始 tool 順序、response ids、tool key set、unknown method、parse error、unknown tool 與 Content-Length framed ping。
- [x] 擴充完整 MCP golden：alias、tool schema transcript、indexed repo tool-call transcript。
- [x] 建立第一批 SQLite/artifact golden fixture：`persistence=true` export、artifact metadata 摘要、`.gitattributes` 與 graph histogram。
- [x] 建立 artifact bootstrap/import golden fixture：從 `.codebase-memory/graph.db.zst` 啟動、schema mismatch 與匯入後 graph 等價。
- [x] 建立代表性 repository self-index baseline 手動 gate。
- [x] 建立大型效能基準。

## Phase 1：加入 Rust 骨架

- [x] 新增 root `Cargo.toml`、`Cargo.lock` 與 Rust workspace。
- [x] 新增 `rust/cbm-core` crate，先以 `staticlib` 提供 C opt-in 連結。
- [x] 新增 `rust/CBM_FFI.md`，定義 C/Rust 邊界、ownership 與錯誤規則。
- [x] 新增 `scripts/rust-check.sh`，整合 `cargo fmt`、`cargo clippy`、`cargo test` 與 FFI smoke test。
- [x] 在 `Makefile.cbm` 加入 `rust-*` targets，並保持產品 C binary 預設不連入 Rust。
- [x] 在 `.github/dependabot.yml` 加入 Cargo dependency 檢查入口。
- [x] 將 Rust dangerous API allowlist gate 接入 `_security.yml` 的 PR/release security workflow。
- [x] 將 `cargo-audit` 必要模式接入 `_security.yml`，CI 會安裝工具並以 `RUST_AUDIT_REQUIRED=1` fail-closed 執行。

## Phase 2：低風險 Foundation 遷移

- [x] 實作 Rust `dump_verify` parity 與 `CBM_USE_RUST_DUMP_VERIFY=1` opt-in。
- [x] 實作 Rust `str_intern` parity、C ABI 與 `CBM_USE_RUST_STR_INTERN=1` opt-in。
- [x] 實作 Rust `str_util` predicate / validator / `json_escape` path 與 `CBM_USE_RUST_STR_UTIL=1` opt-in。
- [x] 擴充 `CBM_USE_RUST_STR_UTIL=1`，涵蓋 `path_join`、`path_join_n`、`path_ext`、`path_base`、`path_dir`、`str_tolower`、`str_replace_char`、`str_strip_ext` 與 `str_split`，並保留 C arena ownership。
- [x] 實作 Rust `platform` 路徑正規化與 cgroup quota helpers，提供 `CBM_USE_RUST_PLATFORM_PATH=1`、`CBM_USE_RUST_PLATFORM_CGROUP=1` opt-in。
- [x] 實作 Rust `platform` env/dirs helpers，提供 `CBM_USE_RUST_PLATFORM_ENV_DIRS=1` opt-in，覆蓋 `cbm_safe_getenv`、home/config/local/cache 優先序、覆寫、截斷與路徑正規化。
- [x] 新增 `tests/test_rust_ffi.c`，覆蓋 null、embedded NUL、large length、buffer truncation 與 ABI contracts。
- [x] 新增 `rust-foundation-optin-test` matrix，驗證預設 C path、各單一 Rust opt-in（含 `CBM_USE_RUST_PLATFORM_ENV_DIRS=1`）與全 foundation opt-in。
- [x] 修正 parent-death watchdog 測試的啟動 race，避免完整測試在 Rust 步驟前誤失敗。
- [x] 遷移剩餘 `str_util` arena/path helpers，並以 C wrapper 保留 `CBMArena` 配置與 caller-visible contract。
- [x] 評估 `hash_table`、diagnostics、platform helpers 等下一批 foundation 模組。
- [x] 為下一批 foundation 模組建立 C/Rust parity fixture 與 opt-in strategy。
- [x] 補齊 `hash_table` contract tests：null args、content lookup、overwrite key pointer、NULL value、foreach userdata/exactly-once 與 high-bit C-string key；暫不加入產品 Rust opt-in。
- [x] 為 diagnostics 建立 deterministic query stats、env parser、path、JSON 與 NDJSON fixture；writer thread、rotation 與 stderr lifecycle 暫留 C。
- [x] 新增 diagnostics Rust parity module 與 test-only FFI fixture；hash_table 策略固定為 C-only hot path，未來 Rust 版需先走 test-only parity fixture。
- [x] 新增 `hash_table` Rust parity module（`foundation::hash_table`，純安全 Rust）與 test-only `cbm_rs_ht_*` FFI fixture，固定 content-key 比對、`get_key` 回傳 stored 指標（覆寫更新）、NULL value 為有效存在項、foreach exactly-once 與非 UTF-8 高位元 key；仍不導入產品 opt-in（hot path，維持 C-only）。

## Phase 3：核心服務邊界

- [x] 規劃並切分 `store` Rust 遷移，保留 SQLite schema、FTS5、WAL 與 fixture 相容。
- [x] 規劃並切分 `cypher` parser/query 遷移，建立 AST/result/error contract 而非誤用 SQL output。
- [x] 規劃並切分 `mcp` JSON-RPC 遷移，保留 tool schema、錯誤碼與 stdout/stderr 行為。
- [x] 新增第一批 `store` contract fixture：minimal schema/index existence、`edges.url_path_gen` generated column、user index drop/create symmetry。
- [x] 擴充 `store` contract fixture：`open_path_query` missing DB no-create、read-only open 與 read-only write rejection。
- [x] 擴充 `store` contract fixture：URL path API project-scoped substring 行為、`nodes_fts` manual rebuild 與 camelCase tokenization。
- [x] 擴充 `store` contract fixture：WAL journal mode、readonly WAL read/write rejection 與 generated column query plan/golden。
- [x] 新增第一批 `cypher` contract fixture：parser AST shape、deterministic ordered rows、exact unsupported CREATE error。
- [x] 擴充 `cypher` contract fixture：WITH/post-WHERE AST、explicit `LIMIT 0` columns/rows、list indexing exact error。
- [x] 擴充 `cypher` golden fixture：OPTIONAL MATCH、UNION、edge property、unknown function 與更多 exact error messages。
- [x] 擴充 `mcp` transcript fixture：`tools/list` pagination、原始 tool 順序、notification no-response、error code、unknown tool 與 Content-Length。
- [x] 擴充 `mcp` BM25 fixture：`search_graph` query path 的 `file_pattern`、label boost rank/order、`total` 與 `has_more` contract。
- [x] 擴充 `mcp` transcript fixture：alias、tool schema、indexed repo query。

## Phase 4：Pipeline 與 Extraction

- [x] 建立 registry name lookup / candidate selection 的 Rust parity module 與一次性 FFI fixture。
- [x] 將 `CBM_USE_RUST_PIPELINE_REGISTRY=1` opt-in 接入 `cbm_registry_resolve`，以 Rust opaque handle 承接 registry resolve hot path，並保留 C registry-owned return pointer contract。
- [x] 建立 Rust pipeline pass plan parity module 與 test-only FFI fixture，固定 full pipeline、sequential、parallel extraction、predump、incremental extract/resolve 與 incremental dump/persist tail 的 pass order、fast-mode gating 與 parallel threshold。
- [x] 將 `CBM_USE_RUST_PIPELINE_PLAN=1` opt-in 接入 full 與 incremental extraction phase，僅由 Rust plan 選擇 parallel/sequential，實際 pass function pointers 與資料 mutation 仍留 C。
- [x] 建立 language graph parity gate，固定代表性語言 definitions、calls、imports、routes、semantic/LSP edges，並比對 default C path 與 Rust registry/plan opt-in path。
- [x] 將 `CBM_USE_RUST_PIPELINE_PLAN=1` 擴充至 full pre-dump pass dispatch，由 Rust plan 決定 `decorator_tags`、`configlink`、`route_match`、`similarity`、`semantic_edges` 與 `complexity` 的順序與 fast-mode gating，實際 pass function pointers 與資料 mutation 仍留 C。
- [ ] 遷移 pipeline orchestration、pass registry 與資料流管理。
- [ ] 保留現有 tree-sitter grammar shim，再逐步替換周邊 glue code。
- [ ] 隨 extraction、tree-sitter 與 LSP glue 遷移持續擴充 language graph parity coverage。

## Phase 5：預設切換與清理

- [ ] Rust-backed binary 達到等價門檻後，切換預設 build path。
- [ ] 保留 C fallback 至少一個 release cycle。
- [ ] 清理已取代 C 模組前，驗證 packaging、install/update/uninstall、UI variant 與 security suite。
- [ ] 更新 `README.md`、`CONTRIBUTING.md`、release workflow 與 contributor guide。


## 完整移除 C 路線圖追蹤

- [ ] Foundation 剩餘葉模組：完成 `yaml`、`log`/`profile`、`compat_*`（`hash_table` test-only parity 已完成），以及 `arena`/`slab_alloc`/`mem`/`vmem` allocator contract/stress/fuzz gate。
- [ ] Pipeline orchestration 全面化：Rust 承接 incremental post-pass adapter、pass registry metadata、full/incremental dispatch 與 graph-buffer mutation 邊界；language graph parity 持續擴充。
- [ ] Store：補完整 SQLite schema/index/FTS/checkpoint/bulk/artifact golden，分段遷移 open/readonly/WAL、CRUD/search、FTS/BM25、vector 與 writer path。
- [ ] Cypher：補 CALL/EXISTS/malformed/aggregation exact golden，依序遷移 lexer/parser、AST normalizer、expression evaluator、projection/aggregation 與 executor。
- [ ] MCP：補 14 tools transcript 與錯誤路徑，依序遷移 JSON-RPC codec、tool schema generator、read-only handlers、狀態型 handlers 與 install/config 類 handler。
- [ ] `internal/cbm`：擴充 158 語言 graph parity，保留 C grammar shim 起步，逐語言遷移 extractor/helper，最後處理 Hybrid LSP timeout/fallback/process I/O。
- [ ] Phase 5 預設切換：全 Rust-backed opt-in matrix 達等價門檻後，建立 release candidate、通過 default/單一/全 opt-in/Rust-backed matrix，才切預設 build。
- [ ] C fallback release cycle：預設 Rust-backed 後保留 C fallback 至少一個 release cycle，文件標明回滾方式與相容期。
- [ ] 移除 C：逐子系統刪除已取代 C fallback、`CBM_USE_RUST_*` 過渡旗標與文件，最後處理 `internal/cbm`，並保留必要 vendored grammar C shim。
- [ ] 最終 gate：packaging、install/update/uninstall、UI variant、security suite、跨平台、效能 ≤10% 退化、binary size ≤20%、SQLite/artifact/MCP/CLI/language parity 全部通過。

## 驗證狀態

- [x] `scripts/rust-check.sh`
- [x] `make -f Makefile.cbm rust-ci`（本機未安裝 `cargo-audit` 時依設計跳過 audit）
- [x] `make -f Makefile.cbm rust-ffi-test`（2026-07-02：含 diagnostics 與 pipeline plan parity FFI smoke）
- [x] `make -f Makefile.cbm rust-foundation-optin-test`
- [x] `make -f Makefile.cbm test-foundation`（2026-07-02：default C path 241 passed）
- [x] `make -f Makefile.cbm CBM_USE_RUST_STR_UTIL=1 test-foundation`
- [x] `make -f Makefile.cbm CBM_USE_RUST_PLATFORM_PATH=1 test-foundation`
- [x] `make -f Makefile.cbm CBM_USE_RUST_PLATFORM_CGROUP=1 test-foundation`
- [x] `make -f Makefile.cbm CBM_USE_RUST_PLATFORM_ENV_DIRS=1 test-foundation`
- [x] `make -f Makefile.cbm CBM_USE_RUST_DUMP_VERIFY=1 CBM_USE_RUST_STR_INTERN=1 CBM_USE_RUST_STR_UTIL=1 CBM_USE_RUST_PLATFORM_PATH=1 CBM_USE_RUST_PLATFORM_CGROUP=1 CBM_USE_RUST_PLATFORM_ENV_DIRS=1 test-foundation`
- [x] `tests/test_parent_watchdog.sh` 連續 5 次通過
- [x] `scripts/test.sh` 完整重跑（2026-07-02 通過：完整 C runner、parent-death watchdog、security-string allow-list、Rust 單元測試、FFI smoke 與含 env/dirs 的 foundation opt-in matrix 均通過）
- [x] `scripts/build.sh`
- [x] `python3 scripts/rust-baseline-fixtures.py build/c/codebase-memory-mcp`
- [x] `make -f Makefile.cbm rust-baseline-fixtures`
- [x] `make -f Makefile.cbm build/c/test-runner && build/c/test-runner store_compat`（2026-07-02：8 passed）
- [x] `make -f Makefile.cbm build/c/test-runner && build/c/test-runner cypher_contract`（2026-07-02：13 passed）
- [x] `build/c/test-runner cypher cypher_contract`（2026-07-02：156 passed）
- [x] `make -f Makefile.cbm build/c/test-runner && build/c/test-runner mcp`（2026-07-02：119 passed）
- [x] `scripts/smoke-test.sh build/c/codebase-memory-mcp`
- [x] `scripts/smoke-invariants.sh build/c/codebase-memory-mcp`（30 passed / 0 failed）
- [x] `scripts/lint.sh --ci`（2026-07-02：以 CI 對齊版本 `cppcheck 2.20.0` + `clang-format 20.1.8` 驗證通過；本次已為重構檔補上 clang-format-20 格式化。full lint 的 `clang-tidy` 屬本機重型 gate，非 `--ci` 範圍）
- [x] `make -f Makefile.cbm security`（含預設 `rust-security`；本機 `cargo-audit` 缺工具時仍跳過）
- [x] Rust 相依套件稽核：`make -f Makefile.cbm RUST_AUDIT_REQUIRED=1 rust-audit`（2026-07-02 通過；以暫存 `cargo-audit v0.22.2` 掃描 `Cargo.lock` 的 1 個 crate dependency）
- [x] `make -f Makefile.cbm rust-dangerous-api`（2026-07-02：81 findings all allowlisted；已接入 `_security.yml`）
- [x] `make -f Makefile.cbm rust-security`（Rust source allowlist 通過；本機預設仍可跳過 `cargo-audit`，CI 以 `RUST_AUDIT_REQUIRED=1` 強制執行）
- [x] Rust unsafe/process/network/filesystem allowlist gate（不代表 Rust 相依套件稽核完成）
- [x] `cargo test --workspace --all-targets --all-features --locked`（2026-07-02：38 passed）
- [x] `cargo test -p cbm-core pipeline::registry --locked`（2026-07-02：3 passed）
- [x] `make -f Makefile.cbm build/c/test-runner && build/c/test-runner registry`（2026-07-02：default C path 52 passed）
- [x] `make -f Makefile.cbm rust-pipeline-registry-optin-test`（2026-07-02：`CBM_USE_RUST_PIPELINE_REGISTRY=1` registry suite 53 passed）
- [x] `make -f Makefile.cbm rust-pipeline-plan-optin-test`（2026-07-02：`CBM_USE_RUST_PIPELINE_PLAN=1` pipeline suite 209 passed；涵蓋 extraction choice 與 full pre-dump pass dispatch）
- [x] `make -f Makefile.cbm rust-language-graph-parity-update`（2026-07-02：已產生 `language-graph-parity.json`，default C path 與 Rust registry/plan opt-in path graph 完全一致）
- [x] `make -f Makefile.cbm rust-language-graph-parity`（2026-07-02：代表性語言 graph parity golden 驗證通過）
- [x] `build/c/test-runner pipeline`（2026-07-02：以 `CBM_USE_RUST_PIPELINE_REGISTRY=1` runner 執行，209 passed）
- [x] `git diff --check`
- [x] `make -f Makefile.cbm rust-self-index-baseline`
- [x] `make -f Makefile.cbm rust-large-performance-baseline`
- [x] `cargo test -p cbm-core foundation::diagnostics --locked`（2026-07-02：3 passed）
- [x] `cargo test -p cbm-core pipeline::plan --locked`（2026-07-02：5 passed）

## 本次工作階段（2026-07-02 收尾）

- [x] 解除 lint blocker：本機補齊 `cppcheck`、`clang-format`、`clang-tidy`（Homebrew LLVM）與 `cargo-audit`；並以 CI 對齊版本 `cppcheck 2.20.0` + `clang-format 20.1.8` 複驗 `scripts/lint.sh --ci` 通過。
- [x] 以 `clang-format 20`（對齊 CI 的 `clang-format-20`）格式化重構觸及的 28 個 C/H 檔（foundation、pipeline、store、mcp、discover 與新測試檔）；`make lint-format` 全 tree clean，`make lint-cppcheck`（2.20.0）全 tree clean。
- [x] CI 接入 Rust gates：新增 `.github/workflows/_rust.yml`（rust-lint / unit / FFI / foundation 與 pipeline registry+plan opt-in / language-graph parity），`pr.yml` 納入 `rust` job 與 `ci-ok`；新增 `rust-ci-tests` make 目標；`_lint.yml` 設 `CBM_SKIP_RUST=1`，Rust lint 改由專責 gate 執行。
- [x] README badge 列新增 `Rust core（experimental opt-in）` badge（保留 `Pure C`，因預設仍為純 C 建置）；`CONTRIBUTING.md` 新增「Rust Core（experimental, opt-in）」貢獻者章節。
- [x] 格式化後複驗：`scripts/build.sh`、`scripts/test.sh`、`make rust-lint rust-test rust-ffi-test rust-pipeline-registry-optin-test rust-pipeline-plan-optin-test rust-language-graph-parity rust-dangerous-api`、`make RUST_AUDIT_REQUIRED=1 rust-audit` 均通過（`rust-dangerous-api` 125 findings 全允許清單）。

### Phase 4/5 剩餘範圍（尚未完成，刻意保留）

- Phase 4 的 `pipeline orchestration` 全面遷移（items 72–74）屬多輪工作：目前 registry + plan 決策層、full/incremental extraction 選擇與 full pre-dump dispatch 已在 opt-in 下運作並有 parity gate；incremental post-pass 為異質簽章序列，需 adapter 才能由 plan dispatch，暫不強行導入以符合「簡單優先／外科手術式」原則。
- Phase 5 的「切換預設 build path 為 Rust-backed」（items 78–79）**刻意尚未執行**：Rust 端目前僅承接決策層，實際 pass/extraction/tree-sitter/LSP 仍在 C；提前切換會破壞產品，且違反本計畫「C fallback 需經至少一個 release cycle 驗證」的完成標準。item 80（清理前的 packaging/install/UI/security 驗證）為 Phase 5 清理的前置 gate，於切換前不觸發。item 81 的 README/CONTRIBUTING/contributor guide 與 CI 已於本階段更新，release workflow 待預設切換時再調整。

## 本次工作階段（2026-07-02 朝完整移除 C 推進）

- [x] 新增「完整移除 C 的路線圖」：`Rust-Refactor.md` 依子系統排序（foundation 剩餘 → pipeline → store → cypher → mcp → internal/cbm → 預設切換 → 移除）並附最終檢核表；`Tasks.md` 新增對應追蹤 checklist。誠實反映現況（預設 100% 純 C、Rust 約 1.6%、零預設 Rust 取代）。
- [x] 推進 foundation 下一切片：新增 `hash_table` Rust parity module（`rust/cbm-core/src/foundation/hash_table.rs`，純安全 Rust + 4 個單元測試）與 test-only `cbm_rs_ht_*` FFI（`ffi.rs`），並在 `tests/test_rust_ffi.c` 新增 parity 測試，固定 content-key、`get_key` stored 指標（覆寫更新）、NULL value 為存在項、foreach exactly-once、非 UTF-8 高位元 key、null 契約與 resize。維持 C-only hot path，不導入產品 opt-in。
- [x] 嚴謹驗證全綠：`scripts/rust-check.sh`（fmt、clippy `-D warnings`、`cargo test --workspace`、`rust-ffi-test`、foundation opt-in matrix、pipeline registry/plan opt-in、language-graph parity）、`make rust-dangerous-api`（148 findings 全允許清單，新 FFI 由 `ffi.rs` wildcard 覆蓋）、`scripts/lint.sh --ci`（CI 對齊 `cppcheck 2.20.0` + `clang-format 20.1.8`，含 `test_rust_ffi.c`）。
- 說明：此切片為 test-only parity（不改變預設 C 路徑），是既有方法論中 production opt-in 的前置步驟；離「可移除 C」仍需完成路線圖各階段。
