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
- [x] 建立 Rust pipeline pass plan parity module 與 test-only FFI fixture，固定 full pipeline、sequential、parallel extraction、predump、incremental extract/resolve 與 incremental post/tail typed order、fast-mode gating 與 parallel threshold。
- [x] 將 `CBM_USE_RUST_PIPELINE_PLAN=1` opt-in 接入 full 與 incremental extraction phase，僅由 Rust plan 選擇 parallel/sequential，實際 pass function pointers 與資料 mutation 仍留 C。
- [x] 建立 language graph parity gate，固定代表性語言 definitions、calls、imports、routes、semantic/LSP edges，並比對 default C path 與 Rust registry/plan opt-in path。
- [x] 將 `CBM_USE_RUST_PIPELINE_PLAN=1` 擴充至 full pre-dump pass dispatch，由 Rust plan 決定 `decorator_tags`、`configlink`、`route_match`、`similarity`、`semantic_edges` 與 `complexity` 的順序與 fast-mode gating，實際 pass function pointers 與資料 mutation 仍留 C。
- [x] 將 `CBM_USE_RUST_PIPELINE_PLAN=1` 擴充至 typed incremental post/tail dispatch，由 Rust typed plan 決定 `k8s`、`incr_tests`、`incr_decorator_tags`、`incr_configlink`、`incr_similarity`、`incr_semantic_edges`、`edge_relink`、`incremental_dump`、`persist_hashes` 與 `artifact_export` 的順序與 policy；C 端仍固定驗證 tail 順序並執行既有 C side effects。
- [x] 固定 full/fast predump pass trace contract：`CBM_USE_RUST_PIPELINE_PLAN=1` 會記錄 `rust_plan.dispatch phase=predump`，測試以 inline ordered trace 驗證 full 6 pass 與 fast 4 pass gating。
- [x] 新增 additive typed v2 predump metadata ABI：Rust 回傳 predump `kind/phase/policy/gate_flags/requires_mask/effect_flags`，C 端逐欄驗證本地 dispatch table 後以 `source=typed_v2` 執行既有 pass；字串 plan ABI 與 C fallback 保留。
- [x] 新增 additive typed v2 sequential extraction metadata ABI：Rust 回傳 `definitions`、`k8s`、`lsp_cross`、`calls`、`usages` 與 `semantic` 的 `kind/phase/policy/gate_flags/requires_mask/effect_flags`，C 端逐欄驗證後以 `source=typed_v2` 執行既有 pass；`infra_routes` / `infra_bindings` after-success tail 保留 C 控制。
- [x] 新增 additive typed v2 parallel extraction metadata ABI：Rust 回傳 `parallel_extract`、`registry_build`、`lsp_cross_prepare`、`parallel_resolve`、`infra_routes`、`infra_bindings` 與 `k8s` 的 `kind/phase/policy/gate_flags/requires_mask/effect_flags`，C 端逐欄驗證後記錄 `source=typed_v2` 外層 trace；cache、shared ids、cross-LSP cleanup、infra 實際執行與錯誤傳遞仍保留 C 控制。
- [x] 新增 additive typed v2 incremental extract/resolve metadata ABI：Rust 回傳 sequential `definitions`、`calls`、`usages`、`semantic` 或 parallel `incr_extract`、`incr_registry`、`incr_resolve` 的 `kind/phase/policy/gate_flags/requires_mask/effect_flags`，C 端逐欄驗證後依 typed steps dispatch；實際 extract、registry build、resolve、cache、shared ids 與 cross-LSP prebuild skip 仍保留 C 控制。
- [x] 收斂 graph-buffer mutation 的 test-only FFI command 邊界：Rust metadata/validation 固定 `UpsertNode`、`InsertEdge` 與 `DeleteByFile` 的 required/optional fields、result kind、effect flags、reserved bits、empty string 與 null contract；實際 graph-buffer mutation、storage 與 ownership 仍由 C 執行。
- [x] 新增 C-side graph-buffer mutation command adapter：`cbm_gbuf_apply_mutation` 與 `cbm_gbuf_apply_*` wrappers 同步 dispatch 到既有 C `cbm_gbuf_*` API，並將 full structure pass 與 incremental purge/file-upsert 的代表性呼叫點改走 adapter；不新增 Rust production opt-in，不讓 Rust 持有 `cbm_gbuf_t`。
- [x] 將 `TESTS` / `TESTS_FILE` edge creation 改走 C-side `cbm_gbuf_apply_insert_edge` adapter，保留既有 edge dedup 與 count 行為。
- [x] 將 sequential usage edges、configlink edges 與 incremental inbound restore edge 改走 C-side `cbm_gbuf_apply_insert_edge` adapter，保留既有 dedup、回傳忽略與計數語意。
- [x] 將 `pass_similarity.c` 的 `SIMILAR_TO`、`pass_semantic_edges.c` 的 `SEMANTICALLY_RELATED` edge creation 與 `pass_enrichment.c` 的 decorator-tag node upsert 改走 C-side graph-buffer mutation adapter，保留 properties、dedup、edge count/total_edges 與 node update 語意；static guard 已納入這三個檔案。
- [x] 固定 incremental dump failure side-effect contract：dump 失敗會原樣傳回錯誤，不再落入 full fallback，且會跳過後續 `persist_hashes` / `artifact_export` tail。
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
- [ ] Pipeline orchestration 全面化：Rust 承接 incremental post-pass adapter、pass registry metadata、full/incremental dispatch 與 graph-buffer mutation 邊界（command contract 與多批 C-side adapter 呼叫點已固定，完整資料流仍未完成）；language graph parity 持續擴充。
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
- [x] `make -f Makefile.cbm rust-ffi-test`（2026-07-04：含 diagnostics、pipeline plan parity、typed sequential/incremental/predump/parallel extraction v2 metadata、PlanStepV2 ABI layout、typed incremental post step FFI smoke，以及 graph-buffer mutation metadata/validation ABI；C smoke 共用 `graph_buffer.h` command struct/constants，並鎖定 unknown+reserved validation 優先序）
- [x] `make -f Makefile.cbm rust-foundation-optin-test`
- [x] `make -f Makefile.cbm test-foundation`（2026-07-02：default C path 241 passed）
- [x] `make -f Makefile.cbm CBM_USE_RUST_STR_UTIL=1 test-foundation`
- [x] `make -f Makefile.cbm CBM_USE_RUST_PLATFORM_PATH=1 test-foundation`
- [x] `make -f Makefile.cbm CBM_USE_RUST_PLATFORM_CGROUP=1 test-foundation`
- [x] `make -f Makefile.cbm CBM_USE_RUST_PLATFORM_ENV_DIRS=1 test-foundation`
- [x] `make -f Makefile.cbm CBM_USE_RUST_DUMP_VERIFY=1 CBM_USE_RUST_STR_INTERN=1 CBM_USE_RUST_STR_UTIL=1 CBM_USE_RUST_PLATFORM_PATH=1 CBM_USE_RUST_PLATFORM_CGROUP=1 CBM_USE_RUST_PLATFORM_ENV_DIRS=1 test-foundation`
- [x] `tests/test_parent_watchdog.sh` 連續 5 次通過
- [x] `scripts/test.sh` 完整重跑（2026-07-02 通過：完整 C runner、parent-death watchdog、security-string allow-list、Rust 單元測試、FFI smoke 與含 env/dirs 的 foundation opt-in matrix 均通過）
- [ ] `scripts/test.sh` 完整重跑（2026-07-05 恢復後曾啟動但依使用者要求中斷；不視為通過，最近完整通過仍為 2026-07-02）
- [x] `scripts/build.sh`
- [x] `python3 scripts/rust-baseline-fixtures.py build/c/codebase-memory-mcp`
- [x] `make -f Makefile.cbm rust-baseline-fixtures`
- [x] `make -f Makefile.cbm build/c/test-runner && build/c/test-runner store_compat`（2026-07-02：8 passed）
- [x] `make -f Makefile.cbm build/c/test-runner && build/c/test-runner cypher_contract`（2026-07-02：13 passed）
- [x] `build/c/test-runner cypher cypher_contract`（2026-07-02：156 passed）
- [x] `make -f Makefile.cbm build/c/test-runner && build/c/test-runner mcp`（2026-07-02：119 passed）
- [x] `scripts/smoke-test.sh build/c/codebase-memory-mcp`
- [x] `scripts/smoke-invariants.sh build/c/codebase-memory-mcp`（30 passed / 0 failed）
- [x] `scripts/lint.sh --ci`（2026-07-05 通過；以 CI 對齊版本 `cppcheck 2.20.0` + `clang-format 20.1.8` 驗證。full lint 的 `clang-tidy` 屬本機重型 gate，非 `--ci` 範圍）
- [x] `make -f Makefile.cbm security`（含預設 `rust-security`；本機 `cargo-audit` 缺工具時仍跳過）
- [x] Rust 相依套件稽核：`make -f Makefile.cbm RUST_AUDIT_REQUIRED=1 rust-audit`（2026-07-02 通過；以暫存 `cargo-audit v0.22.2` 掃描 `Cargo.lock` 的 1 個 crate dependency）
- [x] `make -f Makefile.cbm rust-dangerous-api`（2026-07-02：81 findings all allowlisted；已接入 `_security.yml`）
- [x] `make -f Makefile.cbm rust-security`（Rust source allowlist 通過；本機預設仍可跳過 `cargo-audit`，CI 以 `RUST_AUDIT_REQUIRED=1` 強制執行）
- [x] Rust unsafe/process/network/filesystem allowlist gate（不代表 Rust 相依套件稽核完成）
- [x] `cargo test --workspace --all-targets --all-features --locked`（2026-07-04：46 passed）
- [x] `cargo test -p cbm-core pipeline::registry --locked`（2026-07-02：3 passed）
- [x] `make -f Makefile.cbm build/c/test-runner && build/c/test-runner registry`（2026-07-02：default C path 52 passed）
- [x] `make -f Makefile.cbm rust-pipeline-registry-optin-test`（2026-07-02：`CBM_USE_RUST_PIPELINE_REGISTRY=1` registry suite 53 passed）
- [x] `CBM_USE_RUST_PIPELINE_PLAN=1 build/c/test-runner pipeline`（2026-07-04：pipeline suite 217 passed；涵蓋 extraction choice、sequential extraction typed v2 metadata/trace、incremental extract/resolve typed v2 dispatch/trace、parallel extraction typed v2 metadata/trace、full pre-dump typed v2 metadata/trace、typed incremental post/tail dispatch 與 incremental dump failure propagation）
- [x] `make -f Makefile.cbm rust-language-graph-parity-update`（2026-07-02：已產生 `language-graph-parity.json`，default C path 與 Rust registry/plan opt-in path graph 完全一致）
- [x] `make -f Makefile.cbm rust-language-graph-parity`（2026-07-04：代表性語言 graph parity golden 驗證通過，未更新 golden）
- [x] `build/c/test-runner pipeline`（2026-07-04：default C path，217 passed）
- [x] `git diff --check`
- [x] `make -f Makefile.cbm rust-self-index-baseline`
- [x] `make -f Makefile.cbm rust-large-performance-baseline`
- [x] `cargo test -p cbm-core foundation::diagnostics --locked`（2026-07-02：3 passed）
- [x] `cargo test -p cbm-core pipeline::plan --locked`（2026-07-04：11 passed，含 typed sequential/incremental/predump/parallel extraction v2 metadata）
- [x] `cargo test -p cbm-core pipeline::graph_mutation --locked`（2026-07-04：3 passed，固定 graph-buffer mutation command metadata/validation）
- [x] `make -f Makefile.cbm build/c/test-runner` + `build/c/test-runner graph_buffer`（2026-07-04：51 passed；覆蓋 graph-buffer mutation adapter，並以 static guard 鎖定 `pipeline.c`、`pipeline_incremental.c`、`pass_tests.c`、`pass_usages.c` 與 `pass_configlink.c` 的代表性 adapter call points）
- [x] `build/c/test-runner pipeline`（2026-07-04：default C path，217 passed；覆蓋 cleanup、fallback trace、typed post trace、incremental dump failure 與 hash/FTS tail contract）
- [x] `make -f Makefile.cbm rust-pipeline-plan-optin-test`（2026-07-04：`CBM_USE_RUST_PIPELINE_PLAN=1` pipeline suite 217 passed）
- [x] `make -f Makefile.cbm rust-language-graph-parity`（2026-07-04：default C path 與 `CBM_USE_RUST_PIPELINE_REGISTRY=1 CBM_USE_RUST_PIPELINE_PLAN=1` golden 一致，未更新 `tests/golden/rust-refactor`）
- [x] 格式與 golden hygiene（2026-07-04）：`cargo fmt --all -- --check`、`make -f Makefile.cbm lint-format`、`git diff --check`、`git diff --exit-code -- tests/golden/rust-refactor`
- [x] 2026-07-05 adapter targeted gates：`make -f Makefile.cbm clean-c build/c/test-runner`、`build/c/test-runner graph_buffer pipeline`（268 passed）、`make -f Makefile.cbm rust-pipeline-plan-optin-test`（217 passed）、`make -f Makefile.cbm rust-language-graph-parity`、`scripts/lint.sh --ci`、`cargo fmt --all -- --check`、`make -f Makefile.cbm lint-format`、`git diff --check`、`git diff --exit-code -- tests/golden/rust-refactor`

## 本次工作階段（2026-07-05 Phase 4 adapter callpoint 收斂）

- [x] 將 `pass_similarity.c` 的 deferred `SIMILAR_TO` merge 改走 `cbm_gbuf_apply_insert_edge`，保留 jaccard/same_file props、dedup 與 total edge count 語意。
- [x] 將 `pass_semantic_edges.c` 的 deferred `SEMANTICALLY_RELATED` merge 改走 `cbm_gbuf_apply_insert_edge`，保留 score/same_file props、worker buffer merge 與 total_edges 語意。
- [x] 將 `pass_enrichment.c` 的 decorator-tag node update 改走 `cbm_gbuf_apply_upsert_node`，保留原本 properties injection 與 node update 語意。
- [x] 更新 graph-buffer adapter static guard，鎖定 `pass_similarity.c`、`pass_semantic_edges.c`、`pass_enrichment.c` 不退回代表性 direct `cbm_gbuf_*` call。
- [x] 驗證通過：`build/c/test-runner graph_buffer pipeline`（268 passed）、`make -f Makefile.cbm rust-pipeline-plan-optin-test`（217 passed）、`make -f Makefile.cbm rust-language-graph-parity`、`scripts/lint.sh --ci` 與格式/golden hygiene。
- [ ] 完整 `scripts/test.sh` 未於本輪完成：2026-07-05 曾啟動但依使用者要求中斷；`make -f Makefile.cbm security` 亦尚未針對本批 adapter 變更重跑。

## 本次工作階段（2026-07-04 Phase 4 ABI / adapter / side-effect 收斂）

- [x] 將 graph-buffer mutation command constants 與 `cbm_gbuf_mutation_cmd_t` 放在 `src/graph_buffer/graph_buffer.h`，讓 C adapter 與 FFI smoke 共用 ABI 來源；補上 borrowed C string、scalar slot、endpoint lookup、effect flag 與 dedup 回傳語意說明。
- [x] 補 `cbm_rs_pipeline_plan_step_t` v1 layout static assert，避免 typed incremental post ABI 漂移；`requires_mask` 測試改用 `uint64_t` 比對。
- [x] 補 `rust_plan.fallback` trace：predump、sequential extraction、parallel extraction、incremental extract/resolve 與 incremental post 的 typed metadata/steps 不可用時皆有 phase/reason/path。
- [x] 修正 incremental run-level cleanup：incremental 成功或錯誤都會走共用 cleanup，避免早退跳過 userconfig、pkgmap、gbuf、registry 與 path alias cleanup。
- [x] 收斂 incremental dump failure contract：dump 失敗保留原 DB/hash，跳過 `persist_hashes` / `artifact_export` tail，第二次清除 fault 後仍可重新偵測並寫入 probe；typed incremental post trace 明確標 `source=typed_steps`。
- [x] `persist_hashes` / FTS tail 錯誤傳遞範圍收斂為 DB open 與 FTS rebuild hard error；單筆 hash upsert 仍依現有程式註解維持 warning/best-effort，不宣稱為 hard error。
- [x] 補 adapter static guard：`pipeline.c` structure adapter、`pipeline_incremental.c` purge/file-upsert/inbound restore、`pass_tests.c`、`pass_usages.c` 與 `pass_configlink.c` 代表性呼叫點不可退回直接底層 call。

## 本次工作階段（2026-07-04 Phase 4 usage/configlink/incremental adapter 收斂）

- [x] 將 `pass_usages.c` 的 `USAGE`、`THROWS`/`RAISES` 與 `READS`/`WRITES` edge creation 改走 C-side `cbm_gbuf_apply_insert_edge` adapter，不改變既有回傳忽略與 resolved 計數語意。
- [x] 將 `pass_configlink.c` 的 key-symbol 與 dependency-import `CONFIGURES` edge creation 改走 C-side `cbm_gbuf_apply_insert_edge` adapter，保留 confidence props 與 edge_count 語意。
- [x] 將 `pipeline_incremental.c` 的 inbound edge restore 改走 C-side `cbm_gbuf_apply_insert_edge` adapter，保留找到 src/tgt 即 `restored++` 的既有語意。
- [x] 同步文件現況：Rust 目前已涵蓋 foundation 與 pipeline opt-in 切片；`rust/cbm-core/src` 約 4.6K 行 / 13 檔，仍非預設 Rust-backed binary。
- [x] 驗證通過：`build/c/test-runner graph_buffer`（51 passed）、`build/c/test-runner pipeline`（217 passed）、`make -f Makefile.cbm rust-pipeline-plan-optin-test`（pipeline suite 217 passed）、`make -f Makefile.cbm rust-language-graph-parity`（golden 一致，未更新）。

## 本次工作階段（2026-07-04 Phase 4 typed incremental extract/resolve v2 dispatch）

- [x] 新增 Rust `IncrementalExtractResolve` typed v2 metadata：sequential path 固定 `definitions`、`calls`、`usages`、`semantic`；parallel path 固定 `incr_extract`、`incr_registry`、`incr_resolve`，並以 gate flag 標示 incremental parallel resolve 不做 cross-LSP prebuild。
- [x] 擴充 `cbm_rs_pipeline_plan_step_count_v2` / `cbm_rs_pipeline_plan_steps_v2` 的 `kind=3` 行為；C FFI smoke 覆蓋 sequential/parallel count、phase、policy、gate、requires mask、effect flags、null 與 short cap。
- [x] 將 `CBM_USE_RUST_PIPELINE_PLAN=1` incremental extract/resolve path 加入 typed v2 metadata 驗證、trace 與 dispatch；C 端逐欄驗證後依 Rust typed steps 執行 sequential `definitions`/`calls`/`usages`/`semantic` 或 parallel `incr_extract`/`incr_registry`/`incr_resolve`，實際 C pass/helper、cache/shared id、registry build、resolve 與 LSP skip 語意仍留在 C。
- [x] 補強 incremental extract/resolve trace oracle：sequential path 驗證 dispatch order、`changed=1`、新增節點、CALLS/USAGE edge、FTS 與 artifact import；parallel path 以 52 個新增 Go 檔穩定觸發 `incr_extract -> incr_registry -> incr_resolve`，並驗證 probe 節點、CALLS/USAGE edge 與 `no_cross_lsp_prebuild` skip log。
- [x] 修正 typed dispatch fallback：Rust extraction choice 只用於成功的 typed dispatch；若 typed v2 不可用，fallback 回原始 C heuristic 並記錄 `rust_plan.fallback phase=incremental_extract_resolve`。
- [x] 將 `pass_tests.c` 的 `TESTS` / `TESTS_FILE` edge creation 改走 C-side `cbm_gbuf_apply_insert_edge` adapter，保留既有無條件計數與 edge dedup 行為。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::plan --locked`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm build/c/test-runner && build/c/test-runner graph_buffer lang_contract edge_structural pipeline`、`make -f Makefile.cbm rust-pipeline-plan-optin-test`；最新計數見「驗證狀態」。

## 本次工作階段（2026-07-04 Phase 4 C-side graph-buffer mutation adapter）

- [x] 新增 `src/graph_buffer/graph_buffer_mutation.c` 與 `cbm_gbuf_mutation_cmd_t`，以同步 command adapter 包住 `cbm_gbuf_upsert_node`、`cbm_gbuf_insert_edge` 與 `cbm_gbuf_delete_by_file`；回傳語意沿用底層 C API。
- [x] 將 full structure pass 的 Project/Branch/Folder/File upsert 與 HAS_BRANCH/CONTAINS_FOLDER/CONTAINS_FILE edge insert 改走 `cbm_gbuf_apply_*` wrappers。
- [x] 將 incremental purge 的 changed/deleted file delete-by-file 與 changed File node upsert 改走 `cbm_gbuf_apply_*` wrappers，保留 edge snapshot -> purge -> registry seed -> changed file node upsert 的既有順序。
- [x] 新增 graph-buffer adapter tests，覆蓋 upsert/edge/delete-by-file command apply、cascade edge、unknown kind、reserved bit、null command、empty QN 與 required string 缺漏。
- [x] 驗證通過：`make -f Makefile.cbm build/c/test-runner`、`build/c/test-runner graph_buffer`、`build/c/test-runner pipeline`；最新計數見「驗證狀態」。

## 本次工作階段（2026-07-04 Phase 4 graph-buffer mutation command boundary）

- [x] 新增 Rust `pipeline::graph_mutation` 純 metadata/validation 模組，只描述 `UpsertNode`、`InsertEdge` 與 `DeleteByFile`；不持有 `cbm_gbuf_t`，不呼叫 C graph-buffer，也不解析 JSON。
- [x] 新增 test-only FFI：`cbm_rs_gbuf_mutation_command_count_v1`、`cbm_rs_gbuf_mutation_commands_v1` 與 `cbm_rs_gbuf_mutation_validate_v1`；C smoke 鎖定 `MetaV1`、`CmdV1`、`ValidationV1` ABI layout。
- [x] 固定 command contract：empty QN 有效、null QN / null edge type / null file path 失敗、orphan edge endpoints 不做 lookup、reserved bits 必須為 0、optional null C string 只標記 normalized flag。
- [x] 維持後續邊界：vector storage、merge、dump/store writer、shared id lifecycle 與 graph-buffer storage/ownership 仍留 C，等待後續獨立切片。
- [x] 驗證通過：`cargo test -p cbm-core pipeline::graph_mutation --locked`（3 passed）、`make -f Makefile.cbm rust-ffi-test`。

## 本次工作階段（2026-07-04 Phase 4 typed parallel extraction v2 metadata）

- [x] 新增 Rust parallel extraction `PlanStepV2` metadata：`parallel_extract`、`registry_build`、`lsp_cross_prepare`、`parallel_resolve`、`infra_routes`、`infra_bindings` 與 `k8s` 以 stable kind、`ParallelExtract` phase、required/env_optional/ignore_err policy、requires bitset 與 graph mutation effect 表達；既有 `describe(PlanKind::ParallelExtraction)` 字串 ABI 保持不變。
- [x] 擴充 `cbm_rs_pipeline_plan_step_count_v2` / `cbm_rs_pipeline_plan_steps_v2` additive FFI：`kind=5` parallel extraction 現在回傳 7 個 outer steps；C smoke 覆蓋 `uint64_t` requires bitset、`env_optional` policy、effect flags、short cap 與 PlanStepV2 layout。
- [x] 將 `CBM_USE_RUST_PIPELINE_PLAN=1` 的 parallel path 加入 typed v2 metadata 驗證與 trace：C 端逐欄驗證本地 `parallel_passes[]` 的 kind、phase、policy、gate、requires、effect 與順序，驗證成功時記錄 `rust_plan.dispatch phase=parallel_extract ... source=typed_v2`；驗證失敗時仍沿用既有 C hard-coded path。
- [x] 新增 pipeline trace contract：測試建立 52 個 Go fixture 並暫設 `CBM_WORKERS=2`，穩定觸發 `pipeline.mode mode=parallel`，再以 ordered trace 驗證 `parallel_extract -> registry_build -> lsp_cross_prepare -> parallel_resolve -> k8s` 的 pass timing 與完整 7-step dispatch log；infra routes/bindings 只由 dispatch log 固定，不新增假 timing。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::plan --locked`、`cargo test --workspace --all-targets --all-features --locked`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm rust-pipeline-plan-optin-test`、`build/c/test-runner pipeline`、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm lint-format`、`make -f Makefile.cbm rust-language-graph-parity`、`git diff --check`、`git diff --exit-code -- tests/golden/rust-refactor`；最新計數見「驗證狀態」。

## 本次工作階段（2026-07-04 Phase 4 typed sequential extraction v2 metadata）

- [x] 新增 Rust sequential `PlanStepV2` metadata：`definitions`、`k8s`、`lsp_cross`、`calls`、`usages` 與 `semantic` 以 stable kind、`SequentialExtract` phase、required/ignore_err policy、`requires_result_cache` gate、requires bitset 與 graph mutation effect 表達；既有 `describe(PlanKind::Sequential)` 字串 ABI 保持不變並仍包含 infra after-success tail。
- [x] 擴充 `cbm_rs_pipeline_plan_step_count_v2` / `cbm_rs_pipeline_plan_steps_v2` additive FFI：`kind=0` sequential 現在回傳 6 個 dispatch steps；`kind=1` predump 與 `kind=5` parallel extraction 行為不變，其他 valid kind 仍回 `-1`。
- [x] 將 `CBM_USE_RUST_PIPELINE_PLAN=1` 的 sequential extraction adapter 改為優先消費 typed v2 metadata；C 端逐欄驗證本地 `seq_passes[]` 的 kind、phase、policy、gate、requires、effect 與順序，失敗時退回既有固定 C dispatch。
- [x] 新增 pipeline trace contract：`rust_plan.dispatch phase=sequential_extract ... source=typed_v2`，並以 ordered trace 驗證 `definitions -> k8s -> lsp_cross -> calls -> usages -> semantic`；infra routes/bindings tail 仍由 C 端在 `rc == 0 && seq_cache` 時執行。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::plan --locked`（8 passed）、`cargo test --workspace --all-targets --all-features --locked`（45 passed）、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm rust-pipeline-plan-optin-test`（pipeline suite 215 passed）、`build/c/test-runner pipeline`（default C path，215 passed）、`make -f Makefile.cbm rust-language-graph-parity`、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`git diff --check`、`git diff --exit-code -- tests/golden/rust-refactor`。

## 本次工作階段（2026-07-03 Phase 4 typed predump v2 metadata）

- [x] 新增 Rust `PlanStepV2` metadata：predump steps 以 stable kind、phase、required policy、fast skip gate、requires bitset 與 graph mutation effect 表達；既有 `describe(PlanKind::Predump)` 字串由 typed metadata render，保持 v1 字串 ABI。
- [x] 新增 `cbm_rs_pipeline_plan_step_count_v2` / `cbm_rs_pipeline_plan_steps_v2` additive FFI；當時先支援 `kind=1` predump，C smoke 覆蓋 full/moderate/advanced 6 steps、fast 4 steps、欄位值、unsupported kind、null、短 cap 與 negative cap。
- [x] 將 `CBM_USE_RUST_PIPELINE_PLAN=1` 的 full predump adapter 改為優先消費 typed v2 metadata；C 端逐欄驗證本地 `predump_passes[]` 的 kind、phase、policy、gate、requires、effect 與 FAST-only gating，失敗時仍退回既有字串 decoder，再退回 C fallback。
- [x] 更新 pipeline trace contract，要求 `rust_plan.dispatch phase=predump ... source=typed_v2`，並維持 full 6 pass、fast 4 pass 與 skip `similarity` / `semantic_edges` 的 ordered trace。
- [x] 驗證通過：`cargo test -p cbm-core pipeline::plan --locked`（7 passed）、`cargo test --workspace --all-targets --all-features --locked`（44 passed）、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm rust-pipeline-plan-optin-test`（pipeline suite 214 passed）、`build/c/test-runner pipeline`（default C path，214 passed）、`make -f Makefile.cbm rust-language-graph-parity`、`cargo fmt --all -- --check`、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`git diff --check`、`git diff --exit-code -- tests/golden/rust-refactor`。

## 本次工作階段（2026-07-03 Phase 4 typed incremental post tail / trace / error contract）

- [x] 將 Rust `pipeline::plan` 的 incremental post plan 改為 typed `PlanStep` / `PlanStepPolicy` 來源，既有 `describe()` 字串由 typed steps render，保留字串 ABI contract。
- [x] 新增 `cbm_rs_pipeline_incremental_post_step_count` / `cbm_rs_pipeline_incremental_post_steps` typed FFI，固定 stable numeric kind/policy，並以 C FFI smoke 覆蓋 count、tail policy、null 與 cap 不足。
- [x] 將 `CBM_USE_RUST_PIPELINE_PLAN=1` incremental adapter 改用 typed steps 驅動完整 post/tail dispatch；typed decoder 會驗證 `edge_relink -> incremental_dump -> persist_hashes -> artifact_export` 的固定 tail 順序與 policy，實際 C pass、dump、hash persist、FTS rebuild 與 artifact export side effects 仍由既有 C 函式執行。
- [x] 更新 pipeline opt-in 測試，驗證 `rust_plan.dispatch phase=incremental_post passes=10 tail=edge_relink,incremental_dump,persist_hashes,artifact_export`，並以 DB node、`file_hashes`、`nodes_fts` 與 artifact import oracle 覆蓋 dump、persist、FTS rebuild 與 existing artifact export。
- [x] 補上 incremental `persistence=true` 無既有 artifact 仍匯出 artifact 的資料面測試，import artifact 後確認新增函式存在，避免 issue #434 回歸。
- [x] 固定 full pre-dump Rust plan trace：新增 `rust_plan.dispatch phase=predump` log，並以 full/fast inline ordered trace 驗證 `decorator_tags -> configlink -> route_match -> similarity -> semantic_edges -> complexity` 與 fast skip 行為。
- [x] 收斂 predump C decoder：拒絕缺漏、重複、錯序或 fast-mode 不合法的 Rust predump plan，失敗時回到既有 C fallback。
- [x] 修正 incremental error propagation：以 `PL_INCREMENTAL_FALLBACK` 區分「刻意 full fallback」與真實 incremental rc，dump failure 不再被 full reindex 吃掉。
- [x] 補上 incremental dump failure regression：用 narrow fault hook 驗證 dump `-37` 會回傳、記錄 `phase=incremental_dump`、跳過 `persist_hashes` / `artifact_export` tail，且不記錄 `incremental.done`。
- [x] 驗證通過：`cargo test -p cbm-core pipeline::plan --locked`（6 passed）、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner pipeline`（default C path，214 passed）、`CBM_USE_RUST_PIPELINE_PLAN=1 build/c/test-runner pipeline`（214 passed）、`make -f Makefile.cbm rust-language-graph-parity`、`cargo fmt --all -- --check`、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`git diff --check`、`git diff --exit-code -- tests/golden/rust-refactor`。

## 本次工作階段（2026-07-02 收尾）

- [x] 解除 lint blocker：本機補齊 `cppcheck`、`clang-format`、`clang-tidy`（Homebrew LLVM）與 `cargo-audit`；並以 CI 對齊版本 `cppcheck 2.20.0` + `clang-format 20.1.8` 複驗 `scripts/lint.sh --ci` 通過。
- [x] 以 `clang-format 20`（對齊 CI 的 `clang-format-20`）格式化重構觸及的 28 個 C/H 檔（foundation、pipeline、store、mcp、discover 與新測試檔）；`make lint-format` 全 tree clean，`make lint-cppcheck`（2.20.0）全 tree clean。
- [x] CI 接入 Rust gates：新增 `.github/workflows/_rust.yml`（rust-lint / unit / FFI / foundation 與 pipeline registry+plan opt-in / language-graph parity），`pr.yml` 納入 `rust` job 與 `ci-ok`；新增 `rust-ci-tests` make 目標；`_lint.yml` 設 `CBM_SKIP_RUST=1`，Rust lint 改由專責 gate 執行。
- [x] README badge 列新增 `Rust core（experimental opt-in）` badge（保留 `Pure C`，因預設仍為純 C 建置）；`CONTRIBUTING.md` 新增「Rust Core（experimental, opt-in）」貢獻者章節。
- [x] 格式化後複驗：`scripts/build.sh`、`scripts/test.sh`、`make rust-lint rust-test rust-ffi-test rust-pipeline-registry-optin-test rust-pipeline-plan-optin-test rust-language-graph-parity rust-dangerous-api`、`make RUST_AUDIT_REQUIRED=1 rust-audit` 均通過（`rust-dangerous-api` 125 findings 全允許清單）。

### Phase 4/5 剩餘範圍（尚未完成，刻意保留）

- Phase 4 未完成任務 `Pipeline orchestration 全面化` 仍屬多輪工作：目前 registry + plan 決策層、full/incremental extraction 選擇、sequential extraction typed v2 metadata/trace、incremental extract/resolve typed v2 dispatch/trace、parallel extraction typed v2 metadata/trace、full pre-dump typed v2 metadata/trace、typed incremental post/tail dispatch、graph-buffer mutation command contract、多批 C-side adapter 呼叫點（structure、incremental purge/file-upsert/inbound restore、`TESTS`/`TESTS_FILE`、`pass_usages.c` usage/throws/read-write edges、`pass_configlink.c` key-symbol/dependency-import configlink edges），以及 incremental dump failure side-effect contract 已在 opt-in/default path 下驗證；C 端仍固定驗證 infra、dump、hash persist、FTS rebuild 與 artifact tail 順序，實際 side effects 與 graph-buffer 寫入仍由既有 C 函式執行，暫不讓 Rust 重排具副作用步驟以符合「簡單優先／外科手術式」原則。
- Phase 5 未完成任務 `切換預設 build path 為 Rust-backed` **刻意尚未執行**：Rust 端目前承接決策層與 graph-buffer mutation command contract，C 端僅有同步 adapter 呼叫點；實際 pass/extraction/tree-sitter/LSP 與 graph-buffer 寫入仍在 C；提前切換會破壞產品，且違反本計畫「C fallback 需經至少一個 release cycle 驗證」的完成標準。`切換前 packaging/install/UI/security/perf 驗證` 為 Phase 5 清理前置 gate，於切換前不觸發；README/CONTRIBUTING/contributor guide 與 CI 已於本階段更新，release workflow 待預設切換時再調整。

## 本次工作階段（2026-07-02 朝完整移除 C 推進）

- [x] 新增「完整移除 C 的路線圖」：`Rust-Refactor.md` 依子系統排序（foundation 剩餘 → pipeline → store → cypher → mcp → internal/cbm → 預設切換 → 移除）並附最終檢核表；`Tasks.md` 新增對應追蹤 checklist。誠實反映現況（預設 100% 純 C、Rust 仍只佔整體自有程式碼少數、零預設 Rust 取代）。
- [x] 推進 foundation 下一切片：新增 `hash_table` Rust parity module（`rust/cbm-core/src/foundation/hash_table.rs`，純安全 Rust + 4 個單元測試）與 test-only `cbm_rs_ht_*` FFI（`ffi.rs`），並在 `tests/test_rust_ffi.c` 新增 parity 測試，固定 content-key、`get_key` stored 指標（覆寫更新）、NULL value 為存在項、foreach exactly-once、非 UTF-8 高位元 key、null 契約與 resize。維持 C-only hot path，不導入產品 opt-in。
- [x] 嚴謹驗證全綠：`scripts/rust-check.sh`（fmt、clippy `-D warnings`、`cargo test --workspace`、`rust-ffi-test`、foundation opt-in matrix、pipeline registry/plan opt-in、language-graph parity）、`make rust-dangerous-api`（148 findings 全允許清單，新 FFI 由 `ffi.rs` wildcard 覆蓋）、`scripts/lint.sh --ci`（CI 對齊 `cppcheck 2.20.0` + `clang-format 20.1.8`，含 `test_rust_ffi.c`）。
- 說明：此切片為 test-only parity（不改變預設 C 路徑），是既有方法論中 production opt-in 的前置步驟；離「可移除 C」仍需完成路線圖各階段。
