# C/Rust FFI 邊界

## 目前停點（2026-07-19）— 先讀

> 完整交接以 `docs/rust-refactor-current-handoff.md` 為準。本檔是 **ABI／ownership 權威**。
> true-source 累計 **48 / 48**；不得重跑 #29–#53；#54 為進行中且尚未 true-source；
> `pipeline-complexity-json` 延後。

### #29–#53 true-source 速查

| # | Public ABI | Fallback CU | Direct feature | Ownership |
|---|------------|-------------|----------------|-----------|
| 29 | `char *cbm_pipeline_calls_extract_local_name` | `src/pipeline/calls_json.c` | `pipeline-calls-json-only` | C malloc／caller free |
| 30 | `char *cbm_pipeline_usages_extract_local_name` | `src/pipeline/usages_json.c` | `pipeline-usages-json-only` | C malloc／caller free |
| 31 | `int cbm_mcp_search_mode` | `src/mcp/search_mode.c` | `mcp-search-mode-only` | pure int |
| 32 | `int cbm_mcp_index_mode` | `src/mcp/index_mode.c` | `mcp-index-mode-only` | pure int |
| 33 | `bool cbm_mcp_search_path_arg_valid` | `src/mcp/search_path_arg.c` | `mcp-search-path-arg-only` | pure bool |
| 34 | `int cbm_mcp_search_line_match_span` | `src/mcp/search_line_span.c` | `mcp-search-line-span-only` | pure int |
| 35 | `bool cbm_mcp_search_args_valid` | `src/mcp/search_args.c` | `mcp-search-args-only` | pure bool |
| 36 | `int cbm_mcp_search_score_cmp` | `src/mcp/search_score_cmp.c` | `mcp-search-score-cmp-only` | pure int |
| 37 | `cbm_split_camel_case`／`cbm_tokenize_decorator` | `src/pipeline/enrichment_tokens.c` | `pipeline-enrichment-tokens-only` | C malloc／caller free |
| 38 | 6 個 `cbm_pipeline_pkgmap_*` helper | `src/pipeline/pkgmap_text.c` | `pipeline-pkgmap-text-only` | line value 借用 source；path C malloc／caller free |
| 39 | `int cbm_mcp_search_code_score` | `src/mcp/search_code_score.c` | `mcp-search-code-score-only` | pure int |
| 40 | `size_t cbm_mcp_search_top_dir` | `src/mcp/search_top_dir.c` | `mcp-search-top-dir-only` | pure bounded buffer |
| 41 | `bool cbm_mcp_detect_changes_wants_symbols` | `src/mcp/detect_changes_scope.c` | `mcp-detect-changes-scope-only` | pure bool |
| 42 | `bool cbm_mcp_detect_changes_impacted_label` | `src/mcp/detect_changes_label.c` | `mcp-detect-changes-label-only` | pure bool |
| 43 | `size_t cbm_mcp_detect_changes_status_path_offset` | `src/mcp/detect_changes_status.c` | `mcp-detect-changes-status-only` | pure offset |
| 44 | `long cbm_mcp_node_resolution_score` | `src/mcp/node_resolution_score.c` | `mcp-node-resolution-score-only` | pure long |
| 45 | `bool cbm_mcp_utf8_is_cont_byte` | `src/mcp/utf8_is_cont.c` | `mcp-utf8-is-cont-only` | pure bool |
| 46 | `int cbm_mcp_adr_mode` | `src/mcp/adr_mode.c` | `mcp-adr-mode-only` | pure int |
| 47 | `uint32_t cbm_mcp_trace_mode_edge_mask` | `src/mcp/trace_mode_edge_mask.c` | `mcp-trace-mode-edge-mask-only` | pure uint32_t |
| 48 | `bool cbm_mcp_trace_is_test_file` | `src/mcp/trace_is_test_file.c` | `mcp-trace-is-test-file-only` | pure bool |
| 49 | `bool cbm_mcp_project_db_file_name` | `src/mcp/project_db_file_name.c` | `mcp-project-db-file-name-only` | pure bool |
| 50 | `void cbm_mcp_sanitize_ascii_in_place` | `src/mcp/sanitize_ascii_in_place.c` | `mcp-sanitize-ascii-in-place-only` | pure in-place mutator |
| 51 | `size_t cbm_mcp_bm25_file_pattern_like(char *, size_t, const char *)` | `src/mcp/bm25_file_pattern_like.c` | `mcp-bm25-file-pattern-like-only` | caller-owned buffer；fallback temporary heap |
| 52 | `int cbm_mcp_bm25_build_match(const char *, char *, size_t)` | `src/mcp/bm25_build_match.c` | `mcp-bm25-build-match-only` | pure caller-owned buffer |
| 53 | `char *cbm_mcp_sanitize_utf8_lossy(const char *)` | `src/mcp/sanitize_utf8_lossy.c` | `mcp-sanitize-utf8-lossy-only` | C malloc／caller free |

詳見本檔後段「Calls import JSON」「Usages import JSON」「MCP search_mode」…「MCP search_code_score」
各切片段落。wrapper 多相容 `CBM_USE_RUST_MCP_CODEC`（MCP）或專屬 pipeline flag。

### 下一候選 ABI 狀態

- **#54 architecture_aspect_wanted**：已拆 fallback／wrapper／direct ABI，但 Make full gate 尚未有完整成功證據；
  先收尾，不得列入上表或選擇下一候選。
- **complexity JSON**：延後（strtol locale／overflow／NULL key）。

---

## 2026-07-19 第 54 項 MCP architecture_aspect_wanted（進行中）ABI

Public C ABI 與 Rust direct ABI 預定相同：

    bool cbm_mcp_architecture_aspect_wanted(const char *input, const char *name);

`input == NULL`、invalid JSON（含 trailing bytes）、root 非 object、缺少 `aspects` 或其非 array 都回 true；空 array
或無匹配回 false；array 中的 `"all"` 或 exact `name` 回 true，非字串元素忽略。比較大小寫與空白敏感，C string
只讀至 first-NUL，`name == NULL` 按空字串處理。此 ABI 不轉移 heap ownership；C fallback 可為 yyjson 解析暫配
文件，無 I/O。

C fallback 為 `src/mcp/architecture_aspect_wanted.c`。`mcp.c` 仍保留已解析 aspects 作 Store architecture
traversal；僅三個 emission filters 以 raw `params_json` 呼叫新 ABI。wrapper
`CBM_USE_RUST_MCP_ARCHITECTURE_ASPECT_WANTED`（相容 `CBM_USE_RUST_MCP_CODEC`）委派
`cbm_rs_mcp_architecture_aspect_wanted_v1`；direct
`CBM_USE_RUST_MCP_ARCHITECTURE_ASPECT_WANTED_ONLY`／Cargo
`mcp-architecture-aspect-wanted-only` 匯出同名 ABI。Makefile 的 direct test-rust-ffi support 另補入
`$(YYJSON_SRC)`，使 C fallback 可解析 JSON。

fmt、Rust unit **1 passed**、切片 clippy 與 `make -pn` direct selector 已通過。首次 optin gate 的 default
`test-rust-ffi` 因缺少 `yyjson_read_opts` link 失敗；修正後的重啟 run 未保存可採信最終結果，ONLY gate 尚未跑。
因此本段是 **WIP ABI 記錄，不是 true-source 宣告**；接手時須序列執行並保存：

~~~sh
make -j1 -f Makefile.cbm rust-mcp-architecture-aspect-wanted-optin-test
make -j1 -f Makefile.cbm rust-mcp-architecture-aspect-wanted-only-test
~~~

---

## 2026-07-19 第 53 項 MCP sanitize_utf8_lossy true-source ABI

Public C ABI 與 Rust direct ABI 相同：

    char *cbm_mcp_sanitize_utf8_lossy(const char *input);

NULL input 回 null。其餘輸入讀至 first-NUL，保留合法 UTF-8 sequence，且對每個 invalid byte 輸出一個
U+FFFD（`EF BF BD`）。成功結果均以 C `malloc` 配置，caller 必須 `free()`；不得用 Rust allocator 或假設
borrowed input，無 I/O。wrapper 以 v1 length-only／caller-buffer ABI 配置 C output；direct Rust export 也直接
呼叫 C `malloc`，因此保留既有 C caller 的釋放責任。

C fallback `src/mcp/sanitize_utf8_lossy.c`、wrapper `CBM_USE_RUST_MCP_SANITIZE_UTF8_LOSSY`（相容 CODEC）、
direct `CBM_USE_RUST_MCP_SANITIZE_UTF8_LOSSY_ONLY`／`mcp-sanitize-utf8-lossy-only` 已通過 full gate；direct 排除
fallback、保留 `mcp.c`。

---

## 2026-07-19 第 52 項 MCP bm25_build_match true-source ABI

Public C direct ABI 維持既有 C call site 的 input-first 順序：

    int cbm_mcp_bm25_build_match(const char *input, char *buf, size_t bufsize);

這不同於既有 v1 bridge `cbm_rs_mcp_bm25_build_match_v1(char *buf, size_t bufsize, const char *input)` 的
buffer-first 順序；direct Rust export 在邊界負責轉接。`input == NULL`、`buf == NULL` 或 `bufsize < 2` 回 0
且不寫入。其餘情況只收集 ASCII alnum／underscore token，以 ` OR ` 連接；不足以容納下一個完整 token
（含 NUL）時停在先前完整 token，NUL-terminate 並回 token 數。標點與 non-ASCII 是分隔符，輸入採
first-NUL；`buf` 由 caller 擁有，無配置、無 I/O。

C fallback `src/mcp/bm25_build_match.c`、wrapper `CBM_USE_RUST_MCP_BM25_BUILD_MATCH`（相容 CODEC）、direct
`CBM_USE_RUST_MCP_BM25_BUILD_MATCH_ONLY`／`mcp-bm25-build-match-only` 已通過 full gate；direct 排除 fallback、
保留 `mcp.c`。

---

## 2026-07-19 第 51 項 MCP bm25_file_pattern_like true-source ABI

Public C ABI 與 Rust direct ABI 相同：

    size_t cbm_mcp_bm25_file_pattern_like(char *buf, size_t bufsize, const char *input);

`input == NULL` 回 `SIZE_MAX`；成功時回完整 SQL LIKE output byte 長度。`buf == NULL` 或 `bufsize == 0`
只回長度；短 buffer 最多寫 `bufsize - 1` bytes 並 NUL-terminate。輸入採 first-NUL，先使用 Store
glob-to-LIKE（`*`→`%`、`?`→`_`），若原 pattern 不含 wildcard 則加前後 `%`。`buf` 由 caller 擁有，
不得假設 Rust 或 C 回傳可 free 的字串；C fallback 只在函式內暫時配置並釋放 glob 結果，無 I/O。
`mcp.c` 的 BM25 orchestration 另行配置 final buffer 並維持既有釋放責任。

C fallback `src/mcp/bm25_file_pattern_like.c`、wrapper `CBM_USE_RUST_MCP_BM25_FILE_PATTERN_LIKE`（相容
CODEC）、direct `CBM_USE_RUST_MCP_BM25_FILE_PATTERN_LIKE_ONLY`／`mcp-bm25-file-pattern-like-only` 已通過
full gate；direct 排除 fallback、保留 `mcp.c`。

---

## 2026-07-19 第 50 項 MCP sanitize_ascii_in_place true-source ABI

Public C ABI 與 Rust direct ABI 相同：

    void cbm_mcp_sanitize_ascii_in_place(char *input);

NULL 為 no-op；僅掃描 first-NUL 前的 bytes，將每個大於 127 的 byte 就地改為 `?`，並保留 ASCII（含
`0x7f`）；NUL 後 bytes 不讀不改。ABI 預期 caller 提供可寫 NUL-terminated C string；不配置、不保存，
也不執行 I/O。C fallback `src/mcp/sanitize_ascii_in_place.c`、wrapper
`CBM_USE_RUST_MCP_SANITIZE_ASCII_IN_PLACE`（相容 CODEC）、direct
`CBM_USE_RUST_MCP_SANITIZE_ASCII_IN_PLACE_ONLY`／`mcp-sanitize-ascii-in-place-only` 已通過 full gate；direct
排除 fallback、保留 `mcp.c`。

---

## 2026-07-19 第 49 項 MCP project_db_file_name true-source ABI

Public C ABI 與 Rust direct ABI 相同：

    bool cbm_mcp_project_db_file_name(const char *name);

NULL、空字串、`.db`、非精準 `.db` suffix、`_` 開頭與 `:memory:` 開頭回 false；至少 `x.db`、精準
`.db` suffix 才可能為 true，`tmp-*.db` 為合法。name 採 first-NUL C-string；不配置、不保存、不改寫
pointer，也不執行 I/O。C fallback `src/mcp/project_db_file_name.c`、wrapper
`CBM_USE_RUST_MCP_PROJECT_DB_FILE_NAME`（相容 CODEC）、direct
`CBM_USE_RUST_MCP_PROJECT_DB_FILE_NAME_ONLY`／`mcp-project-db-file-name-only` 已通過 full gate；direct
排除 fallback、保留 `mcp.c`。

---

## 2026-07-19 第 48 項 MCP trace_is_test_file true-source ABI

Public C ABI 與 Rust direct ABI 相同：

    bool cbm_mcp_trace_is_test_file(const char *path);

NULL、空字串回 false；path 含 `/test`、`test_`、`_test.`、`/tests/`、`/spec/` 或 `.test.` 任一
case-sensitive substring 回 true，其餘回 false。path 採 first-NUL C-string；不配置、不保存、不改寫 pointer，
也不執行 I/O。C fallback `src/mcp/trace_is_test_file.c`、wrapper
`CBM_USE_RUST_MCP_TRACE_IS_TEST_FILE`（相容 CODEC）、direct
`CBM_USE_RUST_MCP_TRACE_IS_TEST_FILE_ONLY`／`mcp-trace-is-test-file-only` 已通過 full gate；direct 排除
fallback、保留 `mcp.c`。

---

## 2026-07-19 第 47 項 MCP trace_mode_edge_mask true-source ABI

Public C ABI 與 Rust direct ABI 相同：

    uint32_t cbm_mcp_trace_mode_edge_mask(const char *input);

NULL、空字串、`calls`、未知、大小寫不符或尾端空白回 `CALLS` bit；`data_flow` 回
`CALLS|DATA_FLOWS`；`cross_service` 回完整 bit 0..9 edge set（`0x3ff`）。input 採 first-NUL C-string；
不配置、不保存、不改寫 pointer，也不執行 I/O。C fallback `src/mcp/trace_mode_edge_mask.c`、wrapper
`CBM_USE_RUST_MCP_TRACE_MODE_EDGE_MASK`（相容 CODEC）、direct
`CBM_USE_RUST_MCP_TRACE_MODE_EDGE_MASK_ONLY`／`mcp-trace-mode-edge-mask-only` 已通過 full gate；direct
排除 fallback、保留 `mcp.c`。

---

## 2026-07-19 第 46 項 MCP adr_mode true-source ABI

Public C ABI 與 Rust direct ABI 相同：

    int cbm_mcp_adr_mode(const char *input);

NULL、空字串、`get`、未知、大小寫不符或尾端空白回 0（get/default）；`update`、`store` 回 1；`sections`
回 2。input 採 first-NUL C-string；不配置、不保存、不改寫 pointer，也不執行 I/O。C fallback
`src/mcp/adr_mode.c`、wrapper `CBM_USE_RUST_MCP_ADR_MODE`（相容 CODEC）、direct
`CBM_USE_RUST_MCP_ADR_MODE_ONLY`／`mcp-adr-mode-only` 已通過 full gate；direct 排除 fallback、保留
`mcp.c`。

---

## 2026-07-19 第 45 項 MCP utf8_is_cont_byte true-source ABI

Public C ABI 與 Rust direct ABI 相同：

    bool cbm_mcp_utf8_is_cont_byte(int byte);

僅檢查 `byte` 的低 8 bits；`0x80..0xBF`（`10xxxxxx`）回 true，其他回 false。負數與超過 byte 範圍的
輸入同樣先採 byte mask。不配置、不保存、不改寫輸入，也不執行 I/O。C fallback
`src/mcp/utf8_is_cont.c`、wrapper `CBM_USE_RUST_MCP_UTF8_IS_CONT`（相容 CODEC）、direct
`CBM_USE_RUST_MCP_UTF8_IS_CONT_ONLY`／`mcp-utf8-is-cont-only` 已通過 full gate；direct 排除 fallback、
保留 `mcp.c`。

---

## 2026-07-19 第 44 項 MCP node_resolution_score true-source ABI

Public C ABI 與 Rust direct ABI 相同：

    long cbm_mcp_node_resolution_score(const char *label, int start_line, int end_line);

Function／Method 為 tier 2，Module／File 與 NULL 為 tier 0，其他 label（含空字串）為 tier 1；回傳
tier * 1000000 加 `max(end_line - start_line, 0)`。label 採 first-NUL C-string；不配置、不保存、不改寫
pointer，也不執行 I/O。C fallback `src/mcp/node_resolution_score.c`、wrapper
`CBM_USE_RUST_MCP_NODE_RESOLUTION_SCORE`（相容 CODEC）、direct
`CBM_USE_RUST_MCP_NODE_RESOLUTION_SCORE_ONLY`／`mcp-node-resolution-score-only` 已通過 full gate；direct
排除 fallback、保留 `mcp.c`。

---

## 2026-07-19 第 43 項 MCP detect_changes_status_path_offset true-source ABI

Public C ABI 與 Rust direct ABI 相同：

    size_t cbm_mcp_detect_changes_status_path_offset(const char *line);

NULL、空字串或最終 path 空回 `SIZE_MAX`。`XY ` porcelain prefix 跳過三個 bytes；rename 的 `old -> new`
回 destination 起始 offset；bare path 與 unknown prefix 均回 0。line 採 first-NUL C-string；不配置、不保存、
不改寫 pointer，也不執行 I/O。C fallback `src/mcp/detect_changes_status.c`、wrapper
`CBM_USE_RUST_MCP_DETECT_CHANGES_STATUS`（相容 CODEC）、direct
`CBM_USE_RUST_MCP_DETECT_CHANGES_STATUS_ONLY`／`mcp-detect-changes-status-only` 已通過 full gate；direct
排除 fallback、保留 `mcp.c`。

---

## 2026-07-19 第 42 項 MCP detect_changes_impacted_label true-source ABI

Public C ABI 與 Rust direct ABI 相同：

    bool cbm_mcp_detect_changes_impacted_label(const char *label);

NULL、`File`、`Folder`、`Project` 回 false；空字串與所有其他值回 true。label 採 first-NUL C-string；
不配置、不保存、不改寫 pointer，也不執行 I/O。C fallback `src/mcp/detect_changes_label.c`、wrapper
`CBM_USE_RUST_MCP_DETECT_CHANGES_LABEL`（相容 CODEC）、direct
`CBM_USE_RUST_MCP_DETECT_CHANGES_LABEL_ONLY`／`mcp-detect-changes-label-only` 已通過 full gate；direct
排除 fallback、保留 `mcp.c`。

---

## 2026-07-19 第 41 項 MCP detect_changes_wants_symbols true-source ABI

Public C ABI 與 Rust direct ABI 相同：

    bool cbm_mcp_detect_changes_wants_symbols(const char *scope);

NULL、`symbols`、`impact` 回 true；空字串、`files`、大小寫不符、尾端空白與其他值回 false。scope 採
first-NUL C-string；不配置、不保存、不改寫 pointer，也不執行 I/O。C fallback
`src/mcp/detect_changes_scope.c`、wrapper `CBM_USE_RUST_MCP_DETECT_CHANGES_SCOPE`（相容 CODEC）、
direct `CBM_USE_RUST_MCP_DETECT_CHANGES_SCOPE_ONLY`／`mcp-detect-changes-scope-only` 已通過 full gate；
direct 排除 fallback、保留 `mcp.c`。

---

## 2026-07-19 第 40 項 MCP search_top_dir true-source ABI

Public C ABI 與 Rust direct ABI 相同：

    size_t cbm_mcp_search_top_dir(char *buf, size_t bufsize, const char *file);

`file == NULL` 回 `SIZE_MAX` 且不得改寫 `buf`。成功時回完整 top-level key 長度：第一個 `/` 存在時
包含該 slash，否則為完整 file。`buf == NULL` 或 `bufsize == 0` 為 length-only；可寫短 buffer 最多寫
`bufsize - 1` bytes，並以 NUL 結尾。file 採 first-NUL C-string；不配置、不保存、不改寫 pointer，也不
執行 I/O。C fallback `src/mcp/search_top_dir.c`、wrapper
`CBM_USE_RUST_MCP_SEARCH_TOP_DIR`（相容 CODEC）、direct
`CBM_USE_RUST_MCP_SEARCH_TOP_DIR_ONLY`／`mcp-search-top-dir-only` 已通過 full gate；direct 排除
fallback、保留 `mcp.c`。

---

## 2026-07-17 第 28 項 Pipeline split-command true-source ABI

pipeline-split-command 已完成，true-source 累計由 **22 / 22 -> 23 / 23**。fallback 為
src/pipeline/split_command.c/.h；src/pipeline/pass_compile_commands.c 繼續承擔
compile_commands orchestration 與 legacy resolve_path，本項只替換 splitter。

ABI 固定為：

    int cbm_split_command(const char *cmd, char **out, int max_out);

cmd == NULL、out == NULL、max_out <= 0 回傳 0；有 out 時不得改寫 caller sentinel。有效字串以
first-NUL 截斷，採 raw bytes，space/tab 分隔，單／雙引號移除；未閉合引號延續到 first-NUL。
每個 token payload 上限為 4095 bytes，輸出 first-NUL 結尾。只寫 out[0..count)，不以 NULL
改寫 out[count] 或後續 slot；token 由 C malloc 配置，caller 逐一 free()。OOM 不屬保證契約，
保留為非保證殘餘差異。

wrapper CBM_USE_RUST_PIPELINE_SPLIT_COMMAND=1 維持 v1
cbm_rs_pipeline_split_command_v1，Rust v1 透過 C allocator 產生可由 C free() 的 token。direct
only 使用 CBM_USE_RUST_PIPELINE_SPLIT_COMMAND_ONLY=1 與 Cargo
pipeline-split-command-only，Rust staticlib 匯出同名 public cbm_split_command，不改變所有權。

Make 的 rust-pipeline-split-command-optin-test 與
rust-pipeline-split-command-only-test 已覆蓋 default/wrapper/direct FFI、pipeline **231 passed**
與 production --version。direct 要求 PIPELINE_SPLIT_COMMAND_SRCS =，source-negative 排除
src/pipeline/split_command.c，並正向確認 src/pipeline/pass_compile_commands.c 仍在產品 recipe。

接手時不要再動 #28；先做候選 inventory。pipeline-complexity-json 必須先解決 C strtol 的
locale whitespace、overflow 與 NULL key 契約，才可排入下一個 true-source slice。

## 2026-07-17 第 27 項 Pipeline LSP cross classifiers true-source

本項是新的 true-source slice，累計為 **22 / 22**；既有 21 / 21 歷史、第 22 至第 26 項、
path_join、route-node classifiers、language_markers 與 JSON prop hardening 均不重算。C fallback
為 `src/pipeline/lsp_cross_classifiers.c/.h`，但 `pass_lsp_cross.c` 的檔案 I/O、arena、registry、
LSP resolver、graph merge 與 orchestration 未搬移。

同名 C public／Rust direct ABI 為：

- `void cbm_pxc_ts_modes(CBMLanguage lang, const char *rel_path, bool *out_js, bool *out_jsx, bool *out_dts)`
- `bool cbm_pxc_has_cross_lsp(CBMLanguage lang)`
- `const char *cbm_pxc_map_label(const char *label)`

Rust direct-only 用 `c_int` 接 enum、C `bool` 接 TS output flags 與 predicate、raw pointer 接收
`rel_path`／`label`。`tests/test_rust_ffi.c` 以
`_Static_assert(sizeof(CBMLanguage) == sizeof(int))` 鎖住 enum ABI。`rel_path == NULL` 合法；三個
TS output pointers 必須非 NULL 且可寫，既有 C 沒有 NULL output pointer 契約。mode bits 固定為
JS `1`、JSX `2`、DTS `4`，結果只在 `0..7`。

`cbm_pxc_has_cross_lsp` 對 Go、C、C++、CUDA、Python、JavaScript、TypeScript、TSX、PHP、C#、
Java、Kotlin、Rust 共 13 個 language 回 true；未知或未支援 enum 回 false。C# 的 true 只表示
prebuilt registry eligibility，不保證 fallback resolver 執行或能解析 call。`cbm_pxc_map_label`
只接受 Class、Struct、Interface、Enum、Type、Trait、Protocol、Function、Method 九個 raw-byte、
大小寫敏感 label；成功時回傳原始 borrowed label pointer，其他值或 NULL 回 NULL。字串以 first-NUL
C-string 截斷；Rust 不配置、保存、釋放或回傳 Rust-owned label。

wrapper `CBM_USE_RUST_PIPELINE_LSP_CLASSIFIERS=1` 保留既有 v1 exports；direct
`CBM_USE_RUST_PIPELINE_LSP_CLASSIFIERS_ONLY=1`／Cargo `pipeline-lsp-classifiers-only` 由 Rust
staticlib 匯出三個同名 public ABI。`PIPELINE_LSP_CLASSIFIERS_SRCS` 在 ONLY 為空，只排除
`src/pipeline/lsp_cross_classifiers.c`，不排除 `pass_lsp_cross.c`；`RUST_FFI_SUPPORT_SRCS` 加入此
selector，讓 default/wrapper FFI 連入 C public bridge，direct 由 Rust symbols 提供。

已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline_lsp_cross --locked`
（**3 passed**）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、
`make -j1 -f Makefile.cbm rust-pipeline-lsp-classifiers-optin-test` 與
`make -j1 -f Makefile.cbm rust-pipeline-lsp-classifiers-only-test`。四個 gate leg 的 FFI／pipeline
皆通過，pipeline 均為 **230 passed**，production `--version` 成功；direct selector 空值與
`make -Bn` source-negative 均通過。

下一步只建立下輪候選 inventory；尚未選定或開始下一個候選。

## 2026-07-17 Pipeline JSON property 既有 true-source ABI/gate hardening

此項只補強既有 Pipeline JSON property true-source 的 ABI、FFI regression 與 Make gate，並非
新的 true-source slice；歷史計數維持 **21 / 21**，不重算既有第 22 至第 26 項、path_join、
route-node classifiers 或 language_markers。

public C ABI 為
`bool cbm_pipeline_extract_json_prop(const char *json, const char *key, char *buf, int bufsz)`。
既有 wrapper v1 ABI `cbm_rs_pipeline_extract_json_prop_v1` 回傳 `int`；direct Rust ABI 對同一
public 名稱回傳 `bool`。Rust FFI boundary 的 `bufsz` 一律使用有號 `c_int`，不得在檢查前轉為
`usize`。

`json`、`key` 或 `buf` 為 NULL，或 `bufsz <= 0` 時，兩條 ABI 均回失敗且不寫入 caller buffer。
空 key 可成功；空 value 失敗且 buffer 保持不變。key 長度上限是 59 bytes，60 bytes 以上拒絕且
buffer 不變。掃描仍是既有 raw scanner，輸入採 first-NUL C-string 截斷語意，不解析或讀取 NUL
後資料。Rust 不保存借用指標、不配置、釋放或取得 caller buffer 的所有權。

`RUST_FFI_SUPPORT_SRCS` 已加入 `$(PIPELINE_JSON_PROP_SRCS)`，確保 default/wrapper FFI 真正連入
C fallback public bridge；`CBM_USE_RUST_PIPELINE_JSON_PROP_ONLY=1` 時 selector
`PIPELINE_JSON_PROP_SRCS =`，則由 Rust direct staticlib 匯出同名 public ABI。兩個 JSON prop
gate 的 default leg 均新增隔離 production `cbm --version`。

已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::route --locked`
（**14 passed**）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、
`make -j1 -f Makefile.cbm rust-pipeline-json-prop-optin-test` 與
`make -j1 -f Makefile.cbm rust-pipeline-json-prop-only-test`。兩個 gate 合計覆蓋
default/wrapper/direct FFI、pipeline runner 各 **229 passed** 與全部模式 production `--version`；
direct selector 空值與 `make -Bn` source-negative 均通過。

此處當時的 LSP cross classifiers 預稽核已完成；最新停點見文件頂端。

## 2026-07-17 Discover language_markers current-revision hardening

此項只補強既有 language_markers true-source 的 parity、ABI regression 與 Make gate，非新
true-source slice；歷史計數維持 **21 / 21**。既有第 22 至第 26 項、path_join 與
route-node classifiers 記錄不受影響。

public bridge 與 direct Rust staticlib 皆維持 `cbm_discover_language_marker`；wrapper 維持
既有 v1 ABI。輸入 `line` 是 borrowed、NUL 結尾 raw-byte C string，Rust 不保存其指標、不跨
ABI 配置記憶體，且不轉移所有權。`line == NULL` 或 `kind` 無效時回 `0`。

kind `2` 的語意已凍結為 NUL 結束 bytes 的 ASCII 判定：僅當 `intrinsic ` 或 `procedure `
後有至少一個 ASCII `[A-Za-z]` 名稱字元，並且名稱後立即接 `(` 時回真。`\xE9`、空名稱、
空白、缺 `(` 與內嵌 NUL 截斷後的尾端內容皆回 false。C fallback 已移除 locale-sensitive
`isalpha`，因此與 Rust ASCII predicate 對齊。

v1／public／direct 三路 FFI regression 已納入；Make gate 亦補強 default production coverage。
已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core discover --locked`（**11 passed**）及
`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`。已通過
`rust-discover-language-markers-optin-test` 與
`rust-discover-language-markers-only-test`：FFI regression、Discover runner **90 passed**、正式版
`--version` 均成功；ONLY 另證明 `DISCOVER_LANGUAGE_MARKERS_SRCS =`，且 `make -Bn`
source-negative 靜默成功。

後續不得將 `pipeline-json-prop` 直接視為已 parity；先 audit 其 `int`／`usize` ABI 邊界、
NULL／`<= 0` 行為與 key truncation 的 C/Rust 可觀察契約。

## 第 25 項：Discover filters true-source ABI（2026-07-17）

本項為原 `21/21` 結束後的後續 true-source slice；不改寫第 22 項原預算外 true-source，亦不
改變第 23 項 artifact-path parity/gate hardening 的非 true-source 定位。

### C fallback、wrapper 與 direct ABI

- C fallback 為 `src/discover/filters.c`，公開 predicate 為 `cbm_should_skip_dir`、
  `cbm_has_ignored_suffix`、`cbm_should_skip_filename`、`cbm_matches_fast_pattern`。
  walk、ignore 組態與其餘 discover orchestration 不跨此 FFI 邊界。
- `CBM_USE_RUST_DISCOVER_FILTERS=1` 時，C bridge 呼叫既有 v1 ABI；
  `CBM_USE_RUST_DISCOVER_FILTERS_ONLY=1`／Cargo `discover-filters-only` 時，由 Rust
  staticlib 直接提供同名 public ABI。Rust direct ABI 原已存在，並非本項新增；direct
  `DISCOVER_FILTERS_SRCS =`，且 `make -Bn` source-negative 已靜默證明 production recipe
  不連結 C fallback。
- 四個 predicate 的 NULL 輸入均回 `false`；只有 `CBM_MODE_FULL`（0）是 full，任何非零
  mode 均為 restricted。字串比對採 raw byte 精確、大小寫敏感語意，不配置、保存或釋放 C
  記憶體。
- wrapper 啟用 Rust bridge 時，C fallback filter tables 以條件編譯排除，避免
  `-Werror` unused-variable；default C fallback 行為不變。

### 已執行驗證

- `cargo fmt --all -- --check`、`cargo test -p cbm-core discover --locked`（5 passed）與
  `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings` 均成功。
- `make -j1 -f Makefile.cbm rust-discover-filters-optin-test` 已通過 default/wrapper FFI、
  Discover 各 89 passed 與正式版 `--version`。
- `make -j1 -f Makefile.cbm rust-discover-filters-only-test` 已通過 default/direct FFI、
  direct Discover 89 passed、正式版 `--version`、空白 `DISCOVER_FILTERS_SRCS =` 與靜默
  source-negative。

## 2026-07-17 Discover path_join parity 追加紀錄（既有 ABI）

此項是既有 Discover path_join true-source 的 superseding parity 修正與 C/Rust regression 補強，
不新增 true-source，也不改變公開 ABI、Cargo feature、C fallback 或 direct selector；較早的
「尚未驗證」歷史文字保持不變。

- Rust path_join 在輸出寫入、截斷並將 `\` 轉為 `/` 後，僅當輸出第一 byte 為 ASCII 小寫
  drive、第二 byte 為 `:`，且第三 byte 是 `/` 或字串在第二 byte 結束時，才大寫第一 byte。
  `c:/tmp` 與 `c:\\tmp` 皆變 `C:/tmp`；bare `c:` 變 `C:`；`c:relative` 不變；
  `c:x` 截斷成 `c:` 時亦變 `C:`。
- 已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core discover --locked`
  （5 passed）及 `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`。
  `make -j1 -f Makefile.cbm rust-discover-path-join-optin-test` 已通過 default/wrapper FFI、
  Discover 各 89 passed 與 wrapper 正式版 `--version`；
  `make -j1 -f Makefile.cbm rust-discover-path-join-only-test` 已通過 default/direct FFI、
  Discover 各 89 passed、direct 正式版 `--version`、空白
  `DISCOVER_PATH_JOIN_SRCS =` 與靜默 source-negative。

## 第 26 項：Discover local_rel_path offset true-source ABI（2026-07-17）

本項為後續 true-source slice，不改寫原 `21/21`、第 22 項原預算外 true-source、第 23 項非新增
true-source、第 25 項或 path_join parity 的歷史定位。

### C fallback、wrapper 與 direct ABI

- C fallback 為 `src/discover/local_rel_path.c/.h`。`discover.c` 的兩個巢狀 `.gitignore`
  callsite 已由原 static pointer helper 改為 offset projection。
- 公開 API 為
  `size_t cbm_discover_local_rel_path_offset(const char *rel_path, const char *local_prefix)`。
  `rel_path == NULL`、`local_prefix == NULL`、空 prefix 或未匹配均回 `0`；只有 raw
  byte 精確、大小寫敏感 prefix，且下一 byte 是 `/` 時，才回 `prefix_len + 1`。不進行
  正規化、配置、修改或 C 指標保存。
- `CBM_USE_RUST_DISCOVER_LOCAL_REL_PATH=1` wrapper 經 v1 ABI；
  `CBM_USE_RUST_DISCOVER_LOCAL_REL_PATH_ONLY=1`／Cargo
  `discover-local-rel-path-only` direct 使用 direct ABI。direct
  `DISCOVER_LOCAL_REL_PATH_SRCS =`，且 `make -Bn` source-negative 已靜默證明 production
  recipe 不連結 C fallback。

### 已執行驗證

- `cargo fmt --all -- --check`、`cargo test -p cbm-core local_rel_path --locked`（5 passed）
  與 `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings` 均成功。
- `make -j1 -f Makefile.cbm rust-discover-local-rel-path-optin-test` 已通過 default/wrapper
  FFI、Discover 各 90 passed、default/wrapper 正式版 `--version`。
- `make -j1 -f Makefile.cbm rust-discover-local-rel-path-only-test` 已通過 default/direct FFI、
  Discover 各 90 passed、default/direct 正式版 `--version`、空白
  `DISCOVER_LOCAL_REL_PATH_SRCS =` 與靜默 source-negative。

## 2026-07-17 Route-node classifiers 既有 true-source ABI/gate hardening

本項是既有 route-node classifiers true-source 的 ABI/gate hardening，並非新 true-source slice；
原 `21/21`、第 22 項原預算外、第 23 項非新增 true-source、第 25、26 項與 path_join parity
歷史均不改變。

### 固定 classifier 契約與 Make direct 邊界

- 新增 `src/pipeline/route_node_classifiers.h`，固定既有 `hash_segment`／`broker_route`
  API，不把既有 C/Rust 演算法重新分類為新實作。
- `hash_segment` 對 NULL、長度 0、長度大於 12 拒絕；僅小寫 ASCII 字母與數字可參與判定。
  長度至少 3 而含字母拒絕；短合法字母／數字與全數字可接受；大寫、符號、NUL、非 ASCII 拒絕。
- `broker_route` 對 NULL 回 `false`，只接受從首 byte 開始、大小寫敏感的七個既有 prefix；
  `__route__GET__` 明確為負例。
- direct macro 已修正為 `CBM_USE_RUST_PIPELINE_ROUTE_NODE_CLASSIFIERS_ONLY`；FFI support
  使用 `ROUTE_NODE_CLASSIFIERS_SRCS` selector，direct 排除既有 C fallback。

### 已執行驗證

- `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::route --locked`
  （13 passed）及 `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`
  均成功。
- `make -j1 -f Makefile.cbm rust-pipeline-route-node-classifiers-optin-test` 已通過
  default/wrapper FFI、route_canon pipeline 各 241 passed 與正式版 `--version`。
- `make -j1 -f Makefile.cbm rust-pipeline-route-node-classifiers-only-test` 已通過
  default/wrapper/direct FFI、route_canon pipeline 各 241 passed、正式版 `--version`、空白
  `ROUTE_NODE_CLASSIFIERS_SRCS =` 與靜默 `make -Bn` source-negative。

## 第 24 項：AST profile classifiers true-source ABI（2026-07-17）

此項為原 `21/21` 結束後的後續 true-source slice；不改寫第 22 項原預算外 true-source，亦不改變
第 23 項既有 artifact-path parity/gate hardening 的非 true-source 定位。

### C fallback、wrapper 與 direct ABI

- C fallback 已拆至 `src/semantic/ast_profile_classifiers.c/.h`；`src/semantic/ast_profile.c` 保留
  Tree-sitter traversal 與 codec。
- `CBM_USE_RUST_AST_PROFILE_CLASSIFIERS=1` 時，C bridge 經既有 v1 ABI 委派 Rust；direct
  `CBM_USE_RUST_AST_PROFILE_CLASSIFIERS_ONLY=1`／Cargo `ast-profile-classifiers-only` 則排除 C
  fallback，由 Rust staticlib 匯出同名 public ABI：
  `cbm_ast_profile_kind_flags`、`cbm_ast_profile_halstead_insert`、
  `cbm_ast_profile_is_param_name`。
- 契約為 kind flags、固定 512 槽 Halstead insert 與 parameter-name 比對；防禦性 NULL／無效輸入
  結果必須與 Rust ABI 對齊。此 slice 不移轉 traversal、codec 或其餘 semantic orchestration。

### 已執行驗證

- `cargo fmt --all -- --check`、
  `cargo test -p cbm-core ast_profile_classifiers --locked`（3 passed）與
  `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings` 均成功。
- `rust-ast-profile-classifiers-optin-test` 已通過 default/wrapper FFI、pipeline 228 passed 與
  production `--version`；`rust-ast-profile-classifiers-only-test` 已通過 default/direct 相同檢查、
  `AST_PROFILE_CLASSIFIERS_SRCS =` 與靜默成功的 `make -Bn` source-negative。完整
  `scripts/test.sh` 未執行。

## 第 23 項：pipeline artifact-path parity 與 direct gate hardening（2026-07-17）

本項是既有 artifact-path ABI 的契約與 gate 強化，不是新的 true-source replacement；原 `21/21`
與第 22 項原預算外 true-source 的歷史計數均不變。

### 公開 C ABI 與 buffer 契約

```c
bool cbm_pipeline_artifact_path(char *buf, size_t bufsz,
                                const char *repo_path, const char *name);
```

- `buf` 是 caller-owned output buffer；`repo_path` 與 `name` 是 borrowed、有效 NUL 結尾輸入。所有
  路徑組合依 raw byte 進行，沒有 UTF-8 前提，且 output buffer 不可與任一 input string 重疊。
- 完整輸出放得下時，函式寫入完整 NUL 結尾值並回 `true`。若輸入有效、`bufsz > 0` 但容量過短，
  函式仍寫入 `min(full_len, bufsz - 1)` byte prefix 與尾端 NUL，然後回 `false`；不得把 legacy
  `snprintf` 所回的完整需求長度當成 public `bool` 成功。
- `buf == NULL` 或 `bufsz == 0` 時不寫入。無效輸入亦不得建立未定義 ownership；Rust 不保存 C
  pointer，也不配置或釋放 caller buffer。

### wrapper、v1 與 direct ABI

- C fallback 維持 `snprintf` 行為。wrapper `CBM_USE_RUST_PIPELINE_ARTIFACT_PATH=1` 透過 v1
  ABI 委派 Rust：成功回完整 `full_len`；過短或無效回 `SIZE_MAX`，其中過短仍保留 prefix 加 NUL 的
  side effect。
- `CBM_USE_RUST_PIPELINE_ARTIFACT_PATH_ONLY=1`／Cargo
  `pipeline-artifact-path-only` 時，Rust staticlib 直接匯出同名 public `bool` ABI；過短時同樣寫
  prefix 加 NUL 並回 `false`，`NULL`／zero-size 一律不寫。
- FFI selector 在 default/wrapper 連結 C bridge、direct 連結 Rust symbol；artifact runner 與
  production staticlib 都已採相同 direct 選擇。direct 的 `ARTIFACT_PATH_SRCS =`，並以 `make -Bn`
  source-negative 確認 production link 排除 C fallback。

### 已執行驗證

- `cargo fmt --all -- --check`、artifact-path Rust unit（4 passed）與 clippy 成功。
- opt-in gate 的 default/wrapper FFI、artifact runner（各 10 passed）及 production `--version`
  成功；only gate 最終重跑的 default/direct FFI、artifact runner（各 10 passed）、production
  `--version`、empty selector 與 source-negative 均成功。
- 完整 `scripts/test.sh` 未執行。

## 2026-07-16 ABI 交接優先規則

目前交接快照為 [../docs/rust-refactor-current-handoff.md](../docs/rust-refactor-current-handoff.md)。
stable ABI、wrapper opt-in 與 direct source replacement 是不同狀態；只有 direct mode 排除獨立
C fallback source、Rust 匯出同名 public ABI，並完成實際 default/wrapper/direct gate 後，才可
宣稱 source replacement。source-negative 檢查必須使用 make -Bn，避免最新 target 讓單獨
make -n 漏印 link recipe。

SvelteKit file-kind、trailing separator 以及 2026-07-16 multi-agent 新增的 path_join、
incremental_edge_type、semantic_fp_suffix、json_prop、incremental_registry_label、
mcp edge_type_valid 均已具備 true source replacement layout 且完成 current-revision
default/wrapper/direct gate。trailing-separator 與相關公開 bridge 的 NULL 契約為凍結的
防禦性行為，不是舊 private helper UB 的等價宣稱。

本文件定義 Rust 重構期間新增 Rust 程式碼的初始 FFI 規則。預設 build path 仍保留 C implementation；只有明確列出的 `CBM_USE_RUST_*` opt-in 切片會透過 C wrapper 呼叫 Rust staticlib。

## 所有權

- C 傳入 Rust 的指標只在該次呼叫期間借用。
- Rust 不可在回傳後保存借用的 C 指標。
- 若 Rust 配置 buffer 給 C 使用，必須提供明確的 Rust-side free function。
- C 配置的 buffer 仍由既有 C owner 釋放，通常是 arena-backed code。

## 字串與 Buffer

- FFI 邊界的 C string 視為 nullable、NUL-terminated byte sequence。
- Rust API 必須明確表示 nullable C input，通常使用 `Option`。
- 模擬 C byte-oriented 行為時，若涉及截斷或 escape，Rust 必須操作 bytes，而不是 Unicode scalar values。
- stdout 保留給 MCP JSON-RPC；Rust 診斷輸出必須走 stderr 或既有 logging path。

## 錯誤

- FFI function 應回傳穩定的 integer status code 或 nullable output pointer，並符合被替換 C 模組的慣例。
- Human-readable error text 必須是選用行為，除非該 migration 明確允許，否則不可改變公開 MCP 或 CLI 輸出。

## Unsafe Code

- Workspace Rust code 預設 deny unsafe。
- `ffi.rs` 是目前唯一允許 expose `no_mangle` 等 unsafe linkage surface 的模組。
- 未來若新增需要 unsafe 的 FFI module，必須隔離在小型 boundary module，並文件化 pointer validity assumptions、ownership transfer、null handling、allocation/free pairing，以及 invalid/empty input 測試覆蓋。

## 目前匯出

- `cbm_rs_dump_verify_is_degraded`：value-only ABI，由 `tests/test_rust_ffi.c` 驗證；回傳 integer `0`/`1`。
- `cbm_rs_dump_verify_parse_min_ratio_v1`：`CBM_DUMP_VERIFY_MIN_RATIO` 的 parser ABI，由 `CBM_USE_RUST_DUMP_VERIFY=1` 的 C wrapper 使用。Rust 以 byte-level parser 對齊 C `strtod` 的 ASCII whitespace、十進位/十六進位浮點前綴、partial-prefix 接受語意與 `0..=1` 範圍檢查；`valid_out` 回傳 0/1，讓 C 端保留 invalid env warning 與 default fallback，不保存 C 指標也不呼叫 C `strtod`。
- `cbm_rs_traces_extract_service_name_v1`、`cbm_rs_traces_extract_http_info_v1`、`cbm_rs_traces_extract_path_from_url_v1`、`cbm_rs_traces_parse_duration_v1`、`cbm_rs_traces_calculate_p99_v1`：`CBM_USE_RUST_TRACES=1` 使用的 OTLP traces helper ABI。`ffi.rs` 負責 C 指標、呼叫端 buffer、C struct 與 URL static buffer 的 ABI 邊界；`traces.rs` 只承接 service.name lookup、HTTP span info、URL path extraction、nanosecond duration 與 P99 的純位元組/值型 helper。預設仍編譯 `src/traces/traces.c` 作為 C fallback；`CBM_USE_RUST_TRACES_ONLY=1` 則以 Cargo `traces-only` feature 讓 Rust staticlib 直接匯出 `traces.h` 的五個 `cbm_extract_*` / `cbm_parse_duration` / `cbm_calculate_p99` 符號，且 C build 不編譯 `traces.c`。兩個 staticlib variant 使用不同 Cargo target dir，避免 feature archive 快取混用。MCP `ingest_traces` handler 與其他上層行為仍由 C 控制。
- `cbm_rs_intern_create/free/n/count/bytes`：給 `CBM_USE_RUST_STR_INTERN=1` 使用的 pool-owned string interning ABI。pool 由 Rust 配置與釋放，回傳的字串指標只保證在 pool free 前有效。
- `cbm_rs_intern_n` 接受 raw bytes 與 `len`，支援 embedded NUL；若 `pool` 或 `value` 為 null、`len > UINT32_MAX`、或 `len > isize::MAX`，會在讀取 input 前回傳 null。
- `cbm_rs_path_join`、`cbm_rs_path_join_n`、`cbm_rs_path_dir`、`cbm_rs_str_tolower`、`cbm_rs_str_replace_char`、`cbm_rs_str_strip_ext`、`cbm_rs_str_split_part`：寫入 caller-provided buffer，回傳完整輸出長度；若對應 C contract 應回傳 `NULL`，則回傳 `SIZE_MAX`。C wrapper 先查詢長度、用 `CBMArena` 配置，再呼叫 Rust 寫入。
- `cbm_rs_path_ext`、`cbm_rs_path_base`：回傳 borrowed pointer，指向 input C string 內部或 Rust static empty string；不得由 caller free。
- `cbm_rs_str_split_count`：回傳 split token 數；若 input 為 null 則回傳 `SIZE_MAX`。array 與 substring allocation 仍由 C wrapper 使用 `CBMArena` 完成。
- `cbm_rs_str_starts_with`、`cbm_rs_str_ends_with`、`cbm_rs_str_contains`、`cbm_rs_validate_shell_arg`、`cbm_rs_validate_project_name`：接受 nullable NUL-terminated C string，回傳 integer `0`/`1`。
- `cbm_rs_json_escape`：寫入 caller-provided buffer，遵守 `bufsize`，且永遠在可寫入時 NUL-terminate；不配置 Rust-owned output buffer。
- `cbm_rs_normalize_path_sep`：接受 nullable、可寫 NUL-terminated C string，原地將 backslash 轉成 slash，並 canonicalize drive letter；回傳原 input pointer。
- `cbm_rs_safe_getenv`：給 `CBM_USE_RUST_PLATFORM_ENV_DIRS=1` 使用；env 值以位元組處理，寫入呼叫端提供的 buffer，依呼叫端 buffer 大小截斷，並回傳完整輸出長度；缺值或無效參數以 `SIZE_MAX` 表示。
- `cbm_rs_get_home_dir`、`cbm_rs_app_config_dir`、`cbm_rs_app_local_dir`、`cbm_rs_resolve_cache_dir`：給 `CBM_USE_RUST_PLATFORM_ENV_DIRS=1` 使用；寫入呼叫端提供的 buffer，回傳完整輸出長度；若對應 C contract 應回傳 `NULL`，則回傳 `SIZE_MAX`。這些目錄 helpers 保留 C helper 的 255-byte 暫存值截斷與路徑分隔符正規化。Rust 不配置需要 C 釋放的 buffer，C wrapper 保留呼叫端 buffer 與靜態 buffer 所有權。
- `cbm_rs_detect_cgroup_cpus`、`cbm_rs_detect_cgroup_mem`：接受 nullable cgroup root C string，讀取 cgroup v2/v1 quota 檔案，僅回傳 scalar；null 或無有效 limit 時分別回傳 `-1`、`0`。
- `cbm_rs_diag_env_enabled_value`、`cbm_rs_diag_query_snapshot_values`：給 `CBM_USE_RUST_DIAGNOSTICS_FORMAT=1` 使用的 diagnostics deterministic helper ABI。Rust 處理 env value 判斷與已由 C atomic load 的 query scalar snapshot；atomic global stats、system metrics、writer thread、file write/rename、NDJSON rotation 與 stderr lifecycle 仍保留在 C。Rust ABI 回傳異常時 C wrapper fallback 到原邏輯。
- `cbm_rs_diag_format_path`、`cbm_rs_diag_format_json`、`cbm_rs_diag_format_ndjson`：給 `CBM_USE_RUST_DIAGNOSTICS_FORMAT=1` 使用的 deterministic formatter ABI。Rust 寫入 caller-provided buffer，短 buffer、null input 與輸出長度遵守既有 C contract；C wrapper 在 Rust ABI 回傳異常時 fallback 到原 formatter。atomic global stats、system metrics、writer thread、file write/rename、NDJSON rotation 與 stderr lifecycle 仍保留在 C。
- `cbm_rs_progress_sink_init`、`cbm_rs_progress_sink_fini`、`cbm_rs_progress_sink_fn`：CLI progress sink Rust backend ABI。C wrapper 仍保留 `cbm_progress_sink_init/fini/fn` 對外介面與 `cbm_log_set_sink()` 註冊點；Rust 只負責 structured log line 的事件解析、phase 格式化、`gbuf.dump`/`pipeline.done` 狀態記錄，以及 `parallel.extract.progress` 的 inline counter 與 fini newline flush。`cbm_progress_sink_init()` 仍由 C 傳入 `FILE*`（預設 `stderr`），Rust 不接管 log level/format 設定，也不改變 `cbm_log` 的其他 sink contract。
- `cbm_rs_profile_env_enabled`、`cbm_rs_profile_elapsed_us`、`cbm_rs_profile_elapsed_ms`、`cbm_rs_profile_rate_per_s`：profile deterministic helper ABI。`elapsed_*` 與 `rate_per_s` 仍是 test-only scalar parity；設定 `CBM_USE_RUST_PROFILE_ENV=1` 時，C `cbm_profile_init()` 只將 nullable `CBM_PROFILE` env string 的啟用判斷委派給 Rust `env_enabled`，維持 `NULL`、empty、`0`、`00` 為 false，`1` 與 `false` 等非零開頭字串為 true。`cbm_profile_active` global state、`cbm_profile_enable()`、clock、elapsed logging、stderr/log sink 與 formatter 仍由 C 負責；此切片不代表 profile runtime 已遷移。
- `cbm_rs_compat_regex_known_flags`、`cbm_rs_compat_regex_match_cap`、`cbm_rs_compat_regex_status`、`cbm_rs_compat_thread_effective_stack_size`、`cbm_rs_compat_aligned_alloc_precheck`：test-only compat deterministic helper ABI。Rust 只固定 pure scalar contract；regex engine、filesystem/process wrapper、thread lifecycle、aligned allocation 實作與 Windows shim 仍保留 C。
- `cbm_rs_arena_init`、`cbm_rs_arena_init_sized`、`cbm_rs_arena_alloc`、`cbm_rs_arena_calloc`、`cbm_rs_arena_strdup`、`cbm_rs_arena_strndup`、`cbm_rs_arena_reset`、`cbm_rs_arena_destroy`、`cbm_rs_arena_total`：test-only arena public-struct parity ABI。這批 API 直接接受 layout-compatible `CBMArena*`，固定 default/min block size、8-byte alignment、block growth、calloc、string helpers、reset 保留首塊、destroy 歸零與 total accounting；Rust 會配置 arena blocks，因此由 Rust init 的 arena 必須以 Rust reset/destroy 釋放，不可呼叫 C `cbm_arena_destroy`，也不可把 C init 的 arena 交給 Rust destroy。variadic `cbm_arena_sprintf` 不暴露 Rust FFI。
- `cbm_rs_store_camel_split_v1`：Store FTS camelCase tokenization ABI。預設 C path 仍使用 C `cbm_camel_split`；設定 `CBM_USE_RUST_STORE_FTS_TOKENIZER=1` 時，C 註冊的 SQLite UDF callback 只將 token 字串計算委派給 Rust helper。Rust 以 raw bytes 對齊既有 contract，寫入 caller-provided buffer 並回傳完整輸出長度；NULL/empty 回傳空字串，輸出固定為「原 identifier + 空白 + split identifier」，只依 ASCII lower/upper 判斷 camel/acronym boundary，並保留 2048-byte buffer fallback/truncation contract。SQLite open/pragma/schema、UDF registration、FTS5 runtime、`sqlite3_context`/`sqlite3_result_text`、CRUD/search/BM25 與 Store runtime 仍由 C 負責；此切片不代表 SQLite runtime 已遷移。
- `cbm_rs_store_resolve_mmap_size_value_v1`：Store `CBM_SQLITE_MMAP_SIZE` resolver ABI。Rust 接受 nullable NUL-terminated C string，不讀 env、不保存指標、不開 SQLite，回傳要套用到 `PRAGMA mmap_size` 的 `int64_t`。NULL/unset、empty、malformed、partial numeric、trailing bytes 與 overflow 回 67108864；合法負值 clamp 成 0；leading `strtoll`-style whitespace 與 `+` 可接受。設定 `CBM_USE_RUST_STORE_MMAP_RESOLVER=1` 時，C `cbm_store_resolve_mmap_size()` 只將已由 `cbm_safe_getenv` 讀入的字串委派給 Rust parser；SQLite open、pragma dispatch、readonly/write 分流與 SQL execution 仍由 C 負責。
- `cbm_rs_store_build_immutable_uri_v1`：Store readonly immutable URI builder ABI。Rust 接受 nullable NUL-terminated path C string，寫入 caller-provided buffer 並回傳完整 URI 長度；null path 回傳 `SIZE_MAX`，短 buffer 會 NUL-terminate 截斷並仍回完整長度。設定 `CBM_USE_RUST_STORE_IMMUTABLE_URI=1` 時，C `build_immutable_uri()` 只把 SQLite `file://...?immutable=1` fallback URI 字串建構委派給 Rust，並以 `len != SIZE_MAX && len < out_sz` 維持既有成功條件。安全字元固定為 `A-Z a-z 0-9 / . - _ ~ :`，backslash 正規化為 slash，其他 byte 以大寫 `%XX` percent-encode，且 `%` 不視為已 escape。SQLite `open_v2`、readonly/no-create probe、immutable fallback timing、WAL/pragma dispatch、UDF registration、FTS/CRUD/search/BM25 與 Store lifetime 仍由 C 負責。
- `cbm_rs_store_schema_manifest_*_v1/v2`、`cbm_rs_store_connection_manifest_*_v1`：test-only Store schema/connection manifest ABI。Rust 回傳 static NUL-terminated manifest 字串，呼叫端不得 free；`entries` API 在 null output、負 cap 或 cap 不足時回 `-1`。connection manifest 固定 open.memory/path_write/path_query、read probe、immutable fallback、read-only no-create、WAL、mmap env、bulk begin/end、open-time best-effort WAL checkpoint，以及 public `cbm_store_checkpoint()` 的 `SQLITE_CHECKPOINT_PASSIVE` + `PRAGMA optimize;` filtered mode contract；`*_for_mode_v1` 只依 mode bitset 過濾 static metadata，不開 SQLite、不執行 pragma、不改變 Store runtime。
- `cbm_rs_store_glob_to_like_v1`、`cbm_rs_store_ensure_case_insensitive_v1`、`cbm_rs_store_strip_case_flag_v1`、`cbm_rs_store_like_hint_count_v1`、`cbm_rs_store_like_hint_v1`：Store search pattern 純字串 ABI。Rust 接受 nullable NUL-terminated pattern，glob/case/hint output 都寫入 caller-provided buffer 並回傳完整長度；短 buffer 會 NUL-terminate 截斷，`glob` null 與不存在的 hint index 回 `SIZE_MAX`。LIKE hints 固定 3-byte 最小 literal、255-byte segment cap、任何 `|`（含 escaped pipe）直接放棄、regex meta flush segment、反斜線下一個 byte 視為 literal、`max_out <= 0` 回 0。設定 `CBM_USE_RUST_STORE_SEARCH_PATTERN=1` 時，C 只委派 `cbm_glob_to_like()`、`cbm_extract_like_hints()`、`cbm_ensure_case_insensitive()` 與 `cbm_strip_case_flag()` 的 byte-level helper；SQL builder、LIKE bind lifetime、FTS/BM25、SQLite prepare/step/finalize、REGEXP UDF、CRUD/search runtime 與 Store ownership 仍留 C。
- `cbm_rs_store_qn_to_package_v1`、`cbm_rs_store_qn_to_top_package_v1`、`cbm_rs_store_is_test_file_path_v1`、`cbm_rs_store_hop_to_risk_v1` 與 `cbm_rs_store_risk_label_v1`：Store architecture helper ABI。Rust 接受 nullable C string/整數與 caller-provided buffer，保留 `qn_to_package()` / `qn_to_top_package()` / `is_test_file_path()` / `hop_to_risk()` / `risk_label()` 的 byte-level contract。`cbm_rs_store_qn_to_package_v1` / `cbm_rs_store_qn_to_top_package_v1` 使用 `bufsize == 0` / `buf == NULL` 時仍回傳完整長度；短 buffer 只在可寫入時截斷並 NUL-terminate。`is_test_file_path` case-sensitive 尋找 `test` 子字串，`hop_to_risk` 固定 `1 -> 0`、`2 -> 1`、`3 -> 2`、其他 -> 3。設定 `CBM_USE_RUST_STORE_ARCH_HELPERS=1` 時，C 只將這五個 helper 的純邏輯委派給 Rust，`risk`、package 轉換、test path 判斷與字串輸出均維持純 helper 契約；Store runtime、SQLite、BFS/search/CRUD 與 ownership 仍保留 C。
- `cbm_rs_store_normalize_arch_path_v1`：給 `CBM_USE_RUST_STORE_ARCH_PATH_SCOPE=1` 使用的 architecture path scope 正規化 ABI。Rust 只產生 C `arch_path_prepare()` 的 `norm_out`：跳過前導空白、一次 `./` 與前導 `/`，以 `cap - 1` 截斷，再移除尾端空白或 `/` 並折疊重複 `/`；null、空白或正規化後空字串回 `SIZE_MAX`。`like_out` 組裝、SQLite scope query 與 bind lifetime 仍由 C 負責。
- `cbm_rs_store_file_ext_lower_v1`：給 `CBM_USE_RUST_STORE_FILE_EXT=1` 使用的副檔名轉換 ABI。Rust 擷取最後一個 `.` 起的副檔名並做 ASCII 小寫；無副檔名、null input、輸出 buffer 不可寫入，或副檔名長度不符合 C 的固定 TLS buffer 容量時回 `SIZE_MAX`。C 仍持有 TLS buffer，並負責 `ext_to_lang()` 查表與全部 Store runtime。
- `cbm_rs_store_ext_lang_kind_v1`：給 `CBM_USE_RUST_STORE_LANGUAGE_MAP=1` 使用的 extension-to-language index ABI。Rust 接受 nullable、僅於呼叫期間借用的 NUL-terminated extension，將既有 C `ext_lang_table` 的 44 個位置回傳為 `0..43`；null、未知或大小寫不符回 `-1`，不保存指標且不回傳 Rust-owned 字串。C 僅接受範圍內索引，並以既有 C 靜態 `ext_lang_table[kind].lang` 回傳指標；任何非預期 ABI 值回退原 C 查表。SQLite architecture query、計數/排序、allocation 與 Store ownership 仍留在 C。
- `cbm_rs_cypher_lex_token_count_v1` / `cbm_rs_cypher_lex_tokens_v1` / `cbm_rs_cypher_lex_summary_v1`：test-only Cypher lexer parity ABI。Rust 以 `*const u8 + len` 借用 input，固定 `src/cypher/cypher.c` 的 token kind、byte offset、token text length、keyword case-insensitive lookup、註解/空白略過、number / `..` 邊界，以及 string literal escape / 4095-byte truncation contract；null input 或負 len 回傳錯誤。`tokens_v1` 只輸出 scalar metadata，不回傳 Rust-owned token text；`summary_v1` 寫入 caller-provided buffer 並回傳完整 summary 長度，短 buffer 會 NUL-terminate 截斷。此 ABI 不建立 AST、不執行 query、不接 production opt-in；lexer/parser/evaluator/executor 與錯誤訊息仍由 C 負責。
- `cbm_rs_cypher_parse_summary_v1`：test-only Cypher normalized AST summary parity ABI。Rust 以 `*const u8 + len` 借用 input，寫入 caller-provided buffer 並回傳完整 summary 長度；短 buffer 會 NUL-terminate 截斷，`buf == NULL` 或 `bufsize == 0` 時只回傳完整長度且不寫入。null input、負 len 或 parse failure 回傳 `SIZE_MAX`。summary 只固定 parser AST shape、OPTIONAL MATCH、UNION ALL、EXISTS direction、WITH/post-WHERE 與 RETURN projection 等 contract；不保存 C 指標、不回傳 Rust-owned AST、不執行 query、不接 production opt-in。完整 parser/normalizer、evaluator、executor、planner 與 human-readable error text 仍由 C 負責。
- `cbm_rs_cypher_scalar_func_index_v1`：給 `CBM_USE_RUST_CYPHER_SCALAR_FUNC=1` 使用的 Cypher 單參數 function canonical ABI。Rust 以 NUL-terminated C string 輸入，回傳 names 索引（`labels`、`type`、`id`、`keys`、`properties`、`toInteger`、`toFloat`、`toBoolean`、`size`、`length`、`trim`、`ltrim`、`rtrim`、`reverse`）或 `-1`；C helper 依索引回傳本地 static `names[]` 指標，並維持原有 ownership 語意。`cbm_parse`、AST normalizer、evaluator 與 executor 仍為 C 路徑。
- `cbm_rs_cypher_multiarg_func_index_v1`：給 `CBM_USE_RUST_CYPHER_MULTIARG_FUNC=1` 使用的 Cypher 多參數 function canonical ABI。Rust 以 NUL-terminated C string 輸入，回傳 `coalesce`、`substring`、`replace`、`left`、`right` 名稱索引或 `-1`；C helper 依索引回傳本地 static `names[]` 指標，並維持 parser/AST normalizer/evaluator/executor 的 C 路徑。
- `cbm_rs_cypher_aggregate_func_index_v1`：給 `CBM_USE_RUST_CYPHER_AGG_FUNC=1` 使用的 Cypher aggregate token-name ABI。Rust 接受 C `cbm_token_type_t` 整數，將 `TOK_COUNT` / `TOK_SUM` / `TOK_AVG` / `TOK_MIN_KW` / `TOK_MAX_KW` / `TOK_COLLECT` 映射到 C `agg_func_name()` 的本地 `names[]` 索引，未知 token 回 `-1`。C helper 仍回傳自己的 static aggregate 名稱字串，並保留 parse item 建立、ORDER BY formatting、aggregation evaluator/executor、AST normalizer 與 query result 行為在 C。
- `cbm_rs_cypher_string_func_index_v1`：給 `CBM_USE_RUST_CYPHER_STRING_FUNC=1` 使用的 Cypher string function token-name ABI。Rust 接受 C `cbm_token_type_t` 整數，將 `TOK_TOLOWER` / `TOK_TOUPPER` / `TOK_TOSTRING` 映射到 C `str_func_name()` 的本地 `names[]` 索引，未知 token 回 `-1`。C helper 仍回傳自己的 static string function 名稱字串，並保留 parse item 建立、scalar/string evaluator、AST normalizer 與 query result 行為在 C。
- `cbm_rs_cypher_single_char_kind_v1`：給 `CBM_USE_RUST_CYPHER_LEX_SINGLE_CHAR=1` 使用的 Cypher 單字元 lexer classifier ABI。Rust 接受 `0..=255` 的整數 byte，回傳 `(`、`)`、`[`、`]`、`-`、`>`、`<`、`:`、`.`、`{`、`}`、`*`、`,`、`=`、`|` 對應的既有 C token enum；未知 byte 回 `TOK_EOF`，超出範圍回 `-1`。C `lex_single_char()` 只接受 `TOK_EOF`、`TOK_LPAREN..TOK_EQ` 或 `TOK_PIPE`，其他結果回退既有 C `switch`。Rust 不借用、不保存、不回傳 C 指標；token allocation、position、two-character tokens、lexer/parser/evaluator/executor 與錯誤行為仍在 C。
- `cbm_rs_cypher_two_char_kind_v1`：給 `CBM_USE_RUST_CYPHER_LEX_TWO_CHAR=1` 使用的 Cypher 雙字元 lexer classifier ABI。Rust 接受兩個各自位於 `0..=255` 的整數 byte，將 `!=`／`<>`、`=~`、`>=`、`<=`、`..` 映射到既有 C token enum；未知 pair 回 `TOK_EOF`，任一超出範圍回 `-1`。C `lex_try_two_char()` 只接受 `TOK_NEQ`、`TOK_EQTILDE`、`TOK_GTE`、`TOK_LTE`、`TOK_DOTDOT`，並以 C-owned text/token allocation 推進 cursor；其他結果回退既有 C pair table。Rust 不借用、不保存、不回傳 C 指標；input/cursor、parser/evaluator/executor 與錯誤行為仍在 C。
- `cbm_rs_log_parse_level_value` / `cbm_rs_log_parse_format_value`：給 `CBM_USE_RUST_LOG_ENV_PARSE=1` 使用的 logger environment value parser ABI。Rust 接受 nullable NUL-terminated value 與 C current enum scalar，僅回傳 scalar，不保存 C 指標。level 接受 ASCII case-insensitive `debug`／`info`／`warn`／`error`／`none`，或完整 C `strtol(..., 10)` 相容整數 `0..4`；format 只接受 ASCII case-insensitive `text`／`json` 且不 trim 空白。null、空、未知、尾隨字元與越界值皆回 current。C 仍呼叫 `getenv()`、驗證合法 enum、設定 global logger state、處理 sink/stderr/lifecycle；預設 C parser 保留。
- `cbm_rs_mcp_jsonrpc_request_summary_v1`：test-only MCP JSON-RPC request codec parity ABI。Rust 以 `*const u8 + len` 借用 input，寫入 caller-provided buffer 並回傳完整 envelope summary 長度；短 buffer 會 NUL-terminate 截斷，`buf == NULL` 或 `bufsize == 0` 時只回傳完整長度且不寫入。null input、負 len、非 object root、invalid JSON 或缺少 string `method` 回傳 `SIZE_MAX`。summary 固定 C parser 的 `jsonrpc` 預設為 `2.0`、numeric/string/other id 分類、notification 無 id、`params` presence/kind、whitespace、duplicate key first-wins、UTF-8 驗證與 string escape handling；不保存 C 指標、不分配 Rust-owned response、不處理 Content-Length transport、不 dispatch tool。公開 MCP stdout/stderr、JSON-RPC response formatting、tools/list schema 與 14 個 tool handler 仍由 C 負責。
- `cbm_rs_mcp_tool_index_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP tool name lookup ABI。Rust 接受 nullable NUL-terminated `name`，依 C `TOOLS[]` 順序回傳 0..13 的 tool index，未知、空字串、大小寫不符或 null 回傳 -1。此 opt-in 只委派 `cbm_mcp_tool_input_schema()` 的 name -> index 純查找；input schema JSON、title、description、tools/list serialization、tool dispatch 與 handlers 仍由 C `TOOLS[]` / yyjson 負責。
- `cbm_rs_mcp_tool_dispatch_index_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP tool dispatch classifier ABI。Rust 接受 nullable NUL-terminated `name`，依 C `TOOLS[]` 順序回傳 handler index，並將相容 alias `trace_call_path` 對應到 `trace_path` 的 index；未知、空字串、大小寫不符或 null 回傳 -1。C 端 `cbm_mcp_handle_tool()` 只在 Rust index 與本地 `TOOLS[]` 驗證通過時用 switch 選擇既有 C handler，否則 fallback 到原 `strcmp` chain；handler-specific 參數解析、tool implementation、response wrapping、logging、framing 與 server lifecycle 仍由 C 負責。
- `cbm_rs_mcp_tool_count_v1` / `cbm_rs_mcp_tool_name_v1` / `cbm_rs_mcp_tool_title_v1` / `cbm_rs_mcp_tool_description_v1` / `cbm_rs_mcp_tool_input_schema_v1` / `cbm_rs_mcp_tool_output_schema_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP tool manifest ABI。Rust 固定 C `TOOLS[]` 的 tool count、index -> name、index -> title、index -> description、index -> input schema JSON，以及共用 output schema JSON，`tool_name_v1` / `tool_title_v1` / `tool_description_v1` / `tool_input_schema_v1` / `tool_output_schema_v1` 寫入 caller-provided buffer 並回傳完整 byte 長度；index 超界回 `SIZE_MAX`，短 buffer 會 NUL-terminate 截斷。此 opt-in 只讓 `tools/list` 的 `name` / `title` / `description` / `inputSchema` / `outputSchema` 欄位在 Rust manifest 與本地 C 常數完全一致時由 Rust 輸出；JSON parse/copy/serialization、dispatch 與 handlers 仍由 C/yyjson 負責。
- `cbm_rs_mcp_tools_page_bounds_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `tools/list` page bounds ABI。Rust 接受 offset、limit、include-next-cursor 與 tool_count，寫入 caller-provided `CbmRsMcpToolsPageBoundsV1`，固定 C `cbm_mcp_tools_list_range()` 的 offset/limit clamp、end 與 nextCursor contract；out 為 null 回 -1。此 opt-in 只委派 pagination range 計算；tool metadata、input/output schema JSON、tools/list serialization、JSON-RPC response wrapping、dispatch 與 handlers 仍由 C/yyjson 負責。
- `cbm_rs_mcp_tools_cursor_offset_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `tools/list` cursor parser ABI。Rust 驗證完整 params JSON、擷取首個 `cursor` 字串，並對齊 C 的 `strtol` 完整消耗、非負與 `tool_count` clamp 語意；null、無效 JSON、非 object 或缺少 cursor 回 0，cursor 存在但無效時回 `tool_count`。tools/list JSON 建構、response wrapping、transport、dispatch 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_tools_list_json_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `tools/list` result JSON builder ABI。Rust 接受 offset、limit 與 include-next-cursor，產生 compact result object `{"tools":[...],"nextCursor":"..."}`，固定 14 tools 的 name/title/description/inputSchema/outputSchema 欄位順序、schema object embedding、pagination clamp、length-only 與短 buffer NUL 截斷 contract。C 端 `cbm_mcp_tools_list_range()` 只在 ABI 成功時使用 Rust JSON，否則 fallback 到原 yyjson builder；JSON-RPC response wrapper、Content-Length framing、server loop、tool dispatch 與 14 handlers 仍由 C/yyjson 負責。
- `cbm_rs_mcp_content_length_header_matches_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP Content-Length framed-message classifier ABI。Rust 接受 nullable NUL-terminated line，若精準以 `Content-Length:` 開頭回傳 1，否則回傳 0；大小寫、前導空白與缺少冒號都不符合。C 端 `cbm_mcp_server_run()` 只在 opt-in 下用此 classifier 決定是否進入 framed branch；header length parsing、body allocation/read、response framing、poll loop、idle eviction、server lifecycle、dispatch 與 handlers 仍由各自既有路徑負責。
- `cbm_rs_mcp_content_length_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP Content-Length header parser ABI。Rust 接受 nullable NUL-terminated header line 與 max length，固定 C event loop 既有 gate：header 必須精準以 `Content-Length:` 開頭，數值區對齊 `strtol(..., NULL, 10)` 的前導空白、正負號與尾端殘留容忍；回傳 `>0 && <= max_len` 的 body 長度，其他情況回 0。此 opt-in 只委派 header length parsing/gating；讀取 body、呼叫 `cbm_mcp_server_handle()`、poll loop、idle eviction 與 server lifecycle 仍由 C 負責。
- `cbm_rs_mcp_content_length_raw_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 raw Content-Length parser ABI。Rust 接受 nullable NUL-terminated header line，成功時回傳對齊 `strtol(..., NULL, 10)` 的原始數值；null、前綴不符或無數字時回傳 -1。C 用它區分無效 header 與語法有效但超過上限的 frame；後者不交給 handler，但 default C 與 Rust opt-in 都會消耗其 header/body，避免下一個 frame 失同步。Rust 不配置或讀取 body；body I/O、排空、`cbm_mcp_server_handle()`、response framing、poll loop、server lifecycle、dispatch 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_content_length_header_is_blank_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP Content-Length header/body separator ABI。Rust 接受 nullable NUL-terminated header line，移除尾端 CR/LF 後若為空字串回傳 1，其他情況回傳 0；null 回傳 0。此 opt-in 只委派 `handle_content_length_frame()` 跳過 header 與 body 之間空行的 byte-level 判斷；`getline`、body allocation/read、`cbm_mcp_server_handle()`、poll loop、idle eviction、server lifecycle、dispatch 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_content_length_response_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP Content-Length response framing ABI。Rust 接受 nullable NUL-terminated response JSON，產生 `Content-Length: <byte_len>\r\n\r\n<response>` framed bytes，寫入 caller-provided buffer 並回傳完整 byte 長度；`resp == NULL` 回傳 `SIZE_MAX`，短 buffer 會 NUL-terminate 截斷。C 端 `handle_content_length_frame()` 只在 ABI 成功時用 `fwrite` 輸出 Rust frame，否則 fallback 到原 `fprintf`；body read、header blank line handling、`cbm_mcp_server_handle()`、poll loop、idle eviction、server lifecycle、dispatch 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_parse_file_uri_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP file URI parser ABI。Rust 接受 nullable NUL-terminated URI，僅接受 `file://` prefix，保留 raw path 與 percent encoding，並對 `file:///C:/...` / `file:///d:/...` 類 Windows drive path 移除一個 leading slash；成功時寫入 caller-provided buffer 並回傳完整 path byte 長度，短 buffer 會 NUL-terminate 截斷，invalid URI 或 null 回傳 `SIZE_MAX`。C 端 `cbm_parse_file_uri()` 只在 opt-in 下委派此純字串轉換；caller buffer ownership、錯誤時清空輸出、MCP root/session lifecycle 與 URI 使用端仍由 C 負責。
- `cbm_rs_mcp_method_kind_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP method dispatch classifier ABI。Rust 接受 nullable NUL-terminated method string，回傳 stable enum：`0` unknown、`1` initialize、`2` ping、`3` tools/list、`4` tools/call、`5` notifications/cancelled；大小寫、尾端空白與未知 method 都回 unknown。C 端只在 opt-in 下用此 classifier 選擇既有 dispatch branch；initialize side effects、ping/tools/list/tools/call handlers、cancellation side effect、logging、response wrapping、Content-Length transport、server lifecycle 與 14 個 tool handlers 仍由 C 負責。
- `cbm_rs_mcp_method_not_found_error_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP Method not found error object builder ABI。Rust 產生固定 compact error object `{"code":-32601,"message":"Method not found"}`，支援 caller-provided buffer、length-only 與短 buffer NUL 截斷。C 端 `cbm_mcp_server_handle()` 只在 unknown method branch 的 error object 建構使用 Rust；request id echo、JSON-RPC response wrapper、logging、Content-Length transport、server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_parse_error_message_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP JSON-RPC parse error message builder ABI。Rust 產生固定文字 `Parse error`，支援 caller-provided buffer、length-only 與短 buffer NUL 截斷。C 端 `cbm_mcp_server_handle()` 只在 request parse 失敗時使用 Rust 建構 error message；numeric-id JSON-RPC error response formatting、Content-Length transport、logging、server lifecycle、dispatch 與 handlers 仍由 C/既有 Rust formatter 分工路徑負責。
- `cbm_rs_mcp_ping_result_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `ping` result object builder ABI。Rust 產生固定 compact result object `{}`，支援 caller-provided buffer、length-only 與短 buffer NUL 截斷。C 端 `cbm_mcp_server_handle()` 只在 `ping` branch 的 `result_json` 建構使用 Rust；JSON-RPC response wrapper、request logging、Content-Length transport、server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_tools_call_default_arguments_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `tools/call` no-params default arguments builder ABI。Rust 產生固定 compact arguments object `{}`，支援 caller-provided buffer、length-only 與短 buffer NUL 截斷。C 端 `cbm_mcp_server_handle()` 只在 `tools/call` request 沒有 `params` 時使用 Rust 建構 handler arguments fallback；有 `params` 時仍走 `cbm_rs_mcp_tools_call_arguments_v1`，tool dispatch、handler-specific 參數解析、JSON-RPC response wrapper、logging、framing、server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_missing_tool_name_message_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP missing-tool-name error text builder ABI。Rust 產生固定文字 `missing tool name`，支援 caller-provided buffer、length-only 與短 buffer NUL 截斷。C 端 `cbm_mcp_handle_tool()` 只在 `tool_name == NULL` branch 使用 Rust 建構 error text；ABI 異常或 buffer 放不下時 fallback 到原 literal，`cbm_mcp_text_result()`、tool dispatch、handler-specific 參數解析、JSON-RPC response wrapper、logging、framing、server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_missing_project_error_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP missing-project JSON error object builder ABI。Rust 產生固定 compact object `{"error":"missing required argument: project","hint":"Pass the project as the \"project\" argument, e.g. {\"project\":\"<name from list_projects>\"}. Run list_projects to see indexed projects."}`，支援 caller-provided buffer、length-only 與短 buffer NUL 截斷。C 端 `build_missing_project_error()` 只在 project argument 缺失時使用 Rust 建構 C-owned heap string；ABI 異常、配置失敗或寫入長度不符時 fallback 到原 literal。`build_project_list_error()` 的 cache-dir 掃描、available projects list、`cbm_mcp_text_result()` result formatting、Store resolution、JSON-RPC wrapper、logging、transport、server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_project_not_found_message_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP project-not-found error text builder ABI。Rust 產生固定文字 `project not found`，支援 caller-provided buffer、length-only 與短 buffer NUL 截斷。C 端 `build_project_not_found_message()` 只在無法解析或開啟目標 project 時使用 Rust 建構 C-owned heap string；ABI 異常、配置失敗或寫入長度不符時 fallback 到原 literal。project list 掃描、Store resolution、`cbm_mcp_text_result()` result formatting、JSON-RPC wrapper、logging、transport、server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_project_list_error_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP project-list error JSON builder ABI。Rust 依 `count` 產生 `build_project_list_error()` 的兩種固定 shape：`count > 0` 時輸出 `{"error":"...","hint":"Use list_projects ...","available_projects":[...],"count":N}`；否則輸出 `{"error":"...","hint":"No projects indexed yet. Call index_repository first."}`。支援 caller-provided buffer、length-only 與短 buffer NUL 截斷；`reason == NULL` 或 `count > 0 且 projects_csv == NULL` 回傳 `SIZE_MAX`。C 端 `build_project_list_error()` 仍負責 cache dir 掃描與 `collect_db_project_names()`，只在 ABI 成功時採用 Rust JSON，否則 fallback 到原 `snprintf`；`build_no_store_error()` dispatch、Store resolution、`cbm_mcp_text_result()`、JSON-RPC wrapper、transport、server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_unknown_tool_message_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP unknown-tool error text builder ABI。Rust 產生 `unknown tool: <name>`，支援 caller-provided buffer、length-only 與短 buffer NUL 截斷；`tool_name == NULL` 回傳 `SIZE_MAX`。C 端 `cbm_mcp_handle_tool()` 只在 tool dispatch 找不到 handler 時使用 Rust 建構 error text；長度放不進既有 `CBM_SZ_256` stack buffer 或 ABI 異常時 fallback 到原 `snprintf`，`cbm_mcp_text_result()`、tool dispatch、handler-specific 參數解析、JSON-RPC response wrapper、logging、framing、server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_initialize_protocol_version_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP initialize protocol version selection ABI。Rust 以 `*const u8 + len` 借用 params JSON，寫入 caller-provided buffer 並回傳完整版本字串長度；null input、負 len、invalid JSON、root 非 object、缺少 `protocolVersion`、首個 `protocolVersion` 非 string 或 unsupported string 都 fallback 到最新支援版本 `2025-11-25`。若首個 `protocolVersion` 精準等於支援清單之一，回傳該版本；duplicate key 採 first-wins。此 opt-in 只委派版本選擇；initialize response JSON 組裝、`serverInfo`、`capabilities.tools.listChanged`、yyjson serialization/ownership、JSON-RPC dispatch 與 Content-Length transport 仍由 C 負責。
- `cbm_rs_mcp_initialize_response_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP initialize result JSON builder ABI。Rust 以 `*const u8 + len` 借用 initialize params JSON，套用同一套 protocol version selection contract，產生 compact result object：`protocolVersion`、`serverInfo.name/version` 與 `capabilities.tools.listChanged=false`，寫入 caller-provided buffer 並回傳完整 byte 長度；短 buffer 會 NUL-terminate 截斷。C 端 `cbm_mcp_initialize_response()` 只在 ABI 成功時使用 Rust result JSON，否則 fallback 到原 yyjson builder；JSON-RPC response wrapper、initialize dispatch side effects（update check、session detection、auto-index）、Content-Length transport、server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_jsonrpc_parse_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP JSON-RPC request parse ABI。Rust 只解析 request envelope，透過 `CbmRsMcpJsonRpcParseOutV1` 回傳 scalar metadata 與各欄位完整長度，並寫入 caller-provided buffers；C wrapper 仍用 `malloc` 建立 `cbm_jsonrpc_request_t` 的 `jsonrpc`、`method`、`id_str` 與 `params_raw`，所以 `cbm_jsonrpc_request_free()` ownership 不變。此 opt-in 固定 root object、required string `method`、missing/non-string `jsonrpc` 預設 `2.0`、int/string/other id、notification、duplicate key first-wins、Unicode escape、invalid UTF-8 rejection 與短 buffer 截斷。`params_raw` 在 Rust path 是原始 JSON value slice 的 C-owned copy，不要求 yyjson-style canonical minify；後續 tool argument parsing 仍由 C/yyjson 執行。response formatting、Content-Length framing、tool schema、tool dispatch/handlers、stdout/stderr 與 server lifecycle 仍留 C。
- `cbm_rs_mcp_tools_call_name_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `tools/call` dispatch name ABI。Rust 只解析 params object 的首個 `name` 字串，寫入 caller-provided buffer 並回傳完整 byte 長度；null input、負 len、invalid JSON、root 非 object、缺少 `name` 或首個 `name` 非 string 回傳 `SIZE_MAX`。C wrapper 仍負責 `malloc`/`free` 並回傳 C-owned NUL 結尾字串；`arguments`、handler-specific 參數、tool schema、response formatting 與 Content-Length transport 仍由 C/yyjson 負責。
- `cbm_rs_mcp_tools_call_arguments_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `tools/call` arguments ABI。Rust 只解析 params object 的首個 `arguments` JSON value，回傳原始 JSON slice 的完整 byte 長度並寫入 caller-provided buffer；缺少 `arguments` 時回傳 `{}`，null input、負 len、invalid JSON 或 root 非 object 回傳 `SIZE_MAX`。C wrapper 仍負責配置/釋放 C-owned NUL 結尾字串；handler-specific 參數解析、validation、tool schema、response formatting、Content-Length transport 與 14 個 handlers 仍由 C/yyjson 負責。
- `cbm_rs_mcp_get_string_arg_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP handler args string extraction ABI。Rust 接受 `*const u8 + len` 的 args JSON 與 NUL-terminated key，只解析 root object 的首個同名欄位；成功時回傳 string value 的完整 byte 長度並寫入 caller-provided buffer，key 不存在、value 非 string、root 非 object、invalid JSON、invalid UTF-8、空 key 或 null input/key 回傳 `SIZE_MAX`。C wrapper 只在 Rust 成功時配置 C-owned NUL 字串，否則 fallback 到原 yyjson helper；int/bool argument helpers、project alias resolution、handler-specific validation、response/transport/server lifecycle 與 14 個 handlers 仍由 C 負責。
- `cbm_rs_mcp_get_int_arg_v1` / `cbm_rs_mcp_get_bool_arg_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP handler args int/bool extraction ABI。Rust 接受 `*const u8 + len` 的 args JSON 與 NUL-terminated key，只解析 root object 的首個同名欄位；int helper 成功時回傳 int value，missing、非 int、invalid JSON、invalid UTF-8、空 key 或 null input/key 回傳 caller-provided default；bool helper 只有首個同名欄位為 JSON bool true 時回 1，false 或任何 missing/非 bool/invalid case 回 0。C wrapper 在 opt-in 下只委派 shared extraction；project alias resolution、handler-specific range/default validation、response/transport/server lifecycle 與 14 個 handlers 仍由 C 負責。
- `cbm_rs_mcp_edge_type_valid_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `search_graph.relationship` edge type validator ABI。Rust 接受 nullable NUL-terminated C string，回傳 1 表示 value 只包含 ASCII uppercase `A-Z` 或 `_` 且長度不超過 64 bytes；空字串目前對齊 C helper 視為有效，null、過長、數字、小寫、空白、dash 與非 ASCII byte 回 0。C 端只在 `validate_edge_type()` 中委派此 pure classifier；`search_graph` 參數解析、project/index validation、store search/query planning、error result wrapping、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_search_path_arg_valid_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `search_code` root path / `file_pattern` shell-safety validator ABI。Rust 接受 nullable NUL-terminated C string，回傳 1 表示 argument 不含會破壞 quoting 的 denylist byte；null 回 0。denylist 固定為 single quote、double quote、`;`、`|`、`$`、backtick、`<`、`>`、LF、CR，且非 Windows 也拒絕 backslash；`&` 與空字串維持 C helper 行為視為有效。C 端只在 `validate_search_path_arg()` 中委派此 pure classifier；project root resolution、grep command construction、temp file handling、regex compilation、search execution、result parsing、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_search_args_valid_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `search_code` root/file 組合 validator ABI。Rust 接受 nullable NUL-terminated `root_path` 與 `file_pattern`，固定 `validate_search_args()` contract：root path 必填且需通過 path validator；file_pattern 可省略，但若提供也要通過同一 denylist。C 端只在 `validate_search_args()` 中委派此 pure combiner；project root resolution、grep command construction、temp file handling、regex compilation、search execution、result parsing、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_strip_root_prefix_offset_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `search_code` grep result path root-prefix stripping ABI。Rust 接受 nullable NUL-terminated `path` / `root` 與 caller 傳入的 `root_len`，回傳可從原 `path` 安全跳過的 byte offset；0 表示不移除 prefix。契約對齊 C `strip_root_prefix()`：只做 byte prefix 比對，prefix 後若正好是 `/` 再多跳過 1 byte，不檢查 path component boundary；exact root 回 `root_len`；`root_len == 0` 時會因空 prefix 比對成功而跳過 leading `/`；`root_len > path/root length` 視為無效參數並回 0。C 端仍從原 `path` 借用指標，不接收 Rust-owned string；grep execution、path filter、result parsing、JSON/transport、server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_search_mode_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `search_code.mode` classifier ABI。Rust 接受 nullable NUL-terminated C string，回傳 0 表示 compact/default、1 表示 full、2 表示 files；null、空字串、`compact`、大小寫不符、尾端空白或未知 mode 都回 0，對齊 C helper fallback。C 端只在 `parse_search_mode()` 中委派此 pure classifier；handler args extraction、grep command construction、context/source/file-list result shaping、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_index_mode_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `index_repository.mode` classifier ABI。Rust 接受 nullable NUL-terminated C string，回傳 0 表示 full/default、1 表示 moderate、2 表示 fast、3 表示 cross-repo-intelligence；null、空字串、`full`、大小寫不符、尾端空白或未知 mode 都回 0，對齊 C fallback。C 端只在 `parse_index_repository_mode()` 中委派此 pure classifier；repo_path/name/persistence args extraction、cross-repo matching、pipeline creation/run、artifact/degraded response shaping、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_trace_mode_edge_mask_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `trace_path.mode` default edge-type classifier ABI。Rust 接受 nullable NUL-terminated C string，回傳 bitmask：bit0 `CALLS`、bit1 `DATA_FLOWS`、bit2 `HTTP_CALLS`、bit3 `ASYNC_CALLS`、bit4..9 為 `CROSS_*`；null、空字串、`calls`、大小寫不符、尾端空白或未知 mode 都回 `CALLS`，`data_flow` 回 `CALLS|DATA_FLOWS`，`cross_service` 回 HTTP/async/data/calls/cross-service edge set。C 端只在 `resolve_trace_edge_types()` 沒有 explicit `edge_types` array 時委派此 pure classifier；explicit edge array parsing/ownership、BFS traversal、data-flow arg extraction、risk labels、test filtering、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_sanitize_ascii_in_place_v1`：MCP `search_code` output ASCII sanitizer ABI。#50 已有
  `src/mcp/sanitize_ascii_in_place.c/.h` fallback 與公開
  `void cbm_mcp_sanitize_ascii_in_place(char *input)`；wrapper
  `CBM_USE_RUST_MCP_SANITIZE_ASCII_IN_PLACE=1`（或 CODEC）委派 v1，direct
  `CBM_USE_RUST_MCP_SANITIZE_ASCII_IN_PLACE_ONLY=1`／`mcp-sanitize-ascii-in-place-only` 匯出同名 ABI。
  nullable mutable NUL-terminated C string 的每個 >127 byte 就地改為 `?`，ASCII 與 `0x7f` 保留；null 為
  no-op，first-NUL 後不讀不改。grep execution、source/context reading、JSON building、snippet UTF-8 lossy
  sanitizer、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_search_code_score_v1`：MCP `search_code` deduped result ranking scorer ABI。Rust 接受 nullable NUL-terminated label/file path 與 `in_degree`，回傳 C helper 相同的整數 score：Function/Method 加 10、Route 加 15、vendored/vendor/node_modules 扣 50、test/spec/`_test.` path 扣 5。#39 的 C wrapper 在專屬 flag 或 CODEC 下委派此 ABI；grep execution、graph node lookup、dedup/classification、sort comparator、result JSON、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_search_score_cmp_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `search_code` result score comparator ABI。Rust 接受 left/right score scalar，回傳 C `search_result_cmp()` 相同的 descending comparator delta：left score 較高回負值、right score 較高回正值、相同分數回 0。C 端只在 `search_result_cmp()` 中委派此 pure scalar helper；qsort 呼叫、`search_result_t` ownership、grep execution、graph node lookup、dedup/classification、score 寫入、result JSON、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_search_top_dir_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `search_code` directory distribution top-key ABI。Rust 接受 nullable NUL-terminated file path；若 path 含 `/`，輸出到第一個 slash（含 slash）的 top-level key，否則輸出整個 path；`file == NULL` 回 `SIZE_MAX`，成功時回完整 byte 長度，短 buffer 會 NUL-terminate 截斷。C 端只在 `build_dir_distribution()` 的 per-result top-key 擷取中透過 `search_result_top_dir()` 委派此 pure helper；directory count 累計、yyjson object building、`search_result_t` ownership、grep execution、graph node lookup、dedup/classification、result JSON、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_detect_changes_wants_symbols_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `detect_changes.scope` classifier ABI。Rust 接受 nullable NUL-terminated scope，回傳 1 表示要輸出 impacted symbols：null/default、`symbols` 與 `impact` 啟用；空字串、`files`、大小寫不符、尾端空白與未知值回 0。C 端只在 `detect_changes_wants_symbols()` 中委派此 pure classifier；project/root resolution、base branch validation、git diff/status command、changed file parsing、Store symbol lookup、JSON result building、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_detect_changes_impacted_label_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `detect_changes` impacted symbols label filter ABI。Rust 接受 nullable NUL-terminated label，回傳 1 表示該 node label 應列入 impacted symbols；null、`File`、`Folder` 與 `Project` 回 0，其餘 label（含空字串、大小寫不符與尾端空白）對齊 C helper 皆回 1。C 端只在 `detect_changes_impacted_label()` 中委派此 pure classifier；Store node query、node ownership、changed file parsing、impacted JSON item building、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_detect_changes_status_path_offset_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `detect_changes` changed-files path parser ABI。Rust 接受 nullable NUL-terminated line，回傳應採用的 path 起始 byte offset；對 `git status --porcelain` 會略過 `XY ` 前綴，rename 行（`old -> new`）回傳 destination offset，空路徑與 null 回 `SIZE_MAX`。C 端只在 `detect_changes` changed-files 迴圈中委派此 pure parser，並保留 ABI 異常 fallback 到原 C parser；git command execution、project/root resolution、Store symbol lookup、JSON result building、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_search_line_match_span_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `search_code` tightest-node classifier 單一 node span ABI。Rust 接受 `start_line`、`end_line`、`line`，命中區間時回傳 `end_line - start_line`，未命中回傳 `-1`。C 端只在 `find_tightest_node()` 的 per-node span 判斷中委派此 pure helper，且 Rust 回傳 `-1` 時 fallback 到原 C 條件判斷；node query、hit merge、result ranking、JSON result building、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_search_pick_resolved_index_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `trace_call_path` name-resolution tie 判斷 ABI。Rust 接受 `long` score 陣列與 `count`，回傳 best index，並透過 `ambiguous_out` 回傳 top score 是否有多個候選；invalid args（null `ambiguous_out`、`count<=0`、null scores）回 `-1` 並把 ambiguous 清為 0。C 端只在 `pick_resolved_node()` 的 best-index/ambiguous 計算中委派此 pure helper，且 ABI 異常時 fallback 到原 C 兩段式比較與 top_count 邏輯；score 計算、BFS traversal、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_search_pick_tightest_index_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `search_code` tightest-node classifier 最小有效 span 選擇 ABI。Rust 接受 `int` span 陣列與 `count`，回傳最小且非負 span 的第一個 index；若 spans 全為負值或參數無效（`count<=0`、null spans）回 `-1`。C 端只在 `find_tightest_node()` 的 best-index 選擇中委派此 pure helper，且 ABI 異常時 fallback 到原 C 逐筆比較邏輯；node query、hit merge、result ranking、JSON result building、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_utf8_is_cont_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP UTF-8 continuation-byte classifier ABI。Rust 接受 `int` byte（只取低 8-bit），符合 `10xxxxxx` 回 1，否則回 0。C 端只在 `utf8_is_cont()` 中委派此 pure helper；`sanitize_utf8_lossy.c` 的 fallback loop 會使用此 helper，wrapper 則優先走 `cbm_rs_mcp_sanitize_utf8_lossy_v1`；snippet/source 讀取、JSON result building、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_node_resolution_score_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP trace/snippet candidate resolution scoring ABI。Rust 接受 nullable NUL-terminated label 與 start/end line，回傳 `Function`/`Method` 最高、其他非 `Module`/`File` label 居中、`Module`/`File`/null 最低的 label-tier score，再加上非負 line span；大小寫精準比對，負 span 視為 0。C 端只在 `node_resolution_score()` 中委派此 pure scorer；`pick_resolved_node()` 的 top-score tie/ambiguity、candidate query、BFS/snippet response shaping、node ownership、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_adr_mode_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `manage_adr.mode` classifier ABI。Rust 接受 nullable NUL-terminated C string，回傳 0 表示 get/default、1 表示 update/store、2 表示 sections；null、空字串、`get`、未知、大小寫不符或尾端空白都回 0。C 端只在 `parse_adr_mode()` 中委派此 pure classifier；project/store resolution、legacy file migration、ADR read/write、sections JSON building、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_adr_sections_json_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `manage_adr(mode=sections)` sections JSON array builder ABI。Rust 接受 nullable NUL-terminated ADR markdown content，產生 compact JSON array；null/empty 輸出 `[]`，逐行移除行尾 `\r`，只接受第一個 byte 為 `#` 且非空的行，單一 header 最多保留 1023 bytes，並由 Rust JSON string serializer 處理 quote/backslash/control 與 invalid UTF-8 lossy replacement。支援 caller-provided buffer、length-only 與短 buffer NUL 截斷。C 端只在 `adr_list_sections_from_content()` 中委派此 sections array builder，並以 `yyjson_mut_rawncpy()` 複製 Rust output；project/store resolution、legacy file migration、ADR read/write、`cbm_mcp_text_result()` response wrapping、JSON-RPC wrapper、transport、server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_bm25_build_match_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `search_graph.query` BM25 FTS5 MATCH builder ABI。Rust 接受 nullable NUL-terminated query 與 caller-owned output buffer；null query、null buffer 或過小 buffer 對齊 C helper 直接回 0 且不寫入。有效 query 只保留 ASCII alnum/underscore token，以 ` OR ` 串接；輸出空間不足時停在上一個完整 token，寫入 NUL-terminated output 並回傳 token 數。C 端只在 `bm25_build_match()` 中委派此 pure tokenizer/serializer；SQLite/FTS5 query execution、BM25 ranking/boosting、file pattern LIKE、result JSON、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_bm25_file_pattern_like_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `search_graph.file_pattern` BM25 side SQL LIKE pattern builder ABI。Rust 接受 nullable NUL-terminated file pattern 與 caller-owned output buffer，先沿用 Store glob-to-LIKE byte-level helper；若原始 pattern 不含 `*` 或 `?`，再加上前後 `%` 形成 contains match。成功時回傳完整 byte 長度，短 buffer 會 NUL-terminate 截斷；null pattern 回 `SIZE_MAX`。C 端只在 `bm25_file_pattern_like()` 中委派此 pure pattern serializer；SQLite/FTS5 query execution、LIKE binding/query planning、BM25 ranking/boosting、result JSON、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_sanitize_utf8_lossy_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP code snippets 來源 UTF-8 lossy 消毒 ABI。Rust 接受 nullable NUL-terminated C string 與 caller-owned output buffer。對非 UTF-8 byte 進行 lossy 消毒，將其替換為 REPLACEMENT CHARACTER U+FFFD。成功時回傳消毒後所需的位元組長度，短 buffer 會 NUL-terminate 截斷；`input == NULL` 回傳 `SIZE_MAX`。#53 的 `sanitize_utf8_lossy.c` wrapper 以此 ABI 配置 C output；yyjson building/serialization、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_architecture_aspect_wanted_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `get_architecture` aspects filter 判斷 ABI。Rust 接受 nullable NUL-terminated C string `input` (params_json) 與 `name` (aspect name)，若該 aspect 被請求則回傳 1，否則回傳 0。缺少 aspects、invalid JSON（含尾端殘留）、root 非 object、aspects 非 array 均視為 wanted (true)；空 array 或無匹配為 false；"all" 或 exact name match 為 true；大小寫與空白敏感；非字串元素忽略。C 端在 `aspect_wanted()` 中委派此 pure filter；yyjson parsing/serialization、Store links/traversal、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_trace_is_test_file_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `trace_call_path` test-file classifier ABI。Rust 接受 nullable NUL-terminated C string，回傳 1 表示 path 含有 `/test`、`test_`、`_test.`、`/tests/`、`/spec/` 或 `.test.` substring；null 回 0。此 ABI 固定 MCP helper 的寬鬆 substring contract，刻意不重用 pipeline/store test detection。C 端只在 `is_test_file()` 中委派此 pure classifier；BFS traversal、`include_tests` filtering、`is_test` JSON 標記、risk labels、data-flow args extraction、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- `cbm_rs_mcp_project_db_file_name_v1`：MCP cache project DB filename classifier ABI。#49 已有
  `src/mcp/project_db_file_name.c/.h` fallback 與公開 `bool cbm_mcp_project_db_file_name(const char *name)`；
  wrapper `CBM_USE_RUST_MCP_PROJECT_DB_FILE_NAME=1`（或 CODEC）委派 v1，direct
  `CBM_USE_RUST_MCP_PROJECT_DB_FILE_NAME_ONLY=1`／`mcp-project-db-file-name-only` 匯出同名 ABI。nullable
  NUL-terminated filename 長度至少為 `x.db`、精準以 `.db` 結尾，且不是 `_` 開頭或 `:memory:` 開頭時回 true；
  null、`.db`、`*.db-wal`、`*.sqlite`、internal/hidden filename 與 SQLite memory marker 回 false。`tmp-*.db`
  維持有效，first-NUL，無 heap／I/O。directory scanning、SQLite query-open、internal project-name resolution、
  ghost/corrupt DB filtering、JSON list building、resolve fallback、response/transport/server lifecycle 與 handlers
  仍由 C 負責。
- `cbm_rs_mcp_cancel_request_matches_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP `notifications/cancelled` request matcher ABI。Rust 接受 nullable NUL-terminated `params_json` 與 `active_id_str`，只解析 params object 首個 `requestId`，並回傳 1/0。`active_id_str == NULL` 時只接受 JSON integer 且等於 `active_id`；`active_id_str != NULL` 時只接受 JSON string 且 byte 完全相等。null、invalid JSON、root 非 object、缺少 `requestId`、型別不符、float/bool/null/array/object、invalid UTF-8 與尾端殘留皆回 0。server lifecycle、active pipeline cancellation side effect、logging、response formatting 與 Content-Length transport 仍由 C 負責。
- `cbm_rs_mcp_jsonrpc_format_error_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 JSON-RPC numeric-id error response formatter ABI。Rust 接受 nullable NUL-terminated `message`，產生 compact JSON `{"jsonrpc":"2.0","id":...,"error":{"code":...,"message":"..."}}`，寫入 caller-provided buffer 並回傳完整 byte 長度；短 buffer 會 NUL-terminate 截斷，`message == NULL` 對齊空字串。此 opt-in 只委派 `cbm_jsonrpc_format_error()` 的 numeric-id error path；string id response、result embedding、invalid embedded JSON fallback、Content-Length transport、dispatch 與 handlers 仍由 C/yyjson 負責。
- `cbm_rs_mcp_jsonrpc_format_response_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 JSON-RPC response formatter ABI。Rust 接受 nullable NUL-terminated `id_str`、`result_json` 與 `error_json`，產生 compact JSON response，支援 numeric/string id、`result` embedding、`error` 優先、無 result/error 時輸出 `result:null`，並保留 invalid embedded JSON 時只輸出 `jsonrpc/id` 的 C/yyjson contract。此 opt-in 只委派 `cbm_jsonrpc_format_response()` 的 response JSON 組裝；Content-Length transport、server loop、tool schema、tool dispatch 與 handlers 仍由 C/yyjson 負責。
- `cbm_rs_mcp_text_result_v1`：給 `CBM_USE_RUST_MCP_CODEC=1` 使用的 MCP text result formatter ABI。Rust 接受 nullable NUL-terminated `text` 與 `is_error`，產生 compact JSON `content[0].type/text` 與 `isError`；僅在非 error 且 `text` 是 JSON object 時加入 minified `structuredContent`，plain text、array、invalid JSON、null 與 error result 都不加入。此 opt-in 只委派 `cbm_mcp_text_result()` 的 JSON 組裝；Content-Length transport、server loop、tool schema、tool dispatch 與 handlers 仍由 C/yyjson 負責。
- `cbm_rs_pipeline_project_name_from_path`：給 `CBM_USE_RUST_PIPELINE_PROJECT_NAME=1` 使用的 FQN project-name ABI。Rust 以 raw bytes 對齊 `cbm_project_name_from_path`，寫入 caller-provided buffer 並回傳完整輸出長度；短 buffer 會截斷並 NUL-terminate。C 端 `cbm_project_name_from_path()` 只在 opt-in 下先呼叫 Rust、再用 heap copy 回傳 C-owned 字串；NULL/empty/root fallback、separator/drive normalization、unsafe ASCII collapse、non-ASCII byte lowercase hex encoding 與 truncation contract 仍與 C fallback 對齊。
- `cbm_rs_pipeline_is_env_var_name_v1`、`cbm_rs_pipeline_normalize_config_key_v1`、`cbm_rs_pipeline_has_config_extension_v1`：給 `CBM_USE_RUST_PIPELINE_CONFIGURES=1` 使用的 config helper ABI。Rust 只承接 `cbm_is_env_var_name()` 的 uppercase-plus-digit classifier、`cbm_normalize_config_key()` 的 camel/snake/dot 正規化與 token count、以及 `cbm_has_config_extension()` 的副檔名判斷；C wrapper 仍保留 input validation 與既有 fallback 行為，config link pass 與其他 pipeline side effects 仍由 C 負責。
- `CBM_USE_RUST_PIPELINE_CONFIGURES_ONLY=1` / Cargo `pipeline-configures-only` feature：C build 會排除 `src/pipeline/pass_configures.c`，Rust staticlib 直接匯出同名 `cbm_is_env_var_name()`、`cbm_normalize_config_key()`、`cbm_has_config_extension()` ABI。direct aliases 仍由 `ffi.rs` 處理 nullable C string、`norm_out == NULL`、`norm_sz == 0`、short buffer NUL 結尾、511-byte key payload 截斷與 token count；純邏輯維持在 `pipeline::configures`。預設 C fallback 與既有 wrapper opt-in 保留，config link、graph mutation 與 pipeline side effects 不遷移。
- `cbm_rs_pipeline_route_canon_path_v1`：給 `CBM_USE_RUST_PIPELINE_ROUTE_CANON=1` 使用的 route path canonicalization ABI。Rust 以 nullable NUL-terminated C string 輸入，將 `:name`、`{name}`、`<name>` / `<int:name>` 與 `${name}` 等 route parameter placeholder collapse 成 `{}`，寫入 caller-provided buffer 並回傳完整輸出長度；null input 對齊 C helper 輸出空字串，短 buffer 會 NUL-terminate 截斷。此 opt-in 只委派 `cbm_route_canon_path()` 的純 byte-level path normalization；route extraction、HTTP/async call detection、Route node/edge 建立、graph-buffer mutation、language parser 與 pipeline side effects 仍由 C 負責。
- `cbm_rs_pipeline_is_path_keyword_v1` / `cbm_rs_pipeline_normalize_url_arg_v1`：給 `CBM_USE_RUST_PIPELINE_ROUTE_ARGS=1` 使用的 parallel pipeline route argument helper ABI。Rust 只判定固定 path keyword 清單，並將反引號/引號包裝與 `${name}` template URL 轉為 `:name`，同時拒絕相對路徑、過短路徑、regex metacharacter、空白與 double slash；normalized output 由 caller-owned buffer 擁有。route call 掃描、handler resolution、Route node/edge、graph mutation 與 pipeline side effects 仍由 C 負責。
- `cbm_rs_pipeline_is_hash_segment_v1` / `cbm_rs_pipeline_is_broker_route_v1`：給 `CBM_USE_RUST_PIPELINE_ROUTE_NODE_CLASSIFIERS=1` 使用的 route-node classifier ABI。Rust 只判定 Cloud Run hostname suffix 的小寫英數 segment（長度上限 12、含字母且長度至少 3 視為 meaningful word）與固定 `__route__infra__` / pubsub / cloud-tasks / async / scheduler / kafka / sqs prefix；前者接受 caller-owned bytes+length，後者接受 nullable C string。Cloud Run URL extraction、suffix stripping、Route node/edge 與 graph mutation 仍由 C 負責。
- `CBM_USE_RUST_PIPELINE_ROUTE_NODE_CLASSIFIERS_ONLY=1` / Cargo `pipeline-route-node-classifiers-only` feature：C build 會排除 `src/pipeline/route_node_classifiers.c`，Rust staticlib 直接匯出同名 `cbm_pipeline_is_hash_segment()` 與 `cbm_pipeline_is_broker_route()` ABI。`ffi_route_node_classifiers.rs` 負責 `segment==NULL`、`len == 0` 與 `qn==NULL` 的邊界；純 route-node classifier 邏輯仍留在 `pipeline::route`，其餘 route extraction、URL suffix stripping、Route node/edge 與 graph mutation side effect 仍由 C 負責。
- `cbm_rs_pipeline_is_dockerfile_v1`、`cbm_rs_pipeline_is_compose_file_v1`、`cbm_rs_pipeline_is_env_file_v1`、`cbm_rs_pipeline_is_cloudbuild_file_v1`、`cbm_rs_pipeline_is_kustomize_file_v1`、`cbm_rs_pipeline_is_shell_script_v1`：給 `CBM_USE_RUST_PIPELINE_INFRASCAN=1` 使用的 infra file classifier ABI。Rust 只承接 Dockerfile、docker-compose、`.env`、`cloudbuild*`、`kustomization.*` 與 shell script 的純 byte-level 判斷；`cbm_is_k8s_manifest()` 的 content scan、tree-sitter/extraction 與其他 pipeline side effects 仍由 C 負責。
- `cbm_rs_pipeline_is_helm_chart_file_v1`、`cbm_rs_pipeline_is_gomod_file_v1`、`cbm_rs_pipeline_is_requirements_file_v1`：給 `CBM_USE_RUST_PIPELINE_K8S_FILE_CLASSIFIERS=1` 使用的 Kubernetes dependency-manifest classifier ABI。Rust 精準比對 `Chart.yaml` / `Chart.yml`、`go.mod` 與 `requirements.txt`；null 或其他名稱回 false。C 端只在 `pass_k8s.c` 的 Helm、Go module 與 Python requirements 分流委派 Rust；檔案讀取、Helm/Go/Python parser、圖節點／邊寫入與 pipeline side effects 仍由 C 負責。
- `CBM_USE_RUST_PIPELINE_K8S_FILE_CLASSIFIERS_ONLY=1` / Cargo `pipeline-k8s-file-classifiers-only` feature：C build 會編入 `src/pipeline/k8s_file_classifiers.c` 作為同名 `cbm_pipeline_is_*` 輕量 wrapper，實際判斷邏輯改由 Rust staticlib 匯出 `cbm_pipeline_is_helm_chart_file()`、`cbm_pipeline_is_gomod_file()`、`cbm_pipeline_is_requirements_file()` ABI。`ffi_k8s_file_classifiers.rs` 只補上 C 呼叫端邊界（`name == NULL` 回 0）；原始分類邏輯仍保留於 `pipeline::infrascan`，`pass_k8s.c` 只做檔案分流決策與剩餘圖節點／邊建立、副作用仍由 C 負責。
- `cbm_rs_pipeline_is_trackable_file_v1`：給 `CBM_USE_RUST_PIPELINE_GITHISTORY=1` 使用的 git history file classifier ABI。Rust 只承接 `cbm_is_trackable_file()` 的路徑前綴、lock/generated basename 與非 source suffix byte-level 判斷；git log 解析、change coupling、`FILE_CHANGES_WITH` edge、graph-buffer mutation、檔案 I/O 與 pipeline side effects 仍由 C 負責。
- `cbm_rs_pipeline_is_test_path_v1` / `cbm_rs_pipeline_is_test_func_name_v1` / `cbm_rs_pipeline_test_to_prod_path_v1`：給 `CBM_USE_RUST_PIPELINE_TEST_DETECT=1` 使用的 test detection ABI。Rust 接受 nullable NUL-terminated C string，對齊 `cbm_is_test_path()` / `cbm_is_test_func_name()` 的 byte-level classifier contract，並承接 `test_to_prod_path()` 的 test->prod path rewrite contract：basename `test_`、語言特定 test/spec suffix、`.test/.spec` substring、`/tests/` 類目錄，以及 Go `Test/Benchmark/Example` 大寫規則；`test_to_prod_path()` 會把 test 命名轉成對應 production path。此 opt-in 只委派 pure classifier 與 path rewrite；`TESTS` / `TESTS_FILE` edge creation、`node_is_test()`、graph-buffer adapter、tree-sitter/extraction 與 pipeline side effects 仍由 C 負責。
- `cbm_rs_pipeline_is_checked_exception_v1`：給 `CBM_USE_RUST_PIPELINE_CHECKED_EXCEPTION=1` 使用的 exception classifier ABI。Rust 接受 nullable NUL-terminated C string，回傳 1/0，對齊 `is_checked_exception()` 的 byte-level contract：空字串與一般 checked exception 名稱回 1；含 `Error`、`Panic`、`error` 或 `panic` 的名稱回 0。此 opt-in 只委派 pure classifier；THROWS / RAISES edge type 選擇、exception resolution、graph-buffer mutation 與 pipeline side effects 仍由 C 負責。
- `cbm_rs_pipeline_artifact_path_v1`：給 `CBM_USE_RUST_PIPELINE_ARTIFACT_PATH=1` 使用的 artifact path builder ABI。Rust 接受 nullable NUL-terminated repo path 與 artifact 名稱，寫入 caller-provided buffer 並回傳完整路徑長度；null input、`buf == NULL`、`bufsize == 0` 或短 buffer 皆回 `SIZE_MAX`。此 opt-in 只委派 `<repo>/.codebase-memory/<name>` 的 pure byte-level path 組裝；artifact 壓縮、解壓、metadata、`.gitattributes` 寫入與 pipeline side effects 仍由 C 負責。
- `cbm_rs_pipeline_plan_describe`：pipeline pass plan parity ABI。Rust 只回傳 full pipeline、sequential、parallel extraction、predump、parallel threshold、incremental extract/resolve 與 incremental post/tail 的 plan 字串；不接受或共用 `CbmRsRegistryHandle`，不保存 C 指標，不執行任何 tree-sitter/extraction/LSP/store code。
  - `kind=0` sequential pass table；字串 ABI 仍包含 `infra_routes:after_success` 與 `infra_bindings:after_success` tail。typed v2 metadata 目前只描述前段 6 個 sequential dispatch pass，tail side effects 仍由 C 端既有條件控制。
  - `kind=1` predump pass table；此字串 ABI 保持相容，typed v2 metadata 另由 `cbm_rs_pipeline_plan_steps_v2` 提供。
  - `kind=2` extraction choice，依 `worker_count > 1 && file_count > 50` 回傳 `parallel` 或 `sequential`。
  - `kind=3` incremental extract/resolve plan；typed v2 metadata 另由 `cbm_rs_pipeline_plan_steps_v2` 提供，C 端驗證 typed steps 後逐步 dispatch，實際 extract/resolve 仍呼叫 C 實作。
  - `kind=4` incremental post/tail typed plan；產品 opt-in 透過 typed step FFI dispatch k8s/postpass prefix 與 `edge_relink -> incremental_dump -> persist_hashes -> artifact_export` tail，C 端固定驗證順序與 policy。
  - `kind=5` parallel extraction pass order。
  - `kind=6` full pipeline outer orchestration。
  - `CBM_USE_RUST_PIPELINE_PLAN=1` 使用 `kind=2` 選擇 full/incremental extraction phase 的 parallel 或 sequential path，使用 typed v2 metadata 驅動 sequential extraction dispatch、incremental extract/resolve dispatch 與 full pre-dump pass dispatch/trace，使用 typed v2 metadata 驗證並記錄 parallel extraction 外層順序，並使用 typed incremental post steps 驅動完整 incremental post/tail dispatch；graph-buffer mutation command contract 由 test-only FFI 固定，C-side adapter 已可同步 dispatch structure、incremental purge/file-upsert/inbound restore、`TESTS`/`TESTS_FILE`、usage 與 configlink 等代表性呼叫點，實際 pass 執行、parallel cache/cross-LSP cleanup、dump、hash persist、FTS rebuild、artifact export、infra tail、graph-buffer 寫入、storage 與 ownership 仍由 C 負責。
- `cbm_rs_pipeline_plan_step_count_v2` / `cbm_rs_pipeline_plan_steps_v2`：additive typed plan metadata ABI。目前支援 `kind=0` sequential dispatch、`kind=1` predump、`kind=3` incremental extract/resolve 與 `kind=5` parallel extraction；其他 valid kind 回傳 `-1`，保留未來擴充空間。每個 step 回傳 stable `kind`、`phase`、`policy`、`gate_flags`、`requires_mask` 與 `effect_flags`；C adapter 必須逐欄比對本地 dispatch table，任何缺漏、重複、錯序、未知 bit 或 mode gate 不符都不可採用 typed path。
  - sequential kind：`16=definitions`、`17=k8s`、`18=lsp_cross`、`19=calls`、`20=usages`、`21=semantic`。這批 metadata 只覆蓋 sequential extraction dispatch loop，不包含 `infra_routes` / `infra_bindings` after-success tail。
  - predump kind：`1=decorator_tags`、`2=configlink`、`3=route_match`、`4=similarity`、`5=semantic_edges`、`6=complexity`。
  - incremental extract/resolve kind：`48=definitions`、`49=calls`、`50=usages`、`51=semantic` 用於 sequential incremental path；`52=incr_extract`、`53=incr_registry`、`54=incr_resolve` 用於 parallel incremental path。這批 metadata 會被 C adapter 驗證後用於 dispatch；C 仍負責 extraction cache、shared id、registry build、resolve、cross-LSP prebuild skip 與所有副作用。
  - parallel extraction kind：`32=parallel_extract`、`33=registry_build`、`34=lsp_cross_prepare`、`35=parallel_resolve`、`36=infra_routes`、`37=infra_bindings`、`38=k8s`。這批 metadata 只固定 parallel path 的外層順序與 trace；C 仍負責 result cache、shared id、cross-LSP resources、infra 執行時機、cleanup 與 error propagation。
  - `phase=1` 表示 predump；`phase=2` 表示 sequential extraction dispatch；`phase=3` 表示 parallel extraction outer dispatch；`phase=4` 表示 incremental extract/resolve outer dispatch。`policy=0` 是 required，`policy=1` 是 ignore_err，`policy=4` 是 env_optional；sequential 的 `k8s` 與 `lsp_cross` 使用 ignore_err，incremental extract/resolve 全部使用 ignore_err，parallel 的 `lsp_cross_prepare` 使用 env_optional，其餘 dispatch pass 使用 required。
  - `gate_flags=1` 表示只在 `CBM_MODE_FAST` 跳過；目前只適用於 predump `similarity` 與 `semantic_edges`。`gate_flags=2` 表示 pass 依賴 `ctx->result_cache`；目前只適用於 sequential `lsp_cross`。`gate_flags=4` 表示 incremental parallel resolve 刻意不做 cross-file LSP prebuild。非 FAST（含 advanced mode）仍回傳 6 個 predump steps。
  - `requires_mask` 使用 stable kind 的 bitset 表達已驗證的前置 pass；C 端以本地順序精確比對，不用 Rust metadata 推導可否執行。
  - `effect_flags=1` 表示該 pass mutates graph；未知 effect 必須視為 typed metadata 不可信。parallel 的 `parallel_extract`、`registry_build`、`parallel_resolve`、`infra_routes`、`infra_bindings` 與 `k8s` 皆標示此 effect，`lsp_cross_prepare` 保持 0。
- `cbm_rs_pipeline_full_plan_step_count_v1` / `cbm_rs_pipeline_full_plan_steps_v1`：FullPipeline top-level orchestration metadata ABI。C 呼叫端必須傳入實際 `mode`、`worker_count` 與 `file_count`，因為 `extraction_dispatch.nested_plan_kind` 會依 parallel threshold 指向 `PlanKind::ParallelExtraction` 或 `PlanKind::Sequential`。Rust 回傳外層階段的 stable `kind`、`phase`、`policy`、`gate_flags`、`requires_mask`、`effect_flags` 與 `nested_plan_kind`；full/moderate/advanced mode 回傳 12 個 top-level steps，fast mode 跳過 `githistory` 後回傳 11 個。`predump.nested_plan_kind` 指向 `PlanKind::Predump`，其他 step 使用 `-1`。`pipeline.c` 在 `CBM_USE_RUST_PIPELINE_PLAN=1` 會於 discovery 後載入此 top-level metadata、驗證 stable order / `requires_mask`，並用 `extraction_dispatch.nested_plan_kind` 驅動 full extraction choice；`pipeline_incremental.c` 仍不直接消費 FullPipeline metadata。`cbm_rs_pipeline_plan_steps_v2(kind=FullPipeline)` 仍回傳 `-1`，避免把外層 orchestration 與既有 typed pass dispatch ABI 混用。
- `cbm_rs_pipeline_incremental_post_step_count` / `cbm_rs_pipeline_incremental_post_steps`：給 `CBM_USE_RUST_PIPELINE_PLAN=1` 使用的 typed incremental post plan ABI。Rust 回傳 stable numeric `kind` / `policy` pairs，C 端必須驗證順序與 policy：post-pass prefix 為 `ignore_err`，`edge_relink` / `incremental_dump` / `persist_hashes` 為 `best_effort`，`artifact_export` 為 `optional_existing_artifact`。驗證通過後，C adapter 會依 typed steps 呼叫既有 C handlers；Rust 不保存 C 指標，也不搬移 dump、hash persist、FTS rebuild 或 artifact export 的 side effects。Artifact policy 仍描述既有 artifact 自動更新分支；C handler 另外會依 `pipeline.persistence` 對 explicit export 使用 full-path 錯誤語意。若 `incremental_dump` 失敗，C handler 會傳回實際 rc，並跳過後續 persist/artifact tail；這屬 C side-effect contract，不由 Rust ABI 決定。
- `cbm_rs_gbuf_mutation_command_count_v1` / `cbm_rs_gbuf_mutation_commands_v1` / `cbm_rs_gbuf_mutation_validate_v1`：test-only graph-buffer mutation command boundary ABI。Rust 回傳 stable command metadata，並只驗證 command shape；目前固定 `1=UpsertNode`、`2=InsertEdge`、`3=DeleteByFile`，以及 required/optional field bitset、result kind、effect flags、reserved bits、empty string 與 null C string contract。此 ABI 的 command struct/constants 由 `src/graph_buffer/graph_buffer.h` 與 FFI smoke 共用；C string 只在呼叫期間借用，底層 graph-buffer 會複製需要保存的值。數值欄位（例如 source/target id 與行號）是固定 ABI scalar slot，Rust validation 不推斷 presence、不做 endpoint lookup，也不解析 JSON；effect flags 表示可能副作用，`InsertEdge` dedup 時可能回傳既有 edge id。此 ABI 不接產品 opt-in，不接受 `cbm_gbuf_t*`，也不執行任何 graph-buffer mutation。C-side `cbm_gbuf_apply_mutation` / `cbm_gbuf_apply_*` adapter 只是同步 normalization/dispatch fixture：它可讓既有 C 呼叫走 command apply 邊界，但仍呼叫 C `cbm_gbuf_*` API，不代表 Rust-backed graph-buffer；除非另有明確 production opt-in flag、C fallback、default/opt-in matrix 與 language graph parity gate，文件不得稱為 graph-buffer migration。
- `cbm_rs_registry_resolve_once`：一次性 registry parity fixture；每次呼叫從 C entry 陣列建立暫時 registry，只供 `tests/test_rust_ffi.c` smoke 使用，不接產品 hot path。
- `cbm_rs_registry_create/free/add/resolve`：給 `CBM_USE_RUST_PIPELINE_REGISTRY=1` 使用的 opaque registry handle ABI。handle 由 Rust 配置與釋放，`add` 複製 QN bytes；`resolve` 只在呼叫期間借用 callee/import 指標，將 QN 與 strategy 寫入 caller-provided buffers。C wrapper 會用回傳 QN 查回 C registry-owned key，維持 `cbm_resolution_t.qualified_name` 的 borrowed-from-registry contract。
- `cbm_rs_registry_is_test_qn_v1`：給 `CBM_USE_RUST_PIPELINE_REGISTRY_TEST_QN=1` 使用的 registry candidate classifier ABI。Rust 接受 nullable NUL-terminated `qn`，不保存 C 指標，回傳 `1` 或 `0`；`NULL` 與空字串回 `0`。它以 raw byte、大小寫敏感 substring 比對 `Test`、`test`、`Mock`、`mock`、`Stub`、`stub`、`Fake`、`fake`、`Fixture`、`spec`，非 UTF-8 bytes 不轉碼。C `is_test_qn()` 只採用 `0` / `1`，其他回傳值回退既有 C classifier。此 opt-in 只影響 candidate score 的 test-like QN 判定；opaque registry handle、add/resolve/cache、所有權與 pipeline side effects 仍由 C 負責。
- 同一 pool 不保證 thread-safe；這維持目前 C interner 的使用模型。
- `cbm_rs_pipeline_parse_range_v1`：給 `CBM_USE_RUST_PIPELINE_GITDIFF_RANGE=1` 使用的 unified diff hunk range scalar parser ABI。Rust 接受 nullable NUL-terminated `start,count` / `start` 字串，將 start/count 寫入呼叫端提供的兩個 `int` 輸出；省略 count 預設為 1，NULL/無效輸入安全回 `(0,1)`，保留零 count 與正負整數。C 的 name-status/hunk 結構、固定 buffer、git diff I/O、graph mutation 與 pipeline side effects 仍由 C 負責。
- `cbm_rs_pipeline_parse_name_status_v1`：給 `CBM_USE_RUST_PIPELINE_GITDIFF_NAME_STATUS=1` 使用的 `git diff --name-status` parser ABI。Rust 接受 nullable NUL-terminated 輸出與 caller-owned `cbm_changed_file_t` 等價 fixed-array buffer，解析第一個 status/tab 欄、rename 的新舊 path、max output 與 `cbm_is_trackable_file()` filter，path 依 C 的 512-byte buffer contract 截斷；NULL/空輸入、NULL output 或非正 max 回 0。hunk parser、git I/O、change graph mutation 與 pipeline side effects 仍由 C 負責。
- `cbm_rs_pipeline_parse_hunks_v1`：給 `CBM_USE_RUST_PIPELINE_GITDIFF_HUNKS=1` 使用的 `git diff --unified=0` hunk header parser ABI。Rust 接受 nullable NUL-terminated 輸出與 caller-owned `cbm_changed_hunk_t` 等價 fixed-array buffer，追蹤 `+++ b/` current file、解析 `@@` 新行 range、套用 trackable filter 與 512-byte path 截斷；NULL/空輸入、NULL output 或非正 max 回 0。git diff I/O、hunk 後續消費、graph mutation 與 pipeline side effects 仍由 C 負責。
- `cbm_rs_pipeline_is_module_dir_v1`：給 `CBM_USE_RUST_PIPELINE_MODULE_DIR=1` 使用的 language module-directory classifier ABI。Rust 接受 `CBMLanguage` 整數值，僅 `CBM_LANG_GO=0` 與 `CBM_LANG_JAVA=6` 回傳 1，其餘值回傳 0；不保存指標、無配置與無 side effect。C 的 calls、parallel、cross-LSP、semantic、usages 五個 pass wrapper 仍負責所有 module QN 組裝、解析與 pipeline side effects。
### `str-intern-only` direct ABI

啟用 Cargo feature `str-intern-only` 且設定 `CBM_USE_RUST_STR_INTERN_ONLY=1` 時，Rust staticlib 直接輸出 `cbm_intern_create`、`cbm_intern_free`、`cbm_intern`、`cbm_intern_n`、`cbm_intern_count` 與 `cbm_intern_bytes`。此模式會從 `FOUNDATION_SRCS` 排除 `src/foundation/str_intern.c`；一般 `CBM_USE_RUST_STR_INTERN=1` 仍保留 C wrapper fallback。

`CInternPool` 由 Rust 建立並釋放；`cbm_intern*` 回傳的字串指標借用 pool，直到 `cbm_intern_free` 前保持有效，C 呼叫端不得自行釋放。`cbm_intern_n` 支援嵌入 NUL，輸入為 null、pool 為 null 或長度超過 ABI 上限時回傳 null；`cbm_intern` 依 NUL 終止字串計算長度。

### `dump-verify-only` direct ABI

啟用 Cargo feature `dump-verify-only` 且設定 `CBM_USE_RUST_DUMP_VERIFY_ONLY=1` 時，Rust staticlib 直接輸出 `cbm_dump_verify_is_degraded` 與 `cbm_dump_verify_min_ratio`；此模式會從 `FOUNDATION_SRCS` 排除 `src/foundation/dump_verify.c`。既有 `CBM_USE_RUST_DUMP_VERIFY=1` 仍是 C wrapper 呼叫 `cbm_rs_*` 的 fallback 模式。

`cbm_dump_verify_min_ratio` 使用 Rust bounded environment/parser，保留 `strtod` 前綴、十進位/十六進位、不完整 exponent、`0..=1` 驗證、default fallback 與 invalid warning。warning 透過 `log.c` 的固定參數 `cbm_log_dump_verify_invalid_ratio` bridge 發出，避免 Rust 依賴 variadic C ABI。

### `yaml-only` direct ABI

啟用 Cargo feature `yaml-only` 且設定 `CBM_USE_RUST_YAML_ONLY=1` 時，Rust staticlib 直接輸出 `cbm_yaml_parse`、`cbm_yaml_free`、`cbm_yaml_get_str`、`cbm_yaml_get_float`、`cbm_yaml_get_bool`、`cbm_yaml_get_str_list` 與 `cbm_yaml_has`；此模式會從 `FOUNDATION_SRCS` 排除 `src/foundation/yaml.c`。

YAML root 由 Rust `Box` 擁有，query 回傳的字串與 list pointers 借用 root，直到 `cbm_yaml_free` 前有效，C 呼叫端不得自行釋放。float parser 維持 C `strtod` 的前綴與 whitespace 行為，包含十進位、十六進位、`inf`/`nan` 與不完整 exponent。

### `yaml` production wrapper

啟用 `CBM_USE_RUST_YAML=1` 時，C 仍保留 `src/foundation/yaml.c` 並改以
`cbm_rs_yaml_*` 實作 `cbm_yaml_*` wrapper fallback；此模式不排除 YAML C source。

## `store-search-pattern-only` direct ABI

`store-search-pattern-only` 直接由 Rust 提供 `cbm_glob_to_like` 與
`cbm_extract_like_hints`。兩個 API 都維持既有 C 標頭與呼叫慣例；回傳字串由
C heap 配置，呼叫端必須以 `free()` 釋放，Rust 不保留其所有權。Rust 實作只處理
pattern 轉換與 hint 萃取，不接觸 SQLite、Store handle 或其他長生命週期指標。

當 direct slice 啟用時，Makefile 會從 production source list 排除
`src/store/search_pattern.c`；其他 Store 搜尋與 SQLite 邏輯仍由 C 執行。

## `store-arch-helpers-only` direct ABI

`store-arch-helpers-only` 直接由 Rust 匯出既有 Store ABI：
`cbm_hop_to_risk`、`cbm_risk_label`、`cbm_qn_to_package`、
`cbm_qn_to_top_package` 與 `cbm_is_test_file_path`。risk label 指向 Rust
靜態 NUL 結尾字串；QN 結果使用每執行緒固定 256-byte buffer，與原本 C
`CBM_TLS` buffer 的生命週期及「呼叫端不得 free」契約一致。

此 slice 不涉及 SQLite 或 Store handle。direct build 會排除
`src/store/arch_helpers.c`，而其他 architecture query、path scope、file
extension 與 Store 核心仍維持原有邊界。

## `store-file-ext-only` direct ABI

`store-file-ext-only` 直接由 Rust 匯出內部 Store helper
`cbm_store_file_ext`。無副檔名或超過 15 bytes 時回傳 null；有效結果寫入
每執行緒 16-byte buffer 並以 NUL 結尾，呼叫端只讀取結果且不得 free，與原本
C `CBM_TLS` buffer 契約一致。此 ABI 不接觸 SQLite 或 Store handle，direct build
會排除 `src/store/file_ext.c`。

## `store-immutable-uri-only` direct ABI

`store-immutable-uri-only` 直接由 Rust 匯出
`cbm_store_build_immutable_uri(path, out, out_sz)`。Rust 只寫入呼叫端提供的
buffer，成功時保證 NUL 結尾，容量不足、null path 或無效 buffer 回傳 false；
不保存 C 指標、不開 SQLite。direct build 會排除
`src/store/immutable_uri.c`，而 query open 的 SQLite fallback 流程仍由 C 控制。

## `store-mmap-resolver-only` direct ABI

`store-mmap-resolver-only` 直接由 Rust 匯出 `cbm_store_resolve_mmap_size`。
Rust 讀取 `CBM_SQLITE_MMAP_SIZE`，使用既有 ASCII `strtoll` parity parser，
維持 unset/empty/malformed/overflow 回傳 64 MiB、負值 clamp 到 0、leading
space 與 `+` 可接受、trailing bytes 拒絕的契約。此 API 不接觸 SQLite handle；
direct build 會排除 `src/store/mmap_resolver.c`。

## `store-arch-path-scope-only` direct ABI

`store-arch-path-scope-only` 直接由 Rust 匯出
`cbm_store_arch_path_prepare(path, norm_out, norm_sz, like_out, like_sz)`。它依 C
契約正規化 architecture path，並以呼叫端持有的緩衝區產生 `/%` LIKE pattern；失敗時回傳
`false`，不保存 C 指標、不接觸 SQLite。direct build 會排除
`src/store/arch_path_scope.c`，SQL fragment 與 SQLite bind 仍由 C 控制。

## `store-fts-tokenizer-only` direct ABI

`store-fts-tokenizer-only` 直接由 Rust 匯出
`cbm_store_sqlite_camel_split(ctx, argc, argv)`。它保留 SQLite scalar callback 的 C ABI，
以 Rust 執行 camelCase tokenization；結果使用 `sqlite3_malloc` 配置，並以
`sqlite3_free` destructor 交由 SQLite 接管，不保存 SQLite 或 C 指標。

callback 實作位於獨立的 `rust/cbm-core/src/fts_sqlite.rs`，避免通用
`test-rust-ffi` archive link 拉入 SQLite imports。direct build 排除
`src/store/fts_tokenizer.c`；`src/store/store.c` 僅保留 SQLite function 註冊。

## `pipeline-project-name-only` direct ABI

`pipeline-project-name-only` 直接由 Rust 匯出
`cbm_project_name_from_path(abs_path)`。Rust 依既有 path-to-project-name contract 產生結果，
但以 C `malloc` 配置 NUL-terminated 字串，因此既有 C 呼叫端仍可直接以 `free()` 釋放。
direct build 排除 `src/pipeline/project_name.c`；其他 FQN 與 pipeline 邏輯仍保留 C。

## `pipeline-artifact-path-only` direct ABI

`pipeline-artifact-path-only` 直接由 Rust 匯出
`cbm_pipeline_artifact_path(buf, bufsz, repo_path, name)`。它只依既有 byte-level
contract 將 `<repo>/.codebase-memory/<name>` 寫入呼叫端提供的 buffer；null、零長度或
不足 buffer 時回傳 `false`，不配置記憶體、不做 I/O。direct build 排除
`src/pipeline/artifact_path.c`，artifact 的 SQLite、zstd、metadata 與檔案 I/O 維持 C。
## `pipeline-test-detect-only` direct ABI

`pipeline-test-detect-only` 直接由 Rust 匯出 `cbm_is_test_path(path)` 與
`cbm_is_test_func_name(name)`。兩者只讀取 NUL-terminated 輸入並回傳分類結果，
不配置記憶體、不接觸 graph state。direct build 排除 `src/pipeline/test_detect.c`；
`src/pipeline/pass_tests.c` 保留 TESTS/TESTS_FILE edge 建立與
`test_to_prod_path` 的 C ownership 邊界。

## `pipeline-checked-exception-only` direct ABI

`pipeline-checked-exception-only` 由 Rust 匯出
`cbm_pipeline_is_checked_exception(name)`。輸入為可為 NULL 的 NUL-terminated 名稱；
函式只回傳 `0` 或 `1`，不配置記憶體、不接觸 graph state。direct build 排除
`src/pipeline/checked_exception.c`；`src/pipeline/pass_parallel.c` 保留 THROWS/RAISES
edge resolution。

2026-07-15 的 direct FFI smoke 在
`CBM_USE_RUST_PIPELINE_CHECKED_EXCEPTION_ONLY=1` 下直接呼叫同名 ABI，涵蓋 NULL、空字串、
`NotFoundException`、`NotFoundError`、`panic`、`ClientPanic` 與一般名稱。target 另以隔離
default/direct build 驗證 `edge_types_probe pipeline` 各 `273 passed`、Rust unit `1 passed`、
direct production link 與 `--version`；`CHECKED_EXCEPTION_SRCS =` 證實 C source 已排除。

## `progress-sink-only` direct ABI

啟用 Cargo feature `progress-sink-only` 且設定
`CBM_USE_RUST_PROGRESS_SINK_ONLY=1` 時，Rust staticlib 直接匯出既有
`cbm_progress_sink_init`、`cbm_progress_sink_fini` 與 `cbm_progress_sink_fn`。
`Makefile.cbm` 會排除 `src/cli/progress_sink.c`；Rust direct wrapper 只呼叫既有
C `cbm_log_set_sink()` 註冊或清除 callback，`FILE*`、stderr、logger global state
與 sink lifecycle 仍由 C 擁有。Rust `cli::progress_sink` 只承接 structured log
事件解析、phase 格式化、gbuf 計數與 pending newline 狀態機，不保存跨呼叫的 C
指標以外的 ownership，也不接觸 pipeline、server lifecycle 或 handler。

## `str-util-only` direct ABI

啟用 Cargo feature `str-util-only` 且設定 `CBM_USE_RUST_STR_UTIL_ONLY=1` 時，Rust
staticlib 直接匯出 `str_util.h` 的 path/string helper：`cbm_path_join`、
`cbm_path_join_n`、`cbm_path_ext`、`cbm_path_base`、`cbm_path_dir`、starts/ends/
contains、tolower、replace、strip、split、shell/project validator 與
`cbm_json_escape`。`cbm_path_ext` / `cbm_path_base` 仍回傳 borrowed input/static
empty pointer；其他字串結果與 split array 一律透過 C `cbm_arena_alloc` 配置，
呼叫端仍以既有 `CBMArena` lifetime 管理，Rust 不保存或釋放 C 指標。
`Makefile.cbm` 的 direct build 排除 `src/foundation/str_util.c`；`CBM_SNPRINTF_APPEND`
仍是 C header macro，不涉及 Rust allocation。既有 null、empty、短輸入、path
slash、split count 與 JSON buffer contract 維持由 Rust byte-level helper 對齊。

## 2026-07-13 全組合 direct gate evidence

`make -f Makefile.cbm rust-all-optin-test` 的 wrapper 與 direct 變體均完成 `5869 passed`。direct production binary 成功連結，且 compilation line 排除 `src/foundation/str_util.c` 與 `src/cli/progress_sink.c`，證明這兩個 C compilation unit 已由 Rust direct ABI 取代。此結果只涵蓋目前已註冊的 slices；未納入的 C subsystem 仍維持 C fallback，直到各自完成 ABI、所有權、平台與回歸 gate。

## 2026-07-13 hash table direct ABI

`hash-table-only` 以 Rust `HashMap<Vec<u8>, HashEntry>` 取代 Verstable C compilation unit。C API 的 borrowed key contract 保持不變：Rust 不釋放呼叫端 key/value，`cbm_ht_get_key` 回傳原始 key pointer；overwrite 更新 pointer，`foreach` 先 snapshot `(key, value)` 再呼叫 C callback。direct gate 已確認 `src/foundation/hash_table.c` 不在 link line，且 foundation 與 production tests 通過。

## 2026-07-13 hash table 全矩陣證據

all-optin wrapper 與 direct 變體各自完成 `5869 passed`，direct production link 成功且排除 `src/foundation/hash_table.c`。這提供 hash table ABI、借用 key ownership 與 callback snapshot 行為在完整既有測試矩陣中的整合證據。

## 2026-07-13 diagnostics formatter ABI

`diagnostics-format-only` 使用 `repr(C)` snapshot 對齊 `c_long`、`size_t`、`int` 與 `long long` 欄位；Rust 只寫入 caller-owned buffer，失敗回傳 `-1`，不配置任何需由 C 釋放的記憶體。direct gate 已通過 foundation、FFI smoke 與 production link。

## Store language map direct ABI

`CBM_USE_RUST_STORE_LANGUAGE_MAP_ONLY=1` 使用既有 `cbm_rs_store_ext_lang_kind_v1(const char*) -> int` ABI。Rust 只回傳 `ext_lang_table` 的索引；C 端仍從靜態表取出 borrowed language string，故沒有 Rust-owned C-visible memory，也不需要跨邊界釋放。無效或未知副檔名回傳負值並由 C 端保留原本的 `NULL` 結果。

## Pipeline module-directory direct ABI

`cbm_rs_pipeline_is_module_dir_v1(int) -> int` 使用既有整數 ABI：輸入 `CBMLanguage` 的數值，回傳非零表示 module QN 由 containing directory 推導。`CBM_USE_RUST_PIPELINE_MODULE_DIR_ONLY=1` 已透過 `cbm_lang_module_is_dir()` 同步套用到 `pass_calls.c`、`pass_parallel.c`、`pass_semantic.c`、`pass_usages.c`、`pass_lsp_cross.c`，跨 pass 一致性已完成。

本 ABI slice 已納入 all-optin direct matrix，wrapper/direct 連結與整合 runner 均成功。

## Platform path direct ABI

`cbm_rs_normalize_path_sep(char*) -> char*` 保留輸入指標並在原 buffer 內正規化 separator，null 輸入仍回傳 null；`CBM_USE_RUST_PLATFORM_PATH_ONLY=1` 在兩個 OS implementation 分支共用此 ABI，沒有 Rust-owned memory 跨 FFI。

platform path only gate 已納入 all-optin direct matrix，wrapper/direct runner 與 production link 均成功。

## Compat thread direct ABI

`cbm_rs_compat_thread_effective_stack_size(size_t) -> size_t` 與 `cbm_rs_compat_aligned_alloc_precheck(size_t, size_t, size_t) -> bool` 都只跨 scalar ABI；Rust 不取得 pthread handle 或 allocator pointer ownership。only gate 已納入 all-optin direct matrix。

compat thread only gate 已納入 all-optin direct matrix，wrapper/direct runner 與 production link 均成功。

## MCP codec direct ABI

MCP codec ABI 以 caller-owned output buffers 與 scalar return values 為主；`CBM_USE_RUST_MCP_CODEC_ONLY=1` 只移除對應 C codec fallback，不讓 Rust 持有 C request/response 的生命週期。direct focused suite 與 FFI smoke 已通過。

MCP codec only gate 已納入 all-optin direct matrix，wrapper/direct runner 與 production link 均成功。

## Cypher classifier direct ABI

Cypher classifier ABI 只跨字元、token kind、字串與 integer index；only gates 不跨 parser AST 或 SQLite execution ownership。六組 direct ABI 已通過 `cypher` focused suite 與 FFI smoke。

六組 Cypher classifier only gates 已納入 all-optin direct matrix，wrapper/direct runner 與 production link 均成功。

## Compat regex direct ABI

compat regex only ABI 僅跨 integer flags/capacity/status；Rust 不持有 `regex_t` 或平台 regex object。focused FFI 與 foundation gate 已通過。

### compat_regex direct gate

`CBM_USE_RUST_COMPAT_REGEX_ONLY=1` 會把既有 Rust compat-regex 純 helper 設為 direct 實作，並讓 `compat_regex.c` 排除相同 C fallback；regex engine 與 C 資源管理仍保留。此 gate 已納入 all-optin direct matrix。

### log env parse direct gate

`CBM_USE_RUST_LOG_ENV_PARSE_ONLY=1` 會讓 `log.c` 直接使用既有 Rust level/format parser，並排除相同 C parser；logger 狀態、sink、輸出及環境讀取仍保留在 C。此 gate 已納入 all-optin direct matrix。

完整 all-optin wrapper/direct runner 及正式版連結已驗證包含 `CBM_USE_RUST_LOG_ENV_PARSE_ONLY=1`。

### profile env direct gate

`CBM_USE_RUST_PROFILE_ENV_ONLY=1` 會讓 `profile.c` 直接使用既有 Rust profile env helper，並排除對應 C fallback；C 仍管理 `getenv` 呼叫、全域狀態及 profiling 時間生命週期。此 gate 已加入 all-optin direct matrix。

完整 all-optin wrapper/direct runner 及正式版連結已驗證包含 `CBM_USE_RUST_PROFILE_ENV_ONLY=1`。

### platform env dirs direct gate

`CBM_USE_RUST_PLATFORM_ENV_DIRS_ONLY=1` 會讓 `platform.c` 直接使用既有 Rust env-directory helper，並排除相同 C fallback；C 仍管理平台系統呼叫、環境讀取及 buffer 生命週期。此 gate 已加入 all-optin direct matrix。

完整 all-optin wrapper/direct runner 及正式版連結已驗證包含 `CBM_USE_RUST_PLATFORM_ENV_DIRS_ONLY=1`。

### pipeline route canonicalizer direct gate

`CBM_USE_RUST_PIPELINE_ROUTE_CANON_ONLY=1` 會讓 route path canonicalizer 直接使用 Rust，並排除 C parameter scanner；Route graph 建立與 edge re-target 仍保留在 C。此 gate 已加入 all-optin direct matrix。

完整 all-optin wrapper/direct runner 及正式版連結已驗證包含 `CBM_USE_RUST_PIPELINE_ROUTE_CANON_ONLY=1`。

### pipeline route args direct gate

`CBM_USE_RUST_PIPELINE_ROUTE_ARGS_ONLY=1` / `pipeline-route-args-only` feature 會讓 C build 排除 `src/pipeline/route_args.c`，Rust staticlib 直接匯出 `cbm_pipeline_is_path_keyword()` 與 `cbm_pipeline_normalize_url_arg()` 的直接實作 ABI。C 端僅保留 path keyword 與 URL 參數掃描 call-site，`route_match` pass 仍維持原有 graph mutation、handler resolution、suffix stripping 與其他 pipeline side effects。

### pipeline git history direct gate

`CBM_USE_RUST_PIPELINE_GITHISTORY_ONLY=1` 會讓 git history pass 直接使用既有 Rust trackable-file helper，並排除對應 C filter；git log、commit 與 graph ownership 仍保留在 C。此 gate 已加入 all-optin direct matrix。

完整 all-optin wrapper/direct runner 及正式版連結已驗證包含 `CBM_USE_RUST_PIPELINE_GITHISTORY_ONLY=1`。

### pipeline git diff parsers direct gates

`CBM_USE_RUST_PIPELINE_GITDIFF_RANGE_ONLY`、`CBM_USE_RUST_PIPELINE_GITDIFF_NAME_STATUS_ONLY` 與 `CBM_USE_RUST_PIPELINE_GITDIFF_HUNKS_ONLY` 會排除 `pass_gitdiff.c` 對應 C parser，保留 C caller 與輸出結構 ownership。三個 gate 已加入 all-optin direct matrix。

完整 all-optin wrapper/direct runner 及正式版連結已驗證包含 pipeline gitdiff 三個 `_ONLY` flags。

### pipeline infrascan direct gate

`CBM_USE_RUST_PIPELINE_INFRASCAN_ONLY=1` 會讓 infrascan filename classifiers 直接使用既有 Rust helper，並排除對應 C classifier；secret/manifest parsing 與 graph ownership 仍保留在 C。此 gate 已加入 all-optin direct matrix。

### Pipeline test detection direct gate

`pass_tests.c` 的 Rust 路徑轉換器支援 `CBM_USE_RUST_PIPELINE_TEST_DETECT_ONLY`；Makefile 會傳遞 `_ONLY` CFLAGS 並連結 `pipeline-test-detect-only` staticlib。base/direct pipeline gate 均為 `221 passed`。

### Linux cgroup direct gate

`CBM_USE_RUST_PLATFORM_CGROUP_ONLY` 會讓 `system_info.c` 的 cgroup CPU/memory helper 直接呼叫 Rust ABI，並排除 C 檔案解析 fallback；`rust-platform-cgroup-only-test` 已通過 foundation、FFI 與 production version gate。

### 全量矩陣回歸

cgroup `_ONLY` 接入後重新執行 `rust-all-optin-test`，wrapper/direct test-runner、production link 及版本 smoke 均成功。

### Registry test-QN direct gate

`CBM_USE_RUST_PIPELINE_REGISTRY_TEST_QN_ONLY` 會排除 `registry.c` 的 test-QN C fallback 並使用 Rust ABI；其餘 registry resolver/cache 仍是 C。default registry `53 passed`，direct gate、FFI 與 production version 均成功。

### Registry classifier 全量回歸

registry test-QN `_ONLY` 接入後，全量 wrapper/direct test-runner 與 production version smoke 均成功。


## 2026-07-13：Pipeline plan direct-only ABI 使用

`CBM_USE_RUST_PIPELINE_PLAN_ONLY=1` 與 `CBM_USE_RUST_PIPELINE_PLAN=1` 共用既有 pipeline plan ABI：`cbm_rs_pipeline_plan_steps_v2`、`cbm_rs_pipeline_full_plan_steps_v1`、incremental post steps 與 extraction-choice describe。`PLAN_ONLY` 僅改變 C caller 的 fallback policy：Rust plan decode/驗證失敗即回傳錯誤，不呼叫 C planner；ABI 不擁有 C graph/pass side-effect 資料結構。`rust-pipeline-plan-only-test` 與 `rust-all-optin-test` 負責維持這個 direct link 與 runtime contract。


## 2026-07-13：Arena C ABI compatibility

`cbm_rs_arena_*` 使用 `#[repr(C)] CbmRsArena` 對齊 `CBMArena` layout。`CBM_USE_RUST_ARENA_ONLY=1` 時，Makefile 排除原 `arena.c`，由 `arena_sprintf.c` 將 `cbm_arena_init`、`init_sized`、`alloc`、`calloc`、`strdup`、`strndup`、`reset`、`destroy`、`total` 轉接到 Rust；`cbm_arena_sprintf` 保留在 C shim 以維持 variadic ABI。Rust allocator ownership 與 destroy/reset 必須在同一 Rust FFI boundary 內配對。

## graph buffer mutation direct-only ABI

`graph-buffer-mutation-only` feature 由 `ffi_graph_buffer_mutation_only.rs` 匯出既有 mutation adapter ABI。`CbmGbufMutationCmd` 使用 `#[repr(C)]`，欄位順序與 `cbm_gbuf_mutation_cmd_t` 相同；Rust 僅檢查 null command、`reserved0` 與三個 kind，之後呼叫 `cbm_gbuf_upsert_node`、`cbm_gbuf_insert_edge` 或 `cbm_gbuf_delete_by_file`。所有字串仍是呼叫期間借用，保存與釋放責任留在 C graph buffer。direct-only build 排除 `graph_buffer_mutation.c`，但保留 `graph_buffer.c` 的全部 storage/SQLite 邊界。

## pipeline FQN direct-only ABI

`pipeline-fqn-only` feature 由 `ffi_pipeline_fqn_only.rs` 匯出 `cbm_pipeline_fqn_compute`、`cbm_pipeline_fqn_module`、`cbm_pipeline_fqn_module_dir`、`cbm_pipeline_fqn_folder` 與 `cbm_pipeline_resolve_relative_import`。每個結果使用 C `malloc` 配置並以 NUL 結尾；呼叫端必須用 `free()` 釋放，relative import 無法解析或超過 C 1024-byte path buffer 時回傳 null。輸入以 C string bytes 處理，不要求 UTF-8，且保留既有 C `bool module_is_dir` ABI。

## Pipeline registry direct-only ABI

`pipeline-registry-only` 提供下列既有 C ABI：`cbm_registry_new/free/add/resolve/fuzzy_resolve/exists/label_of/find_by_name/size/find_ending_with/is_import_reachable`、`cbm_confidence_band`、Perl builtin guard，以及三組 registry cache lifecycle 函式。

- registry handle 為 Rust 所有；C 端只持有 opaque pointer。
- resolve、label、find-by-name 與 fuzzy 結果中的字串由 handle 持有至 `cbm_registry_free`。
- `find_ending_with` 的 `*out` 陣列由 Rust 以 C `malloc` 配置，呼叫端須以 `free(*out)` 釋放陣列，不得釋放其中字串。
- cache lifecycle symbol 目前維持相容 no-op；這不改變結果或 ownership，但不等同於原 C thread-local cache 的效能。

## `system_info` direct-only ABI

`CBM_USE_RUST_SYSTEM_INFO_ONLY=1` 時，C build 不再編譯 `src/foundation/system_info.c`，改由 `rust/cbm-core/src/ffi_system_info_only.rs` 提供下列符號：

- `cbm_system_info`
- `cbm_default_worker_count`
- `cbm_detect_cgroup_cpus`
- `cbm_detect_cgroup_mem`

`CbmSystemInfo` 使用與 C `cbm_system_info_t` 相同的 `repr(C)` 欄位順序。Rust 端自行執行平台與 cgroup 偵測；worker policy 的 invalid `CBM_WORKERS` warning 使用 `cbm_log_workers_env_invalid` 固定 arity bridge，避免直接跨 Rust/C variadic ABI。

此 gate 的 FFI smoke、C/Rust 對照測試、production link 與全 opt-in wrapper/direct 矩陣均已通過。這是單一 foundation slice，不代表其餘 C 核心模組已移除。

## Pipeline worker pool direct-only ABI

`CBM_USE_RUST_PIPELINE_WORKER_POOL_ONLY=1` 時，`cbm_parallel_for` 由 `ffi_pipeline_worker_pool_only.rs` 匯出，C header ABI 維持不變：`count`、nullable callback、opaque context 與 `cbm_parallel_for_opts_t` 的欄位配置不變。Rust 實作保留 serial path、auto worker count、單 worker/單項目短路、主執行緒消費索引、worker stack size 與建立 worker 失敗時的 fallback。

此切片仍是 opt-in direct replacement；未設定旗標時使用原始 `src/pipeline/worker_pool.c`。完整 build/test/link evidence 已記錄於 `docs/Rust-Refactor.md`、`docs/Tasks.md` 與 `docs/Handoff.md`。

## Slab allocator direct-only ABI

`CBM_USE_RUST_SLAB_ALLOC_ONLY=1` 時，`cbm_slab_install`、`cbm_slab_reset_thread`、`cbm_slab_destroy_thread`、`cbm_slab_reclaim` 與 `cbm_slab_test_*` 由 `ffi_slab_alloc_only.rs` 匯出，C header ABI 維持不變。`cbm_slab_install` 仍將 Rust callback 設定至 Tree-sitter；所有配置頁面透過 C allocator 取得，避免改變 mimalloc/CRT ownership。

此切片仍是 opt-in direct replacement；未設定旗標時使用原始 `src/foundation/slab_alloc.c`。完整 build/test/link evidence 已記錄於 `docs/Rust-Refactor.md`、`docs/Tasks.md` 與 `docs/Handoff.md`。

## Profiling runtime direct-only（2026-07-13）

- Rust 實作：`rust/cbm-core/src/ffi_profile_only.rs`。
- C fallback：`src/foundation/profile.c`；`CBM_USE_RUST_PROFILE_ONLY=1` 時不編譯。
- ABI：`cbm_profile_active`、profile init/enable/now/log elapsed 函式；Rust 只透過固定參數 `cbm_log_profile_elapsed` bridge 呼叫 C logging。
- Build：`profile-only` feature、Makefile source selector、CFLAGS 與 all-optin direct flag 已接通。
- Gate：Rust unit tests、profile default/direct test、FFI smoke、production link/version、all-optin wrapper/direct。
- 組合注意事項：profile 與 worker pool 均依賴 `log.c`，Makefile 已避免重複加入該 support source。
- 狀態：此 slice 已完成，但整個專案仍不是完整 C 到 Rust 遷移。

## AST profile codec direct-only（2026-07-13）

- Rust 實作：`rust/cbm-core/src/ffi_ast_profile_codec_only.rs`。
- C fallback：`src/semantic/ast_profile.c` 的 AST traversal；direct flag 只排除三個 codec 函式。
- ABI：`CbmAstProfile` 的 25 個 `uint16_t` 欄位、`cbm_ast_profile_to_str`、`cbm_ast_profile_from_str`、`cbm_ast_profile_to_vector`。
- Build：`ast-profile-codec-only` feature、`CBM_USE_RUST_AST_PROFILE_CODEC_ONLY` C conditional、direct link filter 與 all-optin flag 已接通。
- Gate：Rust all-features 244 tests、clippy、codec default/direct runner、FFI smoke、production link/version、all-optin wrapper/direct。
- 狀態：codec 元件已 direct-only；Tree-sitter traversal 與整個 semantic profile 模組仍未完成 Rust replacement。

## Semantic dense-vector direct-only（2026-07-13）

- Rust 實作：`rust/cbm-core/src/ffi_semantic_vector_only.rs`。
- C fallback：`src/semantic/semantic.c` 的 pretrained/random-index、corpus、scoring 與 diffusion；direct flag 只排除三個 dense-vector primitive。
- ABI：`CbmSemVec` 對齊 `cbm_sem_vec_t` 的 768 維 `float` array；exports 為 cosine、normalize、scaled-add。
- Build：`semantic-vector-only` feature、`CBM_USE_RUST_SEMANTIC_VECTOR_ONLY` C conditional、direct link filter 與 all-optin flag 已接通。
- Gate：Rust all-features 247 tests、clippy、semantic-vector default/direct runner、FFI smoke、production link/version、all-optin wrapper/direct。
- 狀態：dense-vector primitive 已 direct-only；整個 semantic module 與專案仍未完成 Rust replacement。

## Semantic proximity direct-only（2026-07-13）

- Rust 實作：`rust/cbm-core/src/ffi_semantic_proximity_only.rs`。
- C fallback：`src/semantic/semantic.c` 的 vectors、corpus、combined score 與 diffusion；direct flag 只排除 `cbm_sem_proximity`。
- ABI：`cbm_sem_proximity(const char*, const char*) -> float`；Rust 以 C string bytes 保留非 UTF-8 path 行為。
- Build：`semantic-proximity-only` feature、`CBM_USE_RUST_SEMANTIC_PROXIMITY_ONLY` C conditional、direct link filter 與 all-optin flag 已接通。
- Gate：Rust all-features 250 tests、clippy、proximity default/direct runner、FFI smoke、production link/version、all-optin wrapper/direct。
- 狀態：proximity helper 已 direct-only；整個 semantic module 與專案仍未完成 Rust replacement。

## Semantic tokenizer direct-only（2026-07-13）

- Rust 實作：`rust/cbm-core/src/ffi_semantic_tokenize_only.rs`。
- C fallback：`src/semantic/semantic.c` 的 tokenizer；direct flag 啟用時排除 tokenizer helpers 與 `cbm_sem_tokenize` definition。
- ABI：`cbm_sem_tokenize(const char*, char**, int) -> int`；Rust 輸出由 C `malloc` 配置，呼叫端仍負責 `free`。
- Build：`semantic-tokenize-only` feature、`CBM_USE_RUST_SEMANTIC_TOKENIZE_ONLY` C conditional、direct link filter 與 all-optin flag 已接通。
- Gate：Rust all-features 253 tests、clippy、tokenize default/direct runner、FFI smoke、production link/version、all-optin wrapper/direct。
- 狀態：token extraction 已 direct-only；semantic corpus、random-index、combined scoring 與專案其他 C modules 仍未完成 Rust replacement。

## Semantic config/env direct-only（2026-07-13）

`ffi_semantic_config_only.rs` exports：

- `cbm_sem_get_config() -> CbmSemConfig`：以 `repr(C)` 回傳與 `cbm_sem_config_t` 相容的九個 `float` 欄位及 `int max_edges`；threshold 依 C 契約讀取 `CBM_SEMANTIC_THRESHOLD`。
- `cbm_sem_is_enabled() -> bool`：依 C 契約讀取 `CBM_SEMANTIC_ENABLED`，僅首字元為 `1` 時回傳 true。

啟用方式：Cargo feature `semantic-config-only`，C build define `CBM_USE_RUST_SEMANTIC_CONFIG_ONLY`。未啟用時仍使用 C 實作；此切片只替換 config/env 邊界，不包含 semantic indexing、LSH、scoring 或 pass 的全量遷移。

## Discover language direct-only（2026-07-14）

`ffi_language_only.rs` exports：

- `cbm_language_for_extension(const char*) -> int`：查詢 userconfig 覆寫後，再查 Rust 靜態副檔名表。
- `cbm_language_for_filename(const char*) -> int`：查詢特殊檔名、`.env.*`、compound extension 與標準副檔名。
- `cbm_language_name(int) -> const char*`：回傳與 C `LANG_NAMES` 相容的靜態 NUL 結尾名稱。
- `cbm_disambiguate_m(const char*) -> int`：讀取最多 4 KiB，依 C marker 順序判定 Objective-C、Magma 或 MATLAB。

啟用方式：Cargo feature `discover-language-only`，C define `CBM_USE_RUST_DISCOVER_LANGUAGE_ONLY`。production link 使用 `userconfig_bridge.c`；FFI smoke link 使用 `userconfig_bridge_stub.c`。未啟用時仍保留並使用 C `language.c`。

## Discover language 與 gitignore（2026-07-14）

新增兩個 opt-in ABI slice：

- `discover-language-only` 對應 `ffi_language_only.rs`，匯出 `cbm_language_for_extension`、`cbm_language_for_filename`、`cbm_language_name` 與 `cbm_disambiguate_m`。
- `discover-gitignore-only` 對應 `ffi_gitignore_only.rs`，匯出 `cbm_gitignore_load`、`cbm_gitignore_parse`、`cbm_gitignore_matches`、`cbm_gitignore_free`、`cbm_gitignore_merge` 與 `cbm_gitignore_match_result`。

language 的正式 C 設定查詢由 `userconfig_bridge.c` 接回既有 `cbm_get_user_lang_config`；FFI smoke 使用 `userconfig_bridge_stub.c`。所有 raw-pointer ABI 都保留 Rust `# Safety` 契約。gitignore merge 先建立暫存複本，完成後才擴充目的集合，以維持原有 failure atomicity。

本輪證據：Cargo all-features 259 項通過、Clippy strict 通過、gitignore dedicated default/direct 完整 C runner 各 5868 項通過，以及 all-optin wrapper/direct production version 啟動成功。這些 flag 仍是 opt-in；未啟用時 C 實作保留為 fallback，並不代表全專案已完成 Rust 化。

## Discover userconfig（2026-07-14）

`discover-userconfig-only` 對應 `ffi_userconfig_only.rs`，直接匯出：

- `cbm_userconfig_load(repo_path)`：讀取 global/project config，回傳 `repr(C)` 配置指標；NULL repo path 或缺檔視為無 project／空配置。
- `cbm_userconfig_lookup(cfg, ext)`：回傳 language enum，NULL、空 extension 或未命中回傳 `CBM_LANG_COUNT`。
- `cbm_userconfig_free(cfg)`：NULL-safe，回收 Rust-owned entry vector 與每個 `CString`。
- `cbm_set_user_lang_config`／`cbm_get_user_lang_config`：保留 borrowed process-global pointer contract，不取得或延長呼叫端 ownership。

JSON `extra_extensions` 僅接受以 `.` 開頭的 extension 與字串 language；未知語言、非字串值、invalid JSON、非 object root 皆沿用 C 的 fail-open 行為。project entry 會覆蓋 global 同名 entry。language name lookup 透過既有 `cbm_language_name` ABI，讓 C language 實作與 `discover-language-only` 共用唯一 enum source。

`userconfig_app_config_ffi_stub.c` 只提供獨立 FFI smoke 的 `/tmp` config path；`userconfig_ffi_stub.c` 只在 language 未 direct 時提供最小 PHP name。兩者均不進 production。Rust raw-pointer exports 都有 `# Safety` 契約；正式 runtime 的 config directory、language table、pipeline ownership 與 C fallback 仍在 C 或既有 Rust slice 邊界。

## Foundation mem ABI

`mem-only` feature 由 Rust 直接輸出下列既有 C ABI：

- `cbm_mem_init(double)`：只接受第一次初始化，優先使用 `CBM_MEM_BUDGET_MB`。
- `cbm_mem_rss()`、`cbm_mem_peak_rss()`：透過 C bridge 取得 mimalloc process info，必要時回退至平台 RSS。
- `cbm_mem_budget()`、`cbm_mem_over_budget()`、`cbm_mem_worker_budget(int)`：保留原有 budget 與 pressure contract。
- `cbm_mem_collect()`：呼叫專用 `cbm_rs_mem_bridge_collect`，production 由 mimalloc bridge 實作。

`src/foundation/mem.c` 僅在未設定 `CBM_USE_RUST_MEM_ONLY` 時編譯；production bridge 與 FFI smoke stub 分離，避免測試替身進入正式 binary。此切片已通過 Rust、clippy、兩側 5868 項 C suite、FFI smoke 與 production 啟動驗證。

### Foundation diagnostics ABI

`diagnostics-only` feature 與 `CBM_USE_RUST_DIAGNOSTICS_ONLY=1` 會以 Rust 取代 `src/foundation/diagnostics.c`。保留的 C bridge `cbm_rs_diag_process_info` 只負責取得 mimalloc process-info；direct FFI 使用 stub，不改公開 ABI。

保留的 C ABI 包含：

- `g_query_stats`
- `cbm_diag_env_enabled_value`
- `cbm_diag_query_stats_snapshot`
- `cbm_diag_record_query`
- `cbm_diag_format_path`
- `cbm_diag_format_json`
- `cbm_diag_format_ndjson`
- `cbm_diag_start`
- `cbm_diag_stop`

Rust 實作維持既有 JSON 欄位順序、NDJSON 生命週期、atomic snapshot rename、8 MiB trajectory rotation 與 POSIX `/tmp` 路徑政策。`diagnostics-format-only` 同時啟用時， formatter-only Rust slice 提供共用 formatter symbols；完整 diagnostics slice 仍提供 writer 與統計狀態。

## CLI 版本比較 direct slice

cbm_rs_cli_compare_versions_v1(left, right) 以 byte-level parser 對齊
src/cli/cli.c 的 cbm_compare_versions()：支援 v/V 前綴、最多三段十進位
版本、缺少段落補零，以及含 - 的 prerelease 排序。null input 在 Rust ABI
中視為空版本，回傳值只保證維持 C API 的負值、零值或正值比較語意。

設定 CBM_USE_RUST_CLI_VERSION=1 時，C cbm_compare_versions() 委派
cbm_rs_cli_compare_versions_v1()；設定 CBM_USE_RUST_CLI_VERSION_ONLY=1
時，C 端不編譯原本的 semver parser，改呼叫 Cargo feature
cli-version-only 匯出的 cbm_cli_compare_versions_v1()。CLI 的版本狀態、
install/update 檔案 I/O 與其他子命令仍由 C 負責。

本切片已加入 Rust unit、tests/test_rust_ffi.c smoke 與
rust-cli-version-optin-test / rust-cli-version-only-test；本輪尚未執行
這些驗證 gate。


## Discover 字串 helper Rust slice（2026-07-15）

本輪新增四個窄切片 ABI：cbm_rs_discover_str_in_list_v1、cbm_rs_discover_ends_with_v1、cbm_rs_discover_str_contains_v1 與 cbm_rs_discover_ascii_ieq_v1。Rust 實作位於 rust/cbm-core/src/discover.rs，只處理位元組字串比較；NULL 結尾的 C 字串陣列仍由 C 端提供給 str_in_list，沒有搬移 discover 遞迴、檔案 I/O、Git 設定或 tree-sitter runtime。

`CBM_USE_RUST_DISCOVER_STRING_HELPERS=1` 會讓
`src/discover/discover_string_helpers.c` 的 C wrapper 呼叫 Rust v1 ABI；
`CBM_USE_RUST_DISCOVER_STRING_HELPERS_ONLY=1` 則排除整個 C fallback compilation unit，
由 feature-gated Rust direct ABI 取代四個 C helper。default 路徑仍保留 C fallback。

2026-07-15 已驗證 Rust unit `1 passed`、stable/direct FFI smoke、default/wrapper/direct
`discover` suite 各 `85 passed`，以及 wrapper/direct production link 與 `--version`。
direct link line 不含 `src/discover/discover_string_helpers.c`，確認四個同名 symbol 由 Rust
staticlib 供應；本段不代表完整 Discover 或完整 CI 已完成 Rust 驗證。


## Watcher polling interval Rust slice（2026-07-15）

本輪新增 `cbm_rs_watcher_poll_interval_ms_v1`，Rust 實作固定基準、檔案數分段增量與上限
計算。watcher 的時間來源、Git 狀態、輪詢迴圈、thread 與 callback 仍由 C 管理。

`CBM_USE_RUST_WATCHER_POLL_INTERVAL=1` 會讓 C 公開函式成為 Rust v1 wrapper；
`CBM_USE_RUST_WATCHER_POLL_INTERVAL_ONLY=1` 則以 `watcher-poll-interval-only` feature
匯出同名 Rust ABI。C fallback 已拆至 `src/watcher/poll_interval.c`，direct source list
會排除該檔案；既有 `tests/test_watcher.c` 的 interval 測試仍是行為覆蓋，
`tests/test_rust_ffi.c` 另有 stable/direct smoke。

2026-07-15 已驗證：Rust unit `1 passed`、隔離 default/direct full runner 各
`5868 passed`、default/direct FFI smoke、wrapper FFI 與 watcher suite `53 passed`，以及
direct/wrapper production link/`--version`。direct link line 不含
`src/watcher/poll_interval.c`，確認同名 ABI 由 Rust staticlib 供應。


## Pipeline SvelteKit file kind Rust slice（2026-07-15）

`cbm_rs_pipeline_sveltekit_file_kind_v1(file_path)` 回傳 `int` 分類值。`file_path` 可為 null；非 null
時必須指向有效的 NUL 結尾 C 字串。輸入必須包含精確 `/routes/`，再以最後一個 `/` 後的 basename
做 byte-level 比對：`+server.ts`／`.js` 回 1，`+page.server.ts`／`.js` 回 2，
`+layout.server.ts`／`.js` 回 3，其他與 NULL 回 0。Rust 不配置、不保存且不持有輸入指標。

預設 C fallback 位於 `src/pipeline/sveltekit_file_kind.c`／`.h`，
`src/pipeline/pass_route_nodes.c` 只經由 `cbm_pipeline_sveltekit_file_kind()` 公開 bridge 呼叫。
`CBM_USE_RUST_PIPELINE_SVELTEKIT_FILE_KIND=1` 會讓該 C wrapper 呼叫 Rust v1 ABI；預設仍使用同檔
C fallback。`CBM_USE_RUST_PIPELINE_SVELTEKIT_FILE_KIND_ONLY=1` 會由
`pipeline-sveltekit-file-kind-only` feature 匯出同名 direct ABI，且 source selector 排除該 C
compilation unit。

2026-07-15 已驗證原先 direct local-helper 編譯缺口修正後的聚焦 Rust unit `1 passed`、完整 Rust
suite `307 passed`、隔離 default/wrapper/direct `test-rust-ffi`、三種路徑的 pipeline suite 各
`227 passed`，以及 wrapper/direct production link 與 `--version`。direct Make 展開與 production
link line 不含 `src/pipeline/sveltekit_file_kind.c`；本輪未將這些 targeted gates 視為完整 pipeline
route-node 或完整 CI 驗證。

2026-07-16 更正：上述是補齊兩個合法 basename case 與 make -Bn source-negative gate 前的
歷史驗證。本 revision 的 wrapper gate 已重跑；direct gate 尚待重跑。source layout 與 ABI
契約仍成立，但不應把舊的完整 Rust suite、Clippy 或 direct runner 結果寫成目前 revision 的結果。


## Pipeline SvelteKit server method Rust slice（2026-07-15）

本輪新增 `cbm_rs_pipeline_sveltekit_server_method_v1`，將 SvelteKit `+server` handler 名稱映射為
GET、POST、PUT、PATCH、DELETE、OPTIONS、HEAD，並將 `fallback` 映射為 ANY；未知名稱與 null
回傳 null。回傳指標指向 Rust 內部 NUL 結尾靜態常數，Rust 不保存 C 輸入指標。

`CBM_USE_RUST_PIPELINE_SVELTEKIT_SERVER_METHOD=1` 會讓
`src/pipeline/sveltekit_server_method.c` 的 C wrapper 呼叫 Rust v1 ABI；預設仍使用同檔的
C fallback。`CBM_USE_RUST_PIPELINE_SVELTEKIT_SERVER_METHOD_ONLY=1` 會由
`pipeline-sveltekit-server-method-only` feature 匯出同名 direct ABI，且 source selector
排除該 C compilation unit。SvelteKit route synthesis、graph traversal 與 graph mutation 仍在 C。

2026-07-15 已驗證 Rust unit `1 passed`、隔離 default/wrapper/direct FFI smoke、三種路徑的
pipeline suite 各 `222 passed`，以及 wrapper/direct production link 與 `--version`。direct
link line 不含 `src/pipeline/sveltekit_server_method.c`；本輪未將這些 targeted gates 視為完整
pipeline 或完整 CI 驗證。


## CLI hook token Rust slice（2026-07-15）

本輪新增 cbm_rs_cli_hook_extract_token_v1，將 hook augment 的 identifier-like token 掃描移至 Rust：最短 4 個字元、最長 96 個字元，取最長 token 並依輸出 buffer 截斷。hook 的 deadline、stdin、yyjson、MCP query、輸出與 process lifecycle 仍在 C。

CBM_USE_RUST_CLI_HOOK_TOKEN=1 會讓 C static helper 呼叫 Rust v1 ABI；CBM_USE_RUST_CLI_HOOK_TOKEN_ONLY=1 會由 cli-hook-token-only feature 匯出 direct ABI。Rust unit、tests/test_rust_ffi.c smoke 與 Makefile default/wrapper/direct gates 已加入。

本輪尚未執行 Rust unit、FFI、CLI runner、direct production link 或完整 CI gate。


## Git path absolute Rust slice（2026-07-15）

`cbm_rs_git_path_is_absolute_v1(path)` 回傳 `int` 0/1。`path` 可為 null；非 null 時必須指向有效的
NUL 結尾 C 字串。NULL／空字串回傳 0，首 byte 為 `/` 回傳 1（因此 `//server` 亦為 1）；Windows
只接受 ASCII 英文字母後接 `:`，故 `C:` 與 `C:project` 為 1，而數字、非 ASCII drive byte 與
反斜線 UNC 路徑為 0。Rust 不配置、不保存且不持有輸入指標。

預設 C fallback 位於 `src/git/path_absolute.c`／`.h`，`src/git/git_context.c` 只經由
`cbm_git_path_is_absolute()` 公開 bridge 呼叫。`CBM_USE_RUST_GIT_PATH_ABSOLUTE=1` 會讓該 C
wrapper 呼叫 Rust v1 ABI；預設仍使用同檔 C fallback。
`CBM_USE_RUST_GIT_PATH_ABSOLUTE_ONLY=1` 會由 `git-path-absolute-only` feature 匯出同名 direct
ABI，且 source selector 排除該 C compilation unit。

2026-07-15 已驗證聚焦 Rust unit `4 passed`、完整 Rust suite `307 passed`、隔離
default/wrapper/direct `test-rust-ffi`、三種路徑的 pipeline suite 各 `226 passed`，以及
wrapper/direct production link 與 `--version`。direct Make 展開與 production link line 不含
`src/git/path_absolute.c`；本輪未將這些 targeted gates 視為完整 Git runtime 或完整 CI 驗證。


## Git JSON escaped length Rust slice（2026-07-15）

`cbm_rs_git_json_escaped_len_v1` 計算 JSON 字串 escape 後、不含結尾 NUL 的長度。NULL 為 0；在
第一個 NUL 前，引號、反斜線、LF、CR、TAB 各為 2，其餘 `< 0x20` 控制 byte 為 6，其餘 byte
為 1。最大成功值是 `INT_MAX - 1`；若連同 NUL 的 buffer size 無法以 `int` 表示，ABI 回傳 -1。
Rust 不配置 escaped buffer，也不改變 Git context 的 JSON 寫入與 ownership。

預設 C fallback 位於 `src/git/json_escaped_len.c`／`.h`。
`CBM_USE_RUST_GIT_JSON_ESCAPED_LEN=1` 會讓該 C wrapper 呼叫 Rust v1 ABI；預設仍使用同檔的
C fallback。`CBM_USE_RUST_GIT_JSON_ESCAPED_LEN_ONLY=1` 會由 `git-json-escaped-len-only` feature
匯出同名 direct ABI，且 source selector 排除該 C compilation unit。C 的 `json_append_string()`
會在配置前拒絕 -1。

2026-07-15 已驗證 Rust unit `4 passed`、隔離 default/wrapper/direct `test-rust-ffi`、三種路徑的
pipeline suite 各 `224 passed`，以及 wrapper/direct production link 與 `--version`。direct Make
展開與 production link line 不含 `src/git/json_escaped_len.c`；本輪未將這些 targeted gates 視為
完整 Git runtime 或完整 CI 驗證。


## Git trim newlines Rust slice（2026-07-15）

`cbm_rs_git_trim_newlines_v1` 對既有 C command-output buffer 尾端的連續 CR/LF 寫入 NUL，
保留 `trim_newlines` 的字串語意；NULL 為 no-op。Rust 不配置、不保存或持有該 buffer；Git command、
capture 與 process 行為仍在 C。

預設 C fallback 位於 `src/git/trim_newlines.c`／`.h`。
`CBM_USE_RUST_GIT_TRIM_NEWLINES=1` 會讓該 C wrapper 呼叫 Rust v1 ABI；預設仍使用同檔的
C fallback。`CBM_USE_RUST_GIT_TRIM_NEWLINES_ONLY=1` 會由 `git-trim-newlines-only` feature
匯出同名 direct ABI，且 source selector 排除該 C compilation unit。

2026-07-15 已驗證 Rust unit `1 passed`、隔離 default/wrapper/direct `test-rust-ffi`、三種
路徑的 pipeline suite 各 `223 passed`，以及 wrapper/direct production link 與 `--version`。
direct link line 不含 `src/git/trim_newlines.c`；本輪未將這些 targeted gates 視為完整 Git runtime
或完整 CI 驗證。


## Log path query Rust slice（2026-07-15）

`cbm_rs_log_copy_path_without_query_v1(path, buf, bufsize)` 是 void buffer-copy ABI。`path`
可為 null，否則必須是有效的 NUL 結尾 C 字串；`buf` 若非 null，必須指向至少 `bufsize`
bytes 的可寫緩衝區，且不得與 `path` 重疊。`buf == NULL` 或 `bufsize == 0` 時不寫入；
否則 ABI 最多處理 `bufsize - 1` bytes，於首個 NUL、`?` 或 `#` 停止並寫入 NUL。`path ==
NULL` 時輸出空字串。Rust 不保存輸入指標，也不為此 ABI 配置完整 path。

預設 C fallback 位於 `src/foundation/log_path.c`／`.h`。
`CBM_USE_RUST_LOG_COPY_PATH=1` 會讓該 C bridge 呼叫 Rust v1 ABI；預設仍使用同檔 C
fallback。`CBM_USE_RUST_LOG_COPY_PATH_ONLY=1` 會由 `log-copy-path-only` feature 匯出同名
direct ABI，且 source selector 排除該 C compilation unit。logger state、stream、formatting
與輸出生命週期仍由 C 管理。

2026-07-15 已驗證 Rust unit `1 passed`、隔離 default/wrapper/direct FFI smoke、三種路徑的
`log` suite 各 `16 passed`，以及 wrapper/direct production link 與 `--version`；direct link
line 不含 `src/foundation/log_path.c`。這些 targeted gates 不代表完整 logger、foundation runtime
或完整 CI 驗證。


## Pipeline JSON property Rust slice（2026-07-15）

本輪新增 cbm_rs_pipeline_extract_json_prop_v1，搜尋固定的 "key":"value" pattern，將未解碼的 value 複製到 C caller-owned buffer；空值或 buffer 不足維持 C helper 的 false 行為。graph traversal、route synthesis 與 JSON ownership 仍在 C。

CBM_USE_RUST_PIPELINE_JSON_PROP=1 會讓 C static helper 呼叫 Rust v1 ABI；CBM_USE_RUST_PIPELINE_JSON_PROP_ONLY=1 會由 pipeline-json-prop-only feature 匯出 direct ABI。Rust unit、tests/test_rust_ffi.c smoke 與 Makefile default/wrapper/direct gates 已加入。

**2026-07-16 true-source：** 已拆 `src/pipeline/json_prop.c`／公開 bridge
`cbm_pipeline_extract_json_prop`；`PIPELINE_JSON_PROP_SRCS` 在 `_ONLY` 下為空；pipeline
suite 各 **227**、production `--version`、`make -Bn` source-negative 已通過。完整
`scripts/test.sh` 尚未重跑。


## Pipeline URL path Rust slice（2026-07-15）

cbm_rs_pipeline_url_path_v1 維持既有 raw string scan：先找第一個 ://，再找其後第一個
/；無 scheme 時回傳原 input pointer，有 slash 時回傳 input 內部 pointer，僅無後續 slash
時回傳 NUL 結尾的 Rust 靜態根 path。這不是 RFC URL parser；Rust 不配置、不保存 input，
回傳 pointer 均為 borrowed，caller 不得釋放。

預設 C fallback 位於 src/pipeline/url_path.c/.h。CBM_USE_RUST_PIPELINE_URL_PATH=1
使公開 C bridge cbm_pipeline_url_path() 呼叫 Rust v1 ABI；預設仍使用同檔 C fallback。
CBM_USE_RUST_PIPELINE_URL_PATH_ONLY=1 由 pipeline-url-path-only feature 匯出同名 direct
ABI，且 source selector 排除該 C compilation unit；route graph、HTTP matching 與 graph
mutation 仍由 C 管理。

2026-07-15 已驗證 cargo test -p cbm-core --locked（307 passed）、clippy、fmt、隔離
default/wrapper/direct FFI smoke 與 pipeline suite 各 225 passed，以及 wrapper/direct
production link 與 --version；direct Make selector、link line 和 source-negative gate 均不含
src/pipeline/url_path.c。這些 targeted gates 不代表完整 CI 已通過。


## Discover trailing separator Rust slice（2026-07-15；2026-07-16 source replacement）

`cbm_rs_discover_has_trailing_sep_v1(value)` 回傳 `int`：尾端為 `/` 或 `\\` 時非 0，否則 0。
`value` 可為 null 或 NUL 結尾 C 字串；null 與空字串回 0。Rust 只處理字串 predicate，不配置
或保存輸入指標。discover path join、Git path expansion、I/O 與遞迴掃描仍在 C。

預設 C fallback 位於 `src/discover/trailing_sep.c`／`.h`，公開 bridge 為
`cbm_discover_has_trailing_sep()`。`src/discover/discover.c` 的 path_join C fallback 只呼叫
此 bridge。公開 bridge 契約明確為 NULL／empty → false；這是將 private helper 公開化時凍結
的防禦性行為，不是舊 static helper 對 NULL 呼叫 `strlen` 的 UB parity。

`CBM_USE_RUST_DISCOVER_TRAILING_SEP=1` 時，C fallback 委派 Rust v1 ABI。
`CBM_USE_RUST_DISCOVER_TRAILING_SEP_ONLY=1` 時，`discover-trailing-sep-only` feature 匯出同名
direct ABI，且 `DISCOVER_TRAILING_SEP_SRCS` selector 排除 C fallback。`RUST_ALL_OPTIN_DIRECT_FLAGS`
會 filter-out wrapper flag。

2026-07-16 已驗證：Rust unit、default/wrapper/direct 實際執行的 FFI smoke、discover suite 各
86 passed、wrapper/direct production `--version`，以及 `make -pn` selector 空值與 `make -Bn`
production recipe 不含 `src/discover/trailing_sep.c`。完整 `scripts/test.sh` 尚未重跑。


## Discover path join Rust slice（2026-07-15）

本輪新增 cbm_rs_discover_path_join_v1，組合 base/relative path、處理尾端分隔符、依 caller buffer 容量截斷，並正規化反斜線。Rust 不處理 Git path expansion、環境變數、檔案 I/O 或 discover recursion。

CBM_USE_RUST_DISCOVER_PATH_JOIN=1 會讓 C static helper 呼叫 Rust v1 ABI；CBM_USE_RUST_DISCOVER_PATH_JOIN_ONLY=1 會由 discover-path-join-only feature 匯出 direct ABI。Rust unit、tests/test_rust_ffi.c smoke 與 Makefile default/wrapper/direct gates 已加入。

**2026-07-16 true-source：** 已拆 `src/discover/path_join.c`／公開 bridge
`cbm_discover_path_join`；`DISCOVER_PATH_JOIN_SRCS` 在 `_ONLY` 下為空；discover suite 各
**87**、production `--version`、`make -Bn` source-negative 已通過。完整 `scripts/test.sh`
尚未重跑。


## SimHash MinHash Jaccard stable FFI slice

`cbm_minhash_jaccard` / `cbm_minhash_to_hex` / `cbm_minhash_from_hex` 已新增
`CBM_USE_RUST_SIMHASH_JACCARD=1` opt-in wrapper，將固定 64 槽的 `uint32_t` MinHash 比對
與 hex codec 委派至 `rust/cbm-core/src/simhash.rs`，並由 `cbm_rs_simhash_*_v1` 提供 stable
ABI。此切片不搬移 Tree-sitter AST tokenization、xxHash、MinHash fingerprint 建立或 LSH
index ownership。

**2026-07-16 true-source：** 已拆 `src/simhash/minhash_jaccard.c`；
`SIMHASH_JACCARD_SRCS` 在 `_ONLY` 下為空；direct feature `simhash-jaccard-only` 匯出同名
`cbm_minhash_jaccard` / `cbm_minhash_to_hex` / `cbm_minhash_from_hex`；simhash suite 各
**24**、production `--version`、`make -Bn` source-negative 已通過。`minhash.c` 的 compute/LSH
仍在 C。


## Pipeline complexity JSON helper slice

`src/pipeline/pass_complexity.c` 的 `json_get_int` 與 `json_get_bool` 已新增 `CBM_USE_RUST_PIPELINE_COMPLEXITY_JSON=1` opt-in wrapper，委派至 `rust/cbm-core/src/pipeline_complexity.rs` 與 stable ABI。Rust 只處理 flat JSON property lookup；graph DFS、transitive loop depth、cycle detection、node property mutation 與 logging 仍由 C 負責。已加入 `rust-pipeline-complexity-json-optin-test`，尚未執行。


## VMem page-round helper slice

`src/foundation/vmem.c` 未納入產品 source list。Rust `round_to_page` stable ABI 只作為非產品 FFI parity fixture，`rust-vmem-page-round-optin-test` 只執行 FFI smoke；2026-07-15 的 smoke 與 Rust unit 2 項已通過。它沒有 default/opt-in 產品 runtime 證據，不得視為 C→Rust replacement。


## Pipeline pkgmap text true-source slice（#38，2026-07-19）

`src/pipeline/pkgmap_text.c/.h` 是可選 C fallback，`pass_pkgmap.c` 僅保留 manifest parsing、I/O、
entry ownership 與 orchestration。公開 ABI 包含 `cbm_pipeline_pkgmap_at_prefix`、
`cbm_pipeline_pkgmap_find_line_value`、`cbm_pipeline_pkgmap_path_dirname`、
`cbm_pipeline_pkgmap_strip_extension`、`cbm_pipeline_pkgmap_join_and_strip` 與
`cbm_pipeline_pkgmap_build_entry_path`。wrapper `CBM_USE_RUST_PIPELINE_PKGMAP_TEXT=1` 轉調既有
v1；direct `CBM_USE_RUST_PIPELINE_PKGMAP_TEXT_ONLY=1`／`pipeline-pkgmap-text-only` 由 Rust 匯出同名
public ABI。

bounded prefix 的 NULL／負長度回 false；line lookup 的 NULL／負長度回 NULL，成功值借用 C source，
source 可依 explicit length 含 NUL byte。路徑輸入採 first-NUL；NULL dirname／stem 回配置空字串，
NULL dir 視為空、NULL／空 entry 的 join 回 NULL。四個 path 產物以 C malloc 配置且 caller free；
join/build 的既有 1023-byte 截斷發生在 strip extension 前。FFI v1 的 path 介面使用 length-query +
caller buffer，Rust direct 同樣用 C malloc 配置結果，避免跨邊界 allocator 不明。Rust unit **4 passed**、
slice clippy、`rust-pipeline-pkgmap-text-{optin,only}-test` 的 default/wrapper/direct FFI、pipeline
**235 passed**、production `--version`、empty selector、source-negative（排除 fallback，保留
`pass_pkgmap.c`）均通過。


## Pipeline configlink helper slice

`src/pipeline/pass_configlink.c` 的 manifest/dependency predicates 與 dependency-to-import confidence score 已新增 `CBM_USE_RUST_PIPELINE_CONFIGLINK_HELPERS=1` opt-in，透過 stable ABI 接到 Rust。graph buffer traversal、edge insertion、node borrowing、property JSON 與三種 linking strategy orchestration 仍在 C。

**2026-07-16 true-source：** 已拆 `src/pipeline/configlink_helpers.c`／公開 bridge
`cbm_pipeline_configlink_is_manifest_file`、`is_dep_section`、`is_cargo_dep_section`、
`lowercase_into`、`match_dep_to_import`；`CONFIGLINK_HELPERS_SRCS` 在 `_ONLY` 下為空；
direct feature `pipeline-configlink-helpers-only`；pipeline suite 各 **227**、production
`--version`、`make -Bn` source-negative 已通過。


## Compile command tokenizer slice

`src/pipeline/pass_compile_commands.c` 的 `cbm_split_command` 已新增 `CBM_USE_RUST_PIPELINE_SPLIT_COMMAND=1` opt-in。Rust 只負責 quote/space tokenizer，結果透過 C `malloc` 配置每個 token，沿用 C 呼叫端既有 free contract；yyjson parsing、include/define extraction、compile flag ownership 與 repo path filtering 仍在 C。`rust-pipeline-split-command-optin-test` 已建立，尚未執行。


## Cross-repo JSON property slice

`src/pipeline/pass_cross_repo.c` 的 `json_str_prop` 已新增 `CBM_USE_RUST_PIPELINE_CROSS_REPO_JSON=1` opt-in。Rust 只在 caller buffer 中複製 bounded JSON string value，SQLite cross-repo lookup、雙向 edge insertion、store ownership 與 property assembly 仍由 C 負責；`rust-pipeline-cross-repo-json-optin-test` 已建立但尚未執行。


## Pipeline enrichment tokenizer slice（#37 true-source）

`src/pipeline/enrichment_tokens.c/.h` 是 `cbm_split_camel_case` 與
`cbm_tokenize_decorator` 的 C fallback；`pass_enrichment.c` 只保留 graph label collection、hash
table word frequency、yyjson decorator_tags injection 與 store pass orchestration。兩個 public ABI
都接受 `const char *`、`char **out`、`int max_out`：NULL input、NULL out 或 `max_out <= 0` 回 0 且
不得改寫 out；輸入是 raw-byte、first-NUL C string。成功 token 由 C `malloc` 配置，caller 逐一
`free()`。

camel 僅在 ASCII 小寫→大寫邊界切開。decorator 最多處理前 255 bytes；處理開頭 `@` 與 `#[`（只在
末尾 `]` 時移除），捨棄第一個 `(` 後內容，並以 `.`、`_`、`-`、`:`、`/` 分隔。lowercase 契約
明確為 ASCII `A`–`Z`；非 ASCII byte 原樣保留，與 LC_CTYPE 無關。OOM 排除於 normal-resource
C/Rust parity；不保證 count／NULL output slot 的可觀察結果。

wrapper `CBM_USE_RUST_PIPELINE_ENRICHMENT_TOKENS=1` 透過
`cbm_rs_pipeline_split_camel_case_v1`／`cbm_rs_pipeline_tokenize_decorator_v1` 委派 Rust。direct
`CBM_USE_RUST_PIPELINE_ENRICHMENT_TOKENS_ONLY=1`／Cargo `pipeline-enrichment-tokens-only` 由 Rust
staticlib 匯出同名 public ABI。`PIPELINE_ENRICHMENT_TOKENS_SRCS =` 時排除 fallback，
`RUST_FFI_SUPPORT_SRCS` 在 default/wrapper 連入 C bridge，production recipe 仍保留
`pass_enrichment.c`。

**2026-07-19 已驗證：** `cargo fmt --all -- --check`、`pipeline_enrichment` Rust unit **5 passed**、
切片 clippy、`rust-pipeline-enrichment-tokens-optin-test` 與 `…-only-test`。default/wrapper/direct
FFI、pipeline 各 **233 passed**、production `--version` 均成功；direct empty selector、fallback
source-negative 與 `pass_enrichment.c` 正向檢查均通過。完整 all-features clippy 仍有既有
pipeline calls/usages 的重複 `malloc` 宣告，未為本切片修正。


## Calls import JSON helper slice

`src/pipeline/pass_calls.c` 的 local_name 擷取已 true-source 拆至 `src/pipeline/calls_json.c`。

公開 bridge：`char *cbm_pipeline_calls_extract_local_name(const char *props_json)` — NULL／缺鍵／
空 value 回 NULL；成功為 C `malloc` 字串（caller `free`）；raw-byte first-NUL 掃描。

v1：`cbm_rs_pipeline_calls_extract_local_name_v1(buf, bufsize, json)` 為 length-query ABI
（失敗 `SIZE_MAX`）。wrapper `CBM_USE_RUST_PIPELINE_CALLS_JSON=1` 經 C CU 委派 v1；
direct `CBM_USE_RUST_PIPELINE_CALLS_JSON_ONLY=1`／Cargo `pipeline-calls-json-only` 匯出同名
public bridge（C malloc）。`PIPELINE_CALLS_JSON_SRCS` 在 ONLY 為空。

**2026-07-17 已驗證：** fmt、Rust unit 1 passed、clippy；optin/only default/wrapper/direct
pipeline 各 **231 passed**、production `--version`；direct empty selector + source-negative；
`pass_calls.c` 仍編入。import map／CALLS edge 仍在 C。


## Usages import JSON helper slice（#30）

`src/pipeline/pass_usages.c` 的 IMPORTS edge `"local_name":"..."` 擷取已 true-source 拆至
`src/pipeline/usages_json.c`。

公開 bridge：`char *cbm_pipeline_usages_extract_local_name(const char *props_json)` — NULL／缺鍵／
空 value 回 NULL；成功為 C `malloc` 字串（caller `free`）；raw-byte first-NUL 掃描，無 JSON
unescape。

v1 保留：`cbm_rs_pipeline_usages_local_name_len_v1`／`_copy_v1`（失敗 `SIZE_MAX`）。
wrapper `CBM_USE_RUST_PIPELINE_USAGES_JSON=1` 經 C CU 委派 v1；
direct `CBM_USE_RUST_PIPELINE_USAGES_JSON_ONLY=1`／Cargo `pipeline-usages-json-only` 匯出同名
public bridge（C malloc）。`PIPELINE_USAGES_JSON_SRCS` 在 ONLY 為空；
`RUST_FFI_SUPPORT_SRCS` 已納入。

**2026-07-17 已驗證：** fmt、Rust unit 1 passed、clippy；optin/only default/wrapper/direct
pipeline 各 **231 passed**、production `--version`；direct empty selector + source-negative；
`pass_usages.c` 仍編入。import map／graph／edge ownership 仍在 C。


## MCP search_mode classifier slice（#31）

`src/mcp/mcp.c` 的 `parse_search_mode` 已 true-source 拆至 `src/mcp/search_mode.c`。

公開 bridge：`int cbm_mcp_search_mode(const char *mode_str)` — `full`→1、`files`→2；
NULL／空／`compact`／未知／大小寫不符／尾端空白→0（byte-exact strcmp）。

v1：`cbm_rs_mcp_search_mode_v1`。wrapper `CBM_USE_RUST_MCP_SEARCH_MODE=1` 或既有
`CBM_USE_RUST_MCP_CODEC=1` 經 C CU 委派 v1；
direct `CBM_USE_RUST_MCP_SEARCH_MODE_ONLY=1`／Cargo `mcp-search-mode-only` 匯出同名 public ABI。
`MCP_SEARCH_MODE_SRCS` 在 ONLY 為空；不得以整個 `mcp.c` 為 ONLY 排除目標。

**2026-07-17 已驗證：** fmt、Rust unit、clippy；optin/only default/wrapper/direct mcp 各
**136 passed**、production `--version`；direct empty selector + source-negative；`mcp.c` 仍編入。
handler／grep／response／transport 仍在 C。


## MCP index_mode classifier slice（#32）

`src/mcp/mcp.c` 的 `parse_index_repository_mode` 已 true-source 拆至 `src/mcp/index_mode.c`。

公開 bridge：`int cbm_mcp_index_mode(const char *mode_str)` —
`moderate`→1、`fast`→2、`cross-repo-intelligence`→3（dispatch sentinel）；
NULL／空／`full`／未知／大小寫不符／尾端空白→0（full/default；byte-exact strcmp）。

v1：`cbm_rs_mcp_index_mode_v1`。wrapper `CBM_USE_RUST_MCP_INDEX_MODE=1` 或既有
`CBM_USE_RUST_MCP_CODEC=1` 經 C CU 委派 v1；
direct `CBM_USE_RUST_MCP_INDEX_MODE_ONLY=1`／Cargo `mcp-index-mode-only` 匯出同名 public ABI。
`MCP_INDEX_MODE_SRCS` 在 ONLY 為空；不得以整個 `mcp.c` 為 ONLY 排除目標。

**2026-07-18 已驗證：** fmt、Rust unit、clippy；optin/only default/wrapper/direct mcp 各
**136 passed**、production `--version`；direct empty selector + source-negative；`mcp.c` 仍編入。
handler／pipeline／transport 仍在 C。


## MCP search_path_arg classifier slice（#33）

`validate_search_path_arg` 已 true-source 拆至 `src/mcp/search_path_arg.c`。

公開 bridge：`bool cbm_mcp_search_path_arg_valid(const char *input)` — NULL→false；空→true；
拒 `'";|$`<>` CR/LF；非 Windows 另拒 `\`；接受 space 與 `&`。

v1：`cbm_rs_mcp_search_path_arg_valid_v1`。wrapper `CBM_USE_RUST_MCP_SEARCH_PATH_ARG=1` 或
CODEC；direct `…_ONLY`／`mcp-search-path-arg-only`。`MCP_SEARCH_PATH_ARG_SRCS` ONLY 為空。

**2026-07-18 已驗證：** mcp 各 **136 passed**、production `--version`、empty selector +
source-negative；grep／handler 仍在 C。


## MCP search_line_span classifier slice（#34）

`find_tightest_node` 的 per-node span 已 true-source 拆至 `src/mcp/search_line_span.c`。

公開 bridge：`int cbm_mcp_search_line_match_span(int start_line, int end_line, int line)` —
命中→`end-start`；未命中→`-1`。

v1：`cbm_rs_mcp_search_line_match_span_v1`。wrapper `CBM_USE_RUST_MCP_SEARCH_LINE_SPAN=1` 或
CODEC；direct `…_ONLY`／`mcp-search-line-span-only`。`MCP_SEARCH_LINE_SPAN_SRCS` ONLY 為空。

**2026-07-18 已驗證：** mcp 各 **136 passed**、production `--version`、empty selector +
source-negative；node 迴圈／pick_tightest 仍在 C。


## MCP search_args combiner slice（#35）

`validate_search_args` 已 true-source 拆至 `src/mcp/search_args.c`。

公開 bridge：`bool cbm_mcp_search_args_valid(const char *root_path, const char *file_pattern)` —
root 必填且通過 path_arg；file_pattern 可 NULL。C fallback 呼叫
`cbm_mcp_search_path_arg_valid`。

v1：`cbm_rs_mcp_search_args_valid_v1`。wrapper `CBM_USE_RUST_MCP_SEARCH_ARGS=1` 或 CODEC；
direct `…_ONLY`／`mcp-search-args-only`。`MCP_SEARCH_ARGS_SRCS` ONLY 為空。

**2026-07-18 已驗證：** mcp 各 **136 passed**、production `--version`、empty selector +
source-negative。


## MCP search_score_cmp slice（#36）

`search_result_cmp` 的 score delta 已 true-source 拆至 `src/mcp/search_score_cmp.c`。

公開 bridge：`int cbm_mcp_search_score_cmp(int left_score, int right_score)` →
`right_score - left_score`（descending qsort）。

v1：`cbm_rs_mcp_search_score_cmp_v1`。wrapper `CBM_USE_RUST_MCP_SEARCH_SCORE_CMP=1` 或 CODEC；
direct `…_ONLY`／`mcp-search-score-cmp-only`。`MCP_SEARCH_SCORE_CMP_SRCS` ONLY 為空。

**2026-07-18 已驗證：** mcp 各 **136 passed**、production `--version`、empty selector +
source-negative；qsort／result ownership 仍在 C。

## MCP search_code_score slice（#39）

`compute_search_score` 的純加權已 true-source 拆至 `src/mcp/search_code_score.c`。

公開 bridge：`int cbm_mcp_search_code_score(const char *label, const char *file, int in_degree)`。
label／file 為 nullable NUL-terminated C string，僅讀到 first-NUL；NULL 不加權。以 `in_degree`
為基底，Function／Method +10、Route +15、vendored/vendor/node_modules -50、test/spec/_test. -5，
可累加。無 heap、無輸入改寫、無排序或 I/O。

v1：`cbm_rs_mcp_search_code_score_v1`。wrapper `CBM_USE_RUST_MCP_SEARCH_CODE_SCORE=1` 或 CODEC；
direct `…_ONLY`／`mcp-search-code-score-only`。`MCP_SEARCH_CODE_SCORE_SRCS` ONLY 為空，Rust 匯出
同名 public ABI；`mcp.c` 保留 result ownership、qsort、grep 與 JSON orchestration。

**2026-07-19 已驗證：** mcp 各 **137 passed**、production `--version`、empty selector +
source-negative；Rust unit **1 passed**、fmt 與切片 clippy 通過。
## Envscan filename classifier slice

`src/pipeline/pass_envscan.c` 的 Dockerfile、env file、secret file、ignored directory 與 file type classifiers 已新增 `CBM_USE_RUST_PIPELINE_ENVSCAN_CLASSIFIERS=1` opt-in。Regex compilation、檔案掃描、URL/secret policy 與 binding output 仍在 C。

**2026-07-16 true-source：** 已拆 `src/pipeline/envscan_classifiers.c`／公開 bridge
`cbm_pipeline_envscan_is_{dockerfile_name,env_file_name,secret_file,ignored_dir}`、
`cbm_pipeline_envscan_detect_file_type`；`ENVSCAN_CLASSIFIERS_SRCS` 在 `_ONLY` 下為空；
direct feature `pipeline-envscan-classifiers-only`；pipeline suite 各 **227**、production
`--version`、`make -Bn` source-negative 已通過。


## LSP cross classifier slice

`src/pipeline/pass_lsp_cross.c` 的 `cbm_pxc_ts_modes` 與 `cbm_pxc_has_cross_lsp` 已新增 `CBM_USE_RUST_PIPELINE_LSP_CLASSIFIERS=1` opt-in。Rust 只回傳 TypeScript mode bitmask 與 language capability predicate；file I/O、arena/borrowed definitions、LSP resolver、graph merge 與 cross-call ownership 仍在 C。`rust-pipeline-lsp-classifiers-optin-test` 已建立但尚未執行。

LSP classifier slice 另包含 `pxc_map_label` 的 type-like/Protocol/Function/Method allowlist，仍以 C 借用 label pointer 回傳原字串。


## Semantic edges JSON reader slice

`src/pipeline/pass_semantic_edges.c` 的 `json_str_value` 與 `json_str_array` 已新增 `CBM_USE_RUST_PIPELINE_SEMANTIC_JSON=1` opt-in。Rust 保留簡單 strstr/quote scan 與 caller-owned `char**` allocations；embedding、vector/LSH、parallel workers、graph diffusion 與 edge emission 仍在 C。`rust-pipeline-semantic-json-optin-test` 已建立但尚未執行。

Pkgmap slice 現再包含 `join_and_strip` 與 `build_entry_path`，同樣使用 length-query/caller buffer ABI；C 仍掌管 heap ownership。

Configlink slice 另納入 fixed-buffer `lowercase_into`，Rust 只寫入 caller buffer，不取得其 ownership。

Cross-repo slice 另加入 `build_cross_props` formatter，Rust 只寫入 caller-owned JSON buffer，不進行 JSON escape，並保留 optional `url_path`/`transport` 欄位契約。

Compile-command slice 另包含 `resolve_path` length-query helper，C 仍負責 compile flag allocations 與 lifecycle。

### pipeline infrascan JSON array cleaner

`CBM_USE_RUST_PIPELINE_INFRASCAN=1`（以及 direct gate）現在也可將 `cbm_clean_json_brackets()` 委派至 `pipeline::infrascan::clean_json_brackets()` 與 `cbm_rs_pipeline_clean_json_brackets_v1`。Rust 只處理 bounded 的陣列括號、引號、逗號與空白正規化；infrascan 的檔案讀取、內容解析、secret detection、manifest 判斷、graph ownership 與 pipeline side effects 仍由 C 負責。C default path 保持不變；本切片尚未驗證。

### pipeline definitions JSON escape helper

新增 `CBM_USE_RUST_PIPELINE_DEFINITIONS_JSON=1` 與 `pipeline_definitions::{json_escape_char,json_escaped_len}`，由 `cbm_rs_pipeline_definitions_json_escape_char` 和 `cbm_rs_pipeline_definitions_json_escaped_len` 提供 stable ABI。此 slice 只委派 definitions pass 的 JSON 字元 escape 與 escaped length；檔案讀取、Tree-sitter 結果、atomic field append、graph mutation 與 pipeline side effects 仍由 C 負責。default path 保持 C。

**2026-07-16 true-source：** 已拆 `src/pipeline/definitions_json.c`／`.h`，公開 bridge 為 `cbm_pipeline_definitions_json_escape_char` 與 `cbm_pipeline_definitions_json_escaped_len`；`PIPELINE_DEFINITIONS_JSON_SRCS` 在 `_ONLY` 下為空，direct feature `pipeline-definitions-json-only` 匯出同名 ABI。`value == NULL` 時 escaped length 回傳 0；`output == NULL` 或容量不足時不寫入、仍回傳所需 1/2 bytes；raw control byte 降級為空白，Rust 不保存或配置 C buffer。已實際通過 `rust-pipeline-definitions-json-optin-test` 與 `rust-pipeline-definitions-json-only-test` 的 default/wrapper/direct FFI smoke、pipeline `227 passed`、wrapper/direct production `--version`、selector 空與 `make -Bn` production source-negative（不含 `src/pipeline/definitions_json.c`）。true-source 進度由 **13/18 -> 14/18**；完整 `scripts/test.sh` 尚未重跑。

### pipeline similarity fingerprint parser

新增 `CBM_USE_RUST_PIPELINE_SIMILARITY_FP=1` 與 `simhash::parse_fp_from_props()`，由
`cbm_rs_pipeline_similarity_parse_fp_v1` 將固定格式的 `"fp":"..."` properties 欄位解碼至
`uint32_t[64]`。此 slice 只處理 JSON 欄位定位與 MinHash hex decode；LSH index、similarity
threshold、graph edge 建立與 pipeline side effects 仍由 C 負責。default path 保持 C。

**2026-07-16 true-source：** 已拆 `src/pipeline/similarity_fp.c`／`.h`，公開 bridge 為
`cbm_pipeline_similarity_parse_fp`；`PIPELINE_SIMILARITY_FP_SRCS` 在 `_ONLY` 下為空，direct
feature `pipeline-similarity-fp-only` 匯出同名 ABI。`props_json == NULL` 或 output 為 NULL 時
回傳 0 且不寫入；成功時完整覆寫 64 個 `uint32_t`，Rust 不保存 C 指標或配置 C buffer。
已實際通過 `rust-pipeline-similarity-fp-optin-test` 與
`rust-pipeline-similarity-fp-only-test` 的 default/wrapper/direct FFI smoke、pipeline
`227 passed`、wrapper/direct production `--version`、selector 空與 `make -Bn` production
source-negative（不含 `src/pipeline/similarity_fp.c`）。true-source 進度由
**15/18 -> 16/18**；完整 `scripts/test.sh` 尚未重跑。

### pipeline infrascan JSON cleaner

新增 `CBM_USE_RUST_PIPELINE_INFRASCAN_JSON=1`，由
`cbm_rs_pipeline_clean_json_brackets_v1` 處理 Dockerfile CMD／ENTRYPOINT 的 JSON bracket
array 正規化。此 slice 只處理引號移除、逗號轉空白、ASCII 空白與 tab 折疊、尾端空白移除，
以及非 array 的截斷複製；Dockerfile parse、命令擷取、graph ownership 與 pipeline side
effects 仍由 C 負責。default path 保持 C。

**2026-07-17 true-source：** 已拆 `src/pipeline/infrascan_json.c`／`.h`，公開 bridge 為
`cbm_clean_json_brackets`；ONLY feature `pipeline-infrascan-json-only` 匯出同名 ABI，且
production selector 在 `_ONLY` 下不含 `src/pipeline/infrascan_json.c`。`input == NULL`、
output 為 NULL 或 `output_size == 0` 時不寫入；Rust 不保存 C 指標或配置 C buffer。

已實際通過 `rust-pipeline-infrascan-json-optin-test` 與
`rust-pipeline-infrascan-json-only-test` 的 default/wrapper/direct FFI smoke、pipeline
`227 passed`、wrapper/direct production `--version`、selector 空與 `make -Bn` production
source-negative（不含 `src/pipeline/infrascan_json.c`）。true-source 進度由
**16/18 -> 17/18**；完整 `scripts/test.sh` 尚未重跑。

### pipeline test-node `is_test` classifier

新增 `CBM_USE_RUST_PIPELINE_TEST_NODE_IS_TEST=1`，由 wrapper v1 ABI
`cbm_rs_pipeline_test_node_is_test_v1` 承接 tests pass 的 properties classifier。此 slice
只判定 properties JSON 中是否含有嚴格、連續的 `"is_test":true`；空白變體、`false` 與
其他內容均回 false。TESTS / TESTS_FILE edge 建立、test path classifier、graph traversal、
graph-buffer adapter、tree-sitter/extraction 與 pipeline side effects 仍由 C 負責。

**2026-07-17 true-source：** 已拆 `src/pipeline/test_node_is_test.c`／`.h`，公開 bridge 為
`cbm_pipeline_test_node_is_test(const char *properties_json) -> bool`。default 與 wrapper 均編入
此 C bridge；default 保留 C fallback，wrapper 則呼叫 v1 ABI。`properties_json == NULL` 回 false；
Rust 不保存 C 指標，也不配置 C 記憶體。

`CBM_USE_RUST_PIPELINE_TEST_NODE_IS_TEST_ONLY=1`／Cargo
`pipeline-test-node-is-test-only` 會由 Rust staticlib 直接匯出同名 public ABI，且 production
selector 在 `_ONLY` 下為空，不編譯 `src/pipeline/test_node_is_test.c` fallback。已實際通過
`rust-pipeline-test-node-is-test-optin-test` 與
`rust-pipeline-test-node-is-test-only-test` 的 default/wrapper/direct FFI smoke、pipeline 各
`228 passed`、wrapper/direct production `--version`、selector 空與 `make -Bn` production
source-negative（不含 `src/pipeline/test_node_is_test.c`）。true-source 進度由
**17/18 -> 18/18**；完整 `scripts/test.sh` 尚未重跑。

### Semantic camel-break classifier true-source

新增 `src/semantic/camel_break.c`／`.h`，公開 ABI 為
`cbm_semantic_is_camel_break(const char *name, int index)`。既有
`CBM_USE_RUST_SEMANTIC_TOKEN_CLASSIFIERS=1` wrapper 透過
`cbm_rs_semantic_is_camel_break_v1` 委派此 classifier，但不代表整個 semantic module 已移轉；
delimiter 與 tokenization 仍由 C 負責。

**2026-07-17 true-source：** `CBM_USE_RUST_SEMANTIC_CAMEL_BREAK_ONLY=1`／Cargo
`semantic-camel-break-only` 由 Rust staticlib 直接匯出同名 public ABI；
`SEMANTIC_CAMEL_BREAK_SRCS =`，direct source-negative 證明 production link 不含
`src/semantic/camel_break.c`。`name == NULL`、負值或零 `index` 均回 false；非 NULL 的正
`index` 只針對有效 NUL 結尾 `name` 中的非 NUL byte 判定，且僅 ASCII 小寫轉大寫 transition
回 true。Rust 不跨 ABI 配置或取得 C 記憶體 ownership。

已實際通過 Rust fmt、semantic token classifier test `2 passed` 與 clippy；
default/wrapper/direct FFI smoke 與 pipeline 各 `228 passed`，wrapper/direct production
`--version`、direct empty selector 與 source-negative 均成功。完整 `scripts/test.sh` 尚未重跑。
true-source 進度由 **18/18 -> 19/19**。

### Semantic token delimiter true-source

新增 `src/semantic/token_delim.c`／`.h`，公開 bridge 與 direct ABI 均為
`bool cbm_semantic_is_token_delim(unsigned char byte)`。既有
`CBM_USE_RUST_SEMANTIC_TOKEN_CLASSIFIERS=1` wrapper 透過
`cbm_rs_semantic_is_token_delim_v1(int byte)` 委派此 scalar classifier；不傳遞 pointer、
沒有 NULL／buffer 契約，且 Rust 不跨 ABI 配置或取得 C 記憶體 ownership。

**2026-07-17 true-source：** `CBM_USE_RUST_SEMANTIC_TOKEN_DELIM_ONLY=1`／Cargo
`semantic-token-delim-only` 由 Rust staticlib 匯出同名 public ABI；direct
`SEMANTIC_TOKEN_DELIM_SRCS =`，且 `make -Bn` source-negative 證明 production link 不含
`src/semantic/token_delim.c`。九個 ASCII delimiter `.`、`/`、`_`、`-`、空白、`(`、`)`、`,`、
`:` 回 true；其餘 byte（包括 `0x80` 與 `0xff`）回 false。此 ONLY 不排除
`camel_break.c`，camel-break 狀態獨立；tokenization、embedding 與其餘 semantic runtime 仍由 C
負責。

已實際通過 Rust fmt、semantic token delimiter unit `2 passed` 與 clippy；
default/wrapper/direct FFI smoke 與 pipeline 各 `228 passed`，三路 production `--version`、
direct empty selector 與 `make -Bn` source-negative 均成功。完整 `scripts/test.sh` 尚未重跑。
true-source 進度由 **19/19 -> 20/20**。

### Pipeline registry test-QN classifier true-source

新增 `src/pipeline/registry_test_qn.c`／`.h`，公開 bridge 與 direct ABI 均為
`bool cbm_pipeline_registry_is_test_qn(const char *qn)`。`qn == NULL` 或空字串回 false；在
有效 NUL 結尾 C 字串的前提下，判定一律採 raw byte、非 UTF-8 safe 的大小寫敏感 `strstr`：
`Test`、`test`、`Mock`、`mock`、`Stub`、`stub`、`Fake`、`fake`、`Fixture`、`spec` 十個
needle 任一命中即回 true，因此 `contest` 也回 true。

`CBM_USE_RUST_PIPELINE_REGISTRY_TEST_QN=1` wrapper 保留既有 v1 ABI
`cbm_rs_registry_is_test_qn_v1`；僅接受 0/1，其他回傳值回退 C fallback。
**2026-07-17 true-source：** `CBM_USE_RUST_PIPELINE_REGISTRY_TEST_QN_ONLY=1`／Cargo
`pipeline-registry-test-qn-only` 由 Rust staticlib 匯出同名 public ABI；direct
`PIPELINE_REGISTRY_TEST_QN_SRCS =`，且 `make -Bn` source-negative 證明 production link 不含
`src/pipeline/registry_test_qn.c`。

本切片只將 registry candidate score 的 test-QN classifier 交接為 true source；registry handle、
cache、resolution、graph 與其餘 pipeline side effects 持續由 C 管理，並不代表整個 registry 已
direct 移轉。

已實際通過 Rust fmt、registry Rust unit `4 passed` 與 clippy；default/wrapper/direct FFI smoke
與 registry suite 各 `54 passed`，三路 production `--version`、direct empty selector 與
`make -Bn` source-negative 均成功。完整 `scripts/test.sh` 尚未重跑。true-source 進度由
**20/20 -> 21/21**。

### CLI archive byte helpers

新增 `CBM_USE_RUST_CLI_ARCHIVE_HELPERS=1`、`cli_archive::is_tar_end_of_archive()` 與 `cli_archive::zip_read_u16()`，由 `cbm_rs_cli_archive_tar_end_v1` 和 `cbm_rs_cli_archive_zip_read_u16_v1` 提供 stable ABI。只委派 tar 全零 block 判斷與 zip little-endian `uint16` 讀取；gzip 解壓、archive entry bounds、buffer ownership、檔案 I/O 與安裝流程仍由 C 負責。default path 保持 C。

同一 gate 也涵蓋 `cli_archive::zip_read_u32()` 與 `cbm_rs_cli_archive_zip_read_u32_v1`；仍只負責 little-endian byte decoding。

**2026-07-16 true-source：** 已拆 `src/cli/archive_helpers.c`／公開 bridge
`cbm_cli_is_tar_end_of_archive`、`cbm_cli_zip_read_u16`、`cbm_cli_zip_read_u32`；
`CLI_ARCHIVE_HELPERS_SRCS` 在 `_ONLY` 下為空；direct feature `cli-archive-helpers-only`；
cli suite 各 **113**、production `--version`、`make -Bn` source-negative 已通過。

### pipeline K8s text helpers

新增 `CBM_USE_RUST_PIPELINE_K8S_HELPERS=1` 與 `pipeline_k8s::{basename_offset,indent,split_kv}`，由三個 stable FFI 保留 K8s pass 的 basename、前導空格計數與單行 key/value 分割契約。YAML path stack、manifest scanning、file I/O、label matching、graph mutation 與 pipeline side effects 仍由 C 負責。default path 保持 C。

**2026-07-16 true-source：** 已拆 `src/pipeline/k8s_text_helpers.c`／公開 bridge
`cbm_pipeline_k8s_basename`、`cbm_pipeline_k8s_indent`、`cbm_pipeline_k8s_split_kv`；
`PIPELINE_K8S_TEXT_HELPERS_SRCS` 在 `_ONLY` 下為空；direct feature `pipeline-k8s-helpers-only`。

### pipeline incremental edge predicate

新增 `CBM_USE_RUST_PIPELINE_INCREMENTAL_EDGE=1` 與 `pipeline_incremental::edge_type_is_recomputed()`，由 stable FFI 判斷四種不應從 snapshot 恢復的 edge type。Incremental purge、graph traversal、registry seed、SQLite persistence 與 dispatch lifecycle 仍由 C 負責。default path 保持 C。

**2026-07-16 true-source：** 已拆 `src/pipeline/incremental_edge_type.c`／公開 bridge
`cbm_pipeline_incremental_edge_type_is_recomputed`；`PIPELINE_INCREMENTAL_EDGE_SRCS` 在
`_ONLY` 下為空；direct feature `pipeline-incremental-edge-only`；current-revision gate
pipeline **227**、production `--version`、`make -Bn` source-negative 已通過。

### pipeline parallel JSON escape helper

新增 `CBM_USE_RUST_PIPELINE_PARALLEL_JSON=1`，由 `pipeline_json` 共用 Rust helper 與 parallel-specific stable FFI `cbm_rs_pipeline_parallel_json_escape_char`／`cbm_rs_pipeline_parallel_json_escaped_len` 承接 JSON escape。2026-07-16 新增公開 C bridge `cbm_pipeline_parallel_json_escape_char`／`cbm_pipeline_parallel_json_escaped_len` 與 `pipeline-parallel-json-only` direct ABI。`value` 可為 NULL，escaped length 回傳 0；`output` 可為 NULL，或指向至少 `available` bytes 的 caller-owned buffer，NULL/容量不足時不寫入但仍回傳所需的 1 或 2 bytes。Rust 不保存 C 指標、不配置或釋放 C buffer。raw control byte 維持降級為空白。Atomic JSON append、definition/parallel data flow、graph mutation 與 pipeline side effects 仍由 C 負責，default path 保持 C。**2026-07-16 true-source：** `rust-pipeline-parallel-json-optin-test` 與 `rust-pipeline-parallel-json-only-test` 均通過；default/wrapper/direct FFI smoke 與 pipeline suite 各 `227 passed`，wrapper/direct production `--version` 成功，`_ONLY` source selector 為空，且 `make -Bn` source-negative 不含 `src/pipeline/parallel_json.c`。true-source 進度由 **12/18 -> 13/18**。

### discover language `.m` marker helpers

新增 `CBM_USE_RUST_DISCOVER_LANGUAGE_MARKERS=1` 與 `discover::language_marker()`，由 kind 0..3 stable ABI 承接 Objective-C、Magma end/callable 與 MATLAB line markers。`.m` 檔案讀取、marker 優先順序、language selection 與 fallback 仍由 C 負責。default path 保持 C。

**2026-07-16 true-source：** 已拆 `src/discover/language_markers.c`，公開 bridge 為
`cbm_discover_language_marker`；`DISCOVER_LANGUAGE_MARKERS_SRCS` 在 `_ONLY` 下為空，direct
feature 匯出同名 ABI。Rust 不保存 C 指標；kind 只接受既有 0..3 marker 種類。已實際通過
`rust-discover-language-markers-optin-test` 與 `rust-discover-language-markers-only-test` 的
default/wrapper/direct FFI smoke、discover `87 passed`、wrapper/direct production `--version`、
selector 空與 `make -Bn` production source-negative（不含
`src/discover/language_markers.c`）。true-source 進度由 **14/18 -> 15/18**；完整
`scripts/test.sh` 尚未重跑。

### pipeline incremental registry label predicate

新增 `CBM_USE_RUST_PIPELINE_INCREMENTAL_REGISTRY_LABEL=1` 與 `pipeline_incremental::label_is_registry_symbol()`，承接 registry seed 的 Function/Method、六種 type-like、Variable/Field label 判斷。graph traversal、registry mutation、SQLite persistence 與 incremental lifecycle 仍由 C 負責。default path 保持 C。

**2026-07-16 true-source：** 已拆 `src/pipeline/incremental_registry_label.c`／公開 bridge
`cbm_pipeline_incremental_label_is_registry_symbol`；`PIPELINE_INCREMENTAL_REGISTRY_LABEL_SRCS`
在 `_ONLY` 下為空；direct feature `pipeline-incremental-registry-label-only`；pipeline **227**、
production `--version`、`make -Bn` source-negative 已通過。

### pipeline semantic Go suffix predicate

新增 `CBM_USE_RUST_PIPELINE_SEMANTIC_FP_SUFFIX=1` 與 `cbm_rs_pipeline_semantic_fp_ends_with_v1`，重用 Rust `discover::ends_with` 承接 Go interface file suffix 判斷。interface matching、graph traversal、edge 建立與 semantic pass side effects 仍由 C 負責。default path 保持 C。

**2026-07-16 true-source：** 已拆 `src/pipeline/semantic_fp_suffix.c`／公開 bridge
`cbm_pipeline_semantic_fp_ends_with`；`PIPELINE_SEMANTIC_FP_SUFFIX_SRCS` 在 `_ONLY` 下為空；
direct feature `pipeline-semantic-fp-suffix-only`；pipeline **227**、production `--version`、
`make -Bn` source-negative 已通過。

### store architecture JSON property extractor

新增 `CBM_USE_RUST_STORE_ARCH_JSON_PROP=1` 與 `store::arch_helpers::extract_json_string_prop()`，透過 length/copy stable ABI 讓 Rust 執行 bounded JSON property locate；C 仍負責 malloc/free 與 architecture result ownership。SQLite query、architecture aggregation 與 store runtime 保持 C。default path 保持 C；本切片尚未驗證。

## Discover path join / incremental edge / semantic fp / json prop / registry label / mcp edge type（2026-07-16）

| Bridge | Stable ABI | ONLY feature | Fallback CU |
|--------|------------|--------------|-------------|
| `cbm_discover_path_join` | `cbm_rs_discover_path_join_v1` | `discover-path-join-only` | `src/discover/path_join.c` |
| `cbm_pipeline_incremental_edge_type_is_recomputed` | `cbm_rs_pipeline_incremental_edge_type_is_recomputed_v1` | `pipeline-incremental-edge-only` | `src/pipeline/incremental_edge_type.c` |
| `cbm_pipeline_semantic_fp_ends_with` | `cbm_rs_pipeline_semantic_fp_ends_with_v1` | `pipeline-semantic-fp-suffix-only` | `src/pipeline/semantic_fp_suffix.c` |
| `cbm_pipeline_extract_json_prop` | `cbm_rs_pipeline_extract_json_prop_v1` | `pipeline-json-prop-only` | `src/pipeline/json_prop.c` |
| `cbm_pipeline_incremental_label_is_registry_symbol` | `cbm_rs_pipeline_incremental_label_is_registry_symbol_v1` | `pipeline-incremental-registry-label-only` | `src/pipeline/incremental_registry_label.c` |
| `cbm_mcp_edge_type_valid` | `cbm_rs_mcp_edge_type_valid_v1` | `mcp-edge-type-only` | `src/mcp/edge_type_valid.c` |
| `cbm_is_trackable_file` | `cbm_rs_pipeline_is_trackable_file_v1` | `pipeline-githistory-only` | `src/pipeline/trackable_file.c` |
| `cbm_cli_is_tar_end_of_archive` / `cbm_cli_zip_read_u{16,32}` | `cbm_rs_cli_archive_*_v1` | `cli-archive-helpers-only` | `src/cli/archive_helpers.c` |
| `cbm_pipeline_k8s_basename` / `_indent` / `_split_kv` | `cbm_rs_pipeline_k8s_*_v1` | `pipeline-k8s-helpers-only` | `src/pipeline/k8s_text_helpers.c` |
| `cbm_minhash_jaccard` / `to_hex` / `from_hex` | `cbm_rs_simhash_*_v1` | `simhash-jaccard-only` | `src/simhash/minhash_jaccard.c` |
| `cbm_pipeline_envscan_*` | `cbm_rs_pipeline_envscan_*_v1` | `pipeline-envscan-classifiers-only` | `src/pipeline/envscan_classifiers.c` |
| `cbm_pipeline_configlink_*` | `cbm_rs_pipeline_configlink_*_v1` | `pipeline-configlink-helpers-only` | `src/pipeline/configlink_helpers.c` |

全部為 pure helper；Rust 不保存 C 指標。ONLY 時排除 fallback `.c`。`MCP_CODEC` wrapper 仍可
委派 edge-type Rust ABI；edge-type ONLY 只排除小 CU，不排除整個 `mcp.c`。`pass_githistory.c`、
`cli.c`、`pass_k8s.c`、`pass_envscan.c`、`pass_configlink.c`、`minhash.c`（compute/LSH）本體仍
連結（orchestration 未遷移）。
> **2026-07-17 第 22 項 ABI 註記（原 `21/21` 預算外後續 slice）**：新增 discover language-name
> public bridge `const char *cbm_language_name(CBMLanguage)`。其回傳值是 static、borrowed、NUL
> 結尾、immutable 字串，C 呼叫端不得保存為可釋放所有權或呼叫 `free`；invalid、
> `CBM_LANG_COUNT`、out-of-range 與未來 enum hole 必須回 `"Unknown"`。CFSCRIPT（157）與
> CFML（158）皆回 `"CFML"`。C fallback 位於 `src/discover/language_name.c/.h`，`language.c`
> 保留偵測。wrapper `CBM_USE_RUST_DISCOVER_LANGUAGE_NAME=1` 呼叫 v1
> `cbm_rs_discover_language_name_v1(int)`，僅 Rust 回 `NULL` 才 fallback C。direct
> `CBM_USE_RUST_DISCOVER_LANGUAGE_NAME_ONLY=1`／Cargo `discover-language-name-only` 由 Rust
> staticlib 匯出同一 public ABI，`DISCOVER_LANGUAGE_NAME_SRCS =` 且 production source-negative
> 排除 fallback；此 direct 與舊 full-language direct 因同名 ABI 互斥。已通過 fmt、3 個 unit、
> clippy、default/wrapper/direct FFI、discover 87、production `--version`、selector/source-negative
> 與 userconfig combo gate；完整 `scripts/test.sh` 未重跑。
