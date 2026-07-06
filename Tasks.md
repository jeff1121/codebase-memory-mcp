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
- [x] 將既有 YAML C contract suite 納入 `test-foundation` 與 Rust foundation opt-in matrix，作為未來 Rust YAML test-only/opt-in 的前置基準。
- [x] 新增 YAML Rust test-only parity module（`foundation::yaml`）與 `cbm_rs_yaml_*` FFI smoke，固定 minimal YAML parser/query contract；仍不導入產品 opt-in。
- [x] 新增 log/profile deterministic Rust test-only parity module（`foundation::log`、`foundation::profile`）與 `cbm_rs_log_*` / `cbm_rs_profile_*` FFI smoke，固定 env parsing、text/JSON helper、HTTP path/status level、profile env gate、elapsed 與 rate contract；stderr/sink/clock/global state 仍留 C。
- [x] 新增 `CBM_USE_RUST_PROFILE_ENV=1` production opt-in，讓 `cbm_profile_init()` 只把 `CBM_PROFILE` env gate 判斷委派給 Rust `cbm_rs_profile_env_enabled()`；`cbm_profile_active`、clock 與 log emission 仍留 C。
- [x] 新增 `tests/test_profile.c` C-side contract suite 並接入 `test-foundation`，作為未來 profile opt-in 前置基準。
- [x] 新增 compat deterministic Rust test-only parity module（`foundation::compat`）與 `cbm_rs_compat_*` FFI smoke，固定 regex flags/cap/status、thread default stack 與 aligned alloc precheck；fs/process/thread runtime 與 production opt-in 仍留 C。
- [x] 新增 `tests/test_compat.c` C-side contract suite，並將 `profile`、`diagnostics`、`compat` suite 接入 full `test-runner` selector。
- [x] 新增 arena Rust test-only parity module（`foundation::arena`）與 public `CBMArena*` FFI smoke，固定 block growth、8-byte alignment、calloc、string helpers、reset/destroy 與 total accounting；allocator hot path 與 production opt-in 仍留 C。
- [x] 新增 mem Rust test-only parity module（`foundation::mem`）與 `cbm_rs_mem_*` FFI smoke，固定 budget 初始化、worker budget、RSS/peak RSS 與 over-budget contract；僅 test-only，不接產品 opt-in。

## Phase 3：核心服務邊界

- [x] 規劃並切分 `store` Rust 遷移，保留 SQLite schema、FTS5、WAL 與 fixture 相容。
- [x] 規劃並切分 `cypher` parser/query 遷移，建立 AST/result/error contract 而非誤用 SQL output。
- [x] 規劃並切分 `mcp` JSON-RPC 遷移，保留 tool schema、錯誤碼與 stdout/stderr 行為。
- [x] 新增第一批 `store` contract fixture：minimal schema/index existence、`edges.url_path_gen` generated column、user index drop/create symmetry。
- [x] 擴充 `store` contract fixture：`open_path_query` missing DB no-create、read-only open 與 read-only write rejection。
- [x] 擴充 `store` contract fixture：URL path API project-scoped substring 行為、`nodes_fts` manual rebuild 與 camelCase tokenization。
- [x] 擴充 `store` contract fixture：WAL journal mode、readonly WAL read/write rejection 與 generated column query plan/golden。
- [x] 新增 Store schema/index manifest Rust test-only helper 與 V1/V2 FFI smoke，固定核心表、`nodes_fts`、`edges.url_path_gen` 與 9 個 user index 的名稱/表/欄位/旗標，以及 V2 table layout / index / FTS shadow metadata；不開 SQLite、不接 production opt-in。
- [x] 新增 Store connection/pragma manifest Rust test-only helper 與 FFI smoke，固定 read-only no-create、immutable fallback、WAL、mmap env、bulk begin/end pragma 與 open-mode contract；不開 SQLite、不接 production opt-in。
- [x] 新增 Store FTS camelCase tokenization Rust helper、`cbm_rs_store_camel_split_v1` FFI smoke 與 `CBM_USE_RUST_STORE_FTS_TOKENIZER=1` 窄範圍 opt-in；只委派 C SQLite UDF callback 內的 token 字串計算，SQLite runtime 仍留 C。
- [x] 新增 Store mmap resolver Rust helper、`cbm_rs_store_resolve_mmap_size_value_v1` FFI smoke 與 `CBM_USE_RUST_STORE_MMAP_RESOLVER=1` 窄範圍 opt-in；只委派 `CBM_SQLITE_MMAP_SIZE` 字串解析，SQLite open/pragma execution 仍留 C。
- [x] 新增 Store immutable URI Rust helper、`cbm_rs_store_build_immutable_uri_v1` FFI smoke 與 `CBM_USE_RUST_STORE_IMMUTABLE_URI=1` 窄範圍 opt-in；只委派 readonly immutable fallback URI 字串建構，SQLite open/probe/WAL/pragma/FTS runtime 仍留 C。
- [x] 新增 Store search pattern Rust helper、glob/case flag/LIKE hint FFI smoke 與 `CBM_USE_RUST_STORE_SEARCH_PATTERN=1` 窄範圍 opt-in；只委派 glob→LIKE、LIKE hint extraction 與 `(?i)` case flag byte-level helper，SQL builder、bind lifetime、FTS/BM25 與 SQLite runtime 仍留 C。
- [x] 新增 Store architecture helper Rust helper、`cbm_rs_store_qn_to_package_v1`、`cbm_rs_store_qn_to_top_package_v1`、`cbm_rs_store_is_test_file_path_v1`、`cbm_rs_store_hop_to_risk_v1` 與 `cbm_rs_store_risk_label_v1`，並以 `CBM_USE_RUST_STORE_ARCH_HELPERS=1` 委派 QN 解析/風險與 path 判斷，保持 SQLite/open/runtime 不移轉。
- [x] 新增第一批 `cypher` contract fixture：parser AST shape、deterministic ordered rows、exact unsupported CREATE error。
- [x] 擴充 `cypher` contract fixture：WITH/post-WHERE AST、explicit `LIMIT 0` columns/rows、list indexing exact error。
- [x] 擴充 `cypher` golden fixture：OPTIONAL MATCH、UNION、edge property、unknown function 與更多 exact error messages。
- [x] 新增 Cypher lexer Rust test-only parity module 與 FFI smoke，固定 token kind、byte offset、string escape、comments、number / `..` boundary、summary truncation 與 C enum ABI；不接 production opt-in。
- [x] 新增 Cypher normalized AST summary Rust test-only parity helper 與 FFI smoke，固定 parser AST shape、OPTIONAL MATCH、UNION ALL、EXISTS direction、WITH/post-WHERE、caller-buffer truncation 與 malformed query contract；不接 production opt-in。
- [x] 擴充 Cypher hop shorthand parity，固定 `*N -> 1..N`、`* -> 1..0`、`*..N -> 1..N` 的 C parser AST contract 與 Rust normalized AST summary。
- [x] 擴充 `mcp` transcript fixture：`tools/list` pagination、原始 tool 順序、notification no-response、error code、unknown tool 與 Content-Length。
- [x] 擴充 `mcp` BM25 fixture：`search_graph` query path 的 `file_pattern`、label boost rank/order、`total` 與 `has_more` contract。
- [x] 擴充 `mcp` transcript fixture：alias、tool schema、indexed repo query。
- [x] 新增 MCP JSON-RPC request summary Rust test-only parity module 與 `cbm_rs_mcp_jsonrpc_request_summary_v1` FFI smoke，固定 request envelope parse contract；不接 production opt-in。
- [x] 新增 MCP JSON-RPC parse Rust helper、`cbm_rs_mcp_jsonrpc_parse_v1` FFI smoke 與 `CBM_USE_RUST_MCP_CODEC=1` 窄範圍 opt-in；只委派 `cbm_jsonrpc_parse()` request envelope parse，response formatting、Content-Length transport、tool schema 與 14 個 handlers 仍留 C。

## Phase 4：Pipeline 與 Extraction

- [x] 建立 registry name lookup / candidate selection 的 Rust parity module 與一次性 FFI fixture。
- [x] 將 `CBM_USE_RUST_PIPELINE_REGISTRY=1` opt-in 接入 `cbm_registry_resolve`，以 Rust opaque handle 承接 registry resolve hot path，並保留 C registry-owned return pointer contract。
- [x] 建立 Rust pipeline pass plan parity module 與 test-only FFI fixture，固定 full pipeline、sequential、parallel extraction、predump、incremental extract/resolve 與 incremental post/tail typed order、fast-mode gating 與 parallel threshold。
- [x] 將 `CBM_USE_RUST_PIPELINE_PLAN=1` opt-in 接入 full 與 incremental extraction phase，僅由 Rust plan 選擇 parallel/sequential，實際 pass function pointers 與資料 mutation 仍留 C。
- [x] 建立 language graph parity gate，固定代表性語言 definitions、calls、imports、routes、semantic/LSP edges，並比對 default C path 與 Rust pipeline/store opt-in path。
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
- [x] 將 sequential `pass_definitions.c`、`pass_calls.c` 與 `pass_semantic.c` 的 node/edge 寫入改走 C-side graph-buffer mutation adapter，保留 registry、route、channel、env/import、decorator、inheritance 與 implements/override 計數語意；static guard 已納入這三個檔案。
- [x] 將 `pass_parallel.c` 的 shared/main `ctx->gbuf` registry/import/channel 寫入、worker-local `ws->local_gbuf` / `ws->local_edge_buf` 寫入，以及 service-helper 裸 `gbuf` route/call 寫入改走 C-side graph-buffer mutation adapter；static guard 已納入完整檔案，並新增 no-endpoint-lookup contract test。
- [x] 擴充 language graph parity gate，新增 Rust、Java 與 C++ 小型 fixture，固定更多 definitions、calls、DEFINES_METHOD、INHERITS/IMPLEMENTS 與 LSP strategy 觀測面，並比對 default C path 與 Rust pipeline/store opt-in path。
- [x] 新增 `pipeline::fqn` Rust test-only helper 與 `cbm_rs_pipeline_project_name_from_path` FFI smoke，固定 `cbm_project_name_from_path` 的 byte-level sanitizer、drive normalization、non-ASCII hex encoding 與 buffer truncation contract；不接 production opt-in。
- [x] 新增 FullPipeline top-level orchestration metadata FFI（`cbm_rs_pipeline_full_plan_step_count_v1` / `cbm_rs_pipeline_full_plan_steps_v1`），固定 12 個 full/moderate/advanced outer steps、fast-mode 11 steps、`requires_mask`、gate/effect flags 與 nested plan kind；metadata-only，不接 `pipeline.c` / `pipeline_incremental.c` runtime dispatch，`PlanStepV2(kind=FullPipeline)` 仍維持 unsupported。
- [x] 固定 incremental dump failure side-effect contract：dump 失敗會原樣傳回錯誤，不再落入 full fallback，且會跳過後續 `persist_hashes` / `artifact_export` tail。
- [ ] 遷移 pipeline orchestration、pass registry 與資料流管理。
- [ ] 保留現有 tree-sitter grammar shim，再逐步替換周邊 glue code。
- [ ] 隨 extraction、tree-sitter 與 LSP glue 遷移持續擴充 language graph parity coverage。

## Phase 5：預設切換與清理

- [ ] Rust-backed binary 達到等價門檻後，切換預設 build path。
- [ ] 保留 C fallback 至少一個 release cycle。
- [ ] 清理已取代 C 模組前，驗證 packaging、install/update/uninstall、UI variant 與 security suite。
- [x] 更新 `README.md`、`CONTRIBUTING.md`、release workflow 與 contributor guide。


## 完整移除 C 路線圖追蹤

- [ ] Foundation 剩餘葉模組：完成 `yaml` production opt-in 評估、`log`/`profile` stderr/sink/clock/global-state wrapper parity 與 production opt-in 評估、`compat_*` fs/process/thread runtime wrapper（`hash_table`、`yaml`、log/profile、compat 與 arena deterministic/public-struct test-only parity 已完成），以及 `arena`/`slab_alloc`/`mem`/`vmem` allocator stress/fuzz 與 production opt-in 評估。
- [ ] Pipeline orchestration 全面化：Rust 承接 incremental post-pass adapter、pass registry metadata、full/incremental dispatch 與 graph-buffer mutation 邊界（command contract 與多批 C-side adapter 呼叫點已固定，完整資料流仍未完成）；language graph parity 持續擴充。
- [ ] Store：補完整 SQLite schema/index/FTS/checkpoint/bulk/artifact golden，分段遷移 open/readonly/WAL、CRUD/search、FTS/BM25、vector 與 writer path（schema/index/FTS shadow、connection/pragma static contract、FTS camelCase tokenization helper、mmap resolver、immutable URI builder、search pattern helper、architecture helper 窄 opt-in 已先完成）。
- [x] Cypher：補 CALL/EXISTS/malformed/aggregation exact golden，並新增 lexer/token summary 與 normalized AST summary Rust test-only parity FFI；production parser/evaluator/executor 仍留 C。
- [ ] Cypher：依序遷移 parser、AST normalizer、expression evaluator、projection/aggregation 與 executor。
- [ ] MCP：補 14 tools transcript 與錯誤路徑，依序遷移 JSON-RPC codec、tool schema generator、read-only handlers、狀態型 handlers 與 install/config 類 handler（JSON-RPC request summary test-only parity 與 request parse opt-in 已起步；response formatting、Content-Length transport、tool schema 與 handler 仍未遷移）。
- [ ] `internal/cbm`：擴充 158 語言 graph parity，保留 C grammar shim 起步，逐語言遷移 extractor/helper，最後處理 Hybrid LSP timeout/fallback/process I/O。
- [ ] Phase 5 預設切換：全 Rust-backed opt-in matrix 達等價門檻後，建立 release candidate、通過 default/單一/全 opt-in/Rust-backed matrix，才切預設 build。
- [ ] C fallback release cycle：預設 Rust-backed 後保留 C fallback 至少一個 release cycle，文件標明回滾方式與相容期。
- [ ] 移除 C：逐子系統刪除已取代 C fallback、`CBM_USE_RUST_*` 過渡旗標與文件，最後處理 `internal/cbm`，並保留必要 vendored grammar C shim。
- [ ] 最終 gate：packaging、install/update/uninstall、UI variant、security suite、跨平台、效能 ≤10% 退化、binary size ≤20%、SQLite/artifact/MCP/CLI/language parity 全部通過。

## 驗證狀態

- [x] `scripts/rust-check.sh`
- [x] `make -f Makefile.cbm rust-ci`（本機未安裝 `cargo-audit` 時依設計跳過 audit）
- [x] `make -f Makefile.cbm rust-ffi-test`（2026-07-06：含 diagnostics、hash_table、YAML、log/profile、compat、arena public-struct、Cypher lexer/AST summary test-only parity、MCP JSON-RPC request summary test-only parity、pipeline project-name test-only parity、pipeline plan parity、typed sequential/incremental/predump/parallel extraction v2 metadata、FullPipeline top-level metadata ABI、PlanStepV2 ABI layout、typed incremental post step FFI smoke、graph-buffer mutation metadata/validation ABI，以及 store schema/connection manifest、FTS camelCase tokenization、mmap resolver 與 immutable URI builder；C smoke 共用 `graph_buffer.h` command struct/constants，並鎖定 unknown+reserved validation 優先序）
- [x] `make -f Makefile.cbm rust-ffi-test`（2026-07-06：新增 Store search pattern FFI smoke，覆蓋 glob length-only/NULL/短 buffer、case flag helpers、LIKE hint count/value、escaped pipe、escaped meta、max_out、invalid index 與 255-byte literal truncation）
- [x] `make -f Makefile.cbm rust-foundation-optin-test`（2026-07-05：已納入 YAML、profile 與 compat C contract，每輪 foundation runner 322 passed；2026-07-06 起納入 `CBM_USE_RUST_PROFILE_ENV=1` 單一與全量 foundation opt-in）
- [x] `make -f Makefile.cbm test-foundation`（2026-07-05：default C path 322 passed，含 YAML、profile、diagnostics 與 compat suite）
- [x] `cargo test -p cbm-core foundation::yaml --locked`（2026-07-05：3 passed）
- [x] `cargo test -p cbm-core foundation::log --locked`（2026-07-05：2 passed）
- [x] `cargo test -p cbm-core foundation::profile --locked`（2026-07-05：2 passed）
- [x] `cargo test -p cbm-core foundation::compat --locked`（2026-07-05：2 passed）
- [x] `cargo test -p cbm-core foundation::arena --locked`（2026-07-05：3 passed）
- [x] `cargo test -p cbm-core foundation::mem --locked`（2026-07-05：4 passed）
- [x] `cargo test -p cbm-core pipeline::fqn --locked`（2026-07-05：3 passed）
- [x] `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`（2026-07-05：通過，含 YAML、log/profile、compat 與 pipeline project-name Rust test-only modules）
- [x] `make -f Makefile.cbm build/c/test-runner && build/c/test-runner profile diagnostics compat`（2026-07-05：15 passed，確認新增/既有 foundation suite 已接入 full runner selector）
- [x] `make -f Makefile.cbm build/c/test-runner && build/c/test-runner fqn`（2026-07-05：77 passed，確認 C-side project-name contract 仍與 Rust test-only parity 對齊）
- [x] `make -f Makefile.cbm CBM_USE_RUST_STR_UTIL=1 test-foundation`
- [x] `make -f Makefile.cbm CBM_USE_RUST_PLATFORM_PATH=1 test-foundation`
- [x] `make -f Makefile.cbm CBM_USE_RUST_PLATFORM_CGROUP=1 test-foundation`
- [x] `make -f Makefile.cbm CBM_USE_RUST_PLATFORM_ENV_DIRS=1 test-foundation`
- [x] `make -f Makefile.cbm CBM_USE_RUST_DUMP_VERIFY=1 CBM_USE_RUST_STR_INTERN=1 CBM_USE_RUST_STR_UTIL=1 CBM_USE_RUST_PLATFORM_PATH=1 CBM_USE_RUST_PLATFORM_CGROUP=1 CBM_USE_RUST_PLATFORM_ENV_DIRS=1 test-foundation`
- [x] `tests/test_parent_watchdog.sh` 連續 5 次通過
- [x] `scripts/test.sh` 完整重跑（2026-07-02 通過：完整 C runner、parent-death watchdog、security-string allow-list、Rust 單元測試、FFI smoke 與含 env/dirs 的 foundation opt-in matrix 均通過）
- [x] `scripts/test.sh` 完整含 Rust 重跑（2026-07-05 通過：C runner 5798 passed、parent-death watchdog、security-string 2 passed / 0 failed、Rust unit 51 passed、registry opt-in 53 passed、pipeline plan opt-in 217 passed，最終 `=== All tests passed ===`；同日也先補做 `CBM_SKIP_RUST=1 scripts/test.sh` C-only 回歸）
- [x] `scripts/test.sh` 完整含 Rust 重跑（2026-07-06 通過：Rust unit 86 passed、registry opt-in 53 passed、pipeline plan opt-in 217 passed、Store FTS tokenizer opt-in `store_compat mcp` 131 passed，最終 `=== All tests passed ===`）
- [x] `scripts/build.sh`（2026-07-06 通過，輸出 `build/c/codebase-memory-mcp`）
- [x] `scripts/build.sh --with-ui`（2026-07-06 通過，建置 `graph-ui` 並產生內嵌 UI binary）
- [x] `python3 scripts/rust-baseline-fixtures.py build/c/codebase-memory-mcp`
- [x] `make -f Makefile.cbm rust-baseline-fixtures`
- [x] `make -f Makefile.cbm build/c/test-runner && build/c/test-runner store_compat`（2026-07-02：8 passed）
- [x] Phase 3 Store schema/connection manifest helper targeted gates（2026-07-05）：`cargo fmt --all -- --check`、`cargo test -p cbm-core store::schema_manifest --locked`（4 passed）、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm build/c/test-runner && build/c/test-runner store_compat`（11 passed，含 V2 metadata 與 connection/pragma contract tests）
- [x] Phase 3 Store FTS camelCase tokenization helper targeted gates（2026-07-05）：`cargo fmt --all -- --check`、`cargo test -p cbm-core store::tokenization --locked`（3 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm build/c/test-runner && build/c/test-runner store_compat`（12 passed）與 `make -f Makefile.cbm rust-store-fts-tokenizer-optin-test`（`store_compat mcp` 131 passed）
- [x] Phase 3 Store mmap resolver opt-in targeted gates（2026-07-06）：`cargo fmt --all -- --check`、`cargo test -p cbm-core store::pragmas --locked`（1 passed）、`cargo test -p cbm-core --locked`（92 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm build/c/test-runner && build/c/test-runner store_pragmas store_compat`（23 passed）與 `make -f Makefile.cbm rust-store-mmap-resolver-optin-test`（23 passed）
- [x] Phase 3 Store immutable URI opt-in targeted gates（2026-07-06）：`cargo test -p cbm-core store::open --locked`（3 passed）、`make -f Makefile.cbm rust-ffi-test` 與 `make -f Makefile.cbm rust-store-immutable-uri-optin-test`（`store_compat mcp` 131 passed）。
- [x] Phase 3 Store search pattern opt-in targeted gates（2026-07-06）：`cargo fmt --all -- --check`、`cargo test -p cbm-core store::search_pattern --locked`（4 passed）、`cargo test -p cbm-core --locked`（102 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm build/c/test-runner && build/c/test-runner store_search mcp`（191 passed）、`make -f Makefile.cbm rust-store-search-pattern-optin-test`（191 passed）與 `make -f Makefile.cbm lint-format`。
- [x] Phase 3 Store architecture helper opt-in targeted gates（2026-07-06）：`cargo fmt --all -- --check`、`cargo test -p cbm-core store::arch_helpers --locked`（4 passed）、`cargo test -p cbm-core --locked`（106 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm build/c/test-runner && build/c/test-runner store_arch store_search mcp`（245 passed）、`make -f Makefile.cbm rust-store-arch-helper-optin-test`（245 passed）與 `make -f Makefile.cbm lint-format`、`git diff --check`。
- [x] `make -f Makefile.cbm build/c/test-runner && build/c/test-runner cypher_contract`（2026-07-02：13 passed）
- [x] 2026-07-05 Phase 3 Cypher contract 補洞：`make -f Makefile.cbm build/c/test-runner && build/c/test-runner cypher_contract`（20 passed）
- [x] 2026-07-06 Phase 3 Cypher lexer parity targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（3 passed）、`cargo test -p cbm-core --locked`（80 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner cypher_contract`（20 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- [x] 2026-07-06 Phase 3 Cypher AST summary parity targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（6 passed）、`cargo test -p cbm-core --locked`（83 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner cypher_contract`（20 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- [x] 2026-07-06 Phase 3 MCP JSON-RPC request summary parity targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（4 passed）、`cargo test -p cbm-core --locked`（90 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner mcp`（119 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- [x] 2026-07-06 Phase 3 MCP JSON-RPC parse codec opt-in targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（6 passed）、`cargo test -p cbm-core --locked`（98 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm build/c/test-runner && build/c/test-runner mcp`（123 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（123 passed）、`make -f Makefile.cbm CBM_USE_RUST_MCP_CODEC=1 build/c/test-runner && build/c/test-runner mcp`（123 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- [x] 2026-07-06 Phase 3 Cypher hop shorthand parity targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（10 passed）、`cargo test -p cbm-core --locked`（91 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner cypher_contract`（21 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- [x] 2026-07-06 Phase 4 targeted verification：`build/c/test-runner pipeline graph_buffer lang_contract edge_structural`（332 passed）、`make -f Makefile.cbm rust-pipeline-plan-optin-test`（217 passed）與 `make -f Makefile.cbm rust-language-graph-parity`（golden 一致）。並行跑多個會 `clean-c` 的 Make 目標會互相清掉 `build/c`，因此 registry/plan/language graph 這類共享 build dir gate 需序列執行。
- [x] `build/c/test-runner cypher cypher_contract`（2026-07-02：156 passed）
- [x] `make -f Makefile.cbm build/c/test-runner && build/c/test-runner mcp`（2026-07-02：119 passed）
- [x] `scripts/smoke-test.sh build/c/codebase-memory-mcp`（2026-07-06 通過，含 install/update/uninstall/config、MCP、CLI、binary signature 與 kill E2E；UI HTTP phase 因不可達而 skip，不視為 UI release packaging gate）
- [x] `scripts/smoke-invariants.sh build/c/codebase-memory-mcp`（2026-07-06：30 passed / 0 failed）
- [x] `scripts/lint.sh --ci`（2026-07-06 通過；以 CI 對齊版本 `cppcheck` + `clang-format` 驗證。full lint 的 `clang-tidy` 屬本機重型 gate，非 `--ci` 範圍）
- [x] `make -f Makefile.cbm security`（2026-07-06 通過，含 security-fuzz 23/23、vendored integrity、Rust allowlist 與 cargo-audit 掃描）
- [x] Rust 相依套件稽核：`make -f Makefile.cbm RUST_AUDIT_REQUIRED=1 rust-audit`（2026-07-02 通過；以暫存 `cargo-audit v0.22.2` 掃描 `Cargo.lock` 的 1 個 crate dependency）
- [x] `make -f Makefile.cbm rust-dangerous-api`（2026-07-02：81 findings all allowlisted；已接入 `_security.yml`）
- [x] `make -f Makefile.cbm rust-security`（Rust source allowlist 通過；本機預設仍可跳過 `cargo-audit`，CI 以 `RUST_AUDIT_REQUIRED=1` 強制執行）
- [x] Rust unsafe/process/network/filesystem allowlist gate（不代表 Rust 相依套件稽核完成）
- [x] `cargo test --workspace --all-targets --all-features --locked`（2026-07-04：46 passed）
- [x] `cargo test -p cbm-core pipeline::registry --locked`（2026-07-02：3 passed）
- [x] `make -f Makefile.cbm build/c/test-runner && build/c/test-runner registry`（2026-07-02：default C path 52 passed）
- [x] `make -f Makefile.cbm rust-pipeline-registry-optin-test`（2026-07-02：`CBM_USE_RUST_PIPELINE_REGISTRY=1` registry suite 53 passed）
- [x] `CBM_USE_RUST_PIPELINE_PLAN=1 build/c/test-runner pipeline`（2026-07-04：pipeline suite 217 passed；涵蓋 extraction choice、sequential extraction typed v2 metadata/trace、incremental extract/resolve typed v2 dispatch/trace、parallel extraction typed v2 metadata/trace、full pre-dump typed v2 metadata/trace、typed incremental post/tail dispatch 與 incremental dump failure propagation）
- [x] `make -f Makefile.cbm rust-language-graph-parity-update`（2026-07-02：已產生 `language-graph-parity.json`，default C path 與 Rust pipeline/store opt-in path graph 完全一致）
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
- [x] `make -f Makefile.cbm rust-language-graph-parity`（2026-07-04：default C path 與 `CBM_USE_RUST_PIPELINE_REGISTRY=1 CBM_USE_RUST_PIPELINE_PLAN=1 CBM_USE_RUST_STORE_FTS_TOKENIZER=1` golden 一致，未更新 `tests/golden/rust-refactor`）
- [x] 格式與 golden hygiene（2026-07-04）：`cargo fmt --all -- --check`、`make -f Makefile.cbm lint-format`、`git diff --check`、`git diff --exit-code -- tests/golden/rust-refactor`
- [x] 2026-07-05 較早 adapter targeted gates：`make -f Makefile.cbm clean-c build/c/test-runner`、`build/c/test-runner graph_buffer pipeline`（268 passed）、`make -f Makefile.cbm rust-pipeline-plan-optin-test`（217 passed）、`make -f Makefile.cbm rust-language-graph-parity`、`scripts/lint.sh --ci`、`cargo fmt --all -- --check`、`make -f Makefile.cbm lint-format`、`git diff --check`、`git diff --exit-code -- tests/golden/rust-refactor`
- [x] 2026-07-05 針對較早 adapter 收斂補做 C/安全回歸：`CBM_SKIP_RUST=1 scripts/test.sh`（5798 passed）與 `make -f Makefile.cbm security`（security-fuzz 23/23、`security-ui`/`security-rust`/`security-binary-string` 全通過）；完整含 Rust 的 `scripts/test.sh` 亦已補跑通過。此完整測試紀錄早於 `pass_parallel.c` adapter 收斂，最新 pass_parallel 批次以後續 targeted gates 為證。
- [x] 2026-07-05 sequential pass adapter gates：`build/c/test-runner graph_buffer pipeline lang_contract edge_structural`（331 passed）、`make -f Makefile.cbm lint-format`、`git diff --check`；direct-call search 確認 `pass_definitions.c`、`pass_calls.c`、`pass_semantic.c` 無 `cbm_gbuf_upsert_node` / `cbm_gbuf_insert_edge`。
- [x] 2026-07-05 pass_parallel adapter gates：`build/c/test-runner graph_buffer parallel edge_types_probe lang_contract`（156 passed）、`make -f Makefile.cbm lint-format`、`git diff --check`；direct-call search 確認 `pass_parallel.c` 無 `cbm_gbuf_upsert_node` / `cbm_gbuf_insert_edge`，並以 graph-buffer adapter test 固定 `InsertEdge` 不做 endpoint lookup。
- [x] 2026-07-05 language graph parity expansion：`make -f Makefile.cbm rust-language-graph-parity-update` 更新 Rust/Java/C++ fixture golden，`python3 scripts/rust-language-graph-parity.py target/rust-parity/cbm-default target/rust-parity/cbm-rust-pipeline` 非 update 驗證通過；`python3 -m py_compile scripts/rust-language-graph-parity.py` 與 `git diff --check` 通過。

## 本次工作階段（2026-07-05 Phase 3 Store schema manifest helper）

- [x] 新增 `rust/cbm-core/src/store/schema_manifest.rs`，以 static、無副作用 manifest 固定核心表、`nodes_fts`、`edges.url_path_gen` 與 9 個 user index 的最小 schema/index contract。
- [x] 新增 `cbm_rs_store_schema_manifest_entry_count_v1`、`cbm_rs_store_schema_manifest_user_index_count_v1` 與 `cbm_rs_store_schema_manifest_entries_v1` test-only FFI，C 端以 `CbmRsStoreSchemaManifestEntryV1` static assert 鎖定 ABI layout。
- [x] 擴充 `tests/test_rust_ffi.c`，驗證 manifest count、null/short/negative cap 錯誤、entry kind/flags/table/column 與 user index 數量。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core store::schema_manifest --locked`、`cargo test -p cbm-core --locked`、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm lint-format`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner store_compat`。

## 本次工作階段（2026-07-05 Phase 3 Store connection/pragma manifest helper）

- [x] 擴充 `rust/cbm-core/src/store/schema_manifest.rs`，新增 static、無副作用的 connection/pragma manifest，固定 `open.memory`、`open.path_write`、`open.path_query`、read probe、immutable fallback、WAL、mmap env、synchronous、busy_timeout、bulk cache_size 與 optimize contract。
- [x] 新增 `cbm_rs_store_connection_manifest_entry_count_v1`、`cbm_rs_store_connection_manifest_write_pragma_count_v1` 與 `cbm_rs_store_connection_manifest_entries_v1` test-only FFI，C 端以 `CbmRsStoreConnectionManifestEntryV1` static assert 鎖定 ABI layout。
- [x] 擴充 `tests/test_rust_ffi.c`，驗證 connection manifest count、null/short/negative cap、read-only no-create、immutable URI fallback、write pragma、mmap env 與 bulk cache_size entries。
- [x] 擴充 `tests/test_store_compat.c`，新增 `store_compat_bulk_pragmas_preserve_wal_contract`，固定 bulk begin/end 的 `synchronous`、`cache_size` 與 WAL 不變 contract。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core store::schema_manifest --locked`（4 passed）、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm build/c/test-runner && build/c/test-runner store_compat`（11 passed）。

## 本次工作階段（2026-07-05 Phase 3 Store FTS camelCase tokenization helper）

- [x] 新增 `rust/cbm-core/src/store/tokenization.rs`，以 byte-level Rust helper 對齊 `cbm_camel_split` 的 FTS tokenization contract。
- [x] 新增 `cbm_rs_store_camel_split_v1` FFI，沿用 caller-provided buffer、完整輸出長度回傳與短 buffer NUL 結尾慣例。
- [x] 擴充 `tests/test_rust_ffi.c`，覆蓋 NULL/empty、camelCase、acronym boundary、snake_case、非 ASCII byte、短 buffer、2048-byte fallback 與 near-limit truncation contract。
- [x] 新增 `CBM_USE_RUST_STORE_FTS_TOKENIZER=1` wrapper，讓 C 註冊的 `cbm_camel_split` SQLite UDF callback 只把 token 字串計算委派給 Rust；SQLite connection、UDF registration、FTS5 runtime、`sqlite3_result_text`、CRUD/search/BM25 仍留 C。
- [x] 擴充 `tests/test_store_compat.c`，新增 direct scalar UDF exact contract，固定 acronym、非 ASCII byte、2048-byte fallback 與 near-limit 行為。
- [x] 新增 `rust-store-fts-tokenizer-optin-test`，並接入 `rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7；language graph parity 的 Rust opt-in binary 也同步帶 `CBM_USE_RUST_STORE_FTS_TOKENIZER=1`。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core store::tokenization --locked`（3 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm build/c/test-runner && build/c/test-runner store_compat`（12 passed）與 `make -f Makefile.cbm rust-store-fts-tokenizer-optin-test`（`store_compat mcp` 131 passed）。

## 本次工作階段（2026-07-06 Phase 3 Store mmap resolver opt-in）

- [x] 新增 `rust/cbm-core/src/store/pragmas.rs`，以 byte-level Rust helper 固定 `CBM_SQLITE_MMAP_SIZE` parser contract。
- [x] 新增 `cbm_rs_store_resolve_mmap_size_value_v1` FFI smoke，覆蓋 NULL/unset、empty、garbage、partial、trailing space、overflow、zero、negative、leading whitespace、`+` 與 `INT64_MAX`。
- [x] 修正 C `cbm_store_resolve_mmap_size()`，以 `errno == ERANGE` 將 overflow 回 64 MiB default，並補 `tests/test_store_pragmas.c` exact contract。
- [x] 新增 `CBM_USE_RUST_STORE_MMAP_RESOLVER=1` 與 `rust-store-mmap-resolver-optin-test`；只委派 env 字串解析，SQLite open、pragma dispatch、readonly/write 分流與 SQL execution 仍留 C。
- [x] 將新 target 接入 `rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core store::pragmas --locked`（1 passed）、`cargo test -p cbm-core --locked`（92 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner store_pragmas store_compat`（23 passed）與 `make -f Makefile.cbm rust-store-mmap-resolver-optin-test`（23 passed）。

## 本次工作階段（2026-07-06 Phase 3 Store immutable URI opt-in）

- [x] 新增 `rust/cbm-core/src/store/open.rs`，以 byte-level Rust helper 固定 SQLite readonly immutable fallback URI builder contract。
- [x] 新增 `cbm_rs_store_build_immutable_uri_v1` FFI smoke，覆蓋 POSIX/relative/root/Windows/UNC path、安全字元、space、`%`、`?`、`#`、`&`、`=`、UTF-8 raw bytes、NULL、length-only 與短 buffer 截斷。
- [x] 新增 `CBM_USE_RUST_STORE_IMMUTABLE_URI=1`，讓 C `build_immutable_uri()` 只把 URI 字串建構委派給 Rust；SQLite `open_v2`、readonly/no-create probe、immutable fallback timing、WAL/pragma dispatch、UDF/FTS/search/CRUD 與 Store lifetime 仍留 C。
- [x] 新增 `rust-store-immutable-uri-optin-test` 並接入 `rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7。
- [x] 驗證通過：`cargo test -p cbm-core store::open --locked`（3 passed）、`make -f Makefile.cbm rust-ffi-test` 與 `make -f Makefile.cbm rust-store-immutable-uri-optin-test`（`store_compat mcp` 131 passed）。

## 本次工作階段（2026-07-06 Phase 3 Store search pattern opt-in）

- [x] 新增 `rust/cbm-core/src/store/search_pattern.rs`，固定 `cbm_glob_to_like`、`cbm_extract_like_hints`、`cbm_ensure_case_insensitive` 與 `cbm_strip_case_flag` 的 byte-level contract。
- [x] 新增 `cbm_rs_store_glob_to_like_v1`、case flag helpers、LIKE hint count/value FFI，沿用 caller-provided buffer、完整長度回傳、短 buffer NUL 截斷、`SIZE_MAX` error 與 C-owned allocation contract。
- [x] 新增 `CBM_USE_RUST_STORE_SEARCH_PATTERN=1`，讓 C wrapper 只委派 search pattern 純字串 helpers；SQL builder、LIKE bind lifetime、REGEXP UDF、FTS/BM25、SQLite statement lifecycle 與 Store runtime 仍留 C。
- [x] 補強 Rust unit、direct FFI smoke 與 `tests/test_store_search.c`，覆蓋 escaped pipe bail、escaped meta、max_out、NULL、短 buffer、invalid index 與 255-byte literal truncation。
- [x] 新增 `rust-store-search-pattern-optin-test` 並接入 `rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh`、`scripts/test.sh` Step 7 與 reusable Rust workflow 註解。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core store::search_pattern --locked`（4 passed）、`cargo test -p cbm-core --locked`（102 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner store_search mcp`（191 passed）、`make -f Makefile.cbm rust-store-search-pattern-optin-test`（191 passed）與 `make -f Makefile.cbm lint-format`。

## 本次工作階段（2026-07-06 Phase 3 Store architecture helper opt-in）

- 新增 Store architecture helper Rust helpers 的 ABI + C wrapper opt-in 通道：`cbm_qn_to_package`、`cbm_qn_to_top_package`、`cbm_is_test_file_path`、`cbm_hop_to_risk`、`cbm_risk_label`。
- 擴充 `tests/test_rust_ffi.c`，補齊長度-only、`buf==NULL`、`bufsize==0` 與 short buffer 邊界測試。
- 新增 `CBM_USE_RUST_STORE_ARCH_HELPERS=1`，讓 C store wrapper 只在 pure helper 層委派 architecture helper，SQLite、BFS/search/CRUD runtime 與 store ownership 仍保留 C。
- 目前預計完成度：`src/store/store.c` 註解更新、C helper 測試補齊已完成；`Phase 3 Store architecture helper opt-in` 驗證欄已補齊且已通過（`245` passed）。

## 本次工作階段（2026-07-06 Phase 4 FullPipeline top-level metadata FFI）

- [x] 擴充 `rust/cbm-core/src/pipeline/plan.rs`，新增 FullPipeline top-level metadata helper，固定 macro extraction、user config、discover、incremental-or-delete-db、structure、extraction dispatch、tests、githistory、predump、dump、persist hashes 與 artifact export 的外層順序。
- [x] 新增 `CbmRsPipelineTopStepV1` 與 `cbm_rs_pipeline_full_plan_step_count_v1` / `cbm_rs_pipeline_full_plan_steps_v1`，以 C static assert 鎖定 ABI layout、full/fast step count、gate/effect flags、`requires_mask`、nested plan kind 與 null/short/negative cap contract。
- [x] 保留 `cbm_rs_pipeline_plan_steps_v2(kind=FullPipeline)` unsupported，並在 `tests/test_graph_buffer.c` 加入 static guard，確認 `pipeline.c` 與 `pipeline_incremental.c` 不消費新的 FullPipeline metadata API。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::plan --locked`（12 passed）、`cargo test -p cbm-core --locked`（93 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test` 與 `build/c/test-runner graph_buffer pipeline`（269 passed）。

## 本次工作階段（2026-07-06 Foundation profile env opt-in）

- [x] 新增 `CBM_USE_RUST_PROFILE_ENV=1`，讓 `src/foundation/profile.c` 的 `cbm_profile_init()` 在 opt-in 時委派 Rust `cbm_rs_profile_env_enabled()` 判斷 `CBM_PROFILE` 是否啟用。
- [x] 預設 C path 保持不變；`cbm_profile_active`、`cbm_profile_enable()`、clock、elapsed logging 與 log sink/formatter 仍由 C 負責。
- [x] 將新 flag 納入 Rust staticlib link trigger、`rust-foundation-optin-test` 單一 opt-in 與全量 foundation opt-in matrix。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core foundation::profile --locked`（2 passed）、`cargo test -p cbm-core --locked`（93 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm test-foundation`（322 passed）、`make -f Makefile.cbm CBM_USE_RUST_PROFILE_ENV=1 test-foundation`（322 passed）、`make -f Makefile.cbm rust-foundation-optin-test`（每輪 322 passed，含 profile env 單一與全量 opt-in）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-05 Phase 3 Cypher contract 補洞）

- [x] 新增 `cypher_contract_exists_predicate_any_direction_filters`，補任何方向 `EXISTS { ... }` 對 `f` 的回傳契約。
- [x] 新增 `cypher_contract_aggregation_edge_properties_numeric`，補齊 edge property 聚合 `SUM`、`AVG`、`MIN`、`MAX` 契約。
- [x] 將 `CALL`、`EXISTS`、malformed `EXISTS` 與 `COUNT(DISTINCT)` 契約補齊於 `tests/test_cypher_contract.c`，維持 C-side contract 收斂方向。

## 本次工作階段（2026-07-06 Phase 3 Cypher lexer test-only parity）

- [x] 新增 `rust/cbm-core/src/cypher/mod.rs`，以 byte-level Rust lexer 對齊 `src/cypher/cypher.c` 的 token kind、keyword lookup、single/two-char symbols、comments、number / `..` 邊界與 string literal escape/truncation contract。
- [x] 新增 `cbm_rs_cypher_lex_token_count_v1`、`cbm_rs_cypher_lex_tokens_v1` 與 `cbm_rs_cypher_lex_summary_v1` test-only FFI；只輸出 scalar token metadata 與 caller-buffer summary，不保存 C 指標、不配置 Rust-owned token text、不接 production opt-in。
- [x] 擴充 `tests/test_rust_ffi.c`，用 `cypher/cypher.h` token enum static assert 鎖定 C/Rust ABI，並覆蓋 null/short buffer、CALL/EXISTS 前置語法核心 token、string escape、comment skipping、DOTDOT 與 EOF offset。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 Cypher lexer parity 仍不代表 parser、AST、evaluator 或 executor 遷移。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（3 passed）、`cargo test -p cbm-core --locked`（80 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner cypher_contract`（20 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-06 Phase 3 Cypher AST summary test-only parity）

- [x] 擴充 `rust/cbm-core/src/cypher/mod.rs`，在既有 token stream 上新增最小 normalized AST summary helper，固定 parser AST shape、OPTIONAL MATCH optional flags、UNION ALL linked shape、EXISTS inbound direction 與 WITH/post-WHERE shape。
- [x] 新增 `cbm_rs_cypher_parse_summary_v1` test-only FFI，使用 caller-provided buffer、完整長度回傳、短 buffer NUL 截斷與 `SIZE_MAX` error contract；不保存 C 指標、不配置 Rust-owned AST、不執行 query、不接 production opt-in。
- [x] 擴充 `tests/test_rust_ffi.c`，覆蓋 AST summary golden、length-only、short buffer、null input、negative len 與 malformed query。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 AST summary 只固定 C parser shape 的 test-only 對照，production parser/evaluator/executor 仍留 C。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（6 passed）、`cargo test -p cbm-core --locked`（83 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner cypher_contract`（20 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-06 Cypher AST summary edge-case parity）

- [x] 補充 `rust/cbm-core/src/cypher/mod.rs` `parse_summary` unit 測試，覆蓋 `UNION` 非 `ALL`（`distinct`）與 `EXISTS (f)` 失敗路徑；新增 `WITH` / `RETURN DISTINCT` 解析及輸出驗證。
- [x] 擴充 `tests/test_rust_ffi.c`，新增 `cypher_parse_summary` 的 `UNION` distinct、`EXISTS (f)` 與 `WITH`/`RETURN DISTINCT` 契約 smoke 測試，沿用既有 `SIZE_MAX`/buffer 行為。

## 本次工作階段（2026-07-06 Phase 3 MCP JSON-RPC request summary parity）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs`，以 byte-level Rust parser 固定 MCP JSON-RPC request envelope summary contract。
- [x] 新增 `cbm_rs_mcp_jsonrpc_request_summary_v1` test-only FFI，使用 caller-provided buffer、完整長度回傳、短 buffer NUL 截斷與 `SIZE_MAX` error contract；不保存 C 指標、不接 production opt-in。
- [x] 擴充 `tests/test_rust_ffi.c`，覆蓋 initialize、notification、missing `jsonrpc` default、string id、tools/call params、invalid JSON/root/method、length-only、short buffer 與 escape contract。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 MCP JSON-RPC summary 只固定 request codec 對照，不代表 Content-Length transport、response formatting、tools/list schema 或 14 tool handlers 遷移。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（4 passed）、`cargo test -p cbm-core --locked`（90 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner mcp`（119 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-06 Phase 3 MCP JSON-RPC parse codec opt-in）

- [x] 擴充 `rust/cbm-core/src/mcp/mod.rs`，新增 production-grade request parse helper，固定 duplicate key first-wins、Unicode escape、invalid UTF-8 rejection、numeric/string/other id、notification 與 `params` raw value slice contract。
- [x] 新增 `cbm_rs_mcp_jsonrpc_parse_v1` 與 `CBM_USE_RUST_MCP_CODEC=1`，讓 `src/mcp/mcp.c` 的 `cbm_jsonrpc_parse()` 可窄範圍委派 Rust；C wrapper 仍配置並釋放 `cbm_jsonrpc_request_t` 欄位，response formatting、Content-Length framing、tool schema 與 handlers 仍留 C。
- [x] 擴充 `tests/test_mcp.c` 與 `tests/test_rust_ffi.c`，覆蓋 duplicate key first-wins、Unicode method/id、invalid UTF-8、other id、length-only、短 buffer 與 invalid args。
- [x] 新增 `rust-mcp-codec-optin-test` 並接入 `rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（6 passed）、`cargo test -p cbm-core --locked`（98 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm build/c/test-runner && build/c/test-runner mcp`（123 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（123 passed）、`make -f Makefile.cbm CBM_USE_RUST_MCP_CODEC=1 build/c/test-runner && build/c/test-runner mcp`（123 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-06 Phase 3 Cypher hop shorthand parity）

- [x] 修正 Rust `parse_hops()`，讓 `*N` 依 C parser contract 表示 `1..N`，並保留 `*` 為 `1..0`、`*..N` 為 `1..N`。
- [x] 擴充 `rust/cbm-core/src/cypher/mod.rs` unit test 與 `tests/test_rust_ffi.c` smoke，覆蓋三種 hop shorthand summary。
- [x] 擴充 `tests/test_cypher_contract.c`，新增 C AST contract `cypher_contract_parser_hop_range_shorthand`。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（10 passed）、`cargo test -p cbm-core --locked`（91 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner cypher_contract`（21 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-05 Foundation YAML matrix gate）

- [x] 將 `tests/test_yaml.c` 加入 `TEST_FOUNDATION_SUITE_SRCS`，讓 fast foundation runner 也覆蓋既有 YAML C contract。
- [x] 在 `tests/test_foundation_main.c` 註冊 `suite_yaml`，保留 parser 實作不變。
- [x] 驗證通過：`make -f Makefile.cbm test-foundation`（314 passed）與 `make -f Makefile.cbm rust-foundation-optin-test`（每輪 foundation runner 314 passed）。

## 本次工作階段（2026-07-05 Foundation YAML Rust test-only parity）

- [x] 新增 `rust/cbm-core/src/foundation/yaml.rs`，以純 Rust module 對齊 C minimal YAML parser contract：NULL/empty/partial len、nested map、string list、inline comment、duplicate key、float/bool default、path overflow 與 top-level list promotion。
- [x] 新增 `cbm_rs_yaml_parse/free/get_str/get_float/get_bool/get_str_list/has` test-only FFI，使用 opaque handle 與 `*const u8 + len` 解析，不接 production opt-in。
- [x] 擴充 `tests/test_rust_ffi.c` YAML smoke，鏡像核心 C contract 並固定 tree-owned string/list pointer lifecycle。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`cargo test -p cbm-core foundation::yaml --locked`（3 passed）、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm test-foundation`（314 passed）與 `make -f Makefile.cbm rust-foundation-optin-test`（每輪 314 passed）。

## 本次工作階段（2026-07-05 Foundation log/profile deterministic parity）

- [x] 新增 `rust/cbm-core/src/foundation/log.rs`，固定 log level/format env parsing、text atom sanitization、JSON string escaping、HTTP path query/fragment stripping 與 HTTP status-to-level contract。
- [x] 新增 `rust/cbm-core/src/foundation/profile.rs`，固定 `CBM_PROFILE` env gate、elapsed us/ms 與 `rate_per_s` 計算；clock、global active state 與 log emission 仍留 C。
- [x] 擴充 `tests/test_rust_ffi.c`，新增 `cbm_rs_log_*` / `cbm_rs_profile_*` FFI smoke。
- [x] 新增 `tests/test_profile.c` 並接入 `test-foundation`，補齊 profile C-side env gate 與 elapsed log shape contract。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`cargo test -p cbm-core foundation::log --locked`（2 passed）、`cargo test -p cbm-core foundation::profile --locked`（2 passed）、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm test-foundation`（316 passed）與 `make -f Makefile.cbm rust-foundation-optin-test`（每輪 316 passed）。

## 本次工作階段（2026-07-05 Foundation compat deterministic parity）

- [x] 新增 `rust/cbm-core/src/foundation/compat.rs`，固定 regex known flags、match cap/status、thread default stack 與 aligned alloc precheck。
- [x] 新增 `cbm_rs_compat_*` test-only FFI smoke，不接 production opt-in。
- [x] 新增 `tests/test_compat.c` C-side contract suite，覆蓋 regex capture/cap、compile error、thread create/join/mutex 與 aligned allocation。
- [x] 將 `profile`、`diagnostics` 與 `compat` suite 接入 full `test-runner` selector，避免只在 `test-foundation` 被覆蓋。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`cargo test -p cbm-core foundation::compat --locked`（2 passed）、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm test-foundation`（322 passed）、`make -f Makefile.cbm rust-foundation-optin-test`（每輪 322 passed）與 `make -f Makefile.cbm build/c/test-runner && build/c/test-runner profile diagnostics compat`（15 passed）。

## 本次工作階段（2026-07-05 Phase 4 FQN project-name parity）

- [x] 新增 `rust/cbm-core/src/pipeline/fqn.rs`，以 byte-level Rust helper 對齊 `cbm_project_name_from_path` 的 sanitizer contract。
- [x] 新增 `cbm_rs_pipeline_project_name_from_path` test-only FFI，沿用 caller-provided buffer、回傳完整輸出長度與短 buffer NUL 結尾慣例。
- [x] 擴充 `tests/test_rust_ffi.c`，覆蓋 NULL/empty、root fallback、Windows drive canonicalization、unsafe ASCII、Unicode byte hex encoding 與 truncation。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`cargo test -p cbm-core pipeline::fqn --locked`（3 passed）、`make -f Makefile.cbm rust-ffi-test` 與 `make -f Makefile.cbm build/c/test-runner && build/c/test-runner fqn`（77 passed）。

## 本次工作階段（2026-07-05 Foundation arena public-struct parity）

- [x] 新增 `rust/cbm-core/src/foundation/arena.rs`，以安全 Rust model 固定 C arena 的 default/min block size、8-byte alignment、growth、reset 與 total accounting。
- [x] 新增 `cbm_rs_arena_init/init_sized/alloc/calloc/strdup/strndup/reset/destroy/total` test-only FFI，直接接受 layout-compatible `CBMArena*`，並以 `tests/test_rust_ffi.c` 鎖定公開 struct 欄位與 ABI 常數。
- [x] 擴充 `tests/test_rust_ffi.c`，覆蓋 null guard、zero-size alloc、alignment、block growth、calloc zeroing、raw `strndup`、reset 保留首塊、destroy 歸零與 double destroy。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core foundation::arena --locked`（3 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm test-foundation`（322 passed）與 `make -f Makefile.cbm build/c/test-runner && build/c/test-runner arena`（31 passed）。

## 本次工作階段（2026-07-05 Phase 4 language graph parity 擴充）

- [x] 在 `scripts/rust-language-graph-parity.py` 的 generated repo 新增 `rs/lib.rs`、`java/App.java` 與 `cpp/app.cpp`，覆蓋 trait/interface、class/struct、method dispatch、constructor/static call、inheritance/implements 與 same-file helper call。
- [x] 擴充 parity summary 的 `interesting` 節點清單，讓 golden 明確記錄 Rust/Java/C++ 的 representative nodes 與 edges。
- [x] 更新 `tests/golden/rust-refactor/language-graph-parity.json` 與 README；coverage 旗標仍全為 true，edge histogram 新增/擴大 `CALLS`、`DEFINES`、`DEFINES_METHOD`、`INHERITS`、`IMPLEMENTS`、`USAGE` 與 `WRITES` 觀測面。
- [x] 驗證通過：`make -f Makefile.cbm rust-language-graph-parity-update`、`python3 scripts/rust-language-graph-parity.py target/rust-parity/cbm-default target/rust-parity/cbm-rust-pipeline`、`python3 -m py_compile scripts/rust-language-graph-parity.py`、`git diff --check`。

## 本次工作階段（2026-07-05 Phase 4 adapter callpoint 收斂）

- [x] 將 `pass_similarity.c` 的 deferred `SIMILAR_TO` merge 改走 `cbm_gbuf_apply_insert_edge`，保留 jaccard/same_file props、dedup 與 total edge count 語意。
- [x] 將 `pass_semantic_edges.c` 的 deferred `SEMANTICALLY_RELATED` merge 改走 `cbm_gbuf_apply_insert_edge`，保留 score/same_file props、worker buffer merge 與 total_edges 語意。
- [x] 將 `pass_enrichment.c` 的 decorator-tag node update 改走 `cbm_gbuf_apply_upsert_node`，保留原本 properties injection 與 node update 語意。
- [x] 將 `pipeline.c` infrascan 路徑 `Route` node upsert（`__route__`、topic route）與 `INFRA_MAPS` edge 改走 `cbm_gbuf_apply_upsert_node` / `cbm_gbuf_apply_insert_edge`，保留既有 fallback 行為與邏輯。
- [x] 將 `pass_githistory.c` `FILE_CHANGES_WITH` edge 與 File temporal props upsert 改走 `cbm_gbuf_apply_insert_edge` / `cbm_gbuf_apply_upsert_node`。
- [x] 將 `pass_route_nodes.c` 的 Route / HANDLES / DATA_FLOWS 上述 node-edge 寫入改走 `cbm_gbuf_apply_upsert_node` / `cbm_gbuf_apply_insert_edge`，保留既有 route 邊緣語意。
- [x] 將 `pass_k8s.c` 的 `Module` / `Resource` / `Chart` / `Package` upsert 與 `IMPORTS` / `DEFINES` / `DEPENDS_ON` / `INFRA_MAPS` edge 寫入改走 `cbm_gbuf_apply_upsert_node` / `cbm_gbuf_apply_insert_edge`。
- [x] 將 `pass_definitions.c` 的 Definition/Channel/EnvVar upsert 與 `DEFINES` / `DEFINES_METHOD` / `EMITS` / `LISTENS_ON` / `CONFIGURES` / `IMPORTS` edge 寫入改走 `cbm_gbuf_apply_upsert_node` / `cbm_gbuf_apply_insert_edge`，保留 registry 與計數語意。
- [x] 將 `pass_calls.c` 的 Route upsert、`CALLS` / `HANDLES` edge、service route node helper、共用 `calls_emit_edge` 與 FastAPI `Depends` edge 寫入改走 `cbm_gbuf_apply_upsert_node` / `cbm_gbuf_apply_insert_edge`，保留既有 edge props 與回傳值語意。
- [x] 將 `pass_semantic.c` 的 `IMPLEMENTS` / `OVERRIDE` / `DECORATES` / `USAGE` / `INHERITS` edge 與 synthetic `Decorator` upsert 改走 `cbm_gbuf_apply_upsert_node` / `cbm_gbuf_apply_insert_edge`，保留 inheritance、decorator 與 implements 計數語意。
- [x] 將 `pass_parallel.c` 的 shared/main `ctx->gbuf` `DEFINES` / `DEFINES_METHOD` / `IMPORTS` / Channel edges、worker-local definition/route/semantic edges，以及 service helper `CALLS` / `HANDLES` / `HTTP_CALLS` / `GRPC_CALLS` / `GRAPHQL_CALLS` / `TRPC_CALLS` 改走 `cbm_gbuf_apply_upsert_node` / `cbm_gbuf_apply_insert_edge`，保留 shared id、route id、line/args props、worker counters 與 edge dedup 語意。
- [x] 新增 `gbuf_mutation_apply_insert_edge_no_endpoint_lookup`，固定 adapter `InsertEdge` 不查端點，避免 worker-local edge buffer 引用 main graph 節點 ID 時被誤擋。
- [x] 更新 graph-buffer adapter static guard，鎖定 `pipeline.c`、`pass_similarity.c`、`pass_semantic_edges.c`、`pass_enrichment.c`、`pass_githistory.c`、`pass_route_nodes.c`、`pass_k8s.c`、`pass_definitions.c`、`pass_calls.c`、`pass_semantic.c` 與 `pass_parallel.c` 不退回代表性 direct `cbm_gbuf_*` call。
- [x] 較早 adapter 批次驗證通過：`build/c/test-runner graph_buffer pipeline`（268 passed）、`make -f Makefile.cbm rust-pipeline-plan-optin-test`（217 passed）、`make -f Makefile.cbm rust-language-graph-parity`、`scripts/lint.sh --ci` 與格式/golden hygiene。
- [x] C/安全回歸已於較早 adapter 批次補跑完成（`CBM_SKIP_RUST=1 scripts/test.sh`，5798 passed；`make -f Makefile.cbm security` 通過）。完整含 Rust 的 `scripts/test.sh` 亦已補跑通過；此紀錄早於 `pass_parallel.c` adapter 收斂。
- [x] sequential pass adapter 驗證通過：`build/c/test-runner graph_buffer pipeline lang_contract edge_structural`（331 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- [x] pass_parallel adapter 驗證通過：`build/c/test-runner graph_buffer parallel edge_types_probe lang_contract`（156 passed）、`make -f Makefile.cbm lint-format`、`git diff --check`，且 `src/pipeline/pass_parallel.c` 已無直接底層 graph-buffer mutation call。

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

- Phase 4 未完成任務 `Pipeline orchestration 全面化` 仍屬多輪工作：目前 registry + plan 決策層、full/incremental extraction 選擇、sequential extraction typed v2 metadata/trace、incremental extract/resolve typed v2 dispatch/trace、parallel extraction typed v2 metadata/trace、full pre-dump typed v2 metadata/trace、typed incremental post/tail dispatch、graph-buffer mutation command contract、多批 C-side adapter 呼叫點（structure、incremental purge/file-upsert/inbound restore、`TESTS`/`TESTS_FILE`、usage/configlink、similarity/semantic/decorator、infra/githistory、k8s/route/definitions/calls/semantic 與 `pass_parallel.c`），以及 language graph parity 的 Rust/Java/C++ 擴充與 incremental dump failure side-effect contract 已在 opt-in/default path 下驗證；C 端仍固定驗證 infra、dump、hash persist、FTS rebuild 與 artifact tail 順序，實際 side effects 與 graph-buffer storage/ownership 仍由既有 C 函式執行，暫不讓 Rust 重排具副作用步驟以符合「簡單優先／外科手術式」原則。
- Phase 5 未完成任務 `切換預設 build path 為 Rust-backed` **刻意尚未執行**：Rust 端目前承接決策層與 graph-buffer mutation command contract，C 端已有多批同步 adapter 呼叫點；實際 pass/extraction/tree-sitter/LSP 與 graph-buffer storage/ownership 仍在 C；提前切換會破壞產品，且違反本計畫「C fallback 需經至少一個 release cycle 驗證」的完成標準。`切換前 packaging/install/UI/security/perf 驗證` 為 Phase 5 清理前置 gate，於切換前不觸發；README/CONTRIBUTING/contributor guide 與 CI 已於本階段更新，release workflow 待預設切換時再調整。

## 本次工作階段（2026-07-02 朝完整移除 C 推進）

- [x] 新增「完整移除 C 的路線圖」：`Rust-Refactor.md` 依子系統排序（foundation 剩餘 → pipeline → store → cypher → mcp → internal/cbm → 預設切換 → 移除）並附最終檢核表；`Tasks.md` 新增對應追蹤 checklist。誠實反映現況（預設 100% 純 C、Rust 仍只佔整體自有程式碼少數、零預設 Rust 取代）。
- [x] 推進 foundation 下一切片：新增 `hash_table` Rust parity module（`rust/cbm-core/src/foundation/hash_table.rs`，純安全 Rust + 4 個單元測試）與 test-only `cbm_rs_ht_*` FFI（`ffi.rs`），並在 `tests/test_rust_ffi.c` 新增 parity 測試，固定 content-key、`get_key` stored 指標（覆寫更新）、NULL value 為存在項、foreach exactly-once、非 UTF-8 高位元 key、null 契約與 resize。維持 C-only hot path，不導入產品 opt-in。
- [x] 嚴謹驗證全綠：`scripts/rust-check.sh`（fmt、clippy `-D warnings`、`cargo test --workspace`、`rust-ffi-test`、foundation opt-in matrix、pipeline registry/plan opt-in、language-graph parity）、`make rust-dangerous-api`（148 findings 全允許清單，新 FFI 由 `ffi.rs` wildcard 覆蓋）、`scripts/lint.sh --ci`（CI 對齊 `cppcheck 2.20.0` + `clang-format 20.1.8`，含 `test_rust_ffi.c`）。
- 說明：此切片為 test-only parity（不改變預設 C 路徑），是既有方法論中 production opt-in 的前置步驟；離「可移除 C」仍需完成路線圖各階段。
