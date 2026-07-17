# Rust 重構當前交接快照（2026-07-17）

## 2026-07-17 第 28 項 Pipeline split-command true-source

pipeline-split-command 已完成，true-source 累計由 **22 / 22 -> 23 / 23**。C fallback 已從
src/pipeline/pass_compile_commands.c 拆至 src/pipeline/split_command.c/.h；原檔仍保留
compile_commands parse、flag extraction、path/orchestration 與 legacy resolve_path C 實作，並未遷移
整個 pass。

公開 C ABI 不變：

    int cbm_split_command(const char *cmd, char **out, int max_out);

cmd == NULL、out == NULL 或 max_out <= 0 一律回傳 0；若呼叫端有提供 out，這些失敗路徑不得寫入
既有 sentinel。有效輸入採 first-NUL C-string 邊界、raw-byte 語意，不做 UTF-8 或 locale 轉換；
space/tab 分隔 token，單／雙引號會移除，未閉合引號從開啟處一路收集至 first-NUL。每個 token
最多保留 **4095** payload bytes，輸出皆有 first-NUL。函式只寫入 out[0..count)，不寫入
out[count] 或其後 slot 作 sentinel；count 內的非 NULL token 均以 C malloc 配置，呼叫端逐一
free()。

CBM_USE_RUST_PIPELINE_SPLIT_COMMAND=1 wrapper 保留既有 v1 行為並委派
cbm_rs_pipeline_split_command_v1；Rust v1 與 direct 都透過 C allocator 配置 token，確保
C free() ownership 不變。CBM_USE_RUST_PIPELINE_SPLIT_COMMAND_ONLY=1／Cargo
pipeline-split-command-only 時，Rust staticlib 匯出同名 direct cbm_split_command；OOM 行為未
凍結，仍是非保證的殘餘差異。

Make gate 為 rust-pipeline-split-command-optin-test 與
rust-pipeline-split-command-only-test。兩者覆蓋 isolated default/wrapper/direct 的 FFI、
pipeline runner **231 passed** 與 production --version。direct 確認
PIPELINE_SPLIT_COMMAND_SRCS =，make -Bn source-negative 不含
src/pipeline/split_command.c，並以正向 recipe 證據確認
src/pipeline/pass_compile_commands.c 仍編入產品；RUST_FFI_SUPPORT_SRCS 使用同一 selector。

**下一個 session 起手式：** 不要再修改 #28；先做新的候選 inventory。pipeline-complexity-json
延後，直到另行解決 C strtol 的 locale whitespace、overflow 與 NULL key 契約，再決定是否建立
true-source slice。

## 2026-07-17 第 27 項 Pipeline LSP cross classifiers true-source

本項是新的 true-source slice，累計進度為 **22 / 22**；既有 21 / 21 歷史、第 22 至第 26 項、
path_join、route-node classifiers、language_markers 與 JSON prop hardening 均不重算或改寫。
C fallback 是 `src/pipeline/lsp_cross_classifiers.c/.h`；只有三個 pure classifier 搬移，
`src/pipeline/pass_lsp_cross.c` 的檔案 I/O、arena、registry、LSP resolver、graph merge 與其餘
orchestration 均未搬移。

三個同名 C public ABI 為：

- `void cbm_pxc_ts_modes(CBMLanguage lang, const char *rel_path, bool *out_js, bool *out_jsx, bool *out_dts)`
- `bool cbm_pxc_has_cross_lsp(CBMLanguage lang)`
- `const char *cbm_pxc_map_label(const char *label)`

Rust direct-only symbols以 `c_int` 接收 `CBMLanguage`、以 C `bool` 處理 flags／predicate，並以 raw
pointer 處理 `rel_path` 與 `label`。FFI smoke 有
`_Static_assert(sizeof(CBMLanguage) == sizeof(int))` guard。`rel_path == NULL` 合法；三個 TS
output pointers 則沿用既有前置條件，必須非 NULL 且可寫，沒有 NULL output pointer 契約。TS mode
bit 為 JS `1`、JSX `2`、DTS `4`，只會組合為 `0..7`。

cross-LSP capability 支援 Go、C、C++、CUDA、Python、JavaScript、TypeScript、TSX、PHP、C#、
Java、Kotlin、Rust 共 13 種 language；未知或未支援 enum 回 false。C# 僅代表 prebuilt registry
eligibility，不保證 fallback resolver 會執行或產生 resolution。label 只接受 Class、Struct、
Interface、Enum、Type、Trait、Protocol、Function、Method 九個 raw-byte、大小寫敏感值；允許時
`cbm_pxc_map_label` 必須回傳同一個 borrowed input pointer，拒絕或 NULL 回 NULL。所有字串均以
first-NUL C-string 語意判定，Rust 不配置、保存、釋放或以 Rust-owned pointer 取代 caller label。

`CBM_USE_RUST_PIPELINE_LSP_CLASSIFIERS=1` wrapper 保留既有三個 v1 ABI；
`CBM_USE_RUST_PIPELINE_LSP_CLASSIFIERS_ONLY=1`／Cargo
`pipeline-lsp-classifiers-only` 時 Rust staticlib 匯出同名 direct ABI。Make selector
`PIPELINE_LSP_CLASSIFIERS_SRCS` 在 ONLY 為空，僅排除新 fallback CU，`pass_lsp_cross.c` 繼續編入。
`RUST_FFI_SUPPORT_SRCS` 已加入該 selector，確保 default/wrapper FFI 真正連入 C public bridge，
direct 則由 Rust symbols 提供連結。

已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline_lsp_cross --locked`
（**3 passed**）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、
`make -j1 -f Makefile.cbm rust-pipeline-lsp-classifiers-optin-test` 與
`make -j1 -f Makefile.cbm rust-pipeline-lsp-classifiers-only-test`。四個 gate leg 的 FFI 與
pipeline runner 均通過，pipeline 均為 **230 passed**，production `--version` 均成功；direct 另
確認 `PIPELINE_LSP_CLASSIFIERS_SRCS =` 與靜默成功的 `make -Bn` source-negative。

**目前停點與下一步：**建立下輪候選 inventory；尚未選定或開始下一個候選。

## 2026-07-17 Pipeline JSON property 既有 true-source ABI/gate hardening

此項只補強既有 Pipeline JSON property true-source 的 ABI、FFI regression 與 Make gate，**不是**
新的 true-source slice；歷史計數維持 **21 / 21**，不重算既有第 22 至第 26 項、path_join、
route-node classifiers 或 language_markers。

公開 C ABI 為
`bool cbm_pipeline_extract_json_prop(const char *json, const char *key, char *buf, int bufsz)`；
既有 v1 ABI 回傳 `int`，direct ABI 回傳 `bool`。Rust FFI boundary 以有號 `c_int` 傳遞
`bufsz`，不得先轉為 `usize`。`json`、`key` 或 `buf` 為 NULL，或 `bufsz <= 0` 時一律失敗且
不寫入 buffer；空 key 可成功，空 value 失敗且 buffer 不變；key 最多 59 bytes，60 bytes 以上
拒絕且 buffer 不變。既有 raw scanner 與 first-NUL C-string 截斷語意不變，Rust 不保存 caller
buffer 或借用指標。

`RUST_FFI_SUPPORT_SRCS` 已加入 `$(PIPELINE_JSON_PROP_SRCS)`，所以 default/wrapper FFI 會真正
連入 C fallback public bridge；direct selector 為空時則由 Rust direct public symbol 提供同名 ABI。
兩個 JSON prop gate 的 default leg 都新增隔離 production `cbm --version`，與既有
default/wrapper/direct FFI 及 pipeline runner 一併執行。

已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::route --locked`
（**14 passed**）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、
`make -j1 -f Makefile.cbm rust-pipeline-json-prop-optin-test` 與
`make -j1 -f Makefile.cbm rust-pipeline-json-prop-only-test`。兩個 Make gate 合計覆蓋
default/wrapper/direct FFI、pipeline runner 各 **229 passed** 與全部模式 production `--version`；
ONLY 另確認 `PIPELINE_JSON_PROP_SRCS =` 及靜默成功的 `make -Bn` source-negative。

> 此處當時的 LSP cross classifiers 預稽核已完成；最新停點見文件頂端。

> **這是目前唯一的交接權威快照。** 接手前先讀本文件與根目錄
> [Handoff.md](../Handoff.md)、[Rust-Refactor.md](../Rust-Refactor.md)、[Tasks.md](../Tasks.md)、
> [rust/CBM_FFI.md](../rust/CBM_FFI.md)。

產品預設仍是 **C11**。Rust 只透過明確 `CBM_USE_RUST_*` opt-in／`*_ONLY` 接管已凍結契約的
pure helper。不得宣稱全庫 Rust default 或 Phase 5 RC。

## 2026-07-17 Discover language_markers current-revision parity／ABI／gate hardening

此項是既有 language_markers true-source 的契約、ABI 與 gate 補強，**不是**新的 true-source
slice；歷史總數維持 **21 / 21**，既有第 22 至第 26 項、path_join 與 route-node classifiers
紀錄均保留。

凍結契約為：`cbm_discover_language_marker` 對 NULL `line` 或無效 `kind` 一律回 `0`。kind `2`
以 NUL 結尾 raw bytes 判定；只有 `intrinsic ` 或 `procedure ` 後緊接至少一個 ASCII
`[A-Za-z]` 名稱字元，且名稱後立刻為 `(`，才回真。`\xE9`、空名稱、名稱內空白、缺少 `(`，
以及內嵌 NUL 截斷後的尾端內容，均為 false。此 helper 不配置記憶體、不保存借用指標，也不
改變所有權；`.m` 檔案讀取、marker 優先順序、語言選擇與 Discover orchestration 仍由 C 負責。

C fallback 已移除 locale-sensitive `isalpha` 語意，改與 Rust ASCII 判定對齊。wrapper 保留
既有 v1 ABI；public bridge、direct Rust staticlib ABI 與 v1 wrapper 均補強 regression。Make
gate 也補足 default production coverage。

已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core discover --locked`
（**11 passed**）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、
`make -j1 -f Makefile.cbm rust-discover-language-markers-optin-test` 與
`make -j1 -f Makefile.cbm rust-discover-language-markers-only-test`。兩個 Make gate 均覆蓋 FFI
regression、Discover runner **90 passed** 與正式版 `--version`；ONLY 另確認
`DISCOVER_LANGUAGE_MARKERS_SRCS =` 及靜默成功的 `make -Bn` source-negative。

> 此處當時的 JSON prop 下一步已完成；最新停點見文件頂端。

## 本目標（multi-agent，limit 21）已完成 true-source 切片

下列切片皆具備：獨立 C fallback CU、公開 bridge、direct 同名 ABI、ONLY 時 `*_SRCS` 為空、
`make -Bn` production recipe 不含 fallback `.c`、default/wrapper/direct 實際執行 FFI + suite +
production `--version`。Gate 日誌：implementer scratch `gates/<slice>/`（含
`bn-source-negative.log` 全文 transcript）。

| # | 切片 | Fallback CU | Bridge | Suite | 備註 |
|---|------|-------------|--------|-------|------|
| 1 | Discover path join | `src/discover/path_join.c` | `cbm_discover_path_join` | discover **87** | 參數順序 (base,rel,out,sz) |
| 2 | Incremental recomputed-edge | `src/pipeline/incremental_edge_type.c` | `cbm_pipeline_incremental_edge_type_is_recomputed` | pipeline **227** | 四種 edge type |
| 3 | Semantic fp_ends_with | `src/pipeline/semantic_fp_suffix.c` | `cbm_pipeline_semantic_fp_ends_with` | pipeline **227** | empty suffix → true |
| 4 | Pipeline JSON prop | `src/pipeline/json_prop.c` | `cbm_pipeline_extract_json_prop` | pipeline **227** | 簡易 "k":"v" 擷取 |
| 5 | Incremental registry label | `src/pipeline/incremental_registry_label.c` | `cbm_pipeline_incremental_label_is_registry_symbol` | pipeline **227** | 10 labels |
| 6 | MCP edge type valid | `src/mcp/edge_type_valid.c` | `cbm_mcp_edge_type_valid` | mcp **136** | A–Z/_ ≤64；空字串 true |
| 7 | Githistory trackable file | `src/pipeline/trackable_file.c` | `cbm_is_trackable_file` | pipeline **227** | `PIPELINE_TRACKABLE_FILE_SRCS` |
| 8 | CLI archive helpers | `src/cli/archive_helpers.c` | `cbm_cli_is_tar_end_of_archive` / `cbm_cli_zip_read_u{16,32}` | cli **113** | tar 512 全零 + LE 讀取 |
| 9 | K8s text helpers | `src/pipeline/k8s_text_helpers.c` | `cbm_pipeline_k8s_basename` / `_indent` / `_split_kv` | pipeline **227** | 與 file classifiers 分離 |
| 10 | SimHash jaccard/hex | `src/simhash/minhash_jaccard.c` | `cbm_minhash_jaccard` / `to_hex` / `from_hex` | simhash **24** | compute/LSH 仍在 minhash.c |
| 11 | Envscan classifiers | `src/pipeline/envscan_classifiers.c` | `cbm_pipeline_envscan_is_*` / `detect_file_type` | pipeline **227** | pass_envscan 本體仍連結 |
| 12 | Configlink helpers | `src/pipeline/configlink_helpers.c` | `cbm_pipeline_configlink_*` | pipeline **227** | pass_configlink 本體仍連結 |
| 13 | Pipeline parallel JSON escape/length | `src/pipeline/parallel_json.c` | `cbm_pipeline_parallel_json_escape_char` / `_escaped_len` | pipeline **227** | raw control byte -> 空白 |
| 14 | Pipeline definitions JSON escape/length | `src/pipeline/definitions_json.c` | `cbm_pipeline_definitions_json_escape_char` / `_escaped_len` | pipeline **227** | raw control byte -> 空白 |
| 15 | Discover language `.m` marker helpers | `src/discover/language_markers.c` | `cbm_discover_language_marker` | discover **87** | Objective-C/Magma/MATLAB |
| 16 | Pipeline similarity fingerprint parser | `src/pipeline/similarity_fp.c` | `cbm_pipeline_similarity_parse_fp` | pipeline **227** | `"fp":"..."` -> `uint32_t[64]` |
| 17 | Pipeline infrascan JSON cleaner | `src/pipeline/infrascan_json.c` | `cbm_clean_json_brackets` | pipeline **227** | JSON bracket array normalization |
| 18 | Pipeline test-node is_test classifier | `src/pipeline/test_node_is_test.c` | `cbm_pipeline_test_node_is_test` | pipeline **228** | NULL -> false；嚴格連續 `"is_test":true` |

本 revision 追加驗證：

| 19 | Semantic camel-break classifier | `src/semantic/camel_break.c/.h` | `cbm_semantic_is_camel_break` | pipeline **228** | ASCII camel-boundary predicate |
| 20 | Semantic token delimiter | `src/semantic/token_delim.c/.h` | `cbm_semantic_is_token_delim` | pipeline **228** | scalar byte delimiter；camel-break 獨立 |
| 21 | Pipeline registry test-QN classifier | `src/pipeline/registry_test_qn.c/.h` | `cbm_pipeline_registry_is_test_qn` | registry **54** | raw byte substring predicate |

~~~sh
cargo test -p cbm-core --locked          # 307 passed（cargo-suite-final.log）
cargo clippy --all-targets --all-features --locked -- -D warnings  # implementer/cargo-clippy.log
cargo fmt --all -- --check                                        # implementer/cargo-fmt.log
# 十九個 ONLY selector 空；make -Bn source-negative 皆 BN_RESULT=OK
~~~

Pipeline parallel JSON 已依序通過
`make -j1 -f Makefile.cbm rust-pipeline-parallel-json-optin-test` 與
`make -j1 -f Makefile.cbm rust-pipeline-parallel-json-only-test`：涵蓋
default/wrapper/direct FFI smoke、pipeline suite 各 **227**、wrapper/direct production
`--version`、ONLY selector 空值，以及 `make -Bn` production source-negative 不含
`src/pipeline/parallel_json.c`。

Pipeline definitions JSON 已依序通過
`make -j1 -f Makefile.cbm rust-pipeline-definitions-json-optin-test` 與
`make -j1 -f Makefile.cbm rust-pipeline-definitions-json-only-test`：涵蓋
default/wrapper/direct FFI smoke、pipeline suite 各 **227**、wrapper/direct production
`--version`、ONLY selector 空值，以及 `make -Bn` production source-negative 不含
`src/pipeline/definitions_json.c`。

Discover language `.m` marker helpers 已依序通過
`make -j1 -f Makefile.cbm rust-discover-language-markers-optin-test` 與
`make -j1 -f Makefile.cbm rust-discover-language-markers-only-test`：涵蓋
default/wrapper/direct FFI smoke、discover suite 各 **87**、wrapper/direct production
`--version`、ONLY selector 空值，以及 `make -Bn` production source-negative 不含
`src/discover/language_markers.c`。

Pipeline similarity fingerprint parser 已依序通過
`make -j1 -f Makefile.cbm rust-pipeline-similarity-fp-optin-test` 與
`make -j1 -f Makefile.cbm rust-pipeline-similarity-fp-only-test`：涵蓋
default/wrapper/direct FFI smoke、pipeline suite 各 **227**、wrapper/direct production
`--version`、ONLY selector 空值，以及 `make -Bn` production source-negative 不含
`src/pipeline/similarity_fp.c`。

Pipeline infrascan JSON cleaner 已依序通過
`make -j1 -f Makefile.cbm rust-pipeline-infrascan-json-optin-test` 與
`make -j1 -f Makefile.cbm rust-pipeline-infrascan-json-only-test`：涵蓋
default/wrapper/direct FFI smoke、pipeline suite 各 **227**、wrapper/direct production
`--version`、ONLY selector 空值，以及 `make -Bn` production source-negative 不含
`src/pipeline/infrascan_json.c`。

Pipeline test-node is_test classifier 已依序通過
`make -j1 -f Makefile.cbm rust-pipeline-test-node-is-test-optin-test` 與
`make -j1 -f Makefile.cbm rust-pipeline-test-node-is-test-only-test`：C fallback 為
`src/pipeline/test_node_is_test.c`／`.h`，公開 bridge 與 direct ABI 均為
`cbm_pipeline_test_node_is_test`，wrapper v1 ABI 為
`cbm_rs_pipeline_test_node_is_test_v1`，Cargo feature 為
`pipeline-test-node-is-test-only`。default/wrapper/direct FFI smoke、pipeline suite 各
**228**、wrapper/direct production `--version`、ONLY selector 空值，以及 `make -Bn`
production source-negative 不含 `src/pipeline/test_node_is_test.c` 均已通過；NULL 回
false，且只匹配嚴格連續的 `"is_test":true`。

平行 multi-agent audit：envscan／configlink／next-pure-helpers explore agents；
implementer audits under scratch `audits/*`。

Semantic camel-break classifier 已依序通過
`make -j1 -f Makefile.cbm rust-semantic-camel-break-optin-test` 與
`make -j1 -f Makefile.cbm rust-semantic-camel-break-only-test`：default C fallback、既有
`CBM_USE_RUST_SEMANTIC_TOKEN_CLASSIFIERS=1` wrapper，以及 narrow
`CBM_USE_RUST_SEMANTIC_CAMEL_BREAK_ONLY=1` direct 均實際執行 FFI 與 pipeline suite，各
**228 passed**；wrapper/direct production `--version` 亦成功。C fallback 為
`src/semantic/camel_break.c/.h`，public bridge 與 Rust direct ABI 同名
`cbm_semantic_is_camel_break`；Cargo feature 為 `semantic-camel-break-only`。direct 的
`SEMANTIC_CAMEL_BREAK_SRCS =`，且 `make -Bn` source-negative 證明 production recipe 未連結
`src/semantic/camel_break.c`。契約為 NULL 或 `index <= 0` 回傳 false；非 NULL 正 index 時，
`name` 必須指向 NUL 結尾名稱的有效非 NUL byte，僅 ASCII 前小寫、後大寫的邊界回傳 true。無跨
ABI 配置；tokenization、delimiter、semantic embedding、所有權與 pipeline side effects 仍由 C 管理。

Semantic token delimiter 已依序通過
`make -j1 -f Makefile.cbm rust-semantic-token-delim-optin-test` 與
`make -j1 -f Makefile.cbm rust-semantic-token-delim-only-test`：default C fallback、既有
`CBM_USE_RUST_SEMANTIC_TOKEN_CLASSIFIERS=1` wrapper 與 narrow
`CBM_USE_RUST_SEMANTIC_TOKEN_DELIM_ONLY=1` direct 均實際執行 FFI 與 pipeline suite，各
**228 passed**；三路 production `--version` 亦成功。C fallback 為
`src/semantic/token_delim.c/.h`，public bridge 與 Rust direct ABI 同名
`cbm_semantic_is_token_delim(unsigned char byte)`；Cargo feature 為
`semantic-token-delim-only`。僅 `.`、`/`、`_`、`-`、空白、`(`、`)`、`,`、`:` 回傳 true，
其他 byte（含 `0x80` 與 `0xff`）回傳 false；無 nullable pointer、buffer 或跨 ABI 配置。
direct 的 `SEMANTIC_TOKEN_DELIM_SRCS =`，且 `make -Bn` source-negative 證明 production
recipe 未連結 `src/semantic/token_delim.c`。camel-break 的 C/Rust direct 狀態獨立，delimiter
ONLY 不替換 `camel_break.c`；tokenization、semantic orchestration、embedding 與 ownership
持續由 C 管理。Rust fmt、token-classifier unit **2 passed**、clippy 均通過；完整
`scripts/test.sh` 未重跑。

Pipeline registry test-QN classifier 已依序通過
`make -j1 -f Makefile.cbm rust-pipeline-registry-test-qn-optin-test` 與
`make -j1 -f Makefile.cbm rust-pipeline-registry-test-qn-only-test`：default C fallback、
`CBM_USE_RUST_PIPELINE_REGISTRY_TEST_QN=1` wrapper 與 narrow
`CBM_USE_RUST_PIPELINE_REGISTRY_TEST_QN_ONLY=1` direct 均實際執行 FFI 與 registry suite，各
**54 passed**；三路 production `--version` 亦成功。C fallback 為
`src/pipeline/registry_test_qn.c/.h`，public bridge 與 Rust direct ABI 同名
`bool cbm_pipeline_registry_is_test_qn(const char *qn)`；Cargo feature 為
`pipeline-registry-test-qn-only`。NULL 與空字串回 false；非 NULL 必須是有效 NUL 結尾 C string，
以大小寫敏感 raw `strstr` needles `Test`、`test`、`Mock`、`mock`、`Stub`、`stub`、`Fake`、
`fake`、`Fixture`、`spec` 判定，故 `contest` 因 raw substring 回 true，non-UTF8 byte 亦維持
raw-safe 行為。wrapper 透過既有 v1 ABI，非 `0`／`1` 回傳會回退 C；direct 的
`PIPELINE_REGISTRY_TEST_QN_SRCS =`，且 `make -Bn` source-negative 證明 production recipe
未連結 `src/pipeline/registry_test_qn.c`。Rust fmt、`cargo test -p cbm-core pipeline::registry
--locked` **4 passed**、clippy 均通過；完整 `scripts/test.sh` 未重跑。此切片只切換 test-QN
score classifier；registry handles、cache、resolution 與 graph 仍由 C 管理，並不代表
`CBM_USE_RUST_PIPELINE_REGISTRY` 整體 registry 已替換。

LSP language enum parity 修正（不計入 true-source 切片）：Rust `LANG_C` 更正為 `14`，Bash
`15` 為 false；Rust unit **3 passed**，wrapper FFI 與 pipeline **228 passed**。此修正沒有
direct source selector，不增加 true-source 計數。

本目標完成 **21 / 21** 預算；無剩餘切片。

## 原預算外後續：第 22 個 true-source（不改變原 21 / 21 歷史）

Discover `cbm_language_name` 已完成狹窄 true-source replacement。C fallback 從
`src/discover/language.c` 拆至 `src/discover/language_name.c/.h`，而 `language.c` 保留其餘
語言偵測邏輯。公開 ABI 為 `const char *cbm_language_name(CBMLanguage)`；回傳值為 borrowed 的
static、NUL 結尾、immutable 字串，呼叫端不得 `free`。無效值、`COUNT`、out-of-range 與未來 enum
hole 一律回傳 `"Unknown"`。全 enum sweep 已覆蓋，且 `CBM_LANG_CFSCRIPT=157` 與
`CBM_LANG_CFML=158` 均維持回傳 `"CFML"`。

`CBM_USE_RUST_DISCOVER_LANGUAGE_NAME=1` wrapper 透過 v1 ABI
`cbm_rs_discover_language_name_v1(int)` 呼叫 Rust，僅在 Rust 回傳 NULL 時回退 C。narrow direct
模式為 `CBM_USE_RUST_DISCOVER_LANGUAGE_NAME_ONLY=1`，Cargo feature 為
`discover-language-name-only`，其 `DISCOVER_LANGUAGE_NAME_SRCS =`，且 source-negative 證明
production recipe 排除 `src/discover/language_name.c`。它與 legacy 完整 language direct
`CBM_USE_RUST_DISCOVER_LANGUAGE_ONLY=1` 因同名公開 ABI 互斥。`rust-discover-language-name-optin-test`
已通過 default/wrapper 的 FFI、discover **87 passed** 與 production `--version`；
`rust-discover-language-name-only-test` 已通過 default/direct 的 FFI、discover **87 passed**、
production `--version`、empty selector 與 source-negative；
`rust-discover-userconfig-language-name-only-test` 已通過 userconfig combo 的 FFI、discover
**87 passed** 與 production `--version`。Rust fmt、language-name unit **3 passed**、clippy 均成功；
完整 `scripts/test.sh` 未跑。

## 原預算外後續：第 23 項 artifact-path parity/gate hardening（非新 true-source）

此項是既有 artifact-path ABI 的 parity 與 gate 補強，不新增 true-source，亦不改變原始
**21 / 21** 與第 22 項原預算外 true-source 的歷史。新增 public `src/pipeline/artifact_path.h`
ABI：`bool cbm_pipeline_artifact_path(char *buf, size_t bufsz, const char *repo_path, const char *name)`。
`buf` 由呼叫端擁有；`repo_path` 與 `name` 為 borrowed、NUL 結尾 raw-byte C strings，不可與輸出
buffer overlap，亦不採 UTF-8 語意。C fallback 的 `snprintf` 行為保留：當 `bufsz > 0` 而輸出不足時，
仍寫入 `min(full_len, bufsz - 1)` bytes prefix 與 NUL，public `bool` 回 false；NULL 或 zero-capacity
失敗時不寫入。

`CBM_USE_RUST_PIPELINE_ARTIFACT_PATH=1` wrapper 經 v1 ABI
`cbm_rs_pipeline_artifact_path_v1` 呼叫 Rust。v1 成功時回完整所需長度；short 或 invalid 時回
`SIZE_MAX`，但 short 仍必須寫入 prefix 與 NUL。narrow direct 模式為
`CBM_USE_RUST_PIPELINE_ARTIFACT_PATH_ONLY=1`，Cargo feature 為
`pipeline-artifact-path-only`；Rust staticlib 直接匯出同名 public `bool` ABI，並維持 short 時
prefix + false 的 C 可觀察行為。default/wrapper 會編入 C bridge，direct 的
`ARTIFACT_PATH_SRCS =`，runner/production staticlib 選擇也已納入 `_ONLY`；direct production
source-negative 證明不連結 fallback C source。

已通過 Rust fmt、artifact unit **4 passed** 與 clippy；
`rust-pipeline-artifact-path-optin-test` 已通過 default/wrapper FFI、artifact suite **10 passed** 與
production `--version`；最終重跑的 `rust-pipeline-artifact-path-only-test` 已通過 default/direct
FFI、artifact suite **10 passed**、production `--version`、`ARTIFACT_PATH_SRCS =` 與 `make -Bn`
source-negative。完整 `scripts/test.sh` 未跑。

## 原預算外後續：第 24 項 AST profile classifiers true-source

AST profile classifiers 已完成後續 true-source slice；此項不改寫原始 **21 / 21** 歷史，亦不改變
第 22 項原預算外 true-source 或第 23 項既有 artifact-path parity/gate hardening（非新
true-source）的定位。C fallback 已拆至 `src/semantic/ast_profile_classifiers.c/.h`；
`src/semantic/ast_profile.c` 保留 Tree-sitter traversal 與 codec。

wrapper `CBM_USE_RUST_AST_PROFILE_CLASSIFIERS=1` 經 C bridge 呼叫既有 v1 ABI。narrow direct 模式為
`CBM_USE_RUST_AST_PROFILE_CLASSIFIERS_ONLY=1`，Cargo feature 為
`ast-profile-classifiers-only`；direct 排除 C fallback，Rust staticlib 匯出同名
`cbm_ast_profile_kind_flags`、`cbm_ast_profile_halstead_insert` 與
`cbm_ast_profile_is_param_name`。契約涵蓋 kind flags、固定 512 槽 Halstead insert 與 parameter-name
比對；防禦性 NULL／無效輸入行為與 Rust ABI 對齊。

已通過 `cargo fmt --all -- --check`、
`cargo test -p cbm-core ast_profile_classifiers --locked`（**3 passed**）與
`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`。
`rust-ast-profile-classifiers-optin-test` 已通過 default/wrapper FFI、pipeline **228 passed** 與
production `--version`；`rust-ast-profile-classifiers-only-test` 已通過 default/direct 相同檢查、
`AST_PROFILE_CLASSIFIERS_SRCS =` 與靜默成功的 `make -Bn` source-negative。完整
`scripts/test.sh` 未跑。

## 原預算外後續：第 25 項 Discover filters true-source

Discover filters 已完成後續 true-source slice；此項不改寫原始 **21 / 21** 歷史，亦不改變第 22 項
原預算外 true-source 與第 23 項既有 artifact-path parity/gate hardening（非新 true-source）的
定位。C fallback 為 `src/discover/filters.c`，只承接四個公開 predicate：
`cbm_should_skip_dir`、`cbm_has_ignored_suffix`、`cbm_should_skip_filename` 與
`cbm_matches_fast_pattern`；walk、ignore 組態與其餘 discover orchestration 仍由 C 管理。

wrapper 為 `CBM_USE_RUST_DISCOVER_FILTERS=1`，經 C bridge 呼叫既有 v1 ABI；direct 為
`CBM_USE_RUST_DISCOVER_FILTERS_ONLY=1`，Cargo feature 為 `discover-filters-only`。Rust
direct ABI 原已存在，本項不宣稱新寫 ABI；direct 時 `DISCOVER_FILTERS_SRCS =`，且
`make -Bn` source-negative 已靜默證明 production recipe 不連結 C fallback。四個 predicate
的契約均為 NULL 回 `false`、只有 `CBM_MODE_FULL`（0）是 full，任何非零 mode 均視為
restricted，並採 raw byte 精確、大小寫敏感比對。wrapper 啟用 Rust bridge 時，C fallback 的
filter tables 以條件編譯排除，避免 `-Werror` 的 unused-variable 失敗。

已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core discover --locked`
（**5 passed**）與 `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`。
`make -j1 -f Makefile.cbm rust-discover-filters-optin-test` 已通過 default/wrapper FFI、
Discover suite 各 **89 passed** 與正式版 `--version`；
`make -j1 -f Makefile.cbm rust-discover-filters-only-test` 已通過 default/direct FFI、direct
Discover **89 passed**、正式版 `--version`、空白 `DISCOVER_FILTERS_SRCS =` 與靜默
source-negative。

## 2026-07-17 Discover path_join parity 修正（既有 true-source）

此項是既有 Discover path_join true-source 的 parity 修正與 C/Rust regression 補強，不新增
true-source，亦不改變既有公開 ABI、Cargo feature、C fallback 或 direct selector。本段為
2026-07-17 的 superseding parity 紀錄；較早的歷史「尚未驗證」文字維持原樣。

Rust path_join 完成輸出寫入、截斷，以及將 `\` 轉為 `/` 後，若輸出開頭是 ASCII 小寫 drive、
第二 byte 為 `:`，且第三 byte 為 `/` 或字串在第二 byte 結束，便將 drive 字母轉為大寫。因此
`c:/tmp` 與 `c:\\tmp` 均為 `C:/tmp`，bare `c:` 為 `C:`；`c:relative` 不變，且
`c:x` 截斷成 `c:` 時也會變為 `C:`。

已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core discover --locked`
（**5 passed**）與 `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`。
`make -j1 -f Makefile.cbm rust-discover-path-join-optin-test` 已通過 default/wrapper FFI、
Discover suite 各 **89 passed** 與 wrapper 正式版 `--version`；
`make -j1 -f Makefile.cbm rust-discover-path-join-only-test` 已通過 default/direct FFI、
Discover suite 各 **89 passed**、direct 正式版 `--version`、空白
`DISCOVER_PATH_JOIN_SRCS =` 與靜默 source-negative。

## 原預算外後續：第 26 項 Discover local_rel_path offset true-source

Discover local_rel_path offset 已完成後續 true-source slice；此項不改寫原始 **21 / 21** 歷史，亦不
改變第 22 項原預算外 true-source、第 23 項非新增 true-source、以及第 25 項與 path_join parity
的既有定位。C fallback 為 `src/discover/local_rel_path.c/.h`；`src/discover/discover.c` 兩個巢狀
`.gitignore` callsite 已由原本的 static pointer helper 改用 offset projection。

公開 API 為
`size_t cbm_discover_local_rel_path_offset(const char *rel_path, const char *local_prefix)`。NULL
`rel_path`／`local_prefix`、空 prefix 或未匹配均回 `0`；只有 raw byte 精確、大小寫敏感的
prefix，且下一 byte 為 `/`，才回 `prefix_len + 1`。此 helper 不做正規化、配置、修改或保存
指標。

wrapper 為 `CBM_USE_RUST_DISCOVER_LOCAL_REL_PATH=1`，透過 v1 ABI；direct 為
`CBM_USE_RUST_DISCOVER_LOCAL_REL_PATH_ONLY=1`，Cargo feature 為
`discover-local-rel-path-only`，由 direct ABI 接管。direct 的
`DISCOVER_LOCAL_REL_PATH_SRCS =`，且 production `make -Bn` source-negative 已靜默通過。

已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core local_rel_path --locked`
（**5 passed**）及 `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`。
`make -j1 -f Makefile.cbm rust-discover-local-rel-path-optin-test` 已通過 default/wrapper FFI、
Discover suite 各 **90 passed**、default/wrapper 正式版 `--version`；
`make -j1 -f Makefile.cbm rust-discover-local-rel-path-only-test` 已通過 default/direct FFI、
Discover suite 各 **90 passed**、default/direct 正式版 `--version`、空白
`DISCOVER_LOCAL_REL_PATH_SRCS =` 與靜默 source-negative。

## 2026-07-17 Route-node classifiers 既有 true-source ABI/gate hardening

此項是既有 route-node classifiers true-source 的 ABI 與 gate hardening，不是新的 true-source
slice；原始 **21 / 21**、第 22 項原預算外、第 23 項非新增 true-source，以及第 25、26 項與
path_join parity 的歷史定位均不變。新增 `src/pipeline/route_node_classifiers.h`，凍結
`hash_segment` 與 `broker_route` 兩個既有 classifier API 的契約，並不重新分類既有 C/Rust
演算法為新實作。

`hash_segment` 對 NULL、長度 0 或大於 12 一律拒絕；只允許小寫 ASCII 字母或數字，長度至少 3
且含字母時拒絕，短的合法字母／數字與任意長度的全數字可接受；大寫、符號、NUL 與非 ASCII 均
拒絕。`broker_route` 對 NULL 回 `false`，以大小寫敏感且必須從首 byte 開始的方式比對七個既有
prefix；`__route__GET__` 是負例。

Make 修正 direct macro 為 `CBM_USE_RUST_PIPELINE_ROUTE_NODE_CLASSIFIERS_ONLY`；FFI support
改用 `ROUTE_NODE_CLASSIFIERS_SRCS` selector，direct 會排除既有 C fallback。

已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::route --locked`
（**13 passed**）及 `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`。
`make -j1 -f Makefile.cbm rust-pipeline-route-node-classifiers-optin-test` 已通過
default/wrapper FFI、route_canon pipeline 各 **241 passed** 與正式版 `--version`；
`make -j1 -f Makefile.cbm rust-pipeline-route-node-classifiers-only-test` 已通過
default/wrapper/direct FFI、route_canon pipeline 各 **241 passed**、正式版 `--version`、direct
selector 顯示空白 `ROUTE_NODE_CLASSIFIERS_SRCS =`，以及靜默成功的 `make -Bn`
source-negative。

## 明確非宣稱

- 產品 default 未切到 Rust；Phase 5 未完成。
- 完整 `scripts/test.sh`、`rust-all-optin-test` **尚未**本 revision 重跑。
- Store/SQLite、Cypher engine、Tree-sitter/LSP runtime、MCP server lifecycle 仍在 C。
- MCP `MCP_CODEC_ONLY` 仍不排除整個 `mcp.c`；edge-type 只替換小 CU。
- `pass_githistory.c`／`pass_k8s.c`／`cli.c` 本體仍連結（orchestration 未遷移）。

## 下一批 EXTRACTABLE 候選（audit 排名）

1. 其他 wrapper-only pure helpers（parallel audit 排名；禁止在未拆 CU 前把 macro wrapper 寫成完成）

## 接手前步驟

1. 讀 AGENTS.md + 本快照 + 四份根目錄／rust 文件  
2. `git status --short` — **保留** dirty/untracked  
3. 確認無殘留 clean-c make/cargo  
4. clean-c target 一律 `make -j1` 序列  
5. 文件只記已執行 gate

## 已有歷史 true-source（改動後必須重跑）

discover string helpers、watcher poll interval、SvelteKit server method、Git path absolute、
Git JSON escaped length、Git trim newlines、log copy path、Pipeline URL path、
SvelteKit file kind、Discover trailing separator，以及上表各項。
