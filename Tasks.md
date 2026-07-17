# Rust 重構任務清單

## 第 28 項：Pipeline split-command true-source（2026-07-17）

- [x] 新 true-source slice 已完成，總計由 **22 / 22** 推進至 **23 / 23**。
- [x] C fallback 已拆至 src/pipeline/split_command.c/.h；pass_compile_commands.c 保留
  compile_commands orchestration 與既有 resolve_path wrapper，resolve_path 未遷移。
- [x] ABI 維持 int cbm_split_command(const char *cmd, char **out, int max_out)。NULL cmd、
  NULL out 或非正 max_out 回 0 且不寫入；輸出只保證 [0, count)，不提供 out[count] sentinel。
  token 是 caller free 的 C malloc 配置。
- [x] 固定 raw-byte first-NUL、4095-byte payload、quote 外 ASCII 空白或 Tab 分隔、移除單雙
  引號、empty quote 不輸出、未閉合 quote 仍輸出及反斜線無跳脫的契約。
- [x] wrapper 使用 CBM_USE_RUST_PIPELINE_SPLIT_COMMAND 與既有 v1；direct 使用 Cargo
  pipeline-split-command-only、CBM_USE_RUST_PIPELINE_SPLIT_COMMAND_ONLY 與空白
  PIPELINE_SPLIT_COMMAND_SRCS selector。
- [x] 正常資源路徑 parity 已驗證。OOM 不作 ABI parity 承諾：既有 C strdup failure 會保留 NULL
  slot 並遞增 count，現有 downstream caller 有 NULL 解參考風險；Rust Vec 與 C malloc 的配置
  順序及 OOM 效果不同。
- [x] 已通過 cargo fmt、pipeline_compile_commands Rust unit（10 passed）、clippy；兩個 Make
  gate 的 default/wrapper/direct FFI、pipeline 各 231 passed、production --version、direct
  空 selector/source-negative，以及 production 仍含 pass_compile_commands.c 的正向檢查。
- [ ] 不得重新推進 #28。complexity JSON 延後，先另行 hardening C strtol locale whitespace、
  overflow 與 NULL key 差異；下一步只能先做唯讀 inventory，不得先宣稱新候選。

## 第 27 項：Pipeline LSP classifiers true-source（2026-07-17）

- [x] 新 true-source slice 已完成，總計由 **21 / 21** 推進至 **22 / 22**，不是既有 ABI hardening。
- [x] C fallback 已拆至 `src/pipeline/lsp_cross_classifiers.c/.h`；`pass_lsp_cross.c` 保留
  orchestration、arena、registry、resolver dispatch 與結果合併。
- [x] 固定既有同名 ABI：`cbm_pxc_ts_modes(CBMLanguage, const char *, bool *, bool *, bool *)`、
  `cbm_pxc_has_cross_lsp(CBMLanguage)`、`cbm_pxc_map_label(const char *)`，並以
  `sizeof(CBMLanguage) == sizeof(int)` guard 固定 Rust `c_int` 邊界。
- [x] TS bits 為 `1=js`、`2=jsx`、`4=dts`；NULL path 保留 language defaults，三個 output 指標
  仍是非 NULL 前置條件。13 個 cross-LSP language 為正例。
- [x] label 僅接受九個既有 ASCII 值；NULL／拒絕回 NULL，允許時回原始 borrowed input pointer，
  並保留 first-NUL C string 語意。
- [x] C# 的 true 是 pipeline eligibility，需 prebuilt cs registry；fallback 不承諾 C# dispatch，
  不改集合或 dispatch。
- [x] wrapper 保留 v1 `int` ABI；direct 為 Cargo `pipeline-lsp-classifiers-only` 與
  `CBM_USE_RUST_PIPELINE_LSP_CLASSIFIERS_ONLY`。Make selector/FFI source 使用
  `PIPELINE_LSP_CLASSIFIERS_SRCS`，direct 排除 C fallback。
- [x] 已通過 fmt、`pipeline_lsp_cross` Rust unit（3 passed）、全 target/features clippy、兩個
  Make gate；optin default/wrapper、only default/direct 四組 FFI、pipeline 各 230 passed、
  production `--version`、direct 空 selector 與 source-negative 全數成功。
- [ ] 下一候選待下一輪唯讀 inventory 選定。

## 目前交接狀態（2026-07-17）

- [x] `pipeline-json-prop` 既有 true-source ABI/gate hardening 已完成，不是新 true-source slice；
  統計維持 **21 / 21**。
- [x] 公開 ABI 維持 `bool cbm_pipeline_extract_json_prop(const char *json, const char *key, char *buf, int bufsz)`；
  v1 為 `int`、direct 為 `bool`，Rust FFI buffer 長度使用有號 `c_int`。
- [x] 契約已凍結：NULL 或 `bufsz <= 0` 不寫 buffer；空 key 可用；空 value、60+ byte key、
  buffer 不足均回 `false` 且 buffer 不變；保留 raw scanner、first-NUL C string 與第一個 byte hit 語意。
- [x] FFI support source 已使用 `$(PIPELINE_JSON_PROP_SRCS)`，default/wrapper/direct 都經 public
  bridge，兩個 gate default leg 均含 production `--version`。
- [x] 已通過 fmt、`cargo test -p cbm-core pipeline::route --locked`（14 passed）、全 target/features
  clippy `-D warnings`、optin/only gate；三種 C pipeline runner 各 229 passed、三種 production
  `--version`、direct 空 selector 與 source-negative 均成功。
- [x] LSP cross classifiers 已完成為第 27 項 true-source slice；下一候選待唯讀 inventory 選定。

## 第 24 項：AST profile classifiers true-source（2026-07-17）

- [x] 維持原 `21/21`、第 22 項原預算外 true-source 與第 23 項既有 artifact-path parity/gate
  hardening（非新 true-source）的歷史定位。
- [x] 將 C fallback 拆至 `src/semantic/ast_profile_classifiers.c/.h`；`ast_profile.c` 僅保留
  Tree-sitter traversal 與 codec。
- [x] wrapper `CBM_USE_RUST_AST_PROFILE_CLASSIFIERS=1` 經 C bridge 呼叫既有 v1 ABI；direct
  `CBM_USE_RUST_AST_PROFILE_CLASSIFIERS_ONLY=1`／Cargo `ast-profile-classifiers-only` 排除 C
  fallback，並由 Rust 匯出同名 kind-flags、Halstead-insert、param-name public ABI。
- [x] 固定 kind flags、512 槽 Halstead insert、parameter-name 比對及防禦性 NULL／無效輸入契約。
- [x] 已通過 fmt、Rust unit（3 passed）、clippy，及 opt-in default/wrapper、only default/direct 的
  FFI、pipeline（各 228 passed）、production `--version`、empty selector 與靜默 `make -Bn`
  source-negative。
- [ ] 完整 `scripts/test.sh` 未執行。

## 第 23 項：既有 artifact-path parity／gate hardening（2026-07-17）

- [x] 保留原 `21/21` 與第 22 項原預算外 true-source 的歷史定位；本項只修正既有
  pipeline artifact-path 的 C/Rust 行為與 direct gate，不計為新的 true-source。
- [x] 公開 `src/pipeline/artifact_path.h` 的
  `bool cbm_pipeline_artifact_path(char *buf, size_t bufsz, const char *repo_path, const char *name)`
  契約：caller-owned、borrowed NUL 字串、raw-byte、無 UTF-8 假設且 output 不可與 input 重疊；
  過短且 `bufsz > 0` 時寫 prefix 加 NUL 並回 `false`，`NULL` buffer 或 zero size 不寫入。
- [x] 保留 C `snprintf` fallback；wrapper
  `CBM_USE_RUST_PIPELINE_ARTIFACT_PATH=1` 使用 v1，direct
  `CBM_USE_RUST_PIPELINE_ARTIFACT_PATH_ONLY=1`／Cargo `pipeline-artifact-path-only` 匯出同名
  public ABI。v1 成功回完整長度，過短或無效回 `SIZE_MAX`，但過短仍寫 prefix；direct short buffer
  亦寫 prefix 後回 public `false`。
- [x] 補強 FFI selector：default/wrapper 選 C bridge、direct 選 Rust symbol；artifact runner 與
  production staticlib 同步採 direct 選擇，並確認 `ARTIFACT_PATH_SRCS =` 及 `make -Bn`
  source-negative。
- [x] 已執行 fmt、Rust unit（4 passed）、clippy，及 opt-in default/wrapper、only 最終重跑
  default/direct 的 FFI、artifact runner（各 10 passed）和 production `--version`。
- [ ] 完整 `scripts/test.sh` 未執行。

## 2026-07-16 交接優先工作

完整狀態以 [docs/rust-refactor-current-handoff.md](docs/rust-refactor-current-handoff.md) 為準；
下方歷史勾選只代表當時工作階段，不能取代 current-revision gate。

- [x] 序列重跑 SvelteKit file-kind 的 wrapper 與 direct gate；在最新六個 basename 測試與
  make -Bn source-negative 檢查下，確認 FFI、pipeline（各 227 passed）、production --version 與
  direct link（selector 空、不含 sveltekit_file_kind.c）。另通過 cargo test 307、clippy、fmt。
- [x] 將 Discover trailing separator 拆出獨立 C fallback／公開 bridge，凍結 NULL 契約，新增
  source selector、實際執行的 FFI/discover（各 86 passed）/production gate 與 make -Bn
  source-negative 證據。
- [x] 2026-07-16 multi-agent：path_join（discover 87）、incremental edge（pipeline 227）、
  semantic_fp_suffix（227）、json_prop（227）、registry_label（227）、mcp edge_type（mcp 136）
  皆達 true-source bar；cargo 307；六項 -Bn source-negative（durable transcript）。
- [x] 2026-07-16 續：`cbm_is_trackable_file` true-source（`trackable_file.c`、pipeline 227、
  -Bn）；CLI archive helpers（cli 113）；K8s text helpers（pipeline 227）；simhash jaccard/hex
  （`minhash_jaccard.c`、simhash 24）；envscan classifiers（227）；configlink helpers（227）。
- [ ] 下一個優先：其它 wrapper-only pure helper 拆檔。完整 scripts/test.sh 尚未重跑；
  產品 default 仍是 C11。

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
- [x] 新增 Store connection/pragma manifest Rust test-only helper 與 FFI smoke，固定 read-only no-create、immutable fallback、WAL、mmap env、bulk begin/end pragma、public checkpoint filtered plan 與 open-mode contract；不開 SQLite、不接 production opt-in。
- [x] 新增 Store FTS camelCase tokenization Rust helper、`cbm_rs_store_camel_split_v1` FFI smoke 與 `CBM_USE_RUST_STORE_FTS_TOKENIZER=1` 窄範圍 opt-in；只委派 C SQLite UDF callback 內的 token 字串計算，SQLite runtime 仍留 C。
- [x] 新增 Store mmap resolver Rust helper、`cbm_rs_store_resolve_mmap_size_value_v1` FFI smoke 與 `CBM_USE_RUST_STORE_MMAP_RESOLVER=1` 窄範圍 opt-in；只委派 `CBM_SQLITE_MMAP_SIZE` 字串解析，SQLite open/pragma execution 仍留 C。
- [x] 新增 Store immutable URI Rust helper、`cbm_rs_store_build_immutable_uri_v1` FFI smoke 與 `CBM_USE_RUST_STORE_IMMUTABLE_URI=1` 窄範圍 opt-in；只委派 readonly immutable fallback URI 字串建構，SQLite open/probe/WAL/pragma/FTS runtime 仍留 C。
- [x] 新增 Store search pattern Rust helper、glob/case flag/LIKE hint FFI smoke 與 `CBM_USE_RUST_STORE_SEARCH_PATTERN=1` 窄範圍 opt-in；只委派 glob→LIKE、LIKE hint extraction 與 `(?i)` case flag byte-level helper，SQL builder、bind lifetime、FTS/BM25 與 SQLite runtime 仍留 C。
- [x] 新增 Store architecture helper Rust helper、`cbm_rs_store_qn_to_package_v1`、`cbm_rs_store_qn_to_top_package_v1`、`cbm_rs_store_is_test_file_path_v1`、`cbm_rs_store_hop_to_risk_v1` 與 `cbm_rs_store_risk_label_v1`，並以 `CBM_USE_RUST_STORE_ARCH_HELPERS=1` 委派 QN 解析/風險與 path 判斷，保持 SQLite/open/runtime 不移轉。
- [x] 擴充 Store dump side-effect golden：在 `store_compat` 固定 `cbm_store_dump_to_file()` 輸出資料可讀、目標 DB journal mode 維持 WAL、`.tmp` 不殘留，以及 `cbm_store_open_path_query()` readonly 拒寫；仍不遷移 SQLite dump/open runtime。
- [x] 擴充 Store CRUD/schema side-effect golden：在 `store_compat` 固定 `cbm_store_delete_nodes_by_file()` 會透過 SQLite cascade 同時清掉 source/target 任一端連到刪除節點的 edges，未刪除 file 的 node 保留可查；仍不遷移 CRUD runtime。
- [x] 擴充 Store checkpoint side-effect golden：在 `store_compat` 固定 `cbm_store_checkpoint()` 成功後 `-wal` 檔仍存在且非空，避免未來 Rust Store runtime 誤用 truncating checkpoint；仍不遷移 WAL/checkpoint runtime。
- [x] 新增 Store arch path scope Rust helper `cbm_rs_store_normalize_arch_path_v1` 與 `CBM_USE_RUST_STORE_ARCH_PATH_SCOPE=1` 窄 opt-in，讓 `arch_path_prepare` 只委派 norm_out 正規化（跳過前導空白/`./`/`/`、`cap-1` 截斷對齊 C `strncpy`、去尾端 ` `/`\t`/`/` 與折疊重複 `/`）；`like_out` 的 `%s/%%` 組裝、SQLite scope query 與 bind 仍留 C，`arch_path_is_set` 以 `#ifndef` 守衛避免 opt-in 下未使用。
- [x] 新增 Store file_ext Rust helper `cbm_rs_store_file_ext_lower_v1` 與 `CBM_USE_RUST_STORE_FILE_EXT=1` 窄 opt-in，讓 `file_ext()` 委派「取最後一個 `.` 起副檔名 + ASCII 小寫 + 長度 >= buffer 拒絕」的純轉換（回傳 C TLS buffer）；`strrchr`/TLS buffer 與 `ext_to_lang` 表查找仍留 C。
- [x] 修正 Store arch path scope 與 file_ext opt-in wiring：`CBM_USE_RUST_STORE_ARCH_PATH_SCOPE=1` / `CBM_USE_RUST_STORE_FILE_EXT=1` 現在會定義 C macro 並納入 Rust staticlib link trigger，避免既有 opt-in target 空跑。
- [x] 新增第一批 `cypher` contract fixture：parser AST shape、deterministic ordered rows、exact unsupported CREATE error。
- [x] 擴充 `cypher` contract fixture：WITH/post-WHERE AST、explicit `LIMIT 0` columns/rows、list indexing exact error。
- [x] 擴充 `cypher` golden fixture：OPTIONAL MATCH、UNION、edge property、unknown function 與更多 exact error messages。
- [x] 新增 Cypher lexer Rust test-only parity module 與 FFI smoke，固定 token kind、byte offset、string escape、comments、number / `..` boundary、summary truncation 與 C enum ABI；不接 production opt-in。
- [x] 新增 Cypher normalized AST summary Rust test-only parity helper 與 FFI smoke，固定 parser AST shape、OPTIONAL MATCH、UNION ALL、EXISTS direction、WITH/post-WHERE、caller-buffer truncation 與 malformed query contract；不接 production opt-in。
- [x] 擴充 Cypher hop shorthand parity，固定 `*N -> 1..N`、`* -> 1..0`、`*..N -> 1..N` 的 C parser AST contract 與 Rust normalized AST summary。
- [x] 新增 Cypher scalar function canonical Rust helper `cbm_rs_cypher_scalar_func_index_v1`（回傳 names 索引或 -1，ASCII 大小寫不敏感對齊 `cyp_ci_eq`），並以 `CBM_USE_RUST_CYPHER_SCALAR_FUNC=1` 讓 `scalar_func_canonical()` 委派 Rust 索引後映射回 C static 名稱字串（保留原回傳 static 指標語意）；為 Cypher 第一個 production opt-in。parser/AST normalizer/evaluator/executor 仍留 C。
- [x] 修正 Cypher scalar function opt-in wiring：`CBM_USE_RUST_CYPHER_SCALAR_FUNC=1` 現在會定義 C macro 並納入 Rust staticlib link trigger，讓既有 `rust-cypher-scalar-func-optin-test` 實際穿過 Rust helper。
- [x] 新增 Cypher multi-arg function canonical Rust helper `cbm_rs_cypher_multiarg_func_index_v1`，讓 `CBM_USE_RUST_CYPHER_MULTIARG_FUNC=1` 時 `multiarg_func_canonical()` 委派 Rust 先回傳 names 索引，再映射回 C static 名稱字串；支援 `coalesce`、`substring`、`replace`、`left`、`right`，同時保留 parser/AST/evaluator/executor 在 C。
- [x] 修正 Cypher multi-arg function wiring：`CBM_USE_RUST_CYPHER_MULTIARG_FUNC=1` 已接入 `rust-cypher-multiarg-func-optin-test` 與 `rust-ci`，並沿用既有 C contract。
- [x] 新增 Cypher aggregate function token-name Rust helper `cbm_rs_cypher_aggregate_func_index_v1`，讓 `CBM_USE_RUST_CYPHER_AGG_FUNC=1` 時 `agg_func_name()` 委派 Rust 將 `TOK_COUNT/SUM/AVG/MIN/MAX/COLLECT` 映射到 C static `names[]` 索引；parse item、ORDER BY formatting、aggregation evaluator/executor 與 result 行為仍留 C。
- [x] 新增 Cypher string function token-name Rust helper `cbm_rs_cypher_string_func_index_v1`，讓 `CBM_USE_RUST_CYPHER_STRING_FUNC=1` 時 `str_func_name()` 委派 Rust 將 `TOK_TOLOWER/TOUPPER/TOSTRING` 映射到 C static `names[]` 索引；parse item、scalar/string evaluator 與 result 行為仍留 C。
- [x] 擴充 `mcp` transcript fixture：`tools/list` pagination、原始 tool 順序、notification no-response、error code、unknown tool 與 Content-Length。
- [x] 擴充 `mcp` BM25 fixture：`search_graph` query path 的 `file_pattern`、label boost rank/order、`total` 與 `has_more` contract。
- [x] 擴充 `mcp` transcript fixture：alias、tool schema、indexed repo query。
- [x] 新增 MCP JSON-RPC request summary Rust test-only parity module 與 `cbm_rs_mcp_jsonrpc_request_summary_v1` FFI smoke，固定 request envelope parse contract；不接 production opt-in。
- [x] 新增 MCP JSON-RPC parse Rust helper、`cbm_rs_mcp_jsonrpc_parse_v1` FFI smoke 與 `CBM_USE_RUST_MCP_CODEC=1` 窄範圍 opt-in；只委派 `cbm_jsonrpc_parse()` request envelope parse，response formatting、Content-Length transport、tool schema 與 14 個 handlers 仍留 C。
- [x] 新增 MCP `tools/list` cursor offset Rust helper `cbm_rs_mcp_tools_cursor_offset_v1` 與 `mcp::tools_cursor_offset`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `mcp_tools_cursor_offset()` 委派 Rust（傳入 `TOOL_COUNT`）；Rust 以既有 JSON parser 驗證整份 params、擷取首個 `cursor` 字串，並鏡像 `strtol` + clamp 語意（前導空白/正負號/完整消耗/i64 溢位/非負/`-0`）。response formatting、framing 與 handlers 仍留 C。
- [x] 新增 MCP `tools/call` dispatch name Rust helper `cbm_rs_mcp_tools_call_name_v1` 與 `mcp::tools_call_name`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_get_tool_name()` 委派 Rust 解析首個 `name` 字串；C wrapper 仍負責配置 C-owned 字串，`arguments`、tool schema、response formatting、framing 與 handlers 仍留 C/yyjson。
- [x] 新增 MCP `notifications/cancelled` request matcher Rust helper `cbm_rs_mcp_cancel_request_matches_v1` 與 `mcp::cancel_request_matches`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_cancel_request_matches()` 委派 Rust 解析首個 `requestId`；server lifecycle、pipeline cancellation side effect、logging、response formatting 與 framing 仍留 C。
- [x] 新增 MCP initialize protocol version Rust helper `cbm_rs_mcp_initialize_protocol_version_v1` 與 `mcp::initialize_protocol_version`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_initialize_response()` 只把 `params.protocolVersion` selection 委派 Rust；initialize response JSON 組裝、`serverInfo`、capabilities、yyjson ownership、framing 與 dispatch 仍留 C。
- [x] 新增 MCP initialize response Rust helper `cbm_rs_mcp_initialize_response_v1` 與 `mcp::initialize_response`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_initialize_response()` 的 result object JSON builder 委派 Rust；initialize dispatch side effects、JSON-RPC response wrapper、framing、server lifecycle 與 handlers 仍留 C。
- [x] 新增 MCP JSON-RPC numeric-id error formatter Rust helper `cbm_rs_mcp_jsonrpc_format_error_v1` 與 `mcp::jsonrpc_format_error`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_jsonrpc_format_error()` 委派 Rust；string-id response、result embedding、Content-Length transport、tool schema、dispatch 與 handlers 仍留 C。
- [x] 新增 MCP JSON-RPC response formatter Rust helper `cbm_rs_mcp_jsonrpc_format_response_v1` 與 `mcp::jsonrpc_format_response`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_jsonrpc_format_response()` 委派 Rust；Content-Length transport、tool schema、dispatch 與 handlers 仍留 C。
- [x] 新增 MCP text result formatter Rust helper `cbm_rs_mcp_text_result_v1` 與 `mcp::mcp_text_result`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_text_result()` 委派 Rust；Content-Length transport、tool schema、dispatch 與 handlers 仍留 C。
- [x] 新增 MCP tool index Rust helper `cbm_rs_mcp_tool_index_v1` 與 `mcp::tool_index`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_tool_input_schema()` 的 tool name -> `TOOLS[]` index 查找委派 Rust；schema JSON、tools/list serialization、dispatch 與 handlers 仍留 C。
- [x] 新增 MCP `tools/list` page bounds Rust helper `cbm_rs_mcp_tools_page_bounds_v1` 與 `mcp::tools_page_bounds`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_tools_list_range()` 的 offset/limit clamp 與 nextCursor decision 委派 Rust；schema JSON、tools/list serialization、dispatch 與 handlers 仍留 C。
- [x] 新增 MCP `tools/list` tool name/title/description/inputSchema/outputSchema manifest Rust helper `cbm_rs_mcp_tool_count_v1` / `cbm_rs_mcp_tool_name_v1` / `cbm_rs_mcp_tool_title_v1` / `cbm_rs_mcp_tool_description_v1` / `cbm_rs_mcp_tool_input_schema_v1` / `cbm_rs_mcp_tool_output_schema_v1` 與 `mcp::tool_count` / `mcp::tool_name` / `mcp::tool_title` / `mcp::tool_description` / `mcp::tool_input_schema` / `mcp::tool_output_schema`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `tools/list` 的 name/title/description/inputSchema/outputSchema 欄位於 Rust/C manifest 一致時由 Rust 輸出；JSON parse/copy/serialization、dispatch 與 handlers 仍留 C。
- [x] 新增 MCP `tools/list` result JSON builder Rust helper `cbm_rs_mcp_tools_list_json_v1` 與 `mcp::tools_list_json`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_tools_list_range()` 可直接使用 Rust 產生 compact result object；JSON-RPC response wrapper、Content-Length transport、server loop、dispatch 與 handlers 仍留 C。
- [x] 新增 MCP Content-Length header classifier Rust helper `cbm_rs_mcp_content_length_header_matches_v1` 與 `mcp::content_length_header_matches`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_server_run()` 是否進入 framed branch 的 `Content-Length:` 前綴判斷委派 Rust；body allocation/read、poll loop、server lifecycle、dispatch 與 handlers 仍留 C。
- [x] 新增 MCP Content-Length header parser Rust helper `cbm_rs_mcp_content_length_v1` 與 `mcp::content_length_value`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_server_run()` 的 Content-Length length gate 委派 Rust；body read、response framing、poll loop、server lifecycle、dispatch 與 handlers 仍留 C。
- [x] 新增 MCP Content-Length header/body separator Rust helper `cbm_rs_mcp_content_length_header_is_blank_v1` 與 `mcp::content_length_header_is_blank`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `handle_content_length_frame()` 的 header blank line 判斷委派 Rust；getline、body allocation/read、poll loop、server lifecycle、dispatch 與 handlers 仍留 C。
- [x] 新增 MCP Content-Length response framing Rust helper `cbm_rs_mcp_content_length_response_v1` 與 `mcp::content_length_response`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `handle_content_length_frame()` 可用 Rust 產生 `Content-Length: <byte_len>\r\n\r\n...` frame；body read、poll loop、server lifecycle、dispatch 與 handlers 仍留 C。
- [x] 新增 MCP file URI parser Rust helper `cbm_rs_mcp_parse_file_uri_v1` 與 `mcp::parse_file_uri`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_parse_file_uri()` 的 pure URI path extraction 委派 Rust；caller buffer ownership、錯誤時清空輸出、root/session lifecycle 與 URI 使用端仍留 C。
- [x] 新增 MCP method dispatch classifier Rust helper `cbm_rs_mcp_method_kind_v1` 與 `mcp::method_kind`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_server_handle()` 的 method 分類委派 Rust；initialize/ping/tools/list/tools/call/cancelled branch 的副作用、response wrapping、logging、server lifecycle 與 handlers 仍留 C。
- [x] 新增 MCP tool dispatch classifier Rust helper `cbm_rs_mcp_tool_dispatch_index_v1` 與 `mcp::tool_dispatch_index`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_handle_tool()` 先用 Rust 回傳的 handler index 選擇既有 C handler；`trace_call_path` alias 對應 `trace_path`，Rust/C `TOOLS[]` 驗證不一致時 fallback 到原 `strcmp` chain，handlers 與參數解析仍留 C。
- [x] 新增 MCP Method not found error object Rust helper `cbm_rs_mcp_method_not_found_error_v1` 與 `mcp::method_not_found_error`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_server_handle()` unknown-method branch 的 error object 建構委派 Rust；request id echo、JSON-RPC response wrapper、logging、framing、server lifecycle 與 handlers 仍留 C。
- [x] 新增 MCP parse-error message Rust helper `cbm_rs_mcp_parse_error_message_v1` 與 `mcp::parse_error_message`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_server_handle()` request parse 失敗時的 `Parse error` message 建構委派 Rust；numeric-id JSON-RPC error formatter、framing、logging、server lifecycle、dispatch 與 handlers 仍留 C/既有 Rust formatter 分工路徑。
- [x] 新增 MCP ping result object Rust helper `cbm_rs_mcp_ping_result_v1` 與 `mcp::ping_result`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_server_handle()` `ping` branch 的 `result_json` 建構委派 Rust；JSON-RPC response wrapper、logging、framing、server lifecycle 與 handlers 仍留 C。
- [x] 新增 MCP `tools/call` no-params default arguments Rust helper `cbm_rs_mcp_tools_call_default_arguments_v1` 與 `mcp::tools_call_default_arguments`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_server_handle()` 於 request 沒有 `params` 時用 Rust 建構 handler arguments fallback；tool dispatch、handler-specific 參數解析、response wrapper、logging、framing、server lifecycle 與 handlers 仍留 C。
- [x] 新增 MCP missing-tool-name error text Rust helper `cbm_rs_mcp_missing_tool_name_message_v1` 與 `mcp::missing_tool_name_message`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_handle_tool()` 的 `tool_name == NULL` branch 建構 `missing tool name` error text 時可委派 Rust；ABI 異常或 buffer 放不下時 fallback 到原 C literal，`cbm_mcp_text_result()`、tool dispatch、response wrapper、logging、framing、server lifecycle 與 handlers 仍留 C。
- [x] 新增 MCP missing-project error object Rust helper `cbm_rs_mcp_missing_project_error_v1` 與 `mcp::missing_project_error`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `build_missing_project_error()` 的固定 JSON error object 可委派 Rust；available projects 掃描、Store resolution、result formatting、transport 與 handlers 仍留 C。
- [x] 新增 MCP project-not-found message Rust helper `cbm_rs_mcp_project_not_found_message_v1` 與 `mcp::project_not_found_message`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `build_project_not_found_message()` 的固定 error text 可委派 Rust；project list 掃描、Store resolution、result formatting、transport 與 handlers 仍留 C。
- [x] 新增 MCP project-list error JSON builder Rust helper `cbm_rs_mcp_project_list_error_v1` 與 `mcp::project_list_error`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `build_project_list_error()` 的固定 JSON 序列化可委派 Rust；cache directory 掃描、project name collect、Store resolution、transport 與 handlers 仍留 C。
- [x] 新增 MCP unknown-tool error text Rust helper `cbm_rs_mcp_unknown_tool_message_v1` 與 `mcp::unknown_tool_message`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_handle_tool()` 找不到 handler 時的 `unknown tool: <name>` error text 建構委派 Rust；長度放不進既有 `CBM_SZ_256` stack buffer 或 ABI 異常時 fallback 到 C `snprintf`，`cbm_mcp_text_result()`、tool dispatch、response wrapper、logging、framing、server lifecycle 與 handlers 仍留 C。
- [x] 新增 MCP relationship edge-type validator Rust helper `cbm_rs_mcp_edge_type_valid_v1` 與 `mcp::edge_type_valid`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `search_graph.relationship` 的 `validate_edge_type()` 委派 Rust；只遷移 ASCII 大寫/underscore/64 bytes pure classifier，`search_graph` 參數解析、store search/query planning、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 新增 MCP `search_code` path/file_pattern validator Rust helper `cbm_rs_mcp_search_path_arg_valid_v1` 與 `mcp::search_path_arg_valid`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `validate_search_path_arg()` 委派 Rust；只遷移 shell quoting denylist pure classifier，grep command construction、temp file、regex/search execution、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 新增 MCP `search_code` root/file args combiner Rust helper `cbm_rs_mcp_search_args_valid_v1` 與 `mcp::search_args_valid`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `validate_search_args()` 委派 Rust；只遷移 root/path 雙參數 pure combiner，grep command construction、search execution、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 新增 MCP `search_code` root prefix strip offset Rust helper `cbm_rs_mcp_strip_root_prefix_offset_v1` 與 `mcp::search_strip_root_prefix_offset`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `strip_root_prefix()` 委派 Rust 計算 borrowed offset；只遷移 grep result path 的 prefix offset，grep/search/JSON/handler runtime 與 path ownership 仍留 C。
- [x] 新增 MCP `search_code.mode` classifier Rust helper `cbm_rs_mcp_search_mode_v1` 與 `mcp::search_mode`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `parse_search_mode()` 委派 Rust；只遷移 compact/full/files pure classifier，grep command construction、result shaping、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 新增 traces 純 helper Rust parity module `rust/cbm-core/src/traces.rs`、`cbm_rs_traces_extract_service_name_v1` / `cbm_rs_traces_extract_http_info_v1` / `cbm_rs_traces_extract_path_from_url_v1` / `cbm_rs_traces_parse_duration_v1` / `cbm_rs_traces_calculate_p99_v1` 與 `CBM_USE_RUST_TRACES=1` opt-in；`src/traces/traces.c` 預設仍保留 C fallback，`tests/test_traces.c` 與 `tests/test_rust_ffi.c` 已可共同驗證 helper 契約。
- [x] 收斂 traces Rust 安全邊界：`traces.rs` 僅保留純位元組/值型 helper，C 指標、caller buffer 與 URL static buffer 集中於 `ffi.rs`；新增 Rust FFI URL path 回歸測試，並以 `cargo test -p cbm-core traces --locked`（7 passed）、`make -f Makefile.cbm rust-ffi-test`、`python3 scripts/rust-security-audit.py`（557 findings allowlist passed）驗證。安全掃描只比對 Rust 關鍵字語法，避免 schema 文字誤報；未新增 traces allowlist。
- [x] 新增 traces Rust-only opt-in：`CBM_USE_RUST_TRACES_ONLY=1` 時排除 `src/traces/traces.c`，Cargo `traces-only` feature 直接匯出 `traces.h` 的五個公開 `cbm_*` symbols；保留預設 C fallback 與既有 `CBM_USE_RUST_TRACES=1` wrapper。Make 的 default/Rust-only staticlib 分別隔離在 `target/cbm-default` / `target/cbm-traces-only`，`rust-traces-only-test` 使用隔離 runner 驗證兩條 `traces mcp` 路徑（各 161 passed）。
- [x] 新增 pipeline configures Rust helper `rust/cbm-core/src/pipeline/configures.rs`、`cbm_rs_pipeline_is_env_var_name_v1` / `cbm_rs_pipeline_normalize_config_key_v1` / `cbm_rs_pipeline_has_config_extension_v1` 與 `CBM_USE_RUST_PIPELINE_CONFIGURES=1` opt-in；`src/pipeline/pass_configures.c` 預設仍保留 C fallback，`tests/test_rust_ffi.c` 與 Rust unit tests 已可共同驗證 env var / config key / extension 契約。
- [x] 新增 pipeline configures Rust-only opt-in：`CBM_USE_RUST_PIPELINE_CONFIGURES_ONLY=1` 時排除 `src/pipeline/pass_configures.c`，Cargo `pipeline-configures-only` feature 直接匯出三個既有 `cbm_*` C ABI；保留預設 C fallback 與 `CBM_USE_RUST_PIPELINE_CONFIGURES=1` wrapper，並以獨立 default/Rust-only runner 與 feature target dir 避免 build/archive 混用。
- [x] 新增 pipeline infrascan filename classifier Rust helper `rust/cbm-core/src/pipeline/infrascan.rs`、六個 `cbm_rs_pipeline_is_*_v1` FFI 與 `CBM_USE_RUST_PIPELINE_INFRASCAN=1` opt-in；只委派 Dockerfile、Compose、env、Cloud Build、Kustomize 與 shell 檔名判斷，k8s content、secret 與各類 source parser 仍留 C。
- [x] 新增 pipeline K8s dependency-manifest classifier Rust helper `is_helm_chart_file()` / `is_gomod_file()` / `is_requirements_file()`、三個 `cbm_rs_pipeline_is_*_v1` FFI 與 `CBM_USE_RUST_PIPELINE_K8S_FILE_CLASSIFIERS=1` opt-in；只委派 `Chart.yaml` / `Chart.yml`、`go.mod` 與 `requirements.txt` 的檔名分流，檔案讀取、dependency parser、圖寫入與 pipeline side effects 仍留 C。
- [x] 新增 pipeline githistory trackable-file Rust helper `rust/cbm-core/src/pipeline/githistory.rs`、`cbm_rs_pipeline_is_trackable_file_v1` FFI 與 `CBM_USE_RUST_PIPELINE_GITHISTORY=1` opt-in；只委派 path prefix、lock/generated basename 與非 source suffix classifier，git log/coupling/FILE_CHANGES_WITH 與 I/O 仍留 C。
- [x] 新增 MCP `trace_call_path` test-file classifier Rust helper `cbm_rs_mcp_trace_is_test_file_v1` 與 `mcp::trace_is_test_file`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `is_test_file()` 委派 Rust；只遷移 MCP-local substring classifier，BFS traversal、include_tests filtering、JSON result shaping、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 新增 MCP project DB filename classifier Rust helper `cbm_rs_mcp_project_db_file_name_v1` 與 `mcp::project_db_file_name`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `is_project_db_file()` 委派 Rust；只遷移 cache scan filename classifier，directory scanning、SQLite open/internal-name resolution、list JSON、resolve fallback、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 新增 MCP `index_repository.mode` classifier Rust helper `cbm_rs_mcp_index_mode_v1` 與 `mcp::index_mode`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `parse_index_repository_mode()` 委派 Rust；只遷移 full/moderate/fast/cross-repo pure classifier，pipeline creation/run、cross-repo matching、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 新增 MCP `trace_path.mode` default edge-type classifier Rust helper `cbm_rs_mcp_trace_mode_edge_mask_v1` 與 `mcp::trace_mode_edge_mask`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `resolve_trace_edge_types()` 無 explicit `edge_types` 時委派 Rust；只遷移 mode -> default edge set pure classifier，explicit edge parsing、BFS traversal、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 新增 MCP `search_code` output ASCII sanitizer Rust helper `cbm_rs_mcp_sanitize_ascii_in_place_v1` 與 `mcp::sanitize_ascii_in_place`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `sanitize_ascii()` 委派 Rust；只遷移 `>127` byte 就地改 `?` 的 pure mutator，grep/source/context/JSON/result shaping、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 新增 MCP `search_code` ranking scorer Rust helper `cbm_rs_mcp_search_code_score_v1` 與 `mcp::search_code_score`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `compute_search_score()` 委派 Rust；只遷移 label/file/in_degree pure scorer，grep、graph lookup、dedup/classification、sort comparator、JSON 與 handlers 仍留 C。
- [x] 新增 MCP `search_code` score comparator Rust helper `cbm_rs_mcp_search_score_cmp_v1` 與 `mcp::search_score_cmp`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `search_result_cmp()` 委派 Rust 計算 descending score delta；qsort、result struct ownership、grep、graph lookup、JSON 與 handlers 仍留 C。
- [x] 新增 MCP `search_code` directory top-key Rust helper `cbm_rs_mcp_search_top_dir_v1` 與 `mcp::search_top_dir`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `build_dir_distribution()` 的 top-level directory key 擷取可委派 Rust；directory count、JSON building、search result ownership、grep、graph lookup 與 handlers 仍留 C。
- [x] 新增 MCP `detect_changes.scope` classifier Rust helper `cbm_rs_mcp_detect_changes_wants_symbols_v1` 與 `mcp::detect_changes_wants_symbols`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `detect_changes_wants_symbols()` 委派 Rust；只遷移 scope -> impacted symbols pure classifier，project/root resolution、git diff/status、Store symbol lookup、JSON 與 handlers 仍留 C。
- [x] 新增 MCP `detect_changes` impacted-symbol label filter Rust helper `cbm_rs_mcp_detect_changes_impacted_label_v1` 與 `mcp::detect_changes_impacted_label`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `detect_changes_impacted_label()` 委派 Rust；只遷移 label -> impacted include/exclude pure classifier，Store node query、node ownership、JSON 與 handlers 仍留 C。
- [x] 新增 MCP trace/snippet candidate scorer Rust helper `cbm_rs_mcp_node_resolution_score_v1` 與 `mcp::node_resolution_score`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `node_resolution_score()` 委派 Rust；只遷移 label-tier + span pure scorer，`pick_resolved_node()` ambiguity/tie、candidate query、BFS/snippet response shaping 與 handlers 仍留 C。
- [x] 新增 MCP `manage_adr.mode` classifier Rust helper `cbm_rs_mcp_adr_mode_v1` 與 `mcp::adr_mode`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `parse_adr_mode()` 委派 Rust；只遷移 get/update-store/sections pure classifier，project/store resolution、legacy migration、ADR read/write、sections JSON 與 handlers 仍留 C。
- [x] 新增 MCP `manage_adr(mode=sections)` section JSON builder Rust helper `cbm_rs_mcp_adr_sections_json_v1` 與 `mcp::adr_sections_json`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `adr_list_sections_from_content()` 委派 Rust 產生 sections JSON array；project/store resolution、legacy migration、ADR read/write、response wrapping 與 handlers 仍留 C。
- [x] 新增 MCP `search_graph.query` BM25 MATCH builder Rust helper `cbm_rs_mcp_bm25_build_match_v1` 與 `mcp::bm25_match_query`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `bm25_build_match()` 委派 Rust；只遷移 FTS5 MATCH tokenizer/serializer，SQLite/FTS5 execution、ranking/boosting、file pattern LIKE、result JSON 與 handlers 仍留 C。
- [x] 新增 MCP `search_graph.file_pattern` BM25 LIKE builder Rust helper `cbm_rs_mcp_bm25_file_pattern_like_v1` 與 `mcp::bm25_file_pattern_like`，在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `bm25_file_pattern_like()` 委派 Rust；只遷移 glob-to-LIKE + contains wrapping pure serializer，SQLite/FTS5 execution、LIKE binding/query planning、ranking、result JSON 與 handlers 仍留 C。

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
- [x] 擴充 language graph parity gate，新增 JavaScript 小型 fixture（class 方法呼叫、`function` 定義、CommonJS `require`/`module.exports` import、express `/tasks/:id` route），固定 JS definitions、calls、imports 與 route 觀測面，並確認 default C path 與 Rust pipeline opt-in path graph 完全一致。
- [x] 新增 `pipeline::fqn` Rust test-only helper 與 `cbm_rs_pipeline_project_name_from_path` FFI smoke，固定 `cbm_project_name_from_path` 的 byte-level sanitizer、drive normalization、non-ASCII hex encoding 與 buffer truncation contract；不接 production opt-in。
- [x] 新增 FullPipeline top-level orchestration metadata FFI（`cbm_rs_pipeline_full_plan_step_count_v1` / `cbm_rs_pipeline_full_plan_steps_v1`），固定 12 個 full/moderate/advanced outer steps、fast-mode 11 steps、`requires_mask`、gate/effect flags 與 nested plan kind；`PlanStepV2(kind=FullPipeline)` 仍維持 unsupported。
- [x] 固定 incremental dump failure side-effect contract：dump 失敗會原樣傳回錯誤，不再落入 full fallback，且會跳過後續 `persist_hashes` / `artifact_export` tail。
- [x] 將 FullPipeline top-level orchestration metadata 接入 `pipeline.c` runtime：`CBM_USE_RUST_PIPELINE_PLAN=1` 會在 discovery 後以實際 `mode/worker_count/file_count` 載入 `top_v1` full pipeline step order、記錄 `rust_plan.dispatch phase=full_pipeline`，並把 top-level `extraction_dispatch` nested plan kind 傳入 extraction choice；實際 pass function、graph-buffer ownership 與 incremental runtime 仍留 C。
- [x] 新增 `pipeline::route` Rust helper 與 `CBM_USE_RUST_PIPELINE_ROUTE_CANON=1` opt-in，讓 `cbm_route_canon_path()` 的純 route placeholder normalization 可委派 Rust；route extraction、Route node/edge 建立、graph-buffer mutation 與 pipeline side effects 仍留 C。
- [x] 新增 `cbm_rs_pipeline_test_to_prod_path_v1` FFI smoke，讓 `CBM_USE_RUST_PIPELINE_TEST_DETECT=1` 也可委派 `test_to_prod_path()` 的 production path rewrite；`TESTS` / `TESTS_FILE` edge creation、`node_is_test()`、graph-buffer adapter、tree-sitter/extraction 與 pipeline side effects 仍留 C。
- [x] 新增 `pipeline::test_detect` Rust helper 與 `CBM_USE_RUST_PIPELINE_TEST_DETECT=1` opt-in，讓 `cbm_is_test_path()` / `cbm_is_test_func_name()` 的純 classifier 與 `test_to_prod_path()` 的 path rewrite 可委派 Rust；`TESTS` / `TESTS_FILE` edge creation、`node_is_test()`、graph-buffer adapter 與 pass side effects 仍留 C。
- [x] 完成第 18 個 Rust true-source 切片：新增 `src/pipeline/test_node_is_test.c/.h` C fallback、公開 `cbm_pipeline_test_node_is_test()` bridge、wrapper v1 與 `pipeline-test-node-is-test-only` direct ABI；嚴格連續 `"is_test":true` 判定且 `NULL` 回 `false`。default/wrapper/direct FFI 與 pipeline gate 各 228 passed，wrapper/direct `--version` 通過，ONLY selector 為空，direct production `make -Bn` source-negative 不含該 C compilation unit。
- [x] 完成第 19 個 Rust true-source 切片：新增 `src/semantic/camel_break.c/.h` C fallback 與公開 `cbm_semantic_is_camel_break(const char *name, int index)` ABI；`NULL` 或 `index <= 0` 回 `false`，僅前一個 ASCII 小寫且目前 ASCII 大寫的有效非 NUL byte 回 `true`。既有 `CBM_USE_RUST_SEMANTIC_TOKEN_CLASSIFIERS=1` wrapper 經 `cbm_rs_semantic_is_camel_break_v1` 保留 C fallback；只有 `CBM_USE_RUST_SEMANTIC_CAMEL_BREAK_ONLY=1` / `semantic-camel-break-only` 為 direct replacement，delimiter/tokenization 未遷移。direct `SEMANTIC_CAMEL_BREAK_SRCS =` 為空且 `make -Bn` source-negative 不含 `camel_break.c`；Rust fmt/unit（2 passed）/clippy、default/wrapper/direct FFI、各 pipeline 228 passed、wrapper/direct `--version` 均通過，完整 `scripts/test.sh` 未重跑。
- [x] 完成第 20 個 Rust true-source 切片：新增 `src/semantic/token_delim.c/.h` C fallback 與公開 scalar ABI `cbm_semantic_is_token_delim(unsigned char byte)`；九個 ASCII delimiter `.`、`/`、`_`、`-`、空白、`(`、`)`、`,`、`:` 回 `true`，其他所有 byte（含高位 byte）回 `false`。既有 `CBM_USE_RUST_SEMANTIC_TOKEN_CLASSIFIERS=1` wrapper 經 `cbm_rs_semantic_is_token_delim_v1` 保留 C fallback；只有 `CBM_USE_RUST_SEMANTIC_TOKEN_DELIM_ONLY=1` / `semantic-token-delim-only` 為 direct replacement，camel-break 仍是獨立切片。direct `SEMANTIC_TOKEN_DELIM_SRCS =` 為空且 `make -Bn` source-negative 不含 `token_delim.c`；tokenization、ownership 與 semantic orchestration 仍在 C。Rust fmt/unit（2 passed）/clippy、default/wrapper/direct FFI、各 pipeline 228 passed、三路 production `--version`、direct selector/source-negative 均通過，完整 `scripts/test.sh` 未重跑。
- [x] 完成第 21 個 Rust true-source 切片：新增 `src/pipeline/registry_test_qn.c/.h` C fallback 與公開 `cbm_pipeline_registry_is_test_qn(const char *qn)` ABI；`NULL`／empty false，對有效 NUL 結尾 C string 以大小寫敏感 raw substring 判定 `Test`、`test`、`Mock`、`mock`、`Stub`、`stub`、`Fake`、`fake`、`Fixture`、`spec`，`contest` true，non-UTF-8 raw-safe。wrapper `CBM_USE_RUST_PIPELINE_REGISTRY_TEST_QN=1` 只採 v1 `0`／`1`，其他值回 C；direct 為 `CBM_USE_RUST_PIPELINE_REGISTRY_TEST_QN_ONLY=1` / `pipeline-registry-test-qn-only`。`PIPELINE_REGISTRY_TEST_QN_SRCS =`，`make -Bn` 不含 `registry_test_qn.c`；registry handle/cache/resolution/graph/ownership 仍 C。Rust fmt、registry Rust unit 4 passed、clippy、default/wrapper/direct FFI 與 registry 各 54 passed、三路 production `--version` 通過；`scripts/test.sh` 未重跑。
- [x] 2026-07-17 LSP cross classifier parity regression 修復：Rust `LANG_C` 由 `15` 改為 C enum 實際值 `14`，`CBM_LANG_BASH`（`15`）固定 false；Rust unit 3 passed，wrapper FFI smoke 與 pipeline 228 passed。此項不計入 true-source，未新增 direct selector。
- [x] 新增 `pipeline::exception` Rust helper 與 `CBM_USE_RUST_PIPELINE_CHECKED_EXCEPTION=1` opt-in，讓 `is_checked_exception()` 的純 classifier 可委派 Rust；`THROWS` / `RAISES` edge type 選擇、exception resolution、graph-buffer adapter 與 pass side effects 仍留 C。
- [x] `CBM_USE_RUST_PIPELINE_REGISTRY_TEST_QN=1`：新增 `pipeline::registry::is_test_qn()` / `cbm_rs_registry_is_test_qn_v1`，只委派 registry candidate score 的 test-QN raw-byte classifier；`NULL` / empty 回 `0`，命中既有大小寫敏感 needle 回 `1`，非 `0` / `1` ABI 回傳值回退 C。opaque registry handle、add/resolve/cache、所有權與 pipeline side effects 仍留 C。已實跑 Rust unit（4 passed）、FFI smoke、default/opt-in registry suite（各 53 passed），以及當時 38 個 common flags 的 wrapper/direct all-optin runner（各 5865 passed）與 production `--version` smoke；仍非 Rust-backed RC。
- [x] `CBM_USE_RUST_CYPHER_LEX_SINGLE_CHAR=1`：新增 `cypher::single_char_kind_or_eof()` / `cbm_rs_cypher_single_char_kind_v1`，只委派 `lex_single_char()` 的既有 15 個單字元 token classifier；C 僅採用 `TOK_EOF`、`TOK_LPAREN..TOK_EQ`、`TOK_PIPE`，其餘 ABI 回傳值回退既有 C `switch`。Rust 僅收回傳 scalar，token allocation、position、two-character lexer、parser、AST/evaluator/executor 仍留 C。已實跑 Rust unit（15 passed）、FFI smoke、default/opt-in Cypher gate，及 39-common-flag wrapper/direct all-optin runner（各 5866 passed）與 production `--version` smoke；仍非 Rust-backed RC。
- [x] `CBM_USE_RUST_CYPHER_LEX_TWO_CHAR=1`：新增 `cypher::two_char_kind_or_eof()` / `cbm_rs_cypher_two_char_kind_v1`，只委派 `lex_try_two_char()` 的六個 pair classifier；C 僅採用 `TOK_NEQ`、`TOK_EQTILDE`、`TOK_GTE`、`TOK_LTE`、`TOK_DOTDOT`，並維持 input/cursor、C-owned token text/allocation 與 pair-table fallback。Rust 僅收回傳 scalar，parser、AST/evaluator/executor 仍留 C。已實跑 Rust unit（16 passed）、FFI smoke、default/opt-in Cypher + contract gate（各 166 passed），及 41-common-flag wrapper/direct all-optin runner（各 5866 passed）與 production `--version` smoke；仍非 Rust-backed RC。
- [x] `CBM_USE_RUST_LOG_ENV_PARSE=1`：將既有 `cbm_rs_log_parse_level_value`／`cbm_rs_log_parse_format_value` 接入 `cbm_log_init_from_env()`；C 仍擁有 `getenv()`、global logger state、sink、stderr 與 lifecycle，Rust 只處理 borrowed value 到 scalar。C 僅採用合法 level `0..4`／format `0..1`；預設 C parser 與 fallback 不變。已實跑 Rust unit（2 passed）、FFI smoke、default/單一/combined foundation matrix（各 325 passed），及 40-common-flag wrapper/direct all-optin runner（各 5866 passed）與 production `--version` smoke；仍非 Rust-backed RC。
- [x] `CBM_USE_RUST_STORE_LANGUAGE_MAP=1`：新增 `store::arch_helpers::ext_language_kind()`／`cbm_rs_store_ext_lang_kind_v1`，Rust 將既有 `ext_lang_table` 的 44 個副檔名對齊為 `0..43` 索引，未知／`NULL`／大小寫不符回 `-1`。C 僅採用範圍內索引並從原本 C 靜態表取回 language 指標；其他 ABI 值回退 C 查表。SQLite architecture query、計數/排序、allocation 與 Store ownership 均留 C。已實跑 Rust unit（7 passed）、FFI smoke、default/opt-in `store_arch mcp`（各 187 passed），及 42-common-flag wrapper/direct all-optin runner（各 5867 passed）與 production `--version` smoke；仍非 Rust-backed RC。
- [x] 新增 `pipeline::artifact` Rust helper 與 `CBM_USE_RUST_PIPELINE_ARTIFACT_PATH=1` opt-in，讓 `artifact_path()` 的純 path builder 可委派 Rust；只遷移 `<repo>/.codebase-memory/<name>` 組裝，artifact export/import、metadata、`.gitattributes` 與 I/O 仍留 C。
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
- [ ] Pipeline orchestration 全面化：Rust 承接 incremental post-pass adapter、pass registry metadata、full/incremental dispatch 與 graph-buffer mutation 邊界（command contract、多批 C-side adapter 呼叫點與 FullPipeline top-level runtime metadata 消費已固定，完整資料流仍未完成）；language graph parity 持續擴充。
- [ ] Store：補完整 SQLite schema/index/FTS/checkpoint/bulk/artifact golden，分段遷移 open/readonly/WAL、CRUD/search、FTS/BM25、vector 與 writer path（schema/index/FTS shadow、connection/pragma static/filter contract、dump side-effect、CRUD cascade 與 checkpoint side-effect golden、FTS camelCase tokenization helper、mmap resolver、immutable URI builder、search pattern helper、architecture helper 窄 opt-in 已先完成）。
- [x] Cypher：補 CALL/EXISTS/malformed/aggregation exact golden，並新增 lexer/token summary 與 normalized AST summary Rust test-only parity FFI；production parser/evaluator/executor 仍留 C。
- [ ] Cypher：依序遷移 parser、AST normalizer、expression evaluator、projection/aggregation 與 executor。
- [ ] MCP：補 14 tools transcript 與錯誤路徑，依序遷移 JSON-RPC codec、tool schema generator、read-only handlers、狀態型 handlers 與 install/config 類 handler（JSON-RPC request summary、parse/name/arguments/default arguments/tool dispatch classifier/missing-tool-name error text/unknown-tool error text/cancel/initialize protocol/initialize response、numeric-id error formatter、response formatter、text result formatter、tool index/name/title/description/inputSchema/outputSchema manifest、tools/list page bounds、tools/list result JSON builder、Content-Length header parser/header separator/response framing、file URI parser、method classifier、method-not-found error object 與 ping result object opt-in 已起步；Content-Length body allocation/read、JSON-RPC wrapper、server lifecycle 與 handler 仍未遷移）。
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
- [x] Phase 3 Store arch path scope opt-in targeted gates（2026-07-06）：`cargo fmt --all -- --check`、`cargo test -p cbm-core store::arch_helpers --locked`（5 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner store_arch store_search store_compat mcp`（257 passed）、`make -f Makefile.cbm rust-store-arch-path-scope-optin-test`（245 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。SQLite open/CRUD/search runtime 仍留 C。
- [x] Phase 3 Store file_ext opt-in targeted gates（2026-07-06）：`cargo fmt --all -- --check`、`cargo test -p cbm-core store::arch_helpers --locked`（6 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner store_arch`（52 passed）、`make -f Makefile.cbm rust-store-file-ext-optin-test`（52 passed，Rust 路徑端到端等價）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- [x] 2026-07-07 Phase 3 Store checkpoint/wiring targeted gates：`build/c/test-runner store_compat`（15 passed）、`make -f Makefile.cbm rust-store-arch-path-scope-optin-test`（246 passed）、`make -f Makefile.cbm rust-store-file-ext-optin-test`（52 passed）。
- [x] 2026-07-07 Phase 3 Store checkpoint/bulk filtered manifest targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core store::schema_manifest --locked`（5 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test` 與 `make -f Makefile.cbm build/c/test-runner && build/c/test-runner store_compat`（15 passed）。
- [x] `make -f Makefile.cbm build/c/test-runner && build/c/test-runner cypher_contract`（2026-07-02：13 passed）
- [x] 2026-07-05 Phase 3 Cypher contract 補洞：`make -f Makefile.cbm build/c/test-runner && build/c/test-runner cypher_contract`（20 passed）
- [x] 2026-07-06 Phase 3 Cypher lexer parity targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（3 passed）、`cargo test -p cbm-core --locked`（80 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner cypher_contract`（20 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- [x] 2026-07-06 Phase 3 Cypher AST summary parity targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（6 passed）、`cargo test -p cbm-core --locked`（83 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner cypher_contract`（20 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- [x] 2026-07-06 Phase 3 MCP JSON-RPC request summary parity targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（4 passed）、`cargo test -p cbm-core --locked`（90 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner mcp`（119 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- [x] 2026-07-06 Phase 3 MCP JSON-RPC parse codec opt-in targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（6 passed）、`cargo test -p cbm-core --locked`（98 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm build/c/test-runner && build/c/test-runner mcp`（123 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（123 passed）、`make -f Makefile.cbm CBM_USE_RUST_MCP_CODEC=1 build/c/test-runner && build/c/test-runner mcp`（123 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- [x] 2026-07-06 Phase 3 MCP tools cursor offset opt-in targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（7 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（124 passed，含新 `server_handle_tools_list_cursor_edge_cases` 對齊 yyjson）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（124 passed，Rust 路徑端到端等價）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- [x] 2026-07-07 Phase 3 MCP tools/call name + cancelled matcher targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（9 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（124 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（124 passed）。
- [x] 2026-07-07 Phase 3 MCP tools/call arguments targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（10 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（124 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（124 passed）。
- [x] 2026-07-07 Phase 3 MCP initialize protocol version targeted gates：`cargo fmt --all`、`cargo test -p cbm-core mcp --locked`（11 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（124 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（124 passed）。
- [x] 2026-07-08 Phase 3 MCP parse-error message opt-in targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（36 passed）、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm lint-format` 與 `git diff --check`。曾有一次 default C runner 與 opt-in target 並行造成 `build/c` object 被清掉的 linker failure；已用序列重跑 default gate 通過。
- [x] 2026-07-09 Phase 3 MCP string argument extraction opt-in targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（37 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（125 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）。
- [x] 2026-07-09 Phase 3 MCP int/bool argument extraction opt-in targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（38 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（125 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）。
- [x] 2026-07-09 Phase 3 MCP relationship edge-type validator opt-in targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（39 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（126 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（126 passed）。
- [x] 2026-07-09 Phase 3 MCP search_code path/file_pattern validator opt-in targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（40 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（127 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（127 passed）。
- [x] 2026-07-09 Phase 3 MCP search_code root/file args combiner opt-in targeted gates：`cargo test -p cbm-core mcp --locked`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp` 與 `make -f Makefile.cbm rust-mcp-codec-optin-test`。
- [x] 2026-07-09 Phase 3 MCP search_code strip root prefix opt-in targeted gates：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（57 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- [x] 2026-07-09 Phase 3 MCP missing project error object opt-in targeted gates：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（58 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- [x] 2026-07-09 Phase 3 MCP project-not-found message opt-in targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（59 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- [x] 2026-07-09 Phase 3 MCP project-list error JSON builder opt-in targeted gates：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（60 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- [x] 2026-07-09 Phase 3 MCP manage_adr sections JSON builder opt-in targeted gates：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（61 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- [x] 2026-07-09 Phase 3 MCP search_code score comparator opt-in targeted gates：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（62 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- [x] 2026-07-09 Phase 3 MCP search_code directory top-key opt-in targeted gates：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（63 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- [x] 2026-07-09 Phase 3 MCP detect_changes status-path parser opt-in targeted gates：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（64 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- [x] 2026-07-09 Phase 3 MCP search_code tightest-node span helper opt-in targeted gates：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（65 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- [x] 2026-07-09 Phase 3 MCP pick_resolved_node best-index helper opt-in targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（66 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- [x] 2026-07-09 Phase 3 MCP find_tightest_node best-index helper opt-in targeted gates：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（67 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- [x] 2026-07-09 Phase 3 MCP utf8_is_cont byte classifier opt-in targeted gates：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（68 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- [x] 2026-07-09 Phase 3 MCP search_code mode classifier opt-in targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（41 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（128 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（128 passed）。
- [x] 2026-07-09 Phase 3 MCP trace_call_path test-file classifier opt-in targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（42 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（129 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（129 passed）。
- [x] 2026-07-09 Phase 3 MCP project DB filename classifier opt-in targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（43 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（129 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（129 passed）。
- [x] 2026-07-06 Phase 3 Cypher hop shorthand parity targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（10 passed）、`cargo test -p cbm-core --locked`（91 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner cypher_contract`（21 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- [x] 2026-07-06 Phase 3 Cypher scalar func opt-in targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（11 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner cypher cypher_contract`（164 passed）、`make -f Makefile.cbm rust-cypher-scalar-func-optin-test`（164 passed，Rust 路徑端到端等價）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- [x] 2026-07-07 Phase 3 Cypher scalar + multiarg opt-in wiring targeted gates：`make -f Makefile.cbm rust-cypher-scalar-func-optin-test`（165 passed）與 `make -f Makefile.cbm rust-cypher-multiarg-func-optin-test`（165 passed），都已重跑並覆蓋 `cypher` + `cypher_contract`。
- [x] 2026-07-08 Phase 3 Cypher aggregate function token opt-in targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（13 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner cypher cypher_contract`（165 passed）、`make -f Makefile.cbm rust-cypher-agg-func-optin-test`（165 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- [x] 2026-07-09 Phase 3 Cypher string function token opt-in targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（14 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner cypher cypher_contract`（165 passed）、`make -f Makefile.cbm rust-cypher-string-func-optin-test`（165 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- [x] 2026-07-06 Phase 4 targeted verification：`build/c/test-runner pipeline graph_buffer lang_contract edge_structural`（332 passed）、`make -f Makefile.cbm rust-pipeline-plan-optin-test`（217 passed）與 `make -f Makefile.cbm rust-language-graph-parity`（golden 一致）。並行跑多個會 `clean-c` 的 Make 目標會互相清掉 `build/c`，因此 registry/plan/language graph 這類共享 build dir gate 需序列執行。
- [x] 2026-07-07 Phase 4 route canonicalization opt-in targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::route --locked`（3 passed）、`cargo test -p cbm-core --locked`（117 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner route_canon pipeline`（228 passed）、`make -f Makefile.cbm rust-pipeline-route-canon-optin-test`（228 passed）與 `make -f Makefile.cbm rust-language-graph-parity`（golden 一致）。
- [x] 2026-07-07 Phase 4 pipeline test detection opt-in targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::test_detect --locked`（4 passed）、`cargo test -p cbm-core --locked`（121 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm rust-pipeline-test-detect-optin-test`（217 passed）與 `make -f Makefile.cbm rust-language-graph-parity`（golden 一致；中斷後已補跑完成）。
- [x] 2026-07-10 Phase 4 pipeline test detection path rewrite smoke：`cbm_rs_pipeline_test_to_prod_path_v1` 已納入 FFI smoke，並在 `CBM_USE_RUST_PIPELINE_TEST_DETECT=1` 下驗證 `test_to_prod_path()` 的 production path rewrite；`rust-pipeline-test-detect-optin-test` 續跑保持 217 passed。
- [x] 2026-07-09 Phase 4 pipeline checked-exception classifier opt-in targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::exception --locked`（1 passed）、`cargo test -p cbm-core --locked`（182 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm rust-pipeline-checked-exception-optin-test`（269 passed）與 `make -f Makefile.cbm rust-language-graph-parity`（golden 一致；曾有一次並行跑共享 `build/c` 的 target 造成 clean-c race，已用序列重跑通過）。
- [x] 2026-07-10 Phase 4 pipeline artifact path opt-in targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::artifact --locked`（4 passed）、`cargo test -p cbm-core --locked`（186 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner artifact`（10 passed）、`make -f Makefile.cbm rust-pipeline-artifact-path-optin-test`（10 passed）與 `git diff --check`。
- [x] 2026-07-10 Phase 4 pipeline project-name opt-in targeted gates：`make -f Makefile.cbm rust-pipeline-project-name-optin-test`（208 passed）；`build/c/test-runner fqn mcp` 於 `CBM_USE_RUST_PIPELINE_PROJECT_NAME=1` 下通過，確認 production opt-in 只改 helper 轉接、不改 C fallback contract。
- [x] 2026-07-10 Phase 4 pipeline configures / trace / progress sink targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::configures --locked`（3 passed）、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm rust-pipeline-configures-optin-test`（217 passed）、`make -f Makefile.cbm rust-traces-optin-test`（30 passed）、`make -f Makefile.cbm rust-cli-progress-sink-optin-test`（2 passed）與 `git diff --check`。三個 target 皆需序列執行，避免共用 `build/c`。
- [x] 2026-07-10 Phase 4 pipeline configures Rust-only gate：`pipeline-configures-only` feature 直接輸出 `cbm_is_env_var_name` / `cbm_normalize_config_key` / `cbm_has_config_extension`；`cargo fmt --all -- --check`、feature-targeted Rust unit（3 passed）、all-features clippy、default FFI smoke、既有 wrapper pipeline（219 passed）及 `rust-pipeline-configures-only-test`（隔離 default/Rust-only pipeline 各 219 passed，含 direct ABI smoke）均通過。`traces-only,pipeline-configures-only` archive 使用獨立 `target/cbm-traces-configures-only` 成功建置。
- [x] 2026-07-11 新增手動 `rust-all-optin-test`：39 個 production Rust opt-in 的首次全組合 prerequisite 分為隔離 wrapper/direct 變體；direct 僅以 `CBM_USE_RUST_TRACES_ONLY=1`、`CBM_USE_RUST_PIPELINE_CONFIGURES_ONLY=1` 取代 wrapper 的兩個旗標，不混用兩組旗標，也不納入 test-only parity。每個變體先清理自己的隔離 `BUILD_DIR` 以避免 stale C objects，再跑 FFI smoke、完整 runner 與 production `cbm --version`；不清理共用 `build/c`，且不接入日常 CI 或 scripts。
- [x] 執行 `make -f Makefile.cbm rust-all-optin-test`：wrapper/direct 完整 runner 各 5861 passed，兩個 production binary 均完成 build 與 `--version` smoke；此 prerequisite 仍不等於 Rust-backed release candidate（RC），也不代表預設 Rust 切換證據。
- [x] 2026-07-12 重跑 `make -f Makefile.cbm rust-all-optin-test`：當時 38 個 common flags 加上 wrapper/direct 各 2 個互斥 flags，完整 runner 各 5865 passed，兩個 production binary 均完成 `--version` smoke；此 prerequisite 仍不等於 Rust-backed RC，也不代表預設 Rust 切換。
- [x] 2026-07-12 新增 `CBM_USE_RUST_CYPHER_LEX_SINGLE_CHAR=1` 後重跑 `make -f Makefile.cbm rust-all-optin-test`：39 個 common flags 的 wrapper/direct 變體完整 runner 各 5866 passed，兩個 production binary 均完成 `--version` smoke；此 prerequisite 仍不等於 Rust-backed RC，也不代表預設 Rust 切換。
- [x] 2026-07-12 新增 `CBM_USE_RUST_LOG_ENV_PARSE=1` 後重跑 `make -f Makefile.cbm rust-all-optin-test`：40 個 common flags 的 wrapper/direct 變體完整 runner 各 5866 passed，兩個 production binary 均完成 `--version` smoke；此 prerequisite 仍不等於 Rust-backed RC，也不代表預設 Rust 切換。
- [x] 2026-07-12 新增 `CBM_USE_RUST_CYPHER_LEX_TWO_CHAR=1` 後重跑 `make -f Makefile.cbm rust-all-optin-test`：41 個 common flags 的 wrapper/direct 變體完整 runner 各 5866 passed，兩個 production binary 均完成 `--version` smoke；此 prerequisite 仍不等於 Rust-backed RC，也不代表預設 Rust 切換。
- [x] 2026-07-12 新增 `CBM_USE_RUST_STORE_LANGUAGE_MAP=1` 後重跑 `make -f Makefile.cbm rust-all-optin-test`：42 個 common flags 的 wrapper/direct 變體完整 runner 各 5867 passed，兩個 production binary 均完成 `--version` smoke；此 prerequisite 仍不等於 Rust-backed RC，也不代表預設 Rust 切換。
- [x] 2026-07-10 Phase 4 pipeline infrascan filename classifier opt-in targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core --locked`（203 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner pipeline`（217 passed）、`make -f Makefile.cbm rust-pipeline-infrascan-optin-test`（217 passed）與 `git diff --check`；修正 opt-in 下 C `to_lower()` unused 的 `-Werror` 編譯問題。
- [x] 2026-07-10 Phase 4 pipeline K8s dependency-manifest classifier targeted gates：`cargo test -p cbm-core pipeline::infrascan --locked`（8 passed）、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner pipeline`（219 passed）、`make -f Makefile.cbm rust-pipeline-k8s-file-classifiers-optin-test`（219 passed）與 `git diff --check`；新增 C end-to-end fixture 固定 Chart.yaml／Chart.yml、go.mod 與 requirements.txt 的 `DEFINES`／`DEPENDS_ON` 結果。
- [x] 2026-07-10 將 `CBM_USE_RUST_PIPELINE_K8S_FILE_CLASSIFIERS=1` 納入 language graph parity 候選後，重跑 `make -f Makefile.cbm rust-language-graph-parity`；default 與 Rust pipeline binary 均與既有 golden 一致。
- [x] 2026-07-10 Phase 4 pipeline githistory trackable-file classifier opt-in targeted gates：`cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::githistory --locked`（4 passed）、`cargo test -p cbm-core --locked`（207 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner pipeline`（217 passed）、`make -f Makefile.cbm rust-pipeline-githistory-optin-test`（217 passed）與 `git diff --check`；修正 opt-in 下 C `ends_with()` unused 的 `-Werror` 編譯問題。
- [x] 2026-07-10 Phase 4 pipeline gitdiff range parser：`pipeline::gitdiff::parse_range()`、`cbm_rs_pipeline_parse_range_v1` 與 `CBM_USE_RUST_PIPELINE_GITDIFF_RANGE=1` 已完成；僅遷移 unified diff range scalar parser，保留 C 的 hunk/name-status 結構、I/O 與 side effects。通過 targeted Rust 4 passed、完整 Rust 211 passed、clippy、FFI smoke、default/opt-in pipeline 各 217 passed、fmt、shell syntax 與 diff check。
- [x] 2026-07-10 Phase 4 pipeline gitdiff name-status parser：`pipeline::gitdiff::parse_name_status()`、`cbm_rs_pipeline_parse_name_status_v1` 與 `CBM_USE_RUST_PIPELINE_GITDIFF_NAME_STATUS=1` 已完成；僅遷移 name-status 行解析、rename path 與 trackable filter，保留 C 的 hunk parser、I/O 與 side effects。通過 targeted Rust 6 passed、完整 Rust 213 passed、clippy、FFI smoke、default/opt-in pipeline 各 217 passed、fmt、shell syntax 與 diff check。
- [x] 2026-07-10 Phase 4 pipeline gitdiff hunk parser：`pipeline::gitdiff::parse_hunks()`、`cbm_rs_pipeline_parse_hunks_v1` 與 `CBM_USE_RUST_PIPELINE_GITDIFF_HUNKS=1` 已完成；僅遷移 current-file tracking、hunk range parser 與 trackable filter，保留 C 的 I/O、後續消費與 side effects。通過 targeted Rust 8 passed、完整 Rust 215 passed、clippy、FFI smoke、default/opt-in pipeline 各 217 passed、fmt、shell syntax 與 diff check。
- [x] 2026-07-10 Phase 4 pipeline module-directory classifier：`pipeline::module::is_module_dir()`、`cbm_rs_pipeline_is_module_dir_v1` 與 `CBM_USE_RUST_PIPELINE_MODULE_DIR=1` 已完成；五個 pipeline pass 僅遷移 Go/Java module-directory 判定，保留 module QN 組裝、解析與 side effects。通過 targeted 1 passed、完整 Rust 216 passed、clippy、FFI smoke、default/opt-in pipeline 各 217 passed、fmt、shell syntax 與 diff check。
- [x] 2026-07-10 Phase 4 pipeline route argument helpers：Rust helpers、FFI smoke、C opt-in 與 default/opt-in pipeline gates 已完成；通過 targeted route 6 passed、完整 Rust 219 passed、clippy、FFI smoke、default/opt-in pipeline 各 217 passed、fmt、shell syntax 與 diff check。
- [x] 2026-07-10 Phase 4 pipeline route-node classifiers：Rust helpers、FFI smoke、C opt-in 與 default/opt-in pipeline gates 已完成；通過 targeted route 7 passed、完整 Rust 220 passed、clippy、FFI smoke、default/opt-in pipeline 各 217 passed、fmt、shell syntax 與 diff check。同期完成 `CBM_USE_RUST_PIPELINE_ROUTE_NODE_CLASSIFIERS_ONLY=1` direct-only gate 接入與 `pipeline-route-node-classifiers-only` 變體建置。
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
- [x] 2026-07-06 language graph parity JavaScript 擴充：`make -f Makefile.cbm rust-language-graph-parity-update` 重生 golden（default 與 Rust pipeline opt-in graph 精確一致，內含 JS `JsWorker`/`jsHelper`/`jsStart` 與 `/tasks/:id` route），`python3 scripts/rust-language-graph-parity.py target/rust-parity/cbm-default target/rust-parity/cbm-rust-pipeline` 非 update 決定性驗證通過；`python3 -m py_compile scripts/rust-language-graph-parity.py` 與 `git diff --check` 通過。

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

## 本次工作階段（2026-07-07 Phase 3 Store dump side-effect golden）

- [x] 擴充 `tests/test_store_compat.c`，新增 `store_compat_dump_to_file_side_effect_contract`，固定 `cbm_store_dump_to_file()` dump 後資料可透過 readonly query open 讀取、query open 拒寫、目標 DB `PRAGMA journal_mode` 維持 `wal`，且中途 `.tmp` 檔不殘留。
- [x] 擴充 `tests/test_store_compat.c`，新增 `store_compat_delete_nodes_by_file_cascades_edges`，固定刪除 file-scoped nodes 會 cascade 清除 inbound/outbound edges，並保留其他 file 的 node。
- [x] 此批只補 Store side-effect golden，不修改 `src/store/store.c`、Rust store module、SQLite open/probe/pragma、CRUD/search/BM25、artifact 或 writer runtime。

## 本次工作階段（2026-07-07 Phase 3 Store checkpoint/bulk filtered manifest）

- [x] 擴充 `rust/cbm-core/src/store/schema_manifest.rs` connection manifest，新增 `CONNECTION_MODE_CHECKPOINT` 與 public checkpoint entries：`checkpoint.passive.api` 固定 `SQLITE_CHECKPOINT_PASSIVE` 且非 best-effort、`checkpoint.optimize` 固定 `PRAGMA optimize;`。
- [x] 新增 `cbm_rs_store_connection_manifest_entry_count_for_mode_v1`、`cbm_rs_store_connection_manifest_write_entry_count_for_mode_v1` 與 `cbm_rs_store_connection_manifest_entries_for_mode_v1`，用同一個 static manifest 依 memory/read-only/bulk/checkpoint mode 過濾計畫；不開 SQLite、不執行 pragma、不接 production opt-in。
- [x] 擴充 `tests/test_rust_ffi.c`，驗證 read-only filtered plan 不含 write pragma、bulk begin/end count、checkpoint filtered 順序、null/short/negative cap 與 unknown mode empty contract。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core store::schema_manifest --locked`（5 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm build/c/test-runner && build/c/test-runner store_compat`（15 passed）。

## 本次工作階段（2026-07-07 Phase 4 FullPipeline top-level runtime context）

- [x] 修正 `src/pipeline/rust_plan.h` 的 FullPipeline FFI 宣告與 inline wrapper，改為傳入 Rust export 實際需要的 `mode`、`worker_count` 與 `file_count`。
- [x] 將 `pipeline.c` 的 top plan 載入移到 discovery 成功後，並讓 top plan decision 與實際 `run_extraction_phase()` 使用同一個 `worker_count/file_count`。
- [x] 加強 `pipeline.c` 的 FullPipeline top_v1 decode contract，逐欄驗證 `policy`、`gate_flags`、`effect_flags` 與 `nested_plan_kind`，避免只靠順序與 `requires_mask` 接受錯誤 metadata。
- [x] 擴充 `tests/test_pipeline.c` Rust plan trace contract，固定 sequential/parallel full run 皆先出現 `full_pipeline source=top_v1` 與 `extraction_choice source=top_v1`，再進入 typed v2 extraction dispatch。
- [x] 加強 `tests/test_graph_buffer.c` static guard，確認 `pipeline.c` 透過 wrapper 消費 FullPipeline metadata，且 `pipeline_incremental.c` 不直接消費 FullPipeline API。
- [x] 同步更新 `rust/CBM_FFI.md`、`rust/cbm-core/src/ffi.rs` 註解與 `docs/rust-refactor-baseline.md`，明確標示 `pipeline.c` opt-in 消費 top_v1 metadata，但實際 side effects 仍由 C 執行。

## 本次工作階段（2026-07-07 Phase 4 route canonicalization opt-in）

- [x] 新增 `rust/cbm-core/src/pipeline/route.rs`，以 raw byte helper 對齊 `cbm_route_canon_path()`：`:`、`{}`、`<>`、`${}` placeholder collapse 成 `{}`，mid-segment colon 保持 literal，unclosed brace/angle/template 依既有 C contract collapse。
- [x] 新增 `cbm_rs_pipeline_route_canon_path_v1` caller-buffer FFI，固定完整長度回傳、短 buffer NUL 截斷、NULL input 回空字串與 non-UTF-8 byte 保留 contract。
- [x] 新增 `CBM_USE_RUST_PIPELINE_ROUTE_CANON=1`，讓 `src/pipeline/pass_route_nodes.c` 的 `cbm_route_canon_path()` 在 opt-in 下委派 Rust；route extraction、HTTP/async call detection、Route node/edge 建立、graph-buffer mutation、language parser 與 pipeline side effects 仍留 C。
- [x] 將 `rust-pipeline-route-canon-optin-test` 接入 `.PHONY`、`rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7；`rust-language-graph-parity` 的 Rust pipeline binary 也同時啟用此 opt-in。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::route --locked`（3 passed）、`cargo test -p cbm-core --locked`（117 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner route_canon pipeline`（228 passed）、`make -f Makefile.cbm rust-pipeline-route-canon-optin-test`（228 passed）與 `make -f Makefile.cbm rust-language-graph-parity`（golden 一致）。

## 本次工作階段（2026-07-07 Phase 4 pipeline test detection opt-in）

- [x] 新增 `rust/cbm-core/src/pipeline/test_detect.rs`，以 raw byte helper 對齊 `src/pipeline/pass_tests.c` 的 `cbm_is_test_path()` 與 `cbm_is_test_func_name()` contract，包含 `test_` basename、語言特定 suffix、`.test/.spec` substring、Go `Test/Benchmark/Example` 大寫規則與 fixture helper 名稱。
- [x] 新增 `cbm_rs_pipeline_is_test_path_v1` / `cbm_rs_pipeline_is_test_func_name_v1` FFI smoke，覆蓋 NULL、empty、Windows backslash 不視為 separator、`contest` / `mytests` false positive、防止 `Testable` / `Benchmarkable` / `Examplefoo` 誤判等 edge cases。
- [x] 新增 `cbm_rs_pipeline_test_to_prod_path_v1` FFI smoke，覆蓋 `foo_test.go`、`test_handler.py`、`.test.`、`.spec.`、short buffer 與 non-match path rewrite contract。
- [x] 新增 `CBM_USE_RUST_PIPELINE_TEST_DETECT=1`，讓 `pass_tests.c` 的純 classifier 與 `test_to_prod_path()` 的 path rewrite 在 opt-in 下委派 Rust；`TESTS` / `TESTS_FILE` edge creation、`node_is_test()`、graph-buffer adapter、tree-sitter/extraction 與 pipeline side effects 仍留 C。
- [x] 將 `rust-pipeline-test-detect-optin-test` 接入 `.PHONY`、`rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7；`rust-language-graph-parity` 的 Rust pipeline binary 也啟用此 opt-in。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::test_detect --locked`（3 passed）、`cargo test -p cbm-core --locked`（121 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm rust-pipeline-test-detect-optin-test`（217 passed）與 `make -f Makefile.cbm rust-language-graph-parity`（golden 一致；中斷後已補跑完成）。

## 本次工作階段（2026-07-06 Phase 4 FullPipeline top-level metadata FFI）

- [x] 擴充 `rust/cbm-core/src/pipeline/plan.rs`，新增 FullPipeline top-level metadata helper，固定 macro extraction、user config、discover、incremental-or-delete-db、structure、extraction dispatch、tests、githistory、predump、dump、persist hashes 與 artifact export 的外層順序。
- [x] 新增 `CbmRsPipelineTopStepV1` 與 `cbm_rs_pipeline_full_plan_step_count_v1` / `cbm_rs_pipeline_full_plan_steps_v1`，以 C static assert 鎖定 ABI layout、full/fast step count、gate/effect flags、`requires_mask`、nested plan kind 與 null/short/negative cap contract。
- [x] 保留 `cbm_rs_pipeline_plan_steps_v2(kind=FullPipeline)` unsupported；後續已將 top-level metadata API 接入 `pipeline.c` runtime，並以 static guard 確認 `pipeline_incremental.c` 不直接消費 FullPipeline metadata。
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

## 本次工作階段（2026-07-07 Phase 3 MCP tools/call arguments codec opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `tools_call_arguments(params_json)`，以 byte-level parser 擷取 params object 的首個 `arguments` JSON value；缺少 `arguments` 時回傳 `{}`，存在時保留原始 JSON slice。
- [x] 新增 `cbm_rs_mcp_tools_call_arguments_v1`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `cbm_mcp_get_arguments()` 委派 Rust；C wrapper 仍負責配置/釋放 C-owned NUL 字串。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 missing args、object/array/string value、duplicate first-wins、raw spacing、length-only、短 buffer、invalid JSON/root 與 invalid UTF-8。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 handler-specific 參數解析、validation、tool schema、response formatting、Content-Length transport 與 handlers 仍留 C/yyjson。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（10 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（124 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（124 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP string argument extraction opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `string_arg(args_json, key)`，以 byte-level parser 擷取 args root object 的首個 matching string field；missing、non-string、invalid JSON、invalid UTF-8 與空 key 回傳 `None`。
- [x] 新增 `cbm_rs_mcp_get_string_arg_v1`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `cbm_mcp_get_string_arg()` 先由 Rust 擷取 string arg；成功時配置 C-owned NUL 字串，失敗時 fallback 到原 yyjson helper。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 label/name_pattern、length-only、短 buffer、Unicode escape、duplicate first-wins、missing/non-string、null/empty key、trailing data 與 invalid UTF-8。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 int/bool arg helpers、project alias resolution、handler-specific validation、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（37 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（125 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP int/bool argument extraction opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `int_arg(args_json, key, default_value)` 與 `bool_arg(args_json, key)`，共用 root field parser，固定 first matching key、型別不符回 default/false 與 invalid JSON/UTF-8 handling。
- [x] 新增 `cbm_rs_mcp_get_int_arg_v1` / `cbm_rs_mcp_get_bool_arg_v1`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `cbm_mcp_get_int_arg()` / `cbm_mcp_get_bool_arg()` 委派 Rust；預設 C path 保持不變。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 limit/offset、include_connected/regex、missing、string/bool/int 型別不符、duplicate first-wins、null/empty key、trailing data 與 invalid UTF-8。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 project alias resolution、handler-specific range/default validation、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（38 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（125 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP relationship edge-type validator opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `edge_type_valid(input)`，固定 `search_graph.relationship` edge type contract：只允許 ASCII uppercase `A-Z` 與 `_`，長度最多 64 bytes；空字串依既有 C helper 視為有效。
- [x] 新增 `cbm_rs_mcp_edge_type_valid_v1`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `validate_edge_type()` 委派 Rust；預設 C path 保持不變。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、空字串、合法 uppercase/underscore、lowercase、dash、digit、space、非 ASCII、64 bytes 與 65 bytes。
- [x] 新增 `tests/test_mcp.c` end-to-end regression：以既有 snippet fixture 與 `project:"test-project"` 確保 request 通過 project/store validation 後，invalid `relationship:"calls"` 回傳 `relationship must be uppercase letters and underscores` 與 `isError:true`。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 `search_graph` args extraction、project/index validation、store search/query planning、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（39 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（126 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（126 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP search_code path/file_pattern validator opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `search_path_arg_valid(input)`，固定 `search_code` root path / `file_pattern` shell quoting denylist：null 失敗；空字串、space 與 `&` 合法；quote、`;`、`|`、`$`、backtick、`<`、`>`、LF、CR 失敗；非 Windows 另拒絕 backslash。
- [x] 新增 `cbm_rs_mcp_search_path_arg_valid_v1`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `validate_search_path_arg()` 委派 Rust；預設 C path 保持不變。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、空字串、一般 path、`*R&D*.go`、space path、所有 denylist byte 與平台差異 backslash。
- [x] 新增 `tests/test_mcp.c` end-to-end regression：以既有 snippet fixture 與 `project:"test-project"`，固定非法 `file_pattern:"*.go;rm -rf /"` 回傳 `path or file_pattern contains invalid characters` 與 `isError:true`。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 project root resolution、grep command construction、temp file、regex/search execution、result parsing、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（40 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（127 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（127 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP search_code root/file args combiner opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `search_args_valid(root_path, file_pattern)`，固定 `validate_search_args()` contract：`root_path` 必填且需通過 path validator，`file_pattern` 可省略但若提供也需通過同一 denylist。
- [x] 新增 `cbm_rs_mcp_search_args_valid_v1`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `validate_search_args()` 委派 Rust；預設 C path 保持不變。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null root、空字串 root、root only、合法 pattern、`&`、非法 root、非法 pattern 與含換行 pattern。
- [x] 更新 `rust/CBM_FFI.md`，明確標示此切片只遷移 pure combiner，grep command construction、search execution、result shaping、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 驗證通過：`cargo test -p cbm-core mcp --locked`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp` 與 `make -f Makefile.cbm rust-mcp-codec-optin-test`。

## 本次工作階段（2026-07-09 Phase 3 MCP search_code strip root prefix opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `search_strip_root_prefix_offset(path, root, root_len)`，固定 `strip_root_prefix()` byte-level contract：prefix match 後可選擇性跳過一個 `/`，不檢查 path component boundary。
- [x] 新增 `cbm_rs_mcp_strip_root_prefix_offset_v1`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `strip_root_prefix()` 用 Rust offset 回到原 `path` borrowed pointer；預設 C path 保持不變。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null path/root、一般 strip、exact root、非 component boundary、root trailing slash、no match、`root_len` 過長與 `root_len == 0`。
- [x] 更新 `rust/CBM_FFI.md`、`Rust-Refactor.md` 與 `Handoff.md`，明確標示 grep execution、path filter、result parsing、JSON/transport、server lifecycle、handlers 與 path ownership 仍留 C。
- [x] 驗證通過：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（57 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP search_code mode classifier opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `search_mode(input)`，固定 `search_code.mode` classifier：`full` 回 1、`files` 回 2，null、空字串、`compact`、未知值、大小寫不符與尾端空白皆回 0。
- [x] 新增 `cbm_rs_mcp_search_mode_v1`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `parse_search_mode()` 委派 Rust；預設 C path 保持不變。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、空字串、compact、full、files、大小寫不符、尾端空白與未知值。
- [x] 新增 `tests/test_mcp.c` end-to-end regression：以既有 snippet fixture 與 `mode:"files"`，固定回應含 `files` 且不含 `results` / `isError:true`。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 handler args extraction、grep command construction、context/source/file-list result shaping、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（41 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（128 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（128 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP trace_call_path test-file classifier opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `trace_is_test_file(input)`，固定 MCP-local `trace_call_path` test-file classifier：`/test`、`test_`、`_test.`、`/tests/`、`/spec/`、`.test.` substring 為 true；null 與空字串為 false。
- [x] 新增 `cbm_rs_mcp_trace_is_test_file_v1`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `is_test_file()` 委派 Rust；預設 C path 保持不變，且不重用語意不同的 pipeline/store test detection。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、空字串、`/testdata`、`test_` prefix、`_test.` suffix、`/tests/`、`/spec/`、`.test.`、非匹配 `contest` / `mytests` / `.spec.` 與 Windows backslash。
- [x] 新增 `tests/test_mcp.c` end-to-end regression：以 `entry -> src/tests/test_helper.c` CALLS graph 固定預設濾掉測試 helper，`include_tests:true` 會顯示 helper 並輸出 `is_test:true`。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 BFS traversal、`include_tests` filtering、`is_test` JSON 標記、risk labels、data-flow args extraction、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（42 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（129 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（129 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP project DB filename classifier opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `project_db_file_name(input)`，固定 MCP cache scan project DB filename classifier：長度至少 `x.db`、精準 `.db` suffix、排除 `_` 開頭與 `:memory:` 開頭；`tmp-*.db` 為合法。
- [x] 新增 `cbm_rs_mcp_project_db_file_name_v1`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `is_project_db_file()` 委派 Rust；預設 C path 保持不變。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、空字串、`.db`、`x.db`、一般 `.db`、`tmp-*.db`、`.db-wal`、非 `.db`、`_internal.db`、`:memory:` 與 `:memory:.db`。
- [x] 擴充 `tests/test_mcp.c` #704 end-to-end fixture：新增 `_hidden704.db` 與 `tmp-bench704.db`，固定 `list_projects` 不列 hidden/internal DB，且仍列 tmp-prefixed project。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 directory scanning、SQLite query-open、internal project-name resolution、ghost/corrupt DB filtering、JSON list building、resolve fallback、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（43 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（129 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（129 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP index_repository mode classifier opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `index_mode(input)`，固定 `index_repository.mode` classifier：`moderate` 回 1、`fast` 回 2、`cross-repo-intelligence` 回 3；null、空字串、`full`、未知值、大小寫不符與尾端空白皆回 0。
- [x] 新增 `cbm_rs_mcp_index_mode_v1`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `parse_index_repository_mode()` 委派 Rust；預設 C path 保持不變，cross-repo sentinel 不傳入 pipeline mode enum。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、空字串、full、moderate、fast、cross-repo、大小寫不符、尾端空白與未知值。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 repo_path/name/persistence args extraction、cross-repo matching、pipeline creation/run、artifact/degraded response shaping、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（44 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（129 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（129 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP trace_path mode edge defaults opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `trace_mode_edge_mask(input)`，固定 `trace_path.mode` default edge-type classifier：default/calls/未知回 `CALLS`，`data_flow` 回 `CALLS|DATA_FLOWS`，`cross_service` 回 HTTP/async/data/calls/CROSS_* bitmask。
- [x] 新增 `cbm_rs_mcp_trace_mode_edge_mask_v1`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `resolve_trace_edge_types()` 無 explicit `edge_types` array 時委派 Rust；預設 C path 保持不變。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、空字串、calls、data_flow、cross_service、大小寫不符、尾端空白與未知值。
- [x] 新增 `tests/test_mcp.c` end-to-end regression：只有 `DATA_FLOWS` edge 時，預設 mode 不追到 sink，`mode:"data_flow"` 會追到。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 explicit edge array parsing/ownership、BFS traversal、data-flow arg extraction、risk labels、test filtering、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（45 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（130 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（130 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP search_code sanitize_ascii opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `sanitize_ascii_in_place(bytes)`，固定 ASCII 與 `0x7f` 保留、所有 `>127` byte 就地改成 `?`、empty slice no-op 的 contract。
- [x] 新增 `cbm_rs_mcp_sanitize_ascii_in_place_v1`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `sanitize_ascii()` 委派 Rust；預設 C path 保持不變。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、ASCII、`0x7f`、UTF-8 high bytes、`0xff` 與空字串。
- [x] 新增 `tests/test_mcp.c` end-to-end regression：含 UTF-8 `é` bytes 的 source snippet 在 `search_code` full mode 輸出 `caf??`。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 grep execution、source/context reading、JSON building、snippet UTF-8 lossy sanitizer、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（46 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-09 Phase 3 MCP search_code ranking scorer opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `search_code_score(label, file, in_degree)`，固定 `search_code` deduped result ranking：Function/Method 加 10、Route 加 15、vendored/vendor/node_modules 扣 50、test/spec/`_test.` path 扣 5。
- [x] 新增 `cbm_rs_mcp_search_code_score_v1`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `compute_search_score()` 委派 Rust；預設 C path 保持不變。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、Function/Method、Route、class-like label、大小寫精準、vendored/vendor/node_modules、test/spec/`_test.` 與 vendor+test 疊加。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 grep execution、graph node lookup、dedup/classification、sort comparator、result JSON、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（52 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP search_code score comparator opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `search_score_cmp(left_score, right_score)`，固定 `search_result_cmp()` 的 descending score delta contract。
- [x] 新增 `cbm_rs_mcp_search_score_cmp_v1` FFI，輸入兩個 score scalar，回傳 `right_score - left_score` 的 qsort comparator 值。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `search_result_cmp()` 委派 Rust 計算 comparator delta；預設 C path 保持原 `rb->score - ra->score`。
- [x] 保留 qsort 呼叫、`search_result_t` ownership、grep execution、graph node lookup、dedup/classification、score 寫入、result JSON、response wrapping、Content-Length transport、server lifecycle 與 handlers 在 C。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 left/right higher、equal 與負分數。
- [x] 更新 `rust/CBM_FFI.md`、`Rust-Refactor.md` 與 `Handoff.md`，明確標示此切片只遷移 score comparator scalar helper。
- [x] 驗證通過：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（62 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP search_code directory top-key opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `search_top_dir(file)`，固定 `build_dir_distribution()` 的 top-level directory key 擷取：有 `/` 時保留到第一個 slash（含 slash），否則使用整個 file path。
- [x] 新增 `cbm_rs_mcp_search_top_dir_v1` FFI，支援 length-only 與短 buffer NUL 截斷；`file == NULL` 回 `SIZE_MAX`。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` 新增 `search_result_top_dir()` wrapper，讓 `build_dir_distribution()` 委派 Rust 擷取 top key；ABI 異常時 fallback 到原 C `strchr`/copy 邏輯。
- [x] 保留 directory count 累計、`yyjson` object building、`search_result_t` ownership、grep execution、graph node lookup、dedup/classification、result JSON、response wrapping、Content-Length transport、server lifecycle 與 handlers 在 C。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、empty、nested path、single filename、absolute path 與短 buffer。
- [x] 更新 `rust/CBM_FFI.md`、`Rust-Refactor.md` 與 `Handoff.md`，明確標示此切片只遷移 directory top-key pure helper。
- [x] 驗證通過：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（63 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP detect_changes scope classifier opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `detect_changes_wants_symbols(scope)`，固定 `detect_changes.scope` classifier：null/default、`symbols` 與 `impact` 輸出 impacted symbols；空字串、`files`、大小寫不符、尾端空白與未知值只輸出 changed files。
- [x] 新增 `cbm_rs_mcp_detect_changes_wants_symbols_v1`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `detect_changes_wants_symbols()` 委派 Rust；預設 C path 保持不變。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、symbols、impact、empty、files、大小寫、尾端空白與未知值。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 project/root resolution、base branch validation、git diff/status command、changed file parsing、Store symbol lookup、JSON result building、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（53 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP detect_changes impacted label classifier opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `detect_changes_impacted_label(label)`，固定 impacted symbols label filter：null、`File`、`Folder` 與 `Project` 排除；其餘 label（含空字串、大小寫不符與尾端空白）皆保留。
- [x] 新增 `cbm_rs_mcp_detect_changes_impacted_label_v1`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `detect_changes_impacted_label()` 委派 Rust；預設 C path 保持不變。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、File、Folder、Project、Function、Method、Route、empty、大小寫不符與尾端空白。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 Store node query、node ownership/free、changed file parsing、impacted JSON item building、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（54 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP detect_changes status-path parser opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `detect_changes_status_path_offset(line)`，固定 changed-files parser contract：plain path 回 offset 0；`git status --porcelain` 前綴 `XY ` 會被略過；rename `old -> new` 取 destination path offset；空路徑/null 回 `SIZE_MAX`。
- [x] 新增 `cbm_rs_mcp_detect_changes_status_path_offset_v1` FFI，輸入 NUL-terminated line 並回傳 path 起始 offset sentinel；不回傳 Rust-owned 字串。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `detect_changes` changed-files 迴圈先委派 Rust 計算 path offset；Rust ABI 異常時 fallback 到原 C parser，預設 C path 保持不變。
- [x] 保留 git command execution、project/root resolution、Store symbol lookup、JSON result building、response wrapping、Content-Length transport、server lifecycle 與 handlers 在 C。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、empty、plain path、untracked/added/modified porcelain 前綴、rename destination、rename empty destination 與未知前綴。
- [x] 更新 `rust/CBM_FFI.md`、`Rust-Refactor.md` 與 `Handoff.md`，明確標示此切片只遷移 detect_changes changed-files path parser。
- [x] 驗證通過：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（64 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-09 Phase 3 MCP search_code tightest-node span helper opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `search_line_match_span(start_line, end_line, line)`，固定 `find_tightest_node()` 單一 node span contract：命中區間回 `end_line - start_line`，未命中回 `-1`。
- [x] 新增 `cbm_rs_mcp_search_line_match_span_v1` FFI，輸入 `start_line/end_line/line` 並回傳 span sentinel，不涉及 Rust-owned memory 傳遞。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `find_tightest_node()` 的 per-node span 判斷先委派 Rust；Rust 回 `-1` 時 fallback 到原 C 判斷，預設 C path 保持不變。
- [x] 保留 node query、grep hit merge、result ranking、JSON result building、response wrapping、Content-Length transport、server lifecycle 與 handlers 在 C。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 hit/start/end boundary、before/after miss 與 invalid range。
- [x] 更新 `rust/CBM_FFI.md`、`Rust-Refactor.md` 與 `Handoff.md`，明確標示此切片只遷移 tightest-node span pure helper。
- [x] 驗證通過：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（65 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP pick_resolved_node best-index helper opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `search_pick_resolved_index(scores)`，固定 `pick_resolved_node()` tie-break contract：空輸入回 `-1,false`；回傳第一個 top score index；top score 多於一個時 `ambiguous=true`。
- [x] 新增 `cbm_rs_mcp_search_pick_resolved_index_v1` FFI，輸入 `long` score 陣列與 `count`，輸出 best index 與 `ambiguous_out`；invalid args（null out、null scores、count<=0）回 `-1` 並清 `ambiguous_out`。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `pick_resolved_node()` 會先委派 Rust 計算 best-index/ambiguous；ABI 異常時 fallback 到原 C 兩段式比較與 top_count 判斷，預設 C path 保持不變。
- [x] 保留 score 計算、BFS traversal、response wrapping、Content-Length transport、server lifecycle 與 handlers 在 C。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 empty/unique/tie/negative 分數與 invalid args。
- [x] 更新 `rust/CBM_FFI.md`、`Rust-Refactor.md` 與 `Handoff.md`，明確標示此切片只遷移 pure tie-break helper。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（66 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP find_tightest_node best-index helper opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `search_pick_tightest_index(spans)`，固定 `find_tightest_node()` best-index contract：最小且非負 span 的第一個 index；全 miss 或 invalid args 回 `-1`。
- [x] 新增 `cbm_rs_mcp_search_pick_tightest_index_v1` FFI，輸入 `int` span 陣列與 `count`，回傳 best index；`count<=0` 或 null spans 回 `-1`。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `find_tightest_node()` 會先委派 Rust 做 best-index 選擇；ABI 異常時 fallback 到原 C 逐筆比較邏輯，預設 C path 保持不變。
- [x] 保留 node query、hit merge、result ranking、JSON result building、response wrapping、Content-Length transport、server lifecycle 與 handlers 在 C。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 empty/all-miss/unique/tie-first/mixed spans 與 invalid args。
- [x] 更新 `rust/CBM_FFI.md`、`Rust-Refactor.md` 與 `Handoff.md`，明確標示此切片只遷移 pure best-index helper。
- [x] 驗證通過：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（67 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP utf8_is_cont byte classifier opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `utf8_is_cont_byte(byte)`，固定 continuation-byte classifier：低 8-bit 符合 `10xxxxxx` 回 true，否則 false。
- [x] 新增 `cbm_rs_mcp_utf8_is_cont_v1` FFI，輸入 `int` byte 並回傳 1/0；高位輸入會先 mask 到 low 8-bit。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `utf8_is_cont()` 會先委派 Rust；`sanitize_utf8_lossy()` 仍優先走 Rust serializer，ABI 失敗時 fallback 到 C loop，loop 內 continuation 判斷可用此 helper。
- [x] 保留 source/snippet 讀取、JSON result building、response wrapping、Content-Length transport、server lifecycle 與 handlers 在 C。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 `0x80`、`0xBF`、`0x7F`、`0xC0` 與高位輸入 byte mask 行為。
- [x] 更新 `rust/CBM_FFI.md`、`Rust-Refactor.md` 與 `Handoff.md`，明確標示此切片只遷移 pure byte classifier。
- [x] 驗證通過：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（68 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP node resolution score opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `node_resolution_score(label, start_line, end_line)`，固定 `Function`/`Method` 最高、其他非 `Module`/`File` label 居中、`Module`/`File`/null 最低，同 tier 加非負 line span。
- [x] 新增 `cbm_rs_mcp_node_resolution_score_v1`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `node_resolution_score()` 委派 Rust；預設 C path 保持不變。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null/empty label、Function/Method、class-like label、Module/File、負 span 與大小寫精準比對。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 `pick_resolved_node()` tie/ambiguity、candidate query、BFS/snippet response shaping、node ownership、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（47 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP manage_adr mode classifier opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `adr_mode(input)`，固定 `manage_adr.mode` classifier：get/default 回 0，update/store 回 1，sections 回 2；null、空字串、未知、大小寫不符與尾端空白皆回 get/default。
- [x] 新增 `cbm_rs_mcp_adr_mode_v1`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `parse_adr_mode()` 委派 Rust；預設 C path 保持不變。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、空字串、get、update、store、sections、大小寫不符、尾端空白與未知值。
- [x] 擴充 `tests/test_mcp.c` ADR backend regression：以 `mode:"store"` 寫入，並以 `mode:"sections"` 驗證 `## PURPOSE` / `## STACK` round-trip。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 project/store resolution、legacy file migration、ADR read/write、sections JSON building、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（48 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP manage_adr sections JSON builder opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `adr_sections_json(content)`，固定 `manage_adr(mode=sections)` 的 Markdown header 擷取與 JSON array 序列化。
- [x] 新增 `cbm_rs_mcp_adr_sections_json_v1` FFI，支援 length-only 與短 buffer NUL 截斷；`content == NULL` 對齊 C helper 輸出空 array。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `adr_list_sections_from_content()` 會用 Rust 產生 sections JSON array，再用 `yyjson_mut_rawncpy()` 複製進 yyjson document；ABI 異常、配置失敗或 raw JSON 掛載失敗時 fallback 到原 C loop。
- [x] 契約固定：逐行掃描 `content`，移除行尾 `\r`，只接受第一個 byte 為 `#` 且非空的行，單一 header 最多保留 1023 bytes，字串 escape 由 Rust JSON serializer 處理。
- [x] 保留 project/store resolution、legacy file migration、ADR read/write、`cbm_mcp_text_result()` response wrapping、JSON-RPC wrapper、request logging、Content-Length transport、server lifecycle 與 handlers 在 C。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null/empty、CRLF、非 header 行、escape、invalid UTF-8 lossy 與短 buffer。
- [x] 更新 `rust/CBM_FFI.md`、`Rust-Refactor.md` 與 `Handoff.md`，明確標示此切片只遷移 `manage_adr(mode=sections)` sections JSON array builder。
- [x] 驗證通過：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（61 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP BM25 MATCH builder opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `bm25_match_query(input, out_size)`，固定 ASCII alnum/underscore token、` OR ` 串接、非 token byte 分隔與短 buffer 停在上一個完整 token 的 contract。
- [x] 新增 `cbm_rs_mcp_bm25_build_match_v1`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `bm25_build_match()` 委派 Rust；預設 C path 保持不變。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null buffer/null query、empty/punctuation、hyphen/dot/non-ASCII 分隔、多 token、短 buffer 與過小 buffer 不寫入。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 SQLite/FTS5 query execution、BM25 ranking/boosting、file pattern LIKE、result JSON、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（49 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP get_architecture aspects gate opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `architecture_aspect_wanted(input, name)`，固定缺少 aspects、invalid JSON（含尾端殘留）、root 非 object、aspects 非 array 時視為 wanted；空 array 或無匹配為 false；"all" 或 exact name match 為 true。
- [x] 新增 `cbm_rs_mcp_architecture_aspect_wanted_v1`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `aspect_wanted()` 委派 Rust；預設 C path 保持不變。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、empty、no-aspects、not-array、empty-array、"all" 匹配、exact aspect match、no match、非字串元素忽略與尾端殘留 JSON invalid fallback。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 yyjson parsing/serialization、Store links/traversal、response/transport/server lifecycle 與 handlers 仍留 C.
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（56 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP sanitize_utf8_lossy opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `sanitize_utf8_lossy(input)`，固定對非 UTF-8 byte 進行 lossy 消毒，將其替換為 REPLACEMENT CHARACTER U+FFFD。
- [x] 新增 `cbm_rs_mcp_sanitize_utf8_lossy_v1`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `sanitize_utf8_lossy()` 委派 Rust；預設 C path 保持不變。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、empty、length-only、ASCII plain text、invalid bytes 與短 buffer 截斷。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 yyjson building/serialization、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（52 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP BM25 file_pattern LIKE builder opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `bm25_file_pattern_like(input)`，固定先 glob-to-LIKE，再對無 `*` / `?` pattern 加前後 `%` 的 contains match contract。
- [x] 新增 `cbm_rs_mcp_bm25_file_pattern_like_v1`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `bm25_file_pattern_like()` 委派 Rust；預設 C path 保持不變。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、empty、length-only、plain path contains wrapping、`*`、`?`、bracket literal 與短 buffer 截斷。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 SQLite/FTS5 query execution、LIKE binding/query planning、BM25 ranking/boosting、result JSON、response/transport/server lifecycle 與 handlers 仍留 C。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（50 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-07 Phase 3 MCP initialize protocol version codec opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `initialize_protocol_version(params_json)`，固定 supported protocol versions newest-first、exact match、duplicate first-wins 與 fallback latest contract。
- [x] 新增 `cbm_rs_mcp_initialize_protocol_version_v1` FFI，沿用 caller-provided buffer、完整長度回傳、短 buffer NUL 截斷與 null/invalid fallback latest contract。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `cbm_mcp_initialize_response()` 只把 `params.protocolVersion` selection 委派 Rust；response JSON 組裝、`serverInfo`、capabilities、yyjson serialization/ownership、dispatch 與 Content-Length transport 仍留 C。
- [x] 擴充 Rust unit、`tests/test_rust_ffi.c` smoke 與 `tests/test_mcp.c` end-to-end contract，覆蓋 supported/unknown/missing/non-string/root array/duplicate first-wins/invalid/trailing cases。
- [x] 驗證通過：`cargo fmt --all`、`cargo test -p cbm-core mcp --locked`（11 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（124 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（124 passed）。

## 本次工作階段（2026-07-07 Phase 3 MCP JSON-RPC error formatter codec opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `jsonrpc_format_error(id, code, message)`，固定 numeric-id error response compact JSON 欄位順序、message escaping、Unicode、NULL message 空字串與數值輸出 contract。
- [x] 新增 `cbm_rs_mcp_jsonrpc_format_error_v1` FFI，沿用 caller-provided buffer、完整長度回傳、length-only 與短 buffer NUL 截斷 contract。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `cbm_jsonrpc_format_error()` 會優先委派 Rust formatter，並保留 yyjson fallback；預設 C path 不變。
- [x] 擴充 `tests/test_rust_ffi.c` smoke 與 `tests/test_mcp.c` exact JSON contract，覆蓋一般錯誤、quote/backslash/newline escaping、NULL message 與短 buffer。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 string-id response、result embedding、MCP text result、invalid embedded JSON fallback、Content-Length transport、tool schema、dispatch 與 handlers 仍留 C/yyjson。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（12 passed）、`cargo test -p cbm-core --locked`（123 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（124 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（124 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-07 Phase 3 MCP JSON-RPC response formatter codec opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `jsonrpc_format_response(id, id_str, result_json, error_json)`，固定 numeric/string id、string id escaping、`result` embedding、`error` 優先、missing result/error 時 `result:null`、invalid embedded JSON 時只輸出 `jsonrpc/id` 的 C contract。
- [x] 新增 `cbm_rs_mcp_jsonrpc_format_response_v1` FFI，沿用 caller-provided buffer、完整長度回傳、length-only 與短 buffer NUL 截斷 contract。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `cbm_jsonrpc_format_response()` 會優先委派 Rust formatter，並保留 yyjson fallback；預設 C path 不變。
- [x] 擴充 `tests/test_rust_ffi.c` smoke 與 `tests/test_mcp.c` exact JSON contract，覆蓋 string id issue #253、escaped id、result/error、invalid result/error、`result:null` 與短 buffer。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 Content-Length transport、server loop、MCP text result、tool schema、dispatch 與 handlers 仍留 C/yyjson。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（13 passed）、`cargo test -p cbm-core --locked`（124 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（124 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（124 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-07 Phase 3 MCP text result formatter codec opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `mcp_text_result(text, is_error)`，固定 MCP text result compact JSON contract：`content[0].type/text`、`isError`、字串 escaping、NULL text 空字串、length-only 與短 buffer NUL 截斷。
- [x] 新增 `cbm_rs_mcp_text_result_v1` FFI，沿用 caller-provided buffer、完整長度回傳、length-only 與短 buffer NUL 截斷 contract。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `cbm_mcp_text_result()` 會優先委派 Rust formatter，並保留 yyjson fallback；預設 C path 不變。
- [x] 固定 `structuredContent` contract：只在非 error 且 `text` 是 JSON object 時加入 minified object；plain text、array、invalid JSON、NULL text 與 error result 都不加入。
- [x] 擴充 `tests/test_rust_ffi.c` smoke 與 `tests/test_mcp.c` exact JSON contract，覆蓋 structured object、plain text、array text、escaped error、NULL text、length-only 與短 buffer。
- [x] 更新 `rust/CBM_FFI.md`，明確標示 Content-Length transport、server loop、tool schema、dispatch 與 handlers 仍留 C/yyjson。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（14 passed）、`cargo test -p cbm-core --locked`（125 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（124 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（124 passed）。

## 本次工作階段（2026-07-07 Phase 3 MCP tool index codec opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `tool_index(name)`，固定 C `TOOLS[]` 的 14 個 tool name 順序、case-sensitive lookup、NULL/empty/unknown 回 -1 contract。
- [x] 新增 `cbm_rs_mcp_tool_index_v1` FFI，回傳 0..13 的 tool index 或 -1。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `cbm_mcp_tool_input_schema()` 會先委派 Rust 查 index，再回傳 C 本地 `TOOLS[idx].input_schema`；預設 C path 不變。
- [x] 保留 input schema JSON、title、description、tools/list serialization、tool dispatch、handlers、Content-Length transport 與 lifecycle 在 C/yyjson。
- [x] 擴充 `tests/test_rust_ffi.c` smoke 與 `tests/test_mcp.c` schema lookup contract，覆蓋 14 tools、unknown/null/empty/case-sensitive 與 CLI 共用 schema source。
- [x] 更新 `rust/CBM_FFI.md`，明確標示此切片只遷移 name -> index 純查找。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（15 passed）、`cargo test -p cbm-core --locked`（126 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（125 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）。

## 本次工作階段（2026-07-07 Phase 3 MCP tools/list page bounds codec opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `tools_page_bounds(offset, limit, include_next_cursor, tool_count)`，固定 `cbm_mcp_tools_list_range()` 的 offset/limit clamp、end、has_next 與 next_cursor contract。
- [x] 新增 `CbmRsMcpToolsPageBoundsV1` 與 `cbm_rs_mcp_tools_page_bounds_v1` FFI，回傳 page bounds 或在 out 為 NULL 時回 -1。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `cbm_mcp_tools_list_range()` 會先委派 Rust 計算 bounds，再由 C 用本地 `TOOLS[]` / yyjson 建立 tools/list JSON；預設 C path 不變。
- [x] 保留 tool metadata、input/output schema JSON、tools/list serialization、JSON-RPC response wrapping、dispatch、handlers、Content-Length transport 與 lifecycle 在 C/yyjson。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 first/second/full page、負 offset、超界 offset、負 limit、超界 limit、負 tool_count 與 NULL out。
- [x] 更新 `rust/CBM_FFI.md`，明確標示此切片只遷移 pagination range 計算。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（16 passed）、`cargo test -p cbm-core --locked`（127 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（125 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）。

## 本次工作階段（2026-07-07 Phase 3 MCP tools/list tool name manifest opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `tool_count()` / `tool_name(index)`，固定 C `TOOLS[]` 的 14 個 tool name 順序與 index -> name contract。
- [x] 新增 `cbm_rs_mcp_tool_count_v1` / `cbm_rs_mcp_tool_name_v1` FFI，支援 length-only、短 buffer NUL 截斷、負 index / 超界 index 回 `SIZE_MAX`。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `mcp_add_tool_def()` 會在 Rust count/name 與本地 `TOOLS[]` 完全一致時，用 Rust 回傳的 name 寫入 `tools/list`；預設 C path 不變，ABI 不一致時 fallback 到 C name。
- [x] 保留 title、description、input/output schema JSON、JSON 組裝、dispatch、handlers、Content-Length transport 與 lifecycle 在 C/yyjson。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 count、首中尾 tool name、length-only、短 buffer、負 index 與超界 index。
- [x] 更新 `rust/CBM_FFI.md`，明確標示此切片只遷移 `tools/list` name manifest。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（17 passed）、`cargo test -p cbm-core --locked`（128 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-07 Phase 3 MCP tools/list tool title manifest opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `tool_title(index)`，固定 C `TOOLS[]` 的 14 個 tool title 順序與 index -> title contract。
- [x] 新增 `cbm_rs_mcp_tool_title_v1` FFI，支援 length-only、短 buffer NUL 截斷、負 index / 超界 index 回 `SIZE_MAX`。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `mcp_add_tool_def()` 會在 Rust count/name/title 與本地 `TOOLS[]` 完全一致時，用 Rust 回傳的 title 寫入 `tools/list`；預設 C path 不變，ABI 不一致時 fallback 到 C title。
- [x] 保留 description、input/output schema JSON、JSON 組裝、dispatch、handlers、Content-Length transport 與 lifecycle 在 C/yyjson。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋首中尾 title、length-only、短 buffer、負 index 與超界 index。
- [x] 更新 `rust/CBM_FFI.md`，明確標示此切片只遷移 `tools/list` title manifest。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（18 passed）、`cargo test -p cbm-core --locked`（129 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP tools/list tool description manifest opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `tool_description(index)`，固定 C `TOOLS[]` 的 14 個 tool description 順序與 index -> description contract。
- [x] 新增 `cbm_rs_mcp_tool_description_v1` FFI，支援 length-only、短 buffer NUL 截斷、負 index / 超界 index 回 `SIZE_MAX`。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `mcp_add_tool_def()` 會在 Rust count/name/title/description 與本地 `TOOLS[]` 完全一致時，用 Rust 回傳的 description 寫入 `tools/list`；預設 C path 不變，ABI 不一致時 fallback 到 C description。
- [x] 保留 input/output schema JSON、JSON 組裝、dispatch、handlers、Content-Length transport 與 lifecycle 在 C/yyjson。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋首中尾 description、長描述關鍵片段、length-only、短 buffer、負 index 與超界 index。
- [x] 更新 `rust/CBM_FFI.md`，明確標示此切片只遷移 `tools/list` description manifest。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（19 passed）、`cargo test -p cbm-core --locked`（130 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP tools/list tool inputSchema manifest opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `tool_input_schema(index)`，固定 C `TOOLS[]` 的 14 個 input schema JSON 順序與 index -> schema contract。
- [x] 新增 `cbm_rs_mcp_tool_input_schema_v1` FFI，支援 length-only、短 buffer NUL 截斷、負 index / 超界 index 回 `SIZE_MAX`。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `mcp_add_tool_def()` 會在 Rust count/name/title/description/inputSchema 與本地 `TOOLS[]` 完全一致時，用 Rust 回傳的 schema JSON 建立 `inputSchema`；預設 C path 不變，ABI 不一致時 fallback 到 C schema。
- [x] 保留 yyjson parse/copy/serialization、outputSchema、dispatch、handlers、Content-Length transport 與 lifecycle 在 C/yyjson。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 empty schema、required fields、escaped quotes、path regex backslash、length-only、短 buffer、負 index 與超界 index。
- [x] 更新 `rust/CBM_FFI.md`，明確標示此切片只遷移 `tools/list` inputSchema manifest。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（20 passed）、`cargo test -p cbm-core --locked`（131 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP Content-Length header classifier opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `content_length_header_matches()`，固定 server loop 對 framed message header 的 pure classifier contract。
- [x] 新增 `cbm_rs_mcp_content_length_header_matches_v1` FFI，支援 nullable C string，只有精準 `Content-Length:` 前綴回傳 1。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `cbm_mcp_server_run()` 會用 Rust classifier 決定是否進入 Content-Length framed branch；無效長度仍由 framed branch consume，對齊原 C `strncmp` 行為。
- [x] 保留 length parsing、header/body separator、body allocation/read、response framing、`cbm_mcp_server_handle()`、poll loop、idle eviction、server lifecycle、dispatch 與 handlers 在既有 C/Rust 已分工路徑。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 basic/no-space/empty-value/bad-value、lowercase、leading-space、missing-colon、other header、empty 與 null。
- [x] 更新 `rust/CBM_FFI.md` 與 `Rust-Refactor.md`，明確標示此切片只遷移 Content-Length header classifier。
- [x] 驗證通過：`cargo fmt --all`、`cargo test -p cbm-core mcp --locked`（35 passed）、`make -f Makefile.cbm rust-ffi-test`、`cargo fmt --all -- --check`、`cargo test -p cbm-core --locked`（146 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP tool dispatch classifier opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `tool_dispatch_index()`，固定 `cbm_mcp_handle_tool()` 的 tool name -> handler index classifier。
- [x] 新增 `cbm_rs_mcp_tool_dispatch_index_v1` FFI，支援 nullable C string，未知、空字串、大小寫不符或 null 回傳 -1。
- [x] 保留 `trace_call_path` 相容 alias：Rust dispatch classifier 會回傳 `trace_path` handler index，對齊既有 C dispatch chain。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `cbm_mcp_handle_tool()` 會先用 Rust index switch 到既有 C handler；Rust 回傳超界、未知或與本地 `TOOLS[]` 驗證不一致時 fallback 到原 `strcmp` chain。
- [x] 保留所有 handler implementation、handler-specific 參數解析、tool result formatting、request logging、Content-Length transport、server lifecycle 與 `TOOLS[]` manifest 在 C。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 `trace_path`、`trace_call_path` alias、一般 tool、null、empty、case-sensitive mismatch 與 unknown。
- [x] 更新 `rust/CBM_FFI.md` 與 `Rust-Refactor.md`，明確標示此切片只遷移 tool dispatch classifier。
- [x] 目前已通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core --locked`（145 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP missing tool name message opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `missing_tool_name_message()`，固定 missing-tool-name error text：`missing tool name`。
- [x] 新增 `cbm_rs_mcp_missing_tool_name_message_v1` FFI，支援 length-only 與短 buffer NUL 截斷。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `cbm_mcp_handle_tool()` 的 `tool_name == NULL` branch 會優先用 Rust 建構 error text；ABI 異常或 buffer 放不下時 fallback 到原 literal。
- [x] 保留 `cbm_mcp_text_result()` result formatting、tool dispatch、handler-specific 參數解析、JSON-RPC response wrapper、request logging、Content-Length transport、server lifecycle 與 handlers 在 C。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 exact text、length-only 與短 buffer。
- [x] 更新 `rust/CBM_FFI.md` 與 `Rust-Refactor.md`，明確標示此切片只遷移 missing-tool-name error text builder。
- [x] 驗證通過：`cargo fmt --all`、`cargo test -p cbm-core mcp --locked`（33 passed）、`cargo fmt --all -- --check`、`cargo test -p cbm-core --locked`（144 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-09 Phase 3 MCP missing project error object opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `missing_project_error()`，固定 missing project argument 的 JSON error object 與 hint 字串。
- [x] 新增 `cbm_rs_mcp_missing_project_error_v1` FFI，支援 length-only 與短 buffer NUL 截斷。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `build_missing_project_error()` 會用 Rust 建構 C-owned heap string；ABI 異常、配置失敗或寫入長度不符時 fallback 到原 C literal。
- [x] 保留 `build_project_list_error()` 的 cache directory 掃描、available projects list、`build_no_store_error()` dispatch、`cbm_mcp_text_result()` result formatting、Store resolution、JSON-RPC wrapper、request logging、Content-Length transport、server lifecycle 與 handlers 在 C。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 exact JSON、length-only 與短 buffer。
- [x] 更新 `rust/CBM_FFI.md`、`Rust-Refactor.md` 與 `Handoff.md`，明確標示此切片只遷移 missing-project 固定 message builder。
- [x] 驗證通過：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（58 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP project-not-found message opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `project_not_found_message()`，固定 project open/resolve 失敗時的 error text：`project not found`。
- [x] 新增 `cbm_rs_mcp_project_not_found_message_v1` FFI，支援 length-only 與短 buffer NUL 截斷。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `build_project_not_found_message()` 會用 Rust 建構 C-owned heap string；ABI 異常、配置失敗或寫入長度不符時 fallback 到原 C literal。
- [x] 保留 project list 掃描、Store resolution、`cbm_mcp_text_result()` result formatting、JSON-RPC wrapper、request logging、Content-Length transport、server lifecycle 與 handlers 在 C。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 exact text、length-only 與短 buffer。
- [x] 更新 `rust/CBM_FFI.md`、`Rust-Refactor.md` 與 `Handoff.md`，明確標示此切片只遷移 project-not-found 固定 message builder。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（59 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP project-list error JSON builder opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `project_list_error()`，固定 `build_project_list_error()` 兩種 JSON shape：有 project 清單時輸出 `available_projects` + `count`，沒有清單時輸出 `No projects indexed yet` hint。
- [x] 新增 `cbm_rs_mcp_project_list_error_v1` FFI，支援 length-only 與短 buffer NUL 截斷；`reason == NULL` 或 `count > 0 且 projects_csv == NULL` 回 `SIZE_MAX`。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `build_project_list_error()` 會用 Rust 建構 C-owned heap string；ABI 異常、配置失敗或寫入長度不符時 fallback 到原 `snprintf` 序列化。
- [x] 保留 cache directory 掃描、`collect_db_project_names()`、`build_no_store_error()` dispatch、`cbm_mcp_text_result()` result formatting、Store resolution、JSON-RPC wrapper、request logging、Content-Length transport、server lifecycle 與 handlers 在 C。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null contract、with-projects/empty-list JSON shape、length-only 與短 buffer。
- [x] 更新 `rust/CBM_FFI.md`、`Rust-Refactor.md` 與 `Handoff.md`，明確標示此切片只遷移 project-list error JSON 序列化 helper。
- [x] 驗證通過：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（60 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-08 Phase 3 MCP unknown tool message opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `unknown_tool_message()`，固定 unknown-tool error text：`unknown tool: <name>`。
- [x] 新增 `cbm_rs_mcp_unknown_tool_message_v1` FFI，支援 length-only、短 buffer NUL 截斷，且 `tool_name == NULL` 回傳 `SIZE_MAX`。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `cbm_mcp_handle_tool()` 找不到 handler 時會優先用 Rust 建構 error text；長度放不進既有 `CBM_SZ_256` stack buffer 或 ABI 異常時 fallback 到原 `snprintf`。
- [x] 保留 `cbm_mcp_text_result()` result formatting、tool dispatch、handler-specific 參數解析、JSON-RPC response wrapper、request logging、Content-Length transport、server lifecycle 與 handlers 在 C。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 exact text、length-only、短 buffer 與 null tool name。
- [x] 更新 `rust/CBM_FFI.md` 與 `Rust-Refactor.md`，明確標示此切片只遷移 unknown-tool error text builder。
- [x] 驗證通過：`cargo fmt --all`、`cargo test -p cbm-core mcp --locked`（32 passed）、`cargo fmt --all -- --check`、`cargo test -p cbm-core --locked`（143 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP tools/call default arguments opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `tools_call_default_arguments()`，固定 `tools/call` request 沒有 `params` 時的 handler arguments fallback JSON shape：`{}`。
- [x] 新增 `cbm_rs_mcp_tools_call_default_arguments_v1` FFI，支援 length-only 與短 buffer NUL 截斷。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `cbm_mcp_server_handle()` `tools/call` branch 只在 `req.params_raw == NULL` 時用 Rust 建構 `tool_args` fallback；有 `params` 時仍走既有 `cbm_mcp_get_arguments()` / `cbm_rs_mcp_tools_call_arguments_v1`。
- [x] 保留 tool name dispatch、handler-specific 參數解析、JSON-RPC response wrapper、request logging、Content-Length transport、server lifecycle 與 handlers 在 C。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 exact JSON、length-only 與短 buffer。
- [x] 更新 `rust/CBM_FFI.md` 與 `Rust-Refactor.md`，明確標示此切片只遷移 `tools/call` no-params default arguments builder。
- [x] 驗證通過：`cargo fmt --all`、`cargo test -p cbm-core mcp --locked`（31 passed）、`cargo fmt --all -- --check`、`cargo test -p cbm-core --locked`（142 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP ping result object opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `ping_result()`，固定 `ping` result object JSON shape：`{}`。
- [x] 新增 `cbm_rs_mcp_ping_result_v1` FFI，支援 length-only 與短 buffer NUL 截斷。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `cbm_mcp_server_handle()` `ping` branch 的 `result_json` 會優先使用 Rust；ABI 異常或 buffer 不足時 fallback 到原 `heap_strdup("{}")`。
- [x] 保留 JSON-RPC response wrapper、request logging、Content-Length transport、server lifecycle 與 handlers 在 C。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 exact JSON、length-only 與短 buffer。
- [x] 更新 `rust/CBM_FFI.md` 與 `Rust-Refactor.md`，明確標示此切片只遷移 `ping` result object builder。
- [x] 驗證通過：`cargo fmt --all`、`cargo test -p cbm-core mcp --locked`（30 passed）、`cargo fmt --all -- --check`、`cargo test -p cbm-core --locked`（141 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP Method not found error object opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `method_not_found_error()`，固定 unknown-method error object JSON shape：`{"code":-32601,"message":"Method not found"}`。
- [x] 新增 `cbm_rs_mcp_method_not_found_error_v1` FFI，支援 length-only 與短 buffer NUL 截斷。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `cbm_mcp_server_handle()` unknown-method branch 的 error object 會優先使用 Rust；ABI 異常或 buffer 不足時 fallback 到原 `snprintf`。
- [x] 保留 request id echo、JSON-RPC response wrapper、request logging、Content-Length transport、server lifecycle 與 handlers 在 C。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 exact JSON、length-only 與短 buffer。
- [x] 更新 `rust/CBM_FFI.md` 與 `Rust-Refactor.md`，明確標示此切片只遷移 Method not found error object builder。
- [x] 驗證通過：`cargo fmt --all`、`cargo test -p cbm-core mcp --locked`（29 passed）、`cargo fmt --all -- --check`、`cargo test -p cbm-core --locked`（140 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）。

## 本次工作階段（2026-07-08 Phase 3 MCP initialize response builder opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `initialize_response()`，固定 initialize result object JSON shape：`protocolVersion`、`serverInfo.name/version`、`capabilities.tools.listChanged=false`。
- [x] 新增 `cbm_rs_mcp_initialize_response_v1` FFI，支援 length-only、短 buffer NUL 截斷、supported/unknown/missing/non-string/trailing params fallback 與 duplicate `protocolVersion` first-wins。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `cbm_mcp_initialize_response()` 會優先使用 Rust 產生完整 result JSON；ABI 失敗時 fallback 到原 yyjson builder。
- [x] 保留 initialize dispatch side effects（update check、session detection、auto-index）、JSON-RPC response wrapper、Content-Length transport、server lifecycle 與 handlers 在 C。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 exact JSON、版本 echo/fallback、duplicate first-wins、length-only、短 buffer 與 trailing JSON。
- [x] 更新 `rust/CBM_FFI.md` 與 `Rust-Refactor.md`，明確標示此切片只遷移 initialize result JSON builder。
- [x] 驗證通過：`cargo fmt --all`、`cargo test -p cbm-core mcp --locked`（28 passed）、`cargo fmt --all -- --check`、`cargo test -p cbm-core --locked`（139 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP method dispatch classifier opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `MethodKind` 與 `method_kind()`，固定 `initialize`、`ping`、`tools/list`、`tools/call`、`notifications/cancelled` 的 exact-match dispatch contract。
- [x] 新增 `cbm_rs_mcp_method_kind_v1` FFI，固定 stable enum：`0` unknown、`1` initialize、`2` ping、`3` tools/list、`4` tools/call、`5` notifications/cancelled；null、empty、大小寫不符、尾端空白與未知 method 都回 unknown。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `cbm_mcp_server_handle()` 的 notification cancellation 判斷與 request method 分類會委派 Rust；預設 C path 保持原本 `strcmp` 分類。
- [x] 保留 initialize side effects、ping/tools/list/tools/call handlers、pipeline cancellation side effect、logging、response wrapping、Content-Length transport、server lifecycle 與 14 個 tool handlers 在 C。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋全部 known methods、null/empty、大小寫、尾端空白與未知 method。
- [x] 更新 `rust/CBM_FFI.md` 與 `Rust-Refactor.md`，明確標示此切片只遷移 MCP method classifier。
- [x] 驗證通過：`cargo fmt --all`、`cargo test -p cbm-core mcp --locked`（27 passed）、`cargo fmt --all -- --check`、`cargo test -p cbm-core --locked`（138 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP file URI parser opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `parse_file_uri()`，固定 `file://` prefix、raw path preservation、Windows drive leading slash strip 與 invalid URI contract。
- [x] 新增 `cbm_rs_mcp_parse_file_uri_v1` FFI，支援 length-only、短 buffer NUL 截斷、empty path 與 invalid/null URI 回傳 `SIZE_MAX`。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `cbm_parse_file_uri()` 的 path extraction 會委派 Rust；預設 C path 保持原本 `strncmp` / drive strip / `snprintf` 行為。
- [x] 保留 caller buffer ownership、錯誤時清空輸出、MCP root/session lifecycle、URI 使用端與 tool handlers 在 C。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 Unix/root/Windows drive/percent encoding/length-only/短 buffer/empty path/bad scheme/null。
- [x] 更新 `rust/CBM_FFI.md`，明確標示此切片只遷移 MCP file URI path extraction。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（26 passed）、`cargo test -p cbm-core --locked`（137 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP Content-Length header separator opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `content_length_header_is_blank()`，固定移除尾端 CR/LF 後的 header/body 空行判斷。
- [x] 新增 `cbm_rs_mcp_content_length_header_is_blank_v1` FFI，null input 回傳 0。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `handle_content_length_frame()` 的 header/body separator 判斷會委派 Rust；預設 C path 保持原本 CR/LF strip 後檢查空字串。
- [x] 保留 `getline`、body allocation/read、`cbm_mcp_server_handle()`、response framing、poll loop、idle eviction、server lifecycle、dispatch 與 14 handlers 在 C。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 empty、LF、CRLF、連續 CR/LF、空白字元、一般 header 與 null。
- [x] 更新 `rust/CBM_FFI.md`，明確標示此切片只遷移 Content-Length header/body separator 判斷。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（25 passed）、`cargo test -p cbm-core --locked`（136 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP Content-Length response framing opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `content_length_response()`，固定 response frame 為 `Content-Length: <response byte length>\r\n\r\n<response>`。
- [x] 新增 `cbm_rs_mcp_content_length_response_v1` FFI，支援 length-only、短 buffer NUL 截斷、空 response 與 null response error contract。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `handle_content_length_frame()` 會先嘗試 Rust response frame，成功時以 `fwrite` 輸出完整 byte length；失敗時 fallback 到原本 C `fprintf` path。
- [x] 保留 header/body I/O、`cbm_mcp_server_handle()`、poll loop、idle eviction、server lifecycle、dispatch 與 14 handlers 在 C。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 JSON response、empty response、UTF-8 byte length、length-only、短 buffer 與 null response。
- [x] 更新 `rust/CBM_FFI.md`，明確標示此切片只遷移 Content-Length response framing。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（24 passed）、`cargo test -p cbm-core --locked`（135 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP Content-Length header parser opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `content_length_value()`，固定 Content-Length header length gate contract。
- [x] 新增 `cbm_rs_mcp_content_length_v1` FFI，回傳可接受 body 長度；不合法、非正數或超過 max 時回 0。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `cbm_mcp_server_run()` 的 Content-Length length gate 會委派 Rust；預設 C path 保持原本 `strtol`。
- [x] 保留 header/body I/O、response Content-Length framing、poll loop、idle eviction、server lifecycle、dispatch 與 14 handlers 在 C。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、大小寫不符、空值、正負號、尾端殘留、0/負數與 max gate。
- [x] 更新 `rust/CBM_FFI.md`，明確標示此切片只遷移 Content-Length header parser/gate。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（23 passed）、`cargo test -p cbm-core --locked`（134 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-12 MCP Content-Length raw length ABI 與超長 frame 同步修正）

- [x] 新增 `cbm_rs_mcp_content_length_raw_v1`，讓 C 端在保留既有最大 frame gate 的前提下取得宣告 body 長度。
- [x] 修正超長但語法有效的 Content-Length frame：default C 與 Rust opt-in 都會在拒絕處理前消耗完整 header/body，確保後續 frame 讀取同步。
- [x] 維持 body I/O、排空、response framing、server loop、dispatch 與 handlers 於 C。
- [x] 已實跑：`cargo test -p cbm-core mcp --locked`（69 passed）、default `build/c/test-runner mcp`（134 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（134 passed）、`make -f Makefile.cbm rust-ffi-test`。

## 本次工作階段（2026-07-08 Phase 3 MCP tools/list result JSON builder opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `tools_list_json()`，固定 `tools/list` result object 的 compact JSON contract。
- [x] 新增 `cbm_rs_mcp_tools_list_json_v1` FFI，支援 length-only 與短 buffer NUL 截斷。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `cbm_mcp_tools_list_range()` 會優先使用 Rust 產生完整 result JSON；預設 C path 不變，ABI 異常時 fallback 到 C/yyjson builder。
- [x] 保留 JSON-RPC response wrapper、Content-Length framing、server loop、tool dispatch、14 handlers 與 lifecycle 在 C/yyjson。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋第一頁、完整頁、空頁、length-only 與短 buffer。
- [x] 更新 `rust/CBM_FFI.md`，明確標示此切片只遷移 `tools/list` result JSON builder。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（22 passed）、`cargo test -p cbm-core --locked`（133 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP tools/list tool outputSchema manifest opt-in）

- [x] 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `tool_output_schema()`，固定 C `MCP_TOOL_OUTPUT_SCHEMA` 的共用 output schema JSON contract。
- [x] 新增 `cbm_rs_mcp_tool_output_schema_v1` FFI，支援 length-only 與短 buffer NUL 截斷。
- [x] 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `mcp_add_tool_def()` 會在 Rust outputSchema 與本地 C 常數完全一致時，用 Rust 回傳的 schema JSON 建立 `outputSchema`；預設 C path 不變，ABI 不一致時 fallback 到 C schema。
- [x] 保留 yyjson parse/copy/serialization、dispatch、handlers、Content-Length transport 與 lifecycle 在 C/yyjson。
- [x] 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋完整 JSON、length-only 與短 buffer。
- [x] 更新 `rust/CBM_FFI.md`，明確標示此切片只遷移 `tools/list` outputSchema manifest。
- [x] 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（21 passed）、`cargo test -p cbm-core --locked`（132 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

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
- [x] 後續已將同一 helper 接到 `CBM_USE_RUST_PIPELINE_PROJECT_NAME=1` production opt-in；見 2026-07-10 Phase 4 pipeline project-name opt-in。

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
- [x] 同步文件現況：Rust 目前已涵蓋 foundation 與 pipeline opt-in 切片；目前工作樹的 `rust/cbm-core/src` 約 18K 行 / 41 個 Rust 檔，仍非預設 Rust-backed binary。
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

## 本次工作階段（2026-07-11 diagnostics formatter production opt-in）

- [x] 將 Rust diagnostics 的 `format_path`、`format_json` 與 `format_ndjson` 接入 `CBM_USE_RUST_DIAGNOSTICS_FORMAT=1`，保留 C formatter fallback 與其餘 diagnostics runtime ownership。
- [x] 新增 `rust-diagnostics-format-optin-test`，依序執行 default diagnostics suite、Rust FFI smoke 與 diagnostics opt-in suite。
- [x] 完成本輪 diagnostics formatter gate：`cargo test -p cbm-core foundation::diagnostics --locked`（3 passed）、`rust-diagnostics-format-optin-test`（default/opt-in 各 7 passed）、`make -f Makefile.cbm rust-ffi-test`（exit 0）、`cargo fmt --all -- --check` 與 `git diff --check`。
- [x] 擴充 diagnostics opt-in 至 env value 與 query snapshot helper；default/opt-in suite 各 7 passed，FFI smoke、fmt 與 diff check 通過。
- [x] 修正 `CBM_USE_RUST_DUMP_VERIFY=1` 的 ratio FFI，使其真正使用 Rust byte-level parser，支援既有 C `strtod` 的十進位/十六進位 partial-prefix contract；`test-foundation` opt-in 325 passed、Rust unit 3 passed、clippy、FFI smoke、fmt 與 diff check 均通過。

- Phase 4 未完成任務 `Pipeline orchestration 全面化` 仍屬多輪工作：目前 registry + plan 決策層、full/incremental extraction 選擇、sequential extraction typed v2 metadata/trace、incremental extract/resolve typed v2 dispatch/trace、parallel extraction typed v2 metadata/trace、full pre-dump typed v2 metadata/trace、typed incremental post/tail dispatch、graph-buffer mutation command contract、多批 C-side adapter 呼叫點（structure、incremental purge/file-upsert/inbound restore、`TESTS`/`TESTS_FILE`、usage/configlink、similarity/semantic/decorator、infra/githistory、k8s/route/definitions/calls/semantic 與 `pass_parallel.c`），以及 language graph parity 的 Rust/Java/C++ 擴充與 incremental dump failure side-effect contract 已在 opt-in/default path 下驗證；C 端仍固定驗證 infra、dump、hash persist、FTS rebuild 與 artifact tail 順序，實際 side effects 與 graph-buffer storage/ownership 仍由既有 C 函式執行，暫不讓 Rust 重排具副作用步驟以符合「簡單優先／外科手術式」原則。
- Phase 5 未完成任務 `切換預設 build path 為 Rust-backed` **刻意尚未執行**：Rust 端目前承接決策層與 graph-buffer mutation command contract，C 端已有多批同步 adapter 呼叫點；實際 pass/extraction/tree-sitter/LSP 與 graph-buffer storage/ownership 仍在 C；提前切換會破壞產品，且違反本計畫「C fallback 需經至少一個 release cycle 驗證」的完成標準。`切換前 packaging/install/UI/security/perf 驗證` 為 Phase 5 清理前置 gate，於切換前不觸發；README/CONTRIBUTING/contributor guide 與 CI 已於本階段更新，release workflow 待預設切換時再調整。

## 本次工作階段（2026-07-02 朝完整移除 C 推進）

- [x] 新增「完整移除 C 的路線圖」：`Rust-Refactor.md` 依子系統排序（foundation 剩餘 → pipeline → store → cypher → mcp → internal/cbm → 預設切換 → 移除）並附最終檢核表；`Tasks.md` 新增對應追蹤 checklist。誠實反映現況（預設 100% 純 C、Rust 仍只佔整體自有程式碼少數、零預設 Rust 取代）。
- [x] 推進 foundation 下一切片：新增 `hash_table` Rust parity module（`rust/cbm-core/src/foundation/hash_table.rs`，純安全 Rust + 4 個單元測試）與 test-only `cbm_rs_ht_*` FFI（`ffi.rs`），並在 `tests/test_rust_ffi.c` 新增 parity 測試，固定 content-key、`get_key` stored 指標（覆寫更新）、NULL value 為存在項、foreach exactly-once、非 UTF-8 高位元 key、null 契約與 resize。維持 C-only hot path，不導入產品 opt-in。
- [x] 嚴謹驗證全綠：`scripts/rust-check.sh`（fmt、clippy `-D warnings`、`cargo test --workspace`、`rust-ffi-test`、foundation opt-in matrix、pipeline registry/plan opt-in、language-graph parity）、`make rust-dangerous-api`（148 findings 全允許清單，新 FFI 由 `ffi.rs` wildcard 覆蓋）、`scripts/lint.sh --ci`（CI 對齊 `cppcheck 2.20.0` + `clang-format 20.1.8`，含 `test_rust_ffi.c`）。
- 說明：此切片為 test-only parity（不改變預設 C 路徑），是既有方法論中 production opt-in 的前置步驟；離「可移除 C」仍需完成路線圖各階段。

## `str-intern-only` source replacement

- [x] 執行 `make -f Makefile.cbm rust-str-intern-only-test`：default/direct foundation runner 各 `325 passed`，Rust FFI 與 source exclusion link 通過。
- [x] 將 gate 的實際結果與 direct all-optin 組合結果回填至 `Handoff.md`；wrapper/direct all-optin 各 `5861 passed`。

## `dump-verify-only` source replacement

- [x] Rust direct ABI、bounded environment/parser、固定參數 logger bridge 與 C fallback 已完成。
- [x] `rust-dump-verify-only-test` default/direct foundation runner 各 `325 passed`，direct source list 排除 `src/foundation/dump_verify.c`。
- [x] wrapper/direct all-optin 各 `5861 passed`；direct 使用四個 Rust-only features，production binary 與版本輸出成功。

## `yaml-only` source replacement

- [x] Rust direct opaque-handle ABI、Box ownership 與 C `strtod` parity 已完成。
- [x] `rust-yaml-only-test` default/direct foundation 各 `325 passed`，direct source list 排除 `src/foundation/yaml.c`，production link/version 通過。
- [x] Rust YAML unit `4 passed`；wrapper/direct 五 feature all-optin 各 `5861 passed`。

- [x] 完成 `store-search-pattern-only` direct slice：Rust 接管 glob-to-LIKE 與 LIKE hint heap helper，保留 C heap ownership ABI；聚焦測試與六切片全量 opt-in 均通過。

- [x] 完成 `store-arch-helpers-only` direct slice：Rust 接管 risk/QN/test-file 純 helper，保留 thread-local C ABI 與 default fallback；聚焦 gate、direct FFI、production build 及七切片全量 opt-in 均通過。

- [x] 完成 `store-file-ext-only` direct slice：Rust 接管副檔名純 helper，保留 null 與 16-byte thread-local C ABI；聚焦 gate、direct FFI、production build 及八切片全量 opt-in 均通過。

- [x] 完成 `store-immutable-uri-only` direct slice：Rust 接管 immutable URI buffer encoder，保留容量/null/終止字元契約；聚焦 gate、direct FFI、production build 及九切片全量 opt-in 均通過。

- [x] 完成 `store-mmap-resolver-only` direct slice：Rust 接管 mmap env resolver 與 parser，保留 64 MiB/default、負值、overflow、trailing bytes 契約；聚焦 gate、direct FFI、production build 均通過。

- [x] 完成 `store-arch-path-scope-only` direct 切片：由 Rust 提供 architecture path 正規化與
  LIKE pattern 緩衝區 ABI，保留 SQL fragment 與 SQLite bind 於 C；專屬與 11-slice all-opt-in
  wrapper/direct gate 均已通過。

- [x] 完成 `store-fts-tokenizer-only` direct 切片：Rust 實作 SQLite FTS camelCase callback，
  以 SQLite 配置器移交結果 ownership 並隔離通用 FFI object；default/direct 與 12-slice
  all-opt-in wrapper/direct gate 均已通過。

- [x] 完成 `pipeline-project-name-only` direct 切片：Rust 產生 validator-safe project name，
  並以 C `malloc` 維持呼叫端 `free()` 契約；default/direct FQN suite 與 13-slice
  all-opt-in wrapper/direct gate 均已通過。

- [x] 完成 `pipeline-artifact-path-only` direct 切片：Rust 提供 artifact path 的 caller-buffer
  ABI，C 保留 SQLite、zstd、metadata 與檔案 I/O；default/direct artifact suite 與 14-slice
  all-opt-in wrapper/direct gate 均已通過。
- [x] 完成 `pipeline-test-detect-only` direct 切片：Rust 提供 test path/function
  classifier，C 保留 TESTS/TESTS_FILE graph edge 與 heap-returning path conversion；
  default/direct pipeline suite 與 15-slice all-opt-in wrapper/direct gate 均已通過。

- [x] 完成 `pipeline-checked-exception-only` direct 切片：Rust 提供 null-safe
  exception-name predicate，C 保留 THROWS/RAISES edge resolution；default/direct 聚焦測試
  與 16-slice all-opt-in wrapper/direct gate 均已通過。2026-07-15 再以隔離 build 目錄
  驗證 Rust unit `1 passed`、default/direct `edge_types_probe pipeline` 各 `273 passed`、
  direct 同名 ABI FFI smoke、production link 與 `--version`；Make 展開的
  `CHECKED_EXCEPTION_SRCS =` 與 link line 均確認未編入 C implementation。

## 本次工作階段（2026-07-13 CLI progress sink direct replacement）

- [x] 新增 Cargo `progress-sink-only` feature 與 Rust direct `cbm_progress_sink_*` ABI。
- [x] `CBM_USE_RUST_PROGRESS_SINK_ONLY=1` 排除 `src/cli/progress_sink.c`，並保留 C `cbm_log_set_sink()`、`FILE*`、stderr 與 logger lifecycle ownership。
- [x] 新增 `rust-cli-progress-sink-only-test`，納入 `rust-ci` 與 `rust-ci-tests`；direct FFI link 補入 `src/foundation/log.c`。
- [x] 驗證 default `progress_sink` 2 passed、direct `test-rust-ffi`、direct `progress_sink` 2 passed、direct production link 與 `--version` smoke。
- [ ] CLI 其他 C 模組、MCP/server lifecycle 與整體 Rust-backed default path 仍未完成；本切片不構成 Phase 5 release candidate。

## 本次工作階段（2026-07-13 Foundation `str_util` direct replacement）

- [x] 新增 `str-util-only` Cargo feature 與 Rust direct public C ABI。
- [x] 以 C `cbm_arena_alloc` 保留 `CBMArena` allocation ownership，並保留 path borrowed pointer contract。
- [x] `CBM_USE_RUST_STR_UTIL_ONLY=1` 排除 `src/foundation/str_util.c`，新增 `rust-str-util-only-test` 並接入 CI/all-optin direct flags。
- [x] 驗證 default foundation 325 passed、direct FFI archive smoke、direct foundation 325 passed、direct production link 與 `--version` smoke。
- [ ] Foundation allocator/OS runtime、其餘產品子系統與 Phase 5 Rust-backed default path 仍未完成。

## 2026-07-13 全組合 direct gate

- [x] 在所有既有 opt-in slices 下執行 wrapper full runner，`5869 passed`。
- [x] 在同一組合下執行 direct full runner，`5869 passed`。
- [x] 確認 direct production link 不含 `src/foundation/str_util.c` 與 `src/cli/progress_sink.c`，且 binary `--version` 成功。
- [ ] 持續遷移尚未替換的 C subsystem，直到 release candidate gate 可移除 C fallback。

## 2026-07-13 hash table direct replacement

- [x] 完成 `hash-table-only` Rust opaque ABI 與 borrowed-key ownership contract。
- [x] 完成 default/direct foundation gate：各 `325 passed`。
- [x] direct production link 排除 `src/foundation/hash_table.c` 並成功回報版本。
- [ ] 繼續處理尚未替換的 C foundation、store、Cypher、MCP、pipeline orchestration 與 Tree-sitter/LSP subsystem。

## 2026-07-13 hash table 全矩陣驗證

- [x] 將 hash table 加入 all-optin direct 路徑；wrapper/direct 均為 `5869 passed`。
- [x] 確認 direct production link 排除 `src/foundation/hash_table.c`，並完成 `codebase-memory-mcp dev` 版本檢查。
- [ ] 繼續處理尚未替換的 C foundation、store、pipeline、cypher、MCP 與 runtime 邊界。

## 2026-07-13 diagnostics formatter

- [x] 新增 `diagnostics-format-only` feature 與 direct C guard。
- [x] 以 `repr(C)` snapshot ABI 接管 env/path/JSON/NDJSON formatter，保留 C runtime side effects。
- [x] default/direct foundation gate 各 `325 passed`，direct FFI、production link 與 version 檢查通過。
- [ ] 將 diagnostics formatter direct slice 納入下一輪 all-optin 矩陣與後續預設 Rust 路徑切換。

## 2026-07-13 store language map direct slice

- [x] 新增 `CBM_USE_RUST_STORE_LANGUAGE_MAP_ONLY=1`，以 Rust 完成 store 副檔名到語言種類的查找，保留 C 靜態字串表的 borrowed ABI。
- [x] `store_arch mcp` default/direct focused gate 各 `189 passed`，並完成 direct FFI、production link、版本檢查。
- [x] all-optin wrapper/direct 各 `5869 passed`，production link 與版本檢查通過。
- [ ] 仍需遷移 store 的 SQLite runtime、查詢生命週期、圖形讀寫與其餘 C 狀態管理；本切片不代表整個 store 已由 Rust 取代。

## 2026-07-13 pipeline module-directory direct slice

- [x] 新增 `CBM_USE_RUST_PIPELINE_MODULE_DIR_ONLY=1`，讓 parallel pipeline 的模組目錄分類直接由 Rust 執行。
- [x] default/direct pipeline focused gate 各 `221 passed`，並完成 direct FFI、production link、版本檢查。
- [ ] `internal/cbm/helpers.c` 與 `pass_lsp_cross.c` 仍有同語意 C 判定，必須在共用 ABI 與跨模組一致性驗證後再移除；完整 pipeline runtime 仍是 C。

- [x] 加入 all-optin direct flag 後，wrapper/direct 全矩陣成功，direct production link 與版本檢查通過。

## 2026-07-13 platform path direct slice

- [x] 新增 `CBM_USE_RUST_PLATFORM_PATH_ONLY=1`，直接取代兩個平台分支的 path separator normalization。
- [x] default/direct foundation focused gate 各 `325 passed`，並完成 direct FFI、production link、版本檢查。
- [ ] platform filesystem、home/config/cache directory 與其餘 OS abstraction 仍是 C。

- [x] 加入 platform-path-only 後 all-optin wrapper/direct runner 與 production link 均成功。

## 2026-07-13 compat thread direct slice

- [x] 新增 `CBM_USE_RUST_COMPAT_THREAD_ONLY=1`，直接取代 stack-size 與 aligned allocation precheck 的 C 分支。
- [x] default/direct foundation focused gate 各 `325 passed`，並完成 direct FFI、production link、版本檢查。
- [ ] pthread/mutex wrapper 與 allocator ownership 仍是 C，不能由此切片宣稱 compat thread 完成 Rust 化。

- [x] 加入 compat-thread-only 後 all-optin wrapper/direct runner 與 production link 均成功。

## 2026-07-13 MCP codec direct slice

- [x] 新增 `CBM_USE_RUST_MCP_CODEC_ONLY=1`，直接使用 Rust MCP codec ABI，排除相對應的 C codec fallback。
- [x] default/direct MCP focused gate 各 `136 passed`，並完成 direct FFI、production link、版本檢查。
- [ ] MCP request handler、yyjson document、store lookup 與 server lifecycle 仍是 C。

- [x] 加入 MCP codec-only 後 all-optin wrapper/direct runner 與 production link 均成功。

## 2026-07-13 Cypher classifier direct slice

- [x] 六組 Cypher lexer/function classifier only gates 已加入並切換 direct ABI。
- [x] default/direct Cypher focused gate 各 `144 passed`，並完成 direct FFI、production link、版本檢查。
- [ ] Cypher parser、expression evaluator、SQLite graph execution 仍是 C。

- [x] 加入六組 Cypher classifier only flags 後 all-optin wrapper/direct runner 與 production link 均成功。

## 2026-07-13 compat regex direct slice

- [x] 新增 `CBM_USE_RUST_COMPAT_REGEX_ONLY=1`，直接取代三個 regex 純 helper。
- [x] default/direct foundation focused gate 各 `325 passed`，並完成 direct FFI、production link、版本檢查。
- [ ] regex engine 與 platform regex ABI 仍是 C。

- [x] 將 compat_regex 的既有 Rust 純邏輯擴充為 `CBM_USE_RUST_COMPAT_REGEX_ONLY` direct gate，保留 C regex engine 與資源生命週期。

- [x] 將 log level/format 環境值解析擴充為 `CBM_USE_RUST_LOG_ENV_PARSE_ONLY` direct gate，保留 C logger 狀態與輸出生命週期。

- [x] 將 log env parse direct gate 納入 `rust-all-optin-test`，wrapper/direct runner 與正式版連結成功。

- [x] 將 profile env 判斷擴充為 `CBM_USE_RUST_PROFILE_ENV_ONLY` direct gate，保留 C profiling 狀態與時間生命週期。

- [x] 將 profile env direct gate 納入 `rust-all-optin-test`，wrapper/direct runner 與正式版連結成功。

- [x] 將 platform env-directory helper 擴充為 `CBM_USE_RUST_PLATFORM_ENV_DIRS_ONLY` direct gate，保留 C 平台資源與 buffer 生命週期。

- [x] 將 platform env dirs direct gate 納入 `rust-all-optin-test`，wrapper/direct runner 與正式版連結成功。

- [x] 將 pipeline route canonicalizer 擴充為 `CBM_USE_RUST_PIPELINE_ROUTE_CANON_ONLY` direct gate，排除 C scanner 並保留 Route graph orchestration。

- [x] 將 pipeline route canonicalizer direct gate 納入 `rust-all-optin-test`，wrapper/direct runner 與正式版連結成功。

- [x] 將 pipeline git history trackable-file filter 擴充為 `CBM_USE_RUST_PIPELINE_GITHISTORY_ONLY` direct gate，保留 C git/graph ownership。

- [x] 將 pipeline git history direct gate 納入 `rust-all-optin-test`，wrapper/direct runner 與正式版連結成功。

- [x] 將 pipeline git diff range/name-status/hunks 三個 parser 擴充為 `_ONLY` direct gates，保留 C caller 與輸出結構 ownership。

- [x] 將 pipeline gitdiff 三個 direct gates 納入 `rust-all-optin-test`，wrapper/direct runner 與正式版連結成功。

- [x] 將 pipeline infrascan filename classifiers 擴充為 `CBM_USE_RUST_PIPELINE_INFRASCAN_ONLY` direct gate，保留 C secret/manifest parser 與 graph ownership。

### 2026-07-13 測試偵測 direct gate 接線修正

- [x] 讓 `pass_tests.c` 在 `CBM_USE_RUST_PIPELINE_TEST_DETECT_ONLY` 下使用 Rust 測試路徑轉換器。
- [x] 修正 Makefile 的 `_ONLY` CFLAGS 與 Rust staticlib 選擇條件。
- [x] 通過 base 與 direct pipeline gate，各 `221 passed`。

### 2026-07-13 Linux cgroup helper direct replacement

- [x] 以 Rust 取代 `system_info.c` 的 cgroup CPU/memory 解析 fallback。
- [x] 新增 `rust-platform-cgroup-only-test`，涵蓋 default/direct、FFI、正式版連結與版本輸出。
- [x] default/direct foundation 各 `325 passed`。

### 2026-07-13 全量矩陣回歸

- [x] cgroup `_ONLY` 接線後重新通過 `rust-all-optin-test`，wrapper/direct 測試與 production version 均成功。

### 2026-07-13 Registry test-QN classifier

- [x] `is_test_qn()` direct `_ONLY` 使用 Rust classifier。
- [x] 通過 registry default `53 passed`、direct FFI/runner、production version gate。

### 2026-07-13 Registry classifier 全量回歸

- [x] registry test-QN `_ONLY` 接入後重新通過全量 wrapper/direct 矩陣與 production version。

## 2026-07-15 CLI 版本比較 Rust slice

- [x] 新增 cli::version Rust pure helper、cbm_rs_cli_compare_versions_v1 與 cli-version-only direct ABI。
- [x] 接入 CBM_USE_RUST_CLI_VERSION / CBM_USE_RUST_CLI_VERSION_ONLY、Rust FFI smoke 與 default/wrapper/direct Make target。
- [ ] 尚未執行此切片的 Rust unit、FFI、CLI default/wrapper/direct runner、production link 與版本 smoke；未完成驗證前不得計入 release candidate。
- [ ] CLI install/update/uninstall/config、MCP lifecycle 與其他 C CLI runtime 仍待後續 Rust 化。


## 2026-07-15 discover 字串 helper Rust slice

- [x] 在 rust/cbm-core/src/discover.rs 實作 str_in_list、ends_with、str_contains、ascii_ieq。
- [x] 新增 stable v1 FFI 與 direct ABI，保留 NULL 結尾 C 陣列契約。
- [x] 將 C fallback 拆至 src/discover/discover_string_helpers.c；direct source list 排除該
  compilation unit，discover.c 保留呼叫端。
- [x] 新增 Rust unit、tests/test_rust_ffi.c smoke 與 Makefile gates。
- [x] 驗證 Rust unit `1 passed`、default/wrapper/direct FFI smoke 與 discover suite 各
  `85 passed`、wrapper/direct production link 與 `--version`；direct link line 不含
  src/discover/discover_string_helpers.c。


## 2026-07-15 watcher polling interval Rust slice

- [x] 在 rust/cbm-core/src/watcher.rs 實作 adaptive poll interval 純計算。
- [x] 新增 stable v1 FFI、wrapper opt-in 與 watcher-poll-interval-only direct ABI。
- [x] 將 C fallback 抽至 src/watcher/poll_interval.c；direct source list 排除該
  compilation unit，watcher.c 保留 Git、時間、thread、輪詢與 callback。
- [x] 新增 Rust unit、tests/test_rust_ffi.c smoke 與 Makefile gates。
- [x] 驗證 Rust unit `1 passed`、default/direct full runner 各 `5868 passed`、
  default/direct FFI smoke、wrapper FFI 與 watcher suite `53 passed`、direct/wrapper
  production link 與 `--version`；direct link line 不含 src/watcher/poll_interval.c。


## 2026-07-15 Pipeline SvelteKit file kind Rust slice

- [x] 在 rust/cbm-core/src/pipeline/route.rs 實作 SvelteKit file kind classifier。
- [x] 新增 stable v1 FFI、wrapper opt-in 與 pipeline-sveltekit-file-kind-only direct ABI。
- [x] 將 C fallback 拆至 src/pipeline/sveltekit_file_kind.c/.h，並讓 direct source selector 排除
  該 compilation unit；src/pipeline/pass_route_nodes.c 僅經由公開 C bridge 呼叫。
- [x] 新增 Rust unit、tests/test_rust_ffi.c smoke、C classifier regression 與 Makefile gates。
- [x] 修正 direct-only 仍呼叫大型 C 檔內 local static helper 的編譯缺口。
- [x] 2026-07-15 歷史驗證曾通過 default/wrapper/direct FFI smoke、pipeline suite 各
  227 passed、wrapper/direct production link 與 --version；direct Make 展開與 link line 不含
  src/pipeline/sveltekit_file_kind.c。
- [x] 2026-07-16 在補齊六個 basename 測試與改用 make -Bn 後，重跑 current-revision
  default/wrapper/direct FFI、pipeline（各 227 passed）、production --version 與
  source-negative gate；另通過 cargo test 307、clippy、fmt。
- [x] 凍結 NULL、`/routes/`、六個精確 basename 與 1/2/3/0 回傳值的 C/Rust 契約，並更新交接狀態。


## 2026-07-15 Pipeline SvelteKit server method Rust slice

- [x] 在 rust/cbm-core/src/pipeline/route.rs 實作 SvelteKit server method mapping。
- [x] 新增 stable v1 FFI、wrapper opt-in 與 pipeline-sveltekit-server-method-only direct ABI。
- [x] 將 C fallback 拆至 src/pipeline/sveltekit_server_method.c/.h，並讓 direct source list
  排除該 compilation unit。
- [x] 在 src/pipeline/pass_route_nodes.c 經由公開 C bridge 接入 wrapper/direct 分派，保留
  default C fallback。
- [x] 新增 Rust unit、tests/test_rust_ffi.c smoke 與 Makefile gates。
- [x] 驗證 Rust unit `1 passed`、隔離 default/wrapper/direct FFI smoke、pipeline suite 各
  `222 passed`、wrapper/direct production link 與 `--version`；direct link line 不含
  src/pipeline/sveltekit_server_method.c。


## 2026-07-15 CLI hook token Rust slice

- [x] 在 rust/cbm-core/src/cli/hook.rs 實作 identifier-like token extractor。
- [x] 新增 stable v1 FFI、wrapper opt-in 與 cli-hook-token-only direct ABI。
- [x] 在 src/cli/hook_augment.c 接入 Rust wrapper/direct 分派，保留 default C。
- [x] 新增 Rust unit、tests/test_rust_ffi.c smoke 與 Makefile gates。
- [ ] 執行 CLI hook token default、wrapper、direct 與 production link gate。
- [ ] 依 gate 結果修正問題並更新交接狀態。


## 2026-07-15 Git path absolute Rust slice

- [x] 在 rust/cbm-core/src/git_context.rs 實作平台化 absolute path classifier。
- [x] 新增 stable v1 FFI、wrapper opt-in 與 git-path-absolute-only direct ABI。
- [x] 將 C fallback 拆至 src/git/path_absolute.c/.h，並讓 direct source selector 排除該
  compilation unit；src/git/git_context.c 僅經由公開 C bridge 呼叫。
- [x] 新增 Rust unit、tests/test_rust_ffi.c smoke、C classifier regression 與 Makefile gates。
- [x] 執行並通過 default/wrapper/direct FFI smoke、pipeline suite 各 `226 passed`、
  wrapper/direct production link 與 `--version`；direct Make 展開與 link line 不含
  src/git/path_absolute.c。
- [x] 凍結 NULL、slash、ASCII Windows drive-letter、非 ASCII／UNC 拒絕的 C/Rust 契約，並更新
  交接狀態。


## 2026-07-15 Git JSON escaped length Rust slice

- [x] 在 rust/cbm-core/src/git_context.rs 實作 JSON escaped length 純計算。
- [x] 新增 stable v1 FFI、wrapper opt-in 與 git-json-escaped-len-only direct ABI。
- [x] 在 src/git/git_context.c 接入 Rust wrapper/direct 分派，保留 default C。
- [x] 新增 Rust unit、tests/test_rust_ffi.c smoke 與 Makefile gates。
- [x] 執行並通過 default/wrapper/direct FFI smoke、pipeline suite 各 `224 passed`、
  wrapper/direct production link 與 `--version`；direct Make 展開與 link line 不含
  src/git/json_escaped_len.c。
- [x] 修正 C/Rust 對 LF、CR、TAB 的長度不一致與 C `int` 累加溢位，並更新交接狀態。


## 2026-07-15 Git trim newlines Rust slice

- [x] 在 rust/cbm-core/src/git_context.rs 實作 C buffer 尾端 CR/LF 清除。
- [x] 新增 stable v1 FFI、wrapper opt-in 與 git-trim-newlines-only direct ABI。
- [x] 將 C fallback 拆至 src/git/trim_newlines.c/.h，並讓 direct source list 排除該
  compilation unit。
- [x] 在 src/git/git_context.c 經由公開 C bridge 接入 wrapper/direct 分派，保留 default C
  fallback。
- [x] 新增 Rust unit、tests/test_rust_ffi.c smoke 與 Makefile gates。
- [x] 執行並通過 default/wrapper/direct FFI smoke、pipeline suite 各 `223 passed`、
  wrapper/direct production link 與 `--version`；direct link line 不含
  src/git/trim_newlines.c。


## 2026-07-15 Log path query Rust slice

- [x] 將 C fallback 自 `src/foundation/log.c` 拆至 `src/foundation/log_path.c/.h`，並讓
  direct source selector 排除該 compilation unit。
- [x] 新增 stable v1 FFI、wrapper opt-in 與 `log-copy-path-only` direct ABI；Rust 以容量有界
  copy 保留首個 NUL／`?`／`#` 停止、NUL 結尾與 `outsz=0/1` 行為。
- [x] 明定 `path` 為 nullable NUL 字串，`out` 若非 NULL 必須有 `outsz` bytes 且不得與
  `path` 重疊。
- [x] 在 `src/foundation/log.c` 經由公開 C bridge 接入 wrapper/direct 分派，保留 default C
  fallback。
- [x] 新增 Rust unit、`tests/test_rust_ffi.c` smoke、`outsz=0/1` 與 `?/#` C regression，
  以及 Makefile default/wrapper/direct gates。
- [x] 執行並通過 default/wrapper/direct FFI smoke、`log` suite 各 `16 passed`、
  wrapper/direct production link 與 `--version`；direct link line 不含
  `src/foundation/log_path.c`。


## 2026-07-15 Pipeline JSON property Rust slice

- [x] 在 rust/cbm-core/src/pipeline/route.rs 實作固定 JSON property parser。
- [x] 新增 stable v1 FFI、wrapper opt-in 與 pipeline-json-prop-only direct ABI。
- [x] 在 src/pipeline/pass_route_nodes.c 接入 Rust wrapper/direct 分派，保留 default C。
- [x] 新增 Rust unit、tests/test_rust_ffi.c smoke 與 Makefile gates。
- [ ] 執行 Pipeline JSON property default、wrapper、direct 與 production link gate。
- [ ] 依 gate 結果修正問題並更新交接狀態。


## 2026-07-15 Pipeline URL path Rust slice

- [x] 在 rust/cbm-core/src/pipeline/route.rs 實作保留 pointer provenance 的 URL path raw scan。
- [x] 新增 stable v1 FFI、wrapper opt-in 與 pipeline-url-path-only direct ABI。
- [x] 將 C fallback 拆至 src/pipeline/url_path.c/.h，讓 pass_route_nodes.c 經公開 bridge
  呼叫，並使 direct source selector 排除該 compilation unit。
- [x] 新增 Rust unit、tests/test_rust_ffi.c pointer-provenance smoke、C public bridge contract
  與 Makefile default/wrapper/direct gates。
- [x] 執行並通過 default/wrapper/direct FFI smoke、pipeline suite 各 225 passed、
  wrapper/direct production link 與 --version；direct source-negative gate 不含
  src/pipeline/url_path.c。
- [x] 完成 307 passed Rust suite、clippy、fmt 與交接文件更新。


## 2026-07-15 Discover trailing separator Rust slice

- [x] 在 rust/cbm-core/src/discover.rs 實作 trailing separator predicate。
- [x] 新增 stable v1 FFI、wrapper opt-in 與 discover-trailing-sep-only direct ABI。
- [x] 在 src/discover/discover.c 接入 Rust wrapper/direct 分派，保留 default C。
- [x] 新增 Rust unit、tests/test_rust_ffi.c smoke 與 Makefile gates。
- [x] 拆出獨立 C fallback 與公開 bridge（src/discover/trailing_sep.c/.h）；discover.c 只呼叫
  cbm_discover_has_trailing_sep()，不再 macro 改名。
- [x] 凍結新公開 bridge 的 NULL／empty → false 防禦性契約，並補 C regression 與 direct FFI
  empty/plain/backslash/NULL 覆蓋。
- [x] 新增 DISCOVER_TRAILING_SEP_SRCS selector、實際執行的 default/wrapper/direct
  FFI/discover/production gate，以及 make -Bn source-negative assertion（2026-07-16：discover
  suite 各 86 passed）。


## 2026-07-15 Discover path join Rust slice

- [x] 在 rust/cbm-core/src/discover.rs 實作 path join 純 buffer helper。
- [x] 新增 stable v1 FFI、wrapper opt-in 與 discover-path-join-only direct ABI。
- [x] 在 src/discover/discover.c 接入 Rust wrapper/direct 分派，保留 default C。
- [x] 新增 Rust unit、tests/test_rust_ffi.c smoke 與 Makefile gates。
- [x] 執行 Discover path join default、wrapper、direct 與 production link gate
  （2026-07-16：discover **87**、selector 空、make -Bn 不含 path_join.c）。
- [x] 依 gate 結果更新交接狀態（true-source 完成）。


- [x] SimHash MinHash Jaccard helper：`rust/cbm-core/src/simhash.rs`、`cbm_rs_simhash_jaccard_v1` 與 `CBM_USE_RUST_SIMHASH_JACCARD` wrapper 已建立；只搬移固定 64 槽相似度計算，AST/xxHash/LSH 仍在 C；2026-07-15 已驗證 Rust unit 4 項、default C `simhash` 24 項、FFI smoke 與 Rust opt-in `simhash` 24 項。

- [x] MinHash hex encode/decode 已納入同一 stable FFI opt-in slice；固定 512 字元格式與 caller-owned buffer 已由同一 Rust unit、FFI smoke 與 default/opt-in `simhash` gate 驗證。


- [ ] Pipeline complexity JSON readers：新增 `pipeline_complexity.rs`、stable ABI 與 `CBM_USE_RUST_PIPELINE_COMPLEXITY_JSON` wrapper；graph DFS 與 node mutation 仍在 C；unit、FFI smoke、default/opt-in gate 尚未執行。


- [x] VMem page rounding：已重分類為非產品 FFI parity fixture；`src/foundation/vmem.c` 未納入產品 source list，故不具有 default/opt-in runtime gate，也不計入 C→Rust replacement。2026-07-15 已通過 Rust unit 2 項與 FFI smoke。

- [x] Makefile gate 執行修正：`rust-pipeline-test-node-is-test-optin-test`、`rust-ast-profile-classifiers-optin-test`、`rust-pipeline-usages-json-optin-test`、`rust-semantic-token-classifiers-optin-test` 現跑 FFI smoke 與 `pipeline` suite，2026-07-15 各 221 項通過；`rust-ui-layout-helpers-optin-test` 與 `rust-ui-httpd-helpers-optin-test` 分別跑 FFI smoke 加 `ui` 15 項與 `httpd` 32 項，皆通過。這只是驗證 gate 修正，不表示子系統遷移完成。


- [ ] Pipeline pkgmap text helpers：`at_prefix` 與 `find_line_value` 已以 Rust offset ABI 接入；heap path helpers、manifest I/O、package map merge 仍在 C；unit、FFI smoke、default/opt-in gate 尚未執行。

- [ ] Pkgmap path helpers 已加入同一 opt-in：`path_dirname`、`strip_extension` 具 length-query ABI；尚未執行 default/opt-in gate。


- [x] Pipeline configlink helpers：已 true-source 拆 `configlink_helpers.c`；pipeline 227、
  ONLY selector 空、make -Bn 已通過（2026-07-16）。


- [ ] Compile command tokenizer：已新增 Rust `split_command`、C malloc-compatible FFI 與 `CBM_USE_RUST_PIPELINE_SPLIT_COMMAND` wrapper；JSON/flags extraction 仍在 C；unit、FFI smoke、default/opt-in gate 尚未執行。


- [ ] Cross-repo JSON property helper：已新增 Rust copier 與 `CBM_USE_RUST_PIPELINE_CROSS_REPO_JSON` wrapper；SQLite lookup/edge writes 仍在 C；unit、FFI smoke、default/opt-in gate 尚未執行。


- [ ] Pipeline enrichment tokenizers：已新增 Rust splitter/tokenizer 與 `CBM_USE_RUST_PIPELINE_ENRICHMENT_TOKENS` wrapper；frequency collection、tags JSON injection 仍在 C；unit、FFI smoke、default/opt-in gate 尚未執行。


- [ ] Calls import JSON extractor：已新增 Rust length-query ABI 與 `CBM_USE_RUST_PIPELINE_CALLS_JSON` wrapper；registry/graph resolution 仍在 C；unit、FFI smoke、default/opt-in gate 尚未執行。


- [x] Envscan classifiers：已 true-source 拆 `envscan_classifiers.c`；pipeline 227、ONLY
  selector 空、make -Bn 已通過（2026-07-16）。


- [ ] LSP cross classifiers：已新增 Rust mode bitmask、language allowlist 與 `CBM_USE_RUST_PIPELINE_LSP_CLASSIFIERS` wrapper；LSP/file/graph runtime 仍在 C；unit、FFI smoke、default/opt-in gate 尚未執行。

- [ ] LSP label allowlist 已納入既有 classifier slice；尚未執行相關 gate。


- [ ] Semantic edges JSON readers：已新增 Rust scalar/array reader 與 `CBM_USE_RUST_PIPELINE_SEMANTIC_JSON` wrapper；embedding/LSH/graph diffusion 仍在 C；unit、FFI smoke、default/opt-in gate 尚未執行。

- [ ] Pkgmap path composition：`join_and_strip`、`build_entry_path` 已納入 Rust opt-in；尚未執行 gate。

- [ ] Configlink lowercase helper 已納入既有 stable FFI slice；尚未執行 gate。

- [ ] Cross-repo property formatter 已納入 stable FFI slice；尚未執行 gate。

- [ ] Compile command path helper 已納入 tokenizer stable FFI slice；尚未執行 gate。

- [ ] 2026-07-15：新增 pipeline infrascan `clean_json_brackets` Rust helper、C opt-in 分流、Rust unit 與 FFI smoke；default path 保持 C，待執行 default/opt-in gates。

- [x] 2026-07-15：新增 pipeline definitions JSON escape Rust helper、stable FFI、C opt-in 與 targeted gate；default path 保持 C。**2026-07-16 true-source 完成**（`definitions_json.c`、公開 bridge、ONLY selector、default/wrapper/direct FFI、pipeline 227、wrapper/direct production `--version`、selector 空與 `make -Bn` source-negative）。完整 `scripts/test.sh` 尚未重跑。

- [x] 2026-07-15：新增 pipeline similarity fingerprint properties Rust parser、stable FFI、C opt-in 與 targeted gate；default path 保持 C。**2026-07-16 true-source 完成**（`similarity_fp.c/.h`、公開 bridge、ONLY selector、default/wrapper/direct FFI、pipeline 227、wrapper/direct production `--version`、selector 空與 `make -Bn` source-negative）。完整 `scripts/test.sh` 尚未重跑。
- [x] 2026-07-17：完成 pipeline infrascan JSON cleaner true-source（`infrascan_json.c/.h`、公開 bridge、ONLY selector、default/wrapper/direct FFI、pipeline 各 227、wrapper/direct production `--version`、selector 空與 `make -Bn` source-negative）。完整 `scripts/test.sh` 尚未重跑。

- [x] 2026-07-15：新增 CLI archive tar/zip byte helper Rust slice、stable FFI、C opt-in 與 targeted gate；default path 保持 C。**2026-07-16 true-source 完成**（`archive_helpers.c`、cli 113、make -Bn）。

- [x] CLI archive byte helper slice 追加 zip `uint32` little-endian reader，納入同一 true-source CU。

- [x] 2026-07-15：新增 pipeline K8s basename/indent/key-value Rust helper、stable FFI、C opt-in 與 targeted gate；default path 保持 C。**2026-07-16 true-source 完成**（`k8s_text_helpers.c`、ONLY + make -Bn）。

- [x] 2026-07-15：新增 pipeline incremental recomputed-edge predicate Rust slice、stable FFI、C opt-in 與 targeted gate；default path 保持 C。**2026-07-16 true-source 完成**（獨立 CU、pipeline 227、make -Bn）。

- [x] 2026-07-15/16：pipeline parallel JSON escape/length 已有 stable FFI、C opt-in、獨立
  `parallel_json.c/.h` fallback、公開 bridge、ONLY selector 與 direct ABI；NULL length 為 0，
  NULL/short output 不寫入但回報所需長度。**2026-07-16 true-source 完成**：進度由
  **12/18 -> 13/18**；`rust-pipeline-parallel-json-optin-test` 與
  `rust-pipeline-parallel-json-only-test` 均通過，default/wrapper/direct FFI smoke 與 pipeline
  suite 各 `227 passed`，wrapper/direct production `--version` 成功，`_ONLY` source selector
  為空，且 `make -Bn` source-negative 不含 `src/pipeline/parallel_json.c`。

- [x] 2026-07-15：新增 discover language Objective-C/Magma/MATLAB marker Rust slice、stable FFI、C opt-in 與 targeted gate；default path 保持 C。**2026-07-16 true-source 完成**（`language_markers.c`、公開 bridge、ONLY selector、default/wrapper/direct FFI、discover 87、wrapper/direct production `--version`、selector 空與 `make -Bn` source-negative）。完整 `scripts/test.sh` 尚未重跑。

- [x] 2026-07-15：新增 pipeline incremental registry label Rust slice、stable FFI、C opt-in 與 targeted gate；default path 保持 C。**2026-07-16 true-source 完成**（獨立 CU、pipeline 227、make -Bn）。

- [x] 2026-07-15：新增 pipeline semantic Go suffix Rust slice、stable FFI、C opt-in 與 targeted gate；default path 保持 C。**2026-07-16 true-source 完成**（獨立 CU、pipeline 227、make -Bn）。

- [ ] 2026-07-15：新增 store architecture JSON property Rust slice、length/copy FFI、C opt-in 與 targeted gate；default path 保持 C，待驗證。
- [x] 2026-07-17 原預算外後續第 25 項 Discover filters true-source：C fallback 為
  `src/discover/filters.c`，只涵蓋 `cbm_should_skip_dir`、`cbm_has_ignored_suffix`、
  `cbm_should_skip_filename`、`cbm_matches_fast_pattern`；NULL 回 `false`，僅
  `CBM_MODE_FULL`（0）為 full、任何非零 mode 為 restricted，且維持 raw byte 精確、
  大小寫敏感比對。wrapper `CBM_USE_RUST_DISCOVER_FILTERS=1` 經既有 v1 ABI；direct
  `CBM_USE_RUST_DISCOVER_FILTERS_ONLY=1`／`discover-filters-only` 使用原已存在的 Rust
  direct ABI，`DISCOVER_FILTERS_SRCS =` 與 source-negative 已驗證。wrapper 啟用 Rust bridge
  時以條件編譯排除 C fallback filter tables，避免 `-Werror` unused-variable。已通過
  `cargo fmt --all -- --check`、`cargo test -p cbm-core discover --locked`（5 passed）、
  `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、
  `rust-discover-filters-optin-test`（default/wrapper FFI、Discover 各 89 passed、正式版
  `--version`）與 `rust-discover-filters-only-test`（default/direct FFI、direct Discover
  89 passed、正式版 `--version`、空白 selector、靜默 source-negative）。本項不改寫歷史
  `21/21`、第 22 項原預算外 true-source，或第 23 項 artifact-path hardening（非新
  true-source）的定位。
- [x] 2026-07-17 Discover path_join parity 修正：此為既有 true-source 的 superseding parity 與
  C/Rust regression 補強，不新增 true-source，且不改變 ABI、Cargo feature、C fallback 或
  direct selector；舊有「尚未驗證」歷史文字維持原樣。Rust path_join 在輸出寫入、截斷並將
  `\` 轉為 `/` 後，若開頭為 ASCII 小寫 drive、第二 byte 為 `:`，且第三 byte 為 `/`
  或字串在第二 byte 結束，會將 drive 大寫。故 `c:/tmp`／`c:\\tmp` 變 `C:/tmp`，
  bare `c:` 變 `C:`，`c:relative` 不變，截斷 `c:x` 至 `c:` 亦變 `C:`。已通過
  `cargo fmt --all -- --check`、`cargo test -p cbm-core discover --locked`（5 passed）、
  `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、
  `rust-discover-path-join-optin-test`（default/wrapper FFI、Discover 各 89 passed、wrapper
  正式版 `--version`）與 `rust-discover-path-join-only-test`（default/direct FFI、Discover
  各 89 passed、direct 正式版 `--version`、`DISCOVER_PATH_JOIN_SRCS =` 空白、靜默
  source-negative）。
- [x] 2026-07-17 原預算外後續第 26 項 Discover local_rel_path offset true-source：C fallback 為
  `src/discover/local_rel_path.c/.h`，`discover.c` 兩個巢狀 `.gitignore` callsite 由原
  static pointer helper 改為 offset projection。API
  `size_t cbm_discover_local_rel_path_offset(const char *rel_path, const char *local_prefix)`
  對 NULL rel/prefix、空 prefix、未匹配回 `0`；僅 byte-exact、大小寫敏感 prefix 且下一 byte
  為 `/` 時回 `prefix_len + 1`，不正規化、不配置、不修改、不保存指標。wrapper
  `CBM_USE_RUST_DISCOVER_LOCAL_REL_PATH=1` 經 v1 ABI；direct
  `CBM_USE_RUST_DISCOVER_LOCAL_REL_PATH_ONLY=1`／`discover-local-rel-path-only` 使用
  direct ABI，`DISCOVER_LOCAL_REL_PATH_SRCS =` 與靜默 source-negative 已驗證。已通過
  `cargo fmt --all -- --check`、`cargo test -p cbm-core local_rel_path --locked`（5 passed）、
  `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、
  `rust-discover-local-rel-path-optin-test`（default/wrapper FFI、Discover 各 90 passed、
  default/wrapper 正式版 `--version`）與 `rust-discover-local-rel-path-only-test`
  （default/direct FFI、Discover 各 90 passed、default/direct 正式版 `--version`、
  `DISCOVER_LOCAL_REL_PATH_SRCS =` 空白、靜默 source-negative）。本項不改寫既有
  `21/21`、第 22 項、第 23 項、第 25 項或 path_join parity 歷史。
- [x] 2026-07-17 Route-node classifiers 既有 true-source ABI/gate hardening：此項不是新
  true-source slice，不改變 `21/21`、第 22 項、第 23 項、第 25、26 項或 path_join parity
  歷史。新增 `src/pipeline/route_node_classifiers.h`，凍結既有 `hash_segment`／
  `broker_route` API：hash_segment 對 NULL、0、超過 12 拒絕，只允許小寫 ASCII 字母／數字，
  長度至少 3 含字母拒絕，短合法字母／數字與全數字可接受，並拒絕大寫、符號、NUL、非 ASCII；
  broker_route 對 NULL 回 `false`、大小寫與首 byte 精確匹配七個既有 prefix，
  `__route__GET__` 為負例。Make direct macro 修正為
  `CBM_USE_RUST_PIPELINE_ROUTE_NODE_CLASSIFIERS_ONLY`，FFI support 用
  `ROUTE_NODE_CLASSIFIERS_SRCS` selector，direct 排除 C fallback。已通過
  `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::route --locked`
  （13 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、
  `rust-pipeline-route-node-classifiers-optin-test`（default/wrapper FFI、route_canon
  pipeline 各 241 passed、正式版 `--version`）及
  `rust-pipeline-route-node-classifiers-only-test`（default/wrapper/direct FFI、route_canon
  pipeline 各 241 passed、正式版 `--version`、`ROUTE_NODE_CLASSIFIERS_SRCS =` 空白、靜默
  `make -Bn` source-negative）。
- [x] 2026-07-17 Discover language `.m` marker helpers 既有 true-source parity/ABI/gate hardening：
  此項不是新 true-source slice，歷史 `21/21` 不變。kind 2 凍結為 ASCII byte-only：NULL／
  invalid kind 行為不變；callable 名稱須至少含一個 `[A-Za-z]`，下一 byte 必須立刻為 `(`；
  `\xE9`、`"intrinsic ("`、空白及其他非 `(` 均為 false。C 不再用 locale-sensitive
  `isalpha`，已與 Rust `is_ascii_alphabetic` 收斂，C/Rust/FFI regressions 已補齊；兩個
  Make gate 的 default 階段增加 isolation production `--version`。已通過
  `cargo fmt --all -- --check`、`cargo test -p cbm-core discover --locked`（11 passed）、
  `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、
  `rust-discover-language-markers-optin-test`（default/wrapper FFI、Discover runner 90 passed、
  兩邊 production `--version`）與 `rust-discover-language-markers-only-test`（整體 exit 0、
  default/wrapper/direct FFI、Discover runner、production、空白
  `DISCOVER_LANGUAGE_MARKERS_SRCS =`、靜默 `make -Bn` source-negative）。
- [ ] 新 session 下一步：先做 `pipeline-json-prop` ABI 契約稽核，不可直接 gate；必須先凍結 C
  `bufsz=int` 與 Rust `usize`、NULL／`<= 0` buffer、長 key 64 byte 截斷差異的行為。
- [ ] LSP cross classifiers 是較乾淨的新 true-source 候選，但尚未開始；契約未凍結前不可推進。
> **2026-07-17 原預算外後續切片（第 22 項）**：原 `21/21` true-source 預算已完成且保持為歷史
> 結論；新增的 discover `cbm_language_name` 為獨立後續 slice，不改寫該計數。狹窄 C fallback 已
> 拆至 `src/discover/language_name.c/.h`，而 `src/discover/language.c` 仍負責語言偵測。公開 ABI
> `const char *cbm_language_name(CBMLanguage)` 回傳 static、borrowed、NUL 結尾、immutable 名稱，
> 呼叫端不得 `free`；invalid、`CBM_LANG_COUNT`、out-of-range 與未來 enum hole 均回
> `"Unknown"`。full-enum sweep 與 duplicate parity 已通過，`CFSCRIPT`（157）與 `CFML`（158）皆為
> `"CFML"`。wrapper `CBM_USE_RUST_DISCOVER_LANGUAGE_NAME=1` 經
> `cbm_rs_discover_language_name_v1(int)`，僅 `NULL` 回退 C；direct 使用
> `CBM_USE_RUST_DISCOVER_LANGUAGE_NAME_ONLY=1`／`discover-language-name-only`，
> `DISCOVER_LANGUAGE_NAME_SRCS =` 且 source-negative 排除 `language_name.c`。它與舊
> full-language direct 因同名 public ABI 互斥。已通過 fmt、3 個 Rust unit、clippy、
> `rust-discover-language-name-optin-test`（default/wrapper FFI、discover 87、version）、
> `rust-discover-language-name-only-test`（default/direct FFI、discover 87、version、selector/
> source-negative）及 `rust-discover-userconfig-language-name-only-test`（combo FFI、discover 87、
> version）；`scripts/test.sh` 未跑。
