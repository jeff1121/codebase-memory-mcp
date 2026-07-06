# Rust 重構計畫

## 摘要

本文件規劃以分階段、可驗證、低風險方式，將原本以 C 為主的 `codebase-memory-mcp` 逐步重構為 Rust。產品預設路徑目前仍是 C，Rust 僅透過明確 opt-in 參與部分 foundation、store、MCP 與 pipeline 決策/adapter 切片；重點不是一次性全面改寫，而是先建立 Rust 骨架與行為基準，再按模組遷移，確保現有單一靜態執行檔、MCP 介面、SQLite 儲存格式、tree-sitter 支援、CLI 行為與安全稽核流程都能維持相容。

## 目前執行狀態

- Phase 0：已新增 `docs/rust-refactor-baseline.md` 與 `tests/golden/rust-refactor/` 第一批小型 golden fixtures，固定 build/test/lint/security、CLI index/status/list、CLI invocation forms、MCP initialize/tools list pagination/error/Content-Length transcript、alias/schema/indexed-repo tool call transcript、小型 graph histogram、language graph parity、artifact export/bootstrap/import/schema mismatch、小型效能 smoke、手動 repository self-index baseline 與手動大型 synthetic performance baseline。
- Phase 1：已新增 Cargo workspace、`cbm-core` crate、`rust/CBM_FFI.md`、`scripts/rust-check.sh` 與 `Makefile.cbm` 的 `rust-*` targets；預設產品 C binary 仍不連入 Rust，只有明確 opt-in 才連 `cbm-core` staticlib。
- Phase 2：已建立第一批 foundation parity 模組：`dump_verify`、`str_intern`、`str_util` 與窄切 `platform` helpers 的 Rust 實作及單元測試。`dump_verify` 可用 `CBM_USE_RUST_DUMP_VERIFY=1` opt-in，`str_intern` 可用 `CBM_USE_RUST_STR_INTERN=1` opt-in，`str_util` 可用 `CBM_USE_RUST_STR_UTIL=1` opt-in predicate、validator、`json_escape`、路徑 helpers 與 arena-backed 字串 helpers；C wrapper 仍由 `CBMArena` 配置輸出，Rust 只回傳 offset/required length 或寫入呼叫端 buffer。`platform` 路徑正規化、Linux cgroup quota helpers 與 env/dirs helpers 可分別用 `CBM_USE_RUST_PLATFORM_PATH=1`、`CBM_USE_RUST_PLATFORM_CGROUP=1`、`CBM_USE_RUST_PLATFORM_ENV_DIRS=1` opt-in；env/dirs 覆蓋 `cbm_safe_getenv`、home/config/local/cache 優先序、覆寫、截斷與路徑正規化。下一批 foundation 已補 diagnostics deterministic formatter、hash_table、YAML、log/profile、compat 與 arena public-struct test-only Rust parity/FFI fixture；profile env gate 另可用 `CBM_USE_RUST_PROFILE_ENV=1` 窄範圍 opt-in，僅委派 `CBM_PROFILE` 啟用判斷，clock/log/global state 仍留 C。hash_table 與 arena allocator hot path 暫定 C-only，不導入產品 Rust opt-in。預設仍走原 C implementation。
- Phase 4：已啟動低風險 registry 與 plan 切片。registry 已新增 Rust `pipeline::registry` 純決策模組、`cbm_rs_registry_resolve_once` parity fixture，以及產品用的 opaque registry handle FFI。`CBM_USE_RUST_PIPELINE_REGISTRY=1` 現在會讓 `cbm_registry_add` 同步寫入 Rust handle，`cbm_registry_resolve` 在既有 C per-file cache miss 後先嘗試 Rust resolver，再將 Rust 回傳 QN 映射回 C registry-owned key；`label_of`、`find_by_name`、`fuzzy_resolve` 與預設 C path 仍保留。plan 已新增 Rust `pipeline::plan` parity fixture，固定 full pipeline、sequential、parallel extraction、predump、parallel threshold、incremental extract/resolve 與 incremental post/tail typed order。`CBM_USE_RUST_PIPELINE_PLAN=1` 目前讓 C full/incremental extraction phase 讀 Rust plan 來選擇 parallel 或 sequential，並在驗證 Rust typed v2 metadata 後 dispatch incremental extract/resolve：sequential 執行 `definitions`、`calls`、`usages`、`semantic`，parallel 執行 `incr_extract`、`incr_registry`、`incr_resolve`；讓 sequential extraction dispatch 依 Rust typed v2 metadata 執行 `definitions`、`k8s`、`lsp_cross`、`calls`、`usages` 與 `semantic`，讓 parallel extraction 外層 metadata 固定 `parallel_extract`、`registry_build`、`lsp_cross_prepare`、`parallel_resolve`、`infra_routes`、`infra_bindings` 與 `k8s` 的順序與 policy trace，讓 full pipeline pre-dump 階段依 Rust typed v2 predump metadata dispatch `decorator_tags`、`configlink`、`route_match`、`similarity`、`semantic_edges` 與 `complexity`，並讓 typed incremental post steps dispatch `k8s`、`incr_tests`、`incr_decorator_tags`、`incr_configlink`、`incr_similarity`、`incr_semantic_edges`、`edge_relink`、`incremental_dump`、`persist_hashes` 與 `artifact_export`；C 端仍固定驗證 sequential/predump/parallel/incremental extraction `kind/phase/policy/gate_flags/requires_mask/effect_flags` 與 incremental tail 順序/policy，並執行既有 C pass、cache/shared id、cross-LSP cleanup/skip、infra tail、dump、hash persist、FTS rebuild 與 artifact export side effects。inline pass trace contract 會固定 sequential/predump/parallel/incremental extraction dispatch log、`source=typed_v2` 與 pass 順序；incremental dump failure 會原樣傳回錯誤，並跳過後續 `persist_hashes` / `artifact_export` tail。FullPipeline top-level metadata 已新增獨立 FFI，固定 12 個 full/moderate/advanced outer steps、fast-mode 11 steps、`requires_mask`、gate/effect flags 與 nested plan kind；此 API 僅供 metadata/ABI parity，不接 `pipeline.c` / `pipeline_incremental.c` runtime dispatch，且 `PlanStepV2(kind=FullPipeline)` 仍保持 unsupported。graph-buffer mutation command 邊界已新增 Rust test-only metadata/validation ABI，並新增 C-side `graph_buffer_mutation` adapter，讓 full structure、incremental purge/file-upsert、incremental inbound restore、`TESTS`/`TESTS_FILE`、`pass_usages.c` usage/throws/read-write edges、`pass_configlink.c` configlink edges、`pass_similarity.c` `SIMILAR_TO`、`pass_semantic_edges.c` `SEMANTICALLY_RELATED` 與 `pass_enrichment.c` decorator-tag node upsert、`pipeline.c` infra route/topic route node upsert 與 `INFRA_MAPS` edge、`pass_githistory.c` temporal File metadata upsert/file-change edges、sequential `pass_definitions.c`、`pass_calls.c`、`pass_semantic.c`，以及 `pass_parallel.c` 的 shared/main registry、worker-local graph 與 service helper node/edge 寫入都先走 command apply 邊界；Rust 只固定 `UpsertNode`、`InsertEdge` 與 `DeleteByFile` 的欄位、結果型別、effect flags、null/empty string contract，以及 `InsertEdge` 不做 endpoint lookup 的合約，實際 pass function、tree-sitter/extraction/LSP、graph-buffer storage/ownership 與所有寫入仍留在 C。language graph parity gate 已固定 Python、TypeScript、Go、Rust、Java、C++ 與 YAML 代表性 fixture，並比對 default C path 與 Rust pipeline/store opt-in path 的 definitions、calls、imports、routes、semantic/LSP edges。
- 驗證（2026-07-02 baseline）：本機曾完整通過 `scripts/test.sh`、`scripts/rust-check.sh`、`scripts/build.sh`、`scripts/smoke-test.sh build/c/codebase-memory-mcp`、`scripts/smoke-invariants.sh build/c/codebase-memory-mcp`（30 passed / 0 failed）、`make -f Makefile.cbm security`、`make -f Makefile.cbm rust-ci` 與 `make -f Makefile.cbm rust-baseline-fixtures`；`scripts/lint.sh --ci` 也以 CI 對齊的 `cppcheck 2.20.0` + `clang-format 20.1.8` 通過，`RUST_AUDIT_REQUIRED=1` 已用 `cargo-audit v0.22.2` 驗證 `Cargo.lock`。
- 驗證（2026-07-04 Phase 4 targeted gates）：本輪重跑 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::plan --locked`（11 passed）、`cargo test -p cbm-core pipeline::graph_mutation --locked`（3 passed）、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner graph_buffer`（51 passed）、`build/c/test-runner pipeline`（default C path，217 passed）、`make -f Makefile.cbm rust-pipeline-plan-optin-test`（217 passed）、`make -f Makefile.cbm rust-language-graph-parity`、`make -f Makefile.cbm lint-format`、`git diff --check` 與 `git diff --exit-code -- tests/golden/rust-refactor`。完整 `scripts/test.sh` / `scripts/lint.sh --ci` / `make -f Makefile.cbm security` 尚未在本輪 targeted 修補後重跑，仍以 2026-07-02 baseline 為最近完整紀錄。
- 驗證（2026-07-05 Phase 4 targeted gates）：本輪新增 `SIMILAR_TO`、`SEMANTICALLY_RELATED`、decorator-tag upsert、`pipeline.c` infra route/tail (`Route` upsert、`INFRA_MAPS` 及 route-topic map edge)、`pass_githistory.c` FILE_CHANGES_WITH 與 File temporal props 的 C-side adapter callpoint；並重跑 `make -f Makefile.cbm clean-c build/c/test-runner`、`build/c/test-runner graph_buffer pipeline`（268 passed）、`make -f Makefile.cbm rust-pipeline-plan-optin-test`（217 passed）、`make -f Makefile.cbm rust-language-graph-parity`、`scripts/lint.sh --ci`、`cargo fmt --all -- --check`、`make -f Makefile.cbm lint-format`、`git diff --check` 與 `git diff --exit-code -- tests/golden/rust-refactor`。同日稍早 `scripts/test.sh` 曾啟動但依使用者要求中斷，不視為新的完整通過；`make -f Makefile.cbm security` 也尚未在此批 adapter 變更後重跑。
- 驗證（2026-07-05 Phase 4 completion verification）：新增一輪針對新 adapter callpoint 的 C/安全回歸：`CBM_SKIP_RUST=1 scripts/test.sh`（5798 passed）與 `make -f Makefile.cbm security`。其中 `security` 全量檢核完成 1/1、1/1、2/2、3/1、4/1、5/1、6/1、7/1、8/1；`security-ui` 全通過、`security-fuzz` `23/23`、並完成 vendored 完整性核對；`scripts/lint.sh --ci` 亦在同日通過。`CBM_SKIP_RUST=1` 會跳過 `scripts/test.sh` 的 Step 7 Rust parity，因此另補跑無 skip 的完整 `scripts/test.sh`：C runner 5798 passed、parent-death watchdog 通過、security-string 2 passed / 0 failed、Rust unit tests 51 passed、registry opt-in suite 53 passed、pipeline plan opt-in suite 217 passed，最終 `=== All tests passed ===`。
- 驗證（2026-07-05 Phase 4 sequential pass adapter gates）：新增 `pass_definitions.c`、`pass_calls.c`、`pass_semantic.c` adapter callpoint 收斂，並重跑 `build/c/test-runner graph_buffer pipeline lang_contract edge_structural`（331 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。direct-call search 確認這三個檔案不再直接呼叫 `cbm_gbuf_upsert_node` / `cbm_gbuf_insert_edge`；當時留待獨立批次的 `pass_parallel.c` 已由下一條 pass_parallel adapter gates 覆蓋。
- 驗證（2026-07-05 Phase 4 pass_parallel adapter gates）：新增 `pass_parallel.c` shared/main `ctx->gbuf`、worker-local `ws->local_gbuf` / `ws->local_edge_buf` 與 service-helper 裸 `gbuf` callpoint 收斂，並新增 adapter `InsertEdge` 不查端點的 contract test。重跑 `build/c/test-runner graph_buffer parallel edge_types_probe lang_contract`（156 passed）、`make -f Makefile.cbm lint-format`、`git diff --check`，且 direct-call search 確認 `src/pipeline/pass_parallel.c` 已無 `cbm_gbuf_upsert_node` / `cbm_gbuf_insert_edge`。
- 驗證（2026-07-05 Phase 4 language graph parity expansion）：新增 Rust、Java 與 C++ 小型 fixture，擴充 `language-graph-parity.json` 的 CALLS、DEFINES、DEFINES_METHOD、INHERITS/IMPLEMENTS、USAGE/WRITES 與 LSP strategy 觀測面。`make -f Makefile.cbm rust-language-graph-parity-update` 已確認 default C path 與 Rust pipeline/store opt-in path 完全一致並更新 golden；再以 `python3 scripts/rust-language-graph-parity.py target/rust-parity/cbm-default target/rust-parity/cbm-rust-pipeline` 非 update 驗證 golden 可重讀比對，並通過 `python3 -m py_compile scripts/rust-language-graph-parity.py` 與 `git diff --check`。
- CI 接入（2026-07-02）：新增 `.github/workflows/_rust.yml` 專責 Rust gates（lint / unit / FFI / foundation 與 pipeline registry+plan opt-in / language-graph parity），並於 `pr.yml` 納入 `rust` job 與 `ci-ok` 必要脈絡；`_security.yml` 既有的 `rust-dangerous-api` 與 `cargo-audit` fail-closed 維持不變。新增 `rust-ci-tests` make 目標作為 CI 用的測試/parity 子集。
- 剩餘範圍（刻意保留）：Phase 4 的 pipeline orchestration 全面遷移與 Phase 5 的「切換預設為 Rust-backed」尚未執行——Rust 端目前承接 registry/plan 決策層與 graph-buffer mutation command contract，C 端已有多批同步 command adapter callpoint；實際 pass/extraction/tree-sitter/LSP 與 graph-buffer storage/ownership 仍在 C；提前切換會破壞產品並違反本計畫「C fallback 需經至少一個 release cycle」的完成標準，詳見 `Tasks.md`。
    - 下一批 foundation 評估：`platform` env/dirs 已完成 Rust ABI、C opt-in 與 parity fixture。`hash_table` 已補 C contract/stress tests，但因 borrowed key pointer、`get_key` stored pointer、NULL value 與 foreach callback contract，目前標記為 C-only hot path；若未來試作 Rust 版，先走 test-only `cbm_rs_ht_*` parity fixture。`diagnostics` 已補 deterministic query stats、env parser、path、JSON 與 NDJSON 的 Rust parity/FFI fixture；writer thread、檔案 rotation、system metrics 與 stderr lifecycle 仍留 C。`hash_table` 已補 test-only `cbm_rs_ht_*` parity fixture（`foundation::hash_table` 純安全 Rust module + FFI + `tests/test_rust_ffi.c` parity），固定 borrowed key pointer、`get_key` stored pointer、NULL value 與 foreach callback contract；仍維持 C-only hot path，不導入產品 opt-in。YAML 已將既有 C contract suite 納入 `test-foundation` 與 `rust-foundation-optin-test` matrix，並新增 `foundation::yaml` test-only Rust parity module 與 `cbm_rs_yaml_*` FFI smoke，固定 minimal YAML parser/query contract；仍不導入產品 opt-in。profile env gate 已新增 `CBM_USE_RUST_PROFILE_ENV=1`，只委派 `CBM_PROFILE` 啟用判斷；profile clock、global state 與 log emission 仍留 C。Phase 3 的 store/cypher/MCP 已整理於 `docs/rust-refactor-phase3-plan.md`；`store_compat` contract fixture 已凍結 minimal schema/index existence、`edges.url_path_gen`、user index drop/create symmetry、`open_path_query` no-create/read-only、WAL journal mode、readonly WAL read/write rejection、bulk begin/end pragma 不離開 WAL、generated column query plan、URL path API 的 project-scoped substring 行為，以及 `nodes_fts` 手動 rebuild/camelCase tokenization；同時修正 `idx_edges_url_path` 未被 drop 的不對稱。Store 已新增 Rust static `schema_manifest` test-only helper 與 V1/V2 schema FFI smoke，固定核心表、`nodes_fts`、`edges.url_path_gen`、9 個 user index 的名稱/表/欄位/旗標、table column/index layout 與 FTS shadow metadata；另新增 test-only connection/pragma manifest FFI，固定 `open_path_query` no-create/read-only、immutable fallback、WAL、mmap env 與 bulk pragma contract；FTS tokenizer、mmap resolver、immutable URI builder、search pattern helper 與 architecture helper 已有窄範圍 opt-in，前四者分別只委派 token 字串、env 字串解析、readonly fallback URI 字串建構與 glob/LIKE/case flag byte-level helper；architecture helper 固定 qn_to_package/qn_to_top_package、is_test_file_path、hop_to_risk 與 risk_label 契約。SQLite open/probe/pragma/FTS/CRUD/search runtime 仍留 C。`cypher_contract` 已擴充 CALL、EXISTS、malformed 與 aggregation exact contract，包含 `CALL db.labels()`、`EXISTS { ... }`、`EXISTS (f)` 與 `COUNT(SUM/AVG/MIN/MAX)` 輸出語意；同時保留既有 OPTIONAL MATCH、UNION/UNION ALL、edge property、deterministic ordered rows、WITH/post-WHERE AST 與 explicit `LIMIT 0` 契約。Cypher 已新增 Rust lexer/token summary 與 normalized AST summary test-only parity FFI，固定 token metadata、parser AST shape、OPTIONAL MATCH、UNION ALL、EXISTS direction、WITH/post-WHERE 與 caller-buffer summary contract；production parser、AST normalizer、evaluator、executor 與 production opt-in 仍未遷移。`mcp` suite 已凍結 BM25 `file_pattern` 與 label boost rank/order contract；MCP golden 已凍結 alias、tool schema 與 indexed repo `query_graph` transcript；request parse 已可用 `CBM_USE_RUST_MCP_CODEC=1` 窄範圍委派 Rust，response formatting、Content-Length transport、tool schema 與 handlers 仍留 C。
- 驗證（2026-07-05 Phase 3 Store schema manifest helper）：新增 `rust/cbm-core/src/store/schema_manifest.rs` 與 `cbm_rs_store_schema_manifest_*_v1` test-only FFI；已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core store::schema_manifest --locked`（2 passed）、`cargo test -p cbm-core --locked`（53 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm lint-format`、`make -f Makefile.cbm rust-ffi-test` 與 `build/c/test-runner store_compat`（8 passed）。此批只凍結 manifest/ABI contract，尚未遷移 SQLite open/pragma/CRUD/runtime。
- 驗證（2026-07-05 Phase 3 Store connection/pragma manifest helper）：新增 test-only `cbm_rs_store_connection_manifest_*_v1` FFI，並補 `store_compat_bulk_pragmas_preserve_wal_contract`。本輪通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core store::schema_manifest --locked`（4 passed）、`make -f Makefile.cbm rust-ffi-test` 與 `make -f Makefile.cbm build/c/test-runner && build/c/test-runner store_compat`（11 passed）。此批只凍結 open/pragma contract，尚未遷移 SQLite open/pragma runtime。
- 驗證（2026-07-05 Phase 3 Store FTS camelCase tokenization helper）：新增 `store::tokenization` Rust helper 與 `cbm_rs_store_camel_split_v1` FFI smoke，固定 `cbm_camel_split` 的原 identifier + split identifier、ASCII lower→upper、acronym boundary、NULL/empty、非 ASCII byte 不觸發大小寫邊界、2048-byte fallback/truncation 與 caller-buffer truncation contract。`CBM_USE_RUST_STORE_FTS_TOKENIZER=1` 只讓 C 註冊的 SQLite scalar UDF callback 將 token 字串計算委派給 Rust；SQLite open/pragma/schema、UDF registration、FTS5 runtime、`sqlite3_result_text`、CRUD/search/BM25 與 Store ownership 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core store::tokenization --locked`（3 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm build/c/test-runner && build/c/test-runner store_compat`（12 passed，含 direct scalar UDF exact contract）與 `make -f Makefile.cbm rust-store-fts-tokenizer-optin-test`（`store_compat mcp` 131 passed）。新 target 已接入 `rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7；language graph parity 的 Rust opt-in binary 也同時帶 `CBM_USE_RUST_STORE_FTS_TOKENIZER=1` 作為組合 smoke。
- 驗證（2026-07-06 Phase 3 Store mmap resolver opt-in）：新增 `store::pragmas` Rust helper、`cbm_rs_store_resolve_mmap_size_value_v1` FFI smoke 與 `CBM_USE_RUST_STORE_MMAP_RESOLVER=1` 窄範圍 opt-in；只委派 `CBM_SQLITE_MMAP_SIZE` 字串解析，SQLite open、pragma dispatch、readonly/write 分流與 SQL execution 仍留 C。同時補 C resolver `errno == ERANGE` contract，固定 unset/empty/malformed/partial/trailing/overflow 回 64 MiB、合法負值 clamp 0、leading whitespace 與 `+` 可接受。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core store::pragmas --locked`（1 passed）、`cargo test -p cbm-core --locked`（92 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm build/c/test-runner && build/c/test-runner store_pragmas store_compat`（23 passed）與 `make -f Makefile.cbm rust-store-mmap-resolver-optin-test`（23 passed）。新 target 已接入 `rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7。
- 驗證（2026-07-06 Phase 3 Store immutable URI opt-in）：新增 `store::open` Rust helper、`cbm_rs_store_build_immutable_uri_v1` FFI smoke 與 `CBM_USE_RUST_STORE_IMMUTABLE_URI=1` 窄範圍 opt-in；只委派 SQLite readonly immutable fallback URI 字串建構，SQLite open/probe、readonly no-create、WAL/pragma dispatch、UDF/FTS/search/CRUD 與 Store lifetime 仍留 C。FFI smoke 固定 POSIX/relative/root/Windows/UNC path、安全字元、space、`%`、`?`、`#`、`&`、`=`、UTF-8 raw bytes、NULL、length-only 與短 buffer 截斷。通過 `cargo test -p cbm-core store::open --locked`（3 passed）、`make -f Makefile.cbm rust-ffi-test` 與 `make -f Makefile.cbm rust-store-immutable-uri-optin-test`（`store_compat mcp` 131 passed）。新 target 已接入 `rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7。
- 驗證（2026-07-06 Phase 3 Store search pattern opt-in）：新增 `store::search_pattern` Rust helper、glob/case flag/LIKE hint FFI smoke 與 `CBM_USE_RUST_STORE_SEARCH_PATTERN=1` 窄範圍 opt-in；只委派 `cbm_glob_to_like()`、`cbm_extract_like_hints()`、`cbm_ensure_case_insensitive()` 與 `cbm_strip_case_flag()` 的 byte-level helper，SQL builder、LIKE bind lifetime、REGEXP UDF、FTS/BM25、SQLite statement lifecycle 與 Store runtime 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core store::search_pattern --locked`（4 passed）、`cargo test -p cbm-core --locked`（102 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm build/c/test-runner && build/c/test-runner store_search mcp`（191 passed）、`make -f Makefile.cbm rust-store-search-pattern-optin-test`（191 passed）與 `make -f Makefile.cbm lint-format`。新 target 已接入 `rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7。
- 驗證（2026-07-05 Foundation YAML matrix gate）：`tests/test_yaml.c` 已納入 `test-foundation` source list 與 runner；`make -f Makefile.cbm test-foundation` 目前為 314 passed，`make -f Makefile.cbm rust-foundation-optin-test` 亦通過，讓既有 Rust foundation opt-in matrix 同時覆蓋 YAML C contract。
- 驗證（2026-07-05 Phase 3 Cypher contract 補洞）：`make -f Makefile.cbm build/c/test-runner && build/c/test-runner cypher_contract`（20 passed）。
- 驗證（2026-07-06 Phase 3 Cypher lexer parity）：新增 `rust/cbm-core/src/cypher/mod.rs`、`cbm_rs_cypher_lex_*_v1` test-only FFI 與 `tests/test_rust_ffi.c` smoke，固定 token metadata、summary truncation、string/comment/number/DOTDOT contract 與 C enum ABI。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（3 passed）、`cargo test -p cbm-core --locked`（80 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner cypher_contract`（20 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。同日 targeted pipeline 驗證通過：`build/c/test-runner pipeline graph_buffer lang_contract edge_structural`（332 passed）、`make -f Makefile.cbm rust-pipeline-plan-optin-test`（217 passed）與 `make -f Makefile.cbm rust-language-graph-parity`；共享 `build/c` 的 `clean-c` Make 目標需序列執行，不能並行。
- 驗證（2026-07-06 Phase 3 Cypher AST summary parity）：新增 `cbm_rs_cypher_parse_summary_v1` test-only FFI 與 Rust normalized AST summary helper，固定 parser AST shape、OPTIONAL MATCH optional flags、UNION ALL linked shape、EXISTS inbound direction、WITH/post-WHERE 與 malformed parse failure contract。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（6 passed）、`cargo test -p cbm-core --locked`（83 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner cypher_contract`（20 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。此批不接 production opt-in，C parser/evaluator/executor 仍是產品路徑。
- 驗證（2026-07-06 Phase 3 MCP JSON-RPC request summary parity）：新增 `rust/cbm-core/src/mcp/mod.rs`、`cbm_rs_mcp_jsonrpc_request_summary_v1` test-only FFI 與 `tests/test_rust_ffi.c` smoke，固定 C JSON-RPC parser 的 `jsonrpc` 預設值、string/numeric/other id 分類、notification 無 id、`params` kind、invalid JSON/root/missing method、whitespace 與 string escape summary contract。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（4 passed）、`cargo test -p cbm-core --locked`（90 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner mcp`（119 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- 驗證（2026-07-06 Phase 3 MCP JSON-RPC parse codec opt-in）：擴充 MCP Rust parser 與新增 `cbm_rs_mcp_jsonrpc_parse_v1`，並以 `CBM_USE_RUST_MCP_CODEC=1` 讓 `cbm_jsonrpc_parse()` 可窄範圍委派 Rust request envelope parse；C wrapper 仍配置/釋放 `cbm_jsonrpc_request_t` 欄位，response formatting、Content-Length transport、tool schema、tool dispatch/handlers、stdout/stderr 與 server lifecycle 仍留 C。新增 contract 固定 duplicate key first-wins、Unicode escape、invalid UTF-8 rejection、numeric/string/other id、notification 與 `params` raw value slice。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（6 passed）、`cargo test -p cbm-core --locked`（98 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm build/c/test-runner && build/c/test-runner mcp`（123 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（123 passed）、`make -f Makefile.cbm CBM_USE_RUST_MCP_CODEC=1 build/c/test-runner && build/c/test-runner mcp`（123 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。新 target 已接入 `rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7。
- 驗證（2026-07-06 Phase 3 Cypher hop shorthand parity）：修正 Rust normalized AST summary 的 relationship hop shorthand，讓 `*N` 與 C parser 一致輸出 `1..N`，並固定 `*` 為 `1..0`、`*..N` 為 `1..N`。新增 C parser AST contract 與 FFI smoke。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（10 passed）、`cargo test -p cbm-core --locked`（91 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner cypher_contract`（21 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。此批仍不接 production opt-in。
- 驗證（2026-07-05 Foundation YAML Rust test-only parity）：新增 `rust/cbm-core/src/foundation/yaml.rs`、`cbm_rs_yaml_*` test-only FFI 與 `tests/test_rust_ffi.c` smoke，固定 NULL/empty/partial len、nested map、string list、inline comments、duplicate key、float/bool default 與 path overflow contract；通過 `cargo fmt --all -- --check`、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`cargo test -p cbm-core foundation::yaml --locked`（3 passed）、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm test-foundation`（314 passed）與 `make -f Makefile.cbm rust-foundation-optin-test`（每輪 314 passed）。此批不接 production opt-in，預設產品仍走 C YAML parser。
- 驗證（2026-07-05 Foundation log/profile deterministic parity）：新增 `foundation::log` 與 `foundation::profile` Rust test-only helper、`cbm_rs_log_*` / `cbm_rs_profile_*` FFI smoke，固定 log level/format env parsing、text atom sanitization、JSON string escaping、HTTP path/status level、profile env gate、elapsed us/ms 與 rate 計算；另新增 `tests/test_profile.c` C-side contract 並接入 foundation runner。通過 `cargo fmt --all -- --check`、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`cargo test -p cbm-core foundation::log --locked`（2 passed）、`cargo test -p cbm-core foundation::profile --locked`（2 passed）、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm test-foundation`（316 passed）與 `make -f Makefile.cbm rust-foundation-optin-test`（每輪 316 passed）。stderr/sink lifecycle、clock、global state 與 production opt-in 仍留 C。
- 驗證（2026-07-06 Foundation profile env opt-in）：新增 `CBM_USE_RUST_PROFILE_ENV=1`，讓 `cbm_profile_init()` 只委派 Rust 判斷 `CBM_PROFILE` env gate，預設 C path 與 profile runtime/logging 保持不變。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core foundation::profile --locked`（2 passed）、`cargo test -p cbm-core --locked`（93 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm test-foundation`（322 passed）、`make -f Makefile.cbm CBM_USE_RUST_PROFILE_ENV=1 test-foundation`（322 passed）、`make -f Makefile.cbm rust-foundation-optin-test`（每輪 322 passed，含 profile env 單一與全量 opt-in）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- 驗證（2026-07-05 Foundation compat deterministic parity）：新增 `foundation::compat` Rust test-only helper、`cbm_rs_compat_*` FFI smoke 與 `tests/test_compat.c` C-side contract，固定 regex flags/cap/status、thread default stack 與 aligned allocation precheck；同時將 `profile`、`diagnostics` 與 `compat` suite 接入 full `test-runner` selector。通過 `cargo fmt --all -- --check`、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`cargo test -p cbm-core foundation::compat --locked`（2 passed）、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm test-foundation`（322 passed）、`make -f Makefile.cbm rust-foundation-optin-test`（每輪 322 passed）與 `make -f Makefile.cbm build/c/test-runner && build/c/test-runner profile diagnostics compat`（15 passed）。filesystem/process spawning/thread runtime 與 production opt-in 仍留 C。
- 驗證（2026-07-05 Phase 4 FQN project-name parity）：新增 `pipeline::fqn` Rust test-only helper 與 `cbm_rs_pipeline_project_name_from_path` FFI smoke，固定 `cbm_project_name_from_path` 的 NULL/empty/root fallback、separator/drive normalization、unsafe ASCII sanitizer、`..`/`--` collapse、non-ASCII byte lowercase hex encoding 與 caller-buffer truncation contract。通過 `cargo fmt --all -- --check`、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`cargo test -p cbm-core pipeline::fqn --locked`（3 passed）、`make -f Makefile.cbm rust-ffi-test` 與 `make -f Makefile.cbm build/c/test-runner && build/c/test-runner fqn`（77 passed）。此批不接 production opt-in，project key、DB/cache key 與 MCP session path 仍走 C。
- 驗證（2026-07-06 Phase 4 FullPipeline top-level metadata FFI）：新增 `cbm_rs_pipeline_full_plan_step_count_v1` / `cbm_rs_pipeline_full_plan_steps_v1`，固定 FullPipeline 外層 metadata、fast-mode gating、nested plan kind 與 ABI layout；`PlanStepV2(kind=FullPipeline)` 仍回傳 unsupported，且 `pipeline.c` / `pipeline_incremental.c` 有 static guard 確認不消費新 API。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::plan --locked`（12 passed）、`cargo test -p cbm-core --locked`（93 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test` 與 `build/c/test-runner graph_buffer pipeline`（269 passed）。
- 驗證（2026-07-05 Foundation arena public-struct parity）：新增 `foundation::arena` Rust test-only helper 與 `cbm_rs_arena_*` FFI smoke，直接以 layout-compatible `CBMArena*` 固定 default/min block size、8-byte alignment、growth、calloc、`strdup`/`strndup`、reset 保留首塊、destroy 歸零、total accounting 與 null-safe FFI guard。Rust 配置的 blocks 必須以 Rust `cbm_rs_arena_reset/destroy` 釋放，不可與 C `cbm_arena_destroy` 混用；此批不接 production opt-in，產品 allocator hot path 仍走 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core foundation::arena --locked`（3 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm test-foundation`（322 passed）與 `make -f Makefile.cbm build/c/test-runner && build/c/test-runner arena`（31 passed）。
- 驗證（2026-07-05 Foundation mem pressure parity）：新增 `foundation::mem` Rust test-only helper 與 `cbm_rs_mem_*` FFI smoke，固定 `CBM_MEM_BUDGET_MB` override、`worker_budget`、`over_budget`、`peak_rss` 與 `collect` contract；不接產品 opt-in，僅作為 C path 的行為觀測對照。通過 `cargo test -p cbm-core foundation::mem --locked`（4 passed）、`make -f Makefile.cbm rust-ffi-test`、`cargo fmt --all -- --check`、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings` 與 `git diff --check`。
- 驗證（2026-07-06 本機 final gate 補強）：本輪重跑 `scripts/build.sh`、`scripts/build.sh --with-ui`、`scripts/smoke-test.sh build/c/codebase-memory-mcp`、`scripts/smoke-invariants.sh build/c/codebase-memory-mcp`（30 passed / 0 failed）、`scripts/lint.sh --ci`、`make -f Makefile.cbm security` 與完整 `scripts/test.sh`。`scripts/test.sh` 最終 `=== All tests passed ===`，包含 Rust unit 86 passed、registry opt-in 53 passed、pipeline plan opt-in 217 passed、Store FTS tokenizer opt-in 的 `store_compat mcp` 131 passed；`security` 含 security-fuzz 23/23、vendored integrity、Rust allowlist 與 cargo-audit 掃描均通過。此 gate 證明本機 build/test/lint/smoke/security 狀態，但尚不代表跨平台、package wrappers、release packaging、效能門檻、預設 Rust-backed 切換或 C fallback release cycle 已完成。

## 本次工作階段（2026-07-06 Phase 3 Cypher scalar func canonical opt-in）

- 新增 `rust/cbm-core/src/cypher/mod.rs` 的 `scalar_func_index(input)`，以 ASCII 大小寫不敏感（對齊 C `cyp_ci_eq`）比對 `SCALAR_FUNC_NAMES`（labels/type/id/keys/properties/toInteger/toFloat/toBoolean/size/length/trim/ltrim/rtrim/reverse），回傳符合的索引或 `None`。
- 新增 `cbm_rs_cypher_scalar_func_index_v1(input)` FFI（回傳索引或 -1），並以 `CBM_USE_RUST_CYPHER_SCALAR_FUNC=1` 讓 `src/cypher/cypher.c` `scalar_func_canonical()` 委派 Rust 索引後，映射回其自身 static `names[]` 字串，完整保留原本回傳 static 指標的語意；`cyp_ci_eq` 於他處仍使用，無 unused-function。此為 Cypher 第一個 production opt-in；parser/AST normalizer/evaluator/executor 仍留 C。
- 新增 `tests/test_rust_ffi.c` 14 項 FFI ABI smoke（大小寫、未知、長度不符、null/empty、由別處處理的 toLower/toString）。opt-in matrix target `rust-cypher-scalar-func-optin-test` 已接入 `.PHONY`/`rust-ci`/`rust-ci-tests`/`scripts/rust-check.sh`/`scripts/test.sh`。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（11 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner cypher cypher_contract`（164 passed）、`make -f Makefile.cbm rust-cypher-scalar-func-optin-test`（164 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-06 Phase 3 MCP tools/list cursor offset opt-in）

- 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `tools_cursor_offset(params_json, tool_count)`，重用既有 JSON parser 驗證整份 params（含尾端殘留檢查，對齊 yyjson_read），擷取首個 `cursor` 字串，並以 `strtol_full_nonneg` 鏡像 C `strtol` 搭配 `*endptr=='\0' && errno==0 && parsed>=0` 語意（前導空白、正負號、完整消耗、i64 溢位、非負、`-0` 視為 0），最後 clamp 到 tool_count。
- 新增 `cbm_rs_mcp_tools_cursor_offset_v1(params_json, tool_count)` FFI，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `mcp_tools_cursor_offset()` 委派 Rust（傳入 `TOOL_COUNT`）；response serialization、Content-Length framing、tool schema 與 14 handlers 仍留 C。
- 凍結 contract：`tests/test_mcp.c` 新增 `server_handle_tools_list_cursor_edge_cases`（cursor="0" 等同第一頁；無效字串與負數 cursor → offset=TOOL_COUNT 空頁），於預設 C 與 opt-in 皆執行以確保 end-to-end 平價；`tests/test_rust_ffi.c` 新增 27 項 FFI ABI smoke。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（7 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（124 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（124 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-06 Phase 3 Store arch path scope opt-in）

- 新增 `rust/cbm-core/src/store/arch_helpers.rs` 的 `normalize_arch_path(path, cap)` 純 helper，對齊 `src/store/store.c` `arch_path_prepare` 的 norm_out 正規化：跳過前導空白、去一次 `./`、去前導 `/`、以 `cap-1` 截斷（對齊 C `strncpy`），再去尾端 ` `/`\t`/`/` 並折疊重複 `/`；path 未設定或正規化後為空回傳 `None`。截斷刻意先於 strip/collapse，完全對齊 C。
- 新增 `cbm_rs_store_normalize_arch_path_v1(norm_out, norm_sz, path)` FFI（caller-buffer contract，回傳寫入長度或 `usize::MAX`），並以 `CBM_USE_RUST_STORE_ARCH_PATH_SCOPE=1` 讓 `arch_path_prepare` 只委派 norm_out 計算；`like_out` 的 `%s/%%` 組裝、SQLite scope query（`file_path = ? OR file_path LIKE ?`）與 bind 仍留 C。`arch_path_is_set` 以 `#ifndef` 守衛，避免 opt-in 下 `-Werror=unused-function`。
- 新增 `tests/test_rust_ffi.c` `test_store_arch_path_scope_exports` ABI smoke（含 null/empty/全空白/`./`/多斜線、null out、`norm_sz==0` 與截斷先於 strip/collapse），並新增 `rust-store-arch-path-scope-optin-test` target，接入 `.PHONY`、`rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core store::arch_helpers --locked`（5 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner store_arch store_search store_compat mcp`（257 passed）、`make -f Makefile.cbm rust-store-arch-path-scope-optin-test`（245 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。SQLite open/probe/pragma/FTS/CRUD/search runtime 仍全留 C。

## 本次工作階段（2026-07-06 Phase 4 language graph parity JavaScript 擴充）

- 於 `scripts/rust-language-graph-parity.py` 的 `create_repo` 新增 JavaScript 小型 fixture（`js/service.js`、`js/server.js`），涵蓋 class 方法呼叫、`function` 定義、CommonJS `require`/`module.exports` import 與 express `/tasks/:id` route；並將 `JsWorker`、`jsHelper`、`jsStart` 納入 `interesting` 觀測名單。此為 roadmap 第 6 節（internal/cbm）「先覆蓋高流量語言」的前置 gate 切片：JavaScript 為計畫明列高流量語言中尚未覆蓋者（py/ts/go/rust/java/cpp 與 K8s YAML 已覆蓋）。
- 僅動 parity harness fixture 與 golden，未變更任何產品原始碼、`ffi.rs`、`Makefile.cbm` 或 opt-in 旗標；預設 C 路徑與 Rust pipeline opt-in 路徑（`CBM_USE_RUST_PIPELINE_REGISTRY=1 CBM_USE_RUST_PIPELINE_PLAN=1 CBM_USE_RUST_STORE_FTS_TOKENIZER=1`）的 graph 完全一致。
- 驗證通過：`make -f Makefile.cbm rust-language-graph-parity-update` 重生 golden（内含 default==rust 精確比對，且 harness 檢查無本機/暫存路徑外洩），`python3 scripts/rust-language-graph-parity.py target/rust-parity/cbm-default target/rust-parity/cbm-rust-pipeline` 非 update 決定性驗證，`python3 -m py_compile scripts/rust-language-graph-parity.py` 與 `git diff --check`。

## 本次工作階段（2026-07-05 Foundation mem pressure parity）

- 新增 `rust/cbm-core/src/foundation/mem.rs`，提供純值 helper 對齊 `src/foundation/mem.c` 的 budget/pressure contract。
- 新增 `cbm_rs_mem_init`、`cbm_rs_mem_rss`、`cbm_rs_mem_peak_rss`、`cbm_rs_mem_budget`、`cbm_rs_mem_over_budget`、`cbm_rs_mem_worker_budget`、`cbm_rs_mem_collect` test-only FFI 與 `tests/test_rust_ffi.c` `mem` smoke（含 env override、worker 整除、RSS 峰值關係與 `over_budget` 觀測）。
- 驗證通過：`cargo test -p cbm-core foundation::mem --locked`（4 passed）、`make -f Makefile.cbm rust-ffi-test`、`cargo fmt --all -- --check`、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings` 與 `git diff --check`。

## 本次工作階段（2026-07-05 Phase 3 Cypher contract 補洞）

- `tests/test_cypher_contract.c` 已補齊 CALL、EXISTS（含 outbound、inbound、any-direction）與 malformed 契約，以及聚合 contract（`COUNT/DISTINCT`、`SUM/AVG/MIN/MAX`）。
- 本階段僅補齊 C-side contract，尚未展開 Rust parser / evaluator / executor 遷移；預計作為下一輪 Rust cypher 小切片的輸入。

## 本次工作階段（2026-07-06 Phase 3 Cypher lexer test-only parity）

- 新增 `rust/cbm-core/src/cypher/mod.rs`，以 byte-level Rust lexer 對齊 C lexer 的 keyword lookup、symbol、comments、number / `..`、string escape 與 EOF offset contract。
- 新增 `cbm_rs_cypher_lex_token_count_v1`、`cbm_rs_cypher_lex_tokens_v1` 與 `cbm_rs_cypher_lex_summary_v1` test-only FFI；只輸出 scalar metadata 與 caller-buffer summary，不保存 C 指標、不接 production opt-in。
- 擴充 `tests/test_rust_ffi.c` 與 `rust/CBM_FFI.md`，鎖定 C enum ABI、null/short buffer、token offset/length 與 summary contract。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（3 passed）、`cargo test -p cbm-core --locked`（80 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner cypher_contract`（20 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-06 Phase 3 Cypher AST summary test-only parity）

- 擴充 `rust/cbm-core/src/cypher/mod.rs`，在既有 lexer tokens 上新增最小 normalized AST summary helper，覆蓋 pattern nodes/rels、relationship direction/hops、AND/OR/NOT precedence、EXISTS、WITH/post-WHERE、RETURN 與 UNION ALL shape。
- 新增 `cbm_rs_cypher_parse_summary_v1` test-only FFI，沿用 caller-provided buffer、完整長度回傳、短 buffer NUL 截斷與 `SIZE_MAX` error contract；不保存 C 指標、不回傳 Rust-owned AST、不執行 query、不接 production opt-in。
- 擴充 `tests/test_rust_ffi.c`，覆蓋 parser AST shape、OPTIONAL MATCH、UNION ALL、EXISTS inbound direction、WITH/post-WHERE、null input、negative len、short buffer 與 malformed query。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（6 passed）、`cargo test -p cbm-core --locked`（83 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner cypher_contract`（20 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-06 Cypher AST summary edge-case parity）

- 擴充 `rust/cbm-core/src/cypher/mod.rs` parser 測試，新增 `UNION` 非 `ALL` 的 `distinct` 與 `EXISTS (f)` 失敗路徑；並新增 `WITH` / `RETURN` `DISTINCT` `parse_summary` 支援與輸出驗證。
- 擴充 `tests/test_rust_ffi.c`，同步加入 `cypher_parse_summary` `UNION` distinct、`EXISTS (f)` 與 `WITH`/`RETURN DISTINCT` 契約 smoke 測試，沿用既有 `SIZE_MAX` 失敗與 buffer 契約。

## 本次工作階段（2026-07-06 Phase 3 MCP JSON-RPC request summary parity）

- 新增 `rust/cbm-core/src/mcp/mod.rs`，以無第三方 dependency 的 byte-level parser 固定 JSON-RPC request envelope summary contract。
- 新增 `cbm_rs_mcp_jsonrpc_request_summary_v1` test-only FFI，沿用 caller-provided buffer、完整長度回傳、短 buffer NUL 截斷與 `SIZE_MAX` error contract；不保存 C 指標、不回傳 Rust-owned response、不接 production opt-in。
- 擴充 `tests/test_rust_ffi.c` 與 `rust/CBM_FFI.md`，覆蓋 initialize、notification、missing `jsonrpc` default、string id、tools/call params、invalid JSON/root/method 與 delimiter escape。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（4 passed）、`cargo test -p cbm-core --locked`（90 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner mcp`（119 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-06 Phase 3 MCP JSON-RPC parse codec opt-in）

- 新增 `cbm_rs_mcp_jsonrpc_parse_v1` 與 `CBM_USE_RUST_MCP_CODEC=1`，讓 `src/mcp/mcp.c` 的 `cbm_jsonrpc_parse()` 可委派 Rust request envelope parser。
- C wrapper 仍負責 `malloc` / `free` ownership，`params_raw` 為原始 JSON value slice 的 C-owned copy；response formatting、Content-Length framing、tool schema 與 14 個 handlers 不在本切片內。
- 新增 duplicate key first-wins、Unicode method/id、invalid UTF-8、other id、length-only、短 buffer 與 invalid args contract；`rust-mcp-codec-optin-test` 已接入 Rust CI/local wrapper。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（6 passed）、`cargo test -p cbm-core --locked`（98 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm build/c/test-runner && build/c/test-runner mcp`（123 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（123 passed）、`make -f Makefile.cbm CBM_USE_RUST_MCP_CODEC=1 build/c/test-runner && build/c/test-runner mcp`（123 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- 下一批候選：Store 下一步仍應避開 open/WAL runtime，優先補更多 `store_compat` side-effect golden 後再評估 open/readonly/WAL helper；Pipeline explorer 建議以獨立 `CBM_USE_RUST_PIPELINE_TOP_PLAN=1` 消費 FullPipeline top-level metadata，但需先處理 `try_incremental_or_delete_db()`、graph lifecycle 與既有 static guard。

## 本次工作階段（2026-07-06 Phase 3 Cypher hop shorthand parity）

- 修正 `rust/cbm-core/src/cypher/mod.rs` 的 `parse_hops()`，讓 `*2` summary 對齊 C parser 的 `min_hops=1,max_hops=2`，而不是錯誤的 `2..2`。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c`，覆蓋 `*2`、`*` 與 `*..3` 的 normalized AST summary。
- 擴充 `tests/test_cypher_contract.c`，新增 C parser AST contract，固定 `*2 -> 1..2`、`* -> 1..0`、`*..3 -> 1..3`。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（10 passed）、`cargo test -p cbm-core --locked`（91 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner cypher_contract`（21 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-06 Phase 3 Store mmap resolver opt-in）

- 新增 `rust/cbm-core/src/store/pragmas.rs`，固定 `CBM_SQLITE_MMAP_SIZE` 到 `PRAGMA mmap_size` 的 byte-level parsing contract。
- 新增 `cbm_rs_store_resolve_mmap_size_value_v1` FFI 與 `CBM_USE_RUST_STORE_MMAP_RESOLVER=1`，讓 C resolver 可窄範圍委派 Rust parser；預設 C path 保留。
- 補強 C contract：`strtoll` overflow 現在以 `errno == ERANGE` 回 64 MiB default，並測 unset、empty、garbage、partial、trailing space、overflow、zero、negative、leading whitespace 與 `+`。
- 新增 `rust-store-mmap-resolver-optin-test` 並接入 `rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core store::pragmas --locked`（1 passed）、`cargo test -p cbm-core --locked`（92 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner store_pragmas store_compat`（23 passed）與 `make -f Makefile.cbm rust-store-mmap-resolver-optin-test`（23 passed）。

## 本次工作階段（2026-07-06 Phase 3 Store immutable URI opt-in）

- 新增 `rust/cbm-core/src/store/open.rs`，固定 SQLite readonly immutable fallback URI builder 的 byte-level contract。
- 新增 `cbm_rs_store_build_immutable_uri_v1` FFI smoke，覆蓋 POSIX/relative/root/Windows/UNC path、安全字元、space、`%`、`?`、`#`、`&`、`=`、UTF-8 raw bytes、NULL、length-only 與短 buffer 截斷。
- 新增 `CBM_USE_RUST_STORE_IMMUTABLE_URI=1`，讓 C `build_immutable_uri()` 只把 URI 字串建構委派給 Rust；SQLite `open_v2`、readonly/no-create probe、immutable fallback timing、WAL/pragma dispatch、UDF/FTS/search/CRUD 與 Store lifetime 仍留 C。
- 新增 `rust-store-immutable-uri-optin-test` 並接入 `rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7。
- 驗證通過：`cargo test -p cbm-core store::open --locked`（3 passed）、`make -f Makefile.cbm rust-ffi-test` 與 `make -f Makefile.cbm rust-store-immutable-uri-optin-test`（`store_compat mcp` 131 passed）。

## 本次工作階段（2026-07-06 Phase 3 Store search pattern opt-in）

- 新增 `rust/cbm-core/src/store/search_pattern.rs`，固定 `cbm_glob_to_like`、`cbm_extract_like_hints`、`cbm_ensure_case_insensitive` 與 `cbm_strip_case_flag` 的 byte-level contract。
- 新增 `cbm_rs_store_glob_to_like_v1`、case flag helpers、LIKE hint count/value FFI，沿用 caller-provided buffer、完整長度回傳、短 buffer NUL 截斷、`SIZE_MAX` error 與 C-owned allocation contract。
- 新增 `CBM_USE_RUST_STORE_SEARCH_PATTERN=1`，讓 C wrapper 只委派 search pattern 純字串 helpers；SQL builder、LIKE bind lifetime、REGEXP UDF、FTS/BM25、SQLite statement lifecycle 與 Store runtime 仍留 C。
- 補強 Rust unit、direct FFI smoke 與 `tests/test_store_search.c`，覆蓋 escaped pipe bail、escaped meta、max_out、NULL、短 buffer、invalid index 與 255-byte literal truncation。
- 新增 `rust-store-search-pattern-optin-test` 並接入 `rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh`、`scripts/test.sh` Step 7 與 reusable Rust workflow 註解。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core store::search_pattern --locked`（4 passed）、`cargo test -p cbm-core --locked`（102 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner store_search mcp`（191 passed）、`make -f Makefile.cbm rust-store-search-pattern-optin-test`（191 passed）與 `make -f Makefile.cbm lint-format`。

## 本次工作階段（2026-07-06 Phase 3 Store architecture helper opt-in）

- 新增 `rust/cbm-core/src/store/arch_helpers.rs`，固定 `qn_to_package`、`qn_to_top_package`、`is_test_file_path`、`hop_to_risk`、`risk_label` 的 byte-level contract 與 4 個 helper 的 test-only FFI smoke。
- 新增 `CBM_USE_RUST_STORE_ARCH_HELPERS=1`，讓 `src/store/store.c` 的 architecture helper（QN 解析、測試路徑判斷、風險判斷）在純 helper 層面委派到 Rust；SQLite open/pragma/runtime、BFS/search/CRUD 及 store ownership 仍保留 C。
- 擴充 `tests/test_rust_ffi.c` 與 `tests/test_store_arch.c`，補齊 `length-only`、`buf == NULL`、`bufsize == 0`、short buffer 與 255-byte 邊界測試。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core store::arch_helpers --locked`（4 passed）、`cargo test -p cbm-core --locked`（106 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm build/c/test-runner && build/c/test-runner store_arch store_search mcp`（245 passed）、`make -f Makefile.cbm rust-store-arch-helper-optin-test`（245 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-06 Phase 4 FullPipeline top-level metadata FFI）

- 擴充 `rust/cbm-core/src/pipeline/plan.rs`，新增 FullPipeline top-level metadata helper，固定 macro extraction、user config、discover、incremental-or-delete-db、structure、extraction dispatch、tests、githistory、predump、dump、persist hashes 與 artifact export 外層順序。
- 新增 `CbmRsPipelineTopStepV1` 與 `cbm_rs_pipeline_full_plan_step_count_v1` / `cbm_rs_pipeline_full_plan_steps_v1`，以 C static assert 鎖定 ABI layout、full/fast step count、gate/effect flags、`requires_mask`、nested plan kind 與 null/short/negative cap contract。
- 保留 `cbm_rs_pipeline_plan_steps_v2(kind=FullPipeline)` unsupported，並在 `tests/test_graph_buffer.c` 加入 static guard，確認 `pipeline.c` 與 `pipeline_incremental.c` 不消費新 FullPipeline metadata API。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::plan --locked`（12 passed）、`cargo test -p cbm-core --locked`（93 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test` 與 `build/c/test-runner graph_buffer pipeline`（269 passed）。

## 本次工作階段（2026-07-06 Foundation profile env opt-in）

- 新增 `CBM_USE_RUST_PROFILE_ENV=1`，讓 `src/foundation/profile.c` 的 `cbm_profile_init()` 在 opt-in 時委派 Rust `cbm_rs_profile_env_enabled()` 判斷 `CBM_PROFILE` 是否啟用。
- 預設 C path 保持不變；`cbm_profile_active`、`cbm_profile_enable()`、clock、elapsed logging 與 log sink/formatter 仍由 C 負責。
- 將新 flag 納入 Rust staticlib link trigger、`rust-foundation-optin-test` 單一 opt-in 與全量 foundation opt-in matrix。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core foundation::profile --locked`（2 passed）、`cargo test -p cbm-core --locked`（93 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm test-foundation`（322 passed）、`make -f Makefile.cbm CBM_USE_RUST_PROFILE_ENV=1 test-foundation`（322 passed）、`make -f Makefile.cbm rust-foundation-optin-test`（每輪 322 passed，含 profile env 單一與全量 opt-in）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

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

目前產品預設核心仍位於 `src/` 的 C 程式碼，包含 `foundation/`、`store/`、`cypher/`、`mcp/`、`pipeline/`、`discover/`、`watcher/`、`cli/` 與 `ui/`。Rust workspace 已存在於 `rust/`，但只在明確 opt-in 下支援部分 foundation parity、Store 純 helper、MCP request parse、pipeline registry/plan 決策與 test-only graph-buffer command contract。tree-sitter 擷取、語言規格與 Hybrid LSP 邏輯位於 `internal/cbm/`。測試集中在 `tests/test_*.c` 與 `tests/repro/`。

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
- YAML 進入 Rust test-only/opt-in 前，必須持續讓 `test-foundation` 與 foundation opt-in matrix 跑既有 YAML C contract。

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
  1. `yaml`：既有 C contract suite 已納入 foundation matrix；test-only Rust parity 與 FFI smoke 已完成，production opt-in 需另行評估。
  2. `log` / `profile`：deterministic helper parity 已起步（env parsing、format helper、profile elapsed/rate）；stderr/sink lifecycle、clock、global state、rotation、timestamp 與 production opt-in 仍需後續 contract 與 wrapper parity。
  3. `compat_*`：deterministic helper parity 已起步，固定 regex flags/cap/status、thread default stack 與 aligned alloc precheck；依平台的 fs/process/thread runtime wrapper、Windows shim 與 production opt-in 仍需後續 fixture。
  4. `hash_table`：test-only `cbm_rs_ht_*` parity fixture 起步**已完成**（`foundation::hash_table` 純安全 module + FFI + `tests/test_rust_ffi.c` parity）；仍不承諾 production opt-in，需在 hot-path pointer contract 可證明安全前維持 C-only。
  5. `arena` / `slab_alloc` / `mem` / `vmem`：`arena` public-struct test-only parity 已起步，仍需 allocator stress/fuzz 與 production opt-in 評估；`slab_alloc`、`mem`、`vmem` 最後處理，先只做 contract/stress/fuzz，再評估是否以 Rust ownership 包 C allocator 或整體替換。
- **主要風險**：allocator hot path 與 unsafe 邊界、借用指標生命週期、跨平台 filesystem/thread 差異、log/profile I/O 副作用、效能與 RSS 退化。
- **前置 gate**：每個模組先有預設 C contract/stress tests；allocator 類需 sanitizer/fuzz 或 stress fixture；`hash_table` 需維持 C-only hot path 直到 pointer contract 可證明安全。
- **驗證方式**：`make -f Makefile.cbm test-foundation`、單一 `CBM_USE_RUST_*` foundation opt-in、全 foundation opt-in、`scripts/rust-check.sh`、`make -f Makefile.cbm rust-ffi-test`、`scripts/test.sh`、`scripts/lint.sh --ci`。

### 2. Pipeline orchestration 全面化

- **範圍**：從目前已完成的 registry resolve 與 plan 決策層，擴大到 pass registry、full/incremental orchestration、資料流管理、錯誤傳遞與 graph-buffer mutation 的調度；實際 extraction/tree-sitter/LSP 仍在後續階段。
- **建議切片順序**：
  1. 完整 incremental post/tail adapter 已由 typed steps dispatch；incremental extract/resolve typed v2 dispatch、full/fast predump typed v2 metadata、pass trace golden 與 incremental dump failure side-effect contract 已固定。
  2. 將 pass registry metadata、gating、依賴順序與 parallel/sequential dispatch 持續移到 Rust；sequential extraction dispatch typed v2、parallel extraction 外層 typed v2 metadata/trace、incremental extract/resolve typed v2 dispatch/trace，以及 graph-buffer mutation command contract 已完成。
  3. C-side command adapter 已固定多批代表性 callpoint（structure、incremental purge/file-upsert/inbound restore、`TESTS`/`TESTS_FILE`、usage/configlink、similarity/semantic/decorator、infra/githistory、k8s/route/definitions/calls/semantic 與 pass_parallel）；完整資料流仍未完成，不得一次搬移所有 pass 或 graph-buffer storage。
  4. 擴充 language graph parity，覆蓋更多語言、incremental、fast-mode、semantic/LSP edges。
- **主要風險**：pass order 漂移、parallel race、incremental 與 full 結果不一致、graph-buffer ownership、效能退化。
- **前置 gate**：現有 registry/plan parity 維持全綠；每新增 dispatch 切片前，先固定 default C graph output 與 pass trace golden。
- **驗證方式**：`make -f Makefile.cbm rust-pipeline-registry-optin-test`、`make -f Makefile.cbm rust-pipeline-plan-optin-test`、`make -f Makefile.cbm rust-language-graph-parity`、`build/c/test-runner pipeline`、`scripts/test.sh`、`scripts/smoke-invariants.sh`。

### 3. Store：SQLite 圖儲存

- **範圍**：`src/store/` 的 schema、pragma、authorizer、UDF、FTS、CRUD、search、BFS、ADR、architecture、vector search、artifact import/export 與 readonly/bulk/checkpoint 行為。
- **建議切片順序**：
  1. 補完整 `sqlite_schema`、`table_xinfo`、index layout、FTS shadow object、checkpoint/bulk side effect、artifact import/export fixtures。
  2. Rust schema/index manifest helper 已完成 static/test-only V1/V2 FFI，固定核心表、`nodes_fts`、`edges.url_path_gen`、9 個 user index、table column/index layout、FTS DDL 與 FTS shadow metadata；connection/pragma manifest helper 也已固定 read-only no-create、immutable fallback、WAL、mmap env 與 bulk pragma contract；FTS camelCase tokenization helper 已固定 `cbm_camel_split` 的 byte-level split/fallback contract，並以 `CBM_USE_RUST_STORE_FTS_TOKENIZER=1` 提供只委派字串計算的窄範圍 opt-in；mmap resolver 已以 `CBM_USE_RUST_STORE_MMAP_RESOLVER=1` 委派 env 字串解析；immutable URI builder 已以 `CBM_USE_RUST_STORE_IMMUTABLE_URI=1` 委派 readonly fallback URI 字串建構；search pattern helper 已以 `CBM_USE_RUST_STORE_SEARCH_PATTERN=1` 委派 glob→LIKE、LIKE hint 與 case flag 純字串 contract。SQLite open/probe/pragma/FTS/CRUD/search runtime 仍留 C。
  3. Rust open/readonly/WAL/pragma helper，以 `CBM_USE_RUST_STORE_OPEN=1` 類旗標單獨 opt-in；實作前需沿用上述 static manifest 與 `store_compat` live SQLite contract。
  4. CRUD/search/FTS/BM25 分開切；vector tables、graph_buffer direct writer 與 sqlite_writer 最後。
  5. artifact import/export 與 migration path 完成後，才評估 store 預設切換。
- **主要風險**：SQLite pragma/WAL 副作用、readonly 不得建立 DB、FTS/BM25 rank 漂移、UDF/authorizer 行為差異、vector schema 與既有資料庫相容性。
- **前置 gate**：`tests/test_store_compat.c` 擴充到完整 schema/side-effect golden；預設 C path 必須先通過。
- **驗證方式**：`build/c/test-runner store_compat`、`tests/test_store_*.c`、artifact bootstrap/import fixture、`scripts/test.sh`、`scripts/smoke-test.sh build/c/codebase-memory-mcp`、`make -f Makefile.cbm security`。

### 4. Cypher：查詢引擎

- **範圍**：lexer/parser、AST、query executor、aggregation、OPTIONAL MATCH、UNION、computed properties、error reporting，以及 MCP `query_graph` 依賴的結果格式。
- **建議切片順序**：
  1. 補 CALL、EXISTS pattern、malformed query、aggregation edge cases 與 exact error golden。
  2. Rust lexer/parser 或 normalized AST helper，先以 test-only FFI 比對 C AST shape；lexer/token summary 與 normalized AST summary parity 已先完成，完整 parser/AST normalizer 與 production opt-in 仍待切片。
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
  2. `CBM_USE_RUST_MCP_CODEC=1` 已先接入 request envelope parse；下一步再分段處理 response serialization 與 Content-Length framing。
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
- [x] `scripts/build.sh` 與 `scripts/build.sh --with-ui` 皆通過，artifact 命名、簽章與安裝位置相容。
- [ ] install / update / uninstall / config / package wrappers（npm、PyPI、Homebrew、installer scripts）皆通過。
- [ ] `graph-ui` 內嵌 variant 與 non-UI variant 皆通過 smoke 與 release packaging。
- [x] `scripts/test.sh`、`scripts/lint.sh --ci`、`scripts/smoke-test.sh`、`scripts/smoke-invariants.sh`、`make -f Makefile.cbm security` 全部通過。
- [ ] macOS arm64/x86_64、Linux x86_64/arm64、Windows x86_64 皆通過核心測試與安裝測試。
- [ ] 效能基準符合索引/查詢/RSS 退化 ≤10%，binary size 退化 ≤20%；例外需明確記錄與核准。
- [ ] SQLite 既有 DB、artifact import/export、FTS/WAL/readonly、MCP 14 tools、CLI 子命令、language graph parity 全部相容。
- [x] Rust unsafe/process/network/filesystem allowlist、dependency audit、vendored integrity 與 security suite 均無未處理 finding。
- [ ] 文件、release notes、CONTRIBUTING、README 與 migration notes 均已更新，且不再宣稱預設為純 C。
