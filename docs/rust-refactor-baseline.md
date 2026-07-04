# Rust 重構 Phase 0 基準

本文件固定第一批 Rust 重構前後都要比對的最小基準。它不取代既有 C 測試，而是讓 Rust 遷移每一步都能證明公開行為沒有漂移。

## 必跑命令

第一批 gate 以現有流程為準：

- `scripts/build.sh`
- `scripts/test.sh`
- `scripts/lint.sh --ci`
- `make -f Makefile.cbm security`
- `scripts/smoke-test.sh build/c/codebase-memory-mcp`
- `scripts/smoke-invariants.sh build/c/codebase-memory-mcp`
- `make -f Makefile.cbm rust-baseline-fixtures`

Rust workspace 存在時，另跑：

- `scripts/rust-check.sh`
- `make -f Makefile.cbm rust-ci`
- `make -f Makefile.cbm rust-ffi-test`
- `make -f Makefile.cbm test-foundation`
- `make -f Makefile.cbm CBM_USE_RUST_DUMP_VERIFY=1 test-foundation`
- `make -f Makefile.cbm CBM_USE_RUST_STR_INTERN=1 test-foundation`
- `make -f Makefile.cbm CBM_USE_RUST_STR_UTIL=1 test-foundation`
- `make -f Makefile.cbm CBM_USE_RUST_PLATFORM_PATH=1 test-foundation`
- `make -f Makefile.cbm CBM_USE_RUST_PLATFORM_CGROUP=1 test-foundation`
- `make -f Makefile.cbm CBM_USE_RUST_PLATFORM_ENV_DIRS=1 test-foundation`
- `make -f Makefile.cbm CBM_USE_RUST_DUMP_VERIFY=1 CBM_USE_RUST_STR_INTERN=1 CBM_USE_RUST_STR_UTIL=1 CBM_USE_RUST_PLATFORM_PATH=1 CBM_USE_RUST_PLATFORM_CGROUP=1 CBM_USE_RUST_PLATFORM_ENV_DIRS=1 test-foundation`
- `make -f Makefile.cbm rust-foundation-optin-test`
- `make -f Makefile.cbm rust-pipeline-registry-optin-test`
- `make -f Makefile.cbm rust-pipeline-plan-optin-test`
- `make -f Makefile.cbm rust-language-graph-parity`
- `make -f Makefile.cbm rust-dangerous-api`
- `make -f Makefile.cbm rust-security`

`rust-ffi-test` 可用 `RUST_TARGET=<triple>`、`CARGO_TARGET_DIR=<dir>` 與 `RUST_STATICLIB_NAME=<archive>` 覆寫 Cargo 目標路徑，供 macOS cross-arch 與 Windows GNU/MSVC 差異驗證。

## 行為基準

需要鎖定的公開介面：

- 頂層命令：`--version`、`--help`、預設 MCP stdio、`--ui=true/false`、`--port=N`。
- 子命令：`install`、`uninstall`、`update`、`config list/get/set/reset`。
- CLI mode：`cli <tool> '{...}'`、`cli --json`、`cli --args-file`、stdin JSON、flag form、per-tool `--help`。
- MCP transport：newline JSON-RPC 與 `Content-Length` framing。
- MCP tools：`index_repository`、`search_graph`、`query_graph`、`trace_path`、`get_code_snippet`、`get_graph_schema`、`get_architecture`、`search_code`、`list_projects`、`delete_project`、`index_status`、`detect_changes`、`manage_adr`、`ingest_traces`。
- MCP tool alias：`trace_call_path` 由 handler 接受，但不在 `tools/list` advertised tools 中。

## Golden Fixtures 基準

第一批 golden fixtures 採小而穩定的資料集：

- `tests/golden/rust-refactor/cli-golden.json`：小型 repo 的 CLI index/status/list 與 invocation forms 摘要，涵蓋 raw JSON args、stdin JSON、`--args-file`、flag form、`--json` envelope、非 `--json` 成功與錯誤輸出。
- `tests/golden/rust-refactor/mcp-golden.json`：MCP initialize、`notifications/initialized` no-response、tools/list 兩頁 pagination、原始 tool 順序、response ids、tool key set、tool schema transcript、project argument aliases、`trace_call_path` tool alias、indexed repo `query_graph` tool call、unknown method、parse error、unknown tool 與 Content-Length framed ping。
- `tests/golden/rust-refactor/graph-golden.json`：小型 repo 的 node label 與 edge type histogram。
- `tests/golden/rust-refactor/artifact-golden.json`：小型 repo `persistence=true` artifact export、metadata 穩定摘要、`.gitattributes` 規則、artifact project graph histogram、artifact bootstrap import、schema mismatch skip 與匯入後 graph 等價。
- `tests/golden/rust-refactor/performance-baseline.json`：平台專屬 binary size、index/query wall time 與 child peak RSS smoke baseline。
- `tests/golden/rust-refactor/self-index-baseline.json`：手動 opt-in repository self-index baseline，記錄 schema 摘要、node/edge histogram、索引 guard、核心查詢與搜尋 latency smoke。
- `tests/golden/rust-refactor/large-performance-baseline.json`：手動 opt-in 大型 synthetic repository baseline，記錄 binary size、生成語料摘要、index nodes/edges、node/edge histogram、核心 query/search latency guard 與 child peak RSS guard。
- `tests/golden/rust-refactor/language-graph-parity.json`：Phase 4 代表性語言 graph parity，固定 Python、TypeScript、Go 與 YAML fixture 的 definitions、calls、imports、routes、semantic/LSP edges，並比對 default C path 與 Rust registry/plan opt-in path。
- `scripts/smoke-test.sh` 內建小型多語言 fixture，覆蓋 Python、Go、JavaScript、YAML 與大型 C++ header。

`make -f Makefile.cbm rust-baseline-fixtures` 會用 hermetic 暫存 repo 重跑第一批 golden，並檢查輸出不得含本機或暫存路徑。`make -f Makefile.cbm rust-self-index-baseline` 會手動重跑目前 repository 的 full self-index baseline；需要刻意接受 baseline 變更時使用 `make -f Makefile.cbm rust-self-index-baseline-update`。`make -f Makefile.cbm rust-large-performance-baseline` 會手動重跑 deterministic generated repository 的大型效能 baseline；需要刻意接受 baseline 變更時使用 `make -f Makefile.cbm rust-large-performance-baseline-update`。這兩個大型 gate 不屬於預設 `scripts/test.sh`、`scripts/rust-check.sh` 或 `rust-ci`；目前大型效能 gate 的 RSS 取樣支援 macOS/Linux。現有 fixture 多半內嵌於 `tests/test_pipeline.c`、`tests/grammar_cases.h` 與 `tests/repro/`。若新增獨立 golden 目錄，應避免複製大型第三方程式碼。

待補的完整基準：

- 更完整 MCP transcript，補齊 string id、更多 tool-call responses 與 stdout/stderr 分離案例。

## Foundation 與 FFI Gate

Rust 遷移初期的快速回歸 gate：

- `make -f Makefile.cbm test-foundation`：只編譯並執行 foundation suites，避免 full runner 依賴所有 test objects。
- `make -f Makefile.cbm rust-ffi-test`：編譯 Rust `staticlib`，由 C smoke test 呼叫目前的 Rust ABI export；目前覆蓋 `dump_verify`、`str_intern`、`str_util`、`platform`、diagnostics formatter、pipeline pass plan、typed sequential/incremental/predump/parallel extraction v2 metadata、typed incremental post steps、graph-buffer mutation command metadata/validation、registry one-shot resolver 與 registry handle 的 null、embedded NUL、指標穩定性、oversized length、validation、JSON escape、原地路徑正規化、env/dirs buffer 合約、cgroup parser 防護、diagnostics exact JSON/NDJSON/path bytes、pipeline sequential/predump/incremental/parallel plan order、sequential/incremental/predump/parallel v2 kind/phase/policy/gate/requires/effect 欄位、graph-buffer command kind/field/effect/null/empty/reserved-bit contract、incremental tail kind/policy、parallel threshold、handle lifecycle、output truncation，以及 import map、qualified suffix、candidate cap 與 import reachability penalty。
- `make -f Makefile.cbm rust-foundation-optin-test`：重建並執行 foundation 預設 C path、各單一 Rust opt-in，以及全部 opt-in 的組合。
- `make -f Makefile.cbm rust-pipeline-registry-optin-test`：以 `CBM_USE_RUST_PIPELINE_REGISTRY=1` 重建 full C test runner，執行 registry suite，驗證產品 `cbm_registry_resolve` opt-in 仍保留 C return pointer/cache contract。
- `make -f Makefile.cbm rust-pipeline-plan-optin-test`：以 `CBM_USE_RUST_PIPELINE_PLAN=1` 重建 full C test runner，執行 pipeline suite，驗證 Rust plan 選擇 full/incremental extraction 的 parallel/sequential 路徑、sequential extraction typed v2 dispatch/trace、incremental extract/resolve typed v2 dispatch/trace、parallel extraction typed v2 metadata/trace、full pre-dump typed v2 metadata dispatch/trace，以及 typed incremental post/tail dispatch；C 端仍固定驗證 sequential/incremental/predump/parallel metadata、tail 順序並呼叫既有 C side effects。
- `make -f Makefile.cbm rust-language-graph-parity`：重建 default C binary 與 `CBM_USE_RUST_PIPELINE_REGISTRY=1 CBM_USE_RUST_PIPELINE_PLAN=1` binary，索引 hermetic 代表性語言 fixture，並比對 `language-graph-parity.json`。需要刻意接受變更時使用 `make -f Makefile.cbm rust-language-graph-parity-update`。

目前 C ABI export：

- `cbm_rs_dump_verify_is_degraded`：對應 `src/foundation/dump_verify.c` 的 pure predicate。env parsing 仍保留在 C，避免第一個 ABI gate 牽涉字串 ownership。
- `cbm_rs_intern_*`：對應 `src/foundation/str_intern.c` 的 pool-owned interner。Rust 端以原始 bytes 去重，value 保留 trailing NUL，支援 embedded NUL，回傳指標有效期到 pool free。
- `cbm_rs_str_*`、`cbm_rs_path_*`、`cbm_rs_validate_*`、`cbm_rs_json_escape`：對應 `src/foundation/str_util.c` 中的 byte-oriented string/path helpers。arena-backed public C functions 仍由 C wrapper 配置 `CBMArena` output，Rust 只回傳 borrowed offset、required length 或寫入 caller-provided buffer。
- `cbm_rs_normalize_path_sep`、`cbm_rs_detect_cgroup_cpus`、`cbm_rs_detect_cgroup_mem`：對應 `platform` 路徑正規化與 Linux cgroup quota helper。路徑正規化會原地修改呼叫端 buffer；cgroup helper 僅回傳 scalar。
- `cbm_rs_safe_getenv`、`cbm_rs_get_home_dir`、`cbm_rs_app_config_dir`、`cbm_rs_app_local_dir`、`cbm_rs_resolve_cache_dir`：對應 `src/foundation/platform.c` 的 env/dirs helpers。Rust 寫入呼叫端提供的 buffer，回傳完整輸出長度，缺值以 `SIZE_MAX` 表示；C wrapper 保留呼叫端與靜態 buffer 所有權。
- `cbm_rs_diag_*`：對應 `src/foundation/diagnostics.c` 已抽出的 deterministic helper。Rust parity 只覆蓋 env 值判斷、query scalar snapshot、path formatter、JSON formatter 與 NDJSON formatter；writer thread、系統量測、檔案 write/rename、rotation 與 stop lifecycle 保留 C。
- `cbm_rs_pipeline_plan_describe` / `cbm_rs_pipeline_plan_steps_v2`：對應 `src/pipeline/pipeline.c` / `pipeline_incremental.c` 的 pass plan 資料切片。Rust parity 固定 full pipeline order、sequential dispatch 與 infra tail、parallel extraction order、predump fast-mode gating、typed sequential/incremental/predump/parallel extraction v2 metadata、parallel threshold `file_count > 50`、incremental extract/resolve dispatch plan，以及 incremental post/tail typed order；不觸碰 tree-sitter shim、`CBMFileResult`、LSP 或 graph-buffer storage/ownership。
- `cbm_rs_gbuf_mutation_command_count_v1` / `cbm_rs_gbuf_mutation_commands_v1` / `cbm_rs_gbuf_mutation_validate_v1`：test-only graph-buffer mutation command boundary ABI。Rust 只描述並驗證 `UpsertNode`、`InsertEdge` 與 `DeleteByFile` 的 required/optional fields、result kind、effect flags、reserved bits、empty string 與 null contract；不接產品 opt-in，不持有 `cbm_gbuf_t`，不執行 graph-buffer mutation。C-side `graph_buffer_mutation` adapter 與 FFI smoke 共用 `graph_buffer.h` 的 command struct/constants，讓既有 C 呼叫正規化成 command apply 邊界，但仍同步 dispatch 到 C `cbm_gbuf_*` API。數值 endpoint 欄位是固定 ABI scalar slot，Rust shape validation 不做 endpoint lookup；edge dedup 可能回傳既有 edge id。
- `cbm_rs_registry_resolve_once`：對應 `src/pipeline/registry.c` 的 registry name lookup / candidate selection 純決策切片。此 API 僅作 parity fixture，Rust 不保存 C 傳入的 entry/import 指標，輸出寫入 caller-provided buffers。
- `cbm_rs_registry_create/free/add/resolve`：產品 registry opt-in 使用的 opaque handle ABI。Rust handle 擁有自己的 registry state，`add` 複製 QN bytes；`resolve` 將 QN 與 strategy 寫入 caller-provided buffers，C wrapper 再映射回既有 C registry-owned key 與 static strategy string。

目前 opt-in C wrapper：

- `CBM_USE_RUST_DUMP_VERIFY=1`：讓 `cbm_dump_verify_is_degraded` 轉呼叫 Rust ABI。
- `CBM_USE_RUST_STR_INTERN=1`：讓 `cbm_intern_*` 轉呼叫 Rust ABI，公開 header 與 C 呼叫端不變。
- `CBM_USE_RUST_STR_UTIL=1`：讓 `cbm_path_join`、`cbm_path_join_n`、`cbm_path_ext`、`cbm_path_base`、`cbm_path_dir`、`cbm_str_starts_with`、`cbm_str_ends_with`、`cbm_str_contains`、`cbm_str_tolower`、`cbm_str_replace_char`、`cbm_str_strip_ext`、`cbm_str_split`、`cbm_validate_shell_arg`、`cbm_validate_project_name` 與 `cbm_json_escape` 轉呼叫 Rust ABI；需要配置的回傳值仍由 C arena 擁有。
- `CBM_USE_RUST_PLATFORM_PATH=1`：讓 `cbm_normalize_path_sep` 轉呼叫 Rust ABI，仍原地修改 caller-owned buffer。
- `CBM_USE_RUST_PLATFORM_CGROUP=1`：在 Linux 下讓 `cbm_detect_cgroup_cpus` 與 `cbm_detect_cgroup_mem` 轉呼叫 Rust ABI；其他平台只作為可連結的 opt-in gate。
- `CBM_USE_RUST_PLATFORM_ENV_DIRS=1`：讓 `cbm_safe_getenv`、`cbm_get_home_dir`、`cbm_app_config_dir`、`cbm_app_local_dir` 與 `cbm_resolve_cache_dir` 轉呼叫 Rust ABI；缺值仍回傳 `NULL`，輸出 buffer 所有權不變。
- `CBM_USE_RUST_PIPELINE_REGISTRY=1`：讓 `cbm_registry_add` 同步寫入 Rust opaque registry handle，並讓 `cbm_registry_resolve` 在既有 C resolve cache miss 後先嘗試 Rust resolver。`cbm_registry_label_of`、`cbm_registry_find_by_name`、`cbm_registry_fuzzy_resolve` 與預設 C path 仍維持 C implementation。
- `CBM_USE_RUST_PIPELINE_PLAN=1`：讓 full 與 incremental extraction phase 透過 Rust `pipeline::plan` 回答 parallel/sequential 選擇，讓 sequential extraction dispatch 依 Rust typed v2 metadata 執行 `definitions`、`k8s`、`lsp_cross`、`calls`、`usages` 與 `semantic`，讓 incremental extract/resolve 依 Rust typed v2 metadata 驗證後 dispatch sequential `definitions`、`calls`、`usages`、`semantic` 或 parallel `incr_extract`、`incr_registry`、`incr_resolve`，讓 parallel extraction 依 Rust typed v2 metadata 驗證並記錄 `parallel_extract`、`registry_build`、`lsp_cross_prepare`、`parallel_resolve`、`infra_routes`、`infra_bindings` 與 `k8s` 外層順序，讓 full pre-dump 階段依 Rust typed v2 metadata dispatch `decorator_tags`、`configlink`、`route_match`、`similarity`、`semantic_edges` 與 `complexity`，並透過 typed incremental post steps dispatch `k8s`、`incr_tests`、`incr_decorator_tags`、`incr_configlink`、`incr_similarity`、`incr_semantic_edges`、`edge_relink`、`incremental_dump`、`persist_hashes` 與 `artifact_export`；C 端會逐欄驗證 sequential/incremental/predump/parallel extraction `kind/phase/policy/gate_flags/requires_mask/effect_flags`，實際 C pass function pointers、parallel cache/cross-LSP cleanup、dump、hash persist、FTS rebuild、artifact export、tree-sitter/extraction/LSP、infra processing 與 graph-buffer 寫入不遷移。Incremental dump failure 由 C tail handler 傳回實際 rc，並跳過後續 persist/artifact tail，避免以 full fallback 隱藏錯誤。
- 預設未設定時仍使用原 C implementation；production `cbm` 與 `cbm-with-ui` 只有在任一 Rust opt-in 啟用時才連 Rust staticlib。

Pipeline plan strategy：

- `pipeline::plan` 目前固定 C orchestration 的 pass order 與 gating。產品 opt-in 讓 C 端只讀 Rust plan 來選 full/incremental extraction path、sequential extraction typed v2 metadata、incremental extract/resolve typed v2 dispatch、parallel extraction typed v2 metadata、full pre-dump typed v2 metadata 與 typed incremental post/tail order，仍呼叫既有 C pass function pointers 與 dump/persist/artifact handlers；sequential/incremental/predump/parallel extraction `source=typed_v2` trace 與 incremental dump failure tests 固定 C adapter 的觀測行為與錯誤傳遞。不得在同一切片遷移 extraction、tree-sitter shim、LSP cross-file prebuild、dump/persist/artifact 實作或 graph-buffer storage/ownership；graph-buffer mutation command boundary 目前只可作 metadata/validation，C-side adapter 只可作同步 normalization/dispatch，目前代表性呼叫點包含 structure、incremental purge/file-upsert/inbound restore、`TESTS`/`TESTS_FILE`、usage 與 configlink edges。
- tree-sitter 相關驗證由 language graph parity gate 起步，需保留 default C path、`CBM_USE_RUST_PIPELINE_REGISTRY=1 CBM_USE_RUST_PIPELINE_PLAN=1` opt-in path，以及後續新增的 `CBM_DISABLE_LSP_CROSS=1`/預設 cross-LSP 路徑，確認 plan 切片沒有改變 definitions、calls、imports、routes、semantic/LSP edges。

下一批 foundation strategy：

- `diagnostics` 先維持 test-only Rust parity fixture。若未來需要產品 opt-in，範圍只允許 formatter/value-only helper，例如 `CBM_USE_RUST_DIAGNOSTICS_FORMAT=1`；thread、system metrics、atomic global state、file lifecycle 與 rotation 仍留 C。
- `hash_table` 目前維持 C-only。合約包含 borrowed key pointer、`cbm_ht_get_key` 回傳 stored key 指標、NULL value 仍視為存在、foreach exactly-once 但無順序保證；因它是 hot path 且 pointer/callback contract 複雜，不直接導入 Rust production opt-in。未來若 prototype，先做 test-only `cbm_rs_ht_*` parity fixture，再以 ASan/UBSan、壓力測試與效能 baseline 評估。

## 效能與安全指標

效能先記錄再比較。完整基準的目標門檻沿用 `Rust-Refactor.md`：效能退化不超過 10%，binary size 退化不超過 20%；目前第一批 `performance-baseline.json` 是小型 smoke，使用較寬鬆的 wall-time guard 與 20% binary size guard，只用來偵測明顯退化。`large-performance-baseline.json` 以大型 deterministic corpus 補足 Phase 0 的 index/query/RSS/binary size guard；提交的 golden 不保存本機路徑、暫存路徑或時間戳。

建議記錄欄位：

- binary size、index wall time、file count、LOC、nodes、edges。
- node/edge histogram、`search_graph` latency、BM25 latency。
- peak RSS、RSS ratio、FD drift、max query latency。

安全基準沿用 `make -f Makefile.cbm security`；此目標會執行既有 C/UI/install/network/fuzz/vendored 檢查，並串接預設 `rust-security`。另保留 `scripts/audit-grammar-security.sh`、`scripts/check-no-test-skips.sh`、`scripts/check-nolint-whitelist.sh` 作為 Rust 遷移後的輔助檢查。
Rust source security 另以 `make -f Makefile.cbm rust-dangerous-api` 掃描 `unsafe`、process、network 與 filesystem API，並由 `scripts/rust-security-allowlist.txt` 明確放行目前的 FFI boundary 與 cgroup 唯讀檔案存取；此 source gate 已接入 `_security.yml` 的 `security-static` job。`make -f Makefile.cbm rust-security` 會串接此 gate 與 `rust-audit`；本機缺 `cargo-audit` 時預設仍會跳過，避免破壞開發流程，但 `_security.yml` 會安裝 `cargo-audit` 並以 `RUST_AUDIT_REQUIRED=1` 執行，缺工具時必須 fail-closed。2026-07-02 已用暫存 `cargo-audit v0.22.2` 執行 `make -f Makefile.cbm RUST_AUDIT_REQUIRED=1 rust-audit`，掃描 `Cargo.lock` 的 1 個 crate dependency 並通過。
