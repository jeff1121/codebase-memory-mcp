# Rust 重構計畫

## 目前停點（2026-07-19）

- true-source：**48 / 48**；產品 default 仍 C11。
- 不得重跑 #29–#53；complexity JSON 延後（strtol）。
- #54 architecture_aspect_wanted 為**進行中且非 true-source**：局部驗證已通過，但 optin/ONLY full gate 尚無完整成功證據。
- dirty worktree 為預期（#29–#53 未 commit、#54 WIP）；禁止 reset／clean。
- 完整細節、工作樹地圖與起手命令：`docs/rust-refactor-current-handoff.md` 頂端。
- Skill：`CodebaseMemMcp-Refactory-rust`。
- #37 enrichment tokens 已 true-source：ASCII byte lowercase、OOM 非 parity、C malloc／caller free；
  `rust-pipeline-enrichment-tokens-{optin,only}-test` 的 default/wrapper/direct pipeline 均 **233**。
- #38 pkgmap text 已 true-source：6 個 bounded/path helper、borrowed source、C malloc path result；
  `rust-pipeline-pkgmap-text-{optin,only}-test` 的 default/wrapper/direct pipeline 均 **235**。
- #39 search_code_score 已 true-source：nullable C strings、first-NUL 與累加加權；
  `rust-mcp-search-code-score-{optin,only}-test` 的 default/wrapper/direct mcp 均 **137**。
- #40 search_top_dir 已 true-source：NULL file、length-only、短 buffer 與 first-NUL；
  `rust-mcp-search-top-dir-{optin,only}-test` 的 default/wrapper/direct mcp 均 **138**。
- #41 detect_changes_wants_symbols 已 true-source：NULL、accepted/rejected scope 與 first-NUL；
  `rust-mcp-detect-changes-scope-{optin,only}-test` 的 default/wrapper/direct mcp 均 **139**。
- #42 detect_changes_impacted_label 已 true-source：NULL、excluded label 與 first-NUL；
  `rust-mcp-detect-changes-label-{optin,only}-test` 的 default/wrapper/direct mcp 均 **140**。
- #43 detect_changes_status_path_offset 已 true-source：NULL、porcelain、rename 與 first-NUL；
  `rust-mcp-detect-changes-status-{optin,only}-test` 的 default/wrapper/direct mcp 均 **141**。
- #44 node_resolution_score 已 true-source：nullable label、tier、negative span 與 first-NUL；
  `rust-mcp-node-resolution-score-{optin,only}-test` 的 default/wrapper/direct mcp 均 **142**。
- #45 utf8_is_cont_byte 已 true-source：low 8-bit continuation 判斷、負數／高位 mask；
  `rust-mcp-utf8-is-cont-{optin,only}-test` 的 default/wrapper/direct mcp 均 **143**。
- #46 adr_mode 已 true-source：get/update-store/sections mapping 與 first-NUL；
  `rust-mcp-adr-mode-{optin,only}-test` 的 default/wrapper/direct mcp 均 **144**。
- #47 trace_mode_edge_mask 已 true-source：default mode bitmask 與 first-NUL；
  `rust-mcp-trace-mode-edge-mask-{optin,only}-test` 的 default/wrapper/direct mcp 均 **145**。
- #48 trace_is_test_file 已 true-source：test-file substring 與 first-NUL；
  `rust-mcp-trace-is-test-file-{optin,only}-test` 的 default/wrapper/direct mcp 均 **146**。
- #49 project_db_file_name 已 true-source：精準 `.db` suffix、hidden／memory 排除與 first-NUL；
  `rust-mcp-project-db-file-name-{optin,only}-test` 的 default/wrapper/direct mcp 均 **147**。
- #50 sanitize_ascii_in_place 已 true-source：in-place high-byte replacement、NULL no-op 與 first-NUL；
  `rust-mcp-sanitize-ascii-in-place-{optin,only}-test` 的 default/wrapper/direct mcp 均 **148**。
- #51 bm25_file_pattern_like 已 true-source：length-only／短 buffer、glob-to-LIKE、first-NUL 與 caller-owned
  output buffer；`rust-mcp-bm25-file-pattern-like-{optin,only}-test` 的 default/wrapper/direct mcp 均 **149**。
- #52 bm25_build_match 已 true-source：ASCII token、短 buffer、first-NUL 與 caller-owned output buffer；
  `rust-mcp-bm25-build-match-{optin,only}-test` 的 default/wrapper/direct mcp 均 **150**。
- #53 sanitize_utf8_lossy 已 true-source：invalid byte 替換、first-NUL 與 C malloc／caller free；
  `rust-mcp-sanitize-utf8-lossy-{optin,only}-test` 的 default/wrapper/direct mcp 均 **151**。
- 下一步：先以 `make -j1` 依序重跑 #54 的 optin 與 ONLY gate，取得完整證據；完成前不得另選切片。

## 第 54 項：MCP architecture_aspect_wanted（進行中，2026-07-19）

public ABI `bool cbm_mcp_architecture_aspect_wanted(const char *input, const char *name)` 已由 C fallback
`src/mcp/architecture_aspect_wanted.c/.h` 提供：invalid／missing／nonarray aspects 為 wanted，empty／no-match 為
not wanted，`"all"`／exact name 為 wanted，first-NUL 生效。wrapper
`CBM_USE_RUST_MCP_ARCHITECTURE_ASPECT_WANTED`（相容 CODEC）與 direct
`CBM_USE_RUST_MCP_ARCHITECTURE_ASPECT_WANTED_ONLY`／`mcp-architecture-aspect-wanted-only` 已接線。fmt、Rust unit
**1 passed**、切片 clippy、direct selector 已通過；optin 首次因 `test-rust-ffi` 未連結 `yyjson_read_opts` 失敗，
現已把 `$(YYJSON_SRC)` 補入 `RUST_FFI_SUPPORT_SRCS`。重啟 gate 的結果未保存，ONLY 未跑；故仍是 WIP，不能從
48／48 遞增。

## 第 53 項：MCP sanitize_utf8_lossy true-source（2026-07-19）

總計由 **47 / 47 → 48 / 48**。C fallback 拆至 `src/mcp/sanitize_utf8_lossy.c/.h`，`mcp.c` 保留 source
resolution、yyjson result shaping 與 transport。public ABI `cbm_mcp_sanitize_utf8_lossy(input)`：NULL 回 NULL；
first-NUL 前保留有效 UTF-8，將每個無效 byte 替換為 U+FFFD；成功回傳 C `malloc` 字串，caller 必須 `free()`，
無 I/O。

wrapper `CBM_USE_RUST_MCP_SANITIZE_UTF8_LOSSY` 相容 CODEC；direct
`CBM_USE_RUST_MCP_SANITIZE_UTF8_LOSSY_ONLY`／`mcp-sanitize-utf8-lossy-only` 由 Rust 呼叫 C `malloc` 後匯出同名
ABI。已通過 Rust unit **1 passed**、fmt、切片 clippy，以及 optin/only gate 的 default/wrapper/direct FFI、MCP
**151 passed**、production `--version`、empty selector 與 source-negative（保留 `mcp.c`）。完整
`scripts/test.sh` 未跑。

## 第 52 項：MCP bm25_build_match true-source（2026-07-19）

總計由 **46 / 46 → 47 / 47**。C fallback 拆至 `src/mcp/bm25_build_match.c/.h`，`mcp.c` 保留 FTS5 SQLite
query、ranking／boosting、result JSON 與 transport orchestration。public ABI
`cbm_mcp_bm25_build_match(input, buf, bufsize)` 維持 input-first C 順序：NULL input／buffer 或 `bufsize < 2`
回 0 且不寫入；僅保留 ASCII alnum／underscore token，以 ` OR ` 串接；短 buffer 停在上一個完整 token 並
NUL-terminate，first-NUL 生效。輸出 buffer 由 caller 擁有，無 heap、無 I/O。

wrapper `CBM_USE_RUST_MCP_BM25_BUILD_MATCH` 相容 CODEC；direct
`CBM_USE_RUST_MCP_BM25_BUILD_MATCH_ONLY`／`mcp-bm25-build-match-only` 匯出同名 ABI。已通過 Rust unit
**1 passed**、fmt、切片 clippy，以及 optin/only gate 的 default/wrapper/direct FFI、MCP **150 passed**、
production `--version`、empty selector 與 source-negative（保留 `mcp.c`）。完整 `scripts/test.sh` 未跑。

## 第 51 項：MCP bm25_file_pattern_like true-source（2026-07-19）

總計由 **45 / 45 → 46 / 46**。C fallback 拆至 `src/mcp/bm25_file_pattern_like.c/.h`，`mcp.c` 僅保留
caller-owned final buffer 的配置／釋放與 BM25 SQLite query orchestration。public ABI
`cbm_mcp_bm25_file_pattern_like(buf, bufsize, input)`：NULL input 回 `SIZE_MAX`；成功回完整輸出長度；
NULL／zero buffer 為 length-only，短 buffer NUL 截斷。輸入採 first-NUL，先用 Store glob-to-LIKE，再對無
wildcard input 加前後 `%`。C fallback 暫時配置並釋放 glob 結果，public output buffer 仍由 caller 擁有。

wrapper `CBM_USE_RUST_MCP_BM25_FILE_PATTERN_LIKE` 相容 CODEC；direct
`CBM_USE_RUST_MCP_BM25_FILE_PATTERN_LIKE_ONLY`／`mcp-bm25-file-pattern-like-only` 匯出同名 ABI。已通過
Rust unit **1 passed**、fmt、切片 clippy，以及 optin/only gate 的 default/wrapper/direct FFI、MCP
**149 passed**、production `--version`、empty selector 與 source-negative（保留 `mcp.c`）。完整
`scripts/test.sh` 未跑。

## 第 50 項：MCP sanitize_ascii_in_place true-source（2026-07-19）

總計由 **44 / 44 → 45 / 45**。C fallback 拆至 `src/mcp/sanitize_ascii_in_place.c/.h`，`mcp.c` 保留
`search_code` 的 grep/source/context read、JSON result shaping、snippet UTF-8 lossy sanitizer 與 transport
orchestration。public ABI `cbm_mcp_sanitize_ascii_in_place(input)`：NULL no-op；只掃描至 first-NUL，將每個
大於 127 的 byte 原地改為 `?`，ASCII（含 `0x7f`）保留，NUL 後 byte 不改；無 heap、無 I/O。

wrapper 為 `CBM_USE_RUST_MCP_SANITIZE_ASCII_IN_PLACE`（相容 CODEC），direct 為
`CBM_USE_RUST_MCP_SANITIZE_ASCII_IN_PLACE_ONLY`／Cargo `mcp-sanitize-ascii-in-place-only`；direct
`MCP_SANITIZE_ASCII_IN_PLACE_SRCS =`，排除 fallback 並保留 `mcp.c`。Rust unit **1 passed**、fmt、切片 clippy、
optin/only default/wrapper/direct FFI、mcp **148 passed**、production `--version`、empty selector 與
source-negative 均通過。完整 `scripts/test.sh` 未跑。

## 第 49 項：MCP project_db_file_name true-source（2026-07-19）

總計由 **43 / 43 → 44 / 44**。C fallback 拆至 `src/mcp/project_db_file_name.c/.h`，`mcp.c` 保留 cache
directory scan、SQLite query-open、internal project-name resolution、ghost/corrupt DB filtering、list JSON、
resolve fallback 與 transport orchestration。public ABI `cbm_mcp_project_db_file_name(name)`：NULL、empty、`.db`、
non-`.db`、`_` 開頭與 `:memory:` 開頭為 false；至少 `x.db`、精準 `.db` suffix 才可能為 true，`tmp-*.db`
保持合法；字串採 first-NUL，無 heap、無輸入改寫、無 I/O。

wrapper 為 `CBM_USE_RUST_MCP_PROJECT_DB_FILE_NAME`（相容 CODEC），direct 為
`CBM_USE_RUST_MCP_PROJECT_DB_FILE_NAME_ONLY`／Cargo `mcp-project-db-file-name-only`；direct
`MCP_PROJECT_DB_FILE_NAME_SRCS =`，排除 fallback 並保留 `mcp.c`。Rust unit **1 passed**、fmt、切片 clippy、
optin/only default/wrapper/direct FFI、mcp **147 passed**、production `--version`、empty selector 與
source-negative 均通過。完整 `scripts/test.sh` 未跑。

## 第 48 項：MCP trace_is_test_file true-source（2026-07-19）

總計由 **42 / 42 → 43 / 43**。C fallback 拆至 `src/mcp/trace_is_test_file.c/.h`，`mcp.c` 保留
`trace_call_path` 的 BFS、include_tests filtering、is_test JSON 標記、risk labels、data-flow args、JSON 與
transport orchestration。public ABI `cbm_mcp_trace_is_test_file(path)`：NULL、empty 回 false；path 含
`/test`、`test_`、`_test.`、`/tests/`、`/spec/` 或 `.test.` 回 true；字串採 first-NUL，無 heap、無輸入改寫、
無 I/O。

wrapper 為 `CBM_USE_RUST_MCP_TRACE_IS_TEST_FILE`（相容 CODEC），direct 為
`CBM_USE_RUST_MCP_TRACE_IS_TEST_FILE_ONLY`／Cargo `mcp-trace-is-test-file-only`；direct
`MCP_TRACE_IS_TEST_FILE_SRCS =`，排除 fallback 並保留 `mcp.c`。Rust unit **1 passed**、fmt、切片 clippy、
optin/only default/wrapper/direct FFI、mcp **146 passed**、production `--version`、empty selector 與
source-negative 均通過。完整 `scripts/test.sh` 未跑。

## 第 47 項：MCP trace_mode_edge_mask true-source（2026-07-19）

總計由 **41 / 41 → 42 / 42**。C fallback 拆至 `src/mcp/trace_mode_edge_mask.c/.h`，`mcp.c` 保留
`resolve_trace_edge_types()` 的 explicit edge array parsing、edge-name 順序、BFS、JSON 與 transport
orchestration。public ABI `cbm_mcp_trace_mode_edge_mask(input)`：NULL、empty、`calls`、unknown、大小寫不符與
尾端空白回 CALLS bit；`data_flow` 回 `CALLS|DATA_FLOWS`；`cross_service` 回完整 bit 0..9 edge set（`0x3ff`）；
字串採 first-NUL，無 heap、無輸入改寫、無 I/O。

wrapper 為 `CBM_USE_RUST_MCP_TRACE_MODE_EDGE_MASK`（相容 CODEC），direct 為
`CBM_USE_RUST_MCP_TRACE_MODE_EDGE_MASK_ONLY`／Cargo `mcp-trace-mode-edge-mask-only`；direct
`MCP_TRACE_MODE_EDGE_MASK_SRCS =`，排除 fallback 並保留 `mcp.c`。Rust unit **1 passed**、fmt、切片 clippy、
optin/only default/wrapper/direct FFI、mcp **145 passed**、production `--version`、empty selector 與
source-negative 均通過。完整 `scripts/test.sh` 未跑。

## 第 46 項：MCP adr_mode true-source（2026-07-19）

總計由 **40 / 40 → 41 / 41**。C fallback 拆至 `src/mcp/adr_mode.c/.h`，`mcp.c` 保留
`manage_adr` 的 project／Store resolution、legacy migration、ADR read/write、sections JSON、ownership 與
transport orchestration。public ABI `cbm_mcp_adr_mode(input)`：NULL、empty、`get`、unknown、大小寫不符與
尾端空白回 0；`update`／`store` 回 1；`sections` 回 2；字串採 first-NUL，無 heap、無輸入改寫、無 I/O。

wrapper 為 `CBM_USE_RUST_MCP_ADR_MODE`（相容 CODEC），direct 為
`CBM_USE_RUST_MCP_ADR_MODE_ONLY`／Cargo `mcp-adr-mode-only`；direct `MCP_ADR_MODE_SRCS =`，排除 fallback
並保留 `mcp.c`。Rust unit **1 passed**、fmt、切片 clippy、optin/only default/wrapper/direct FFI、mcp
**144 passed**、production `--version`、empty selector 與 source-negative 均通過。完整
`scripts/test.sh` 未跑。

## 第 45 項：MCP utf8_is_cont_byte true-source（2026-07-19）

總計由 **39 / 39 → 40 / 40**。C fallback 拆至 `src/mcp/utf8_is_cont.c/.h`，
`sanitize_utf8_lossy.c` 保留 scan、replacement-byte 寫入與 buffer ownership，`mcp.c` 保留 JSON 與 transport。
public ABI `cbm_mcp_utf8_is_cont_byte(byte)`：只檢查 low 8-bit，`0x80..0xBF` 回 true，其他回 false；負數與
超過 byte 範圍的輸入先 byte mask；無 heap、無輸入改寫、無 I/O。

wrapper 為 `CBM_USE_RUST_MCP_UTF8_IS_CONT`（相容 CODEC），direct 為
`CBM_USE_RUST_MCP_UTF8_IS_CONT_ONLY`／Cargo `mcp-utf8-is-cont-only`；direct
`MCP_UTF8_IS_CONT_SRCS =`，排除 fallback 並保留 `mcp.c`。Rust unit **1 passed**、fmt、切片 clippy、
optin/only default/wrapper/direct FFI、mcp **143 passed**、production `--version`、empty selector 與
source-negative 均通過。完整 `scripts/test.sh` 未跑。

## 第 44 項：MCP node_resolution_score true-source（2026-07-19）

總計由 **38 / 38 → 39 / 39**。C fallback 拆至 `src/mcp/node_resolution_score.c/.h`，`mcp.c` 保留
candidate query、ambiguity、BFS、JSON 與 transport orchestration。public ABI
`cbm_mcp_node_resolution_score(label, start_line, end_line)`：Function／Method tier 2，Module／File／NULL
tier 0，其他 label tier 1；回傳 tier * 1000000 加非負 line span；字串採 first-NUL，無 heap、無輸入改寫、
無 I/O。

wrapper 為 `CBM_USE_RUST_MCP_NODE_RESOLUTION_SCORE`（相容 CODEC），direct 為
`CBM_USE_RUST_MCP_NODE_RESOLUTION_SCORE_ONLY`／Cargo `mcp-node-resolution-score-only`；direct
`MCP_NODE_RESOLUTION_SCORE_SRCS =`，排除 fallback 並保留 `mcp.c`。Rust unit **1 passed**、fmt、切片
clippy、optin/only default/wrapper/direct FFI、mcp **142 passed**、production `--version`、empty selector
與 source-negative 均通過。完整 `scripts/test.sh` 未跑。

## 第 43 項：MCP detect_changes_status_path_offset true-source（2026-07-19）

總計由 **37 / 37 → 38 / 38**。C fallback 拆至 `src/mcp/detect_changes_status.c/.h`，`mcp.c` 保留
changed-files loop、Store lookup、JSON 與 transport orchestration。public ABI
`cbm_mcp_detect_changes_status_path_offset(line)`：NULL、empty 或最終 path empty 回 `SIZE_MAX`；porcelain
`XY ` 跳 3 bytes，rename `old -> new` 取 destination offset，bare／unknown prefix 回 0；字串採 first-NUL，
無 heap、無輸入改寫、無 I/O。

wrapper 為 `CBM_USE_RUST_MCP_DETECT_CHANGES_STATUS`（相容 CODEC），direct 為
`CBM_USE_RUST_MCP_DETECT_CHANGES_STATUS_ONLY`／Cargo `mcp-detect-changes-status-only`；direct
`MCP_DETECT_CHANGES_STATUS_SRCS =`，排除 fallback 並保留 `mcp.c`。Rust unit **1 passed**、fmt、切片
clippy、optin/only default/wrapper/direct FFI、mcp **141 passed**、production `--version`、empty selector
與 source-negative 均通過。完整 `scripts/test.sh` 未跑。

## 第 42 項：MCP detect_changes_impacted_label true-source（2026-07-19）

總計由 **36 / 36 → 37 / 37**。C fallback 拆至 `src/mcp/detect_changes_label.c/.h`，`mcp.c` 保留
detect_changes 的 Store lookup、node ownership、JSON 與 transport orchestration。public ABI
`cbm_mcp_detect_changes_impacted_label(label)`：NULL、`File`、`Folder`、`Project` 回 false；空字串與
其他值回 true；字串採 first-NUL，無 heap、無輸入改寫、無 I/O。

wrapper 為 `CBM_USE_RUST_MCP_DETECT_CHANGES_LABEL`（相容 CODEC），direct 為
`CBM_USE_RUST_MCP_DETECT_CHANGES_LABEL_ONLY`／Cargo `mcp-detect-changes-label-only`；direct
`MCP_DETECT_CHANGES_LABEL_SRCS =`，排除 fallback 並保留 `mcp.c`。Rust unit **1 passed**、fmt、
切片 clippy、optin/only default/wrapper/direct FFI、mcp **140 passed**、production `--version`、
empty selector 與 source-negative 均通過。完整 `scripts/test.sh` 未跑。

## 第 41 項：MCP detect_changes_wants_symbols true-source（2026-07-19）

總計由 **35 / 35 → 36 / 36**。C fallback 拆至 `src/mcp/detect_changes_scope.c/.h`，`mcp.c` 保留
detect_changes 的 project/root resolution、git、Store、JSON 與 transport orchestration。public ABI
`cbm_mcp_detect_changes_wants_symbols(scope)`：NULL、`symbols`、`impact` 回 true；空字串、`files`、
大小寫不符、尾端空白與其他值回 false；字串採 first-NUL，無 heap、無輸入改寫、無 I/O。

wrapper 為 `CBM_USE_RUST_MCP_DETECT_CHANGES_SCOPE`（相容 CODEC），direct 為
`CBM_USE_RUST_MCP_DETECT_CHANGES_SCOPE_ONLY`／Cargo `mcp-detect-changes-scope-only`；direct
`MCP_DETECT_CHANGES_SCOPE_SRCS =`，排除 fallback 並保留 `mcp.c`。Rust unit **1 passed**、fmt、
切片 clippy、optin/only default/wrapper/direct FFI、mcp **139 passed**、production `--version`、
empty selector 與 source-negative 均通過。完整 `scripts/test.sh` 未跑。

## 第 40 項：MCP search_top_dir true-source（2026-07-19）

總計由 **34 / 34 → 35 / 35**。C fallback 拆至 `src/mcp/search_top_dir.c/.h`，`mcp.c` 保留
directory distribution orchestration。public ABI `cbm_mcp_search_top_dir(buf, bufsize, file)`：NULL file
回 `SIZE_MAX` 且不寫入，成功回完整 top key 長度；NULL／zero buffer length-only，短 buffer
NUL-terminate，字串採 first-NUL；無 heap、無輸入改寫、無 I/O。

wrapper 為 `CBM_USE_RUST_MCP_SEARCH_TOP_DIR`（相容 CODEC），direct 為
`CBM_USE_RUST_MCP_SEARCH_TOP_DIR_ONLY`／Cargo `mcp-search-top-dir-only`；direct
`MCP_SEARCH_TOP_DIR_SRCS =`，排除 fallback 並保留 `mcp.c`。Rust unit **1 passed**、fmt、切片
clippy、optin/only default/wrapper/direct FFI、mcp **138 passed**、production `--version`、empty selector
與 source-negative 均通過。完整 `scripts/test.sh` 未跑。

## 第 39 項：MCP search_code_score true-source（2026-07-19）

總計由 **33 / 33 → 34 / 34**。C fallback 拆至 `src/mcp/search_code_score.c/.h`，`mcp.c` 保留
search result orchestration。public ABI `cbm_mcp_search_code_score(label, file, in_degree)`：nullable
字串只讀至 first-NUL，Function／Method +10、Route +15、vendored/vendor/node_modules -50、
test/spec/_test. -5，條件可累加；無 heap、無輸入改寫、無排序或 I/O。

wrapper 為 `CBM_USE_RUST_MCP_SEARCH_CODE_SCORE`（相容 CODEC），direct 為
`CBM_USE_RUST_MCP_SEARCH_CODE_SCORE_ONLY`／Cargo `mcp-search-code-score-only`；direct
`MCP_SEARCH_CODE_SCORE_SRCS =`，排除 fallback 並保留 `mcp.c`。Rust unit **1 passed**、fmt、
切片 clippy、optin/only default/wrapper/direct FFI、mcp **137 passed**、production `--version`、
empty selector 與 source-negative 均通過。完整 `scripts/test.sh` 未跑。

## 第 38 項：Pipeline pkgmap text true-source（2026-07-19）

總計由 **32 / 32 → 33 / 33**。C fallback 拆至 `src/pipeline/pkgmap_text.c/.h`，
`pass_pkgmap.c` 保留 manifest parsing、I/O、package entry ownership 與 graph orchestration。public ABI
為 6 個 `cbm_pipeline_pkgmap_*` helper；bounded line value 成功時借用 source，四個 path helper 成功時
由 C malloc 配置並由 caller free。NULL、first-NUL、bounded byte、1023-byte join/build truncate 均已固定。

wrapper 為 `CBM_USE_RUST_PIPELINE_PKGMAP_TEXT`，direct 為
`CBM_USE_RUST_PIPELINE_PKGMAP_TEXT_ONLY`／Cargo `pipeline-pkgmap-text-only`；direct
`PIPELINE_PKGMAP_TEXT_SRCS =`，排除 fallback 且保留 `pass_pkgmap.c`。Rust unit **4 passed**、fmt、
切片 clippy、optin/only default/wrapper/direct FFI、pipeline **235 passed**、production `--version`、
empty selector 與 source-negative 均通過。完整 `scripts/test.sh` 未跑。

## 第 37 項：Pipeline enrichment tokens true-source（2026-07-19）

總計由 **31 / 31 → 32 / 32**。C fallback 拆至 `src/pipeline/enrichment_tokens.c/.h`，
`pass_enrichment.c` 保留 graph/store orchestration。公開 ABI 是 `cbm_split_camel_case` 與
`cbm_tokenize_decorator`；NULL／NULL out／非正 `max_out` 回 0 且不寫入，input first-NUL raw bytes。
decorator 僅 ASCII lowercase，非 ASCII byte 原樣；正常資源 token 為 C malloc／caller free，OOM 非
parity 承諾。wrapper 為 `CBM_USE_RUST_PIPELINE_ENRICHMENT_TOKENS`，direct 為
`CBM_USE_RUST_PIPELINE_ENRICHMENT_TOKENS_ONLY`／Cargo `pipeline-enrichment-tokens-only`；direct
`PIPELINE_ENRICHMENT_TOKENS_SRCS =` 並排除 fallback、保留 `pass_enrichment.c`。

`cargo test -p cbm-core pipeline_enrichment --locked` **5 passed**；fmt、切片 clippy、optin/only
default/wrapper/direct FFI、pipeline **233 passed**、production `--version`、empty selector 與
source-negative 均通過。完整 `scripts/test.sh` 未跑。

## 第 35–36 項：MCP args + score_cmp true-source（2026-07-18）

新 true-source 兩項，累計 **31 / 31**。#29–#34 未重開；complexity 仍延後。

**#35** `cbm_mcp_search_args_valid`：root + optional pattern combiner；fallback
`src/mcp/search_args.c`；feature `mcp-search-args-only`；Make
`rust-mcp-search-args-{optin,only}-test`；mcp **136**。

**#36** `cbm_mcp_search_score_cmp`：descending score delta（`right - left`）；fallback
`src/mcp/search_score_cmp.c`；feature `mcp-search-score-cmp-only`；Make
`rust-mcp-search-score-cmp-{optin,only}-test`；mcp **136**。

## 第 33–34 項：MCP path_arg + line_span true-source（2026-07-18）

新 true-source 兩項，累計 **29 / 29**。#29–#32 未重開；complexity 仍延後。

**#33** `cbm_mcp_search_path_arg_valid`：shell denylist pure classifier；fallback
`search_path_arg.c`；feature `mcp-search-path-arg-only`；mcp **136**。

**#34** `cbm_mcp_search_line_match_span`：三 int span 純計算；fallback
`search_line_span.c`；feature `mcp-search-line-span-only`；mcp **136**。

## 第 32 項：MCP index_mode true-source（2026-07-18）

新 true-source：**27 / 27**。產品 default 仍 C11。#29–#31 未重開；complexity JSON 仍延後。

C fallback `src/mcp/index_mode.c/.h`；公開 `int cbm_mcp_index_mode(const char *)`：
moderate→1、fast→2、cross-repo-intelligence→3、其餘（含 NULL/full/未知）→0。
wrapper `CBM_USE_RUST_MCP_INDEX_MODE`（相容 CODEC）；direct `…_ONLY`／`mcp-index-mode-only`。
Make `rust-mcp-index-mode-{optin,only}-test`：mcp 各 **136 passed**、production `--version`；
direct 空 `MCP_INDEX_MODE_SRCS`、`make -Bn` 不含 `index_mode.c`、仍含 `mcp.c`。
handler／pipeline 仍在 C。fmt／unit／clippy 通過。

## 第 30–31 項：usages-json + MCP search_mode true-source（2026-07-17）

新 true-source 兩項，累計 **26 / 26**。產品 default 仍 C11。#29 未重開；complexity JSON
仍延後（strtol locale／overflow／NULL key）。

### 第 30 項 pipeline-usages-json

C fallback `src/pipeline/usages_json.c/.h`；公開
`char *cbm_pipeline_usages_extract_local_name(const char *)`（malloc／caller free）。
NULL／缺鍵／空 value → NULL；raw-byte first-NUL，無 JSON unescape。wrapper
`CBM_USE_RUST_PIPELINE_USAGES_JSON` 經既有 len/copy v1；direct `…_ONLY`／
`pipeline-usages-json-only`。Make `rust-pipeline-usages-json-{optin,only}-test`：pipeline 各
**231 passed**、production `--version`；direct 空 `PIPELINE_USAGES_JSON_SRCS`、`make -Bn`
不含 `usages_json.c`、仍含 `pass_usages.c`。fmt／unit 1／clippy 通過。

### 第 31 項 MCP search_mode

C fallback `src/mcp/search_mode.c/.h`；公開 `int cbm_mcp_search_mode(const char *)`：
full→1、files→2、其餘→0。wrapper `CBM_USE_RUST_MCP_SEARCH_MODE`（相容 CODEC）；direct
`…_ONLY`／`mcp-search-mode-only`。Make `rust-mcp-search-mode-{optin,only}-test`：mcp 各
**136 passed**、production `--version`；direct 空 `MCP_SEARCH_MODE_SRCS`、`make -Bn` 不含
`search_mode.c`、仍含 `mcp.c`。handler／grep／response 仍在 C。fmt／unit／clippy 通過。

## 第 29 項：Pipeline calls-json true-source（2026-07-17）

新 true-source：**24 / 24**。C fallback `src/pipeline/calls_json.c/.h`；公開
`char *cbm_pipeline_calls_extract_local_name(const char *)`（malloc／caller free）。
wrapper `CBM_USE_RUST_PIPELINE_CALLS_JSON`；direct `…_ONLY`／`pipeline-calls-json-only`。
pipeline 各 **231 passed**；direct empty selector + source-negative；`pass_calls` 仍連結。
fmt／unit 1／clippy 通過。產品 default 仍 C11。
## 第 28 項：Pipeline split-command true-source（2026-07-17）

pipeline-split-command 是新的 true-source replacement，總計由 **22 / 22** 推進至
**23 / 23**。只遷移 command tokenizer：C fallback 已拆至
src/pipeline/split_command.c/.h，pass_compile_commands.c 繼續保留 compile_commands JSON
orchestration、flag extraction 與既有 resolve_path wrapper；resolve_path 不在本項範圍，不能視為
已遷移。

同名 ABI 維持 int cbm_split_command(const char *cmd, char **out, int max_out)。輸入為 raw-byte、
first-NUL C string；NULL cmd、NULL out 或 max_out <= 0 回 0 且不寫入。結果沒有 sentinel，
只有 [0, count) slot 有效；正常資源條件下每個 token 是 caller 以 free 回收的 C malloc 配置。
token payload 上限 4095 bytes；僅 quote 外 ASCII 空白或 Tab 分隔，單雙引號移除、empty quote
不產生 token、未閉合 quote 仍輸出，反斜線不具 shell escape 語意。

wrapper CBM_USE_RUST_PIPELINE_SPLIT_COMMAND 經既有 v1 將 Rust token 配置為 C malloc，
保留既有 caller free 路徑。direct 使用 Cargo pipeline-split-command-only 與
CBM_USE_RUST_PIPELINE_SPLIT_COMMAND_ONLY；PIPELINE_SPLIT_COMMAND_SRCS = 空白時排除 C fallback，
由 Rust 匯出同名 public ABI。正常資源路徑的 C/Rust parity 已驗證，但 OOM allocation 不列入 ABI
parity 承諾：C strdup failure 會留下 NULL slot 且 count 仍遞增，既有 downstream caller 有 NULL
解參考風險；Rust 內部 Vec 與後續 C malloc 的配置順序及 OOM 行為不可直接等價。

已通過 cargo fmt --all -- --check、cargo test -p cbm-core pipeline_compile_commands --locked
（10 passed）及 cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings。
rust-pipeline-split-command-optin-test 與 rust-pipeline-split-command-only-test 已通過
default/wrapper/direct FFI、pipeline runner 各 **231 passed**、production --version、direct
PIPELINE_SPLIT_COMMAND_SRCS = 空白與 make -Bn source-negative；正向 production recipe 亦確認仍
連結 src/pipeline/pass_compile_commands.c。

下一步不是立即選新切片：complexity JSON 必須延後，先另行 hardening C strtol 的 locale whitespace、
overflow 與 NULL key 差異。Grok 新 session 應保留 dirty worktree，不得重新推進或重跑 #28，
並且只能先做唯讀 inventory；inventory 完成前不得宣稱新候選、實作或執行 gate。

## 第 27 項：Pipeline LSP classifiers true-source（2026-07-17）

此項是新的 `pipeline-lsp-classifiers` true-source slice，總計由 **21 / 21** 推進至
**22 / 22**，不是既有 ABI hardening。C fallback 為
`src/pipeline/lsp_cross_classifiers.c/.h`；`pass_lsp_cross.c` 只保留 cross-LSP orchestration、
arena、registry、language resolver dispatch 與結果合併。

公開同名 ABI 為 `cbm_pxc_ts_modes(CBMLanguage, const char *, bool *, bool *, bool *)`、
`cbm_pxc_has_cross_lsp(CBMLanguage)`、`cbm_pxc_map_label(const char *)`。header 的
`sizeof(CBMLanguage) == sizeof(int)` guard 凍結 Rust `c_int` 邊界。TS mode 的 bit 為
`1=js`、`2=jsx`、`4=dts`；NULL path 保留 language defaults，三個 output pointer 維持既有
非 NULL 前置條件。13 個 language 是 cross-LSP positive set。

label classifier 僅接受 Class、Struct、Interface、Enum、Type、Trait、Protocol、Function、Method；
NULL／拒絕回 NULL，允許時回原始 borrowed input pointer，不配置、不轉碼，維持 first-NUL C string
語意。C# 回 true 代表 pipeline eligibility，需 prebuilt cs registry；`cbm_pxc_run_one` fallback
不承諾 C# dispatch，未改變既有集合或行為。

wrapper 保留既有 v1 `int` ABI；direct 使用 Cargo `pipeline-lsp-classifiers-only` 與
`CBM_USE_RUST_PIPELINE_LSP_CLASSIFIERS_ONLY`。Make selector/FFI support source 使用
`PIPELINE_LSP_CLASSIFIERS_SRCS`，direct source-negative 排除 C fallback。

已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline_lsp_cross --locked`
（**3 passed**）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings` 與
兩個對應 Make gate。optin default/wrapper、only default/direct 共四組 FFI、pipeline runner
（各 **230 passed**）與 production `--version` 皆成功；direct
`PIPELINE_LSP_CLASSIFIERS_SRCS =` selector 和 source-negative 均通過。

下一候選尚未選定，待下一輪唯讀 inventory 依 C fallback、ABI、ownership、direct selector 與 gate
可驗證性決定；不可預先杜撰。

## 2026-07-17 current-revision：Pipeline JSON property ABI/gate hardening

`pipeline-json-prop` 是既有 true-source 的 current-revision ABI/gate hardening，不增加新的
true-source slice，歷史統計維持 **21 / 21**。公開 C ABI 為
`bool cbm_pipeline_extract_json_prop(const char *json, const char *key, char *buf, int bufsz)`；
v1 bridge 回傳 `int`，direct 匯出回傳 `bool`，Rust FFI 長度參數使用 signed `c_int`。

契約：NULL `json`／`key`／`buf` 或 `bufsz <= 0` 回 `false` 且 buffer 完全不變；空 key 可擷取，
空 value 回 `false` 且 buffer 不變；key 上限 59 bytes，60+（包括 `snprintf` 截斷）拒絕且不寫入。
此 helper 維持 raw scanner，不是 JSON parser，保留 first-NUL C string、第一個 byte hit 與原本
不做 escape decode 的語意。

`RUST_FFI_SUPPORT_SRCS` 已納入 `$(PIPELINE_JSON_PROP_SRCS)`，所以 default、wrapper、direct
三模式都走 public bridge；兩個 Make gate 的 default leg 都有 isolation production `--version`。
已通過 fmt、`cargo test -p cbm-core pipeline::route --locked`（**14 passed**）、全 target/features
clippy `-D warnings`、optin/only gate。三種 C pipeline runner 各 **229 passed**、三種 production
`--version` 均成功，direct `PIPELINE_JSON_PROP_SRCS =` selector 和 source-negative 均通過。

後續候選尚未選定，待下一輪唯讀 inventory 決定；不可預先杜撰。

## 第 25 項：Discover filters true-source（2026-07-17）

- 本項是原 `21/21` 預算結束後的後續 true-source slice；不改寫原始 `21/21`、第 22 項
  discover language-name true-source，或第 23 項既有 artifact-path parity/gate hardening（非新
  true-source）的定位。
- C fallback 為 `src/discover/filters.c`，僅涵蓋
  `cbm_should_skip_dir`、`cbm_has_ignored_suffix`、`cbm_should_skip_filename` 與
  `cbm_matches_fast_pattern`。walk、ignore 組態與其餘 discover orchestration 不遷移。
- `CBM_USE_RUST_DISCOVER_FILTERS=1` wrapper 經 C bridge 呼叫既有 v1 ABI；direct
  `CBM_USE_RUST_DISCOVER_FILTERS_ONLY=1`／Cargo `discover-filters-only` 使用 Rust
  同名 ABI。Rust direct ABI 原已存在，不宣稱為本項新寫；direct
  `DISCOVER_FILTERS_SRCS =`，且 `make -Bn` source-negative 已靜默證明 production 不連結
  C fallback。
- 契約為 NULL 回 `false`、只有 `CBM_MODE_FULL`（0）是 full，任何非零 mode 均為
  restricted；檔名、目錄、suffix 與 pattern 均採 raw byte 精確且大小寫敏感的既有比對。wrapper
  啟用 Rust bridge 時，C fallback filter tables 以條件編譯排除，避免 `-Werror`
  unused-variable。
- 已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core discover --locked`
  （5 passed）與 `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`。
  `make -j1 -f Makefile.cbm rust-discover-filters-optin-test` 已通過 default/wrapper FFI、
  Discover 各 89 passed、正式版 `--version`；`make -j1 -f Makefile.cbm
  rust-discover-filters-only-test` 已通過 default/direct FFI、direct Discover 89 passed、正式版
  `--version`、空白 `DISCOVER_FILTERS_SRCS =` 與靜默 source-negative。

## 2026-07-17 Discover path_join parity 修正（既有 true-source）

- 此項是既有 Discover path_join true-source 的 superseding parity 修正與 C/Rust regression 補強；
  不新增 true-source，亦不改變公開 ABI、Cargo feature、C fallback 或 direct selector。舊有
  「尚未驗證」歷史文字維持原樣。
- Rust path_join 在輸出寫入、截斷並將 `\` 轉成 `/` 後，若第一 byte 是 ASCII 小寫 drive、
  第二 byte 是 `:`，且第三 byte 為 `/` 或字串剛好在第二 byte 結束，會將第一 byte 大寫。
  因此 `c:/tmp` 與 `c:\\tmp` 均變為 `C:/tmp`，bare `c:` 變 `C:`；`c:relative`
  不變，`c:x` 截斷至 `c:` 時亦變 `C:`。
- 已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core discover --locked`
  （5 passed）與 `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`。
  `make -j1 -f Makefile.cbm rust-discover-path-join-optin-test` 已通過 default/wrapper FFI、
  Discover 各 89 passed 與 wrapper 正式版 `--version`；
  `make -j1 -f Makefile.cbm rust-discover-path-join-only-test` 已通過 default/direct FFI、
  Discover 各 89 passed、direct 正式版 `--version`、空白
  `DISCOVER_PATH_JOIN_SRCS =` 與靜默 source-negative。

## 第 26 項：Discover local_rel_path offset true-source（2026-07-17）

- 本項是原 `21/21` 預算結束後的後續 true-source slice；不改寫第 22 項原預算外 true-source、
  第 23 項非新增 true-source，以及第 25 項與 path_join parity 的既有歷史。
- C fallback 為 `src/discover/local_rel_path.c/.h`。`discover.c` 的兩個巢狀 `.gitignore`
  callsite 已將原 static pointer helper 改為 offset projection。
- 公開 API 為
  `size_t cbm_discover_local_rel_path_offset(const char *rel_path, const char *local_prefix)`。
  NULL rel/prefix、空 prefix 或未匹配回 `0`；只有 byte-exact、大小寫敏感的 prefix，並且下一 byte
  是 `/` 時回 `prefix_len + 1`。helper 不正規化、不配置、不修改、不保存指標。
- wrapper `CBM_USE_RUST_DISCOVER_LOCAL_REL_PATH=1` 經 v1 ABI；direct
  `CBM_USE_RUST_DISCOVER_LOCAL_REL_PATH_ONLY=1`／Cargo `discover-local-rel-path-only` 使用
  direct ABI。direct `DISCOVER_LOCAL_REL_PATH_SRCS =`，且 `make -Bn` source-negative 已靜默
  證明 production 不連結 C fallback。
- 已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core local_rel_path --locked`
  （5 passed）及 `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`。
  `make -j1 -f Makefile.cbm rust-discover-local-rel-path-optin-test` 已通過 default/wrapper FFI、
  Discover 各 90 passed、default/wrapper 正式版 `--version`；
  `make -j1 -f Makefile.cbm rust-discover-local-rel-path-only-test` 已通過 default/direct FFI、
  Discover 各 90 passed、default/direct 正式版 `--version`、空白
  `DISCOVER_LOCAL_REL_PATH_SRCS =` 與靜默 source-negative。

## 2026-07-17 Route-node classifiers 既有 true-source ABI/gate hardening

- 本項是既有 route-node classifiers true-source 的 ABI/gate hardening，不是新 true-source slice；
  原始 `21/21`、第 22 項原預算外、第 23 項非新增 true-source、第 25、26 項與 path_join parity
  的歷史均不改變。
- 新增 `src/pipeline/route_node_classifiers.h`，僅凍結既有 `hash_segment` 與
  `broker_route` classifier API 契約，不重新分類既有 C/Rust 演算法為新實作。
- `hash_segment` 對 NULL、長度 0、長度大於 12 拒絕；只接受小寫 ASCII 字母／數字，長度至少 3
  且含字母拒絕，短的合法字母／數字與全數字可接受；大寫、符號、NUL、非 ASCII 拒絕。
  `broker_route` 對 NULL 回 `false`，需從首 byte 以大小寫敏感方式匹配七個既有 prefix；
  `__route__GET__` 是負例。
- Make direct macro 改為 `CBM_USE_RUST_PIPELINE_ROUTE_NODE_CLASSIFIERS_ONLY`；FFI support
  使用 `ROUTE_NODE_CLASSIFIERS_SRCS` selector，direct 排除既有 C fallback。
- 已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::route --locked`
  （13 passed）及 `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`。
  `make -j1 -f Makefile.cbm rust-pipeline-route-node-classifiers-optin-test` 已通過
  default/wrapper FFI、route_canon pipeline 各 241 passed、正式版 `--version`；
  `make -j1 -f Makefile.cbm rust-pipeline-route-node-classifiers-only-test` 已通過
  default/wrapper/direct FFI、route_canon pipeline 各 241 passed、正式版 `--version`、空白
  `ROUTE_NODE_CLASSIFIERS_SRCS =` 與靜默 `make -Bn` source-negative。

## 2026-07-17 Discover language `.m` marker helpers 既有 true-source parity/ABI/gate hardening

- 本項是既有 true-source 的 parity、ABI 與 gate hardening，不是新 true-source slice；歷史
  `21/21` 維持不變，也不把既有 C/Rust 演算法重新分類為新實作。
- kind 2 契約凍結為 ASCII byte-only。NULL／invalid kind 行為不變；callable 名稱必須至少有一個
  `[A-Za-z]`，而下一 byte 必須立刻是 `(`。高位元組 `\xE9`、空名稱
  `"intrinsic ("`、空白或任何其他非 `(` 的下一 byte 均為負例，回 `false`。
  C 已移除 locale-sensitive `isalpha`，與 Rust `is_ascii_alphabetic` 收斂。
- C/Rust/FFI regressions 已加入；兩個 Make gate 的 default 階段增加 isolation production
  `--version`。
- 已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core discover --locked`
  （11 passed）及 `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`。
  `make -j1 -f Makefile.cbm rust-discover-language-markers-optin-test` 已通過
  default/wrapper FFI、Discover runner 90 passed、兩邊 production `--version`；
  `make -j1 -f Makefile.cbm rust-discover-language-markers-only-test` 整體 exit 0，已通過
  default/wrapper/direct FFI、Discover runner、production，direct
  `DISCOVER_LANGUAGE_MARKERS_SRCS =` 空白與靜默 `make -Bn` source-negative。
- 下一步是 `pipeline-json-prop` ABI 契約稽核，不可直接 gate；先凍結 C `bufsz=int` 對 Rust
  `usize`、NULL／`<= 0` buffer 與長 key 64 byte 截斷差異。LSP cross classifiers 是較乾淨的
  新 true-source 候選，但尚未開始。

## 第 24 項：AST profile classifiers true-source（2026-07-17）

- 本項是原 `21/21` 預算結束後的後續 true-source slice；不改寫原始 `21/21` 歷史、第 22 項
  discover language-name true-source，或第 23 項既有 artifact-path parity/gate hardening（非新
  true-source）的定位。
- 已將 classifier C fallback 拆至 `src/semantic/ast_profile_classifiers.c/.h`。`ast_profile.c` 保留
  Tree-sitter traversal 與 codec；本項不宣稱整個 AST profile 或 semantic module 已移轉。
- wrapper `CBM_USE_RUST_AST_PROFILE_CLASSIFIERS=1` 經 C bridge 呼叫既有 v1 ABI；direct
  `CBM_USE_RUST_AST_PROFILE_CLASSIFIERS_ONLY=1`／Cargo `ast-profile-classifiers-only` 排除該 C
  fallback，並由 Rust staticlib 匯出同名 `cbm_ast_profile_kind_flags`、
  `cbm_ast_profile_halstead_insert`、`cbm_ast_profile_is_param_name`。
- 契約凍結 kind flags、固定 512 槽 Halstead insert 與 parameter-name 比對；防禦性 NULL／無效
  輸入行為與 Rust ABI 對齊。
- 已通過 `cargo fmt --all -- --check`、
  `cargo test -p cbm-core ast_profile_classifiers --locked`（3 passed）及
  `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`。Make gate
  `rust-ast-profile-classifiers-optin-test` 已通過 default/wrapper FFI、pipeline 228 passed、
  production `--version`；`rust-ast-profile-classifiers-only-test` 已通過 default/direct 相同檢查、
  `AST_PROFILE_CLASSIFIERS_SRCS =` 與靜默成功的 `make -Bn` source-negative。完整
  `scripts/test.sh` 未執行。

## 第 23 項：既有 pipeline artifact-path parity 與 gate hardening（2026-07-17）

- 本項只強化既有 artifact-path bridge 的 C/Rust 短 buffer parity、direct linkage 與驗證；不是新的
  true-source replacement。原 multi-agent `21/21` 歷史結論及第 22 項的原預算外
  discover language-name true-source 均維持不變。
- 新公開 C header 的 ABI 為
  `bool cbm_pipeline_artifact_path(char *buf, size_t bufsz, const char *repo_path, const char *name)`。
  `buf` 是 caller-owned buffer；輸入與輸出只依 raw byte 處理、不做 UTF-8 解讀，且 `buf` 不得與
  任一輸入字串重疊。成功需寫入 NUL 結尾完整路徑並回 `true`；有效輸入但 `bufsz > 0` 過短時，仍須
  寫入 `min(full_len, bufsz - 1)` byte prefix 加 NUL，public ABI 回 `false`。`buf == NULL` 或
  `bufsz == 0` 時一律不寫入。
- C fallback 仍保留既有 `snprintf` 實作；v1 ABI 成功時回完整 `full_len`，過短或無效時回
  `SIZE_MAX`，其中過短仍保留上述 prefix 寫入副作用。direct Rust 匯出的同名 public `bool` ABI 也必須
  在過短時寫 prefix 加 NUL 後回 `false`，不可把 C `snprintf` 的完整長度誤當 public 成功回傳值。
- wrapper `CBM_USE_RUST_PIPELINE_ARTIFACT_PATH=1` 經 v1 ABI 委派；direct replacement 使用
  `CBM_USE_RUST_PIPELINE_ARTIFACT_PATH_ONLY=1` 與 Cargo
  `pipeline-artifact-path-only`。FFI selector 在 default/wrapper 使用 C bridge、direct 使用 Rust
  同名 symbol；artifact runner 與 production staticlib 的 direct 選擇亦已補強。direct 時
  `ARTIFACT_PATH_SRCS =`，`make -Bn` source-negative 證明 production link 不含 C fallback。
- 已通過 `cargo fmt --all -- --check`、artifact-path Rust unit（4 passed）與 clippy；
  `rust-pipeline-artifact-path-optin-test` 的 default/wrapper FFI、artifact runner（各 10 passed）與
  production `--version`，以及最終重跑的 `rust-pipeline-artifact-path-only-test` default/direct
  FFI、artifact runner（各 10 passed）、production `--version`、empty selector 與 `-Bn`
  source-negative 均成功。完整 `scripts/test.sh` 未執行。

## 原預算外後續切片（2026-07-17：discover `cbm_language_name` true-source replacement，第 22 項）

- 原 multi-agent 預算的 `21/21` 為已結束的歷史結論；本項是預算外、獨立編號的第 22 個
  true-source 切片，不重寫或延伸該歷史計數。產品 default 仍為 C11。
- 將狹窄的 `cbm_language_name` 拆為 `src/discover/language_name.c/.h` C fallback；
  `src/discover/language.c` 保留語言偵測與其餘行為。公開 ABI 為
  `const char *cbm_language_name(CBMLanguage)`，回傳 static、borrowed、NUL 結尾且 immutable 的
  名稱，不得由呼叫端釋放；invalid、`CBM_LANG_COUNT`、out-of-range 與未來 enum hole 一律為
  `"Unknown"`。full-enum sweep 已覆蓋全部有效 enum，並固定 `CBM_LANG_CFSCRIPT`（157）與
  `CBM_LANG_CFML`（158）均回 `"CFML"`。
- `CBM_USE_RUST_DISCOVER_LANGUAGE_NAME=1` 的 wrapper 經 v1 ABI
  `cbm_rs_discover_language_name_v1(int)` 委派 Rust，僅當 Rust 意外回傳 `NULL` 時才回退 C。
  direct replacement 使用 `CBM_USE_RUST_DISCOVER_LANGUAGE_NAME_ONLY=1` 與 Cargo feature
  `discover-language-name-only`；`DISCOVER_LANGUAGE_NAME_SRCS =`，`make -Bn` source-negative
  證實 production link 排除 `src/discover/language_name.c`。既有 full-language direct 與本項
  direct 因匯出同一 public ABI 而互斥。
- 已通過 `cargo fmt --all -- --check`、language-name Rust unit（3 passed）與 clippy；
  `rust-discover-language-name-optin-test` 的 default/wrapper FFI、discover（各 87 passed）與
  production `--version`，`rust-discover-language-name-only-test` 的 default/direct FFI、discover
  （各 87 passed）、production `--version`、empty selector/source-negative，以及
  `rust-discover-userconfig-language-name-only-test` combo 的 FFI、discover（87 passed）與
  production `--version` 均成功。完整 `scripts/test.sh` 未重跑。

## 2026-07-17 現況優先規則

目前權威交接快照為 [docs/rust-refactor-current-handoff.md](docs/rust-refactor-current-handoff.md)。
本文件後續的歷史測試數字與切片狀態僅代表各自日期；近期測試、Make gate 或 source selector
變更後，必須重新執行受影響 gate，不能沿用歷史結果。

真正的 direct source replacement 必須有獨立 C fallback compilation unit、公開 bridge、direct
Rust ABI、Makefile source selector、實際執行的 default/wrapper/direct gate，以及 make -Bn 的
production source-negative 證據。C wrapper 呼叫 Rust 不足以構成 source replacement。

本輪 multi-agent（limit 21）已完成 21 個 true-source 切片（含 envscan classifiers、
configlink helpers、parallel JSON、definitions JSON、discover language `.m` markers、
similarity fingerprint parser、infrascan JSON cleaner、Pipeline test-node `is_test` classifier 與
semantic camel-break classifier、semantic token delimiter classifier 與 Pipeline registry
`is_test_qn` classifier）。
皆含獨立 CU、公開 bridge、ONLY selector 空、make -Bn source-negative（durable log）與
production --version。產品 default 仍是 C11。本輪 `21/21` 已結束；後續 wrapper-only pure helper
候選必須列為新的 multi-agent 預算，不延伸本輪。

## 摘要

本文件規劃以分階段、可驗證、低風險方式，將原本以 C 為主的 `codebase-memory-mcp` 逐步重構為 Rust。產品預設路徑目前仍是 C，Rust 僅透過明確 opt-in 參與部分 foundation、store、MCP 與 pipeline 決策/adapter 切片；重點不是一次性全面改寫，而是先建立 Rust 骨架與行為基準，再按模組遷移，確保現有單一靜態執行檔、MCP 介面、SQLite 儲存格式、tree-sitter 支援、CLI 行為與安全稽核流程都能維持相容。

## 歷史工作記錄（非當前快照）

## 本次工作階段（2026-07-17 Pipeline registry `is_test_qn` true-source replacement）

- 新增獨立 C fallback compilation unit `src/pipeline/registry_test_qn.c/.h` 與公開 ABI
  `cbm_pipeline_registry_is_test_qn(const char *qn)`。`NULL`／empty 回 `false`；對有效 NUL
  結尾 C string 以 raw byte、大小寫敏感 substring 判定 `Test`、`test`、`Mock`、`mock`、`Stub`、
  `stub`、`Fake`、`fake`、`Fixture`、`spec` 十個 needle，`contest` 命中並回 `true`，non-UTF-8
  byte 亦以 raw byte 安全處理。
- `CBM_USE_RUST_PIPELINE_REGISTRY_TEST_QN=1` wrapper 經既有 v1 ABI 委派 Rust，僅採用 `0`／`1`；
  其他回傳值回退 C。真正 direct replacement 僅由
  `CBM_USE_RUST_PIPELINE_REGISTRY_TEST_QN_ONLY=1` 與 Cargo
  `pipeline-registry-test-qn-only` 啟用。
- direct state 的 `PIPELINE_REGISTRY_TEST_QN_SRCS =` 為空，`make -Bn` production source-negative
  證明不連結 `src/pipeline/registry_test_qn.c`。只遷移 test-QN score classifier；registry handle、
  cache、resolution、graph runtime 與 ownership 仍由 C 管理。
- 已通過 Rust fmt、`cargo test -p cbm-core pipeline::registry --locked`（4 passed）、clippy，
  default/wrapper/direct FFI 與 registry 各 54 passed，三路 production `--version`，以及 direct
  empty selector/source-negative；完整 `scripts/test.sh` 未重跑。

## 本次工作階段（2026-07-17 LSP cross classifier enum parity fix）

- 修正 Rust `pipeline_lsp_cross::LANG_C` 由 `15` 為 C enum 實際值 `14`，並固定
  `CBM_LANG_BASH`（`15`）為 false；Rust unit 3 passed，既有 wrapper FFI smoke 與 pipeline
  228 passed。此為既有 wrapper regression 修復，不計入 true-source，未新增 direct selector。

## 本次工作階段（2026-07-17 semantic token delimiter true-source replacement）

- 新增獨立 C fallback compilation unit `src/semantic/token_delim.c/.h` 與公開 scalar ABI
  `cbm_semantic_is_token_delim(unsigned char byte)`。九個 ASCII delimiter `.`、`/`、`_`、`-`、
  空白、`(`、`)`、`,`、`:` 回 `true`；其他所有 byte（含高位 byte）回 `false`。
- 既有 `CBM_USE_RUST_SEMANTIC_TOKEN_CLASSIFIERS=1` wrapper 經
  `cbm_rs_semantic_is_token_delim_v1` 委派 Rust 並保留 C fallback。真正 direct replacement 僅由
  `CBM_USE_RUST_SEMANTIC_TOKEN_DELIM_ONLY=1` 與 Cargo `semantic-token-delim-only` 啟用；
  camel-break 仍是獨立切片，沒有 broad token-classifiers-only。
- direct state 的 `SEMANTIC_TOKEN_DELIM_SRCS =` 為空，`make -Bn` production source-negative
  證明不連結 `src/semantic/token_delim.c`。tokenization、token/字串 ownership、semantic
  orchestration 與 embedding 仍由 C 管理。
- 已通過 Rust fmt、Rust unit（2 passed）、clippy，default/wrapper/direct FFI 與各 pipeline
  228 passed，default/wrapper/direct production `--version`，以及 direct empty selector/source-negative；
  完整 `scripts/test.sh` 未重跑。

## 本次工作階段（2026-07-17 semantic camel-break classifier true-source replacement）

- 新增獨立 C fallback compilation unit `src/semantic/camel_break.c/.h` 與公開
  `cbm_semantic_is_camel_break(const char *name, int index)` ABI。契約固定為 `NULL` 或
  `index <= 0` 回 `false`；非 NULL 的正 index 必須指向 NUL 結尾 `name` 內有效且非 NUL 的 byte，
  僅前一個 ASCII 小寫、目前 ASCII 大寫時回 `true`，不跨越配置或字串邊界。
- 既有 `CBM_USE_RUST_SEMANTIC_TOKEN_CLASSIFIERS=1` wrapper 經
  `cbm_rs_semantic_is_camel_break_v1` 委派 Rust 並保留 C fallback。真正 direct replacement 僅由
  `CBM_USE_RUST_SEMANTIC_CAMEL_BREAK_ONLY=1` 與 Cargo `semantic-camel-break-only` 啟用；
  delimiter/tokenization 未遷移，因此不使用 broad `semantic-token-classifiers-only`。
- direct state 的 `SEMANTIC_CAMEL_BREAK_SRCS =` 為空，`make -Bn` production source-negative
  證明不連結 `src/semantic/camel_break.c`。已通過 Rust fmt、Rust unit（2 passed）、clippy，
  default/wrapper/direct FFI 與各 pipeline 228 passed，wrapper/direct production `--version`，
  以及 direct empty selector/source-negative；完整 `scripts/test.sh` 未重跑。

## 本次工作階段（2026-07-17 Pipeline test-node `is_test` classifier true-source replacement）

- 新增獨立 C fallback compilation unit `src/pipeline/test_node_is_test.c/.h` 與公開
  `cbm_pipeline_test_node_is_test()` bridge；default path 以嚴格連續 raw-byte
  substring `"is_test":true` 判定，`NULL` 回 `false`。
- `CBM_USE_RUST_PIPELINE_TEST_NODE_IS_TEST=1` wrapper 只透過既有
  `cbm_rs_pipeline_test_node_is_test_v1` 委派 Rust；Cargo
  `pipeline-test-node-is-test-only` feature 與
  `CBM_USE_RUST_PIPELINE_TEST_NODE_IS_TEST_ONLY=1` 讓 Rust staticlib 直接匯出同一公開 ABI。
  `pass_tests.c` 只消費公開 bridge，`TESTS` / `TESTS_FILE` edge 建立、graph-buffer ownership
  與 pipeline side effects 均保留 C。
- 已完成 default、wrapper、direct 三路 FFI 與 pipeline gate，各 228 passed；wrapper/direct
  production binary `--version` smoke 通過。ONLY selector 為空，且 `make -Bn` 的 direct
  production source-negative 證據確認不含 `src/pipeline/test_node_is_test.c`。

## 本次工作階段（2026-07-13 CLI progress sink direct replacement）

- 新增 Cargo `progress-sink-only` feature；Rust staticlib 直接匯出既有 `cbm_progress_sink_init`、`cbm_progress_sink_fini` 與 `cbm_progress_sink_fn` ABI。
- `Makefile.cbm` 在 `CBM_USE_RUST_PROGRESS_SINK_ONLY=1` 時排除 `src/cli/progress_sink.c`，由 Rust 匯出 ABI 接替 C compilation unit；Rust 只透過既有 `cbm_log_set_sink()` 註冊 callback，`FILE*`、logger global state、stderr 與 sink lifecycle ownership 仍由 C 保留。
- 新增 `rust-cli-progress-sink-only-test`，並納入 `rust-ci` 與 `rust-ci-tests`；direct FFI link 額外帶入 `src/foundation/log.c`，避免 test-only staticlib 缺少 logger sink symbol。
- 已驗證：default `progress_sink`（2 passed）、direct Rust-only `test-rust-ffi`、direct `progress_sink`（2 passed）、direct production link 與 `codebase-memory-mcp --version`；direct link line 不含 `src/cli/progress_sink.c`。
- 此切片只代表 CLI progress sink compilation unit 已有 Rust direct replacement，不代表 CLI、logger、server lifecycle 或整體 Rust-backed release candidate 已完成。

## Phase 5 Release Gate 狀態
- 未達成：預設 runtime 切換到 Rust-backed。
- 目前驗證重點：編譯、測試、安全與效能相關門檻均已分批落地，但僅為 opt-in 驗證。
- 發行預設仍為 C-backed；未完成 fallback/回滾與跨週期 default switch 的最終放行條件前不會改變預設。
- 下一步：完成 full pipeline top-plan 一致性 release candidate、跨平台 C/Rust parity、並定義預設切換解除條件。

## 本次工作階段（2026-07-12 Store extension-language map opt-in）

- 已實作 `store::arch_helpers::ext_language_kind()`、`cbm_rs_store_ext_lang_kind_v1` 與 `CBM_USE_RUST_STORE_LANGUAGE_MAP=1`。Rust 僅以 borrowed extension bytes 對齊 C `ext_lang_table` 的 44 個位置並回傳 `0..43`；未知、`NULL`、大小寫不符回 `-1`，不保存或回傳 C/Rust 字串指標。
- C `ext_to_lang()` 僅接受經範圍檢查的索引後，回傳既有 C 靜態表中的 language 指標；任何 ABI 異常值皆回退原 C 查表。SQLite architecture query、aggregation/sort、result allocation 與 Store ownership 均未遷移。
- 已新增 `cc/cxx`、`jsx/tsx`、`yaml/yml` 與未知副檔名的 architecture integration test，並實跑 Rust unit（7 passed）、FFI smoke、`rust-store-language-map-optin-test`（default/opt-in `store_arch mcp` 各 187 passed），以及 42 個 common flags 的 `rust-all-optin-test`（wrapper/direct 完整 runner 各 5867 passed，兩個 production binary 的 `--version` smoke 均通過）。這是 acceptance 前置驗證，並非 Rust-backed release candidate。

## 本次工作階段（2026-07-12 Cypher two-character lexer opt-in）

- 已實作 `cypher::two_char_kind_or_eof()`、`cbm_rs_cypher_two_char_kind_v1` 與 `CBM_USE_RUST_CYPHER_LEX_TWO_CHAR=1`。C `lex_try_two_char()` 只採用 Rust 的 `TOK_NEQ`、`TOK_EQTILDE`、`TOK_GTE`、`TOK_LTE`、`TOK_DOTDOT`，其餘 ABI 值完整回退既有 C pair table；預設 C path 不變。
- 契約固定兩個 `0..=255` byte：`!=`、`<>`、`=~`、`>=`、`<=`、`..` 回既有 token enum，未知 pair 回 `TOK_EOF`，任一範圍外 FFI 輸入回 `-1`。Rust 不借用、不保存或回傳 C 指標。
- 邊界：input/cursor、C-owned token text/allocation、parser、AST normalizer、evaluator、executor 與 query 行為仍在 C。
- 已實跑：`cargo test -p cbm-core cypher --locked`（16 passed）、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm rust-cypher-lex-two-char-optin-test`（default/opt-in Cypher + contract 各 166 passed），以及 41 個 common flags 的 `rust-all-optin-test`（wrapper/direct 完整 runner 各 5866 passed，兩個 production binary 的 `--version` smoke 均通過）。這是 acceptance 前置驗證，並非 Rust-backed release candidate。

## 本次工作階段（2026-07-12 logger environment parser opt-in）

- 已將既有 `foundation::log` 的 `cbm_rs_log_parse_level_value`、`cbm_rs_log_parse_format_value` 接入 `CBM_USE_RUST_LOG_ENV_PARSE=1`。C 繼續負責 `getenv()`、global logger state、sink、stderr 與 lifecycle；Rust 只解析 borrowed value 並回傳 scalar。
- C 只接受合法 level `0..4` 與 format `0..1`，異常 scalar 維持既有 current 值。level 對齊 ASCII case-insensitive 名稱與完整 `strtol(..., 10)` 行為；format 只接受 `text`／`json` 且不 trim 空白。預設 C parser 不變。
- 已補 ` +3`、尾隨空白與 format 前導空白 edge case，並實跑 Rust unit（2 passed）、FFI smoke、`rust-foundation-optin-test`（default、各單一及含新 flag 的 combined foundation path 均 325 passed），以及 40-common-flag `rust-all-optin-test`（wrapper/direct 完整 runner 各 5866 passed、兩個 production `--version` smoke 通過）。這是 acceptance 前置驗證，並非 Rust-backed release candidate。

## 本次工作階段（2026-07-12 Cypher single-character lexer opt-in）

- 已實作 `cypher::single_char_kind_or_eof()`、`cbm_rs_cypher_single_char_kind_v1` 與 `CBM_USE_RUST_CYPHER_LEX_SINGLE_CHAR=1`。C `lex_single_char()` 只採用 Rust 的 `TOK_EOF`、`TOK_LPAREN..TOK_EQ` 或 `TOK_PIPE`，其餘 ABI 回傳值完整回退既有 C `switch`；預設 C path 不變。
- 契約固定 `0..=255` byte：15 個既有符號回既有 token enum，未知 byte 回 `TOK_EOF`，負值或大於 255 的 FFI 輸入回 `-1`。Rust 不借用、不保存或回傳 C 指標。
- 邊界：只遷移單字元 classifier；token allocation、position、two-character lexer、parser、AST normalizer、evaluator、executor 與 query 行為仍在 C。
- 已實跑：`cargo test -p cbm-core cypher --locked`（15 passed）、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm rust-cypher-lex-single-char-optin-test`，以及 39 個 common flags 的 `rust-all-optin-test`（wrapper/direct 完整 runner 各 5866 passed，兩個 production binary 的 `--version` smoke 均通過）。這是 acceptance 前置驗證，並非 Rust-backed release candidate。

## 本次工作階段（2026-07-12 registry test-QN classifier opt-in）

- 已實作 `pipeline::registry::is_test_qn()`、`cbm_rs_registry_is_test_qn_v1` 與 `CBM_USE_RUST_PIPELINE_REGISTRY_TEST_QN=1`。C `is_test_qn()` 只接受 Rust 的 `0` / `1` 回傳值，其他值回退既有 C classifier；預設 C path 不變。
- 契約為 nullable NUL-terminated QN 的 raw-byte、大小寫敏感 substring classifier：`NULL` / empty 回 `0`，命中 `Test`、`test`、`Mock`、`mock`、`Stub`、`stub`、`Fake`、`fake`、`Fixture` 或 `spec` 回 `1`；不轉碼非 UTF-8 bytes。它只影響 registry candidate score 的 test-like 判定，不使用 opaque registry handle，也不遷移 add/resolve/cache、所有權或 pipeline side effects。
- 已實跑：`cargo test -p cbm-core pipeline::registry --locked`（4 passed）、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm rust-pipeline-registry-test-qn-optin-test`（default/opt-in registry suite 各 53 passed），以及當時 38 個 common flags 的 `rust-all-optin-test`（wrapper/direct 完整 runner 各 5865 passed，兩個 production binary 的 `--version` smoke 均通過）。這是 acceptance 前置驗證，並非 Rust-backed release candidate。

## 本次工作階段（2026-07-11 diagnostics formatter production opt-in）

- 已將既有 Rust `foundation::diagnostics` 的 env value、query snapshot、`format_path`、`format_json` 與 `format_ndjson` 接入 `CBM_USE_RUST_DIAGNOSTICS_FORMAT=1`；C wrapper 仍保留原邏輯，Rust ABI 回傳異常時 fallback 到 C。
- 此切片只替換 deterministic diagnostics pure logic；atomic query stats 的載入、system metrics、writer thread、檔案寫入/rename、NDJSON rotation、stderr lifecycle 與 diagnostics start/stop ownership 仍由 C 執行。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core foundation::diagnostics --locked`（3 passed）、`make -f Makefile.cbm rust-ffi-test`（exit 0）、`rust-diagnostics-format-optin-test`（default 與 opt-in diagnostics suite 各 7 passed）、`clang-format --dry-run --Werror src/foundation/diagnostics.c` 與 `git diff --check`。全 tree `lint-format` 仍受既有 pipeline/traces dirty 變更的排版違規影響，未記為通過。

## 本次工作階段（2026-07-11 dump verify parser Rust path 收斂）

- `CBM_USE_RUST_DUMP_VERIFY=1` 的 ratio parser 現在由 Rust byte-level parser 實際執行，不再由 `ffi.rs` 呼叫 C `strtod`；支援 ASCII whitespace、十進位/十六進位浮點與 partial-prefix，C 端 warning/default contract 保持不變。
- 補上 `0x1p-1` hex float regression，並保留 null、invalid、range、incomplete exponent 與非 UTF-8 suffix 的安全 fallback 語意；dump verification gate 的 node-count classifier、logging 與 MCP/store 呼叫仍由 C 控制。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core foundation::dump_verify --locked`（3 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`CBM_USE_RUST_DUMP_VERIFY=1 make -f Makefile.cbm test-foundation`（325 passed）、`make -f Makefile.cbm rust-ffi-test`（exit 0）與 `git diff --check`。

- Phase 0：已新增 `docs/rust-refactor-baseline.md` 與 `tests/golden/rust-refactor/` 第一批小型 golden fixtures，固定 build/test/lint/security、CLI index/status/list、CLI invocation forms、MCP initialize/tools list pagination/error/Content-Length transcript、alias/schema/indexed-repo tool call transcript、小型 graph histogram、language graph parity、artifact export/bootstrap/import/schema mismatch、小型效能 smoke、手動 repository self-index baseline 與手動大型 synthetic performance baseline。
- Phase 1：已新增 Cargo workspace、`cbm-core` crate、`rust/CBM_FFI.md`、`scripts/rust-check.sh` 與 `Makefile.cbm` 的 `rust-*` targets；預設產品 C binary 仍不連入 Rust，只有明確 opt-in 才連 `cbm-core` staticlib。
- Phase 2：已建立第一批 foundation parity 模組：`dump_verify`、`str_intern`、`str_util` 與窄切 `platform` helpers 的 Rust 實作及單元測試。`dump_verify` 可用 `CBM_USE_RUST_DUMP_VERIFY=1` opt-in，`str_intern` 可用 `CBM_USE_RUST_STR_INTERN=1` opt-in，`str_util` 可用 `CBM_USE_RUST_STR_UTIL=1` opt-in predicate、validator、`json_escape`、路徑 helpers 與 arena-backed 字串 helpers；C wrapper 仍由 `CBMArena` 配置輸出，Rust 只回傳 offset/required length 或寫入呼叫端 buffer。`platform` 路徑正規化、Linux cgroup quota helpers 與 env/dirs helpers 可分別用 `CBM_USE_RUST_PLATFORM_PATH=1`、`CBM_USE_RUST_PLATFORM_CGROUP=1`、`CBM_USE_RUST_PLATFORM_ENV_DIRS=1` opt-in；env/dirs 覆蓋 `cbm_safe_getenv`、home/config/local/cache 優先序、覆寫、截斷與路徑正規化。下一批 foundation 已補 diagnostics deterministic formatter、hash_table、YAML、log/profile、compat 與 arena public-struct test-only Rust parity/FFI fixture；profile env gate 另可用 `CBM_USE_RUST_PROFILE_ENV=1` 窄範圍 opt-in，僅委派 `CBM_PROFILE` 啟用判斷，clock/log/global state 仍留 C。hash_table 與 arena allocator hot path 暫定 C-only，不導入產品 Rust opt-in。預設仍走原 C implementation。
- Phase 3：已將 traces 純 helper 模組搬到 Rust parity / FFI 薄包裝層：`cbm_rs_traces_extract_service_name_v1`、`cbm_rs_traces_extract_http_info_v1`、`cbm_rs_traces_extract_path_from_url_v1`、`cbm_rs_traces_parse_duration_v1` 與 `cbm_rs_traces_calculate_p99_v1` 可由 `CBM_USE_RUST_TRACES=1` 委派 Rust；`src/traces/traces.c` 預設仍保留 C fallback，`tests/test_traces.c` 與 `tests/test_rust_ffi.c` 皆已可驗證對外契約。MCP `ingest_traces` handler 行為仍由 C 控制，未納入此 helper 切片。
- 2026-07-10 安全邊界收斂：`traces.rs` 已改為純位元組/值型邏輯，所有 trace C 指標解碼、caller buffer 寫入與 URL static buffer 都留在 `ffi.rs`；安全稽核不為 traces 新增 allowlist。`HttpPath::Url` 會直接複製已解析 path，避免 ABI 層二次解析。已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core traces --locked`（7 passed）、`make -f Makefile.cbm rust-ffi-test`、`python3 scripts/rust-security-audit.py`（557 findings allowlist passed）與 `git diff --check`；未在此收斂後重跑會清除共享 `build/c` 的 opt-in target。
- 2026-07-10 traces Rust-only opt-in：新增 `CBM_USE_RUST_TRACES_ONLY=1` 與 Cargo `traces-only` feature；啟用時 `Makefile.cbm` 排除 `src/traces/traces.c`，由 Rust staticlib 直接匯出既有 `traces.h` 的五個 `cbm_*` symbols，預設及 `CBM_USE_RUST_TRACES=1` wrapper 路徑皆保留。C 連結的 default/Rust-only archive 分別位於 `target/cbm-default`、`target/cbm-traces-only`，避免 Cargo feature cache 混用；聚焦 target `rust-traces-only-test` 使用隔離 runner，依序驗證 default 與 Rust-only 的 `traces mcp` suite。已通過兩組 161 passed、`cargo test -p cbm-core --features traces-only ffi::tests::traces_only_exports_preserve_c_abi --locked`、Rust-only archive symbol 檢查及 Rust-only link line 不含 `src/traces/traces.c`。
- Phase 4：已啟動低風險 registry 與 plan 切片。registry 已新增 Rust `pipeline::registry` 純決策模組、`cbm_rs_registry_resolve_once` parity fixture，以及產品用的 opaque registry handle FFI。`CBM_USE_RUST_PIPELINE_REGISTRY=1` 現在會讓 `cbm_registry_add` 同步寫入 Rust handle，`cbm_registry_resolve` 在既有 C per-file cache miss 後先嘗試 Rust resolver，再將 Rust 回傳 QN 映射回 C registry-owned key；`label_of`、`find_by_name`、`fuzzy_resolve` 與預設 C path 仍保留。plan 已新增 Rust `pipeline::plan` parity fixture，固定 full pipeline、sequential、parallel extraction、predump、parallel threshold、incremental extract/resolve 與 incremental post/tail typed order。`CBM_USE_RUST_PIPELINE_PLAN=1` 目前讓 C full/incremental extraction phase 讀 Rust plan 來選擇 parallel 或 sequential，並在驗證 Rust typed v2 metadata 後 dispatch incremental extract/resolve：sequential 執行 `definitions`、`calls`、`usages`、`semantic`，parallel 執行 `incr_extract`、`incr_registry`、`incr_resolve`；讓 sequential extraction dispatch 依 Rust typed v2 metadata 執行 `definitions`、`k8s`、`lsp_cross`、`calls`、`usages` 與 `semantic`，讓 parallel extraction 外層 metadata 固定 `parallel_extract`、`registry_build`、`lsp_cross_prepare`、`parallel_resolve`、`infra_routes`、`infra_bindings` 與 `k8s` 的順序與 policy trace，讓 full pipeline pre-dump 階段依 Rust typed v2 predump metadata dispatch `decorator_tags`、`configlink`、`route_match`、`similarity`、`semantic_edges` 與 `complexity`，並讓 typed incremental post steps dispatch `k8s`、`incr_tests`、`incr_decorator_tags`、`incr_configlink`、`incr_similarity`、`incr_semantic_edges`、`edge_relink`、`incremental_dump`、`persist_hashes` 與 `artifact_export`；C 端仍固定驗證 sequential/predump/parallel/incremental extraction `kind/phase/policy/gate_flags/requires_mask/effect_flags` 與 incremental tail 順序/policy，並執行既有 C pass、cache/shared id、cross-LSP cleanup/skip、infra tail、dump、hash persist、FTS rebuild 與 artifact export side effects。inline pass trace contract 會固定 sequential/predump/parallel/incremental extraction dispatch log、`source=typed_v2` 與 pass 順序；incremental dump failure 會原樣傳回錯誤，並跳過後續 `persist_hashes` / `artifact_export` tail。FullPipeline top-level metadata 已新增獨立 FFI，固定 12 個 full/moderate/advanced outer steps、fast-mode 11 steps、`requires_mask`、gate/effect flags 與 nested plan kind；`pipeline.c` runtime 現在會在 `CBM_USE_RUST_PIPELINE_PLAN=1` 於 discovery 後以實際 `mode`、`worker_count` 與 `file_count` 載入 top-level metadata、記錄 `rust_plan.dispatch phase=full_pipeline source=top_v1`，並把 `extraction_dispatch` nested plan kind 傳入 extraction choice；`pipeline_incremental.c` 仍不直接消費 FullPipeline metadata，且 `PlanStepV2(kind=FullPipeline)` 仍保持 unsupported。graph-buffer mutation command 邊界已新增 Rust test-only metadata/validation ABI，並新增 C-side `graph_buffer_mutation` adapter，讓 full structure、incremental purge/file-upsert、incremental inbound restore、`TESTS`/`TESTS_FILE`、`pass_usages.c` usage/throws/read-write edges、`pass_configlink.c` configlink edges、`pass_similarity.c` `SIMILAR_TO`、`pass_semantic_edges.c` `SEMANTICALLY_RELATED` 與 `pass_enrichment.c` decorator-tag node upsert、`pipeline.c` infra route/topic route node upsert 與 `INFRA_MAPS` edge、`pass_githistory.c` temporal File metadata upsert/file-change edges、sequential `pass_definitions.c`、`pass_calls.c`、`pass_semantic.c`，以及 `pass_parallel.c` 的 shared/main registry、worker-local graph 與 service helper node/edge 寫入都先走 command apply 邊界；Rust 只固定 `UpsertNode`、`InsertEdge` 與 `DeleteByFile` 的欄位、結果型別、effect flags、null/empty string contract，以及 `InsertEdge` 不做 endpoint lookup 的合約，實際 pass function、tree-sitter/extraction/LSP、graph-buffer storage/ownership 與所有寫入仍留在 C。language graph parity gate 已固定 Python、TypeScript、Go、Rust、Java、C++ 與 YAML 代表性 fixture，並比對 default C path 與 Rust pipeline/store opt-in path 的 definitions、calls、imports、routes、semantic/LSP edges。
- 驗證（2026-07-02 baseline）：本機曾完整通過 `scripts/test.sh`、`scripts/rust-check.sh`、`scripts/build.sh`、`scripts/smoke-test.sh build/c/codebase-memory-mcp`、`scripts/smoke-invariants.sh build/c/codebase-memory-mcp`（30 passed / 0 failed）、`make -f Makefile.cbm security`、`make -f Makefile.cbm rust-ci` 與 `make -f Makefile.cbm rust-baseline-fixtures`；`scripts/lint.sh --ci` 也以 CI 對齊的 `cppcheck 2.20.0` + `clang-format 20.1.8` 通過，`RUST_AUDIT_REQUIRED=1` 已用 `cargo-audit v0.22.2` 驗證 `Cargo.lock`。
- 驗證（2026-07-04 Phase 4 targeted gates）：本輪重跑 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::plan --locked`（11 passed）、`cargo test -p cbm-core pipeline::graph_mutation --locked`（3 passed）、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner graph_buffer`（51 passed）、`build/c/test-runner pipeline`（default C path，217 passed）、`make -f Makefile.cbm rust-pipeline-plan-optin-test`（217 passed）、`make -f Makefile.cbm rust-language-graph-parity`、`make -f Makefile.cbm lint-format`、`git diff --check` 與 `git diff --exit-code -- tests/golden/rust-refactor`。完整 `scripts/test.sh` / `scripts/lint.sh --ci` / `make -f Makefile.cbm security` 尚未在本輪 targeted 修補後重跑，仍以 2026-07-02 baseline 為最近完整紀錄。
- 驗證（2026-07-05 Phase 4 targeted gates）：本輪新增 `SIMILAR_TO`、`SEMANTICALLY_RELATED`、decorator-tag upsert、`pipeline.c` infra route/tail (`Route` upsert、`INFRA_MAPS` 及 route-topic map edge)、`pass_githistory.c` FILE_CHANGES_WITH 與 File temporal props 的 C-side adapter callpoint；並重跑 `make -f Makefile.cbm clean-c build/c/test-runner`、`build/c/test-runner graph_buffer pipeline`（268 passed）、`make -f Makefile.cbm rust-pipeline-plan-optin-test`（217 passed）、`make -f Makefile.cbm rust-language-graph-parity`、`scripts/lint.sh --ci`、`cargo fmt --all -- --check`、`make -f Makefile.cbm lint-format`、`git diff --check` 與 `git diff --exit-code -- tests/golden/rust-refactor`。同日稍早 `scripts/test.sh` 曾啟動但依使用者要求中斷，不視為新的完整通過；`make -f Makefile.cbm security` 也尚未在此批 adapter 變更後重跑。
- 驗證（2026-07-05 Phase 4 completion verification）：新增一輪針對新 adapter callpoint 的 C/安全回歸：`CBM_SKIP_RUST=1 scripts/test.sh`（5798 passed）與 `make -f Makefile.cbm security`。其中 `security` 全量檢核完成 1/1、1/1、2/2、3/1、4/1、5/1、6/1、7/1、8/1；`security-ui` 全通過、`security-fuzz` `23/23`、並完成 vendored 完整性核對；`scripts/lint.sh --ci` 亦在同日通過。`CBM_SKIP_RUST=1` 會跳過 `scripts/test.sh` 的 Step 7 Rust parity，因此另補跑無 skip 的完整 `scripts/test.sh`：C runner 5798 passed、parent-death watchdog 通過、security-string 2 passed / 0 failed、Rust unit tests 51 passed、registry opt-in suite 53 passed、pipeline plan opt-in suite 217 passed，最終 `=== All tests passed ===`。
- 驗證（2026-07-05 Phase 4 sequential pass adapter gates）：新增 `pass_definitions.c`、`pass_calls.c`、`pass_semantic.c` adapter callpoint 收斂，並重跑 `build/c/test-runner graph_buffer pipeline lang_contract edge_structural`（331 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。direct-call search 確認這三個檔案不再直接呼叫 `cbm_gbuf_upsert_node` / `cbm_gbuf_insert_edge`；當時留待獨立批次的 `pass_parallel.c` 已由下一條 pass_parallel adapter gates 覆蓋。
- 驗證（2026-07-05 Phase 4 pass_parallel adapter gates）：新增 `pass_parallel.c` shared/main `ctx->gbuf`、worker-local `ws->local_gbuf` / `ws->local_edge_buf` 與 service-helper 裸 `gbuf` callpoint 收斂，並新增 adapter `InsertEdge` 不查端點的 contract test。重跑 `build/c/test-runner graph_buffer parallel edge_types_probe lang_contract`（156 passed）、`make -f Makefile.cbm lint-format`、`git diff --check`，且 direct-call search 確認 `src/pipeline/pass_parallel.c` 已無 `cbm_gbuf_upsert_node` / `cbm_gbuf_insert_edge`。
- 驗證（2026-07-05 Phase 4 language graph parity expansion）：新增 Rust、Java 與 C++ 小型 fixture，擴充 `language-graph-parity.json` 的 CALLS、DEFINES、DEFINES_METHOD、INHERITS/IMPLEMENTS、USAGE/WRITES 與 LSP strategy 觀測面。`make -f Makefile.cbm rust-language-graph-parity-update` 已確認 default C path 與 Rust pipeline/store opt-in path 完全一致並更新 golden；再以 `python3 scripts/rust-language-graph-parity.py target/rust-parity/cbm-default target/rust-parity/cbm-rust-pipeline` 非 update 驗證 golden 可重讀比對，並通過 `python3 -m py_compile scripts/rust-language-graph-parity.py` 與 `git diff --check`。
- 驗證（2026-07-07 Phase 4 route canonicalization opt-in）：新增 `pipeline::route` Rust helper、`cbm_rs_pipeline_route_canon_path_v1` FFI smoke 與 `CBM_USE_RUST_PIPELINE_ROUTE_CANON=1` 窄範圍 opt-in；只委派 `cbm_route_canon_path()` 的 route placeholder byte-level normalization，route extraction、HTTP/async call detection、Route node/edge 建立、graph-buffer mutation、language parser 與 pipeline side effects 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::route --locked`（3 passed）、`cargo test -p cbm-core --locked`（117 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner route_canon pipeline`（228 passed）、`make -f Makefile.cbm rust-pipeline-route-canon-optin-test`（228 passed）與 `make -f Makefile.cbm rust-language-graph-parity`（golden 一致）。新 target 已接入 `rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7；language graph parity 的 Rust pipeline binary 也同時啟用此 opt-in。
- 驗證（2026-07-07 Phase 4 pipeline test detection opt-in）：新增 `pipeline::test_detect` Rust helper、`cbm_rs_pipeline_is_test_path_v1` / `cbm_rs_pipeline_is_test_func_name_v1` / `cbm_rs_pipeline_test_to_prod_path_v1` FFI smoke 與 `CBM_USE_RUST_PIPELINE_TEST_DETECT=1` 窄範圍 opt-in；只委派 `cbm_is_test_path()` / `cbm_is_test_func_name()` 的 byte-level classifier 與 `test_to_prod_path()` 的 path rewrite，`TESTS` / `TESTS_FILE` edge creation、`node_is_test()`、graph-buffer adapter、tree-sitter/extraction 與 pipeline side effects 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::test_detect --locked`（4 passed）、`cargo test -p cbm-core --locked`（121 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm rust-pipeline-test-detect-optin-test`（217 passed）與 `make -f Makefile.cbm rust-language-graph-parity`（golden 一致；中斷後已補跑完成）。新 target 已接入 `rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7；language graph parity 的 Rust pipeline binary 也同時啟用此 opt-in。
- 驗證（2026-07-09 Phase 4 pipeline checked-exception classifier opt-in）：新增 `pipeline::exception` Rust helper、`cbm_rs_pipeline_is_checked_exception_v1` FFI smoke 與 `CBM_USE_RUST_PIPELINE_CHECKED_EXCEPTION=1` opt-in；只委派 `is_checked_exception()` 的 checked-exception classifier，空字串與一般 checked exception 名稱回 true，含 `Error` / `Panic` / `error` / `panic` 的名稱回 false，null 回 false。此切片只遷移 pure classifier；`THROWS` / `RAISES` edge type 選擇、exception resolution、graph-buffer adapter 與 pass side effects 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::exception --locked`（1 passed）、`cargo test -p cbm-core --locked`（182 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm rust-pipeline-checked-exception-optin-test`（269 passed）與 `make -f Makefile.cbm rust-language-graph-parity`（golden 一致；曾有一次並行跑共享 `build/c` 的 target 造成 clean-c race，後續序列重跑已通過）。
- 驗證（2026-07-10 Phase 4 pipeline artifact path opt-in）：新增 `pipeline::artifact::artifact_path()` Rust helper、`cbm_rs_pipeline_artifact_path_v1` FFI smoke 與 `CBM_USE_RUST_PIPELINE_ARTIFACT_PATH=1` opt-in；只委派 `<repo>/.codebase-memory/<name>` 的 byte-level path 組裝，null repo/name、null buf、`bufsize == 0` 與 short buffer 一律回 `SIZE_MAX`。`artifact` export/import、metadata、`.gitattributes` 與 file I/O 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::artifact --locked`（4 passed）、`cargo test -p cbm-core --locked`（186 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner artifact`（10 passed）、`make -f Makefile.cbm rust-pipeline-artifact-path-optin-test`（10 passed）與 `git diff --check`。
- 驗證（2026-07-10 Phase 4 pipeline configures / trace / progress sink targeted gates）：新增 `pipeline::configures` Rust helper、`cbm_rs_pipeline_is_env_var_name_v1` / `cbm_rs_pipeline_normalize_config_key_v1` / `cbm_rs_pipeline_has_config_extension_v1` FFI smoke 與 `CBM_USE_RUST_PIPELINE_CONFIGURES=1` opt-in；通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::configures --locked`（3 passed）、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm rust-pipeline-configures-optin-test`（217 passed）、`make -f Makefile.cbm rust-traces-optin-test`（30 passed）、`make -f Makefile.cbm rust-cli-progress-sink-optin-test`（2 passed）與 `git diff --check`。三個 target 都會重建 `build/c`，續跑時需序列執行。
- 驗證（2026-07-10 Phase 4 pipeline infrascan filename classifier opt-in）：新增 `pipeline::infrascan` Rust helper、`cbm_rs_pipeline_is_dockerfile_v1` / `cbm_rs_pipeline_is_compose_file_v1` / `cbm_rs_pipeline_is_env_file_v1` / `cbm_rs_pipeline_is_cloudbuild_file_v1` / `cbm_rs_pipeline_is_kustomize_file_v1` / `cbm_rs_pipeline_is_shell_script_v1` FFI smoke 與 `CBM_USE_RUST_PIPELINE_INFRASCAN=1` opt-in；只委派檔名/副檔名 byte-level classifier，k8s manifest content 判斷、secret detection、Dockerfile/.env/shell/Terraform/Helm parsing、graph mutation 與 pipeline side effects 仍留 C。補上 255-byte C truncation parity unit test，並在 opt-in 編譯時排除不再使用的 C `to_lower()` helper。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core --locked`（203 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner pipeline`（217 passed）、`make -f Makefile.cbm rust-pipeline-infrascan-optin-test`（217 passed）與 `git diff --check`。
- 驗證（2026-07-10 Phase 4 pipeline K8s dependency-manifest classifier opt-in）：新增 `pipeline::infrascan::{is_helm_chart_file,is_gomod_file,is_requirements_file}`、三個 `cbm_rs_pipeline_is_*_v1` FFI smoke 與 `CBM_USE_RUST_PIPELINE_K8S_FILE_CLASSIFIERS=1` opt-in；只委派 `Chart.yaml` / `Chart.yml`、`go.mod` 與 `requirements.txt` 的純檔名分流。檔案讀取、Helm／Go／Python dependency parser、Package／Chart node 和 `DEPENDS_ON` edge 寫入、graph-buffer ownership 與 pipeline side effects 仍留 C。通過 `cargo test -p cbm-core pipeline::infrascan --locked`（8 passed）、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner pipeline`（219 passed）、`make -f Makefile.cbm rust-pipeline-k8s-file-classifiers-optin-test`（219 passed）、`make -f Makefile.cbm rust-language-graph-parity`（default 與 Rust pipeline binary 均與 golden 一致）與 `git diff --check`。
- 驗證（2026-07-10 Phase 4 pipeline githistory trackable-file classifier opt-in）：新增 `pipeline::githistory` Rust helper、`cbm_rs_pipeline_is_trackable_file_v1` FFI smoke 與 `CBM_USE_RUST_PIPELINE_GITHISTORY=1` opt-in；只委派路徑前綴、lock/generated basename 與非 source suffix classifier，git log、coupling 計算、`FILE_CHANGES_WITH` edge、graph mutation 與 I/O 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::githistory --locked`（4 passed）、`cargo test -p cbm-core --locked`（207 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner pipeline`（217 passed）、`make -f Makefile.cbm rust-pipeline-githistory-optin-test`（217 passed）與 `git diff --check`。
- 驗證（2026-07-10 Phase 4 pipeline gitdiff range parser opt-in）：新增 `pipeline::gitdiff::parse_range()`、`cbm_rs_pipeline_parse_range_v1` FFI smoke 與 `CBM_USE_RUST_PIPELINE_GITDIFF_RANGE=1` opt-in；只委派 unified diff `start,count` / `start` scalar parser，name-status/hunk 結構、固定 C buffer、git diff I/O、graph mutation 與 pipeline side effects 仍留 C。ABI 對 NULL/無效輸入提供 `(0,1)` 安全 fallback，並固定省略 count 預設 1、零 count、正負整數與 caller-owned output。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::gitdiff --locked`（4 passed）、`cargo test -p cbm-core --locked`（211 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner pipeline`（217 passed）、`make -f Makefile.cbm rust-pipeline-gitdiff-range-optin-test`（217 passed）、`bash -n scripts/rust-check.sh scripts/test.sh` 與 `git diff --check`。
- 驗證（2026-07-10 Phase 4 pipeline gitdiff name-status parser opt-in）：新增 `pipeline::gitdiff::parse_name_status()`、`cbm_rs_pipeline_parse_name_status_v1` FFI smoke 與 `CBM_USE_RUST_PIPELINE_GITDIFF_NAME_STATUS=1` opt-in；只委派 name-status 行解析、rename path、trackable filter 與 caller-owned fixed-array 輸出，hunk parser、git diff I/O、graph mutation 與 pipeline side effects 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::gitdiff --locked`（6 passed）、`cargo test -p cbm-core --locked`（213 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner pipeline`（217 passed）、`make -f Makefile.cbm rust-pipeline-gitdiff-name-status-optin-test`（217 passed）、`bash -n scripts/rust-check.sh scripts/test.sh` 與 `git diff --check`。
- 驗證（2026-07-10 Phase 4 pipeline gitdiff hunk parser opt-in）：新增 `pipeline::gitdiff::parse_hunks()`、`cbm_rs_pipeline_parse_hunks_v1` FFI smoke 與 `CBM_USE_RUST_PIPELINE_GITDIFF_HUNKS=1` opt-in；只委派 current-file tracking、hunk range parser、trackable filter 與 caller-owned fixed-array 輸出，git diff I/O、hunk 後續消費、graph mutation 與 pipeline side effects 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::gitdiff --locked`（8 passed）、`cargo test -p cbm-core --locked`（215 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner pipeline`（217 passed）、`make -f Makefile.cbm rust-pipeline-gitdiff-hunks-optin-test`（217 passed）、`bash -n scripts/rust-check.sh scripts/test.sh` 與 `git diff --check`。
- 驗證（2026-07-10 Phase 4 pipeline module-directory classifier opt-in）：新增 `pipeline::module::is_module_dir()`、`cbm_rs_pipeline_is_module_dir_v1` FFI smoke 與 `CBM_USE_RUST_PIPELINE_MODULE_DIR=1` opt-in；五個 pipeline pass 僅委派 Go/Java module-directory 判定，module QN 組裝、解析、graph mutation 與 pipeline side effects 仍留 C。通過 `cargo test -p cbm-core pipeline::module --locked`（1 passed）、完整 Rust（216 passed）、clippy、FFI smoke、default/opt-in `build/c/test-runner pipeline` 各 217 passed、`cargo fmt --all -- --check`、shell syntax 與 `git diff --check`。
- 驗證（2026-07-10 Phase 4 pipeline route argument helpers opt-in）：新增 `pipeline::route` 的 path keyword、junk URL 與 template URL normalization helpers、`cbm_rs_pipeline_is_path_keyword_v1` / `cbm_rs_pipeline_normalize_url_arg_v1` FFI smoke 與 `CBM_USE_RUST_PIPELINE_ROUTE_ARGS=1` opt-in；只委派 route argument pure helper，route scanning、handler resolution、graph mutation 與 pipeline side effects 仍留 C。通過 targeted route 6 passed、完整 Rust 219 passed、clippy、FFI smoke、default/opt-in `build/c/test-runner pipeline` 各 217 passed、fmt、shell syntax 與 diff check。
- 驗證（2026-07-10 Phase 4 pipeline route-node classifiers opt-in）：新增 `pipeline::route` 的 hash segment 與 broker-route classifiers、`cbm_rs_pipeline_is_hash_segment_v1` / `cbm_rs_pipeline_is_broker_route_v1` FFI smoke 與 `CBM_USE_RUST_PIPELINE_ROUTE_NODE_CLASSIFIERS=1` opt-in；只委派 route-node pure classifier，Cloud Run URL extraction、suffix stripping、Route node/edge、graph mutation 與 pipeline side effects 仍留 C。通過 targeted route 7 passed、完整 Rust 220 passed、clippy、FFI smoke、default/opt-in `build/c/test-runner pipeline` 各 217 passed、fmt、shell syntax 與 diff check。`CBM_USE_RUST_PIPELINE_ROUTE_NODE_CLASSIFIERS_ONLY=1` direct-only gate 也同步接入，啟用 `pipeline-route-node-classifiers-only` Cargo feature。
- CI 接入（2026-07-02）：新增 `.github/workflows/_rust.yml` 專責 Rust gates（lint / unit / FFI / foundation 與 pipeline registry+plan opt-in / language-graph parity），並於 `pr.yml` 納入 `rust` job 與 `ci-ok` 必要脈絡；`_security.yml` 既有的 `rust-dangerous-api` 與 `cargo-audit` fail-closed 維持不變。新增 `rust-ci-tests` make 目標作為 CI 用的測試/parity 子集。
- 剩餘範圍（刻意保留）：Phase 4 的 pipeline orchestration 全面遷移與 Phase 5 的「切換預設為 Rust-backed」尚未執行——Rust 端目前承接 registry/plan 決策層與 graph-buffer mutation command contract，C 端已有多批同步 command adapter callpoint；實際 pass/extraction/tree-sitter/LSP 與 graph-buffer storage/ownership 仍在 C；提前切換會破壞產品並違反本計畫「C fallback 需經至少一個 release cycle」的完成標準，詳見 `Tasks.md`。
    - 下一批 foundation 評估：`platform` env/dirs 已完成 Rust ABI、C opt-in 與 parity fixture。`hash_table` 已補 C contract/stress tests，但因 borrowed key pointer、`get_key` stored pointer、NULL value 與 foreach callback contract，目前標記為 C-only hot path；若未來試作 Rust 版，先走 test-only `cbm_rs_ht_*` parity fixture。`diagnostics` 已補 deterministic query stats、env parser、path、JSON 與 NDJSON 的 Rust parity/FFI fixture；writer thread、檔案 rotation、system metrics 與 stderr lifecycle 仍留 C。`hash_table` 已補 test-only `cbm_rs_ht_*` parity fixture（`foundation::hash_table` 純安全 Rust module + FFI + `tests/test_rust_ffi.c` parity），固定 borrowed key pointer、`get_key` stored pointer、NULL value 與 foreach callback contract；仍維持 C-only hot path，不導入產品 opt-in。YAML 已將既有 C contract suite 納入 `test-foundation` 與 `rust-foundation-optin-test` matrix，並新增 `foundation::yaml` test-only Rust parity module 與 `cbm_rs_yaml_*` FFI smoke，固定 minimal YAML parser/query contract；仍不導入產品 opt-in。profile env gate 已新增 `CBM_USE_RUST_PROFILE_ENV=1`，只委派 `CBM_PROFILE` 啟用判斷；profile clock、global state 與 log emission 仍留 C。Phase 3 的 store/cypher/MCP 已整理於 `docs/rust-refactor-phase3-plan.md`；`store_compat` contract fixture 已凍結 minimal schema/index existence、`edges.url_path_gen`、user index drop/create symmetry、`open_path_query` no-create/read-only、WAL journal mode、readonly WAL read/write rejection、bulk begin/end pragma 不離開 WAL、generated column query plan、URL path API 的 project-scoped substring 行為，以及 `nodes_fts` 手動 rebuild/camelCase tokenization；同時修正 `idx_edges_url_path` 未被 drop 的不對稱。Store 已新增 Rust static `schema_manifest` test-only helper 與 V1/V2 schema FFI smoke，固定核心表、`nodes_fts`、`edges.url_path_gen`、9 個 user index 的名稱/表/欄位/旗標、table column/index layout 與 FTS shadow metadata；另新增 test-only connection/pragma manifest FFI，固定 `open_path_query` no-create/read-only、immutable fallback、WAL、mmap env 與 bulk pragma contract；FTS tokenizer、mmap resolver、immutable URI builder、search pattern helper 與 architecture helper 已有窄範圍 opt-in，前四者分別只委派 token 字串、env 字串解析、readonly fallback URI 字串建構與 glob/LIKE/case flag byte-level helper；architecture helper 固定 qn_to_package/qn_to_top_package、is_test_file_path、hop_to_risk 與 risk_label 契約。SQLite open/probe/pragma/FTS/CRUD/search runtime 仍留 C。`cypher_contract` 已擴充 CALL、EXISTS、malformed 與 aggregation exact contract，包含 `CALL db.labels()`、`EXISTS { ... }`、`EXISTS (f)` 與 `COUNT(SUM/AVG/MIN/MAX)` 輸出語意；同時保留既有 OPTIONAL MATCH、UNION/UNION ALL、edge property、deterministic ordered rows、WITH/post-WHERE AST 與 explicit `LIMIT 0` 契約。Cypher 已新增 Rust lexer/token summary 與 normalized AST summary test-only parity FFI，固定 token metadata、parser AST shape、OPTIONAL MATCH、UNION ALL、EXISTS direction、WITH/post-WHERE 與 caller-buffer summary contract；production parser、AST normalizer、evaluator、executor 與 production opt-in 仍未遷移。`mcp` suite 已凍結 BM25 `file_pattern` 與 label boost rank/order contract；MCP golden 已凍結 alias、tool schema 與 indexed repo `query_graph` transcript；request parse、tools/call name/arguments/default arguments、tool dispatch classifier、missing-tool-name error text、unknown-tool error text、tools/list cursor/page bounds、tool name index/name/title/description/inputSchema/outputSchema manifest、tools/list result JSON builder、Content-Length header classifier/parser/header separator/response framing、file URI parser、method dispatch classifier、method-not-found error object、ping result object、cancel matcher、initialize protocol version、initialize response builder、numeric-id error formatter、JSON-RPC response formatter 與 MCP text result formatter 已可用 `CBM_USE_RUST_MCP_CODEC=1` 窄範圍委派 Rust，Content-Length body allocation/read、JSON-RPC wrapper、server lifecycle 與 handlers 仍留 C。
- 驗證（2026-07-05 Phase 3 Store schema manifest helper）：新增 `rust/cbm-core/src/store/schema_manifest.rs` 與 `cbm_rs_store_schema_manifest_*_v1` test-only FFI；已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core store::schema_manifest --locked`（2 passed）、`cargo test -p cbm-core --locked`（53 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm lint-format`、`make -f Makefile.cbm rust-ffi-test` 與 `build/c/test-runner store_compat`（8 passed）。此批只凍結 manifest/ABI contract，尚未遷移 SQLite open/pragma/CRUD/runtime。
- 驗證（2026-07-05 Phase 3 Store connection/pragma manifest helper）：新增 test-only `cbm_rs_store_connection_manifest_*_v1` FFI，並補 `store_compat_bulk_pragmas_preserve_wal_contract`。本輪通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core store::schema_manifest --locked`（4 passed）、`make -f Makefile.cbm rust-ffi-test` 與 `make -f Makefile.cbm build/c/test-runner && build/c/test-runner store_compat`（11 passed）。此批只凍結 open/pragma contract，尚未遷移 SQLite open/pragma runtime。
- 驗證（2026-07-07 Phase 3 Store checkpoint/bulk filtered manifest）：擴充 `schema_manifest` connection manifest，新增 `CONNECTION_MODE_CHECKPOINT`、public checkpoint `SQLITE_CHECKPOINT_PASSIVE` / `PRAGMA optimize;` metadata，以及 mode-filtered FFI count/entries helper；read-only filtered plan 固定不含 write pragma，bulk begin/end 與 checkpoint 順序可由 C smoke 驗證。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core store::schema_manifest --locked`（5 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test` 與 `make -f Makefile.cbm build/c/test-runner && build/c/test-runner store_compat`（15 passed）。此批仍為 test-only manifest/filter contract，不開 SQLite、不執行 pragma、不接 production opt-in。
- 驗證（2026-07-05 Phase 3 Store FTS camelCase tokenization helper）：新增 `store::tokenization` Rust helper 與 `cbm_rs_store_camel_split_v1` FFI smoke，固定 `cbm_camel_split` 的原 identifier + split identifier、ASCII lower→upper、acronym boundary、NULL/empty、非 ASCII byte 不觸發大小寫邊界、2048-byte fallback/truncation 與 caller-buffer truncation contract。`CBM_USE_RUST_STORE_FTS_TOKENIZER=1` 只讓 C 註冊的 SQLite scalar UDF callback 將 token 字串計算委派給 Rust；SQLite open/pragma/schema、UDF registration、FTS5 runtime、`sqlite3_result_text`、CRUD/search/BM25 與 Store ownership 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core store::tokenization --locked`（3 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm build/c/test-runner && build/c/test-runner store_compat`（12 passed，含 direct scalar UDF exact contract）與 `make -f Makefile.cbm rust-store-fts-tokenizer-optin-test`（`store_compat mcp` 131 passed）。新 target 已接入 `rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7；language graph parity 的 Rust opt-in binary 也同時帶 `CBM_USE_RUST_STORE_FTS_TOKENIZER=1` 作為組合 smoke。
- 驗證（2026-07-06 Phase 3 Store mmap resolver opt-in）：新增 `store::pragmas` Rust helper、`cbm_rs_store_resolve_mmap_size_value_v1` FFI smoke 與 `CBM_USE_RUST_STORE_MMAP_RESOLVER=1` 窄範圍 opt-in；只委派 `CBM_SQLITE_MMAP_SIZE` 字串解析，SQLite open、pragma dispatch、readonly/write 分流與 SQL execution 仍留 C。同時補 C resolver `errno == ERANGE` contract，固定 unset/empty/malformed/partial/trailing/overflow 回 64 MiB、合法負值 clamp 0、leading whitespace 與 `+` 可接受。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core store::pragmas --locked`（1 passed）、`cargo test -p cbm-core --locked`（92 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm build/c/test-runner && build/c/test-runner store_pragmas store_compat`（23 passed）與 `make -f Makefile.cbm rust-store-mmap-resolver-optin-test`（23 passed）。新 target 已接入 `rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7。
- 驗證（2026-07-06 Phase 3 Store immutable URI opt-in）：新增 `store::open` Rust helper、`cbm_rs_store_build_immutable_uri_v1` FFI smoke 與 `CBM_USE_RUST_STORE_IMMUTABLE_URI=1` 窄範圍 opt-in；只委派 SQLite readonly immutable fallback URI 字串建構，SQLite open/probe、readonly no-create、WAL/pragma dispatch、UDF/FTS/search/CRUD 與 Store lifetime 仍留 C。FFI smoke 固定 POSIX/relative/root/Windows/UNC path、安全字元、space、`%`、`?`、`#`、`&`、`=`、UTF-8 raw bytes、NULL、length-only 與短 buffer 截斷。通過 `cargo test -p cbm-core store::open --locked`（3 passed）、`make -f Makefile.cbm rust-ffi-test` 與 `make -f Makefile.cbm rust-store-immutable-uri-optin-test`（`store_compat mcp` 131 passed）。新 target 已接入 `rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7。
- 驗證（2026-07-06 Phase 3 Store search pattern opt-in）：新增 `store::search_pattern` Rust helper、glob/case flag/LIKE hint FFI smoke 與 `CBM_USE_RUST_STORE_SEARCH_PATTERN=1` 窄範圍 opt-in；只委派 `cbm_glob_to_like()`、`cbm_extract_like_hints()`、`cbm_ensure_case_insensitive()` 與 `cbm_strip_case_flag()` 的 byte-level helper，SQL builder、LIKE bind lifetime、REGEXP UDF、FTS/BM25、SQLite statement lifecycle 與 Store runtime 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core store::search_pattern --locked`（4 passed）、`cargo test -p cbm-core --locked`（102 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm build/c/test-runner && build/c/test-runner store_search mcp`（191 passed）、`make -f Makefile.cbm rust-store-search-pattern-optin-test`（191 passed）與 `make -f Makefile.cbm lint-format`。新 target 已接入 `rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7。
- 部分重驗證（2026-07-07 Phase 3 Store checkpoint golden 與 opt-in wiring）：`store_compat` 新增 `cbm_store_checkpoint()` WAL side-effect golden，固定 checkpoint 後 `-wal` 檔仍存在且非空；同時修正 `CBM_USE_RUST_STORE_ARCH_PATH_SCOPE=1` 與 `CBM_USE_RUST_STORE_FILE_EXT=1` 的 Makefile 接線，讓既有 opt-in targets 會真正定義 C macro 並連入 Rust staticlib。本輪已重跑 default path `build/c/test-runner store_compat`（15 passed）；Store arch path/file_ext opt-in targets 的最近紀錄見本文件工作階段與 `Tasks.md`。
- 驗證（2026-07-05 Foundation YAML matrix gate）：`tests/test_yaml.c` 已納入 `test-foundation` source list 與 runner；`make -f Makefile.cbm test-foundation` 目前為 314 passed，`make -f Makefile.cbm rust-foundation-optin-test` 亦通過，讓既有 Rust foundation opt-in matrix 同時覆蓋 YAML C contract。
- 驗證（2026-07-05 Phase 3 Cypher contract 補洞）：`make -f Makefile.cbm build/c/test-runner && build/c/test-runner cypher_contract`（20 passed）。
- 驗證（2026-07-06 Phase 3 Cypher lexer parity）：新增 `rust/cbm-core/src/cypher/mod.rs`、`cbm_rs_cypher_lex_*_v1` test-only FFI 與 `tests/test_rust_ffi.c` smoke，固定 token metadata、summary truncation、string/comment/number/DOTDOT contract 與 C enum ABI。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（3 passed）、`cargo test -p cbm-core --locked`（80 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner cypher_contract`（20 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。同日 targeted pipeline 驗證通過：`build/c/test-runner pipeline graph_buffer lang_contract edge_structural`（332 passed）、`make -f Makefile.cbm rust-pipeline-plan-optin-test`（217 passed）與 `make -f Makefile.cbm rust-language-graph-parity`；共享 `build/c` 的 `clean-c` Make 目標需序列執行，不能並行。
- 驗證（2026-07-06 Phase 3 Cypher AST summary parity）：新增 `cbm_rs_cypher_parse_summary_v1` test-only FFI 與 Rust normalized AST summary helper，固定 parser AST shape、OPTIONAL MATCH optional flags、UNION ALL linked shape、EXISTS inbound direction、WITH/post-WHERE 與 malformed parse failure contract。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（6 passed）、`cargo test -p cbm-core --locked`（83 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner cypher_contract`（20 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。此批不接 production opt-in，C parser/evaluator/executor 仍是產品路徑。
- 驗證（2026-07-06 Phase 3 MCP JSON-RPC request summary parity）：新增 `rust/cbm-core/src/mcp/mod.rs`、`cbm_rs_mcp_jsonrpc_request_summary_v1` test-only FFI 與 `tests/test_rust_ffi.c` smoke，固定 C JSON-RPC parser 的 `jsonrpc` 預設值、string/numeric/other id 分類、notification 無 id、`params` kind、invalid JSON/root/missing method、whitespace 與 string escape summary contract。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（4 passed）、`cargo test -p cbm-core --locked`（90 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner mcp`（119 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- 驗證（2026-07-06 Phase 3 MCP JSON-RPC parse codec opt-in）：擴充 MCP Rust parser 與新增 `cbm_rs_mcp_jsonrpc_parse_v1`，並以 `CBM_USE_RUST_MCP_CODEC=1` 讓 `cbm_jsonrpc_parse()` 可窄範圍委派 Rust request envelope parse；C wrapper 仍配置/釋放 `cbm_jsonrpc_request_t` 欄位，response formatting、Content-Length transport、tool schema、tool dispatch/handlers、stdout/stderr 與 server lifecycle 仍留 C。新增 contract 固定 duplicate key first-wins、Unicode escape、invalid UTF-8 rejection、numeric/string/other id、notification 與 `params` raw value slice。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（6 passed）、`cargo test -p cbm-core --locked`（98 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm build/c/test-runner && build/c/test-runner mcp`（123 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（123 passed）、`make -f Makefile.cbm CBM_USE_RUST_MCP_CODEC=1 build/c/test-runner && build/c/test-runner mcp`（123 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。新 target 已接入 `rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7。
- 驗證（2026-07-07 Phase 3 MCP tools/call name codec opt-in）：新增 `cbm_rs_mcp_tools_call_name_v1` 與 `mcp::tools_call_name`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_get_tool_name()` 只把 `tools/call` params.name 擷取委派給 Rust；C wrapper 仍負責 `malloc`/`free` 的 C-owned NUL 字串，`arguments`、handler-specific 參數、tool schema、response formatting 與 Content-Length transport 仍留 C/yyjson。本輪重跑 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（9 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（124 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（124 passed）。
- 驗證（2026-07-07 Phase 3 MCP cancelled matcher codec opt-in）：新增 `cbm_rs_mcp_cancel_request_matches_v1` 與 `mcp::cancel_request_matches`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_cancel_request_matches()` 只把 `notifications/cancelled` params.requestId matching 委派給 Rust；C default contract 與 FFI smoke 覆蓋 null/invalid/root 非 object、numeric/string id 分流、duplicate first-wins、Unicode escape、float/bool/type mismatch 與 invalid UTF-8。server lifecycle、pipeline cancellation side effect、logging、response formatting 與 framing 仍留 C。本輪重跑同一組 MCP/Rust targeted gates，`rust-mcp-codec-optin-test` 為 124 passed。
- 驗證（2026-07-07 Phase 3 MCP tools/call arguments codec opt-in）：新增 `cbm_rs_mcp_tools_call_arguments_v1` 與 `mcp::tools_call_arguments`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_get_arguments()` 擷取首個 `arguments` JSON value；缺少 `arguments` 時回傳 `{}`，存在時保留原始 JSON slice，不做 canonicalize。C wrapper 仍負責 C-owned NUL 字串；handler-specific 參數解析、validation、tool schema、response formatting、Content-Length transport 與 handlers 仍留 C/yyjson。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（10 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（124 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（124 passed）。
- 驗證（2026-07-07 Phase 3 MCP initialize protocol version codec opt-in）：新增 `cbm_rs_mcp_initialize_protocol_version_v1` 與 `mcp::initialize_protocol_version`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_initialize_response()` 只把 initialize params 的 `protocolVersion` selection 委派 Rust；supported version exact match、duplicate first-wins、unknown/missing/non-string/invalid fallback latest contract 已固定。initialize response JSON 組裝、`serverInfo`、capabilities、yyjson serialization/ownership、JSON-RPC dispatch、Content-Length transport 與 handlers 仍留 C。通過 `cargo fmt --all`、`cargo test -p cbm-core mcp --locked`（11 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（124 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（124 passed）。
- 驗證（2026-07-07 Phase 3 MCP JSON-RPC error formatter codec opt-in）：新增 `cbm_rs_mcp_jsonrpc_format_error_v1` 與 `mcp::jsonrpc_format_error`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_jsonrpc_format_error()` 的 numeric-id error response 委派 Rust；固定 compact JSON 欄位順序、code/id 數值、message escape、NULL message 空字串、length-only 與短 buffer contract。string-id response、result embedding、MCP text result、Content-Length transport、tool schema、dispatch 與 handlers 仍留 C/yyjson。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（12 passed）、`cargo test -p cbm-core --locked`（123 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（124 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（124 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- 驗證（2026-07-07 Phase 3 MCP JSON-RPC response formatter codec opt-in）：新增 `cbm_rs_mcp_jsonrpc_format_response_v1` 與 `mcp::jsonrpc_format_response`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_jsonrpc_format_response()` 委派 Rust；固定 numeric/string id、string id escaping、result embedding、error 優先、missing result/error 時 `result:null`、invalid embedded JSON 時只輸出 `jsonrpc/id`、length-only 與短 buffer contract。Content-Length transport、server loop、MCP text result、tool schema、dispatch 與 handlers 仍留 C/yyjson。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（13 passed）、`cargo test -p cbm-core --locked`（124 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（124 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（124 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- 驗證（2026-07-07 Phase 3 MCP text result formatter codec opt-in）：新增 `cbm_rs_mcp_text_result_v1` 與 `mcp::mcp_text_result`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_text_result()` 委派 Rust；固定 `content[0].type/text`、`isError`、非 error JSON object 才加入 minified `structuredContent`、plain text/array/null/error 不加入 structuredContent、escaping、length-only 與短 buffer contract。Content-Length transport、server loop、tool schema、dispatch 與 handlers 仍留 C/yyjson。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（14 passed）、`cargo test -p cbm-core --locked`（125 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（124 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（124 passed）。
- 驗證（2026-07-07 Phase 3 MCP tool index codec opt-in）：新增 `cbm_rs_mcp_tool_index_v1` 與 `mcp::tool_index`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_tool_input_schema()` 的 name -> `TOOLS[]` index 查找委派 Rust；C 仍持有 input schema JSON、title、description、tools/list serialization、dispatch 與 handlers。固定 14 tools 順序、case-sensitive lookup、null/empty/unknown 回 -1，以及 C-side schema lookup contract。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（15 passed）、`cargo test -p cbm-core --locked`（126 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）。
- 驗證（2026-07-07 Phase 3 MCP tools/list page bounds codec opt-in）：新增 `cbm_rs_mcp_tools_page_bounds_v1` 與 `mcp::tools_page_bounds`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_tools_list_range()` 的 offset/limit clamp、end 與 nextCursor decision 委派 Rust；C 仍持有 tool metadata、input/output schema JSON、tools/list serialization、JSON-RPC response wrapping、dispatch 與 handlers。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（16 passed）、`cargo test -p cbm-core --locked`（127 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）。
- 驗證（2026-07-07 Phase 3 MCP tools/list tool name manifest opt-in）：新增 `cbm_rs_mcp_tool_count_v1` / `cbm_rs_mcp_tool_name_v1` 與 `mcp::tool_count` / `mcp::tool_name`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `tools/list` 的 `name` 欄位於 Rust count/name 與本地 C `TOOLS[]` 完全一致時由 Rust 輸出；C 仍持有 title、description、input/output schema JSON、JSON 組裝、dispatch 與 handlers。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（17 passed）、`cargo test -p cbm-core --locked`（128 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- 驗證（2026-07-07 Phase 3 MCP tools/list tool title manifest opt-in）：新增 `cbm_rs_mcp_tool_title_v1` 與 `mcp::tool_title`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `tools/list` 的 `title` 欄位於 Rust count/name/title 與本地 C `TOOLS[]` 完全一致時由 Rust 輸出；C 仍持有 description、input/output schema JSON、JSON 組裝、dispatch 與 handlers。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（18 passed）、`cargo test -p cbm-core --locked`（129 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- 驗證（2026-07-08 Phase 3 MCP tools/list tool inputSchema manifest opt-in）：新增 `cbm_rs_mcp_tool_input_schema_v1` 與 `mcp::tool_input_schema`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `tools/list` 的 `inputSchema` 欄位於 Rust count/name/title/description/inputSchema 與本地 `TOOLS[]` 完全一致時由 Rust 輸出；C 仍負責 schema JSON parse/copy/serialization、outputSchema、dispatch、handlers 與 transport。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（20 passed）、`cargo test -p cbm-core --locked`（131 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- 驗證（2026-07-08 Phase 3 MCP tools/list tool outputSchema manifest opt-in）：新增 `cbm_rs_mcp_tool_output_schema_v1` 與 `mcp::tool_output_schema`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `tools/list` 的共用 `outputSchema` 欄位於 Rust 與本地 C 常數完全一致時由 Rust 輸出；C 仍負責 schema JSON parse/copy/serialization、dispatch、handlers 與 transport。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（21 passed）、`cargo test -p cbm-core --locked`（132 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- 驗證（2026-07-08 Phase 3 MCP tools/list result JSON builder opt-in）：新增 `cbm_rs_mcp_tools_list_json_v1` 與 `mcp::tools_list_json`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_tools_list_range()` 可直接使用 Rust 產生的 compact result object；C 仍負責 JSON-RPC response wrapper、Content-Length transport、server loop、dispatch 與 handlers。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（22 passed）、`cargo test -p cbm-core --locked`（133 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- 驗證（2026-07-08 Phase 3 MCP Content-Length header parser opt-in）：新增 `cbm_rs_mcp_content_length_v1` 與 `mcp::content_length_value`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_server_run()` 的 Content-Length header length gate 委派 Rust；C 仍負責 header/body I/O、response framing、poll loop、idle eviction、server lifecycle、dispatch 與 handlers。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（23 passed）、`cargo test -p cbm-core --locked`（134 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- 驗證（2026-07-08 Phase 3 MCP Content-Length header separator opt-in）：新增 `cbm_rs_mcp_content_length_header_is_blank_v1` 與 `mcp::content_length_header_is_blank`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `handle_content_length_frame()` 的 header/body 空行 separator 判斷委派 Rust；C 仍負責 `getline`、body allocation/read、呼叫 `cbm_mcp_server_handle()`、poll loop、idle eviction、server lifecycle、dispatch 與 handlers。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（25 passed）、`cargo test -p cbm-core --locked`（136 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- 驗證（2026-07-08 Phase 3 MCP file URI parser opt-in）：新增 `cbm_rs_mcp_parse_file_uri_v1` 與 `mcp::parse_file_uri`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_parse_file_uri()` 的 pure file URI path extraction 委派 Rust；C 仍負責 caller buffer ownership、錯誤時清空輸出、root/session lifecycle 與 URI 使用端。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（26 passed）、`cargo test -p cbm-core --locked`（137 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- 驗證（2026-07-08 Phase 3 MCP Content-Length response framing opt-in）：新增 `cbm_rs_mcp_content_length_response_v1` 與 `mcp::content_length_response`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `handle_content_length_frame()` 可把 response JSON 委派 Rust 包成 `Content-Length: <byte_len>\r\n\r\n...` frame；C 仍負責 header/body I/O、呼叫 `cbm_mcp_server_handle()`、poll loop、idle eviction、server lifecycle、dispatch 與 handlers。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（24 passed）、`cargo test -p cbm-core --locked`（135 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- 驗證（2026-07-09 Phase 3 MCP string argument extraction opt-in）：新增 `cbm_rs_mcp_get_string_arg_v1` 與 `mcp::string_arg`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_get_string_arg()` 優先把 handler args JSON 的 root string field 擷取委派 Rust；固定 root object only、first matching key、missing/non-string/invalid JSON/invalid UTF-8 無值、length-only 與短 buffer contract。C wrapper 只在 Rust 成功時配置 C-owned NUL 字串，否則 fallback 到原 yyjson helper；int/bool arg helpers、project alias resolution、handler-specific validation、response formatting、Content-Length transport、server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（37 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（125 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）。
- 驗證（2026-07-09 Phase 3 MCP int/bool argument extraction opt-in）：新增 `cbm_rs_mcp_get_int_arg_v1` / `cbm_rs_mcp_get_bool_arg_v1` 與 `mcp::int_arg` / `mcp::bool_arg`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `cbm_mcp_get_int_arg()` / `cbm_mcp_get_bool_arg()` 優先把 handler args JSON 的 root int/bool field 擷取委派 Rust；固定 root object only、first matching key、missing/non-matching type/invalid JSON/invalid UTF-8 default/false contract。C wrapper 仍保留預設 C path；project alias resolution、handler-specific range/default validation、response formatting、Content-Length transport、server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（38 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（125 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）。
- 驗證（2026-07-09 Phase 3 MCP relationship edge-type validator opt-in）：新增 `cbm_rs_mcp_edge_type_valid_v1` 與 `mcp::edge_type_valid`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `validate_edge_type()` 優先把 `search_graph.relationship` edge type 驗證委派 Rust；固定只允許 ASCII 大寫字母與 underscore、長度最多 64 bytes、null/過長/其他 byte 失敗，空字串暫與 C helper 對齊為有效。C end-to-end 測試改用有效 `test-project` fixture，避免在 project/store validation 前提前返回。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（39 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（126 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（126 passed）。`search_graph` 參數解析、store query/search、response formatting、Content-Length transport、server lifecycle 與 handlers 仍留 C。
- 驗證（2026-07-09 Phase 3 MCP search_code path/file_pattern validator opt-in）：新增 `cbm_rs_mcp_search_path_arg_valid_v1` 與 `mcp::search_path_arg_valid`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `validate_search_path_arg()` 優先把 `search_code` root path / `file_pattern` shell-safety denylist 委派 Rust；固定 quote、分號、pipe、`$`、backtick、尖括號、CR/LF 與非 Windows backslash 失敗，`&` 維持合法。新增 end-to-end regression 固定非法 `file_pattern` 會回 `path or file_pattern contains invalid characters`。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（40 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（127 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（127 passed）。project root resolution、grep command construction、temp file/regex/search execution、result parsing、response/transport/server lifecycle 與 handlers 仍留 C。
- 驗證（2026-07-09 Phase 3 MCP search_code strip root prefix opt-in）：新增 `cbm_rs_mcp_strip_root_prefix_offset_v1` 與 `mcp::search_strip_root_prefix_offset`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `strip_root_prefix()` 只把 grep result path 的 root-prefix offset 計算委派 Rust；固定 byte prefix contract、不檢查 path component boundary、exact root 回 `root_len`、`root_len == 0` 會跳過 leading `/`，以及 `root_len > path/root length` 回 0 的防禦式 ABI 行為。此切片只遷移 borrowed offset 計算；C 仍從原 `path` 借用指標，grep execution、path filter、result parsing、JSON/transport、server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（57 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- 驗證（2026-07-09 Phase 3 MCP missing project error object opt-in）：新增 `cbm_rs_mcp_missing_project_error_v1` 與 `mcp::missing_project_error`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `build_missing_project_error()` 的固定 JSON error object 委派 Rust；固定 `missing required argument: project` error 與指向 `list_projects` / `project` argument 的 hint 字串、length-only 與短 buffer NUL 截斷 contract。此切片只遷移 missing-project 固定 message builder；available project list 掃描、`build_project_list_error()`、Store resolution、`cbm_mcp_text_result()`、JSON-RPC wrapper、transport、server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（58 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- 驗證（2026-07-09 Phase 3 MCP project-not-found message opt-in）：新增 `cbm_rs_mcp_project_not_found_message_v1` 與 `mcp::project_not_found_message`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `build_project_not_found_message()` 的固定 error text 委派 Rust；固定回傳 `project not found`、length-only 與短 buffer NUL 截斷 contract。此切片只遷移 project-not-found 固定 message builder；project list 掃描、Store resolution、`cbm_mcp_text_result()`、JSON-RPC wrapper、transport、server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（59 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- 驗證（2026-07-09 Phase 3 MCP project-list error JSON builder opt-in）：新增 `cbm_rs_mcp_project_list_error_v1` 與 `mcp::project_list_error`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `build_project_list_error()` 的固定 JSON 序列化委派 Rust；固定 `count > 0` 時輸出 `available_projects` + `count`，否則輸出 `No projects indexed yet` hint，支援 length-only 與短 buffer NUL 截斷。此切片只遷移 project-list error JSON 序列化 helper；cache dir 掃描、`collect_db_project_names()`、`build_no_store_error()` dispatch、Store resolution、`cbm_mcp_text_result()`、JSON-RPC wrapper、transport、server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（60 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- 驗證（2026-07-09 Phase 3 MCP search_code mode classifier opt-in）：新增 `cbm_rs_mcp_search_mode_v1` 與 `mcp::search_mode`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `parse_search_mode()` 優先把 `search_code.mode` 分類委派 Rust；固定 `full -> 1`、`files -> 2`、null/空字串/`compact`/未知/大小寫不符/尾端空白皆回 0。新增 end-to-end regression 固定 `mode:"files"` 產生 `files` result 而非 `results`。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（41 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（128 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（128 passed）。handler args extraction、grep command construction、context/source/file-list shaping、response/transport/server lifecycle 與 handlers 仍留 C。
- 驗證（2026-07-09 Phase 3 MCP trace_call_path test-file classifier opt-in）：新增 `cbm_rs_mcp_trace_is_test_file_v1` 與 `mcp::trace_is_test_file`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `is_test_file()` 優先把 `trace_call_path` BFS result file path 分類委派 Rust；固定 `/test`、`test_`、`_test.`、`/tests/`、`/spec/`、`.test.` substring contract，且不重用語意不同的 pipeline/store test detection。新增 end-to-end regression 固定 `include_tests:false` 預設濾掉測試 helper、`include_tests:true` 會顯示 helper 並加 `is_test:true`。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（42 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（129 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（129 passed）。BFS traversal、filtering、JSON result shaping、risk labels、data-flow args extraction、response/transport/server lifecycle 與 handlers 仍留 C。
- 驗證（2026-07-09 Phase 3 MCP project DB filename classifier opt-in）：新增 `cbm_rs_mcp_project_db_file_name_v1` 與 `mcp::project_db_file_name`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `is_project_db_file()` 優先把 cache directory entry 檔名分類委派 Rust；固定長度至少 `x.db`、精準 `.db` suffix、排除 `_` 開頭與 `:memory:`，並保留合法 `tmp-*.db`。#704 end-to-end fixture 擴充 `_hidden704.db` 與 `tmp-bench704.db`，固定 list_projects 不列 hidden/internal DB 且仍列 tmp-prefixed project。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（43 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（129 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（129 passed）。directory scanning、SQLite query-open、internal project-name resolution、ghost/corrupt DB filtering、JSON list building、resolve fallback、response/transport/server lifecycle 與 handlers 仍留 C。
- 驗證（2026-07-09 Phase 3 MCP index_repository mode classifier opt-in）：新增 `cbm_rs_mcp_index_mode_v1` 與 `mcp::index_mode`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `parse_index_repository_mode()` 優先把 `index_repository.mode` 分類委派 Rust；固定 `moderate -> 1`、`fast -> 2`、`cross-repo-intelligence -> 3`，null/空字串/`full`/未知/大小寫不符/尾端空白皆回 0。此切片只遷移 pure classifier；repo_path/name/persistence args extraction、cross-repo matching、pipeline creation/run、artifact/degraded response shaping、response/transport/server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（44 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（129 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（129 passed）。
- 驗證（2026-07-09 Phase 3 MCP trace_path mode edge defaults opt-in）：新增 `cbm_rs_mcp_trace_mode_edge_mask_v1` 與 `mcp::trace_mode_edge_mask`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `resolve_trace_edge_types()` 在沒有 explicit `edge_types` array 時把 `trace_path.mode` default edge set 分類委派 Rust；固定 default/calls/unknown 為 `CALLS`、`data_flow` 為 `CALLS + DATA_FLOWS`、`cross_service` 為 HTTP/async/data/calls/CROSS_* edge set。新增 C end-to-end regression 固定只有 `DATA_FLOWS` edge 時預設 mode 不追到 sink、`mode:"data_flow"` 會追到。此切片只遷移 pure default classifier；explicit edge array parsing/ownership、BFS traversal、data-flow arg extraction、risk labels、test filtering、response/transport/server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（45 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（130 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（130 passed）。
- 驗證（2026-07-09 Phase 3 MCP search_code sanitize_ascii opt-in）：新增 `cbm_rs_mcp_sanitize_ascii_in_place_v1` 與 `mcp::sanitize_ascii_in_place`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `sanitize_ascii()` 把 `search_code` source/context/result content 的 ASCII fallback sanitizer 委派 Rust；固定 ASCII 與 `0x7f` 保留、所有 `>127` byte 原地改成 `?`、null no-op。新增 C end-to-end regression 固定非 ASCII source snippet 在 full mode 輸出為 `caf??`。此切片只遷移 pure byte mutator；grep execution、source/context reading、JSON building、snippet UTF-8 lossy sanitizer、response/transport/server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（46 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- 驗證（2026-07-09 Phase 3 MCP search_code ranking scorer opt-in）：新增 `cbm_rs_mcp_search_code_score_v1` 與 `mcp::search_code_score`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `compute_search_score()` 的 deduped result ranking 加權委派 Rust；固定 Function/Method 加 10、Route 加 15、vendored/vendor/node_modules 扣 50、test/spec/`_test.` path 扣 5。此切片只遷移 pure scorer；grep execution、graph node lookup、dedup/classification、sort comparator、result JSON、response/transport/server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（52 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- 驗證（2026-07-09 Phase 3 MCP search_code score comparator opt-in）：新增 `cbm_rs_mcp_search_score_cmp_v1` 與 `mcp::search_score_cmp`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `search_result_cmp()` 的 descending score delta 委派 Rust；固定 left score 較高回負值、right score 較高回正值、相同分數回 0。此切片只遷移 qsort comparator scalar helper；qsort 呼叫、`search_result_t` ownership、grep execution、graph node lookup、dedup/classification、score 寫入、result JSON、response/transport/server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（62 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- 驗證（2026-07-09 Phase 3 MCP search_code directory top-key opt-in）：新增 `cbm_rs_mcp_search_top_dir_v1` 與 `mcp::search_top_dir`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `build_dir_distribution()` 的 top-level directory key 擷取委派 Rust；固定有 `/` 時保留到第一個 slash（含 slash），否則使用整個 path，null 回 `SIZE_MAX`，短 buffer NUL 截斷。此切片只遷移 directory top-key pure helper；directory count、yyjson object building、`search_result_t` ownership、grep execution、graph node lookup、dedup/classification、result JSON、response/transport/server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（63 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- 驗證（2026-07-09 Phase 3 MCP search_code tightest-node span helper opt-in）：新增 `cbm_rs_mcp_search_line_match_span_v1` 與 `mcp::search_line_match_span`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `find_tightest_node()` 的 per-node span 判斷先委派 Rust；固定命中區間回 `end_line - start_line`、未命中回 `-1`，Rust 回 `-1` 時保留 C fallback 判斷。此切片只遷移 pure scalar helper；node query、hit merge、result ranking、JSON result building、response/transport/server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（65 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- 驗證（2026-07-09 Phase 3 MCP pick_resolved_node best-index helper opt-in）：新增 `cbm_rs_mcp_search_pick_resolved_index_v1` 與 `mcp::search_pick_resolved_index`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `pick_resolved_node()` 的 best-index/ambiguous 計算先委派 Rust；固定回傳第一個 top score index，且 top score 多於一個時 `ambiguous=true`，invalid args 回 `-1`。此切片只遷移 pure tie-break helper；score 計算、BFS traversal、response/transport/server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（66 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- 驗證（2026-07-09 Phase 3 MCP find_tightest_node best-index helper opt-in）：新增 `cbm_rs_mcp_search_pick_tightest_index_v1` 與 `mcp::search_pick_tightest_index`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `find_tightest_node()` 的 best-index 選擇先委派 Rust；固定回傳最小且非負 span 的第一個 index，若 spans 全為負值或 invalid args 則回 `-1`。此切片只遷移 pure best-index helper；node query、hit merge、result ranking、JSON result building、response/transport/server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（67 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- 驗證（2026-07-09 Phase 3 MCP utf8_is_cont byte classifier opt-in）：新增 `cbm_rs_mcp_utf8_is_cont_v1` 與 `mcp::utf8_is_cont_byte`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `utf8_is_cont()` 的 continuation-byte 判斷委派 Rust；`sanitize_utf8_lossy()` 仍優先走既有 Rust serializer，若 ABI 失敗則 fallback 到 C loop，loop 內 continuation 判斷可用此 helper。此切片只遷移 pure byte classifier；source/snippet 讀取、JSON result building、response/transport/server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（68 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- 驗證（2026-07-09 Phase 3 MCP detect_changes status-path parser opt-in）：新增 `cbm_rs_mcp_detect_changes_status_path_offset_v1` 與 `mcp::detect_changes_status_path_offset`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `detect_changes` changed-files parsing 先委派 Rust 計算 path offset；固定 plain `git diff --name-only` 行回 offset 0、`git status --porcelain` 會略過 `XY ` 前綴、rename (`old -> new`) 取 destination，空路徑/null 回 `SIZE_MAX`，ABI 異常 fallback 到原 C parser。此切片只遷移 pure parser；git command execution、project/root resolution、Store symbol lookup、JSON result building、response/transport/server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（64 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- 驗證（2026-07-09 Phase 3 MCP node resolution score opt-in）：新增 `cbm_rs_mcp_node_resolution_score_v1` 與 `mcp::node_resolution_score`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `node_resolution_score()` 的 trace/snippet candidate scoring 委派 Rust；固定 `Function`/`Method` 最高、其他非 `Module`/`File` label 居中、`Module`/`File`/null 最低，同 tier 加非負 line span，大小寫精準比對。此切片只遷移 pure scorer；`pick_resolved_node()` 的 top-score tie/ambiguity、candidate query、BFS/snippet response shaping、node ownership、response/transport/server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（47 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- 驗證（2026-07-09 Phase 3 MCP manage_adr mode classifier opt-in）：新增 `cbm_rs_mcp_adr_mode_v1` 與 `mcp::adr_mode`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `parse_adr_mode()` 的 `manage_adr.mode` dispatch 分類委派 Rust；固定 get/default 為 0、update/store 為 1、sections 為 2，未知、大小寫不符與尾端空白皆回 get/default。既有 ADR backend regression 擴充 `store` alias 與 `sections` round-trip。此切片只遷移 pure mode classifier；project/store resolution、legacy file migration、ADR read/write、sections JSON building、response/transport/server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（48 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- 驗證（2026-07-09 Phase 3 MCP manage_adr sections JSON builder opt-in）：新增 `cbm_rs_mcp_adr_sections_json_v1` 與 `mcp::adr_sections_json`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `adr_list_sections_from_content()` 的 Markdown header 擷取與 sections JSON array 序列化委派 Rust；固定 null/empty 輸出 `[]`、移除行尾 CR、只接受 `#` 開頭行、單一 header 最多 1023 bytes、短 buffer NUL 截斷。此切片只遷移 sections JSON array builder；project/store resolution、legacy file migration、ADR read/write、response wrapping、transport、server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（61 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- 驗證（2026-07-09 Phase 3 MCP BM25 MATCH builder opt-in）：新增 `cbm_rs_mcp_bm25_build_match_v1` 與 `mcp::bm25_match_query`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `bm25_build_match()` 的 `search_graph.query` FTS5 MATCH tokenizer/serializer 委派 Rust；固定 ASCII alnum/underscore token、` OR ` 串接、非 ASCII/標點分隔、短 buffer 停在上一個完整 token，以及 null query/buffer/過小 buffer 回 0 且不寫入。此切片只遷移 pure MATCH expression builder；SQLite/FTS5 query execution、BM25 ranking/boosting、file pattern LIKE、result JSON、response/transport/server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（49 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- 驗證（2026-07-09 Phase 3 MCP get_architecture aspects gate opt-in）：新增 `cbm_rs_mcp_architecture_aspect_wanted_v1` 與 `mcp::architecture_aspect_wanted`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `aspect_wanted()` 的 aspects 判斷委派 Rust；固定缺少 aspects、invalid JSON（含尾端殘留）、root 非 object、aspects 非 array 時視為 wanted，"all" 或 exact name match 為 true，空 array 或無匹配為 false。此切片只遷移 pure aspect filter；yyjson parsing/serialization、Store links/traversal、response/transport/server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（56 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- 驗證（2026-07-09 Phase 3 MCP sanitize_utf8_lossy opt-in）：新增 `cbm_rs_mcp_sanitize_utf8_lossy_v1` 與 `mcp::sanitize_utf8_lossy`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `sanitize_utf8_lossy()` 的 MCP code snippets 來源 UTF-8 lossy 消毒委派 Rust；固定對非 UTF-8 byte 進行 lossy 消毒，將其替換為 REPLACEMENT CHARACTER U+FFFD。此切片只遷移 pure UTF-8 validator/serializer；yyjson building/serialization、response/transport/server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（55 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- 驗證（2026-07-09 Phase 3 MCP BM25 file_pattern LIKE builder opt-in）：新增 `cbm_rs_mcp_bm25_file_pattern_like_v1` 與 `mcp::bm25_file_pattern_like`，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `bm25_file_pattern_like()` 的 `search_graph.file_pattern` SQL LIKE pattern builder 委派 Rust；沿用 Store glob-to-LIKE helper，並固定無 `*` / `?` pattern 會包成 contains match（前後 `%`）、glob wildcard pattern 不額外包 `%`、null pattern 回 `SIZE_MAX`。此切片只遷移 pure pattern serializer；SQLite/FTS5 query execution、LIKE binding/query planning、BM25 ranking/boosting、result JSON、response/transport/server lifecycle 與 handlers 仍留 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（50 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。
- 驗證（2026-07-06 Phase 3 Cypher hop shorthand parity）：修正 Rust normalized AST summary 的 relationship hop shorthand，讓 `*N` 與 C parser 一致輸出 `1..N`，並固定 `*` 為 `1..0`、`*..N` 為 `1..N`。新增 C parser AST contract 與 FFI smoke。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（10 passed）、`cargo test -p cbm-core --locked`（91 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner cypher_contract`（21 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。此批仍不接 production opt-in。
- 驗證（2026-07-07 Phase 3 Cypher scalar opt-in wiring）：修正 `CBM_USE_RUST_CYPHER_SCALAR_FUNC=1` 的 Makefile 接線，讓既有 `rust-cypher-scalar-func-optin-test` 會真正定義 C macro 並連入 Rust staticlib；`scalar_func_canonical()` 仍只把 Rust 回傳索引映射回 C static names，不改 parser/evaluator/executor。本輪重跑 `cargo test -p cbm-core cypher --locked`（12 passed）、default `build/c/test-runner cypher cypher_contract`（165 passed）與 `make -f Makefile.cbm rust-cypher-scalar-func-optin-test`（165 passed）。
- 驗證（2026-07-05 Foundation YAML Rust test-only parity）：新增 `rust/cbm-core/src/foundation/yaml.rs`、`cbm_rs_yaml_*` test-only FFI 與 `tests/test_rust_ffi.c` smoke，固定 NULL/empty/partial len、nested map、string list、inline comments、duplicate key、float/bool default 與 path overflow contract；通過 `cargo fmt --all -- --check`、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`cargo test -p cbm-core foundation::yaml --locked`（3 passed）、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm test-foundation`（314 passed）與 `make -f Makefile.cbm rust-foundation-optin-test`（每輪 314 passed）。此批不接 production opt-in，預設產品仍走 C YAML parser。
- 驗證（2026-07-05 Foundation log/profile deterministic parity）：新增 `foundation::log` 與 `foundation::profile` Rust test-only helper、`cbm_rs_log_*` / `cbm_rs_profile_*` FFI smoke，固定 log level/format env parsing、text atom sanitization、JSON string escaping、HTTP path/status level、profile env gate、elapsed us/ms 與 rate 計算；另新增 `tests/test_profile.c` C-side contract 並接入 foundation runner。通過 `cargo fmt --all -- --check`、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`cargo test -p cbm-core foundation::log --locked`（2 passed）、`cargo test -p cbm-core foundation::profile --locked`（2 passed）、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm test-foundation`（316 passed）與 `make -f Makefile.cbm rust-foundation-optin-test`（每輪 316 passed）。stderr/sink lifecycle、clock、global state 與 production opt-in 仍留 C。
- 驗證（2026-07-06 Foundation profile env opt-in）：新增 `CBM_USE_RUST_PROFILE_ENV=1`，讓 `cbm_profile_init()` 只委派 Rust 判斷 `CBM_PROFILE` env gate，預設 C path 與 profile runtime/logging 保持不變。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core foundation::profile --locked`（2 passed）、`cargo test -p cbm-core --locked`（93 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm test-foundation`（322 passed）、`make -f Makefile.cbm CBM_USE_RUST_PROFILE_ENV=1 test-foundation`（322 passed）、`make -f Makefile.cbm rust-foundation-optin-test`（每輪 322 passed，含 profile env 單一與全量 opt-in）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。
- 驗證（2026-07-05 Foundation compat deterministic parity）：新增 `foundation::compat` Rust test-only helper、`cbm_rs_compat_*` FFI smoke 與 `tests/test_compat.c` C-side contract，固定 regex flags/cap/status、thread default stack 與 aligned allocation precheck；同時將 `profile`、`diagnostics` 與 `compat` suite 接入 full `test-runner` selector。通過 `cargo fmt --all -- --check`、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`cargo test -p cbm-core foundation::compat --locked`（2 passed）、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm test-foundation`（322 passed）、`make -f Makefile.cbm rust-foundation-optin-test`（每輪 322 passed）與 `make -f Makefile.cbm build/c/test-runner && build/c/test-runner profile diagnostics compat`（15 passed）。filesystem/process spawning/thread runtime 與 production opt-in 仍留 C。
- 驗證（2026-07-05 Phase 4 FQN project-name parity）：新增 `pipeline::fqn` Rust test-only helper 與 `cbm_rs_pipeline_project_name_from_path` FFI smoke，固定 `cbm_project_name_from_path` 的 NULL/empty/root fallback、separator/drive normalization、unsafe ASCII sanitizer、`..`/`--` collapse、non-ASCII byte lowercase hex encoding 與 caller-buffer truncation contract。通過 `cargo fmt --all -- --check`、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`cargo test -p cbm-core pipeline::fqn --locked`（3 passed）、`make -f Makefile.cbm rust-ffi-test` 與 `make -f Makefile.cbm build/c/test-runner && build/c/test-runner fqn`（77 passed）。此批不接 production opt-in，project key、DB/cache key 與 MCP session path 仍走 C。
- 驗證（2026-07-06 Phase 4 FullPipeline top-level metadata FFI）：新增 `cbm_rs_pipeline_full_plan_step_count_v1` / `cbm_rs_pipeline_full_plan_steps_v1`，固定 FullPipeline 外層 metadata、fast-mode gating、nested plan kind 與 ABI layout；`PlanStepV2(kind=FullPipeline)` 仍回傳 unsupported，且 `pipeline.c` / `pipeline_incremental.c` 有 static guard 確認不消費新 API。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::plan --locked`（12 passed）、`cargo test -p cbm-core --locked`（93 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test` 與 `build/c/test-runner graph_buffer pipeline`（269 passed）。
- 驗證（2026-07-05 Foundation arena public-struct parity）：新增 `foundation::arena` Rust test-only helper 與 `cbm_rs_arena_*` FFI smoke，直接以 layout-compatible `CBMArena*` 固定 default/min block size、8-byte alignment、growth、calloc、`strdup`/`strndup`、reset 保留首塊、destroy 歸零、total accounting 與 null-safe FFI guard。Rust 配置的 blocks 必須以 Rust `cbm_rs_arena_reset/destroy` 釋放，不可與 C `cbm_arena_destroy` 混用；此批不接 production opt-in，產品 allocator hot path 仍走 C。通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core foundation::arena --locked`（3 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm test-foundation`（322 passed）與 `make -f Makefile.cbm build/c/test-runner && build/c/test-runner arena`（31 passed）。
- 驗證（2026-07-05 Foundation mem pressure parity）：新增 `foundation::mem` Rust test-only helper 與 `cbm_rs_mem_*` FFI smoke，固定 `CBM_MEM_BUDGET_MB` override、`worker_budget`、`over_budget`、`peak_rss` 與 `collect` contract；不接產品 opt-in，僅作為 C path 的行為觀測對照。通過 `cargo test -p cbm-core foundation::mem --locked`（4 passed）、`make -f Makefile.cbm rust-ffi-test`、`cargo fmt --all -- --check`、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings` 與 `git diff --check`。
- 驗證（2026-07-06 本機 final gate 補強）：本輪重跑 `scripts/build.sh`、`scripts/build.sh --with-ui`、`scripts/smoke-test.sh build/c/codebase-memory-mcp`、`scripts/smoke-invariants.sh build/c/codebase-memory-mcp`（30 passed / 0 failed）、`scripts/lint.sh --ci`、`make -f Makefile.cbm security` 與完整 `scripts/test.sh`。`scripts/test.sh` 最終 `=== All tests passed ===`，包含 Rust unit 86 passed、registry opt-in 53 passed、pipeline plan opt-in 217 passed、Store FTS tokenizer opt-in 的 `store_compat mcp` 131 passed；`security` 含 security-fuzz 23/23、vendored integrity、Rust allowlist 與 cargo-audit 掃描均通過。此 gate 證明本機 build/test/lint/smoke/security 狀態，但尚不代表跨平台、package wrappers、release packaging、效能門檻、預設 Rust-backed 切換或 C fallback release cycle 已完成。

## 本次工作階段（2026-07-06 Phase 3 Store file_ext opt-in）

- 新增 `rust/cbm-core/src/store/arch_helpers.rs` 的 `file_ext_lower(path, cap)`，對齊 `src/store/store.c` `file_ext`：取 path 最後一個 `.` 起的副檔名（含 `.`），ASCII 小寫；無 `.` 或副檔名長度 >= `cap`（對齊 C `len >= sizeof(buf)`，拒絕而非截斷）時回傳 `None`。
- 新增 `cbm_rs_store_file_ext_lower_v1(buf, bufsize, path)` caller-buffer FFI（回傳長度或 `usize::MAX`），並以 `CBM_USE_RUST_STORE_FILE_EXT=1` 讓 `file_ext()` 將純轉換委派 Rust 後仍回傳 C 的 `CBM_TLS` buffer；`strrchr`、TLS buffer 生命週期與 `ext_to_lang` 的 44 項表查找仍留 C。
- 新增 `tests/test_rust_ffi.c` `test_store_file_ext_lower_exports`（14 項：基本/大小寫/最後一個 `.`/dotfile/只有 `.`、無 `.`、null/null out/bufsize==0、長度 >= buffer 拒絕與剛好放得下）。opt-in matrix target `rust-store-file-ext-optin-test` 已接入 `.PHONY`/`rust-ci`/`rust-ci-tests`/`scripts/rust-check.sh`/`scripts/test.sh`。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core store::arch_helpers --locked`（6 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner store_arch`（52 passed）、`make -f Makefile.cbm rust-store-file-ext-optin-test`（52 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-06 Phase 3 Cypher scalar func canonical opt-in）

- 新增 `rust/cbm-core/src/cypher/mod.rs` 的 `scalar_func_index(input)`，以 ASCII 大小寫不敏感（對齊 C `cyp_ci_eq`）比對 `SCALAR_FUNC_NAMES`（labels/type/id/keys/properties/toInteger/toFloat/toBoolean/size/length/trim/ltrim/rtrim/reverse），回傳符合的索引或 `None`。
- 新增 `cbm_rs_cypher_scalar_func_index_v1(input)` FFI（回傳索引或 -1），並以 `CBM_USE_RUST_CYPHER_SCALAR_FUNC=1` 讓 `src/cypher/cypher.c` `scalar_func_canonical()` 委派 Rust 索引後，映射回其自身 static `names[]` 字串，完整保留原本回傳 static 指標的語意；`cyp_ci_eq` 於他處仍使用，無 unused-function。此為 Cypher 第一個 production opt-in；parser/AST normalizer/evaluator/executor 仍留 C。
- 新增 `tests/test_rust_ffi.c` 14 項 FFI ABI smoke（大小寫、未知、長度不符、null/empty、由別處處理的 toLower/toString）。opt-in matrix target `rust-cypher-scalar-func-optin-test` 已接入 `.PHONY`/`rust-ci`/`rust-ci-tests`/`scripts/rust-check.sh`/`scripts/test.sh`。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（11 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner cypher cypher_contract`（164 passed）、`make -f Makefile.cbm rust-cypher-scalar-func-optin-test`（164 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-06 Phase 3 MCP tools/list cursor offset opt-in）

- 新增 `rust/cbm-core/src/mcp/mod.rs` 的 `tools_cursor_offset(params_json, tool_count)`，重用既有 JSON parser 驗證整份 params（含尾端殘留檢查，對齊 yyjson_read），擷取首個 `cursor` 字串，並以 `strtol_full_nonneg` 鏡像 C `strtol` 搭配 `*endptr=='\0' && errno==0 && parsed>=0` 語意（前導空白、正負號、完整消耗、i64 溢位、非負、`-0` 視為 0），最後 clamp 到 tool_count。
- 新增 `cbm_rs_mcp_tools_cursor_offset_v1(params_json, tool_count)` FFI，並在 `CBM_USE_RUST_MCP_CODEC=1` 下讓 `src/mcp/mcp.c` `mcp_tools_cursor_offset()` 委派 Rust（傳入 `TOOL_COUNT`）；response serialization、Content-Length framing、tool schema 與 14 handlers 仍留 C。
- 凍結 contract：`tests/test_mcp.c` 新增 `server_handle_tools_list_cursor_edge_cases`（cursor="0" 等同第一頁；無效字串與負數 cursor → offset=TOOL_COUNT 空頁），於預設 C 與 opt-in 皆執行以確保端到端等價；`tests/test_rust_ffi.c` 新增 27 項 FFI ABI smoke。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（7 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（124 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（124 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-06 Phase 3 Store arch path scope opt-in）

- 新增 `rust/cbm-core/src/store/arch_helpers.rs` 的 `normalize_arch_path(path, cap)` 純 helper，對齊 `src/store/store.c` `arch_path_prepare` 的 norm_out 正規化：跳過前導空白、去一次 `./`、去前導 `/`、以 `cap-1` 截斷（對齊 C `strncpy`），再去尾端 ` `/`\t`/`/` 並折疊重複 `/`；path 未設定或正規化後為空回傳 `None`。截斷刻意先於 strip/collapse，完全對齊 C。
- 新增 `cbm_rs_store_normalize_arch_path_v1(norm_out, norm_sz, path)` FFI（caller-buffer contract，回傳寫入長度或 `usize::MAX`），並以 `CBM_USE_RUST_STORE_ARCH_PATH_SCOPE=1` 讓 `arch_path_prepare` 只委派 norm_out 計算；`like_out` 的 `%s/%%` 組裝、SQLite scope query（`file_path = ? OR file_path LIKE ?`）與 bind 仍留 C。`arch_path_is_set` 以 `#ifndef` 守衛，避免 opt-in 下 `-Werror=unused-function`。
- 新增 `tests/test_rust_ffi.c` `test_store_arch_path_scope_exports` ABI smoke（含 null/empty/全空白/`./`/多斜線、null out、`norm_sz==0` 與截斷先於 strip/collapse），並新增 `rust-store-arch-path-scope-optin-test` target，接入 `.PHONY`、`rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core store::arch_helpers --locked`（5 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner store_arch store_search store_compat mcp`（257 passed）、`make -f Makefile.cbm rust-store-arch-path-scope-optin-test`（245 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。SQLite open/probe/pragma/FTS/CRUD/search runtime 仍全留 C。

## 本次工作階段（2026-07-08 Phase 3 MCP Content-Length header classifier opt-in）

- 新增 `mcp::content_length_header_matches()` 與 `cbm_rs_mcp_content_length_header_matches_v1`，固定 server loop 對 framed message header 的 pure classifier contract：只有精準 `Content-Length:` 前綴符合，大小寫、前導空白、缺少冒號、其他 header、空字串與 null 都不符合。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`cbm_mcp_server_run()` 會用 Rust classifier 決定是否進入 Content-Length framed branch；無效長度仍由既有 framed branch consume，與原 C `strncmp` + length gate 行為一致。
- 此切片只遷移 Content-Length header classifier。length parsing、header/body separator、body allocation/read、response framing、`cbm_mcp_server_handle()`、poll loop、idle eviction、server lifecycle、dispatch 與 handlers 仍由既有 C/Rust 已分工路徑負責。
- 驗證通過：`cargo fmt --all`、`cargo test -p cbm-core mcp --locked`（35 passed）、`make -f Makefile.cbm rust-ffi-test`、`cargo fmt --all -- --check`、`cargo test -p cbm-core --locked`（146 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP parse-error message opt-in）

- 新增 `mcp::parse_error_message()` 與 `cbm_rs_mcp_parse_error_message_v1`，固定 JSON-RPC request parse 失敗時的公開 error message：`Parse error`。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`cbm_mcp_server_handle()` parse failure branch 會優先使用 Rust 建構 error message；ABI 異常或 buffer 放不下時 fallback 到原 literal。
- 此切片只遷移 parse-error message builder。numeric-id JSON-RPC error formatter、Content-Length transport、request logging、server lifecycle、dispatch 與 handlers 仍由既有 C/Rust 已分工路徑負責。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（36 passed）、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm lint-format` 與 `git diff --check`。曾有一次 default C runner 與 opt-in target 並行造成 `build/c` object 被清掉的 linker failure；已用序列重跑 default gate 通過。

## 本次工作階段（2026-07-08 Phase 3 MCP tool dispatch classifier opt-in）

- 新增 `mcp::tool_dispatch_index()` 與 `cbm_rs_mcp_tool_dispatch_index_v1`，固定 `cbm_mcp_handle_tool()` 的 tool name -> handler index classifier；`trace_call_path` 相容 alias 會對應到 `trace_path` handler。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`cbm_mcp_handle_tool()` 會先用 Rust dispatch index 選擇既有 C handler；若 Rust 回傳未知、超界或與本地 `TOOLS[]` 驗證不一致，會 fallback 到原 `strcmp` chain。
- 此切片只遷移 tool dispatch classifier。所有 handler implementation、handler-specific 參數解析、tool result formatting、request logging、Content-Length transport、server lifecycle 與 `TOOLS[]` manifest 仍由 C 執行。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core --locked`（145 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP missing tool name message opt-in）

- 新增 `mcp::missing_tool_name_message()` 與 `cbm_rs_mcp_missing_tool_name_message_v1`，固定 missing-tool-name error text：`missing tool name`。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`cbm_mcp_handle_tool()` 的 `tool_name == NULL` branch 會優先使用 Rust 建構 error text；ABI 異常或 buffer 放不下時 fallback 到原 literal。
- 此切片只遷移 missing-tool-name error text builder。`cbm_mcp_text_result()` result formatting、tool dispatch、handler-specific 參數解析、JSON-RPC response wrapper、request logging、Content-Length transport、server lifecycle 與 handlers 仍由 C 執行。
- 驗證通過：`cargo fmt --all`、`cargo test -p cbm-core mcp --locked`（33 passed）、`cargo fmt --all -- --check`、`cargo test -p cbm-core --locked`（144 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-09 Phase 3 MCP missing project error object opt-in）

- 新增 `mcp::missing_project_error()` 與 `cbm_rs_mcp_missing_project_error_v1`，固定 missing project argument 時回傳的 compact JSON object：`error` 為 `missing required argument: project`，`hint` 指向 `project` argument 與 `list_projects`。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `build_missing_project_error()` 會用 Rust length-only 查詢配置 C-owned heap string，再由 Rust 寫入；ABI 異常、配置失敗或寫入長度不符時 fallback 到原 C literal。
- 此切片只遷移 missing-project 固定 message builder。`build_project_list_error()` 的 cache directory 掃描、available projects list、`cbm_mcp_text_result()` result formatting、Store resolution、JSON-RPC response wrapper、request logging、Content-Length transport、server lifecycle 與 handlers 仍由 C 執行。
- 驗證通過：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（58 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP project-not-found message opt-in）

- 新增 `mcp::project_not_found_message()` 與 `cbm_rs_mcp_project_not_found_message_v1`，固定 project open/resolve 失敗時的錯誤文字：`project not found`。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `build_project_not_found_message()` 會用 Rust length-only 查詢配置 C-owned heap string，再由 Rust 寫入；ABI 異常、配置失敗或寫入長度不符時 fallback 到原 C literal。
- 此切片只遷移固定 message builder。project list 掃描、Store resolution、`cbm_mcp_text_result()` result formatting、JSON-RPC response wrapper、request logging、Content-Length transport、server lifecycle 與 handlers 仍由 C 執行。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（59 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP project-list error JSON builder opt-in）

- 新增 `mcp::project_list_error()` 與 `cbm_rs_mcp_project_list_error_v1`，固定 `build_project_list_error()` 的 JSON 序列化（有 project 清單時輸出 `available_projects` + `count`；無清單時輸出 `No projects indexed yet` hint）。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `build_project_list_error()` 會用 Rust length-only 查詢配置 C-owned heap string，再由 Rust 寫入；ABI 異常、配置失敗或寫入長度不符時 fallback 到原 `snprintf`。
- 此切片只遷移 project-list error JSON 序列化 helper。cache directory 掃描、`collect_db_project_names()`、`build_no_store_error()` dispatch、Store resolution、`cbm_mcp_text_result()` result formatting、JSON-RPC response wrapper、request logging、Content-Length transport、server lifecycle 與 handlers 仍由 C 執行。
- 驗證通過：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（60 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-08 Phase 3 MCP unknown tool message opt-in）

- 新增 `mcp::unknown_tool_message()` 與 `cbm_rs_mcp_unknown_tool_message_v1`，固定 unknown-tool error text：`unknown tool: <name>`。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`cbm_mcp_handle_tool()` tool dispatch 找不到 handler 時會優先使用 Rust 建構 error text；長度放不進既有 `CBM_SZ_256` stack buffer 或 ABI 異常時 fallback 到原 `snprintf`。
- 此切片只遷移 unknown-tool error text builder。`cbm_mcp_text_result()` result formatting、tool dispatch、handler-specific 參數解析、JSON-RPC response wrapper、request logging、Content-Length transport、server lifecycle 與 handlers 仍由 C 執行。
- 驗證通過：`cargo fmt --all`、`cargo test -p cbm-core mcp --locked`（32 passed）、`cargo fmt --all -- --check`、`cargo test -p cbm-core --locked`（143 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP tools/call default arguments opt-in）

- 新增 `mcp::tools_call_default_arguments()` 與 `cbm_rs_mcp_tools_call_default_arguments_v1`，固定 `tools/call` request 沒有 `params` 時傳給 handler 的 default arguments object：`{}`。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`cbm_mcp_server_handle()` `tools/call` branch 只在 `req.params_raw == NULL` 時使用 Rust 建構 `tool_args` fallback；有 `params` 時仍走既有 `cbm_mcp_get_arguments()` / `cbm_rs_mcp_tools_call_arguments_v1`。
- 此切片只遷移 no-params default arguments builder。tool name dispatch、handler-specific 參數解析、JSON-RPC response wrapper、request logging、Content-Length transport、server lifecycle 與 handlers 仍由 C 執行。
- 驗證通過：`cargo fmt --all`、`cargo test -p cbm-core mcp --locked`（31 passed）、`cargo fmt --all -- --check`、`cargo test -p cbm-core --locked`（142 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP ping result object opt-in）

- 新增 `mcp::ping_result()` 與 `cbm_rs_mcp_ping_result_v1`，固定 `ping` result object 的 compact JSON shape：`{}`。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`cbm_mcp_server_handle()` `ping` branch 的 `result_json` 會優先使用 Rust；ABI 異常或 buffer 不足時 fallback 到原 `heap_strdup("{}")`。
- 此切片只遷移 `ping` result object builder。JSON-RPC response wrapper、request logging、Content-Length transport、server lifecycle 與 handlers 仍由 C 執行。
- 驗證通過：`cargo fmt --all`、`cargo test -p cbm-core mcp --locked`（30 passed）、`cargo fmt --all -- --check`、`cargo test -p cbm-core --locked`（141 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP Method not found error object opt-in）

- 新增 `mcp::method_not_found_error()` 與 `cbm_rs_mcp_method_not_found_error_v1`，固定 unknown-method error object 的 compact JSON shape：`{"code":-32601,"message":"Method not found"}`。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`cbm_mcp_server_handle()` unknown-method branch 的 error object 會優先使用 Rust；ABI 異常或 buffer 不足時 fallback 到原 `snprintf` builder。
- 此切片只遷移 Method not found error object builder。request id echo、JSON-RPC response wrapper、request logging、Content-Length transport、server lifecycle 與 handlers 仍由 C 執行。
- 驗證通過：`cargo fmt --all`、`cargo test -p cbm-core mcp --locked`（29 passed）、`cargo fmt --all -- --check`、`cargo test -p cbm-core --locked`（140 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）。

## 本次工作階段（2026-07-08 Phase 3 MCP initialize response builder opt-in）

- 新增 `mcp::initialize_response()` 與 `cbm_rs_mcp_initialize_response_v1`，固定 initialize result object 的 compact JSON shape：`protocolVersion`、`serverInfo.name/version` 與 `capabilities.tools.listChanged=false`。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`cbm_mcp_initialize_response()` 會優先使用 Rust 產生完整 result JSON，ABI 失敗時 fallback 到原 yyjson builder；預設 C path 不變。
- 此切片只遷移 initialize result JSON builder。initialize dispatch side effects（update check、session detection、auto-index）、JSON-RPC response wrapper、Content-Length transport、server lifecycle 與 handlers 仍由 C 執行。
- 驗證通過：`cargo fmt --all`、`cargo test -p cbm-core mcp --locked`（28 passed）、`cargo fmt --all -- --check`、`cargo test -p cbm-core --locked`（139 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP method dispatch classifier opt-in）

- 新增 `mcp::MethodKind` / `mcp::method_kind()` 與 `cbm_rs_mcp_method_kind_v1`，固定 JSON-RPC method exact-match 分類：unknown、initialize、ping、tools/list、tools/call、notifications/cancelled。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`cbm_mcp_server_handle()` 的 notification cancellation method 判斷與 request dispatch method 分類會委派 Rust；預設 C path 仍用原本 `strcmp` 分類。
- 此切片只遷移 pure method classifier。initialize side effects、ping/tools/list/tools/call handlers、pipeline cancellation side effect、request logging、JSON-RPC response wrapping、Content-Length transport、server lifecycle 與 14 個 tool handlers 仍由 C 執行。
- 驗證通過：`cargo fmt --all`、`cargo test -p cbm-core mcp --locked`（27 passed）、`cargo fmt --all -- --check`、`cargo test -p cbm-core --locked`（138 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-07 Phase 4 pipeline test detection opt-in）

- 新增 `rust/cbm-core/src/pipeline/test_detect.rs`，固定 `pass_tests.c` 既有測試檔案與測試函式名稱 classifier contract：`test_` basename、語言特定 `_test` / `Test` / `Spec` suffix、`.test/.spec` substring、`/tests/` 類目錄，以及 Go `Test/Benchmark/Example` 大寫規則。
- 新增 `cbm_rs_pipeline_is_test_path_v1` / `cbm_rs_pipeline_is_test_func_name_v1` / `cbm_rs_pipeline_test_to_prod_path_v1`，並以 `CBM_USE_RUST_PIPELINE_TEST_DETECT=1` 讓 `cbm_is_test_path()` / `cbm_is_test_func_name()` / `test_to_prod_path()` 可 opt-in 委派 Rust；預設 C path 保持不變。
- 此切片只遷移 pure classifier 與 test/prod path rewrite。`TESTS` / `TESTS_FILE` edge creation、`node_is_test()`、graph-buffer mutation adapter、tree-sitter/extraction、pass ordering 與 pipeline side effects 仍由 C 執行。
- 新 target `rust-pipeline-test-detect-optin-test` 已接入 `rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7；`rust-language-graph-parity` 的 Rust pipeline binary 也會同時啟用此 opt-in。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::test_detect --locked`（4 passed）、`cargo test -p cbm-core --locked`（121 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm rust-pipeline-test-detect-optin-test`（217 passed）與 `make -f Makefile.cbm rust-language-graph-parity`（golden 一致；中斷後已補跑完成）。

## 本次工作階段（2026-07-09 Phase 4 pipeline checked-exception classifier opt-in）

- 新增 `pipeline::exception::is_checked_exception()` 與 `cbm_rs_pipeline_is_checked_exception_v1`，固定 `is_checked_exception()` 的 checked-exception classifier contract：空字串與一般 checked exception 名稱回 true；含 `Error`、`Panic`、`error` 或 `panic` 的名稱回 false；null 回 false。
- 在 `CBM_USE_RUST_PIPELINE_CHECKED_EXCEPTION=1` 下，`src/pipeline/pass_usages.c` 與 `src/pipeline/pass_parallel.c` 的 `is_checked_exception()` 會先委派 Rust；預設 C path 保持不變。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、empty、`NotFoundException`、`NotFoundError`、`panic`、`ClientPanic`、`custom error` 與 `CheckedThing`。
- 此切片只遷移 pure classifier；`THROWS` / `RAISES` edge type 選擇、exception resolution、graph-buffer adapter 與 pass side effects 仍由 C 負責。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::exception --locked`（1 passed）、`cargo test -p cbm-core --locked`（182 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm rust-pipeline-checked-exception-optin-test`（269 passed）與 `make -f Makefile.cbm rust-language-graph-parity`（golden 一致；曾有一次並行跑共享 `build/c` 的 target 造成 clean-c race，後續序列重跑已通過）。

## 本次工作階段（2026-07-10 Phase 4 pipeline artifact path opt-in）

- 新增 `rust/cbm-core/src/pipeline/artifact.rs`，把 `src/pipeline/artifact.c` 的 artifact path builder 拆成純 Rust helper，固定 `<repo>/.codebase-memory/<name>` 的 byte-level contract，保留 raw bytes、不做 normalization。
- 新增 `cbm_rs_pipeline_artifact_path_v1` 與 `CBM_USE_RUST_PIPELINE_ARTIFACT_PATH=1` opt-in；`src/pipeline/artifact.c` 的 `artifact_path()` 會先委派 Rust，ABI 失敗、null input 或短 buffer 時回傳 `SIZE_MAX`，C fallback 保持不變。
- 將 `rust-pipeline-artifact-path-optin-test` 接入 `Makefile.cbm`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7；default `build/c/test-runner artifact` 與 opt-in target 都跑 `artifact` suite，確認 export/import、`.gitattributes` 與 null safety 沒有回歸。
- 補齊 `tests/test_rust_ffi.c` 的 FFI smoke，覆蓋 null repo/name、null buffer、`bufsize == 0`、short buffer 與 non-UTF-8 raw bytes。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::artifact --locked`（4 passed）、`cargo test -p cbm-core --locked`（186 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner artifact`（10 passed）、`make -f Makefile.cbm rust-pipeline-artifact-path-optin-test`（10 passed）與 `git diff --check`。

## 本次工作階段（2026-07-10 Phase 4 pipeline project-name opt-in）

- 新增 `CBM_USE_RUST_PIPELINE_PROJECT_NAME=1`，讓 `src/pipeline/fqn.c` 的 `cbm_project_name_from_path()` 先以 Rust helper 做 length-only 探測，再配置 C-owned buffer 寫回；ABI 失敗、null input 或 short buffer 時 fallback 到原 C 實作。
- 將 `rust-pipeline-project-name-optin-test` 接入 `Makefile.cbm` 與 `scripts/rust-check.sh`；`build/c/test-runner fqn mcp` 在該 opt-in 下通過 208 passed。
- `rust/CBM_FFI.md` 與 `docs/rust-refactor-baseline.md` 已同步註記此 helper 從 test-only parity 升為 production opt-in；預設 C path 保持不變。
- 驗證通過：`make -f Makefile.cbm rust-pipeline-project-name-optin-test`（208 passed）。

## 本次工作階段（2026-07-10 Phase 4 pipeline configures opt-in）

- 新增 `rust/cbm-core/src/pipeline/configures.rs`，把 `is_env_var_name()`、`normalize_config_key_bytes()` 與 `has_config_extension()` 拆成純 Rust helper，並對齊 C fallback 的 byte-level contract。
- 新增 `cbm_rs_pipeline_is_env_var_name_v1` / `cbm_rs_pipeline_normalize_config_key_v1` / `cbm_rs_pipeline_has_config_extension_v1`，`CBM_USE_RUST_PIPELINE_CONFIGURES=1` 時 `src/pipeline/pass_configures.c` 會先委派 Rust；預設 C path 保持不變。
- 新增 Cargo `pipeline-configures-only` feature 與 `CBM_USE_RUST_PIPELINE_CONFIGURES_ONLY=1`：此 opt-in 會排除 `src/pipeline/pass_configures.c`，由 Rust staticlib 直接匯出既有 `cbm_is_env_var_name()`、`cbm_normalize_config_key()`、`cbm_has_config_extension()` C ABI；預設 C fallback 與既有 wrapper opt-in 均保留。default、traces-only、configures-only 與兩者合併的 staticlib 各使用獨立 Cargo target dir，避免 feature archive 快取混用。
- Rust-only direct ABI 仍把 nullable C string、null/zero/short caller buffer 與 NUL 結尾處理集中於 `ffi.rs`，純 byte-level 邏輯仍留在 `pipeline::configures`；512-byte key 截斷與 token count 維持 C contract。config link pass、graph-buffer 寫入與其他 pipeline side effects 仍由 C 控制。
- `tests/test_rust_ffi.c` 與 Rust unit tests 已固定 env var 判斷、config key normalization 與 config extension contract。
- `rust-pipeline-configures-only-test` 已接入 `scripts/rust-check.sh`、`scripts/test.sh`、`rust-ci` 與 `rust-ci-tests`；target 以隔離 runner 跑完整 pipeline suite，不與既有共享 `build/c` gate 混用。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::configures --features pipeline-configures-only --locked`（3 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm rust-pipeline-configures-optin-test`（219 passed）與 `make -f Makefile.cbm rust-pipeline-configures-only-test`（隔離 default/Rust-only pipeline 各 219 passed；含 direct ABI FFI smoke）。`traces-only,pipeline-configures-only` 合併 archive 亦在 `target/cbm-traces-configures-only` 成功建置。

## 本次工作階段（2026-07-10 手動 all-optins 組合 prerequisite gate）

- 新增手動 `rust-all-optin-test`，將 39 個現有 production Rust opt-in 組成兩個隔離變體：wrapper 使用 `CBM_USE_RUST_TRACES=1` 與 `CBM_USE_RUST_PIPELINE_CONFIGURES=1`；direct 以 `CBM_USE_RUST_TRACES_ONLY=1` 與 `CBM_USE_RUST_PIPELINE_CONFIGURES_ONLY=1` 替換這兩個旗標。其餘 37 個 production flags 共用，test-only parity 不納入，兩種 trace/configures 變體不會同時傳入。
- 每個變體各有獨立 `BUILD_DIR`，會在自己的首個 build 前執行 `clean-c` 以避免 stale C objects，依序建置並執行 FFI smoke、無 selector 的完整 C runner、production `cbm` 與 `--version`；不對共用 `build/c` 做 clean，且沿用 `RUST_TARGET` 與 staticlib feature-cache 分流。
- 為避免日常 CI 成本暴增，target 刻意未接入 `rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 或 `scripts/test.sh`；本輪已實際執行，wrapper/direct 完整 runner 各 5861 passed，並完成各自 production `--version` smoke。
- 此為首次全組合 prerequisite，不是 Rust-backed release candidate（RC），亦不表示預設 C 產品路徑、C fallback 或整體遷移狀態已改變。

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

## 本次工作階段（2026-07-09 Phase 3 MCP int/bool argument extraction opt-in）

- 新增 `mcp::int_arg()` / `mcp::bool_arg()` 與 `cbm_rs_mcp_get_int_arg_v1` / `cbm_rs_mcp_get_bool_arg_v1`，固定 MCP handler args root object 的 int/bool field extraction：首個 matching key 生效，missing、型別不符、root 非 object、invalid JSON、invalid UTF-8 與空 key 都回到 default/false。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`cbm_mcp_get_int_arg()` / `cbm_mcp_get_bool_arg()` 會先委派 Rust；預設 C path 仍用 yyjson helper。
- 此切片只遷移共用 int/bool argument extraction。project alias resolution、handler-specific range/default validation、response formatting、Content-Length transport、server lifecycle 與 14 個 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（38 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（125 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP relationship edge-type validator opt-in）

- 新增 `mcp::edge_type_valid()` 與 `cbm_rs_mcp_edge_type_valid_v1`，固定 `search_graph.relationship` edge type contract：nullable C string input；非 null 且長度 <= 64 bytes、每個 byte 都是 ASCII `A-Z` 或 `_` 時有效；null、過長、小寫、數字、空白、dash 與非 ASCII byte 失敗。空字串目前維持 C helper 行為視為有效。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `validate_edge_type()` 會優先委派 Rust；預設 C path 仍使用原本 `strlen` + byte loop。
- 新增 Rust unit、FFI smoke 與 C end-to-end regression。C 測試使用既有 snippet fixture 並明確傳入 `project:"test-project"`，確保請求通過 project/store validation 後命中 relationship validator。
- 此切片只遷移 pure validator。`search_graph` args extraction、project/index validation、store search/query planning、result/error wrapping、Content-Length transport、server lifecycle 與 14 個 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（39 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（126 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（126 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP search_code path/file_pattern validator opt-in）

- 新增 `mcp::search_path_arg_valid()` 與 `cbm_rs_mcp_search_path_arg_valid_v1`，固定 `search_code` shell path / `file_pattern` safety validator contract：null 失敗；空字串、一般 path、space 與 `&` 合法；single quote、double quote、`;`、`|`、`$`、backtick、`<`、`>`、LF、CR 失敗；非 Windows 另拒絕 backslash。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `validate_search_path_arg()` 會優先委派 Rust；預設 C path 仍使用原本 denylist loop。
- 新增 Rust unit、FFI smoke 與 C end-to-end regression。C 測試使用既有 snippet fixture 與 `project:"test-project"`，以非法 `file_pattern:"*.go;rm -rf /"` 固定 `path or file_pattern contains invalid characters` 與 `isError:true`。
- 此切片只遷移 pure validator。project root resolution、grep command construction、temp file writing、path filter regex、search process execution、result parsing、response wrapping、Content-Length transport、server lifecycle 與 14 個 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（40 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（127 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（127 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP search_code root/file args combiner opt-in）

- 新增 `mcp::search_args_valid()` 與 `cbm_rs_mcp_search_args_valid_v1`，固定 `validate_search_args()` contract：`root_path` 必填且需通過 path validator，`file_pattern` 可省略但若提供也需通過同一 denylist。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `validate_search_args()` 會優先委派 Rust；預設 C path 仍使用原本兩段式 C 檢查。
- 新增 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null root、空字串 root、root only、合法 pattern、`&`、非法 root、非法 pattern 與含換行 pattern。
- 此切片只遷移 pure combiner。project root resolution、grep command construction、temp file writing、path filter regex、search process execution、result parsing、response wrapping、Content-Length transport、server lifecycle 與 14 個 handlers 仍由 C 負責。
- 驗證通過：`cargo test -p cbm-core mcp --locked`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp` 與 `make -f Makefile.cbm rust-mcp-codec-optin-test`。

## 本次工作階段（2026-07-09 Phase 3 MCP search_code strip root prefix opt-in）

- 新增 `mcp::search_strip_root_prefix_offset()` 與 `cbm_rs_mcp_strip_root_prefix_offset_v1`，固定 `strip_root_prefix()` contract：只做 byte prefix 比對；prefix 後若是 `/` 再多跳過一個 byte；不檢查 path component boundary。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `strip_root_prefix()` 會以 Rust 回傳的 offset 回到原 `path` borrowed pointer；預設 C path 保持不變，不產生 Rust-owned 字串。
- 邊界行為已凍結：null path/root 回 0；exact root 回 `root_len`；root trailing slash 不額外再跳一層；`root_len == 0` 對齊 C `strncmp(..., 0) == 0`，若 path 以 `/` 開頭會回 1；`root_len > path/root length` 作為防禦式 ABI invalid-parameter 行為回 0。正常 C caller 仍傳 `strlen(root)`。
- 此切片只遷移 `search_code` grep result path 的 prefix offset 計算。project root resolution、grep command construction、temp file writing、path filter regex、search process execution、result parsing、JSON response、Content-Length transport、server lifecycle 與 14 個 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（57 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP search_code mode classifier opt-in）

- 新增 `mcp::search_mode()` 與 `cbm_rs_mcp_search_mode_v1`，固定 `search_code.mode` classifier contract：`full` 回 1、`files` 回 2，null、空字串、`compact`、未知值、大小寫不符與尾端空白皆回 0。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `parse_search_mode()` 會優先委派 Rust；預設 C path 仍使用原本 `strcmp` fallback。
- 新增 Rust unit、FFI smoke 與 C end-to-end regression。C 測試使用既有 snippet fixture 與 `mode:"files"`，固定回應包含 `files` 且不包含 `results` / `isError:true`。
- 此切片只遷移 pure classifier。handler args extraction、grep command construction、context/source/file-list result shaping、response wrapping、Content-Length transport、server lifecycle 與 14 個 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（41 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（128 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（128 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP trace_call_path test-file classifier opt-in）

- 新增 `mcp::trace_is_test_file()` 與 `cbm_rs_mcp_trace_is_test_file_v1`，固定 `trace_call_path` 的 MCP-local test-file classifier：path 含 `/test`、`test_`、`_test.`、`/tests/`、`/spec/` 或 `.test.` 即視為測試；null 與空字串為 false。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `is_test_file()` 會優先委派 Rust；預設 C path 仍使用原本 `strstr` chain。此 helper 刻意不重用 pipeline/store test detection，因為那些 contract 不同。
- 新增 Rust unit、FFI smoke 與 C end-to-end regression。C 測試建立 `entry -> src/tests/test_helper.c` CALLS，固定預設會濾掉測試 helper，`include_tests:true` 會顯示 helper 並輸出 `is_test:true`。
- 此切片只遷移 pure classifier。BFS traversal、`include_tests` filtering、`is_test` JSON 標記、risk labels、data-flow args extraction、response wrapping、Content-Length transport、server lifecycle 與 14 個 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（42 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（129 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（129 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP project DB filename classifier opt-in）

- 新增 `mcp::project_db_file_name()` 與 `cbm_rs_mcp_project_db_file_name_v1`，固定 MCP cache scan 的 project DB filename classifier：至少 `x.db`、精準 `.db` suffix、排除 `_` 開頭與 `:memory:` 開頭；`tmp-*.db` 維持合法。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `is_project_db_file()` 會優先委派 Rust；預設 C path 仍使用原本長度/suffix/prefix 檢查。
- 擴充 Rust unit、FFI smoke 與 #704 C end-to-end fixture。C 測試新增 `_hidden704.db` 與 `tmp-bench704.db`，固定 `list_projects` 不列 hidden/internal DB、仍列 tmp-prefixed project。
- 此切片只遷移 pure filename classifier。directory scanning、SQLite query-open、internal project-name resolution、ghost/corrupt DB filtering、JSON list building、resolve fallback、response wrapping、Content-Length transport、server lifecycle 與 14 個 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（43 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（129 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（129 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP index_repository mode classifier opt-in）

- 新增 `mcp::index_mode()` 與 `cbm_rs_mcp_index_mode_v1`，固定 `index_repository.mode` classifier：`moderate` 回 1、`fast` 回 2、`cross-repo-intelligence` 回 3；null、空字串、`full`、未知值、大小寫不符與尾端空白皆回 0。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` 新增 `parse_index_repository_mode()` 並優先委派 Rust；預設 C path 仍使用原本 `strcmp` fallback。回傳 3 僅作為 cross-repo dispatch sentinel，不傳入 `cbm_pipeline_new()`。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、空字串、full、moderate、fast、cross-repo、大小寫不符、尾端空白與未知值。
- 此切片只遷移 pure classifier。repo_path/name/persistence args extraction、cross-repo matching、pipeline creation/run、artifact/degraded response shaping、response wrapping、Content-Length transport、server lifecycle 與 14 個 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（44 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（129 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（129 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP trace_path mode edge defaults opt-in）

- 新增 `mcp::trace_mode_edge_mask()` 與 `cbm_rs_mcp_trace_mode_edge_mask_v1`，固定 `trace_path.mode` default edge-type classifier：default/calls/未知回 `CALLS`，`data_flow` 回 `CALLS|DATA_FLOWS`，`cross_service` 回 HTTP/async/data/calls/CROSS_* bitmask。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `resolve_trace_edge_types()` 只有在沒有 explicit `edge_types` array 時委派 Rust bitmask；C 端仍使用本地 static edge-name 字串餵 BFS，並保留 `data_flow` 與 `cross_service` 的既有 edge order。
- 擴充 Rust unit、`tests/test_rust_ffi.c` smoke 與 `tests/test_mcp.c` end-to-end regression。C 測試固定只有 `DATA_FLOWS` edge 時預設 mode 不追到 sink，`mode:"data_flow"` 會追到。
- 此切片只遷移 pure default classifier。explicit edge array parsing/ownership、BFS traversal、data-flow arg extraction、risk labels、test filtering、response wrapping、Content-Length transport、server lifecycle 與 14 個 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（45 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（130 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（130 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP search_code sanitize_ascii opt-in）

- 新增 `mcp::sanitize_ascii_in_place()` 與 `cbm_rs_mcp_sanitize_ascii_in_place_v1`，固定 MCP `search_code` 輸出端 ASCII sanitizer contract：ASCII 與 `0x7f` 保留，所有大於 127 的 byte 就地改成 `?`，null input 為 no-op。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `sanitize_ascii()` 會優先委派 Rust；預設 C path 仍使用原本 byte loop。
- 擴充 Rust unit、`tests/test_rust_ffi.c` smoke 與 `tests/test_mcp.c` end-to-end regression。C 測試建立含 UTF-8 `é` bytes 的 source snippet，固定 `search_code` full mode 輸出 `caf??`。
- 此切片只遷移 pure byte mutator。grep execution、source/context reading、JSON building、snippet UTF-8 lossy sanitizer、response wrapping、Content-Length transport、server lifecycle 與 14 個 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（46 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-09 Phase 3 MCP search_code ranking scorer opt-in）

- 新增 `mcp::search_code_score()` 與 `cbm_rs_mcp_search_code_score_v1`，固定 `search_code` deduped result ranking score contract：以 `in_degree` 為基底，Function/Method 加 10、Route 加 15、vendored/vendor/node_modules 扣 50、test/spec/`_test.` path 扣 5。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `compute_search_score()` 會委派 Rust；預設 C path 仍使用原本 string compare / substring check。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、Function/Method、Route、class-like label、大小寫精準、vendored/vendor/node_modules、test/spec/`_test.` 與 vendor+test 疊加。
- 此切片只遷移 pure scorer。grep execution、graph node lookup、dedup/classification、sort comparator、result JSON、response wrapping、Content-Length transport、server lifecycle 與 14 個 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（52 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP search_code score comparator opt-in）

- 新增 `mcp::search_score_cmp()` 與 `cbm_rs_mcp_search_score_cmp_v1`，固定 `search_result_cmp()` 的 descending score comparator：left score 較高回負值、right score 較高回正值、相同分數回 0。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `search_result_cmp()` 會委派 Rust 計算 score delta；預設 C path 保持原 `rb->score - ra->score`。
- 此切片只遷移 qsort comparator scalar helper。qsort 呼叫、`search_result_t` ownership、grep execution、graph node lookup、dedup/classification、score 寫入、result JSON、response wrapping、Content-Length transport、server lifecycle 與 handlers 仍由 C 執行。
- 驗證通過：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（62 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP search_code directory top-key opt-in）

- 新增 `mcp::search_top_dir()` 與 `cbm_rs_mcp_search_top_dir_v1`，固定 `search_code` directory distribution 的 top-level key 擷取：有 `/` 時保留到第一個 slash（含 slash），否則使用整個 file path。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` 新增 `search_result_top_dir()` wrapper，讓 `build_dir_distribution()` 委派 Rust 擷取 directory key；ABI 異常時 fallback 到原 C `strchr` / copy 邏輯。
- 此切片只遷移 directory top-key pure helper。directory count 累計、yyjson object building、`search_result_t` ownership、grep execution、graph node lookup、dedup/classification、result JSON、response wrapping、Content-Length transport、server lifecycle 與 handlers 仍由 C 執行。
- 驗證通過：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（63 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP detect_changes scope classifier opt-in）

- 新增 `mcp::detect_changes_wants_symbols()` 與 `cbm_rs_mcp_detect_changes_wants_symbols_v1`，固定 `detect_changes.scope` classifier contract：null/default、`symbols` 與 `impact` 輸出 impacted symbols；空字串、`files`、大小寫不符、尾端空白與未知值只輸出 changed files。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `detect_changes_wants_symbols()` 會委派 Rust；預設 C path 仍使用原本 `strcmp` 判斷。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、symbols、impact、empty、files、大小寫、尾端空白與未知值。
- 此切片只遷移 pure classifier。project/root resolution、base branch validation、git diff/status command、changed file parsing、Store symbol lookup、JSON result building、response wrapping、Content-Length transport、server lifecycle 與 14 個 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（53 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP detect_changes impacted label classifier opt-in）

- 新增 `mcp::detect_changes_impacted_label()` 與 `cbm_rs_mcp_detect_changes_impacted_label_v1`，固定 `detect_changes` impacted symbols label filter contract：null、`File`、`Folder` 與 `Project` 排除；其餘 label（含空字串、大小寫不符與尾端空白）皆保留。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `detect_changes_impacted_label()` 會委派 Rust；預設 C path 仍使用原本 `strcmp` 判斷。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、File、Folder、Project、Function、Method、Route、empty、大小寫不符與尾端空白。
- 此切片只遷移 pure classifier。Store node query、node ownership/free、changed file parsing、impacted JSON item building、response wrapping、Content-Length transport、server lifecycle 與 14 個 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（54 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP detect_changes status-path parser opt-in）

- 新增 `mcp::detect_changes_status_path_offset()` 與 `cbm_rs_mcp_detect_changes_status_path_offset_v1`，固定 changed-files path parser contract：plain path 回 offset 0；`git status --porcelain` 略過 `XY ` 前綴；rename (`old -> new`) 取 destination；空路徑與 null 回 `SIZE_MAX`。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `detect_changes` changed-files 迴圈會先委派 Rust path offset parser；ABI 異常時 fallback 到原 C parser，預設 C path 不變。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、empty、plain path、porcelain 前綴（`??` / `A ` / ` M`）、rename destination、rename empty destination 與未知前綴。
- 此切片只遷移 pure parser。git command execution、project/root resolution、Store symbol lookup、JSON result building、response wrapping、Content-Length transport、server lifecycle 與 14 個 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（64 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-09 Phase 3 MCP search_code tightest-node span helper opt-in）

- 新增 `mcp::search_line_match_span()` 與 `cbm_rs_mcp_search_line_match_span_v1`，固定 `find_tightest_node()` 單一 node span contract：命中區間回 `end_line - start_line`，未命中回 `-1`。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `find_tightest_node()` 的 per-node span 判斷會先委派 Rust；Rust 回 `-1` 時保留原 C 條件判斷 fallback，預設 C path 不變。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 hit/start/end boundary、before/after miss 與 invalid range。
- 此切片只遷移 pure scalar helper。node query、hit merge、result ranking、JSON result building、response wrapping、Content-Length transport、server lifecycle 與 14 個 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（65 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP pick_resolved_node best-index helper opt-in）

- 新增 `mcp::search_pick_resolved_index()` 與 `cbm_rs_mcp_search_pick_resolved_index_v1`，固定 `pick_resolved_node()` 的 best-index/ambiguous contract：空輸入回 `-1,false`；回傳第一個 top score index；top score 多於一個時 `ambiguous=true`。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `pick_resolved_node()` 會先將 score 陣列委派 Rust 計算 best index 與 ambiguous；ABI 異常時 fallback 到原 C 兩段式比較/top_count 邏輯，預設 C path 不變。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 empty/unique/tie/negative 分數與 invalid args（null out、null scores、count<=0）。
- 此切片只遷移 pure tie-break helper。score 計算、BFS traversal、response wrapping、Content-Length transport、server lifecycle 與 14 個 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（66 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP find_tightest_node best-index helper opt-in）

- 新增 `mcp::search_pick_tightest_index()` 與 `cbm_rs_mcp_search_pick_tightest_index_v1`，固定 `find_tightest_node()` best-index contract：回傳最小且非負 span 的第一個 index；全 miss（全負值）或 invalid args 時回 `-1`。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `find_tightest_node()` 會先把 per-node span 陣列委派 Rust 選擇 best index；ABI 異常時 fallback 到原 C 逐筆比較邏輯，預設 C path 不變。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 empty/all-miss/unique/tie-first/mixed spans 與 invalid args（null spans、count<=0）。
- 此切片只遷移 pure best-index helper。node query、hit merge、result ranking、JSON result building、response wrapping、Content-Length transport、server lifecycle 與 14 個 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（67 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP utf8_is_cont byte classifier opt-in）

- 新增 `mcp::utf8_is_cont_byte()` 與 `cbm_rs_mcp_utf8_is_cont_v1`，固定 `utf8_is_cont()` contract：低 8-bit 符合 `10xxxxxx` 回 true，否則 false。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `utf8_is_cont()` 會先委派 Rust；`sanitize_utf8_lossy()` 既有 Rust serializer 失敗時 fallback 到 C loop，loop 內 continuation 判斷可使用此 helper，預設 C path 不變。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 `0x80`、`0xBF`、`0x7F`、`0xC0` 與高位輸入 byte mask 行為。
- 此切片只遷移 pure byte classifier。source/snippet 讀取、JSON result building、response wrapping、Content-Length transport、server lifecycle 與 14 個 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（68 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP node resolution score opt-in）

- 新增 `mcp::node_resolution_score()` 與 `cbm_rs_mcp_node_resolution_score_v1`，固定 trace/snippet candidate scoring contract：`Function`/`Method` 最高、其他非 `Module`/`File` label 居中、`Module`/`File`/null 最低，同 tier 加非負 line span。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `node_resolution_score()` 會委派 Rust；預設 C path 仍使用原本 label/span scoring。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null/empty label、Function/Method、class-like label、Module/File、負 span 與大小寫精準比對。
- 此切片只遷移 pure scorer。`pick_resolved_node()` 的 top-score tie/ambiguity、candidate query、BFS/snippet response shaping、node ownership、response wrapping、Content-Length transport、server lifecycle 與 14 個 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（47 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP manage_adr mode classifier opt-in）

- 新增 `mcp::adr_mode()` 與 `cbm_rs_mcp_adr_mode_v1`，固定 `manage_adr.mode` dispatch classifier：get/default 回 0，update/store 回 1，sections 回 2；null、空字串、未知、大小寫不符與尾端空白皆回 get/default。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `parse_adr_mode()` 會委派 Rust；預設 C path 仍使用原本 string compare contract。
- 擴充 Rust unit、`tests/test_rust_ffi.c` smoke 與 `tests/test_mcp.c` ADR backend regression。C 測試現在以 `mode:"store"` 寫入，並以 `mode:"sections"` 驗證 `## PURPOSE` / `## STACK` round-trip。
- 此切片只遷移 pure mode classifier。project/store resolution、legacy file migration、ADR read/write、sections JSON building、response wrapping、Content-Length transport、server lifecycle 與 14 個 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（48 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP manage_adr sections JSON builder opt-in）

- 新增 `mcp::adr_sections_json()` 與 `cbm_rs_mcp_adr_sections_json_v1`，固定 `manage_adr(mode=sections)` 的 Markdown header 擷取與 JSON array 序列化。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `adr_list_sections_from_content()` 會用 Rust length-only 查詢配置 C-owned heap string，再由 Rust 寫入 sections JSON array，並以 `yyjson_mut_rawncpy()` 複製到 yyjson document；ABI 異常、配置失敗或 raw JSON 掛載失敗時 fallback 到原 C loop。
- 契約固定 null/empty 輸出 `[]`、逐行移除行尾 `\r`、只接受第一個 byte 為 `#` 且非空的行、單一 header 最多保留 1023 bytes，並由 Rust JSON string serializer 處理 quote/backslash/control 與 invalid UTF-8 lossy replacement。
- 此切片只遷移 sections JSON array builder。project/store resolution、legacy file migration、ADR read/write、response wrapping、Content-Length transport、server lifecycle 與 handlers 仍由 C 執行。
- 驗證通過：`cargo fmt --all`、`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（61 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP BM25 MATCH builder opt-in）

- 新增 `mcp::bm25_match_query()` 與 `cbm_rs_mcp_bm25_build_match_v1`，固定 `search_graph.query` BM25 FTS5 MATCH builder contract：只保留 ASCII alnum/underscore token，以 ` OR ` 串接，非 token byte 分隔，短 buffer 停在上一個完整 token。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `bm25_build_match()` 會委派 Rust；預設 C path 仍使用原本 tokenizer loop。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null buffer/null query、empty/punctuation、hyphen/dot/non-ASCII 分隔、多 token、短 buffer 與過小 buffer 不寫入。
- 此切片只遷移 pure MATCH expression tokenizer/serializer。SQLite/FTS5 query execution、BM25 ranking/boosting、file pattern LIKE、result JSON、response wrapping、Content-Length transport、server lifecycle 與 14 個 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（49 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP get_architecture aspects gate opt-in）

- 新增 `mcp::architecture_aspect_wanted()` 與 `cbm_rs_mcp_architecture_aspect_wanted_v1`，固定 `get_architecture` aspects gate 判斷：缺少 aspects、invalid JSON（含尾端殘留）、root 非 object、aspects 非 array 時視為 wanted；空 array 或無匹配為 false；"all" 或 exact name match 為 true；大小寫與空白不寬鬆；非字串元素忽略。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `aspect_wanted()` 會優先委派 Rust；預設 C path 仍使用原本 yyjson parser 判斷。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、empty、no-aspects、not-array、empty-array、"all" 匹配、exact aspect match、no match、非字串元素忽略與尾端殘留 JSON invalid fallback。
- 此切片只遷移 pure aspect filter。yyjson parsing/serialization、Store links/traversal、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（56 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP sanitize_utf8_lossy opt-in）

- 新增 `mcp::sanitize_utf8_lossy()` 與 `cbm_rs_mcp_sanitize_utf8_lossy_v1`，固定 MCP code snippets 來源 UTF-8 lossy 消毒：對非 UTF-8 byte 進行 lossy 消毒，將其替換為 REPLACEMENT CHARACTER U+FFFD。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `sanitize_utf8_lossy()` 會優先委派 Rust；預設 C path 仍使用原本 byte loop 消毒。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、empty、length-only、ASCII plain text、invalid bytes (如 `\xff` 或非法的 UTF-8 byte 組合) 與短 buffer 截斷。
- 此切片只遷移 pure UTF-8 validator/serializer。yyjson building/serialization、response/transport/server lifecycle 與 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（52 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

## 本次工作階段（2026-07-09 Phase 3 MCP BM25 file_pattern LIKE builder opt-in）

- 新增 `mcp::bm25_file_pattern_like()` 與 `cbm_rs_mcp_bm25_file_pattern_like_v1`，固定 `search_graph.file_pattern` BM25 side SQL LIKE pattern builder：先沿用 Store glob-to-LIKE helper，無 `*` / `?` 時加前後 `%` 形成 contains match。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `bm25_file_pattern_like()` 會委派 Rust；預設 C path 仍使用原本 `cbm_glob_to_like()` + contains wrapping。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、empty、length-only、plain path contains wrapping、`*`、`?`、bracket literal 與短 buffer 截斷。
- 此切片只遷移 pure LIKE pattern serializer。SQLite/FTS5 query execution、LIKE binding/query planning、BM25 ranking/boosting、result JSON、response wrapping、Content-Length transport、server lifecycle 與 14 個 handlers 仍由 C 負責。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（50 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、預設 `build/c/test-runner mcp`（131 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（131 passed）。

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

## 本次工作階段（2026-07-07 Phase 3 MCP tools/call name codec opt-in）

- 新增 `cbm_rs_mcp_tools_call_name_v1` FFI 與 `mcp::tools_call_name`，固定 `tools/call` params object 中首個 `name` 字串的擷取 contract。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` 的 `cbm_mcp_get_tool_name()` 會以 Rust helper 擷取 dispatch name，再由 C wrapper 配置 C-owned NUL 結尾字串；預設 C path 不變。
- 新增 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 length-only、短 buffer、duplicate first-wins、Unicode escape、missing name、首個 name 非 string、invalid JSON、root 非 object、NULL input 與負長度。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（9 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner mcp`（124 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（124 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-07 Phase 3 MCP cancelled matcher codec opt-in）

- 新增 `cbm_rs_mcp_cancel_request_matches_v1` FFI 與 `mcp::cancel_request_matches`，固定 `notifications/cancelled` params object 中首個 `requestId` 的 matching contract。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` 的 `cbm_mcp_cancel_request_matches()` 會委派 Rust helper；預設 C path 不變。
- 擴充 C contract 與 `tests/test_rust_ffi.c` smoke，覆蓋 numeric/string id 分流、duplicate first-wins、Unicode escape、float/bool/type mismatch、invalid JSON、root 非 object、missing `requestId`、NULL input 與 invalid UTF-8。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（9 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、`build/c/test-runner mcp`（124 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（124 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-07 Phase 3 MCP initialize protocol version codec opt-in）

- 新增 `mcp::initialize_protocol_version` 與 `cbm_rs_mcp_initialize_protocol_version_v1`，固定 supported protocol versions newest-first、exact match、duplicate first-wins、unsupported/missing/non-string/invalid fallback latest 的 selection contract。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`cbm_mcp_initialize_response()` 只把 initialize params 的 `protocolVersion` selection 委派 Rust；C 仍負責 response JSON 欄位、yyjson serialization/ownership、dispatch、framing 與 handlers。
- 擴充 Rust unit、`tests/test_rust_ffi.c` ABI smoke 與 `tests/test_mcp.c` end-to-end initialize contract，覆蓋 supported/unknown/missing/non-string/root array/duplicate/trailing cases。
- 驗證通過：`cargo fmt --all`、`cargo test -p cbm-core mcp --locked`（11 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（124 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（124 passed）。

## 本次工作階段（2026-07-07 Phase 3 MCP JSON-RPC error formatter codec opt-in）

- 新增 `mcp::jsonrpc_format_error` 與 `cbm_rs_mcp_jsonrpc_format_error_v1`，固定 JSON-RPC numeric-id error response 的 compact JSON 欄位順序、`id`/`code` 數值、message escaping、NULL message 空字串、length-only 與短 buffer NUL 截斷 contract。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` 的 `cbm_jsonrpc_format_error()` 會優先委派 Rust formatter；若 FFI 回傳異常則保留 yyjson fallback。預設 C path 不變。
- 此切片只遷移 numeric-id error formatter。string-id response、result embedding、MCP text result、invalid embedded JSON fallback、Content-Length transport、tool schema、dispatch、server lifecycle 與 handlers 仍留 C/yyjson。
- 擴充 Rust unit、`tests/test_rust_ffi.c` smoke 與 `tests/test_mcp.c` exact JSON contract，覆蓋一般錯誤、quote/backslash/newline escaping、Unicode message、NULL message 與短 buffer。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（12 passed）、`cargo test -p cbm-core --locked`（123 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（124 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（124 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-07 Phase 3 MCP JSON-RPC response formatter codec opt-in）

- 新增 `mcp::jsonrpc_format_response` 與 `cbm_rs_mcp_jsonrpc_format_response_v1`，固定 JSON-RPC response 的 numeric/string id、string id escaping、`result` embedding、`error` 優先、missing result/error 時 `result:null`、invalid embedded JSON 時只輸出 `jsonrpc/id`、length-only 與短 buffer contract。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` 的 `cbm_jsonrpc_format_response()` 會優先委派 Rust formatter；若 FFI 回傳異常則保留 yyjson fallback。預設 C path 不變。
- 此切片只遷移 JSON-RPC response JSON 組裝。Content-Length transport、server loop、MCP text result、tool schema、tool dispatch、handlers 與 lifecycle 仍留 C/yyjson。
- 擴充 Rust unit、`tests/test_rust_ffi.c` smoke 與 `tests/test_mcp.c` exact JSON contract，覆蓋 string id issue #253、escaped id、result/error、invalid result/error、`result:null` 與短 buffer。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（13 passed）、`cargo test -p cbm-core --locked`（124 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（124 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（124 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-07 Phase 3 MCP text result formatter codec opt-in）

- 新增 `mcp::mcp_text_result` 與 `cbm_rs_mcp_text_result_v1`，固定 MCP tool result 的 compact JSON contract：`content[0].type/text`、`isError`、字串 escaping、NULL text 空字串、length-only 與短 buffer NUL 截斷。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` 的 `cbm_mcp_text_result()` 會優先委派 Rust formatter；若 FFI 回傳異常則保留 yyjson fallback。預設 C path 不變。
- `structuredContent` 只在非 error 且 `text` 是 JSON object 時加入，並以 minified JSON object 輸出；plain text、JSON array、invalid JSON、NULL text 與 error result 都不加入。
- 此切片只遷移 MCP text result JSON 組裝。Content-Length transport、server loop、tool schema、tool dispatch、handlers 與 lifecycle 仍留 C/yyjson。
- 擴充 Rust unit、`tests/test_rust_ffi.c` smoke 與 `tests/test_mcp.c` exact JSON contract，覆蓋 structured object、plain text、array text、escaped error、NULL text、length-only 與短 buffer。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（14 passed）、`cargo test -p cbm-core --locked`（125 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（124 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（124 passed）。

## 本次工作階段（2026-07-07 Phase 3 MCP tool index codec opt-in）

- 新增 `mcp::tool_index` 與 `cbm_rs_mcp_tool_index_v1`，固定 C `TOOLS[]` 的 14 個 tool name 順序、case-sensitive lookup、NULL/empty/unknown 回 -1 contract。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` 的 `cbm_mcp_tool_input_schema()` 會先委派 Rust 查找 tool index，再用 C 本地 `TOOLS[idx].input_schema` 回傳既有 static schema 字串；預設 C path 不變。
- 此切片只遷移 name -> index 純查找。input schema JSON、title、description、tools/list serialization、tool dispatch、handlers、Content-Length transport 與 lifecycle 仍留 C/yyjson。
- 擴充 Rust unit、`tests/test_rust_ffi.c` smoke 與 `tests/test_mcp.c` schema lookup contract，覆蓋首尾 tool、unknown/null/empty/case-sensitive 與 CLI 共用 schema source。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（15 passed）、`cargo test -p cbm-core --locked`（126 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）。

## 本次工作階段（2026-07-07 Phase 3 MCP tools/list page bounds codec opt-in）

- 新增 `mcp::tools_page_bounds` 與 `cbm_rs_mcp_tools_page_bounds_v1`，固定 `cbm_mcp_tools_list_range()` 的 offset clamp、limit clamp、end、has_next 與 next_cursor contract。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` 的 `cbm_mcp_tools_list_range()` 會先委派 Rust 計算 page bounds，再由 C 以本地 `TOOLS[]` 和 yyjson 建立 tools/list JSON；預設 C path 不變。
- 此切片只遷移 pagination range 計算。tool metadata、input/output schema JSON、tools/list serialization、JSON-RPC response wrapping、tool dispatch、handlers、Content-Length transport 與 lifecycle 仍留 C/yyjson。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 first/second/full page、負 offset、超界 offset、負 limit、超界 limit、負 tool_count 與 null out；既有 `tests/test_mcp.c` tools/list pagination contract 同時覆蓋 default 與 opt-in path。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（16 passed）、`cargo test -p cbm-core --locked`（127 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）與 `make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）。

## 本次工作階段（2026-07-07 Phase 3 MCP tools/list tool name manifest opt-in）

- 新增 `mcp::tool_count` / `mcp::tool_name` 與 `cbm_rs_mcp_tool_count_v1` / `cbm_rs_mcp_tool_name_v1`，固定 C `TOOLS[]` 的 14 個 tool name 順序與 index -> name contract。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` 的 `mcp_add_tool_def()` 會在 Rust count/name 與本地 C `TOOLS[]` 完全一致時，用 Rust 回傳的 name 寫入 `tools/list`；若 ABI 失敗或不一致，回到原 C name。
- 此切片只遷移 `tools/list` 的 name manifest。title、description、input/output schema JSON、JSON 組裝、dispatch、handlers、Content-Length transport 與 lifecycle 仍留 C/yyjson。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 count、首中尾 tool name、length-only、短 buffer、負 index 與超界 index。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（17 passed）、`cargo test -p cbm-core --locked`（128 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-07 Phase 3 MCP tools/list tool title manifest opt-in）

- 新增 `mcp::tool_title` 與 `cbm_rs_mcp_tool_title_v1`，固定 C `TOOLS[]` 的 14 個 tool title 順序與 index -> title contract。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` 的 `mcp_add_tool_def()` 會在 Rust count/name/title 與本地 `TOOLS[]` 完全一致時，用 Rust 回傳的 title 寫入 `tools/list`；若 ABI 失敗或不一致，回到原 C title。
- 此切片只遷移 `tools/list` 的 title manifest。description、input/output schema JSON、JSON 組裝、dispatch、handlers、Content-Length transport 與 lifecycle 仍留 C/yyjson。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋首中尾 title、length-only、短 buffer、負 index 與超界 index。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（18 passed）、`cargo test -p cbm-core --locked`（129 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP tools/list tool description manifest opt-in）

- 新增 `mcp::tool_description` 與 `cbm_rs_mcp_tool_description_v1`，固定 C `TOOLS[]` 的 14 個 tool description 順序、完整 byte 長度、短 buffer NUL 截斷與超界 index contract。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` 的 `mcp_add_tool_def()` 會在 Rust count/name/title/description 與本地 `TOOLS[]` 完全一致時，用 Rust 回傳的 description 寫入 `tools/list`；若 ABI 失敗或不一致，回到原 C description。
- 此切片只遷移 `tools/list` 的 description manifest。input/output schema JSON、JSON 組裝、dispatch、handlers、Content-Length transport 與 lifecycle 仍留 C/yyjson。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋首中尾 description、長描述關鍵片段、length-only、短 buffer、負 index 與超界 index。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（19 passed）、`cargo test -p cbm-core --locked`（130 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP tools/list tool inputSchema manifest opt-in）

- 新增 `mcp::tool_input_schema` 與 `cbm_rs_mcp_tool_input_schema_v1`，固定 C `TOOLS[]` 的 14 個 input schema JSON 順序、完整 byte 長度、短 buffer NUL 截斷與超界 index contract。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` 的 `mcp_add_tool_def()` 會在 Rust count/name/title/description/inputSchema 與本地 `TOOLS[]` 完全一致時，用 Rust 回傳的 schema JSON 建立 `inputSchema`；若 ABI 失敗或不一致，回到原 C schema。
- 此切片只遷移 `tools/list` 的 inputSchema manifest。yyjson parse/copy/serialization、outputSchema、dispatch、handlers、Content-Length transport 與 lifecycle 仍留 C/yyjson。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 empty schema、required fields、escaped quotes、path regex backslash、length-only、短 buffer、負 index 與超界 index。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（20 passed）、`cargo test -p cbm-core --locked`（131 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP file URI parser opt-in）

- 新增 `mcp::parse_file_uri()`，固定 `cbm_parse_file_uri()` 的 byte-level contract：只接受 `file://` prefix、保留 raw path 與 percent encoding，並對 `file:///C:/...` 類 Windows drive path 移除一個 leading slash。
- 新增 `cbm_rs_mcp_parse_file_uri_v1()` caller-buffer ABI，支援 length-only、短 buffer NUL 截斷、empty path 與 invalid/null URI 回傳 `SIZE_MAX`。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `cbm_parse_file_uri()` 的 path extraction 會委派 Rust；預設 C path 保持原本 `strncmp`、drive slash strip 與 `snprintf` 截斷邏輯。
- 此切片只遷移 pure URI path extraction。caller buffer ownership、錯誤時清空輸出、MCP root/session lifecycle、URI 使用端與 tool handlers 仍留 C。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（26 passed）、`cargo test -p cbm-core --locked`（137 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP Content-Length header separator opt-in）

- 新增 `mcp::content_length_header_is_blank()`，固定 header line 在移除尾端 CR/LF 後是否為空字串的 separator 判斷；空字串、`\n`、`\r\n` 與連續 CR/LF 為 true，空白字元不視為空行。
- 新增 `cbm_rs_mcp_content_length_header_is_blank_v1()` FFI，null input 回傳 0。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `handle_content_length_frame()` 的 header/body blank line 判斷會委派 Rust；預設 C path 保持原本 strip CR/LF 後檢查 `hlen == 0`。
- 此切片只遷移 header separator 判斷。`getline`、body allocation/read、呼叫 `cbm_mcp_server_handle()`、response framing、poll loop、idle eviction、server lifecycle、dispatch 與 14 handlers 仍留 C。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（25 passed）、`cargo test -p cbm-core --locked`（136 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP Content-Length response framing opt-in）

- 新增 `mcp::content_length_response()`，固定 MCP response frame 為 `Content-Length: <response byte length>\r\n\r\n<response>`，長度以 response JSON byte 數計算。
- 新增 `cbm_rs_mcp_content_length_response_v1()` caller-buffer ABI，支援 length-only、短 buffer NUL 截斷、空 response 與 null response error contract。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` `handle_content_length_frame()` 會先嘗試 Rust response frame，再以 `fwrite` 寫出完整 byte length；ABI 失敗或配置失敗時 fallback 到原本 C `fprintf` path。
- 此切片只遷移 response framing。跳過 header blank line、讀取 body、呼叫 `cbm_mcp_server_handle()`、poll loop、idle eviction、server lifecycle、dispatch 與 14 handlers 仍留 C。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（24 passed）、`cargo test -p cbm-core --locked`（135 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP Content-Length header parser opt-in）

- 新增 `mcp::content_length_value` 與 `cbm_rs_mcp_content_length_v1`，固定 `cbm_mcp_server_run()` Content-Length header gate：必須精準以 `Content-Length:` 開頭、數值區對齊 `strtol(..., NULL, 10)` 的前導空白/正負號/尾端殘留容忍，且只接受 `>0 && <= 10 MiB` 的 body 長度。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` 的 Content-Length header length gate 會委派 Rust；預設 C path 保持原本 `strtol`。
- 此切片只遷移 header length parsing/gating。跳過 header blank line、讀取 body、呼叫 `cbm_mcp_server_handle()`、輸出 Content-Length response framing、poll loop、idle eviction、server lifecycle、dispatch 與 14 handlers 仍留 C。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋 null、大小寫不符、空值、正負號、尾端殘留、0/負數與 max gate。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（23 passed）、`cargo test -p cbm-core --locked`（134 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-12 MCP Content-Length raw length ABI 與超長 frame 同步修正）

- 新增 `mcp::content_length_raw_value()` 與 `cbm_rs_mcp_content_length_raw_v1`，將已解析的宣告 body 長度提供給 C；既有接受上限 gate 保持不變。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，語法有效但超過可接受上限的 frame 會完整消耗其 header/body，再繼續讀取下一個 frame；default C 採同一排空流程，避免 transport stream 失同步。
- 此切片不移轉 body allocation/read、排空、`cbm_mcp_server_handle()`、response framing、poll loop、idle eviction、server lifecycle、dispatch 或 handlers；它們仍由 C 負責。
- 已實跑：`cargo test -p cbm-core mcp --locked`（69 passed）、default `build/c/test-runner mcp`（134 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（134 passed）、`make -f Makefile.cbm rust-ffi-test`。

## 本次工作階段（2026-07-08 Phase 3 MCP tools/list result JSON builder opt-in）

- 新增 `mcp::tools_list_json` 與 `cbm_rs_mcp_tools_list_json_v1`，固定 `tools/list` result object 的 compact JSON：`tools` array、tool 欄位順序、input/output schema object embedding、`nextCursor` 字串、offset/limit clamp、length-only 與短 buffer NUL 截斷 contract。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` 的 `cbm_mcp_tools_list_range()` 會先嘗試使用 Rust 產生完整 result JSON；若 ABI 回傳異常、配置失敗或寫入長度不一致，fallback 到既有 C/yyjson builder。預設 C path 不變。
- 此切片只遷移 `tools/list` result JSON builder。JSON-RPC response wrapper、Content-Length framing、server loop、tool dispatch、14 handlers 與 lifecycle 仍留 C/yyjson。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋第一頁、完整頁、空頁、length-only 與短 buffer。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（22 passed）、`cargo test -p cbm-core --locked`（133 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-08 Phase 3 MCP tools/list tool outputSchema manifest opt-in）

- 新增 `mcp::tool_output_schema` 與 `cbm_rs_mcp_tool_output_schema_v1`，固定 C `MCP_TOOL_OUTPUT_SCHEMA` 的共用 output schema JSON、完整 byte 長度與短 buffer NUL 截斷 contract。
- 在 `CBM_USE_RUST_MCP_CODEC=1` 下，`src/mcp/mcp.c` 的 `mcp_add_tool_def()` 會在 Rust outputSchema 與本地 C 常數完全一致時，用 Rust 回傳的 schema JSON 建立 `outputSchema`；若 ABI 失敗或不一致，回到原 C schema。
- 此切片只遷移 `tools/list` 的 outputSchema manifest。yyjson parse/copy/serialization、dispatch、handlers、Content-Length transport 與 lifecycle 仍留 C/yyjson。
- 擴充 Rust unit 與 `tests/test_rust_ffi.c` smoke，覆蓋完整 JSON、length-only 與短 buffer。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core mcp --locked`（21 passed）、`cargo test -p cbm-core --locked`（132 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner mcp`（125 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（125 passed）、`make -f Makefile.cbm lint-format` 與 `git diff --check`。

## 本次工作階段（2026-07-07 Phase 3 Store checkpoint golden 與 opt-in wiring）

- 在 `tests/test_store_compat.c` 新增 `store_compat_checkpoint_preserves_wal_size_contract`，固定 `cbm_store_checkpoint()` 使用非 truncating checkpoint 語意：呼叫成功後 `-wal` 檔仍存在且非空。
- 修正 `Makefile.cbm`，讓 `CBM_USE_RUST_STORE_ARCH_PATH_SCOPE=1` 與 `CBM_USE_RUST_STORE_FILE_EXT=1` 會實際加入 C macro 並觸發 Rust staticlib link；避免既有 opt-in target 看似通過但仍走 C fallback。
- SQLite open/probe/pragma、WAL runtime、CRUD/search/FTS/BM25、store ownership 與 artifact runtime 仍留 C。
- 驗證通過：`build/c/test-runner store_compat`（15 passed）、`make -f Makefile.cbm rust-store-arch-path-scope-optin-test`（246 passed）與 `make -f Makefile.cbm rust-store-file-ext-optin-test`（52 passed）。

## 本次工作階段（2026-07-07 Phase 3 Cypher scalar opt-in wiring）

- 修正 `Makefile.cbm`，讓 `CBM_USE_RUST_CYPHER_SCALAR_FUNC=1` 會實際加入 `-DCBM_USE_RUST_CYPHER_SCALAR_FUNC` 並觸發 Rust staticlib link。
- 這讓既有 `rust-cypher-scalar-func-optin-test` 不再可能空跑 C fallback；若 Rust symbol 或 helper contract 漂移，opt-in build/test 應會暴露。
- `scalar_func_canonical()` ownership 不變：Rust 只回傳 index，C 仍回傳自己的 static `names[]` 指標；parser、AST normalizer、evaluator 與 executor 仍留 C。
- 驗證通過：本輪已執行 `make -f Makefile.cbm rust-cypher-scalar-func-optin-test`（165 passed）。

## 本次工作階段（2026-07-07 Phase 3 Cypher multiarg opt-in wiring）

- 新增 `cbm_rs_cypher_multiarg_func_index_v1` 與對齊 C 的 `multiarg_func_index()`；Rust 只回傳 `coalesce`、`substring`、`replace`、`left`、`right` 的名稱索引。
- `CBM_USE_RUST_CYPHER_MULTIARG_FUNC=1` 時，`src/cypher/cypher.c` `multiarg_func_canonical()` 只將索引比對委派給 Rust，C 仍回傳自己的 static `names[]` 字串並接續現有 evaluator 路徑。
- `tests/test_rust_ffi.c` 已補 `cbm_rs_cypher_multiarg_func_index_v1` 的 FFI 覆蓋：大小寫、未知函式、`null/empty`、prefix/suffix、`toLower/labels/split` 回 -1，以及 `coalesce/substring/replace/left/right` 命中案例。
- `rust-cypher-multiarg-func-optin-test` 已接入 `.PHONY`、`rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7。
- 驗證通過：本輪已重跑 default `build/c/test-runner cypher cypher_contract`（165 passed）與 `make -f Makefile.cbm rust-cypher-multiarg-func-optin-test`（165 passed）。

## 本次工作階段（2026-07-08 Phase 3 Cypher aggregate function token opt-in）

- 新增 `cypher::aggregate_func_index()` 與 `cbm_rs_cypher_aggregate_func_index_v1`，固定 `agg_func_name()` 的 aggregate token mapping：`TOK_COUNT`、`TOK_SUM`、`TOK_AVG`、`TOK_MIN_KW`、`TOK_MAX_KW`、`TOK_COLLECT`。
- 在 `CBM_USE_RUST_CYPHER_AGG_FUNC=1` 下，`agg_func_name()` 只將 token -> name index 查表委派 Rust；C 仍回傳自己的 static `names[]` 字串，parse item、ORDER BY formatting、aggregation evaluator/executor、AST normalizer 與 result 行為仍留 C。
- 新 target `rust-cypher-agg-func-optin-test` 已接入 `.PHONY`、`rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（13 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner cypher cypher_contract`（165 passed）與 `make -f Makefile.cbm rust-cypher-agg-func-optin-test`（165 passed）。`make -f Makefile.cbm lint-format` 與 `git diff --check` 亦已通過。

## 本次工作階段（2026-07-09 Phase 3 Cypher string function token opt-in）

- 新增 `cypher::string_func_index()` 與 `cbm_rs_cypher_string_func_index_v1`，固定 `str_func_name()` 的 string function token mapping：`TOK_TOLOWER`、`TOK_TOUPPER`、`TOK_TOSTRING`。
- 在 `CBM_USE_RUST_CYPHER_STRING_FUNC=1` 下，`str_func_name()` 只將 token -> name index 查表委派 Rust；C 仍回傳自己的 static `names[]` 字串，parse item、scalar/string evaluator、AST normalizer 與 result 行為仍留 C。
- 新 target `rust-cypher-string-func-optin-test` 已接入 `.PHONY`、`rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 與 `scripts/test.sh` Step 7。
- 驗證通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core cypher --locked`（14 passed）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、`make -f Makefile.cbm rust-ffi-test`、default `build/c/test-runner cypher cypher_contract`（165 passed）與 `make -f Makefile.cbm rust-cypher-string-func-optin-test`（165 passed）。`make -f Makefile.cbm lint-format` 與 `git diff --check` 亦已通過。

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

目前產品預設核心仍位於 `src/` 的 C 程式碼，包含 `foundation/`、`store/`、`cypher/`、`mcp/`、`pipeline/`、`discover/`、`watcher/`、`cli/` 與 `ui/`。Rust workspace 已存在於 `rust/`，但只在明確 opt-in 下支援部分 foundation parity、Store 純 helper、MCP request parse、pipeline registry/plan 決策、route/test-detect 純 helper 與 test-only graph-buffer command contract。tree-sitter 擷取、語言規格與 Hybrid LSP 邏輯位於 `internal/cbm/`。測試集中在 `tests/test_*.c` 與 `tests/repro/`。

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

> 目前位置：預設產品 binary 仍是 **100% 純 C、零 Rust**。目前工作樹的 `rust/cbm-core/src` 約 18K 行 / 41 個 Rust 檔，仍只佔整體自有程式碼少數，且所有 production Rust 皆由 `CBM_USE_RUST_*` 明確 opt-in 啟用；C 原始碼與預設路徑仍完整保留。已完成的是 foundation 部分 parity、pipeline registry/plan 的決策層 parity，以及 test-only graph-buffer command contract；離「可移除 C」仍差完整子系統取代、預設 build 切換、至少一個 release cycle 的 C fallback 驗證與最終 packaging/install/UI/security/perf gate。

每個切片一律維持既有方法論：先凍結 contract/golden fixture（預設 C 必須通過）→ 新增 Rust pure helper 與 FFI smoke（不改預設行為）→ 新增單一 `CBM_USE_RUST_*` opt-in → 跑 default / 單一 opt-in / 全 opt-in matrix → 接 `scripts/test.sh`、`scripts/smoke-test.sh`、`scripts/smoke-invariants.sh`、`make -f Makefile.cbm security` 與對應 parity target。每個 PR 維持單一議題、建議 < 500 行。

### 1. Foundation 剩餘葉模組（最低風險，但含 allocator hot path）

- **範圍**：`arena`、`compat_fs`、`compat_regex`、`compat_thread`、`compat`、`hash_table`、`log`、`mem`、`profile`、`slab_alloc`、`vmem`、`yaml`。
- **建議切片順序**：
  1. `yaml`：既有 C contract suite 已納入 foundation matrix；test-only Rust parity 與 FFI smoke 已完成，`CBM_USE_RUST_YAML` production wrapper 已接入，`CBM_USE_RUST_YAML_ONLY` direct 仍另行管理；`rust-yaml-optin-test` 與 `rust-yaml-only-test` 已納入 `scripts/test.sh` Step 7 與 `scripts/rust-check.sh` 矩陣。
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
- **驗證方式**：`make -f Makefile.cbm rust-pipeline-registry-optin-test`、`make -f Makefile.cbm rust-pipeline-plan-optin-test`、`make -f Makefile.cbm rust-pipeline-route-canon-optin-test`、`make -f Makefile.cbm rust-pipeline-test-detect-optin-test`、`make -f Makefile.cbm rust-language-graph-parity`、`build/c/test-runner pipeline`、`scripts/test.sh`、`scripts/smoke-invariants.sh`。

### 3. Store：SQLite 圖儲存

- **範圍**：`src/store/` 的 schema、pragma、authorizer、UDF、FTS、CRUD、search、BFS、ADR、architecture、vector search、artifact import/export 與 readonly/bulk/checkpoint 行為。
- **建議切片順序**：
  1. 補完整 `sqlite_schema`、`table_xinfo`、index layout、FTS shadow object、checkpoint/bulk side effect、artifact import/export fixtures。
  2. Rust schema/index manifest helper 已完成 static/test-only V1/V2 FFI，固定核心表、`nodes_fts`、`edges.url_path_gen`、9 個 user index、table column/index layout、FTS DDL 與 FTS shadow metadata；connection/pragma manifest helper 也已固定 read-only no-create、immutable fallback、WAL、mmap env、bulk pragma 與 public checkpoint filtered plan contract；FTS camelCase tokenization helper 已固定 `cbm_camel_split` 的 byte-level split/fallback contract，並以 `CBM_USE_RUST_STORE_FTS_TOKENIZER=1` 提供只委派字串計算的窄範圍 opt-in；mmap resolver 已以 `CBM_USE_RUST_STORE_MMAP_RESOLVER=1` 委派 env 字串解析；immutable URI builder 已以 `CBM_USE_RUST_STORE_IMMUTABLE_URI=1` 委派 readonly fallback URI 字串建構；search pattern helper 已以 `CBM_USE_RUST_STORE_SEARCH_PATTERN=1` 委派 glob→LIKE、LIKE hint 與 case flag 純字串 contract。SQLite open/probe/pragma/FTS/CRUD/search runtime 仍留 C。
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

## 2026-07-11：`str_intern` direct replacement 實作

- 新增 `str-intern-only` Cargo feature 與 Rust direct `cbm_intern_*` ABI。
- `CBM_USE_RUST_STR_INTERN_ONLY=1` 會從 `FOUNDATION_SRCS` 排除 `src/foundation/str_intern.c`；一般 `CBM_USE_RUST_STR_INTERN=1` 的 C wrapper/fallback 不變。
- `rust-str-intern-only-test` 已加入 Rust CI 與 test subset prerequisite，並覆蓋 default foundation runner、direct Rust FFI runner 與 direct foundation runner。
- `rust-str-intern-only-test` 已通過：default/direct foundation runner 各 `325 passed`；direct all-optin wrapper/direct 各 `5861 passed`，兩個 production binary 均成功輸出 `codebase-memory-mcp dev`。
- direct all-optin 使用 `--features traces-only,pipeline-configures-only,str-intern-only`，link source list 明確排除 `src/foundation/str_intern.c`；此切片已完成驗證。

## 2026-07-11：`dump_verify` direct replacement 實作

- 新增 `dump-verify-only` Cargo feature 與 Rust direct `cbm_dump_verify_is_degraded`、`cbm_dump_verify_min_ratio` ABI。
- `CBM_USE_RUST_DUMP_VERIFY_ONLY=1` 會從 `FOUNDATION_SRCS` 排除 `src/foundation/dump_verify.c`；一般 `CBM_USE_RUST_DUMP_VERIFY=1` C wrapper/fallback 保持不變。
- invalid ratio warning 透過 `log.c` 的固定參數 bridge 保留既有 structured logging，沒有引入 Rust variadic FFI。
- `rust-dump-verify-only-test` default/direct foundation runner 各 `325 passed`；四 feature direct all-optin 與 wrapper all-optin 各 `5861 passed`，兩個 direct production binary 均成功輸出 `codebase-memory-mcp dev`。
- direct all-optin 使用 `--features traces-only,pipeline-configures-only,str-intern-only,dump-verify-only`，且 direct flags 不再帶一般 `CBM_USE_RUST_DUMP_VERIFY=1`；此切片已完成驗證。

## 2026-07-11：`yaml` direct replacement 實作

- 新增 `yaml-only` Cargo feature 與 Rust direct `cbm_yaml_*` ABI，沿用既有 `Node` parser/query/free ownership。
- 補齊 C `strtod` 相容 float prefix：ASCII whitespace、正負號、十進位、十六進位、`inf`/`nan` 與不完整 exponent；Rust YAML unit 為 `4 passed`。
- `CBM_USE_RUST_YAML_ONLY=1` 會從 `FOUNDATION_SRCS` 排除 `src/foundation/yaml.c`；`rust-yaml-only-test` default/direct foundation 各 `325 passed`，direct production binary 與版本輸出成功。
- 五 feature direct all-optin 與 wrapper all-optin 各 `5861 passed`，direct 使用 `traces-only,pipeline-configures-only,str-intern-only,dump-verify-only,yaml-only`；此切片已完成驗證。

### Store search pattern direct slice

`store-search-pattern-only` 已完成 direct 替換。`cbm_glob_to_like` 與
`cbm_extract_like_hints` 現由 Rust 實作，透過 C heap 回傳並由既有呼叫端以
`free()` 釋放；SQLite 與 Store 核心仍保留在 C。預設與 direct 的 Store/相容性/MCP
聚焦測試各通過 216 項，六切片全量 opt-in wrapper/direct 各通過 5861 項。
目前 direct replacement 計數為 52。

### Store architecture helpers direct slice

`store-arch-helpers-only` 已完成 direct 替換。risk、QN package、top package
與 test-file 純 helper 已從 `store.c` 拆至 `src/store/arch_helpers.c`；default
build 保留 C fallback，direct build 則由 Rust 匯出相同 `cbm_*` ABI 並排除該
translation unit。QN 回傳維持每執行緒 256-byte buffer，未改變既有呼叫端
ownership。聚焦 default/direct 各通過 253 項，七切片全量 opt-in wrapper/direct
均成功完成。
目前 direct replacement 計數為 52。

### Store file extension direct slice

`store-file-ext-only` 已完成 direct 替換。`file_ext` 已拆為
`src/store/file_ext.c` 的 C fallback，direct build 改由 Rust 提供
`cbm_store_file_ext`，維持 null/16-byte thread-local buffer 契約。default/direct
Store 聚焦測試各通過 253 項，八切片全量 opt-in wrapper/direct 均成功完成。
目前 direct replacement 計數為 52。

### Store immutable URI direct slice

`store-immutable-uri-only` 已完成 direct 替換。immutable SQLite URI 編碼已拆至
`src/store/immutable_uri.c` 的 C fallback，direct build 改由 Rust 直接寫入 caller
buffer 並回傳成功狀態；SQLite open、WAL fallback 與 Store lifecycle 仍保留在 C。
default/direct 聚焦測試各通過 216 項，九切片全量 opt-in wrapper/direct 均成功。
目前 direct replacement 計數為 52。

### Store mmap resolver direct slice

`store-mmap-resolver-only` 已完成 direct 替換。`cbm_store_resolve_mmap_size`
已從 `store.c` 拆至 `src/store/mmap_resolver.c` 的 C fallback，direct build
改由 Rust 讀取環境變數並使用既有 parser；PRAGMA 執行與 SQLite connection
仍保留在 C。default/direct 聚焦測試各通過 157 項，十切片全量 opt-in 前置 gate
已準備完成。
目前 direct replacement 計數為 52。

### Store architecture path scope 直接替換切片

`store-arch-path-scope-only` 以 Rust 實作 architecture path 正規化與 `/%` LIKE pattern 產生，
保留 SQL fragment 與 SQLite bind 於 C。專屬 default/direct gate 各 `268 passed`；
11-slice all-opt-in wrapper/direct gate 均成功完成。目前 direct replacement 計數為 52。

### Store FTS tokenizer 直接替換切片

`store-fts-tokenizer-only` 將 FTS camelCase SQLite callback 的 tokenization 與結果
ownership 移至 Rust；SQLite function 註冊仍在 C。callback 使用 SQLite 配置器轉移
結果 ownership，且與通用 FFI object 隔離。default/direct 完整 suite 均通過，
12-slice all-opt-in wrapper/direct gate 亦成功完成。目前 direct replacement 計數為 52。

### Pipeline project name 直接替換切片

`pipeline-project-name-only` 將 `cbm_project_name_from_path` 移至 Rust，並以 C `malloc`
維持 caller `free()` ownership ABI。default/direct FQN suite 各 `77 passed`；13-slice
all-opt-in wrapper/direct gate 均成功完成。目前 direct replacement 計數為 52。

### Pipeline artifact path 直接替換切片

`pipeline-artifact-path-only` 將 artifact path buffer helper 移至 Rust，保留 artifact
export/import 的所有 side effect 於 C。default/direct artifact suite 各 `10 passed`；
14-slice all-opt-in wrapper/direct gate 均成功完成。目前 direct replacement 計數為 52。
### Pipeline test detection 直接替換切片

`pipeline-test-detect-only` 將公開的 test path/function classifier 移至 Rust，保留
TESTS/TESTS_FILE graph edge 邏輯與 heap-returning path conversion 於 C。
default/direct pipeline suite 各 `221 passed`；15-slice all-opt-in wrapper/direct gate
均成功完成。目前 direct replacement 計數為 52。

### Pipeline checked-exception 直接替換切片

`pipeline-checked-exception-only` 將 null-safe exception-name predicate 移至 Rust，保留
THROWS/RAISES edge resolution 與所有 graph mutation 於 C。default/direct
`edge_types_probe pipeline` 各 `273 passed`；16-slice all-opt-in wrapper/direct gate
各 `5861 passed`，兩個 release binary 均成功完成。目前 direct replacement 計數為 52。

2026-07-15 再強化 targeted gate：default/direct 使用隔離 build 目錄，Rust unit
`pipeline::exception` 為 `1 passed`；direct 同時通過同名 ABI FFI smoke、production
link 及 `--version`。Make 展開顯示 `CHECKED_EXCEPTION_SRCS =`，direct link line 亦未
納入 `src/pipeline/checked_exception.c`，因此 opt-in 路徑確為 Rust direct replacement，
預設路徑仍維持 C fallback。

## 本次工作階段（2026-07-13 Foundation `str_util` direct replacement）

- 新增 Cargo `str-util-only` feature 與 `ffi_str_util_only.rs`，直接匯出既有 `str_util.h` public ABI。
- Rust direct adapter 重用既有 byte-level `cbm_rs_*` helper；所有 arena-owned 結果透過 C `cbm_arena_alloc` 配置，`path_ext` / `path_base` 保留 borrowed pointer contract，Rust 不接管 `CBMArena` ownership。
- `Makefile.cbm` 在 `CBM_USE_RUST_STR_UTIL_ONLY=1` 時排除 `src/foundation/str_util.c`，新增隔離 direct foundation/production gate，並接入 `rust-ci`、`rust-ci-tests` 與 all-optin direct flags。
- 已驗證：default foundation 325 passed、direct FFI archive smoke、direct foundation 325 passed、direct production link 與 `codebase-memory-mcp --version`；direct link line 不含 `src/foundation/str_util.c`。
- 此切片只替代 string/path helper compilation unit；`CBMArena` allocator、其他 foundation runtime、pipeline/store/MCP/server lifecycle 與整體 Rust-backed default path 仍未完成。

## 本次工作階段（2026-07-13 全組合 direct gate）

- `make -f Makefile.cbm rust-all-optin-test` 的 wrapper 與 direct 變體皆完成完整 runner，結果均為 `5869 passed`。
- 兩個變體的 production binary 皆成功連結，`--version` 回報 `codebase-memory-mcp dev`。
- direct compile command 明確排除 `src/foundation/str_util.c` 與 `src/cli/progress_sink.c`；這兩個 C compilation unit 由 Rust direct ABI 實作取代。
- 此 gate 證明新增切片可與既有 Rust opt-in slices 並存，不代表整個 C backend 已移除；其餘 store、Cypher、MCP、pipeline orchestration、Tree-sitter/LSP、watcher、UI 與平台相容層仍須依序遷移。

## 本次工作階段（2026-07-13 foundation hash table direct replacement）

- 新增 `hash-table-only` Rust FFI，完整實作 `cbm_ht_create/free/set/get/has/get_key/delete/count/foreach/clear`。
- 保留既有 borrowed-key contract：Rust 複製 bytes 作 hash key，但回傳與保存的 canonical key pointer 仍是呼叫端指標；overwrite 會更新該 pointer，不會釋放 key 或 value。
- `CBM_USE_RUST_HASH_TABLE_ONLY=1` 會排除 `src/foundation/hash_table.c`，並納入 `rust-hash-table-only-test` 與 all-optin direct flags。
- 專用 gate：default/direct foundation runner 均為 `325 passed`，direct link line 排除 `src/foundation/hash_table.c`，production binary `--version` 成功。

## 2026-07-13 hash table 全矩陣證據

`rust-all-optin-test` 的 wrapper 與 direct 變體均完成 `5869 passed`；direct 編譯命令同時省略 `src/foundation/hash_table.c`、`src/foundation/str_util.c` 與 `src/cli/progress_sink.c`，並成功完成 production link 與版本檢查。這確認 hash table direct 實作可與現有 Rust opt-in slices 共同運作。

## 2026-07-13 diagnostics formatter direct replacement

新增 `diagnostics-format-only` direct slice。Rust 接管 `CBM_DIAGNOSTICS` env 判斷、diagnostics 路徑、JSON 與 NDJSON formatter；C 仍保留 atomic query stats、writer thread、mimalloc 統計、FD 掃描、trajectory append 與檔案 I/O。default/direct foundation gate 均為 `325 passed`，direct FFI smoke 與 production link/version 皆成功。

## 2026-07-13 store language map direct replacement

- 新增 `CBM_USE_RUST_STORE_LANGUAGE_MAP_ONLY=1` direct gate；`src/store/store.c` 的副檔名到語言分類查找改由既有 Rust `cbm_rs_store_ext_lang_kind_v1` 取得表格索引，C 僅保留靜態語言字串表作為既有 store ABI 的 borrowed 回傳來源。
- default 與 direct `store_arch mcp` focused gate 均為 `189 passed`；direct FFI smoke、production link 與 `--version` 均通過。
- 啟用目前全部 opt-in/direct flags 的 wrapper 與 direct runner 各為 `5869 passed`；direct production link 與 `--version` 均通過。

## 2026-07-13 pipeline module-directory direct replacement

- 新增 `CBM_USE_RUST_PIPELINE_MODULE_DIR_ONLY=1` direct gate，將 `src/pipeline/pass_parallel.c` 的 `pp_module_is_dir` 純語言分類切換至既有 Rust `cbm_rs_pipeline_is_module_dir_v1`；worker、registry 與 LSP 生命週期仍保留 C。
- default 與 direct pipeline focused gate 均為 `221 passed`；direct FFI smoke、production link 與 `--version` 均通過。

- 本切片加入 all-optin direct flag 後，wrapper/direct 全矩陣均成功完成；direct production link 與 `--version` 均通過。

## 2026-07-13 platform path direct replacement

- 新增 `CBM_USE_RUST_PLATFORM_PATH_ONLY=1` direct gate，移除 `src/foundation/platform.c` Windows 與非 Windows 兩份 `cbm_normalize_path_sep` C normalization body，改由 Rust 原地 buffer ABI 執行；其他 platform filesystem/runtime 保留 C。
- default/direct foundation focused gate 均為 `325 passed`；direct FFI smoke、production link 與 `--version` 均通過。

- 加入 platform-path-only 後，all-optin wrapper/direct runner 均成功，兩個 production binary 均完成 link 並回報 `codebase-memory-mcp dev`。

## 2026-07-13 compat thread direct replacement

- 新增 `CBM_USE_RUST_COMPAT_THREAD_ONLY=1` direct gate，讓 Rust 直接處理 thread effective stack size 與 aligned allocation precheck；pthread、mutex、`posix_memalign`/`_aligned_malloc` 和 free ownership 保留 C。
- default/direct foundation focused gate 均為 `325 passed`；direct FFI smoke、production link 與 `--version` 均通過。

- 加入 compat-thread-only 後，all-optin wrapper/direct runner 均成功，兩個 production binary 均完成 link 並回報 `codebase-memory-mcp dev`。

## 2026-07-13 MCP codec direct replacement

- 新增 `CBM_USE_RUST_MCP_CODEC_ONLY=1` direct gate，`src/mcp/mcp.c` 的 JSON-RPC parse/format、tool metadata/list/call argument codec、content-length、URI、validation、search/result formatting 與錯誤訊息等既有 Rust-backed branches 直接排除 C fallback；MCP handler、yyjson document 與 store/pipeline runtime 保留 C。
- default/direct MCP focused gate 均為 `136 passed`；direct FFI smoke、production link 與 `--version` 均通過。

- 加入 MCP codec-only 後，all-optin wrapper/direct runner 均成功，兩個 production binary 均完成 link 並回報 `codebase-memory-mcp dev`。

## 2026-07-13 Cypher classifier direct replacement

- 新增六組 direct gates：`CBM_USE_RUST_CYPHER_LEX_SINGLE_CHAR_ONLY`、`CBM_USE_RUST_CYPHER_LEX_TWO_CHAR_ONLY`、`CBM_USE_RUST_CYPHER_AGG_FUNC_ONLY`、`CBM_USE_RUST_CYPHER_STRING_FUNC_ONLY`、`CBM_USE_RUST_CYPHER_SCALAR_FUNC_ONLY`、`CBM_USE_RUST_CYPHER_MULTIARG_FUNC_ONLY`。
- 這些 gates 直接排除 Cypher lexer/function index 的 C fallback；parser、evaluator、SQLite query execution 保留 C。
- default/direct Cypher focused gate 均為 `144 passed`；direct FFI smoke、production link 與 `--version` 均通過。

- 加入六組 Cypher classifier only flags 後，all-optin wrapper/direct runner 均成功，兩個 production binary 均完成 link 並回報 `codebase-memory-mcp dev`。

## 2026-07-13 compat regex direct replacement

- 新增 `CBM_USE_RUST_COMPAT_REGEX_ONLY=1` direct gate，直接排除 known-flags、match-capacity、match-status 三個 C helper；regex compile/execute 與平台 ABI 保留 C。
- default/direct foundation focused gate 均為 `325 passed`；direct FFI smoke、production link 與 `--version` 均通過。

### 2026-07-13：compat_regex 直接替換切片

`src/foundation/compat_regex.c` 的 Rust opt-in 邊界已擴充為 `_ONLY` 變體。當啟用 `CBM_USE_RUST_COMPAT_REGEX_ONLY=1` 時，regex 狀態、match 上限與狀態轉換等既有 Rust helper 直接取代對應 C 純邏輯；實際 regex 引擎、編譯物件與生命週期仍由 C 管理。

`rust-compat-regex-only-test` 的預設與 direct foundation 測試均為 325 passed，並通過 FFI/link smoke 與正式版 `--version`。完整 `rust-all-optin-test` 的 wrapper/direct 矩陣及 direct 正式版連結均成功。

### 2026-07-13：log env parse 直接替換切片

`src/foundation/log.c` 的 level/format 環境值解析已支援 `CBM_USE_RUST_LOG_ENV_PARSE_ONLY=1`。direct 路徑排除對應 C parser，保留 C 的 `getenv`、logger 全域狀態、sink、輸出與格式設定；Rust 只承擔既有純解析 helper。

default/direct foundation 均為 325 passed；direct FFI smoke、正式版連結及 `--version` 均成功。此 gate 已加入完整 all-optin direct flags。

`rust-all-optin-test`（包含 `CBM_USE_RUST_LOG_ENV_PARSE_ONLY=1`）的 wrapper/direct runner 與正式版連結均成功，direct 正式版 `--version` 回報 `codebase-memory-mcp dev`。

### 2026-07-13：profile env 直接替換切片

`src/foundation/profile.c` 的 `CBM_PROFILE` 判斷已支援 `CBM_USE_RUST_PROFILE_ENV_ONLY=1`。direct 路徑排除 C env 判斷，保留 C 的 `getenv`、公開 profiling 狀態與時間/記錄生命週期；Rust 只承擔既有純 helper。

default/direct foundation 均為 325 passed；direct FFI smoke、正式版連結及 `--version` 均成功。

`rust-all-optin-test`（包含 `CBM_USE_RUST_PROFILE_ENV_ONLY=1`）的 wrapper/direct runner 與正式版連結均成功，direct 正式版 `--version` 回報 `codebase-memory-mcp dev`。

### 2026-07-13：platform env dirs 直接替換切片

`src/foundation/platform.c` 的 env-directory helper 已支援 `CBM_USE_RUST_PLATFORM_ENV_DIRS_ONLY=1`。direct 路徑排除對應 C fallback，保留平台系統呼叫、環境讀取與 C buffer/生命週期邏輯；Rust 只承擔既有 directory/value helper。

default/direct foundation 均為 325 passed；direct FFI smoke、正式版連結及 `--version` 均成功。

`rust-all-optin-test`（包含 `CBM_USE_RUST_PLATFORM_ENV_DIRS_ONLY=1`）的 wrapper/direct runner 與正式版連結均成功，direct 正式版 `--version` 回報 `codebase-memory-mcp dev`。

### 2026-07-13：pipeline route canonicalizer 直接替換切片

`src/pipeline/pass_route_nodes.c` 的 route path canonicalizer 已支援 `CBM_USE_RUST_PIPELINE_ROUTE_CANON_ONLY=1`。direct 路徑排除 C parameter scanner 與其專用 helper，直接使用 Rust canonicalizer；C 的 Route node 建立、graph buffer、edge re-target 與生命週期不變。

default/direct `route_canon pipeline` focused suite 均為 232 passed，包含 route canonical contract tests；direct FFI smoke、正式版連結及 `--version` 均成功。

`rust-all-optin-test`（包含 `CBM_USE_RUST_PIPELINE_ROUTE_CANON_ONLY=1`）的 wrapper/direct runner 與正式版連結均成功，direct 正式版 `--version` 回報 `codebase-memory-mcp dev`。

### 2026-07-13：pipeline git history 直接替換切片

`src/pipeline/pass_githistory.c` 的 trackable-file 判斷已支援 `CBM_USE_RUST_PIPELINE_GITHISTORY_ONLY=1`。direct 路徑排除對應 C path filter，libgit2/git log parsing、commit 資源、coupling 計算與 graph edge ownership 仍由 C 管理。

default/direct pipeline focused suite 均為 221 passed；direct FFI smoke、正式版連結及 `--version` 均成功。

`rust-all-optin-test`（包含 `CBM_USE_RUST_PIPELINE_GITHISTORY_ONLY=1`）的 wrapper/direct runner 與正式版連結均成功，direct 正式版 `--version` 回報 `codebase-memory-mcp dev`。

### 2026-07-13：pipeline git diff parsers 直接替換切片

`src/pipeline/pass_gitdiff.c` 的 range、name-status、hunks 三個純 parser 已分別支援 `_ONLY` direct gates。direct 路徑排除 C parser 與專用 static helpers，保留 C 的 pipeline caller、trackable-file integration、輸出結構 ownership 與後續 graph 行為。

default/direct pipeline focused suite 均為 221 passed；direct FFI smoke、正式版連結及 `--version` 均成功。

`rust-all-optin-test`（包含三個 pipeline gitdiff `_ONLY` flags）的 wrapper/direct runner 與正式版連結均成功，direct 正式版 `--version` 回報 `codebase-memory-mcp dev`。

### 2026-07-13：pipeline infrascan 直接替換切片

`src/pipeline/pass_infrascan.c` 的 Docker/Compose/env/cloudbuild/kustomize/shell 檔案識別已支援 `CBM_USE_RUST_PIPELINE_INFRASCAN_ONLY=1`。direct 路徑排除對應 C filename classifier 與 `to_lower` helper；secret detection、manifest content scan、解析與 graph ownership 仍由 C 管理。

default/direct pipeline focused suite 均為 221 passed；direct FFI smoke、正式版連結及 `--version` 均成功。

### 測試檔偵測的 `_ONLY` 接線修正（2026-07-13）

- `src/pipeline/pass_tests.c` 的測試路徑轉換器現在在 `CBM_USE_RUST_PIPELINE_TEST_DETECT_ONLY` 下也直接呼叫既有 Rust ABI；wrapper 模式仍保留 C fallback。
- 修正 `Makefile.cbm`，將 `_ONLY` 巨集傳給 C 編譯器，並在 `pipeline-test-detect-only` feature 啟用時連結 `cbm_core` staticlib。
- 驗證：`rust-pipeline-test-detect-optin-test` 與 `rust-pipeline-test-detect-only-test` 各 `221 passed`。

### Linux cgroup helper direct replacement（2026-07-13）

- `src/foundation/system_info.c` 的 CPU/memory cgroup 解析在 `CBM_USE_RUST_PLATFORM_CGROUP_ONLY` 下改由既有 Rust ABI 完整提供；Linux C 檔案解析 helper 僅在 wrapper/default 路徑編譯。
- `Makefile.cbm` 新增 `_ONLY` CFLAGS、staticlib 選擇與 `rust-platform-cgroup-only-test`，並將全量 direct flags 接到 Rust cgroup。
- 驗證：default/direct foundation 各 `325 passed`，direct FFI smoke、正式版連結與 `codebase-memory-mcp dev` 均成功。

### cgroup 接線後全量矩陣

`make -f Makefile.cbm rust-all-optin-test` 已重新通過（exit code 0）；wrapper/direct test-runner、wrapper/direct production link 與兩個 `codebase-memory-mcp dev` 版本輸出均成功，direct 變體包含 `CBM_USE_RUST_PLATFORM_CGROUP_ONLY=1`。

### Registry test-QN classifier direct replacement（2026-07-13）

- `src/pipeline/registry.c` 的 `is_test_qn()` 在 `CBM_USE_RUST_PIPELINE_REGISTRY_TEST_QN_ONLY` 下只使用既有 Rust classifier，不再編譯 C fallback；完整 registry resolver/cache 仍維持 C，避免混淆切片範圍。
- 新增 `rust-pipeline-registry-test-qn-only-test`，涵蓋 default/direct registry、FFI、正式版連結與版本輸出。
- 驗證：default registry `53 passed`，direct gate exit code 0。

### Registry classifier 接入後全量回歸

`rust-all-optin-test` 已重新通過（exit code 0）；wrapper/direct test-runner 與 production version 均成功，direct flags 包含 `CBM_USE_RUST_PIPELINE_REGISTRY_TEST_QN_ONLY=1`。

## 2026-07-15：CLI semver comparator Rust slice

新增 rust/cbm-core/src/cli/version.rs，以 byte-level parser 對齊
src/cli/cli.c 的 cbm_compare_versions()：保留 v/V 前綴、三段版本、
缺段補零與 prerelease 排序；Rust FFI 的 null input 明確視為空版本。

CBM_USE_RUST_CLI_VERSION=1 使用 Rust wrapper ABI；
CBM_USE_RUST_CLI_VERSION_ONLY=1 啟用 cli-version-only feature，C
semver parser 不再編譯並改使用 Rust direct ABI。CLI 版本狀態、install/update
與檔案系統副作用仍由 C 管理。新增 FFI smoke 及 default/wrapper/direct
Make target；本輪尚未執行驗證 gate，不宣稱此切片已通過。


## 2026-07-15：discover 字串 helper Rust slice

完成四個 deterministic discover helper 的 Rust opt-in 接線：NULL 結尾表格查找、suffix 判斷、
substring 查找與 ASCII 不分大小寫比較。Rust 只取代純字串運算，保留 discover 的目錄遞迴、
檔案讀取、Git 全域 excludes 解析與語言偵測 runtime 在 C。

C fallback 已從 `src/discover/discover.c` 拆至
`src/discover/discover_string_helpers.c`。default 與 wrapper 仍編譯該 unit；
`CBM_USE_RUST_DISCOVER_STRING_HELPERS_ONLY=1` 時
`DISCOVER_STRING_HELPERS_SRCS` 為空，direct production link 排除它並由 Rust staticlib
匯出四個同名 C ABI。

2026-07-15 已驗證：`cargo test -p cbm-core string_helpers_match_discover_contract --locked`
為 `1 passed`；default/wrapper/direct FFI smoke 均成功；三種組態的 `discover` suite 各
`85 passed`；wrapper/direct production link 與 `--version` 均成功。direct link line 不含
`src/discover/discover_string_helpers.c`。這不是完整 Discover migration，預設仍保留 C fallback。


## 2026-07-15：watcher polling interval Rust slice

將 watcher 的 adaptive poll interval 純計算移至 `rust/cbm-core/src/watcher.rs`：5000 ms
基準、每 500 個檔案增加 1000 ms、60000 ms 上限，並保留負數輸入的既有整數除法行為。未搬移
watcher 的 Git、時間、執行緒與生命週期。

2026-07-15 再將 C fallback 自 `src/watcher/watcher.c` 拆至
`src/watcher/poll_interval.c`。`CBM_USE_RUST_WATCHER_POLL_INTERVAL_ONLY=1` 時，
`WATCHER_POLL_INTERVAL_SRCS` 為空，故 direct link 排除該 C unit；C watcher 其餘部分以
`cbm_watcher_poll_interval_ms(0)` 取得基準值，避免跨檔重複常數。

已驗證 Rust unit `1 passed`、隔離 default/direct full runner 各 `5868 passed`、
default/direct `test-rust-ffi`、wrapper FFI 與 watcher suite `53 passed`、direct/wrapper
release link 與 `--version`。direct production link line 不含 `src/watcher/poll_interval.c`，
因此此處是 opt-in Rust direct replacement；預設仍編譯 C fallback。


## 2026-07-15：Pipeline SvelteKit file kind Rust slice

將 `pass_route_nodes.c` 的 `sveltekit_file_kind` 純 path/name classifier 移至
`rust/cbm-core/src/pipeline/route.rs`，並把 C fallback 拆至
`src/pipeline/sveltekit_file_kind.c`／`.h`。`pass_route_nodes.c` 只保留呼叫
`cbm_pipeline_sveltekit_file_kind()` 的公開 bridge；SvelteKit route synthesis、File/Route graph
traversal、graph mutation 與 ownership 均未搬移。

契約：nullable NUL 字串必須含有精確 `/routes/`，最後 basename 以 byte-level 比對六個固定檔名。
`+server.ts`／`.js` 回 1，`+page.server.ts`／`.js` 回 2，`+layout.server.ts`／`.js` 回 3，其他與
NULL 回 0。Rust 僅分類輸入、不配置且不保存輸入指標。

已加入 stable v1 FFI、wrapper opt-in、feature-gated direct ABI、Rust unit、C FFI smoke、C
classifier regression 與 Makefile gates。預設與 wrapper 仍編譯 C fallback；`ONLY=1` 的 source
selector 排除 `src/pipeline/sveltekit_file_kind.c`，改由 Rust 匯出同名 C API。

初始 direct 組態曾因 `pass_route_nodes.c` 仍呼叫未定義的 local static helper 而無法編譯；現已由
公開 bridge 消除該依賴。已驗證聚焦 Rust unit `1 passed`、完整 Rust suite `307 passed`、隔離
default/wrapper/direct FFI smoke、三種路徑的 pipeline suite 各 `227 passed`，以及 wrapper/direct
production link 與 `--version`。direct Make 展開及 production link line 不含
`src/pipeline/sveltekit_file_kind.c`；Clippy、Rust fmt 與本切片 C 格式亦通過。未以此宣稱完整
pipeline route-node 或完整 CI 已完成。

2026-07-16 current-revision 重跑已完成：在六個 basename 測試與 make -Bn source-negative 下，
`rust-pipeline-sveltekit-file-kind-optin-test` 與 `rust-pipeline-sveltekit-file-kind-only-test`
均通過。default／wrapper／direct 皆實際執行 test-rust-ffi；pipeline suite 各 227 passed；
wrapper／direct production `--version` 成功；`PIPELINE_SVELTEKIT_FILE_KIND_SRCS =` 為空，且
forced dry-run production recipe 不含 `src/pipeline/sveltekit_file_kind.c`。另通過
`cargo test -p cbm-core --locked`（307 passed）、clippy、fmt 與 SvelteKit C 原始檔
clang-format。完整 `scripts/test.sh` 尚未重跑。現況以 docs/rust-refactor-current-handoff.md
為準。


## 2026-07-15：Pipeline SvelteKit server method Rust slice

將 `pass_route_nodes.c` 的 `sveltekit_server_method` 固定名稱映射移入
`rust/cbm-core/src/pipeline/route.rs`。既有七個 HTTP method、`fallback` 對應 `ANY`、
未知名稱與 null 回傳 null 的行為均保留；C fallback 已拆至
`src/pipeline/sveltekit_server_method.c`，route synthesis 與 graph 處理沒有搬移。

已加入 stable v1 FFI、wrapper opt-in、feature-gated direct ABI、Rust unit、C FFI smoke 與
Makefile gates。預設與 wrapper 仍編譯 C fallback；`ONLY=1` 的 source selector 排除
`src/pipeline/sveltekit_server_method.c`，改由 Rust 匯出同名 C API。

已驗證 Rust unit `1 passed`、隔離 default/wrapper/direct FFI smoke、三種路徑的 pipeline
suite 各 `222 passed`，以及 wrapper/direct production link 與 `--version`。direct
production link line 不含 `src/pipeline/sveltekit_server_method.c`；未以此宣稱完整 pipeline
或完整 CI 已完成。


## 2026-07-15：CLI hook token Rust slice

將 src/cli/hook_augment.c 的 ha_extract_token 純掃描器移至 rust/cbm-core/src/cli/hook.rs。保留 ASCII identifier 規則、最短 4、最大 96 與 caller buffer 截斷行為；不搬移 hook 的 JSON、MCP、timeout 或 I/O runtime。

已加入 stable v1 FFI、wrapper opt-in、feature-gated direct ABI、Rust unit、C FFI smoke 與 Makefile gates。default C helper 未移除。

驗證狀態：本輪尚未執行建置、測試、連結或 lint；後續需先執行 rust-cli-hook-token-optin-test 與 rust-cli-hook-token-only-test。


## 2026-07-15：Git path absolute Rust slice

將 `src/git/git_context.c` 的 `path_is_absolute` 純平台 classifier 移至
`rust/cbm-core/src/git_context.rs`，並把 C fallback 拆至 `src/git/path_absolute.c`／`.h`。
`git_context.c` 只保留呼叫 `cbm_git_path_is_absolute()` 的公開 bridge；git command、檔案 I/O、
repository state、root derivation 與字串 ownership 均未搬移。

契約：NULL／空字串為 false，首 byte 為 `/` 為 true；Windows 只將 ASCII 英文字母後接 `:`
視為 true，因此 `C:` 與 `C:project` 為 true，數字或非 ASCII drive byte、UNC 反斜線路徑為
false。Rust 僅分類輸入、不配置且不保存輸入指標。

已加入 stable v1 FFI、wrapper opt-in、feature-gated direct ABI、Rust unit、C FFI smoke 與
Makefile gates。預設與 wrapper 仍編譯 C fallback；`ONLY=1` 的 source selector 排除
`src/git/path_absolute.c`，改由 Rust 匯出同名 C API。

已驗證聚焦 Rust unit `4 passed`、完整 Rust suite `307 passed`、隔離 default/wrapper/direct
FFI smoke、三種路徑的 pipeline suite 各 `226 passed`，以及 wrapper/direct production link 與
`--version`。direct Make 展開及 production link line 不含 `src/git/path_absolute.c`；Clippy、
Rust fmt 與本切片 C 格式亦通過。未以此宣稱完整 Git runtime 或完整 CI 已完成。


## 2026-07-15：Git JSON escaped length Rust slice

將 `src/git/git_context.c` 的 `json_escaped_len` 純計算移至
`rust/cbm-core/src/git_context.rs`，並把 C fallback 拆至 `src/git/json_escaped_len.c`／`.h`。
保留 NULL、第一個 NUL、引號／反斜線／LF／CR／TAB、其餘控制 byte 與一般 byte 的長度規則；
結果不含結尾 NUL，最大成功值是 `INT_MAX - 1`，溢位時回傳 -1。`json_append_string()` 在配置前
拒絕負值，故 `needed + 1` 保持安全。Git metadata、JSON append、配置與 ownership 仍在 C。

已加入 stable v1 FFI、wrapper opt-in、feature-gated direct ABI、Rust unit、C FFI smoke 與
Makefile gates。預設與 wrapper 仍編譯 C fallback；`ONLY=1` 的 source selector 排除
`src/git/json_escaped_len.c`，改由 Rust 匯出同名 C API。

已驗證 Rust unit `4 passed`、隔離 default/wrapper/direct FFI smoke、三種路徑的 pipeline suite
各 `224 passed`，以及 wrapper/direct production link 與 `--version`。direct Make 展開與
production link line 不含 `src/git/json_escaped_len.c`；未以此宣稱完整 Git runtime 或完整 CI 已完成。


## 2026-07-15：Git trim newlines Rust slice

將 `src/git/git_context.c` 的 `trim_newlines` buffer mutation 接至
`rust/cbm-core/src/git_context.rs`，並把 C fallback 拆至 `src/git/trim_newlines.c`／`.h`。
既有的 C-owned、可寫 NUL 結尾 buffer 上尾端 CR/LF 清除語意保留；未搬移 Git command、capture、
malloc、buffer ownership 或 process lifecycle。

已加入 stable v1 FFI、wrapper opt-in、feature-gated direct ABI、Rust unit、C FFI smoke 與
Makefile gates。預設與 wrapper 仍編譯 C fallback；`ONLY=1` 的 source selector 排除
`src/git/trim_newlines.c`，改由 Rust 匯出同名 C API。

已驗證 Rust unit `1 passed`、隔離 default/wrapper/direct FFI smoke、三種路徑的 pipeline
suite 各 `223 passed`，以及 wrapper/direct production link 與 `--version`。direct
production link line 不含 `src/git/trim_newlines.c`；未以此宣稱完整 Git runtime 或完整 CI 已完成。


## 2026-07-15：Log path query Rust slice

將 `src/foundation/log.c` 的 `copy_path_without_query` C fallback 拆至
`src/foundation/log_path.c`／`.h`；`log.c` 保留 HTTP logging、logger state、I/O 與 output
stream，只呼叫公開 bridge。Rust stable v1 ABI 在 `ffi.rs` 以容量有界的 raw copy 實作：最多
掃描／寫入 `outsz - 1` bytes，於首個 NUL、`?` 或 `#` 停止並 NUL 結尾；nullable `path`、
`outsz=0/1` 與不寫入 NULL output 的既有行為均保留。`out` 若非 NULL 不得與 `path` 重疊。

已加入 stable v1 FFI、`CBM_USE_RUST_LOG_COPY_PATH` wrapper opt-in 與
`log-copy-path-only` direct ABI。預設與 wrapper 仍編譯 C fallback；`ONLY=1` 的 source
selector 排除 `src/foundation/log_path.c`，改由 Rust 匯出同名 C API。

已驗證 Rust unit `1 passed`、隔離 default/wrapper/direct FFI smoke、三種路徑的 `log` suite
各 `16 passed`，以及 wrapper/direct production link 與 `--version`；direct production link
line 不含 `src/foundation/log_path.c`。未以此宣稱完整 logger、foundation runtime 或完整 CI 已完成。


## 2026-07-15：Pipeline JSON property Rust slice

將 src/pipeline/pass_route_nodes.c 的 extract_json_prop 純 pattern/value parser 移入 rust/cbm-core/src/pipeline/route.rs。保留固定 key pattern、空值拒絕、buffer 不足拒絕與未進行 escape decode 的行為；route graph mutation 未搬移。

已加入 stable v1 FFI、wrapper opt-in、feature-gated direct ABI、Rust unit、C FFI smoke 與 Makefile gates。default C helper 未移除。

驗證狀態：本輪尚未執行建置、測試、連結或 lint；後續需先執行 rust-pipeline-json-prop-optin-test 與 rust-pipeline-json-prop-only-test。


## 2026-07-15：Pipeline URL path Rust slice

將 src/pipeline/pass_route_nodes.c 的 url_path() raw string scan 移入
rust/cbm-core/src/pipeline/route.rs。C fallback 已拆至 src/pipeline/url_path.c／.h，
route-node pass 僅經公開 cbm_pipeline_url_path() bridge 呼叫；預設與 wrapper 仍可使用 C
fallback，ONLY=1 source selector 則排除該 compilation unit，由 pipeline-url-path-only
匯出同名 direct ABI。未搬移 route graph、HTTP matching 或 graph mutation。

契約固定為第一個 :// 後第一個 / 的 byte-level scan，不宣稱 RFC URL 解析；無 scheme 與
已有 path 的結果保留 input 內部 pointer，僅無後續 slash 時回傳靜態根 path。Rust 不配置、
不保存 input pointer，caller 不得釋放回傳值。

已驗證 cargo test -p cbm-core --locked（307 passed）、clippy 與 fmt；隔離
default/wrapper/direct FFI smoke 均成功，三條路徑的 pipeline suite 各 225 passed，
wrapper/direct production link 與 --version 成功，direct Make selector、production link
line 與 source-negative gate 均確認排除 src/pipeline/url_path.c。這些 targeted gates 不代表
完整 pipeline 或完整 CI 已完成 Rust 化。


## 2026-07-15：Discover trailing separator Rust slice

將 `discover.c` 的 `has_trailing_sep` 純 path predicate 移至
`rust/cbm-core/src/discover.rs`，並把 C fallback 拆至 `src/discover/trailing_sep.c`／`.h`。
`discover.c` 只經公開 bridge `cbm_discover_has_trailing_sep()` 呼叫；path_join 的 C fallback
亦改呼叫此 bridge。Git path expansion、I/O 與遞迴掃描未搬移。

契約：nullable NUL 字串；NULL 與空字串回 false；尾端 `/` 或 `\\` 回 true。這是將 private
helper 公開化時凍結的防禦性契約，不是舊 static helper 對 NULL 呼叫 `strlen` 的 UB 等價宣稱。
Rust 只分類輸入、不配置且不保存輸入指標。

已加入 stable v1 FFI、wrapper opt-in、feature-gated direct ABI、Rust unit、C contract
regression、C FFI smoke（含 empty／NULL／direct 覆蓋）與 Makefile gates。預設與 wrapper 仍
編譯 C fallback；`ONLY=1` 的 `DISCOVER_TRAILING_SEP_SRCS` selector 排除
`src/discover/trailing_sep.c`，改由 Rust 匯出同名 C API。`RUST_ALL_OPTIN_DIRECT_FLAGS` 會
filter-out wrapper flag。

2026-07-16 已驗證：`rust-discover-trailing-sep-optin-test` 與
`rust-discover-trailing-sep-only-test` 均通過。default／wrapper／direct 皆實際執行
test-rust-ffi；discover suite 各 86 passed；wrapper／direct production `--version` 成功；
`DISCOVER_TRAILING_SEP_SRCS =` 為空，且 `make -Bn` production recipe 不含
`src/discover/trailing_sep.c`。另通過完整 Rust suite 307 passed、clippy、fmt 與本切片 C
格式檢查。完整 `scripts/test.sh` 尚未重跑。


## 2026-07-15：Discover path join Rust slice

將 src/discover/discover.c 的 path_join 純 buffer helper 移入 rust/cbm-core/src/discover.rs，保留 base/relative 空值、separator、capacity 與 path separator normalization 行為。Git path expansion 與 I/O 未搬移。

已加入 stable v1 FFI、wrapper opt-in、feature-gated direct ABI、Rust unit、C FFI smoke 與 Makefile gates。default C helper 未移除。

驗證狀態：本輪尚未執行建置、測試、連結或 lint；後續需先執行 rust-discover-path-join-optin-test 與 rust-discover-path-join-only-test。


### SimHash MinHash Jaccard helper slice

本輪將 MinHash signature 的固定寬度 Jaccard 相似度計算移至 Rust，C 端以 `CBM_USE_RUST_SIMHASH_JACCARD` opt-in。Rust 僅接收兩個各 64 個 `uint32_t` 的唯讀陣列，null 輸入回傳 `0.0`；Tree-sitter AST 擷取、xxHash、fingerprint 建立、LSH table 與其生命週期仍保留在 C。此為 helper slice，不代表 SimHash 子系統已完成 Rust 化。`rust-simhash-jaccard-optin-test` 現以分離 build 目錄依序跑 default C `simhash`、opt-in FFI smoke、opt-in `simhash`；2026-07-15 實測 default/opt-in 各 24 項通過，Rust `simhash` unit 4 項通過，完整 lint/CI 尚待執行。

MinHash helper slice 另涵蓋固定寬度 hex round-trip；Rust 不取得輸入或輸出 buffer 的 ownership，C wrapper 仍負責原有 API 邊界。


### Pipeline complexity JSON reader slice

本輪只搬移 complexity pass 的兩個 deterministic JSON readers，保留原有 key pattern、空白處理與 boolean 首字元判斷。Rust 不取得 graph buffer 或 node property ownership；此 helper slice 不代表 complexity propagation 已 Rust 化。Rust unit、FFI smoke 與 opt-in gate 已建立，尚未驗證。


### VMem page alignment slice

`src/foundation/vmem.c` 未納入產品 source list；page-round Rust 程式碼僅是非產品 FFI parity fixture，沒有產品 runtime replacement 證據。`rust-vmem-page-round-optin-test` 只執行 FFI parity smoke；2026-07-15 的 Rust `vmem` unit 2 項與 smoke 已通過，但不得將此項計入 C→Rust runtime replacement。

### Makefile opt-in gate 執行修正

`rust-pipeline-test-node-is-test-optin-test`、`rust-ast-profile-classifiers-optin-test`、`rust-pipeline-usages-json-optin-test`、`rust-semantic-token-classifiers-optin-test` 現會執行 FFI smoke 與 `pipeline` suite；`rust-ui-layout-helpers-optin-test` 執行 `ui`，`rust-ui-httpd-helpers-optin-test` 執行 `httpd`。2026-07-15 實測四個 pipeline gate 各 221 項、UI layout 15 項、HTTPD 32 項均通過。這只修正 gate 的執行行為，不改 ABI ownership，亦不構成子系統遷移。


### Pipeline pkgmap bounded text slice

本輪只搬移 manifest parser 的 bounded byte comparisons 與 line-prefix offset scan，避免讓 Rust 回傳借用 C source pointer。Rust 不持有 manifest buffer 或 package entry；其餘 manifest 格式解析與 package map merge 仍是 C runtime。Rust unit、FFI smoke 與 opt-in gate 已建立，尚未驗證。

Pkgmap path helper 的輸出採兩次呼叫 ABI：先取得長度，再由 C 配置 buffer 讓 Rust 寫入。此設計保留既有 C heap ownership，且不擴大到 manifest parser 的資源管理。


### Pipeline configlink helper slice

本輪只搬移 manifest filename、dependency section、Cargo dependency section 與 import match score 的 deterministic helper。Rust 接收唯讀 C strings，不持有 graph/node；configlink pass 的資料收集與 CONFIGURES edge 寫入仍保留 C。Rust unit、FFI smoke 與 opt-in gate 已建立，尚未驗證。


### Compile command tokenizer slice

本輪將 compile command string tokenizer 移至 Rust，保留原有引號移除、space/tab 分隔、4095-byte token cap 與 max-output 行為。Rust 產生的 token 交由 C 的既有釋放路徑回收；compile_commands JSON 與 `cbm_compile_flags_t` lifecycle 不變。Rust unit、FFI smoke 與 opt-in gate 已建立，尚未驗證。


### Cross-repo JSON property slice

本輪保留 cross-repo pass 的資料庫與 edge lifecycle，只搬移 key pattern、value scan 與 caller-buffer truncation。此 helper 的 empty-value 與 truncate 行為刻意不同於 route JSON parser，需以各自 C contract 驗證。Rust unit、FFI smoke 與 opt-in gate 已建立，尚未驗證。


### Pipeline enrichment tokenizer slice

本輪搬移 camelCase splitting、decorator syntax stripping、ASCII lowercase 與 stopword filtering；保留 256-byte decorator input cap、16 camel parts、max output 行為。Rust 不持有 graph 或 hash table state，token allocations 仍使用 C malloc-compatible ABI。Rust unit、FFI smoke 與 opt-in gate 已建立，尚未驗證。


### Calls import JSON helper slice

本輪只搬移 `local_name` property 的 exact pattern 與非空 value extraction，保留呼叫端 malloc ownership。Rust 不接觸 graph buffer、registry 或 import map；其餘 calls pass runtime 不變。Rust unit、FFI smoke 與 opt-in gate 已建立，尚未驗證。


### Envscan filename classifier slice

本輪只搬移 ASCII case-folded filename classification 與固定 file-type enum mapping，保留原有大小寫與 extension contract。Rust 不持有 filesystem handle、regex state 或 binding output；Rust unit、FFI smoke 與 opt-in gate 已建立，尚未驗證。


### LSP cross classifier slice

本輪搬移 TypeScript `.jsx`/`.d.ts` mode 判斷與 cross-file LSP language allowlist，保留 C bool output 與所有 resolver lifecycle。Rust unit、FFI smoke 與 opt-in gate 已建立，尚未驗證。

LSP label mapping 只在 Rust 回傳 allow/deny，C 保留原有 borrowed pointer 與 `CBMLSPDef` 建構，因此沒有跨 FFI ownership 變更。


### Semantic edges JSON reader slice

本輪只搬移 semantic metadata 的 scalar/array string readers，不做 JSON escape decode，也不接觸 semantic graph state。Rust unit、FFI smoke 與 opt-in gate 已建立，尚未驗證。

Pkgmap path composition 的 1023-byte bounded buffer 與 extension stripping 已在 Rust helper 對齊，未搬移 manifest 格式解析或 package map merge。

Configlink lowercase helper 對齊 C 的 output_size-1 truncation，維持 dependency buffers 的既有大小契約。

Cross-repo property formatter 已移至 Rust helper；SQLite lookup、雙向 edge emission 與 store ownership 仍在 C，formatter 不取得任何外部資源。

Compile command path resolution 已在 Rust 對齊 absolute/relative join 與 4095-byte bounded buffer，未搬移 JSON parser 或 flag struct。

### 2026-07-15：pipeline infrascan JSON array cleaner opt-in

新增 `pipeline::infrascan::clean_json_brackets()` 與 `cbm_rs_pipeline_clean_json_brackets_v1`，並讓 `cbm_clean_json_brackets()` 在既有 `CBM_USE_RUST_PIPELINE_INFRASCAN` opt-in（含 direct gate）下使用 Rust。切片只涵蓋 bounded JSON array 字串清理；infrascan parser、內容掃描、secret detection、manifest 判斷、graph ownership 與副作用仍留在 C。Rust unit、C FFI smoke 與 default/opt-in gate 已補上，但尚未執行驗證。

### 2026-07-16：pipeline definitions JSON escape/length true-source

將 `pass_definitions.c` 的純 JSON escape/escaped length fallback 拆至
`src/pipeline/definitions_json.c`／`.h`，並保留 `pass_definitions.c` 的 caller、JSON field atomic
append、Tree-sitter/graph 與 ownership lifecycle。已加入 stable wrapper FFI、公開 bridge、
`CBM_USE_RUST_PIPELINE_DEFINITIONS_JSON_ONLY=1` feature-gated direct ABI 與 source selector；
ONLY 時 production build 不含 `src/pipeline/definitions_json.c`。

驗證通過：`rust-pipeline-definitions-json-optin-test` 與
`rust-pipeline-definitions-json-only-test`；default/wrapper/direct FFI smoke、pipeline suite 各
`227 passed`、wrapper/direct production `--version`、selector 空與 `make -Bn`
source-negative。產品 default 仍是 C11；此切片不代表 definitions pass 或 extraction runtime 已遷移。

### 2026-07-16：pipeline similarity fingerprint parser true-source

將 `pass_similarity.c` 的純 fingerprint properties parser fallback 拆至
`src/pipeline/similarity_fp.c`／`.h`；`pass_similarity.c` 僅保留 similarity pass 的 caller、
LSH index、threshold、edge/graph ownership 與 side effects。已加入 stable wrapper FFI、
公開 bridge、`CBM_USE_RUST_PIPELINE_SIMILARITY_FP_ONLY=1` direct ABI 與 source selector；
ONLY 時 production build 不含 `src/pipeline/similarity_fp.c`。

驗證通過：`rust-pipeline-similarity-fp-optin-test` 與
`rust-pipeline-similarity-fp-only-test`；default/wrapper/direct FFI smoke、pipeline suite 各
`227 passed`、wrapper/direct production `--version`、selector 空與 `make -Bn`
source-negative。產品 default 仍是 C11；此切片不代表 similarity runtime 已遷移。

### 2026-07-17：pipeline infrascan JSON cleaner true-source

將 `pass_infrascan.c` 的 JSON bracket cleaner fallback 拆至
`src/pipeline/infrascan_json.c`／`.h`；`pass_infrascan.c` 僅保留 infrascan caller、
Dockerfile command handling、graph ownership 與 pipeline side effects。已加入 stable wrapper
FFI、公開 bridge、`CBM_USE_RUST_PIPELINE_INFRASCAN_JSON_ONLY=1` direct ABI 與 source
selector；ONLY 時 production build 不含 `src/pipeline/infrascan_json.c`。

此切片保留既有語意：JSON array 移除引號、逗號轉空白、折疊 ASCII 空白與 tab 並移除尾端
空白；非 array 維持截斷複製。驗證通過：
`rust-pipeline-infrascan-json-optin-test` 與
`rust-pipeline-infrascan-json-only-test`；default/wrapper/direct FFI smoke、pipeline suite
各 `227 passed`、wrapper/direct production `--version`、selector 空與 `make -Bn`
source-negative。產品 default 仍是 C11。

### 2026-07-15：CLI archive byte helpers opt-in

新增 `cli_archive::{is_tar_end_of_archive,zip_read_u16}` 與兩個 stable FFI，並以 `CBM_USE_RUST_CLI_ARCHIVE_HELPERS=1` 讓 CLI archive parser 使用 Rust 純 byte helper。gzip、tar/zip entry parsing、memory ownership、檔案 I/O 與安裝 side effects 仍留 C。Rust unit、C FFI smoke 與 targeted CLI gate 已補上，尚未執行驗證。

CLI archive slice 另涵蓋 `zip_read_u32` 與對應 FFI，仍與 `CBM_USE_RUST_CLI_ARCHIVE_HELPERS` 共用，尚未執行驗證。

### 2026-07-15：pipeline K8s text helpers opt-in

新增 `pipeline_k8s::{basename_offset,indent,split_kv}` 與 stable FFI，並以 `CBM_USE_RUST_PIPELINE_K8S_HELPERS=1` 讓 `pass_k8s.c` 委派純文字 helper。K8s YAML path stack、manifest scanning、label matching、檔案 I/O、graph ownership 與 side effects 仍留 C。Rust unit、C FFI smoke 與 targeted pipeline gate 已補上，尚未執行驗證。

### 2026-07-15：pipeline incremental edge predicate opt-in

新增 `pipeline_incremental::edge_type_is_recomputed()` 與 `cbm_rs_pipeline_incremental_edge_type_is_recomputed_v1`，以 `CBM_USE_RUST_PIPELINE_INCREMENTAL_EDGE=1` 委派固定 edge-type predicate。只搬移四個字串比對；incremental purge、graph traversal、registry seed、SQLite persistence 與 lifecycle 仍留 C。Rust unit、C FFI smoke 與 targeted pipeline gate 已補上，尚未驗證。

### 2026-07-15：pipeline parallel JSON escape opt-in

新增 `pipeline_json` 共用 helper 與 `cbm_rs_pipeline_parallel_json_escape_char`／`cbm_rs_pipeline_parallel_json_escaped_len`，以 `CBM_USE_RUST_PIPELINE_PARALLEL_JSON=1` 委派 parallel pass 的純 JSON escape。2026-07-16 已將 C fallback 拆為 `src/pipeline/parallel_json.c/.h`，並新增 `CBM_USE_RUST_PIPELINE_PARALLEL_JSON_ONLY=1`、`pipeline-parallel-json-only` direct ABI、空 selector 與 `make -Bn` source-negative gate 定義。公開 bridge 的新防禦性契約為：nullable value 的 escaped length 為 0；output 為 NULL 或容量不足時不寫入、但回傳所需長度；raw control byte 仍降級為空白。atomic append、data flow、graph ownership 與 side effects 仍留 C。**2026-07-16 true-source：** `rust-pipeline-parallel-json-optin-test` 與 `rust-pipeline-parallel-json-only-test` 均通過：default/wrapper/direct FFI smoke、pipeline suite 各 `227 passed`、wrapper/direct production `--version`、`_ONLY` selector 為空，以及 `make -Bn` source-negative 不含 `src/pipeline/parallel_json.c`。進度 **12/18 -> 13/18**。

### 2026-07-16：discover language `.m` marker helpers true-source

將 `language.c` 的 `.m` 純 marker predicate fallback 拆至
`src/discover/language_markers.c`；`language.c` 僅保留檔案讀取、marker 優先順序、language
selection 與 fallback。已加入 stable wrapper FFI、公開 bridge、
`CBM_USE_RUST_DISCOVER_LANGUAGE_MARKERS_ONLY=1` direct ABI 與 source selector；ONLY 時
production build 不含 `src/discover/language_markers.c`。

驗證通過：`rust-discover-language-markers-optin-test` 與
`rust-discover-language-markers-only-test`；default/wrapper/direct FFI smoke、discover suite 各
`87 passed`、wrapper/direct production `--version`、selector 空與 `make -Bn`
source-negative。產品 default 仍是 C11；此切片不代表 discover runtime 已遷移。

### 2026-07-15：pipeline incremental registry label predicate opt-in

新增 `pipeline_incremental::label_is_registry_symbol()` 與 `cbm_rs_pipeline_incremental_label_is_registry_symbol_v1`，以 `CBM_USE_RUST_PIPELINE_INCREMENTAL_REGISTRY_LABEL=1` 委派 registry seed label predicate。只搬移固定 label 集合判斷；graph traversal、registry mutation、SQLite persistence 與 incremental lifecycle 仍留 C。Rust unit、C FFI smoke 與 targeted pipeline gate 已補上，尚未驗證。

### 2026-07-15：pipeline semantic Go suffix predicate opt-in

新增 `cbm_rs_pipeline_semantic_fp_ends_with_v1`，以 `CBM_USE_RUST_PIPELINE_SEMANTIC_FP_SUFFIX=1` 委派 semantic pass 的 Go file suffix predicate，實作重用 `discover::ends_with`。interface matching、graph ownership 與 semantic side effects 仍留 C；Rust unit coverage、C FFI smoke 與 targeted gate 已補上，尚未驗證。

### 2026-07-15：store architecture JSON property extractor opt-in

新增 `store::arch_helpers::extract_json_string_prop()` 與 `cbm_rs_store_arch_json_prop_len_v1`／`cbm_rs_store_arch_json_prop_copy_v1`，以 `CBM_USE_RUST_STORE_ARCH_JSON_PROP=1` 委派 architecture route JSON property extraction。Rust 不持有 C result allocation；SQLite query、architecture aggregation、store ownership 與 runtime 仍留 C。Rust unit、C FFI smoke 與 targeted store gate 已補上，尚未驗證。
