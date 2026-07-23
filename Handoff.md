# #81 Rust true-source 權威交接（2026-07-23）

> 本區塊為目前權威交接，覆蓋本文件下方所有較早的 #80／#79 基線與下一步。true-source 為
> **76 / 76**，完成 **#29–#81**；**#29–#81 不得重跑或改寫**。預設產品仍是 C11，Rust 僅由明確
> opt-in／direct-only 取代單一 pure helper。

- 最新 #81 是 `store-safe-props`，前一項為 #80 `pipeline-similarity-file-ext`。C fallback 為
  `src/store/safe_props.c/.h`；`src/store/store.c` 仍是 host。兩個 node／edge property write consumer
  都經 public bridge，最後傳入 `bind_text()`。
- ABI：`const char *cbm_store_safe_props(const char *s);`；Rust v1：
  `const char *cbm_rs_store_safe_props_v1(const char *s);`。wrapper macro 為
  `CBM_USE_RUST_STORE_SAFE_PROPS`；direct macro 為 `CBM_USE_RUST_STORE_SAFE_PROPS_ONLY`；Cargo feature 為
  `store-safe-props-only`。
- wrapper 僅於 v1 回傳 `NULL` 時 fallback C；direct 由 Rust 匯出同名 public ABI。ONLY selector 排除
  `safe_props.c`，但保留 `store.c`。
- 凍結行為：`s == NULL` 回 non-NULL、NUL 終止、process-lifetime static `"{}"`，且不解參；非 NULL 僅讀
  首 byte，首 byte NUL 同樣回 static `"{}"`，否則回完全相同的 borrowed input pointer，絕不掃描字串。
  真正傳入 `bind_text()` 時才適用 legacy 有效 NUL 終止 C 字串 precondition。行為為 byte-oriented，non-UTF-8
  可用，`x\0tail` 回原 pointer。
- 無 allocation、mutation、I/O、locale、pointer 保存或 ownership transfer；呼叫端不得 free。static `"{}"`
  的跨 mode pointer identity 不保證；非空 input 的 pointer identity 必須保留。
- 已驗證：fmt、Rust **1 passed，340 filtered**、direct clippy、FFI default／wrapper／direct、store_nodes
  三態各 **56 passed**、production `--version`=`codebase-memory-mcp dev` x3。direct
  `STORE_SAFE_PROPS_SRCS =`，`make -Bn` 不含 `src/store/safe_props.c`、保留 `src/store/store.c`；
  source-negative 證據為 **18**。完整 `scripts/test.sh` 未跑。
- 保留 deferred：`pipeline-complexity-json` 與 content_length value／raw 的 `strtol` 語義。#82 必須重新
  inventory Pipeline／MCP／Store pure helpers，再凍結下一個最小切片。

# Rust 重構交接文件

## #79 Rust true-source 權威交接（2026-07-23）

本區塊優先於本文件下方的較早工作紀錄。true-source 基線為 **74 / 74**，完成範圍為 **#29–#79**；新 session
不得重跑、改寫或重新認定 **#29–#79**。預設產品仍為 C11，Rust 僅以 opt-in／direct-only 接管已凍結的 pure
helper。

最新 #79 為 `store-package-list-contains`。其 frozen ABI 為：

```c
bool cbm_store_package_list_contains(const char *pkg, char **list, int count);
bool cbm_rs_store_package_list_contains_v1(const char *pkg, char **list, int count);
```

`count <= 0` 回 false，且不解參 `pkg`、`list`。正 count 時，`pkg`、`list` 與實際存取 entries 都必須是有效、
非 NULL、NUL 終止 C 字串，屬既有 legacy precondition。掃描按 count 順序，僅比較 first-NUL 前 bytes，行為為
case-sensitive、byte-exact `strcmp` 等價；命中回 true，否則 false。不得配置、修改、I/O、使用 locale、保存
pointer 或轉移 ownership。

實作結構：C fallback 是 `src/store/package_list.c/.h`；`src/store/store.c` 保留為 host，兩個 `arch_layers`
consumer 均改走 public bridge。`CBM_USE_RUST_STORE_PACKAGE_LIST` wrapper 轉呼叫 v1；
`CBM_USE_RUST_STORE_PACKAGE_LIST_ONLY` 加上 Cargo `store-package-list-only` 時省略 C fallback，Rust 匯出同名
public ABI。production staticlib wiring 已由三態 gate 覆蓋。新 direct contract test 是
`package_list_contains_contract`，既有 `arch_layers` 測試保留為 consumer integration。

已通過：fmt、Rust target **1 passed／338 filtered**、direct clippy、default／wrapper／direct FFI、各模式
`store_arch` **56 passed**、三種 production `--version` 均為 `codebase-memory-mcp dev`；direct selector 為空，
source-negative 不含 `src/store/package_list.c`，source-positive 保留 `src/store/store.c`。`scripts/test.sh` 未執行。

維持 defer：`pipeline-complexity-json`、content_length value／raw 的 `strtol` 語義差異。下一步先重做
Pipeline／MCP／Store pure-helper inventory，再選取並凍結 #80；不可直接實作或 gate。

## 2026-07-23 交接總覽（優先讀取）

目前權威快照為 [docs/rust-refactor-current-handoff.md](docs/rust-refactor-current-handoff.md)
（含「給下一個 Session 的起手式」表格、工作樹地圖與命令）。產品預設仍是 C11；Rust 只透過
明確 opt-in 接管窄範圍契約。下方歷史段落的 flag 數、passed 數與「已完成」敘述只代表記載
當日，不能當成 dirty worktree 的完整驗證結果。

**Skill：** `CodebaseMemMcp-Refactory-rust`。**工作樹 dirty 為預期**；禁止 reset／clean。
完整 `scripts/test.sh` 未跑。

### 停點（給下一個 Session）

| 項目 | 值 |
|------|-----|
| true-source | **72 / 72** |
| 最新完成 | **#77 store language map** |
| 勿重跑 | **#29 至 #77** |
| 延後 | complexity JSON／content_length 數字解析（`strtol` 語義差異） |
| 下一步 ready | 重新盤點 pure helper；不預設下一個切片 |
| suite 參考 | store_arch **55** / pipeline **236** / mcp **161** / cypher **145** / cypher_contract **28** |
| 起手 skill | `CodebaseMemMcp-Refactory-rust`；gate 一律 `make -j1` 序列 |

### 第 76 項：cypher label alternation match true-source（2026-07-23）

true-source 累計 **70 / 70 -> 71 / 71**。C fallback 為 `src/cypher/label_alt_match.c/.h`；
`src/cypher/cypher.c` 的三個 relationship traversal consumer 保留 orchestration，並只經 public bridge 呼叫。
公開 ABI 為 `bool cbm_cypher_label_alt_matches(const char *actual, const char *pat);`。`pat == NULL` 回 true 且不讀
`actual`；`pat` 非 NULL、`actual == NULL` 回 false 且不讀 `pat` 內容。其餘合法 NUL 終止 C 字串以 first-NUL
前的大小寫敏感 bytes 比對：無 ASCII `|` 即 exact compare；有 `|` 則按順序比對 segments，leading／interior
空 segment 可匹配空 actual，trailing 空 segment 不檢查，故 `A|` 不能匹配空字串而 `|` 可以。無 allocation、輸入
修改、pointer 保存、I/O、locale 或 ownership 行為；parser `scan_alternation_labels`／`strtok_r` 路徑不在此切片。

wrapper macro 為 `CBM_USE_RUST_CYPHER_LABEL_ALT_MATCH`；direct macro 為
`CBM_USE_RUST_CYPHER_LABEL_ALT_MATCH_ONLY`；Rust feature 為 `cypher-label-alt-match-only`；v1 ABI 為
`cbm_rs_cypher_label_alt_matches_v1`，direct 匯出同名 public ABI。

完整成功 gate：`cargo fmt --all -- --check`、Rust unit `cypher_label_alt_match` **1 passed（336 filtered）**、
`clippy -D warnings`、`rust-cypher-label-alt-match-{optin,only}-test` 的 default／wrapper／direct FFI；cypher
三模式各 **145 passed**、cypher_contract 三模式各 **28 passed**，產品 `--version` 均為
`codebase-memory-mcp dev`。ONLY `CYPHER_LABEL_ALT_MATCH_SRCS =` 為空，`make -Bn` 已證實排除 fallback source 並保留
`src/cypher/cypher.c`。完整 `scripts/test.sh` 未重跑。

complexity JSON／content_length 的數字解析仍因 `strtol` 語義差異延後。下一步重新盤點 Pipeline／MCP／Store
pure helper，不預設候選；#29 至 #76 均不得重跑。

### 第 77 項：Store language map true-source（2026-07-23）

已將 `store.c` 內的 extension -> language table 與 adapter 拆至
`src/store/language_map.c/.h`。公開 ABI 為
`const char *cbm_store_ext_to_lang(const char *ext)`，凍結 **44** 個既有對應：`NULL`、未知 extension、
大小寫不符均回 `NULL`；合法輸入只讀 first-NUL 前 bytes，採大小寫敏感 `strcmp` 等價比對。成功回傳值是
immutable、NUL 終止、process-lifetime 的 borrowed static pointer，呼叫端不得 `free`；沒有配置、修改、
pointer 保存、I/O 或 locale 行為。

wrapper `CBM_USE_RUST_STORE_LANGUAGE_MAP` 先委派 Rust v1
`cbm_rs_store_ext_to_lang_v1`，其回 `NULL` 才走 C fallback。direct
`CBM_USE_RUST_STORE_LANGUAGE_MAP_ONLY` 搭配 Cargo `store-language-map-only`，由 Rust 匯出同名 public
bridge `cbm_store_ext_to_lang`。`STORE_LANGUAGE_MAP_C_SRC`／`STORE_LANGUAGE_MAP_SRCS` 在 ONLY 為空，
排除 `src/store/language_map.c`，但保留 `src/store/store.c`。

已完整通過下列可複製命令：

```sh
cargo fmt --all -- --check
cargo test -p cbm-core --locked
cargo clippy -p cbm-core --all-targets --features store-language-map-only --locked -- -D warnings
make -j1 -f Makefile.cbm rust-store-language-map-optin-test
make -j1 -f Makefile.cbm rust-store-language-map-only-test
```

Rust test 為 **338 passed**；兩個 Make gate 均已覆蓋 default／wrapper／direct FFI、store_arch 三模式各
**55 passed** 與 production `--version`。ONLY 的 selector 為空，`make -Bn` 證明
`language_map.c` 不在 source list、`store.c` 仍在。初次 gate 發現的舊 duplicate target 已刪除，parse
selector 無 warning。**不得重跑 #29 至 #77，且未執行 `scripts/test.sh`。** `pipeline-complexity-json` 與
content_length 的 `strtol` 語義仍 defer；下一步盤點 Pipeline／MCP／Store pure helpers。

### 第 75 項：cypher regex shape true-source（2026-07-23）

true-source 累計 **69 / 69 -> 70 / 70**。C fallback 為 `src/cypher/regex_shape.c/.h`；唯一 consumer
`src/cypher/cypher.c` 的 `check_inline_props` 保留 orchestration，並只經 public bridge 呼叫。公開 ABI 為
`bool cbm_cypher_looks_like_regex(const char *s);`。NULL 回 false 且不讀取；非 NULL 僅讀 first-NUL 前 bytes，
含 `.*`、`.+` 或任一 `[`, `(`, `|`, `^`, `$` 回 true，其餘回 false。這是 shape classifier，不是 regex parser；
無 allocation、輸入修改、pointer 保存、I/O、locale 或 ownership 行為。POSIX ERE 與 exact `strcmp` 的既有決策
仍由 `check_inline_props` 負責。

wrapper macro 為 `CBM_USE_RUST_CYPHER_REGEX_SHAPE`；direct macro 為
`CBM_USE_RUST_CYPHER_REGEX_SHAPE_ONLY`；Rust feature 為 `cypher-regex-shape-only`；v1 ABI 為
`cbm_rs_cypher_looks_like_regex_v1`，direct 匯出同名 public ABI。

完整成功 gate：`cargo fmt --all -- --check`、Rust unit `cypher_regex_shape` **1 passed（335 filtered）**、
`clippy -D warnings`、`rust-cypher-regex-shape-{optin,only}-test` 的 default／wrapper／direct FFI；cypher
三模式各 **144 passed**、cypher_contract 三模式各 **27 passed**，產品 `--version` 均為
`codebase-memory-mcp dev`。ONLY `CYPHER_REGEX_SHAPE_SRCS =` 為空，`make -Bn` 已證實排除 fallback source 並保留
`src/cypher/cypher.c`。完整 `scripts/test.sh` 未重跑。

complexity JSON／content_length 的數字解析仍因 `strtol` 語義差異延後。下一步重新盤點 pure helper，
不預設候選；#29 至 #75 均不得重跑。

### 第 74 項：store arch aspect filter true-source（2026-07-23）

true-source 累計 **68 / 68 -> 69 / 69**。C fallback 為
`src/store/arch_aspect_filter.c/.h`；`src/store/store.c` 保留 architecture store 的協調流程，並只經公開橋接函式
呼叫 helper。公開 ABI 為
`bool cbm_store_arch_wants_aspect(const char *const *aspects, int aspect_count, const char *name);`。`aspects == NULL`
時不論 count 與 name 均回 true；nonnull 且 count 為 0 回 true、負數回 false。僅正 count 會按序掃描
`[0, count)`，在每個 entry 的 first-NUL 前以精確、大小寫敏感比對 `"all"` 或 `name`，命中即回 true，否則
false；此時合法 C 字串指標是呼叫前置條件。helper 不配置、不保存或修改輸入、不做 I/O，也不依賴 locale。

wrapper macro 為 `CBM_USE_RUST_STORE_ARCH_ASPECT_FILTER`；direct macro 為
`CBM_USE_RUST_STORE_ARCH_ASPECT_FILTER_ONLY`；Rust feature 為 `store-arch-aspect-filter-only`；direct v1 ABI 為
`cbm_rs_store_arch_wants_aspect_v1`。

gate 已完整成功：fmt、Rust unit `wants_aspect_matches_c_contract` **1 passed**、`clippy -D warnings`；opt-in 與
only 均完成 default／wrapper／direct FFI。store_arch suite 三種模式各 **54 passed**，產品 `--version` 均輸出
`codebase-memory-mcp dev`。direct 選擇器 `STORE_ARCH_ASPECT_FILTER_SRCS =` 為空，fallback source 已排除，
但 `src/store/store.c` 的正向 source proof 仍保留。完整 `scripts/test.sh` 未重跑。

complexity JSON／content_length 的數字解析仍因 `strtol` 語義差異延後。下一步是重新盤點 pure helper，
不可據此預設或虛構下一個切片。

### 第 73 項：pipeline configlink path basename true-source（2026-07-23）

true-source 累計 **67 / 67 -> 68 / 68**。C fallback 為
`src/pipeline/configlink_path_basename.c/.h`；`pass_configlink.c` 保留協調流程，並只經公開橋接函式呼叫
helper。公開 ABI 為 `const char *cbm_pipeline_configlink_path_basename(const char *path);`：NULL 回傳非 NULL
的靜態空字串；非 NULL 僅讀到第一個 NUL，只把 ASCII `/` 視為分隔符，回傳輸入內的借用指標，根路徑與
尾端斜線均指向終止 NUL。反斜線是普通字元；不配置、不修改輸入、不做 I/O。

wrapper macro 為 `CBM_USE_RUST_PIPELINE_CONFIGLINK_PATH_BASENAME`；direct macro 為
`CBM_USE_RUST_PIPELINE_CONFIGLINK_PATH_BASENAME_ONLY`；Rust feature 為
`pipeline-configlink-path-basename-only`；direct v1 ABI 為
`cbm_rs_pipeline_configlink_path_basename_v1`。

gate 已完整成功：`cargo fmt --check`、Rust unit **1 passed**、`clippy -D warnings`；opt-in 與 only 均完成
default／wrapper／direct FFI，pipeline suite 各 **236 passed**，產品 `--version` 輸出
`codebase-memory-mcp dev`。direct 選擇器為空，fallback source 已排除，但 `pass_configlink.c` 仍保留。
完整 `scripts/test.sh` 未重跑。

complexity JSON／content_length 的數字解析仍因 `strtol` 語義差異延後。下一步是重新盤點 pure helper，
不可據此預設或虛構下一個切片。

### 第 59 項：MCP parse_file_uri true-source（2026-07-19）

C fallback `src/mcp/parse_file_uri.c/.h`；公開 `bool cbm_parse_file_uri(const char *, char *, int)`。
`file://` prefix、raw path、Windows drive strip、截斷仍成功、錯誤清空。wrapper
`CBM_USE_RUST_MCP_PARSE_FILE_URI`（+CODEC）；direct `…_ONLY`／`mcp-parse-file-uri-only`。
Make `rust-mcp-parse-file-uri-{optin,only}-test`：mcp 各 **156 passed**、production `--version`；
ONLY 空 selector + bn 不含 `parse_file_uri.c`、仍含 `mcp.c`。unit 1／clippy 通過。

### 第 58 項：MCP method_kind true-source（2026-07-19）

C fallback `src/mcp/method_kind.c/.h`；enum 0–5 + 公開 `int cbm_mcp_method_kind(const char *)`。
NULL／空／case／空白／未知→0；exact match initialize/ping/tools/list/tools/call/notifications/cancelled。
wrapper `CBM_USE_RUST_MCP_METHOD_KIND`（+CODEC）；direct `…_ONLY`／`mcp-method-kind-only`。
Make `rust-mcp-method-kind-{optin,only}-test`：mcp 各 **156 passed**、production `--version`；
ONLY 空 selector + bn 不含 `method_kind.c`、仍含 `mcp.c`。fmt／unit 1／clippy 通過。

### 第 54–57 項（2026-07-19，基線 true-source）

**#54：** architecture_aspect_wanted；mcp **152**。  
**#55：** strip_root_prefix_offset；mcp **153**。  
**#56：** search_pick_tightest_index；mcp **154**。  
**#57：** search_pick_resolved_index；mcp **155**。

新 Session 先確認無殘留 Make／cargo。歷史 #54 WIP 敘述以下方完成段落為準。原指令：

~~~sh
make -j1 -f Makefile.cbm rust-mcp-architecture-aspect-wanted-optin-test
make -j1 -f Makefile.cbm rust-mcp-architecture-aspect-wanted-only-test
~~~

### 第 53 項：MCP sanitize_utf8_lossy true-source（2026-07-19）

true-source 累計 **48 / 48**。C fallback 為 `src/mcp/sanitize_utf8_lossy.c/.h`；公開 ABI
`char *cbm_mcp_sanitize_utf8_lossy(const char *)`。NULL 回 NULL；first-NUL 前保留有效 UTF-8、每個無效 byte
替換為 U+FFFD。成功結果由 C `malloc` 配置，caller 必須 `free()`；無 I/O。`mcp.c` 僅保留 source resolution、
yyjson result shaping 與 transport。wrapper `CBM_USE_RUST_MCP_SANITIZE_UTF8_LOSSY`（相容 CODEC）；direct
`CBM_USE_RUST_MCP_SANITIZE_UTF8_LOSSY_ONLY`／`mcp-sanitize-utf8-lossy-only` 由 Rust 呼叫 C `malloc`，維持 caller
free。對應 Make gate 的 default/wrapper/direct FFI、mcp 各 **151 passed**、production `--version`、direct empty
selector、source-negative（仍含 `mcp.c`）均通過。Rust unit **1 passed**、fmt 與切片 clippy 通過；完整
`scripts/test.sh` 未跑。

### 第 52 項：MCP bm25_build_match true-source（2026-07-19）

true-source 累計 **47 / 47**。C fallback 為 `src/mcp/bm25_build_match.c/.h`；公開 ABI
`int cbm_mcp_bm25_build_match(const char *, char *, size_t)`，參數順序維持 C call site 的 input-first 形式。
NULL input／buffer 或 `bufsize < 2` 回 0 且不寫入；只保留 ASCII alnum／underscore token，以 ` OR ` 串接，
短 buffer 停在上一個完整 token 並 NUL-terminate，first-NUL 生效。輸出 buffer 由 caller 擁有，無 heap、無 I/O。
`mcp.c` 僅保留 FTS5 SQLite query、ranking／boosting、result JSON 與 transport。wrapper
`CBM_USE_RUST_MCP_BM25_BUILD_MATCH`（相容 CODEC）；direct
`CBM_USE_RUST_MCP_BM25_BUILD_MATCH_ONLY`／`mcp-bm25-build-match-only`。對應 Make gate 的 default/wrapper/direct
FFI、mcp 各 **150 passed**、production `--version`、direct empty selector、source-negative（仍含 `mcp.c`）均通過。
Rust unit **1 passed**、fmt 與切片 clippy 通過；完整 `scripts/test.sh` 未跑。

### 第 51 項：MCP bm25_file_pattern_like true-source（2026-07-19）

true-source 累計 **46 / 46**。C fallback 為 `src/mcp/bm25_file_pattern_like.c/.h`；公開 ABI
`size_t cbm_mcp_bm25_file_pattern_like(char *, size_t, const char *)`。NULL input 回 `SIZE_MAX`；成功回完整
SQL LIKE byte 長度，NULL／zero buffer 為 length-only，短 buffer 截斷並 NUL-terminate；first-NUL 生效。
先沿用 Store glob-to-LIKE（`*`→`%`、`?`→`_`），無 wildcard 時加前後 `%`。輸出 buffer 由 caller 擁有；
C fallback 的暫時 glob buffer 會在內部釋放，無 I/O。`mcp.c` 僅保留 final buffer ownership 與 BM25 SQLite
query orchestration。wrapper `CBM_USE_RUST_MCP_BM25_FILE_PATTERN_LIKE`（相容 CODEC）；direct
`CBM_USE_RUST_MCP_BM25_FILE_PATTERN_LIKE_ONLY`／`mcp-bm25-file-pattern-like-only`。對應 Make gate 的
default/wrapper/direct FFI、mcp 各 **149 passed**、production `--version`、direct empty selector、
source-negative（仍含 `mcp.c`）均通過。Rust unit **1 passed**、fmt 與切片 clippy 通過；完整
`scripts/test.sh` 未跑。

### 第 50 項：MCP sanitize_ascii_in_place true-source（2026-07-19）

true-source 累計 **45 / 45**。C fallback 為 `src/mcp/sanitize_ascii_in_place.c/.h`；公開 ABI
`void cbm_mcp_sanitize_ascii_in_place(char *)`。NULL 為 no-op；只讀至 first-NUL，將每個 >127 byte 就地改為
`?`，ASCII（含 `0x7f`）不變，NUL 後 byte 不改；無 heap、無 I/O。`mcp.c` 保留 grep/source/context 讀取、
JSON/result shaping、UTF-8 lossy sanitizer 與 transport。wrapper `CBM_USE_RUST_MCP_SANITIZE_ASCII_IN_PLACE`
（相容 CODEC）；direct `CBM_USE_RUST_MCP_SANITIZE_ASCII_IN_PLACE_ONLY`／`mcp-sanitize-ascii-in-place-only`。
對應 Make gate 的 default/wrapper/direct FFI、mcp 各 **148 passed**、production `--version`、direct empty
selector、source-negative（仍含 `mcp.c`）均通過。Rust unit **1 passed**、fmt 與切片 clippy 通過；完整
`scripts/test.sh` 未跑。

### 第 49 項：MCP project_db_file_name true-source（2026-07-19）

true-source 累計 **44 / 44**。C fallback 為 `src/mcp/project_db_file_name.c/.h`；公開 ABI
`bool cbm_mcp_project_db_file_name(const char *)`。NULL、empty、`.db`、非精準 suffix、`_` 開頭與 `:memory:`
開頭回 false；至少 `x.db`、精準 `.db` suffix 才可能為 true，`tmp-*.db` 保持有效；採 first-NUL，無 heap、
無輸入改寫、無 I/O。`mcp.c` 保留目錄掃描、SQLite query-open、internal project-name resolution、corrupt DB
filtering、list JSON、resolve fallback 與 transport。wrapper `CBM_USE_RUST_MCP_PROJECT_DB_FILE_NAME`（相容 CODEC）；
direct `CBM_USE_RUST_MCP_PROJECT_DB_FILE_NAME_ONLY`／`mcp-project-db-file-name-only`。對應 Make gate 的
default/wrapper/direct FFI、mcp 各 **147 passed**、production `--version`、direct empty selector、source-negative
（仍含 `mcp.c`）均通過。Rust unit **1 passed**、fmt 與切片 clippy 通過；完整 `scripts/test.sh` 未跑。

### 第 48 項：MCP trace_is_test_file true-source（2026-07-19）

true-source 累計 **43 / 43**。C fallback 為 `src/mcp/trace_is_test_file.c/.h`；公開 ABI
`bool cbm_mcp_trace_is_test_file(const char *)`。NULL、empty 回 false；`/test`、`test_`、`_test.`、`/tests/`、
`/spec/` 或 `.test.` substring 回 true；字串採 first-NUL，無 heap、無輸入改寫、無 I/O。`mcp.c` 保留 BFS、
include_tests filtering、JSON 與 transport。wrapper `CBM_USE_RUST_MCP_TRACE_IS_TEST_FILE`（相容 CODEC）；
direct `CBM_USE_RUST_MCP_TRACE_IS_TEST_FILE_ONLY`／`mcp-trace-is-test-file-only`。對應 Make gate 的
default/wrapper/direct FFI、mcp 各 **146 passed**、production `--version`、direct empty selector、
source-negative（仍含 `mcp.c`）均通過。Rust unit **1 passed**、fmt 與切片 clippy 通過；完整
`scripts/test.sh` 未跑。

### 第 47 項：MCP trace_mode_edge_mask true-source（2026-07-19）

true-source 累計 **42 / 42**。C fallback 為 `src/mcp/trace_mode_edge_mask.c/.h`；公開 ABI
`uint32_t cbm_mcp_trace_mode_edge_mask(const char *)`。NULL、empty、`calls`、未知、大小寫不符與尾端空白回
`CALLS` bit；`data_flow` 回 `CALLS|DATA_FLOWS`；`cross_service` 回完整 bit 0..9 edge set（`0x3ff`）；字串採
first-NUL，無 heap、無輸入改寫、無 I/O。`mcp.c` 保留 explicit edge array parsing／ownership、edge-name 順序、
BFS、JSON 與 transport。wrapper `CBM_USE_RUST_MCP_TRACE_MODE_EDGE_MASK`（相容 CODEC）；direct
`CBM_USE_RUST_MCP_TRACE_MODE_EDGE_MASK_ONLY`／`mcp-trace-mode-edge-mask-only`。對應 Make gate 的
default/wrapper/direct FFI、mcp 各 **145 passed**、production `--version`、direct empty selector、
source-negative（仍含 `mcp.c`）均通過。Rust unit **1 passed**、fmt 與切片 clippy 通過；完整
`scripts/test.sh` 未跑。

### 第 46 項：MCP adr_mode true-source（2026-07-19）

true-source 累計 **41 / 41**。C fallback 為 `src/mcp/adr_mode.c/.h`；公開 ABI
`int cbm_mcp_adr_mode(const char *)`。NULL、empty、`get`、未知、大小寫不符與尾端空白回 0；`update`／
`store` 回 1；`sections` 回 2；字串採 first-NUL，無 heap、無輸入改寫、無 I/O。`mcp.c` 保留 project／Store
resolution、ADR read/write、sections JSON、ownership 與 transport。wrapper `CBM_USE_RUST_MCP_ADR_MODE`
（相容 CODEC）；direct `CBM_USE_RUST_MCP_ADR_MODE_ONLY`／`mcp-adr-mode-only`。對應 Make gate 的
default/wrapper/direct FFI、mcp 各 **144 passed**、production `--version`、direct empty selector、
source-negative（仍含 `mcp.c`）均通過。Rust unit **1 passed**、fmt 與切片 clippy 通過；完整
`scripts/test.sh` 未跑。

### 第 45 項：MCP utf8_is_cont_byte true-source（2026-07-19）

true-source 累計 **40 / 40**。C fallback 為 `src/mcp/utf8_is_cont.c/.h`；公開 ABI
`bool cbm_mcp_utf8_is_cont_byte(int)`。只檢查低 8 bits，`0x80..0xBF` 回 true、其餘回 false；負數與
高位輸入同樣採 byte mask；無 heap、無輸入改寫、無 I/O。`sanitize_utf8_lossy.c` 保留 scan、replacement
寫入與 buffer ownership，`mcp.c` 保留 JSON／transport。wrapper `CBM_USE_RUST_MCP_UTF8_IS_CONT`（相容
CODEC）；direct `CBM_USE_RUST_MCP_UTF8_IS_CONT_ONLY`／`mcp-utf8-is-cont-only`。對應 Make gate 的
default/wrapper/direct FFI、mcp 各 **143 passed**、production `--version`、direct empty selector、
source-negative（仍含 `mcp.c`）均通過。Rust unit **1 passed**、fmt 與切片 clippy 通過；完整
`scripts/test.sh` 未跑。

### 第 44 項：MCP node_resolution_score true-source（2026-07-19）

true-source 累計 **39 / 39**。C fallback 為 `src/mcp/node_resolution_score.c/.h`；公開 ABI
`long cbm_mcp_node_resolution_score(const char *, int, int)`。Function／Method 為 tier 2，Module／File 與
NULL 為 tier 0，其餘 label 為 tier 1；回傳 tier * 1000000 加 max(line span, 0)，字串採 first-NUL，無 heap、
無輸入改寫、無 I/O。wrapper `CBM_USE_RUST_MCP_NODE_RESOLUTION_SCORE`（相容 CODEC）；direct
`CBM_USE_RUST_MCP_NODE_RESOLUTION_SCORE_ONLY`／`mcp-node-resolution-score-only`。對應 Make gate 的
default/wrapper/direct FFI、mcp 各 **142 passed**、production `--version`、direct empty selector、
source-negative（仍含 `mcp.c`）均通過。Rust unit **1 passed**、fmt 與切片 clippy 通過；完整
`scripts/test.sh` 未跑。

### 第 43 項：MCP detect_changes_status_path_offset true-source（2026-07-19）

true-source 累計 **38 / 38**。C fallback 為 `src/mcp/detect_changes_status.c/.h`；公開 ABI
`size_t cbm_mcp_detect_changes_status_path_offset(const char *)`。NULL、空字串或最終 path 空回 `SIZE_MAX`；
porcelain `XY ` 跳過 3 bytes，rename 取 destination offset，bare／unknown prefix 回 0；字串採 first-NUL，
無 heap、無輸入改寫、無 I/O。wrapper `CBM_USE_RUST_MCP_DETECT_CHANGES_STATUS`（相容 CODEC）；direct
`CBM_USE_RUST_MCP_DETECT_CHANGES_STATUS_ONLY`／`mcp-detect-changes-status-only`。Make
`rust-mcp-detect-changes-status-{optin,only}-test` 已通過 default/wrapper/direct FFI、mcp 各
**141 passed**、production `--version`、direct empty selector、source-negative（仍含 `mcp.c`）。
Rust unit **1 passed**、fmt 與切片 clippy 通過；完整 `scripts/test.sh` 未跑。

### 第 42 項：MCP detect_changes_impacted_label true-source（2026-07-19）

true-source 累計 **37 / 37**。C fallback 為 `src/mcp/detect_changes_label.c/.h`；公開 ABI
`bool cbm_mcp_detect_changes_impacted_label(const char *)`。NULL、`File`、`Folder`、`Project` 回 false；
空字串與其他值回 true；字串採 first-NUL，無 heap、無輸入改寫、無 I/O。wrapper
`CBM_USE_RUST_MCP_DETECT_CHANGES_LABEL`（相容 CODEC）；direct
`CBM_USE_RUST_MCP_DETECT_CHANGES_LABEL_ONLY`／`mcp-detect-changes-label-only`。Make
`rust-mcp-detect-changes-label-{optin,only}-test` 已通過 default/wrapper/direct FFI、mcp 各
**140 passed**、production `--version`、direct empty selector、source-negative（仍含 `mcp.c`）。
Rust unit **1 passed**、fmt 與切片 clippy 通過；完整 `scripts/test.sh` 未跑。

### 第 41 項：MCP detect_changes_wants_symbols true-source（2026-07-19）

true-source 累計 **36 / 36**。C fallback 為 `src/mcp/detect_changes_scope.c/.h`；公開 ABI
`bool cbm_mcp_detect_changes_wants_symbols(const char *)`。NULL、`symbols`、`impact` 回 true；空字串、
`files`、大小寫不符、尾端空白與其他值回 false；字串採 first-NUL，無 heap、無輸入改寫、無 I/O。
wrapper `CBM_USE_RUST_MCP_DETECT_CHANGES_SCOPE`（相容 CODEC）；direct
`CBM_USE_RUST_MCP_DETECT_CHANGES_SCOPE_ONLY`／`mcp-detect-changes-scope-only`。Make
`rust-mcp-detect-changes-scope-{optin,only}-test` 已通過 default/wrapper/direct FFI、mcp 各
**139 passed**、production `--version`、direct empty selector、source-negative（仍含 `mcp.c`）。
Rust unit **1 passed**、fmt 與切片 clippy 通過；完整 `scripts/test.sh` 未跑。

### 第 40 項：MCP search_top_dir true-source（2026-07-19）

true-source 累計 **35 / 35**。C fallback 為 `src/mcp/search_top_dir.c/.h`；公開 ABI
`size_t cbm_mcp_search_top_dir(char *, size_t, const char *)`。NULL file 回 `SIZE_MAX` 且不寫入；成功
回完整 top key 長度，NULL／zero buffer 為 length-only，短 buffer NUL 截斷，字串採 first-NUL。
wrapper `CBM_USE_RUST_MCP_SEARCH_TOP_DIR`（相容 CODEC）；direct
`CBM_USE_RUST_MCP_SEARCH_TOP_DIR_ONLY`／`mcp-search-top-dir-only`。Make
`rust-mcp-search-top-dir-{optin,only}-test` 已通過 default/wrapper/direct FFI、mcp 各 **138 passed**、
production `--version`、direct empty selector、source-negative（仍含 `mcp.c`）。Rust unit **1 passed**、
fmt 與切片 clippy 通過；完整 `scripts/test.sh` 未跑。

### 第 39 項：MCP search_code_score true-source（2026-07-19）

true-source 累計 **34 / 34**。C fallback 為 `src/mcp/search_code_score.c/.h`；公開 ABI
`int cbm_mcp_search_code_score(const char *, const char *, int)`。NULL label／file 不加權、first-NUL
截斷；Function／Method +10、Route +15、vendored/vendor/node_modules -50、test/spec/_test. -5，可累加。
wrapper `CBM_USE_RUST_MCP_SEARCH_CODE_SCORE`（相容 CODEC）；direct
`CBM_USE_RUST_MCP_SEARCH_CODE_SCORE_ONLY`／`mcp-search-code-score-only`。Make
`rust-mcp-search-code-score-{optin,only}-test` 已通過 default/wrapper/direct FFI、mcp 各
**137 passed**、production `--version`、direct empty selector、source-negative（仍含 `mcp.c`）。
Rust unit **1 passed**、fmt 與切片 clippy 通過；完整 `scripts/test.sh` 未跑。

### 第 38 項：pipeline pkgmap text true-source（2026-07-19）

true-source 累計 **33 / 33**。C fallback 為 `src/pipeline/pkgmap_text.c/.h`；`pass_pkgmap.c` 保留
manifest parsing、I/O、entry ownership 與 orchestration。6 個 `cbm_pipeline_pkgmap_*` public helper
涵蓋 bounded prefix、borrowed line value 與四個 C malloc／caller free path result。wrapper
`CBM_USE_RUST_PIPELINE_PKGMAP_TEXT`；direct
`CBM_USE_RUST_PIPELINE_PKGMAP_TEXT_ONLY`／`pipeline-pkgmap-text-only`。Make
`rust-pipeline-pkgmap-text-{optin,only}-test` 已通過 default/wrapper/direct FFI、pipeline 各
**235 passed**、production `--version`、direct empty selector、source-negative（仍含
`pass_pkgmap.c`）。Rust unit **4 passed**、fmt 與切片 clippy 通過；完整 `scripts/test.sh` 未跑。

### 第 37 項：pipeline enrichment tokens true-source（2026-07-19）

true-source 累計 **32 / 32**。C fallback 為 `src/pipeline/enrichment_tokens.c/.h`；公開 ABI 為
`cbm_split_camel_case` 與 `cbm_tokenize_decorator`，成功 token 是 C `malloc`／caller `free`。
ASCII byte lowercase 已凍結，非 ASCII byte 原樣保留；OOM 明確排除於 normal-resource parity。
wrapper `CBM_USE_RUST_PIPELINE_ENRICHMENT_TOKENS`；direct
`CBM_USE_RUST_PIPELINE_ENRICHMENT_TOKENS_ONLY`／`pipeline-enrichment-tokens-only`。
Make `rust-pipeline-enrichment-tokens-{optin,only}-test` 已通過 default/wrapper/direct FFI、pipeline
各 **233 passed**、production `--version`、direct empty selector 與 source-negative（仍含
`pass_enrichment.c`）。Rust unit **5 passed**、fmt 與切片 clippy 通過；完整 `scripts/test.sh` 未跑。

### 第 35–36 項：search_args + search_score_cmp true-source（2026-07-18）

true-source 累計 **31 / 31**。未重開 #29–#34；complexity 仍延後。

**#35 args：** `src/mcp/search_args.c`；`bool cbm_mcp_search_args_valid`；
wrapper／direct `CBM_USE_RUST_MCP_SEARCH_ARGS[_ONLY]`／`mcp-search-args-only`；mcp **136**。
Make：`rust-mcp-search-args-{optin,only}-test`。

**#36 score_cmp：** `src/mcp/search_score_cmp.c`；`int cbm_mcp_search_score_cmp`；
wrapper／direct `…_SEARCH_SCORE_CMP[_ONLY]`／`mcp-search-score-cmp-only`；mcp **136**。
Make：`rust-mcp-search-score-cmp-{optin,only}-test`。

### 第 33–34 項：search_path_arg + search_line_span true-source（2026-07-18）

true-source 累計 **29 / 29**。未重開 #29–#32；complexity 仍延後。

**#33 path_arg：** `src/mcp/search_path_arg.c/.h`；`bool cbm_mcp_search_path_arg_valid`；
wrapper／direct `CBM_USE_RUST_MCP_SEARCH_PATH_ARG[_ONLY]`／`mcp-search-path-arg-only`；
mcp **136**；bn 不含 fallback、仍含 mcp.c。

**#34 line_span：** `src/mcp/search_line_span.c/.h`；`int cbm_mcp_search_line_match_span`；
wrapper／direct `…_SEARCH_LINE_SPAN[_ONLY]`／`mcp-search-line-span-only`；mcp **136**。

### 第 32 項：MCP index_mode true-source（2026-07-18）

true-source 累計 **27 / 27**。C fallback：`src/mcp/index_mode.c/.h`；公開
`int cbm_mcp_index_mode`（moderate→1、fast→2、cross-repo-intelligence→3、其餘→0）。
wrapper `CBM_USE_RUST_MCP_INDEX_MODE`（相容 CODEC）；direct `…_ONLY`／`mcp-index-mode-only`。
mcp 各 **136 passed**；direct 空 selector + `make -Bn` 不含 `index_mode.c`，仍含 `mcp.c`。
fmt／unit／clippy 通過。未重開 #29–#31；complexity 仍延後。完整 `scripts/test.sh` 未跑。

### 第 30–31 項：usages-json + MCP search_mode true-source（2026-07-17）

true-source 累計 **26 / 26**（#30 + #31）。未重開 #29；complexity JSON 仍延後。

**#30 pipeline-usages-json：** C fallback `src/pipeline/usages_json.c/.h`；公開
`char *cbm_pipeline_usages_extract_local_name`（malloc／caller free）。wrapper
`CBM_USE_RUST_PIPELINE_USAGES_JSON`；direct `…_ONLY`／`pipeline-usages-json-only`。
pipeline 各 **231 passed**；direct 空 selector + `make -Bn` 不含 `usages_json.c`，仍含
`pass_usages.c`。

**#31 MCP search_mode：** C fallback `src/mcp/search_mode.c/.h`；公開
`int cbm_mcp_search_mode`（full→1、files→2、其餘→0）。wrapper
`CBM_USE_RUST_MCP_SEARCH_MODE`（亦相容 CODEC）；direct `…_ONLY`／`mcp-search-mode-only`。
mcp 各 **136 passed**；direct 空 selector + `make -Bn` 不含 `search_mode.c`，仍含 `mcp.c`。

fmt／unit／clippy 均通過。完整 `scripts/test.sh` 未跑。

### 第 29 項：Pipeline calls-json true-source（2026-07-17）

true-source 累計 **24 / 24**。C fallback：`src/pipeline/calls_json.c/.h`；公開 ABI
`char *cbm_pipeline_calls_extract_local_name(const char *)`（malloc／caller free）。
wrapper `CBM_USE_RUST_PIPELINE_CALLS_JSON`；direct `…_ONLY`／`pipeline-calls-json-only`。
pipeline 各 **231 passed**；direct 空 selector + `make -Bn` 不含 `calls_json.c`，仍含
`pass_calls.c`。fmt／unit 1／clippy 通過。完整 `scripts/test.sh` 未跑。
### 本輪（2026-07-17 multi-agent limit-21）已完成

權威細節：`docs/rust-refactor-current-handoff.md`。本目標完成 **21** 個 true-source 切片
（預算 21； durable `bn-source-negative.log` + clippy/fmt）：

### 原預算外後續：第 22 個 true-source（保留原始 21 / 21 歷史）

22. **Discover `cbm_language_name`** — 狹窄 true-source replacement。C fallback 由
`src/discover/language.c` 拆至 `src/discover/language_name.c/.h`，`language.c` 保留其他語言偵測。
公開 ABI 為 `const char *cbm_language_name(CBMLanguage)`：回傳 borrowed static、NUL 結尾、
immutable 字串，呼叫端不得 `free`；無效值、`COUNT`、out-of-range 與未來 enum hole 均回傳
`"Unknown"`。全 enum sweep 已覆蓋，`CBM_LANG_CFSCRIPT=157` 與 `CBM_LANG_CFML=158` 均為
`"CFML"`。`CBM_USE_RUST_DISCOVER_LANGUAGE_NAME=1` wrapper 經
`cbm_rs_discover_language_name_v1(int)` 呼叫 Rust，僅 Rust 回傳 NULL 時回退 C；
`CBM_USE_RUST_DISCOVER_LANGUAGE_NAME_ONLY=1` direct 使用 Cargo feature
`discover-language-name-only`，`DISCOVER_LANGUAGE_NAME_SRCS =`，source-negative 證明 production
不連結 `src/discover/language_name.c`。此 direct 與 legacy 完整 language direct
`CBM_USE_RUST_DISCOVER_LANGUAGE_ONLY=1` 因同名公開 ABI 互斥。

`rust-discover-language-name-optin-test` 已通過 default/wrapper FFI、discover **87 passed** 與
production `--version`；`rust-discover-language-name-only-test` 已通過 default/direct FFI、discover
**87 passed**、production `--version`、empty selector 與 source-negative；
`rust-discover-userconfig-language-name-only-test` 已通過 userconfig combo FFI、discover
**87 passed** 與 production `--version`。Rust fmt、language-name unit **3 passed**、clippy 均成功；
完整 `scripts/test.sh` 未跑。

### 原預算外後續：第 23 項 artifact-path parity/gate hardening（非新 true-source）

既有 artifact-path ABI 已完成 parity/gate 補強，不新增 true-source，且不改變原始 **21 / 21** 與
第 22 項原預算外 true-source 的歷史。新增 public `artifact_path.h` ABI
`bool cbm_pipeline_artifact_path(char *buf, size_t bufsz, const char *repo_path, const char *name)`：
呼叫端擁有 `buf`；輸入為 borrowed、NUL 結尾 raw bytes，不可與輸出 overlap，無 UTF-8 語意。C
fallback `snprintf` 行為維持；short buffer 且 `bufsz > 0` 時仍寫 prefix + NUL，public ABI 回 false；
NULL 或 zero-capacity 失敗時不寫入。

wrapper `CBM_USE_RUST_PIPELINE_ARTIFACT_PATH=1` 透過
`cbm_rs_pipeline_artifact_path_v1`；成功回完整所需長度，short/invalid 回 `SIZE_MAX`，但 short 仍寫
prefix + NUL。direct `CBM_USE_RUST_PIPELINE_ARTIFACT_PATH_ONLY=1`／Cargo
`pipeline-artifact-path-only` 匯出同名 public bool ABI，short 同樣 prefix + false。default/wrapper
編入 C bridge，direct `ARTIFACT_PATH_SRCS =`，且 runner/production staticlib 選擇已涵蓋 `_ONLY`。
已通過 fmt、artifact unit **4 passed**、clippy；opt-in default/wrapper 與最終重跑 only default/direct
均通過 FFI、artifact **10 passed**、production `--version`；only gate 另通過 empty selector 與
`make -Bn` source-negative。完整 `scripts/test.sh` 未跑。

### 原預算外後續：第 24 項 AST profile classifiers true-source

AST profile classifiers 已完成後續 true-source slice，不改寫原始 **21 / 21**、第 22 項原預算外
true-source，或第 23 項既有 artifact-path parity/gate hardening（非新 true-source）的歷史定位。
C fallback 已拆至 `src/semantic/ast_profile_classifiers.c/.h`；`src/semantic/ast_profile.c` 保留
Tree-sitter traversal 與 codec。wrapper `CBM_USE_RUST_AST_PROFILE_CLASSIFIERS=1` 經 C bridge 呼叫
既有 v1 ABI；direct `CBM_USE_RUST_AST_PROFILE_CLASSIFIERS_ONLY=1`／Cargo
`ast-profile-classifiers-only` 排除 C fallback，由 Rust 匯出同名
`cbm_ast_profile_kind_flags`、`cbm_ast_profile_halstead_insert` 與
`cbm_ast_profile_is_param_name`。契約為 kind flags、固定 512 槽 Halstead insert、parameter-name
比對，並使防禦性 NULL／無效輸入與 Rust ABI 對齊。

已通過 fmt、`cargo test -p cbm-core ast_profile_classifiers --locked`（**3 passed**）與 clippy；
`rust-ast-profile-classifiers-optin-test` 的 default/wrapper FFI、pipeline **228 passed**、production
`--version`，以及 `rust-ast-profile-classifiers-only-test` 的 default/direct 相同檢查、
`AST_PROFILE_CLASSIFIERS_SRCS =` 與靜默成功的 `make -Bn` source-negative 均成功。完整
`scripts/test.sh` 未跑。

### 原預算外後續：第 25 項 Discover filters true-source

本項是原始 **21 / 21** 後的後續 true-source slice，不改變第 22 項原預算外 true-source 或第
23 項 artifact-path parity/gate hardening（非新 true-source）的歷史定位。C fallback 為
`src/discover/filters.c`，只取代 `cbm_should_skip_dir`、`cbm_has_ignored_suffix`、
`cbm_should_skip_filename` 與 `cbm_matches_fast_pattern` 四個 predicate；walk、ignore
組態與其餘 discover orchestration 仍為 C。

`CBM_USE_RUST_DISCOVER_FILTERS=1` wrapper 經 C bridge 呼叫既有 v1 ABI；
`CBM_USE_RUST_DISCOVER_FILTERS_ONLY=1` direct 使用 Cargo `discover-filters-only`。Rust
direct ABI 原已存在，並非本項新寫；direct 的 `DISCOVER_FILTERS_SRCS =`，且 production
`make -Bn` source-negative 已靜默通過。NULL 一律回 `false`；僅
`CBM_MODE_FULL`（0）為 full，任何非零 mode 均為 restricted；比對維持 raw byte 精確與
大小寫敏感。wrapper 啟用 Rust bridge 時，C fallback filter tables 會以條件編譯排除，以避免
`-Werror` unused-variable。

已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core discover --locked`
（**5 passed**）及 `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`。
`rust-discover-filters-optin-test` 已通過 default/wrapper FFI、Discover 各 **89 passed** 與
正式版 `--version`；`rust-discover-filters-only-test` 已通過 default/direct FFI、direct
Discover **89 passed**、正式版 `--version`、空白 selector 與靜默 source-negative。

### 2026-07-17 Discover path_join parity 修正（既有 true-source）

此為既有 path_join true-source 的 superseding parity 紀錄與 C/Rust regression 補強，不新增
true-source，不改變公開 ABI、Cargo feature、C fallback 或 direct selector；舊有「尚未驗證」歷史
文字不修改。Rust path_join 在輸出寫入、截斷並將 `\` 轉為 `/` 後，若開頭是 ASCII 小寫
drive、第二 byte 為 `:`，且第三 byte 為 `/` 或字串於第二 byte 結束，會大寫 drive。故
`c:/tmp`／`c:\\tmp` 變 `C:/tmp`，bare `c:` 變 `C:`；`c:relative` 不變，截斷
`c:x` 至 `c:` 亦變 `C:`。

已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core discover --locked`
（**5 passed**）與 `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`。
`rust-discover-path-join-optin-test` 已通過 default/wrapper FFI、Discover 各 **89 passed** 與
wrapper 正式版 `--version`；`rust-discover-path-join-only-test` 已通過 default/direct FFI、
Discover 各 **89 passed**、direct 正式版 `--version`、空白
`DISCOVER_PATH_JOIN_SRCS =` 與靜默 source-negative。

### 原預算外後續：第 26 項 Discover local_rel_path offset true-source

本項是後續 true-source slice，不改寫原始 **21 / 21**、第 22 項原預算外 true-source、第 23 項
非新增 true-source，或第 25 項與 path_join parity 的歷史定位。C fallback 為
`src/discover/local_rel_path.c/.h`；`discover.c` 兩個巢狀 `.gitignore` callsite 已從 static
pointer helper 改為 offset projection。

API 為 `size_t cbm_discover_local_rel_path_offset(const char *rel_path, const char *local_prefix)`。
NULL rel/prefix、空 prefix、未匹配均回 `0`；僅 byte-exact、大小寫敏感 prefix 且下一 byte 是
`/` 時回 `prefix_len + 1`。不正規化、不配置、不修改、不保存指標。

`CBM_USE_RUST_DISCOVER_LOCAL_REL_PATH=1` wrapper 經 v1 ABI；
`CBM_USE_RUST_DISCOVER_LOCAL_REL_PATH_ONLY=1` direct／Cargo
`discover-local-rel-path-only` 使用 direct ABI。direct 的
`DISCOVER_LOCAL_REL_PATH_SRCS =`，source-negative 已靜默通過。

已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core local_rel_path --locked`
（**5 passed**）與 `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`。
`rust-discover-local-rel-path-optin-test` 已通過 default/wrapper FFI、Discover 各 **90 passed**、
default/wrapper 正式版 `--version`；`rust-discover-local-rel-path-only-test` 已通過
default/direct FFI、Discover 各 **90 passed**、default/direct 正式版 `--version`、空白 selector
與靜默 source-negative。

### 2026-07-17 Route-node classifiers 既有 true-source ABI/gate hardening

本項是既有 route-node classifiers true-source 的 ABI/gate hardening，絕非新 true-source slice；
原始 **21 / 21**、第 22 項原預算外、第 23 項非新增 true-source、第 25、26 項及 path_join parity
歷史均不變。新增 `src/pipeline/route_node_classifiers.h`，僅凍結既有 `hash_segment` 與
`broker_route` API 契約，不把既有 C/Rust 演算法重新分類為新實作。

`hash_segment`：NULL、長度 0、長度大於 12 拒絕；僅允許小寫 ASCII 字母／數字；長度至少 3
且含字母拒絕，短的合法字母／數字與全數字可接受；大寫、符號、NUL、非 ASCII 拒絕。
`broker_route`：NULL 回 `false`，必須從首 byte 以大小寫敏感方式匹配七個既有 prefix；
`__route__GET__` 為負例。

Make 的 direct macro 已修正為 `CBM_USE_RUST_PIPELINE_ROUTE_NODE_CLASSIFIERS_ONLY`；FFI
support 使用 `ROUTE_NODE_CLASSIFIERS_SRCS` selector，direct 排除既有 C fallback。

已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::route --locked`
（**13 passed**）與 `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`。
`rust-pipeline-route-node-classifiers-optin-test` 已通過 default/wrapper FFI、route_canon
pipeline 各 **241 passed**、正式版 `--version`；`rust-pipeline-route-node-classifiers-only-test`
已通過 default/wrapper/direct FFI、route_canon pipeline 各 **241 passed**、正式版
`--version`、direct 空白 `ROUTE_NODE_CLASSIFIERS_SRCS =` 與靜默 `make -Bn`
source-negative。

### 2026-07-17 Discover language `.m` marker helpers 既有 true-source parity/ABI/gate hardening

本項是既有 Discover language `.m` marker helpers true-source 的 parity、ABI 與 gate hardening，
不是新 true-source slice；原始 **21 / 21** 歷史維持不變，也不重新分類既有 C/Rust 演算法為新實作。

kind 2 契約已凍結為 ASCII byte-only：NULL 與 invalid kind 行為不變；callable 名稱必須至少有一個
`[A-Za-z]`，且下一 byte 必須立刻為 `(`。高位元組 `\xE9`、空名稱 `"intrinsic ("`、
空白或任何非 `(` 的下一 byte 都是負例並回 `false`。C 已不再使用 locale-sensitive
`isalpha`，與 Rust `is_ascii_alphabetic` 收斂；C/Rust/FFI regressions 已補齊。兩個 Make
gate 的 default 階段均增加 isolation production `--version`。

已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core discover --locked`
（**11 passed**）與 `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`。
`make -j1 -f Makefile.cbm rust-discover-language-markers-optin-test` 已通過 default/wrapper
FFI、Discover runner **90 passed**、兩邊 production `--version` 成功；
`make -j1 -f Makefile.cbm rust-discover-language-markers-only-test` 整體 exit 0，已通過
default/wrapper/direct FFI、Discover runner、production，direct selector
`DISCOVER_LANGUAGE_MARKERS_SRCS =` 空白與靜默 `make -Bn` source-negative。

### 2026-07-17 Pipeline JSON property 既有 true-source ABI/gate hardening

`pipeline-json-prop` 是既有 true-source 的 ABI/gate hardening，不是新 true-source slice；歷史統計
維持 **21 / 21**。C 公開 ABI 保持
`bool cbm_pipeline_extract_json_prop(const char *json, const char *key, char *buf, int bufsz)`；
v1 ABI 回傳 `int`，direct ABI 回傳 `bool`，Rust FFI 的 buffer 長度已改為有號 `c_int`。

NULL `json`／`key`／`buf` 或 `bufsz <= 0` 一律回 `false` 且不寫入 buffer；空 key 可擷取
`""` property，空 value 回 `false` 且 buffer 不變。key 最多 59 bytes，60 bytes 以上（含任何
`snprintf` 截斷）拒絕且不寫入。實作仍是 raw scanner，不宣稱為 JSON parser；維持 first-NUL C
string 與第一個 byte hit 的既有語意。

FFI support source 現使用 `$(PIPELINE_JSON_PROP_SRCS)`；default、wrapper、direct 三模式均經
public bridge，兩個 gate 的 default leg 均補 isolation production `--version`。

已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline::route --locked`
（**14 passed**）、`cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`、
`make -j1 -f Makefile.cbm rust-pipeline-json-prop-optin-test` 與
`make -j1 -f Makefile.cbm rust-pipeline-json-prop-only-test`。三種 C pipeline runner 均為
**229 passed**，三種 production `--version` 均成功；direct selector
`PIPELINE_JSON_PROP_SRCS =` 與 source-negative 均通過。

### 原預算外後續：第 27 項 Pipeline LSP classifiers true-source

`pipeline-lsp-classifiers` 是新的 true-source slice，不是既有 ABI hardening；總計由
**21 / 21** 推進至 **22 / 22**。C fallback 已拆至
`src/pipeline/lsp_cross_classifiers.c/.h`，`pass_lsp_cross.c` 保留 cross-LSP orchestration、
arena、registry、resolver dispatch 與結果合併。

同名 ABI 為 `cbm_pxc_ts_modes(CBMLanguage, const char *, bool *, bool *, bool *)`、
`cbm_pxc_has_cross_lsp(CBMLanguage)`、`cbm_pxc_map_label(const char *)`；header 以
`sizeof(CBMLanguage) == sizeof(int)` 固定 Rust `c_int` 邊界。TS mode bits 是
`1=js`、`2=jsx`、`4=dts`；`rel_path == NULL` 保留 language defaults，三個 output 指標仍是
既有非 NULL 前置條件。cross-LSP 支援集合固定為 13 個 language。九個允許 label 採 raw
first-NUL 比對，允許時回原始 borrowed input pointer，NULL／拒絕皆回 NULL。

`CBM_LANG_CSHARP` 的 true 表示 pipeline eligibility，需預建 cs registry；
`cbm_pxc_run_one` fallback 不承諾 C# dispatch，未改變既有集合或 dispatch 行為。
wrapper 保留既有 v1 `int` ABI；direct 使用 Cargo `pipeline-lsp-classifiers-only` 與
`CBM_USE_RUST_PIPELINE_LSP_CLASSIFIERS_ONLY`。Make selector 與 FFI support source 已接入
`PIPELINE_LSP_CLASSIFIERS_SRCS`，direct 排除 C fallback。

已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline_lsp_cross --locked`
（**3 passed**）及 `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`。
兩個對應 Make gate 均通過：optin 的 default/wrapper，以及 only 的 default/direct，四組 FFI、
pipeline runner（各 **230 passed**）與 production `--version` 全數成功；direct
`PIPELINE_LSP_CLASSIFIERS_SRCS =` 和 source-negative 均通過。

### 原預算外後續：第 28 項 Pipeline split-command true-source

pipeline-split-command 是新的 true-source slice，總計由 **22 / 22** 推進至
**23 / 23**。C fallback 已抽至 src/pipeline/split_command.c/.h；既有
src/pipeline/pass_compile_commands.c 保留 compile_commands orchestration 與既有 resolve_path
wrapper，resolve_path 不屬於本次遷移，不能視為已移轉。

公開 ABI 維持不變：
int cbm_split_command(const char *cmd, char **out, int max_out)。輸入是 first-NUL 截斷的
raw-byte C string。NULL cmd、NULL out 或 max_out <= 0 回 0 且不寫 out；有效結果只保證
[0, count) slot，沒有 out[count] sentinel。每個 token 最大 payload 為 4095 bytes；只在 quote
外以 ASCII 空白或 Tab 分隔，單引號與雙引號會移除並可容納空白，empty quote 不產生 token，
未閉合 quote 仍輸出已收集內容，反斜線不具跳脫語意。

wrapper 使用 CBM_USE_RUST_PIPELINE_SPLIT_COMMAND，透過既有 v1 取得 C malloc 配置的 token，
呼叫端仍以 free 回收。direct-only 使用 Cargo feature pipeline-split-command-only 與
CBM_USE_RUST_PIPELINE_SPLIT_COMMAND_ONLY；PIPELINE_SPLIT_COMMAND_SRCS = 空白，Rust 匯出同名
public ABI。正常資源路徑的字元、輸出與 ownership parity 已驗證；但 OOM allocation 不作 ABI
parity 承諾：既有 C fallback 的 strdup 失敗會留下 NULL slot 卻仍遞增 count，既有 caller 亦有
後續解參考 NULL 的風險；Rust 先配置 Vec 再配置 C malloc，配置順序與 OOM 效果不同。

已通過 cargo fmt --all -- --check、cargo test -p cbm-core pipeline_compile_commands --locked
（10 passed）及 cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings。
rust-pipeline-split-command-optin-test 與 rust-pipeline-split-command-only-test 已通過
default/wrapper/direct FFI、pipeline runner 各 **231 passed** 與 production --version。
direct 已確認 PIPELINE_SPLIT_COMMAND_SRCS = 空白、make -Bn source-negative 不連結
src/pipeline/split_command.c，並正向確認 production 仍連結
src/pipeline/pass_compile_commands.c。

### 新 session 短啟動清單

1. 先讀 `AGENTS.md`、權威交接快照與本文件，保留既有 dirty worktree。
2. #28 已完成；除非 #28 範圍再次變更，不得重新推進、重跑或重複計數。
3. complexity JSON 明確延後，必須先另行 hardening C strtol 的 locale whitespace、overflow 與
   NULL key 差異；不得把它宣稱為下一個已選定候選。
4. Grok 新 session 只能先做唯讀 inventory；inventory 未完成前不可杜撰候選、實作或執行 gate。
5. 只在契約凍結與實際 gate 成功後，才建立新的 true-source 記錄。

1–10. 既有 path_join … simhash jaccard（見 current handoff 表）  
11. **Envscan classifiers** — `envscan_classifiers.c`；pipeline 各 **227**  
12. **Configlink helpers** — `configlink_helpers.c`；pipeline 各 **227**  
13. **Pipeline parallel JSON escape/length** — `parallel_json.c`；pipeline 各 **227**  
14. **Pipeline definitions JSON escape/length** — `definitions_json.c`；pipeline 各 **227**  
15. **Discover language `.m` marker helpers** — `language_markers.c`；discover 各 **87**  
16. **Pipeline similarity fingerprint parser** — `similarity_fp.c`；pipeline 各 **227**  
17. **Pipeline infrascan JSON cleaner** — `infrascan_json.c`；pipeline 各 **227**  
18. **Pipeline test-node is_test classifier** — `test_node_is_test.c`；pipeline 各 **228**  

另：`cargo test -p cbm-core --locked` **307**、clippy、fmt 有 durable log；
`rust-pipeline-parallel-json-optin-test` 與 `rust-pipeline-parallel-json-only-test` 均通過，涵蓋
default/wrapper/direct FFI smoke、pipeline suite 各 **227**、wrapper/direct production `--version`、
ONLY selector 空值與 `make -Bn` source-negative 不含 `src/pipeline/parallel_json.c`。  
`rust-pipeline-definitions-json-optin-test` 與
`rust-pipeline-definitions-json-only-test` 均通過，涵蓋 default/wrapper/direct FFI smoke、
pipeline suite 各 **227**、wrapper/direct production `--version`、ONLY selector 空值與
`make -Bn` source-negative 不含 `src/pipeline/definitions_json.c`。  
`rust-discover-language-markers-optin-test` 與
`rust-discover-language-markers-only-test` 均通過，涵蓋 default/wrapper/direct FFI smoke、
discover suite 各 **87**、wrapper/direct production `--version`、ONLY selector 空值與
`make -Bn` source-negative 不含 `src/discover/language_markers.c`。  
`rust-pipeline-similarity-fp-optin-test` 與
`rust-pipeline-similarity-fp-only-test` 均通過，涵蓋 default/wrapper/direct FFI smoke、
pipeline suite 各 **227**、wrapper/direct production `--version`、ONLY selector 空值與
`make -Bn` source-negative 不含 `src/pipeline/similarity_fp.c`。  
`rust-pipeline-infrascan-json-optin-test` 與
`rust-pipeline-infrascan-json-only-test` 均通過，涵蓋 default/wrapper/direct FFI smoke、
pipeline suite 各 **227**、wrapper/direct production `--version`、ONLY selector 空值與
`make -Bn` source-negative 不含 `src/pipeline/infrascan_json.c`。  
`rust-pipeline-test-node-is-test-optin-test` 與
`rust-pipeline-test-node-is-test-only-test` 均通過，涵蓋 default/wrapper/direct FFI smoke、
pipeline suite 各 **228**、wrapper/direct production `--version`、ONLY selector 空值與
`make -Bn` source-negative 不含 `src/pipeline/test_node_is_test.c`。  
完整 `scripts/test.sh` **尚未**重跑。產品 default 仍是 C11。

### 第 19 個 true-source：semantic camel-break classifier

19. **Semantic camel-break classifier** — 將 pure predicate 的 C fallback 拆至
`src/semantic/camel_break.c/.h`。default C fallback、既有
`CBM_USE_RUST_SEMANTIC_TOKEN_CLASSIFIERS=1` wrapper 與 narrow
`CBM_USE_RUST_SEMANTIC_CAMEL_BREAK_ONLY=1` direct 三態 gate 均成功；Cargo direct feature 為
`semantic-camel-break-only`。public bridge 與 Rust direct ABI 同名
`cbm_semantic_is_camel_break`，無跨 ABI 配置。NULL 或 `index <= 0` 回傳 false；非 NULL 正 index
必須指向 NUL 結尾名稱的有效非 NUL byte，僅 ASCII 前小寫、後大寫的邊界回傳 true。三態 pipeline
各 **228 passed**，wrapper/direct production `--version` 成功；direct
`SEMANTIC_CAMEL_BREAK_SRCS =`，且 `make -Bn` source-negative 證明未連結
`src/semantic/camel_break.c`。tokenization、delimiter、semantic embedding、所有權與 pipeline
side effects 持續由 C 管理。

### 第 20 個 true-source：semantic token delimiter

20. **Semantic token delimiter** — 將 pure byte predicate 的 C fallback 拆至
`src/semantic/token_delim.c/.h`。default C fallback、既有
`CBM_USE_RUST_SEMANTIC_TOKEN_CLASSIFIERS=1` wrapper 與 narrow
`CBM_USE_RUST_SEMANTIC_TOKEN_DELIM_ONLY=1` direct 三態 gate 均成功；Cargo direct feature 為
`semantic-token-delim-only`。public bridge 與 Rust direct ABI 同名
`cbm_semantic_is_token_delim(unsigned char byte)`，無 nullable pointer、buffer 或跨 ABI 配置。
僅 `.`、`/`、`_`、`-`、空白、`(`、`)`、`,`、`:` 回傳 true；其他 byte（含 `0x80`、`0xff`）
回傳 false。三態 pipeline 各 **228 passed**，三路 production `--version` 成功；direct
`SEMANTIC_TOKEN_DELIM_SRCS =`，且 `make -Bn` source-negative 證明未連結
`src/semantic/token_delim.c`。Rust fmt、token-classifier unit **2 passed**、clippy 亦成功。
camel-break 的 C/Rust direct 狀態獨立，delimiter ONLY 不替換 `camel_break.c`；tokenization、
semantic orchestration、embedding、ownership 與 pipeline side effects 持續由 C 管理。完整
`scripts/test.sh` 尚未重跑。

### 後續處理

### 第 21 個 true-source：pipeline registry test-QN classifier

21. **Pipeline registry test-QN classifier** — 將 pure score classifier 的 C fallback 拆至
`src/pipeline/registry_test_qn.c/.h`。default C fallback、
`CBM_USE_RUST_PIPELINE_REGISTRY_TEST_QN=1` wrapper 與 narrow
`CBM_USE_RUST_PIPELINE_REGISTRY_TEST_QN_ONLY=1` direct 三態 gate 均成功；Cargo direct feature 為
`pipeline-registry-test-qn-only`。public bridge 與 Rust direct ABI 同名
`bool cbm_pipeline_registry_is_test_qn(const char *qn)`。NULL／empty 回 false；非 NULL 必須為
有效 NUL 結尾 C string，以大小寫敏感 raw `strstr` needles `Test`、`test`、`Mock`、`mock`、
`Stub`、`stub`、`Fake`、`fake`、`Fixture`、`spec` 判定，故 `contest` 回 true，non-UTF8 byte
亦維持 raw-safe 行為。wrapper 維持既有 v1 ABI，非 `0`／`1` 回傳會回退 C。三態 registry 各
**54 passed**，三路 production `--version` 成功；direct
`PIPELINE_REGISTRY_TEST_QN_SRCS =`，且 `make -Bn` source-negative 證明未連結
`src/pipeline/registry_test_qn.c`。Rust fmt、`cargo test -p cbm-core pipeline::registry --locked`
**4 passed**、clippy 亦成功。此切片只切換 test-QN score classifier；registry handles、cache、
resolution 與 graph 持續由 C 管理，不代表 `CBM_USE_RUST_PIPELINE_REGISTRY` 整體 registry
已替換。完整 `scripts/test.sh` 尚未重跑。

### 非 true-source：LSP language enum parity 修正

Rust `LANG_C` 更正為 `14`，Bash `15` 為 false；Rust unit **3 passed**，wrapper FFI 與
pipeline **228 passed**。此為 enum parity 修正，沒有 direct source selector，且不增加
true-source 計數。

### 後續處理

- 本輪 `21/21` true-source 切片均已完成，無剩餘切片。
- 若另開新範圍，先完成 audit，確認 C fallback、direct ABI、gate 與文件的邊界一致。  
- 不要把 macro wrapper、歷史 passed 數當成完成；source-negative 一律 `make -Bn`

## 接手前必讀

- 工作目錄：`/Users/jeff/Documents/repos/codebase-memory-mcp`
- 專案指引：所有文件、註解與對話使用繁體中文（zh-TW）。
- 僅在使用者或交接內容明確指定時，才讀取附帶的 pasted log：`/Users/jeff/.codex/attachments/172bed05-bda5-4b49-ad8d-a883837c7c21/pasted-text-1.txt`。
- 不要清理或 revert 既有 dirty worktree；未追蹤檔也先保留。
- 手動編輯請用 `apply_patch`。格式化命令可直接執行。

## 歷史工作記錄（非當前快照）

## 本次工作階段（2026-07-11 diagnostics formatter production opt-in）

- 已將 `cbm_rs_diag_env_enabled_value`、`cbm_rs_diag_query_snapshot_values`、`cbm_rs_diag_format_path`、`cbm_rs_diag_format_json` 與 `cbm_rs_diag_format_ndjson` 接入 `CBM_USE_RUST_DIAGNOSTICS_FORMAT=1`；C wrapper 保留原邏輯，Rust ABI 回傳異常時 fallback 到 C。
- 此切片只替換 deterministic diagnostics pure logic；atomic query stats 的載入、system metrics、writer thread、檔案寫入/rename、NDJSON rotation、stderr lifecycle 與 diagnostics ownership 仍由 C 執行。
- 已通過：`cargo fmt --all -- --check`、`cargo test -p cbm-core foundation::diagnostics --locked`（3 passed）、`make -f Makefile.cbm rust-ffi-test`（exit 0）、`make -f Makefile.cbm rust-diagnostics-format-optin-test`（default/opt-in diagnostics suite 各 7 passed）、`clang-format --dry-run --Werror src/foundation/diagnostics.c` 與 `git diff --check`。全 tree `lint-format` 仍受既有 pipeline/traces dirty 變更的排版違規影響，未記為通過。
- `RUST_ALL_OPTIN_COMMON_FLAGS` 目前有 42 個 common production flags；2026-07-12 的 `rust-all-optin-test` 已驗證含 Cypher single/two-character lexer、logger env parser 與 Store language map opt-in 的 wrapper/direct 隔離變體，完整 runner 各 5867 passed，兩個 production binary 均完成 `--version` smoke。這仍只是 prerequisite，不代表 Rust-backed RC 或預設切換。
- `CBM_USE_RUST_DUMP_VERIFY=1` 的 ratio parser 已改由 Rust byte-level implementation 實際執行，不再透過 C `strtod`；支援 `0x1p-1` 與既有 partial-prefix contract。已通過 Rust unit 3 passed、opt-in `test-foundation` 325 passed、clippy、`rust-ffi-test`、fmt 與 diff check。

這個 repo 是 C11 MCP 伺服器，Rust workspace 位於 `rust/`。重構策略是逐步新增 Rust parity/helper，透過 `CBM_USE_RUST_*` opt-in 委派低風險純函式；預設產品路徑仍以 C 為主，完整 Rust-backed 預設切換尚未完成。

本輪 route-node classifier 已完成 implementation、完整 Rust、FFI、fmt、shell syntax、diff check 與 default/opt-in pipeline gates；`CBM_USE_RUST_PIPELINE_ROUTE_NODE_CLASSIFIERS=1` 已接入 `pass_route_nodes.c` 的 hash segment 與 broker-route 判定。上一輪的 route argument helper、module-directory classifier 與 gitdiff parsers 已完成。所有 opt-in target 會重建共享的 `build/c`，續跑時請序列執行。

目前 worktree 已有大量累積變更，主要集中於：

- `Makefile.cbm`
- `scripts/rust-check.sh`
- `scripts/test.sh`
- `src/pipeline/fqn.c`
- `src/pipeline/artifact.c`
- `src/pipeline/pass_configures.c`
- `src/cli/progress_sink.c`
- `src/cli/progress_sink.h`
- `src/traces/traces.c`
- `rust/cbm-core/src/pipeline/artifact.rs`
- `rust/cbm-core/src/pipeline/fqn.rs`
- `rust/cbm-core/src/mcp/mod.rs`
- `rust/cbm-core/src/ffi.rs`
- `rust/cbm-core/src/pipeline/exception.rs`
- `rust/cbm-core/src/pipeline/configures.rs`
- `src/pipeline/pass_usages.c`
- `src/pipeline/pass_parallel.c`
- `src/mcp/mcp.c`
- `tests/test_rust_ffi.c`
- `Rust-Refactor.md`
- `Tasks.md`
- `rust/CBM_FFI.md`

已知未追蹤項目仍存在，請勿任意刪除：

- `pkg/npm/codebase-memory-mcp-0.8.1.tgz`
- `rust/cbm-core/src/cli/`
- `rust/cbm-core/src/pipeline/route.rs`
- `rust/cbm-core/src/pipeline/test_detect.rs`
- `rust/cbm-core/src/traces.rs`
- `rust/cbm-core/src/pipeline/configures.rs`
- `scripts/__pycache__/`
- `tests/test_progress_sink.c`
- 其他尚在持續變動的 Rust / C 工作檔，請以 `git status --short` 為準。

## 最近完成的切片

### Traces Rust-only opt-in

- `CBM_USE_RUST_TRACES_ONLY=1` 時，C build 不編譯 `src/traces/traces.c`；Cargo `traces-only` feature 的 Rust staticlib 直接匯出 `traces.h` 的 `cbm_extract_service_name`、`cbm_extract_http_info`、`cbm_extract_path_from_url`、`cbm_parse_duration`、`cbm_calculate_p99`。
- 預設 C fallback 與既有 `CBM_USE_RUST_TRACES=1` wrapper 均保留。C link 使用 `target/cbm-default` 或 `target/cbm-traces-only`，避免不同 Cargo feature 產生的 archive 快取混用。
- `rust-traces-only-test` 使用 `build/c/rust-traces-default` 與 `build/c/rust-traces-only` 隔離 runner，依序執行 default / Rust-only `traces mcp` suite；兩組皆為 161 passed。Rust-only archive 已驗證含五個 direct C ABI symbols，且連結行不含 `src/traces/traces.c`。

### Traces 安全邊界收斂

- `rust/cbm-core/src/traces.rs` 已收斂為純位元組/值型 helper，不再持有 C 指標、寫入 caller buffer 或保存 static URL buffer。
- `rust/cbm-core/src/ffi.rs` 集中處理 trace C ABI 的指標解碼、C struct 寫入與 URL static buffer；`HttpPath::Url` 已直接複製已解析 path，避免二次 URL 解析。
- `scripts/rust-security-audit.py` 的 unsafe 掃描改為只辨識 Rust 關鍵字語法，避免 JSON/schema 說明文字誤報；`traces.rs` 不需新增 allowlist。
- 已驗證：`cargo fmt --all -- --check`、`cargo test -p cbm-core traces --locked`（7 passed）、`make -f Makefile.cbm rust-ffi-test`、`python3 scripts/rust-security-audit.py`（557 findings allowlist passed）與 `git diff --check`。未重跑會清除共享 `build/c` 的 traces opt-in target。

### Pipeline configures Rust-only opt-in

- `CBM_USE_RUST_PIPELINE_CONFIGURES_ONLY=1` 時，C build 不編譯 `src/pipeline/pass_configures.c`；Cargo `pipeline-configures-only` feature 的 Rust staticlib 直接匯出 `pipeline_internal.h` 的 `cbm_is_env_var_name`、`cbm_normalize_config_key`、`cbm_has_config_extension`。
- 預設 C fallback 與既有 `CBM_USE_RUST_PIPELINE_CONFIGURES=1` wrapper 均保留。staticlib 使用 `target/cbm-default`、`target/cbm-traces-only`、`target/cbm-pipeline-configures-only` 或 `target/cbm-traces-configures-only`，依 feature 組合隔離 archive cache。
- `rust-pipeline-configures-only-test` 使用 `build/c/rust-pipeline-configures-default` 與 `build/c/rust-pipeline-configures-only`，依序執行 default pipeline、direct ABI FFI smoke 與 Rust-only pipeline；兩個 pipeline suite 各 219 passed。direct ABI smoke 固定 null input、null/zero/short output buffer、NUL 結尾、512-byte key 截斷與 token count。
- Rust 僅替代三個 config helper；config link pass、graph-buffer 寫入、檔案解析與其餘 pipeline side effects 仍在 C。

### 手動 all-optins 組合 prerequisite gate

- `rust-all-optin-test` 定義 39 個 production Rust opt-in 的首次全組合 prerequisite。wrapper 變體使用 `CBM_USE_RUST_TRACES=1` 與 `CBM_USE_RUST_PIPELINE_CONFIGURES=1`；direct 變體僅以 `CBM_USE_RUST_TRACES_ONLY=1` 與 `CBM_USE_RUST_PIPELINE_CONFIGURES_ONLY=1` 取代這兩個旗標，絕不混用 wrapper/direct pair，也不納入 test-only parity。
- 兩個變體各使用獨立的 `build/c/rust-all-optin-wrapper`、`build/c/rust-all-optin-direct`，並在各自首個 build 前執行該目錄的 `clean-c`，避免上次不同 CFLAGS 留下 stale objects；依序建置並執行 `test-rust-ffi`、無 selector 的完整 `test-runner`、production `cbm` 與 `--version`。target 不清理共用 `build/c`，也保留既有 `RUST_TARGET` 與 staticlib feature cache 行為。
- 此 gate 刻意未接入 `rust-ci`、`rust-ci-tests`、`scripts/rust-check.sh` 或 `scripts/test.sh`，避免日常 CI 成本暴增；2026-07-12 已以 42 個 common flags 重跑，wrapper/direct 完整 runner 各 5867 passed，並完成各自 production `--version` smoke。
- 它只是首次全組合 prerequisite，不構成 Rust-backed release candidate（RC）證明，也不代表預設產品路徑或 C fallback 已切換。

### Pipeline project-name opt-in

完成項目：

- Rust helper：沿用既有 `pipeline::fqn` byte-level project-name sanitizer。
- FFI：`cbm_rs_pipeline_project_name_from_path`
- C opt-in：`CBM_USE_RUST_PIPELINE_PROJECT_NAME=1` 時 `src/pipeline/fqn.c` 的 `cbm_project_name_from_path()` 先以 Rust length-only 探測，再配置 C-owned buffer 寫回；預設 C path 保持不變。
- 契約：`NULL` / empty / root fallback、Windows drive normalization、unsafe ASCII、Unicode byte hex encoding 與 truncation 與既有 test-only parity 一致。
- 邊界：只替換 project-name helper；project key、DB/cache key 與 MCP session path 的上層語意仍由 C 決定。

### Pipeline checked-exception classifier opt-in

完成項目：

- Rust helper：`pipeline::exception::is_checked_exception()`
- FFI：`cbm_rs_pipeline_is_checked_exception_v1`
- C opt-in：`CBM_USE_RUST_PIPELINE_CHECKED_EXCEPTION=1` 時 `src/pipeline/pass_usages.c` 與 `src/pipeline/pass_parallel.c` 的 `is_checked_exception()` 會先委派 Rust；預設 C path 保持不變。
- 契約：空字串與一般 checked exception 名稱回 true；含 `Error`、`Panic`、`error` 或 `panic` 的名稱回 false；null 回 false。
- 邊界：只遷移 pure classifier；`THROWS` / `RAISES` edge type 選擇、exception resolution、graph-buffer adapter 與 pass side effects 仍由 C 負責。

### Pipeline registry test-QN classifier opt-in

完成項目：

- Rust helper：`pipeline::registry::is_test_qn()`
- FFI：`cbm_rs_registry_is_test_qn_v1`
- C opt-in：`CBM_USE_RUST_PIPELINE_REGISTRY_TEST_QN=1` 時，`src/pipeline/registry.c` 的 `is_test_qn()` 只在 Rust 回傳 `0` 或 `1` 時採用該結果；其他回傳值回退既有 C classifier，預設 C path 不變。
- 契約：輸入為 nullable NUL-terminated QN；`NULL` 或空字串回 `0`。以 raw byte、大小寫敏感 substring 比對 `Test`、`test`、`Mock`、`mock`、`Stub`、`stub`、`Fake`、`fake`、`Fixture`、`spec`；非 UTF-8 bytes 不轉碼。
- 邊界：只影響 registry candidate score 的 test-like QN 判定；opaque registry handle、add/resolve/cache、所有權與其餘 pipeline side effects 仍由 C 負責。
- 已實跑：`cargo test -p cbm-core pipeline::registry --locked`（4 passed）、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm rust-pipeline-registry-test-qn-optin-test`（default/opt-in registry suite 各 53 passed），以及當時 38 個 common flags 的 `rust-all-optin-test`（wrapper/direct 完整 runner 各 5865 passed，兩個 production binary 的 `--version` smoke 均通過）。這是 acceptance 前置驗證，並非 Rust-backed release candidate。

### Cypher single-character lexer opt-in

完成項目：

- Rust helper：`cypher::single_char_kind_or_eof()`；FFI：`cbm_rs_cypher_single_char_kind_v1`。
- C opt-in：`CBM_USE_RUST_CYPHER_LEX_SINGLE_CHAR=1` 時，`src/cypher/cypher.c` 的 `lex_single_char()` 只接受 Rust 回傳的 `TOK_EOF`、`TOK_LPAREN..TOK_EQ` 或 `TOK_PIPE`；其他結果回退既有 C `switch`，預設 C path 不變。
- 契約：輸入僅接受 `0..=255` byte；15 個既有 Cypher 單字元符號回既有 token enum，未知 byte 回 `TOK_EOF`，範圍外整數回 `-1`。Rust 不借用、不保存或回傳 C 指標。
- 邊界：token allocation、position、two-character tokens、lexer/parser/AST/evaluator/executor 與公開錯誤行為均留在 C。
- 已實跑：`cargo test -p cbm-core cypher --locked`（15 passed）、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm rust-cypher-lex-single-char-optin-test`，以及更新後 `rust-all-optin-test`（wrapper/direct full runner 各 5866 passed，兩個 production `--version` smoke 均通過）。這不是 Rust-backed release candidate。

### Cypher two-character lexer opt-in

完成項目：

- Rust helper：`cypher::two_char_kind_or_eof()`；FFI：`cbm_rs_cypher_two_char_kind_v1`。
- C opt-in：`CBM_USE_RUST_CYPHER_LEX_TWO_CHAR=1` 時，`lex_try_two_char()` 只採用 Rust 回傳的 `TOK_NEQ`、`TOK_EQTILDE`、`TOK_GTE`、`TOK_LTE` 或 `TOK_DOTDOT`，並以 C-owned local text/token allocation 推進 cursor；`TOK_EOF`、`-1` 或其他 ABI 值回退既有 C pair table，預設 C path 不變。
- 契約：兩個輸入皆為 `0..=255` byte；`!=`、`<>`、`=~`、`>=`、`<=`、`..` 回既有 token enum，未知 pair 回 `TOK_EOF`，範圍外整數回 `-1`。Rust 不借用、不保存或回傳 C 指標。
- 邊界：input buffer、cursor、token text、token allocation、parser、AST/evaluator/executor 與公開錯誤行為均留在 C。
- 已實跑：`cargo test -p cbm-core cypher --locked`（16 passed）、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm rust-cypher-lex-two-char-optin-test`（default/opt-in Cypher + contract 各 166 passed），以及更新後 `rust-all-optin-test`（41 common flags；wrapper/direct full runner 各 5866 passed，兩個 production `--version` smoke 均通過）。這不是 Rust-backed release candidate。

### Foundation logger environment parser opt-in

完成項目：

- 既有 Rust `foundation::log` FFI：`cbm_rs_log_parse_level_value`、`cbm_rs_log_parse_format_value` 已接入 `CBM_USE_RUST_LOG_ENV_PARSE=1`。
- C 仍負責 `getenv()`、global logger state、sink、stderr 與 lifecycle；Rust 只解析 nullable NUL-terminated 值並回傳 scalar。C 只採用合法 level `0..4` 與 format `0..1`，異常 scalar 保持既有 current 值；預設 C parser 完整保留。
- 契約：level 以 ASCII case-insensitive 名稱或完整 C `strtol(..., 10)` 相容整數解析；只接受 `0..4`。format 只接受 ASCII case-insensitive `text`／`json`，不 trim 空白。`NULL`、空、未知、尾隨字元、超出範圍值皆維持 current。
- 已實跑：`cargo test -p cbm-core foundation::log --locked`（2 passed）、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm rust-foundation-optin-test`（default、各單一 opt-in 與含新 flag 的 final combined foundation path 均 325 passed），以及 40-common-flag `rust-all-optin-test`（wrapper/direct full runner 各 5866 passed，兩個 production `--version` smoke 均通過）。這不是 Rust-backed release candidate。

### Store extension-language map opt-in

- 已實作 `store::arch_helpers::ext_language_kind()`、`cbm_rs_store_ext_lang_kind_v1` 與 `CBM_USE_RUST_STORE_LANGUAGE_MAP=1`。Rust 只將既有 `ext_lang_table` 的 44 個副檔名映射為 `0..43` 索引；未知、`NULL` 或非對應大小寫回 `-1`，不配置或回傳字串指標。
- C `ext_to_lang()` 僅接受範圍內索引，並從既有 C 靜態 `ext_lang_table` 取回 language 指標；異常 ABI 回傳一律回退既有 C 查表。SQLite architecture query、語言計數/排序、summary allocation 與 Store lifetime 均留在 C。
- 已新增副檔名別名架構測試（`cc/cxx`、`jsx/tsx`、`yaml/yml` 與未知副檔名），並實跑 `cargo test -p cbm-core store::arch_helpers --locked`（7 passed）、`make -f Makefile.cbm rust-ffi-test`、`make -f Makefile.cbm rust-store-language-map-optin-test`（default/opt-in `store_arch mcp` 各 187 passed），以及 42-common-flag `rust-all-optin-test`（wrapper/direct full runner 各 5867 passed，兩個 production `--version` smoke 均通過）。這不是 Rust-backed release candidate。

### Pipeline artifact path opt-in

完成項目：

- Rust helper：`pipeline::artifact::artifact_path()`
- FFI：`cbm_rs_pipeline_artifact_path_v1`
- C opt-in：`CBM_USE_RUST_PIPELINE_ARTIFACT_PATH=1` 時 `src/pipeline/artifact.c` 的 `artifact_path()` 先委派 Rust；預設 C path 保持不變。
- 契約：`<repo>/.codebase-memory/<name>` 的 byte-level path 組裝；null repo/name、null buf、`bufsize == 0` 或短 buffer 回 `SIZE_MAX`，並保留 raw bytes。
- 邊界：只遷移 path builder；artifact export/import、`.gitattributes`、metadata、file I/O 與 pipeline side effects 仍由 C 負責。

### Pipeline infrascan filename classifier opt-in

完成項目：

- Rust helper：`pipeline::infrascan`
- FFI：六個 `cbm_rs_pipeline_is_*_v1` 檔名/副檔名 classifier
- C opt-in：`CBM_USE_RUST_PIPELINE_INFRASCAN=1` 時由 `src/pipeline/pass_infrascan.c` 委派；預設 C path 保持不變。
- 契約：保留 nullable C string、ASCII 不分大小寫、255-byte C lower-buffer 截斷與 `.sh` / `.bash` / `.zsh` 副檔名判斷。
- 邊界：k8s manifest content 判斷、secret detection、Dockerfile/.env/shell/Terraform/Helm parser、graph-buffer mutation 與 pipeline side effects 仍由 C 負責。

### Pipeline K8s dependency-manifest classifier opt-in

完成項目：

- Rust helper：`pipeline::infrascan::{is_helm_chart_file,is_gomod_file,is_requirements_file}`。
- FFI：`cbm_rs_pipeline_is_helm_chart_file_v1`、`cbm_rs_pipeline_is_gomod_file_v1`、`cbm_rs_pipeline_is_requirements_file_v1`。
- C opt-in：`CBM_USE_RUST_PIPELINE_K8S_FILE_CLASSIFIERS=1` 時，`src/pipeline/pass_k8s.c` 的 Helm、Go module 與 Python requirements 分流委派 Rust；預設 C path 保持不變。
- 契約：精準比對 `Chart.yaml` / `Chart.yml`、`go.mod` 與 `requirements.txt`；null 或其他名稱均回 false。
- 邊界：檔案讀取、Helm／Go／Python dependency parser、Package／Chart node 與 `DEPENDS_ON` edge 寫入、graph-buffer ownership 及所有 pipeline side effects 仍由 C 負責。
- 驗證：`cargo test -p cbm-core pipeline::infrascan --locked`（8 passed）、`make -f Makefile.cbm rust-ffi-test`、default／`CBM_USE_RUST_PIPELINE_K8S_FILE_CLASSIFIERS=1` 的 `build/c/test-runner pipeline`（各 219 passed）、`make -f Makefile.cbm rust-language-graph-parity`（default 與 Rust pipeline binary 均與 golden 一致）及 `git diff --check` 均通過。

### CLI progress sink Rust backend opt-in

完成項目：

- Rust backend：`cli::progress_sink`。
- FFI：`cbm_rs_progress_sink_init`、`cbm_rs_progress_sink_fini`、`cbm_rs_progress_sink_fn`。
- C opt-in：`CBM_USE_RUST_PROGRESS_SINK=1` 時，C wrapper 保留既有 public API 與 `cbm_log_set_sink()` 註冊點，將 structured log event format 與 pending newline flush 委派 Rust。
- 邊界：`FILE*` 仍由 C 傳入且不轉移所有權；log level／format、其餘 sink contract 與 CLI lifecycle 仍由 C 負責。

### MCP JSON-RPC response / cancellation codec opt-in

完成項目：

- Rust helper：`mcp::cancel_request_matches()`、`mcp::jsonrpc_format_response()` 與 `mcp::mcp_text_result()`。
- FFI：`cbm_rs_mcp_cancel_request_matches_v1`、`cbm_rs_mcp_jsonrpc_format_response_v1`、`cbm_rs_mcp_text_result_v1`。
- C opt-in：`CBM_USE_RUST_MCP_CODEC=1` 時只委派 request cancellation 比對、response JSON 與 text result JSON 組裝；ABI 異常時保留 C/yyjson fallback。
- 邊界：active pipeline cancellation、request logging、Content-Length transport、server lifecycle、tool dispatch 與 handlers 仍由 C 負責。

### MCP Content-Length raw length ABI 與超長 frame 同步修正

- `CBM_USE_RUST_MCP_CODEC=1` 新增 `cbm_rs_mcp_content_length_raw_v1`，讓 C 端取得已解析的宣告 body 長度，既有最大 frame gate 保留。
- 語法有效但超過可接受上限的 Content-Length frame，default C 與 Rust opt-in 都會先消耗完整 header/body，再繼續讀取下一個 frame，避免 transport stream 失同步。
- body I/O、排空、`cbm_mcp_server_handle()`、response framing、poll loop、server lifecycle、dispatch 與 handlers 仍由 C 負責。
- 已實跑：default `build/c/test-runner mcp`（134 passed）、`make -f Makefile.cbm rust-mcp-codec-optin-test`（134 passed）、`cargo test -p cbm-core mcp --locked`（69 passed）、`make -f Makefile.cbm rust-ffi-test`。

### Pipeline githistory trackable-file classifier opt-in

完成項目：

- Rust helper：`pipeline::githistory::is_trackable_file()`
- FFI：`cbm_rs_pipeline_is_trackable_file_v1`
- C opt-in：`CBM_USE_RUST_PIPELINE_GITHISTORY=1` 時由 `src/pipeline/pass_githistory.c` 委派；預設 C path 保持不變。
- 契約：保留 `.git/`、`node_modules/`、`vendor/`、`__pycache__/`、`.cache/` 前綴排除、固定 lock basename、`.lock`/`.sum`/minified/binary suffix 排除與 null 回 false。
- 邊界：git log 解析、change coupling、`FILE_CHANGES_WITH` edge、graph-buffer mutation、檔案 I/O 與 pipeline side effects 仍由 C 負責。

### MCP utf8_is_cont byte classifier opt-in

完成項目：

- Rust helper：`mcp::utf8_is_cont_byte()`
- FFI：`cbm_rs_mcp_utf8_is_cont_v1`
- C opt-in：`CBM_USE_RUST_MCP_CODEC=1` 時 `src/mcp/mcp.c` 的 `utf8_is_cont()` 會先委派 Rust；`sanitize_utf8_lossy()` Rust serializer 失敗時 fallback C loop，loop 內可使用此 helper。
- 契約：低 8-bit 符合 `10xxxxxx` 回 true，否則 false；高位輸入先 mask。
- 邊界：只遷移 pure byte classifier；source/snippet 讀取、JSON result building、transport、server lifecycle 與 handlers 仍由 C 負責。

### MCP find_tightest_node best-index helper opt-in

完成項目：

- Rust helper：`mcp::search_pick_tightest_index()`
- FFI：`cbm_rs_mcp_search_pick_tightest_index_v1`
- C opt-in：`CBM_USE_RUST_MCP_CODEC=1` 時 `src/mcp/mcp.c` 的 `find_tightest_node()` 會先委派 Rust 選擇最小有效 span 的 index；ABI 異常時 fallback 到原 C 逐筆比較邏輯。
- 契約：回傳最小且非負 span 的第一個 index；全 miss（全負值）或 invalid args 回 `-1`。
- 邊界：只遷移 pure best-index helper；node query、hit merge、result ranking、JSON result building、transport、server lifecycle 與 handlers 仍由 C 負責。

### MCP pick_resolved_node best-index helper opt-in

完成項目：

- Rust helper：`mcp::search_pick_resolved_index()`
- FFI：`cbm_rs_mcp_search_pick_resolved_index_v1`
- C opt-in：`CBM_USE_RUST_MCP_CODEC=1` 時 `src/mcp/mcp.c` 的 `pick_resolved_node()` 會先委派 Rust 計算 best index 與 ambiguous；ABI 異常時 fallback 到原 C 邏輯。
- 契約：回傳第一個 top score index；top score 多於一個時 `ambiguous=true`；invalid args 回 `-1` 並清 `ambiguous_out`。
- 邊界：只遷移 pure tie-break helper；score 計算、BFS traversal、result shaping、transport、server lifecycle 與 handlers 仍由 C 負責。

### MCP search_code tightest-node span helper opt-in

完成項目：

- Rust helper：`mcp::search_line_match_span()`
- FFI：`cbm_rs_mcp_search_line_match_span_v1`
- C opt-in：`CBM_USE_RUST_MCP_CODEC=1` 時 `src/mcp/mcp.c` 的 `find_tightest_node()` 會先委派 Rust 計算單一 node span；Rust 回 `-1` 時 fallback 到原 C 判斷。
- 契約：命中區間回 `end_line - start_line`，未命中回 `-1`。
- 邊界：只遷移 per-node span pure helper；node query、grep hit merge、result ranking、JSON result building、transport、server lifecycle 與 handlers 仍由 C 負責。

### MCP detect_changes status-path parser opt-in

完成項目：

- Rust helper：`mcp::detect_changes_status_path_offset()`
- FFI：`cbm_rs_mcp_detect_changes_status_path_offset_v1`
- C opt-in：`CBM_USE_RUST_MCP_CODEC=1` 時 `src/mcp/mcp.c` 的 `detect_changes` changed-files 迴圈會先委派 Rust 計算 path offset；ABI 異常時 fallback 到原 C parser。
- 契約：plain path 回 offset 0；`git status --porcelain` 略過 `XY ` 前綴；rename `old -> new` 取 destination；空路徑或 null 回 `SIZE_MAX`。
- 邊界：只遷移 changed-files path parser pure helper；git command execution、project/root resolution、Store symbol lookup、JSON result building、transport、server lifecycle 與 handlers 仍由 C 負責。

### MCP search_code directory top-key opt-in

完成項目：

- Rust helper：`mcp::search_top_dir()`
- FFI：`cbm_rs_mcp_search_top_dir_v1`
- C opt-in：`CBM_USE_RUST_MCP_CODEC=1` 時 `src/mcp/mcp.c` 的 `build_dir_distribution()` 透過 `search_result_top_dir()` 委派 Rust 擷取 top-level directory key。
- 契約：有 `/` 時保留到第一個 slash（含 slash），否則使用整個 file path；`file == NULL` 回 `SIZE_MAX`；支援 length-only 與短 buffer NUL 截斷。
- 邊界：只遷移 directory top-key pure helper；directory count、yyjson object building、`search_result_t` ownership、grep execution、graph node lookup、dedup/classification、result JSON、transport、server lifecycle 與 handlers 仍由 C 負責。

### MCP search_code score comparator opt-in

完成項目：

- Rust helper：`mcp::search_score_cmp()`
- FFI：`cbm_rs_mcp_search_score_cmp_v1`
- C opt-in：`CBM_USE_RUST_MCP_CODEC=1` 時 `src/mcp/mcp.c` 的 `search_result_cmp()` 委派 Rust 計算 descending score delta。
- 契約：left score 較高回負值，right score 較高回正值，相同分數回 0。
- 邊界：只遷移 qsort comparator scalar helper；qsort 呼叫、`search_result_t` ownership、grep execution、graph node lookup、dedup/classification、score 寫入、result JSON、transport、server lifecycle 與 handlers 仍由 C 負責。

### MCP manage_adr sections JSON builder opt-in

完成項目：

- Rust helper：`mcp::adr_sections_json()`
- FFI：`cbm_rs_mcp_adr_sections_json_v1`
- C opt-in：`CBM_USE_RUST_MCP_CODEC=1` 時 `src/mcp/mcp.c` 的 `adr_list_sections_from_content()` 委派 Rust 產生 sections JSON array，再用 `yyjson_mut_rawncpy()` 複製進 yyjson document。
- 契約：null/empty 輸出 `[]`；逐行移除行尾 `\r`；只接受第一個 byte 為 `#` 且非空的行；單一 header 最多保留 1023 bytes；支援 length-only 與短 buffer NUL 截斷。
- 邊界：只遷移 `manage_adr(mode=sections)` sections JSON array builder；project/store resolution、legacy file migration、ADR read/write、`cbm_mcp_text_result()`、JSON-RPC wrapper、transport、server lifecycle 與 handlers 仍由 C 負責。

### MCP project-list error JSON builder opt-in

完成項目：

- Rust helper：`mcp::project_list_error()`
- FFI：`cbm_rs_mcp_project_list_error_v1`
- C opt-in：`CBM_USE_RUST_MCP_CODEC=1` 時 `src/mcp/mcp.c` 的 `build_project_list_error()` 委派 Rust 建構固定 JSON 序列化。
- 契約：`count > 0` 時輸出 `available_projects` + `count`；否則輸出 `No projects indexed yet` hint。支援 length-only 與短 buffer NUL 截斷；`reason == NULL` 或 `count > 0 且 projects_csv == NULL` 回 `SIZE_MAX`。
- 邊界：只遷移 project-list error JSON 序列化 helper；cache directory 掃描、`collect_db_project_names()`、`build_no_store_error()` dispatch、Store resolution、`cbm_mcp_text_result()`、JSON-RPC wrapper、transport、server lifecycle 與 handlers 仍由 C 負責。

### MCP project-not-found message opt-in

完成項目：

- Rust helper：`mcp::project_not_found_message()`
- FFI：`cbm_rs_mcp_project_not_found_message_v1`
- C opt-in：`CBM_USE_RUST_MCP_CODEC=1` 時 `src/mcp/mcp.c` 的 `build_project_not_found_message()` 委派 Rust 建構固定 error text。
- 契約：固定回傳 `project not found`，支援 length-only 與短 buffer NUL 截斷；C 端只在 ABI 寫入長度正確時採用 Rust output。
- 邊界：只遷移 project-not-found 固定 message builder；project list 掃描、Store resolution、`cbm_mcp_text_result()`、JSON-RPC wrapper、transport、server lifecycle 與 handlers 仍由 C 負責。

### MCP missing project error object opt-in

完成項目：

- Rust helper：`mcp::missing_project_error()`
- FFI：`cbm_rs_mcp_missing_project_error_v1`
- C opt-in：`CBM_USE_RUST_MCP_CODEC=1` 時 `src/mcp/mcp.c` 的 `build_missing_project_error()` 委派 Rust 建構固定 JSON error object。
- 契約：固定 `{"error":"missing required argument: project", ...}` 與指向 `project` argument / `list_projects` 的 hint；支援 length-only 與短 buffer NUL 截斷；C 端只在 ABI 寫入長度正確時採用 Rust output。
- 邊界：只遷移 missing-project 固定 message builder；available project list 掃描、`build_project_list_error()`、Store resolution、`cbm_mcp_text_result()`、JSON-RPC wrapper、transport、server lifecycle 與 handlers 仍由 C 負責。

### MCP search_code strip root prefix opt-in

完成項目：

- Rust helper：`mcp::search_strip_root_prefix_offset()`
- FFI：`cbm_rs_mcp_strip_root_prefix_offset_v1`
- C opt-in：`CBM_USE_RUST_MCP_CODEC=1` 時 `src/mcp/mcp.c` 的 `strip_root_prefix()` 委派 Rust 計算 offset。
- 契約：只做 byte prefix 比對；prefix 後若是 `/` 再多跳過一個 byte；不檢查 path component boundary；exact root 回 `root_len`；`root_len == 0` 會跳過 leading `/`；`root_len > path/root length` 回 0。
- 邊界：C 仍從原 `path` 借用指標，不接收 Rust-owned string；grep execution、path filter、result parsing、JSON/transport、server lifecycle 與 handlers 仍由 C 負責。

### MCP search_code root/file args combiner opt-in

完成項目：

- Rust helper：`mcp::search_args_valid()`
- FFI：`cbm_rs_mcp_search_args_valid_v1`
- C opt-in：`CBM_USE_RUST_MCP_CODEC=1` 時 `src/mcp/mcp.c` 的 `validate_search_args()` 委派 Rust。
- 契約：`root_path` 必填且需通過 path validator，`file_pattern` 可省略；若提供也需通過同一 denylist。
- 邊界：只遷移 pure combiner；project root resolution、grep command construction、path filter regex、search process execution、result parsing、response wrapping、Content-Length transport、server lifecycle 與 handlers 仍由 C 負責。

### MCP detect_changes impacted label classifier opt-in

完成項目：

- Rust helper：`mcp::detect_changes_impacted_label()`
- FFI：`cbm_rs_mcp_detect_changes_impacted_label_v1`
- C opt-in：`CBM_USE_RUST_MCP_CODEC=1` 時 `src/mcp/mcp.c` 的 `detect_changes_impacted_label()` 委派 Rust。
- 契約：null、`File`、`Folder` 與 `Project` 排除；其餘 label（含空字串、大小寫不符與尾端空白）皆列入 impacted symbols。
- 邊界：只遷移 pure classifier；Store node query、node ownership/free、changed file parsing、impacted JSON item building、response wrapping、Content-Length transport、server lifecycle 與 handlers 仍由 C 負責。

### MCP detect_changes scope classifier opt-in

完成項目：

- Rust helper：`mcp::detect_changes_wants_symbols()`
- FFI：`cbm_rs_mcp_detect_changes_wants_symbols_v1`
- C opt-in：`CBM_USE_RUST_MCP_CODEC=1` 時 `src/mcp/mcp.c` 的 `detect_changes_wants_symbols()` 委派 Rust。
- 契約：null/default、`symbols` 與 `impact` 輸出 impacted symbols；空字串、`files`、大小寫不符、尾端空白與未知值只輸出 changed files。
- 邊界：只遷移 pure classifier；project/root resolution、base branch validation、git diff/status command、changed file parsing、Store symbol lookup、JSON result building、response wrapping、Content-Length transport、server lifecycle 與 handlers 仍由 C 負責。

### MCP search_code ranking scorer opt-in

完成項目：

- Rust helper：`mcp::search_code_score()`
- FFI：`cbm_rs_mcp_search_code_score_v1`
- C opt-in：`CBM_USE_RUST_MCP_CODEC=1` 時 `src/mcp/mcp.c` 的 `compute_search_score()` 委派 Rust。
- 契約：以 `in_degree` 為基底，`Function` / `Method` 加 10、`Route` 加 15、`vendored/` / `vendor/` / `node_modules/` 扣 50，`test` / `spec` / `_test.` path 扣 5。
- 邊界：只遷移 pure scorer；grep execution、graph node lookup、dedup/classification、sort comparator、result JSON、response wrapping、Content-Length transport、server lifecycle 與 handlers 仍由 C 負責。

### MCP BM25 MATCH builder opt-in

完成項目：

- Rust helper：`mcp::bm25_match_query()`
- FFI：`cbm_rs_mcp_bm25_build_match_v1`
- C opt-in：`CBM_USE_RUST_MCP_CODEC=1` 時 `src/mcp/mcp.c` 的 `bm25_build_match()` 委派 Rust。
- 契約：只保留 ASCII alnum/underscore token，以 ` OR ` 串接；短 buffer 停在上一個完整 token；null query/buffer/過小 buffer 回 0 且不寫入。

### MCP BM25 file_pattern LIKE builder opt-in

完成項目：

- Rust helper：`mcp::bm25_file_pattern_like()`
- FFI：`cbm_rs_mcp_bm25_file_pattern_like_v1`
- C opt-in：`CBM_USE_RUST_MCP_CODEC=1` 時 `bm25_file_pattern_like()` 委派 Rust。
- 契約：先沿用 Store `glob_to_like`；原始 pattern 不含 `*` / `?` 時加前後 `%` 做 contains match；null pattern 回 `SIZE_MAX`。
- 曾遇到 C block scope 格式問題，已用 `clang-format -i src/mcp/mcp.c` 修正。

### MCP sanitize_utf8_lossy lossy UTF-8 sanitizer opt-in

完成項目：

- Rust helper：`mcp::sanitize_utf8_lossy()`
- FFI：`cbm_rs_mcp_sanitize_utf8_lossy_v1`
- C opt-in：`CBM_USE_RUST_MCP_CODEC=1` 時 `sanitize_utf8_lossy()` 委派 Rust。
- 契約：對非 UTF-8 byte 進行 lossy 消毒，將其替換為 REPLACEMENT CHARACTER U+FFFD（`0xEF 0xBF 0xBD`）；回傳消毒後所需的 byte 長度，短 buffer 截斷但 NUL-terminate；null input 回 `SIZE_MAX`。
- 邊界：只遷移 pure UTF-8 validator/serializer；yyjson building/serialization、response/transport/server lifecycle 與 handlers 仍由 C 負責。

### MCP get_architecture aspects gate opt-in

完成項目：

- Rust helper：`mcp::architecture_aspect_wanted()`
- FFI：`cbm_rs_mcp_architecture_aspect_wanted_v1`
- C opt-in：`CBM_USE_RUST_MCP_CODEC=1` 時 `aspect_wanted()` 委派 Rust。
- 契約：缺少 aspects、invalid JSON（含尾端殘留）、root 非 object、aspects 非 array 時視為 wanted (true)；空 array 或無匹配為 false；"all" 或 exact name match 為 true；大小寫與空白敏感；非字串元素忽略。
- 邊界：只遷移 pure aspect filter；yyjson parsing/serialization、Store links/traversal、response/transport/server lifecycle 與 handlers 仍由 C 負責。


## 最近驗證結果

最近一輪（Pipeline route-node classifiers opt-in）完成後通過：

- `cargo fmt --all -- --check`
- `cargo test -p cbm-core pipeline::gitdiff --locked`：8 passed
- `cargo test -p cbm-core --locked`：220 passed
- `cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings`
- `make -f Makefile.cbm rust-ffi-test`
- default `build/c/test-runner pipeline`：217 passed
- `default build/c/test-runner pipeline`：217 passed
- `default build/c/test-runner pipeline`：217 passed
- `default build/c/test-runner pipeline`：217 passed
- `make -f Makefile.cbm rust-pipeline-gitdiff-hunks-optin-test`：217 passed
- `make -f Makefile.cbm rust-pipeline-module-dir-optin-test`：217 passed
- `make -f Makefile.cbm rust-pipeline-route-args-optin-test`：217 passed
- `make -f Makefile.cbm rust-pipeline-route-node-classifiers-optin-test`：217 passed
- `git diff --check`

本交接文件更新後已重跑 `git diff --check` 與行程檢查；先前曾在並行跑共享 `build/c` 的 target 時撞到 clean-c race，後續序列重跑已通過。

## 文件已同步

下列文件已記錄最近的 Rust opt-in 切片：

- `rust/CBM_FFI.md`
- `Rust-Refactor.md`
- `Tasks.md`

重點條目：

- `cbm_rs_pipeline_artifact_path_v1`
- `cbm_rs_pipeline_is_checked_exception_v1`
- `cbm_rs_mcp_bm25_build_match_v1`
- `cbm_rs_mcp_bm25_file_pattern_like_v1`
- `cbm_rs_mcp_search_code_score_v1`
- `cbm_rs_mcp_search_score_cmp_v1`
- `cbm_rs_mcp_search_top_dir_v1`
- `cbm_rs_mcp_detect_changes_status_path_offset_v1`
- `cbm_rs_mcp_search_line_match_span_v1`
- `cbm_rs_mcp_search_pick_resolved_index_v1`
- `cbm_rs_mcp_search_pick_tightest_index_v1`
- `cbm_rs_mcp_utf8_is_cont_v1`
- `cbm_rs_mcp_missing_project_error_v1`
- `cbm_rs_mcp_project_not_found_message_v1`
- `cbm_rs_mcp_project_list_error_v1`
- `cbm_rs_mcp_adr_sections_json_v1`
- `cbm_rs_mcp_strip_root_prefix_offset_v1`
- `cbm_rs_mcp_search_args_valid_v1`
- `cbm_rs_mcp_architecture_aspect_wanted_v1`
- `cbm_rs_mcp_sanitize_utf8_lossy_v1`
- `cbm_rs_mcp_detect_changes_wants_symbols_v1`
- `cbm_rs_mcp_detect_changes_impacted_label_v1`

## 建議下一步

優先沿用「小切片、可 opt-in、可驗證」方式。`artifact_path()`、infrascan filename classifier、githistory trackable-file classifier、gitdiff range/name-status/hunk parsers 與 module-directory classifier 已完成；下一步先做 completion audit，確認文件、gate 與原始碼一致，再評估下一個低風險 pure helper。完整 pass orchestration、tree-sitter/LSP、SQLite runtime 與 graph-buffer ownership 仍不可宣稱已遷移。

建議步驟：
1. 先讀 `AGENTS.md`、`Handoff.md`、`Rust-Refactor.md`、`Tasks.md`、`rust/CBM_FFI.md` 與目前 `git status --short`。
2. 先用 `rg` 驗證目標 helper 的 C contract、Rust helper、FFI、opt-in flag 與 test target 都真的存在。
3. 先做 Rust unit + `tests/test_rust_ffi.c` smoke，再跑 default C gate 與 single opt-in gate。
4. 文件只寫已驗證的 gate，不要把未跑的 target 寫成通過。
5. 更新 `rust/CBM_FFI.md`、`Rust-Refactor.md`、`Tasks.md` 與本交接文件。

## 常用驗證指令

```sh
cargo fmt --all -- --check
cargo test -p cbm-core pipeline::artifact --locked
cargo test -p cbm-core --locked
cargo clippy -p cbm-core --all-targets --all-features --locked -- -D warnings
make -f Makefile.cbm rust-ffi-test
make -f Makefile.cbm build/c/test-runner && build/c/test-runner artifact
make -f Makefile.cbm rust-pipeline-artifact-path-optin-test
git diff --check
```

注意：`rust-pipeline-artifact-path-optin-test`、pipeline parity targets 等會清 `build/c`，不要與其他 Make build target 並行。

## 注意事項

- 不要宣稱整體 Rust 重構完成；目前只是多個 opt-in 小切片完成。
- 不要把 production default 切到 Rust-backed；Phase 5 尚未達標。
- C fallback 仍需保留。
- 修改文件時不要把未驗證的 gate 寫成「通過」。
- 如果新增 C code，最後跑 `clang-format` 或 `make -f Makefile.cbm lint-format`。
- 若要收尾，務必檢查：

```sh
ps -axo pid,ppid,stat,command | rg '(^|[[:space:]])make( [^[:space:]]+)* -f Makefile\.cbm|cargo (test|clippy|build|fmt)|build/c/test-runner|test-rust-ffi|codebase-memory-mcp'
git status --short
```

## 2026-07-11：`str_intern` direct replacement handoff

- 實作已加入 `str-intern-only` feature、Rust direct `cbm_intern_*` exports、Makefile source exclusion，以及 direct all-optin 的三 feature 組合。
- C fallback 與一般 `CBM_USE_RUST_STR_INTERN=1` wrapper 保留；Rust pool 擁有回傳字串記憶體，C 不得釋放 borrowed pointers。
- `rust-str-intern-only-test` 已接入 `rust-ci` 與 `rust-ci-tests`，並已通過 default/direct foundation 各 `325 passed`。
- direct all-optin 已使用 `--features traces-only,pipeline-configures-only,str-intern-only`，wrapper/direct 各 `5861 passed`；兩個 production binary 均成功輸出 `codebase-memory-mcp dev`。
- 此切片狀態為「實作完成、gate 已驗證」；下一步應選擇下一個具備完整 ownership 與副作用覆蓋的 C compilation unit，不得把既有 helper opt-in 誤列為 source replacement。

## 2026-07-11：`dump_verify` direct replacement handoff

- `dump-verify-only` 已完成 source exclusion：Rust 直接提供兩個既有 public API，C `dump_verify.c` 只在 default/fallback 路徑存在。
- warning 行為由 `src/foundation/log.c` 的固定參數 bridge 保留；Rust 不直接呼叫 variadic logger。
- 專屬 gate default/direct foundation 各 `325 passed`；四 feature direct all-optin 與 wrapper all-optin 各 `5861 passed`，direct production binary 成功輸出 `codebase-memory-mcp dev`。
- `RUST_ALL_OPTIN_DIRECT_FLAGS` 已以 `filter-out` 移除一般 `CBM_USE_RUST_DUMP_VERIFY=1`，只使用 `CBM_USE_RUST_DUMP_VERIFY_ONLY=1`；目前 direct replacement 計數為 52，因為這是 wrapper flag 的 direct replacement，不是新增 wrapper 行為。
- 下一個首選切片為 `yaml-only`；先補 `strtod`/`isspace` parity 與 direct opaque-handle ABI，再排除 `src/foundation/yaml.c`。

## 2026-07-11：`yaml` direct replacement handoff

- `yaml-only` 已完成 source exclusion：Rust direct ABI 取代 `src/foundation/yaml.c`，root/tree 與 borrowed query strings 的 ownership contract 已文件化。
- `rust-yaml-only-test` default/direct foundation 各 `325 passed`，direct production binary 成功輸出 `codebase-memory-mcp dev`；Rust YAML unit 為 `4 passed`。
- wrapper/direct 五 feature all-optin 各 `5861 passed`；direct Cargo features 為 `traces-only,pipeline-configures-only,str-intern-only,dump-verify-only,yaml-only`，production source list 排除三個 foundation C units。
- 目前 direct replacement 計數為 52。下一個候選應優先評估 Store `search-pattern-heap-only` 或先完成 source-list assertion/tooling，不能把現有 Store helper wrappers 誤列為 source replacement。

### 最新交接：Store search pattern

`store-search-pattern-only` 已完成。direct ABI 為 `cbm_glob_to_like` 與
`cbm_extract_like_hints`，輸出由 C heap 配置、呼叫端負責 `free()`；
`src/store/search_pattern.c` 只在 wrapper/default build 保留，direct production
build 已排除。Store 聚焦測試 default/direct 各 216 passed；六切片全量 opt-in
wrapper/direct 各 5861 passed，兩個 production binary 均成功建置。
目前 direct replacement 計數為 52；下一個候選是 Store FTS/camel-split 的窄切片，
不可將整個 `store.c` 視為已由 Rust 取代。

### 最新交接：Store architecture helpers

`store-arch-helpers-only` 已完成。Rust 直接提供 `cbm_hop_to_risk`、
`cbm_risk_label`、`cbm_qn_to_package`、`cbm_qn_to_top_package` 與
`cbm_is_test_file_path`；QN 結果維持每執行緒 256-byte buffer，呼叫端不得
free。`src/store/arch_helpers.c` 只在 default/wrapper build 保留，direct
production build 已排除。default/direct 聚焦測試各 253 passed；七切片全量
opt-in runner 均成功，production binary 亦成功建置。
目前 direct replacement 計數為 52。下一個 FTS/camel-split UDF 仍需先設計
SQLite context/value/result 的明確 ABI，不能只以既有 test-only buffer wrapper
宣稱已完成 direct 替換。

### 最新交接：Store file extension

`store-file-ext-only` 已完成。Rust 直接提供 `cbm_store_file_ext`，無副檔名或
超長副檔名回傳 null，有效結果維持每執行緒 16-byte buffer；
`src/store/file_ext.c` 只在 default/wrapper build 保留，direct production 已排除。
default/direct 聚焦測試各 253 passed；八切片全量 opt-in runner、direct FFI 與
production binary 均成功。
目前 direct replacement 計數為 52。下一個可評估項目仍是 FTS UDF；在完成
SQLite context/value/result ABI 前，不應宣稱整個 Store 已由 Rust 取代。

### 最新交接：Store immutable URI

`store-immutable-uri-only` 已完成。Rust 直接提供
`cbm_store_build_immutable_uri(path, out, out_sz)`，保留 caller-owned buffer、
容量檢查與 NUL 結尾契約；`src/store/immutable_uri.c` 只在 default/wrapper
build 保留，direct production 已排除。default/direct 聚焦測試各 216 passed，
九切片全量 opt-in、direct FFI 與 production binary 均成功。
目前 direct replacement 計數為 52。下一步可評估 mmap size parser 或 arch path
normalizer；FTS UDF 仍需明確 SQLite callback ABI 後才能 direct 化。

### 最新交接：Store mmap resolver

`store-mmap-resolver-only` 已完成。Rust 直接提供
`cbm_store_resolve_mmap_size`，保留 `CBM_SQLITE_MMAP_SIZE` 解析契約與 mmap
integration 行為；`src/store/mmap_resolver.c` 只在 default/wrapper build 保留，
direct production 已排除。default/direct 聚焦測試各 157 passed，direct FFI、
production binary 與版本檢查成功。
目前 direct replacement 計數為 52；下一步需跑十切片全量 opt-in，之後再評估
arch path normalizer 或 FTS SQLite UDF ABI。

### 最新交接：Store architecture path scope

已新增 `store-arch-path-scope-only` direct 切片。Rust 匯出
`cbm_store_arch_path_prepare`，負責 architecture path 正規化及 `/%` LIKE pattern；
C 保留 SQL fragment 與 SQLite bind。專屬 default/direct gate 各 `268 passed`，
`make -f Makefile.cbm rust-all-optin-test` 的 11-slice wrapper/direct gate 均成功，
兩個 production binary 皆輸出 `codebase-memory-mcp dev`。direct production source list
排除 `src/store/arch_path_scope.c`；目前 direct replacement 計數為 52。

下一個 Store 候選為 FTS UDF；它需要真實 SQLite context/value/result ABI，應先建立最小
FFI 邊界與 focused regression，再考慮 direct 切換。

### 最新交接：Store FTS tokenizer

已新增 `store-fts-tokenizer-only` direct 切片。Rust 的
`cbm_store_sqlite_camel_split` 位於獨立 `fts_sqlite.rs`，以 `sqlite3_malloc` / `sqlite3_free`
移交結果 ownership，避免通用 FFI runner 拉入 SQLite imports。C 保留 SQLite function 註冊；
direct production source list 排除 `src/store/fts_tokenizer.c`。

default/direct 完整 suite 均通過；`make -f Makefile.cbm rust-all-optin-test` 的 12-slice
wrapper/direct gate 成功，兩個 production binary 均輸出 `codebase-memory-mcp dev`。
目前 direct replacement 計數為 52。

下一個 Store 候選應優先選擇不需要 SQLite context/value/result ABI 的純 helper；若需 SQLite
callback，必須維持專用 FFI object 隔離，並先驗證通用 `test-rust-ffi` link。

### 最新交接：Pipeline project name

已新增 `pipeline-project-name-only` direct 切片。Rust 匯出
`cbm_project_name_from_path`，使用 C `malloc` 配置回傳字串，C 呼叫端可維持 `free()`
ownership 契約。direct production source list 排除 `src/pipeline/project_name.c`；其他
FQN helper 與 pipeline orchestration 仍由 C 控制。

default/direct FQN suite 各 `77 passed`；`make -f Makefile.cbm rust-all-optin-test` 的
13-slice wrapper/direct gate 成功，兩個 production binary 均輸出
`codebase-memory-mcp dev`。目前 direct replacement 計數為 52。

下一個候選應優先選擇已有 Rust pure helper 且能明確處理 C allocation ownership 的小型
pipeline 或 foundation slice；避免將 allocator、OS probe 或大型 graph state 直接跨 FFI。

### 最新交接：Pipeline artifact path

已新增 `pipeline-artifact-path-only` direct 切片。Rust 的
`cbm_pipeline_artifact_path` 只寫入呼叫端 buffer，不配置記憶體也不做 I/O；C 保留
artifact export/import 的 SQLite、zstd、metadata 與檔案系統流程。direct production source
list 排除 `src/pipeline/artifact_path.c`。

default/direct artifact suite 各 `10 passed`；`make -f Makefile.cbm rust-all-optin-test` 的
14-slice wrapper/direct gate 成功，兩個 production binary 均輸出
`codebase-memory-mcp dev`。目前 direct replacement 計數為 52。

下一個候選應繼續以已有 Rust pure helper、caller-buffer 或明確 C allocation ABI 的小型切片為
優先；不可直接搬動 artifact 的 SQLite/zstd/I/O side effects。
### 最新交接：Pipeline test detection

已新增 `pipeline-test-detect-only` direct 切片。Rust 匯出 `cbm_is_test_path` 與
`cbm_is_test_func_name`，只做 byte-level predicate；C 保留 TESTS/TESTS_FILE edge
建立與 `test_to_prod_path` 的 allocation ownership 邊界。direct production source list
排除 `src/pipeline/test_detect.c`。

default/direct pipeline suite 各 `221 passed`；`make -f Makefile.cbm rust-all-optin-test`
的 15-slice wrapper/direct gate 成功，兩個 production binary 均輸出
`codebase-memory-mcp dev`。目前 direct replacement 計數為 52。

下一個候選應優先選擇已有 Rust pure helper 且無 graph state/I/O/SQLite ownership 的小型
pipeline slice；heap-returning path helper 需先明確維持 C allocation contract。

### 最新交接：Pipeline checked exception

已新增 `pipeline-checked-exception-only` direct 切片。Rust 匯出
`cbm_pipeline_is_checked_exception`，只做 null-safe byte-level predicate；C 保留
THROWS/RAISES edge resolution 與 graph mutation。direct production source list 排除
`src/pipeline/checked_exception.c`。

default/direct `edge_types_probe pipeline` 各 `273 passed`；
`make -f Makefile.cbm rust-all-optin-test` 的 16-slice wrapper/direct gate 各
`5861 passed`，兩個 production binary 均輸出 `codebase-memory-mcp dev`。目前 direct
replacement 計數為 52。

2026-07-15 已將此 targeted gate 改為隔離的 default/direct build 目錄：Rust unit
`cargo test -p cbm-core pipeline::exception --locked` 為 `1 passed`；default 與 direct
`edge_types_probe pipeline` 各為 `273 passed`，direct 另通過相同 ABI 的 FFI smoke、
production link 與 `--version`。展開 direct Make 變數時
`CHECKED_EXCEPTION_SRCS =`，且 production link line 不含
`src/pipeline/checked_exception.c`；這證明該 compilation unit 在 opt-in direct 路徑
確由 Rust 提供，而非只呼叫 Rust helper。

下一個候選應優先選擇已有 Rust pure helper、無 graph state/I/O/SQLite ownership，且能以
獨立 C fallback source 封裝的小型 pipeline 或 foundation predicate。

### 最新交接：CLI progress sink direct replacement（2026-07-13）

- `progress-sink-only` 已完成：Rust staticlib 直接匯出 `cbm_progress_sink_init`、`cbm_progress_sink_fini` 與 `cbm_progress_sink_fn`；`CBM_USE_RUST_PROGRESS_SINK_ONLY=1` 時 `Makefile.cbm` 排除 `src/cli/progress_sink.c`。
- Rust 只承接 structured progress line 的狀態機與格式化；C `cbm_log_set_sink()`、`FILE*`、stderr、logger global state 與 sink lifecycle ownership 仍保留。
- 已通過 targeted gate：default progress suite 2 passed、direct Rust FFI smoke、direct progress suite 2 passed、direct production link 與 `codebase-memory-mcp --version`。
- 下一步仍是按 compilation unit 逐步處理 CLI/foundation/pipeline/store 的完整 Rust-backed replacement；此切片不代表整體預設路徑已切換，也不代表可移除其他 C runtime。

### 最新交接：Foundation `str_util` direct replacement（2026-07-13）

- `str-util-only` 已完成：Rust staticlib 直接匯出 `str_util.h` 的 path/string helper；`CBM_USE_RUST_STR_UTIL_ONLY=1` 時排除 `src/foundation/str_util.c`。
- 所有 arena-owned output 與 split array 透過 C `cbm_arena_alloc` 配置；`path_ext` / `path_base` 保留 borrowed input/static pointer，Rust 不保存或釋放 C ownership。
- 已通過 default foundation 325 passed、direct FFI archive smoke、direct foundation 325 passed、direct production link 與 `codebase-memory-mcp --version`。
- 下一步仍應逐 compilation unit 擴大 Rust direct coverage；`CBMArena` allocator、OS compatibility、SQLite、pipeline pass、MCP handlers、tree-sitter/LSP 與預設切換仍不可宣稱已遷移。

## 歷史交接（2026-07-13 全組合 direct gate）

- `rust-all-optin-test` 的 wrapper/direct 兩個變體皆通過完整 `5869 passed` runner。
- wrapper/direct production binary 皆成功建置並輸出 `codebase-memory-mcp dev`。
- direct link line 已確認不含 `src/foundation/str_util.c`、`src/cli/progress_sink.c`；對應 Rust direct ABI 為 `str-util-only` 與 `progress-sink-only`。
- 下一階段不可將此結果描述成全專案 Rust 化；應繼續處理 store runtime、Cypher parser/evaluator、MCP lifecycle、pipeline orchestration、Tree-sitter/LSP、watcher/UI 與平台相容層。

## 歷史交接（2026-07-13 hash table direct replacement）

- `rust/cbm-core/src/ffi_hash_table_only.rs` 已提供完整 `CBMHashTable` C ABI；key bytes 用於 lookup，canonical key pointer 保持 borrowed semantics，callback 使用 snapshot。
- `CBM_USE_RUST_HASH_TABLE_ONLY=1` 已從 `FOUNDATION_SRCS` 排除 `src/foundation/hash_table.c`，並加入 all-optin direct matrix。
- `make -f Makefile.cbm rust-hash-table-only-test`：default/direct foundation 各 `325 passed`，direct production binary 成功。
- 目前仍不是全專案 Rust 化；下一個切片必須以完整 C ABI、ownership、平台與回歸證據為準，不可只以 helper wrapper 宣稱完成。

## 2026-07-13 hash table 全矩陣結果

hash table direct replacement 已通過 all-optin wrapper/direct 兩條路徑，各自 `5869 passed`；direct production link 成功，且 link command 未包含 `src/foundation/hash_table.c`。目前這只是 opt-in direct slice，預設建置仍保留 C fallback，後續仍須逐模組完成替換與預設路徑切換。

## 2026-07-13 diagnostics formatter direct slice

`diagnostics-format-only` 已完成。Rust FFI 僅輸出既有 C ABI 所需的 env/path/JSON/NDJSON 結果；C 仍擁有 atomic stats、thread、mimalloc、FD 與檔案生命週期。focused default/direct gate 各 `325 passed`，direct FFI smoke 與 production binary/version 成功。此 slice 已加入 all-optin direct flags，但尚未把 diagnostics 整個 runtime 模組移除 C。

## 2026-07-13 store language map handoff

`CBM_USE_RUST_STORE_LANGUAGE_MAP_ONLY=1` 已將 `src/store/store.c` 的副檔名語言分類決策切換到 Rust。Rust ABI 回傳 `ext_lang_table` 的穩定索引，C 依索引回傳既有靜態表中的 borrowed `lang` 指標，因此沒有 Rust 配置記憶體跨 FFI 釋放問題；未知副檔名仍回傳 `NULL`。focused `store_arch mcp` default/direct 各為 `189 passed`，all-optin wrapper/direct 各為 `5869 passed`。SQLite store runtime 與其他 store 邏輯仍是 C。

## 2026-07-13 pipeline module-directory handoff

`CBM_USE_RUST_PIPELINE_MODULE_DIR_ONLY=1` 已切換 `src/pipeline/pass_parallel.c` 的 `pp_module_is_dir` 到 Rust。這只是 parallel pass 的 direct slice；`internal/cbm/helpers.c` 與 `pass_lsp_cross.c` 的同語意判定尚未移除，避免破壞 Java/Go module QN 一致性。focused default/direct pipeline 各為 `221 passed`。

all-optin 矩陣在加入 `CBM_USE_RUST_PIPELINE_MODULE_DIR_ONLY=1` 後仍成功完成，direct production binary 可正常連結並回報 `codebase-memory-mcp dev`。

## 2026-07-13 platform path handoff

`CBM_USE_RUST_PLATFORM_PATH_ONLY=1` 已將 Windows 與非 Windows 的 `cbm_normalize_path_sep` 轉為 Rust 原地修改 ABI；C 端不再編譯 separator loop 與 drive canonicalization。其他 platform I/O、environment 與 directory runtime 仍是 C。focused default/direct foundation 各為 `325 passed`。

加入 `CBM_USE_RUST_PLATFORM_PATH_ONLY=1` 後 all-optin wrapper/direct runner 與兩個 production link 均成功。

## 2026-07-13 compat thread handoff

`CBM_USE_RUST_COMPAT_THREAD_ONLY=1` 已將 `compat_thread.c` 的有效 stack size 計算與 aligned allocation precheck 切換到既有 Rust ABI；實際 thread/mutex/allocator 作業與記憶體釋放仍由 C 負責。focused default/direct foundation 各為 `325 passed`。

加入 `CBM_USE_RUST_COMPAT_THREAD_ONLY=1` 後 all-optin wrapper/direct runner 與兩個 production link 均成功。

## 2026-07-13 MCP codec handoff

`CBM_USE_RUST_MCP_CODEC_ONLY=1` 已將 `mcp.c` 內所有既有 Rust-backed codec branches 切成 direct-only，包含 JSON-RPC、tools metadata/list/call、content-length、URI、argument validation、search/result formatter 與錯誤訊息。MCP handler、yyjson 與 runtime ownership 未切換。focused default/direct MCP 各為 `136 passed`。

加入 `CBM_USE_RUST_MCP_CODEC_ONLY=1` 後 all-optin wrapper/direct runner 與兩個 production link 均成功。

## 2026-07-13 Cypher classifier handoff

六組 Cypher classifier only gates 已完成：single/two-char lexer、aggregate/string/scalar/multiarg function index。focused default/direct `cypher` suite 各為 `144 passed`；parser/evaluator/SQLite execution 尚未移除 C。

加入六組 Cypher classifier only flags 後 all-optin wrapper/direct runner 與兩個 production link 均成功。

## 2026-07-13 compat regex handoff

`CBM_USE_RUST_COMPAT_REGEX_ONLY=1` 已切換 known flags、match capacity、match status 到 Rust；regex compile/execute runtime 仍由 C 管理。focused default/direct foundation 各為 `325 passed`。

### 2026-07-13：compat_regex direct gate

`compat_regex.c` 已支援 `CBM_USE_RUST_COMPAT_REGEX_ONLY`；regex 狀態、上限與狀態 helper 在 direct 路徑不再編譯 C fallback，regex engine 與生命週期仍是 C。聚焦 default/direct foundation、FFI/link smoke，以及完整 all-optin wrapper/direct matrix 均成功；direct 正式版回報 `codebase-memory-mcp dev`。

### 2026-07-13：log env parse direct gate

`log.c` 已支援 `CBM_USE_RUST_LOG_ENV_PARSE_ONLY`；direct 路徑不再編譯 level/format C parser，`getenv`、全域 logger、sink、輸出與設定仍由 C 管理。default/direct foundation 各 325 passed，direct FFI smoke 與正式版連結成功，版本為 `codebase-memory-mcp dev`。

`rust-all-optin-test` 已在 log env parse `_ONLY` gate 納入後成功完成 wrapper/direct runner 與正式版連結；direct 命令列包含 `CBM_USE_RUST_LOG_ENV_PARSE_ONLY=1`，版本回報 `codebase-memory-mcp dev`。

### 2026-07-13：profile env direct gate

`profile.c` 已支援 `CBM_USE_RUST_PROFILE_ENV_ONLY`；direct 路徑不再編譯 C env 判斷，`getenv`、profiling 狀態與時間/記錄邏輯仍由 C 管理。default/direct foundation 各 325 passed，direct FFI smoke、正式版連結及版本 `codebase-memory-mcp dev` 成功。

`rust-all-optin-test` 已在 profile env `_ONLY` gate 納入後成功完成 wrapper/direct runner 與正式版連結；direct 命令列包含 `CBM_USE_RUST_PROFILE_ENV_ONLY=1`，版本回報 `codebase-memory-mcp dev`。

### 2026-07-13：platform env dirs direct gate

`platform.c` 已支援 `CBM_USE_RUST_PLATFORM_ENV_DIRS_ONLY`；direct 路徑不再編譯對應 C env-directory helper，平台系統呼叫、環境讀取與 C buffer/生命週期仍保留。default/direct foundation 各 325 passed，direct FFI smoke、正式版連結及版本 `codebase-memory-mcp dev` 成功。

`rust-all-optin-test` 已在 platform env dirs `_ONLY` gate 納入後成功完成 wrapper/direct runner 與正式版連結；direct 命令列包含 `CBM_USE_RUST_PLATFORM_ENV_DIRS_ONLY=1`，版本回報 `codebase-memory-mcp dev`。

### 2026-07-13：pipeline route canonicalizer direct gate

`pass_route_nodes.c` 已支援 `CBM_USE_RUST_PIPELINE_ROUTE_CANON_ONLY`；direct 路徑不再編譯 C parameter scanner，Rust 直接產生 canonical path，Route node/edge graph orchestration 仍由 C 管理。default/direct focused `route_canon pipeline` 各 232 passed；direct FFI smoke、正式版連結及版本 `codebase-memory-mcp dev` 成功。

`rust-all-optin-test` 已在 pipeline route canonicalizer `_ONLY` gate 納入後成功完成 wrapper/direct runner 與正式版連結；direct 命令列包含 `CBM_USE_RUST_PIPELINE_ROUTE_CANON_ONLY=1`，版本回報 `codebase-memory-mcp dev`。

### 2026-07-13：pipeline git history direct gate

`pass_githistory.c` 已支援 `CBM_USE_RUST_PIPELINE_GITHISTORY_ONLY`；direct 路徑不再編譯 C trackable-file filter，libgit2/git log、coupling 與 graph edge 仍由 C 管理。default/direct pipeline focused 各 221 passed；direct FFI smoke、正式版連結及版本 `codebase-memory-mcp dev` 成功。

`rust-all-optin-test` 已在 pipeline git history `_ONLY` gate 納入後成功完成 wrapper/direct runner 與正式版連結；direct 命令列包含 `CBM_USE_RUST_PIPELINE_GITHISTORY_ONLY=1`，版本回報 `codebase-memory-mcp dev`。

### 2026-07-13：pipeline git diff parsers direct gates

`pass_gitdiff.c` 的 range、name-status、hunks parser 已支援 `_ONLY` direct gates；direct 路徑不再編譯 C parser/static helper，C 仍管理 caller、輸出結構與後續 pipeline 行為。default/direct pipeline focused 各 221 passed；direct FFI smoke、正式版連結及版本 `codebase-memory-mcp dev` 成功。

`rust-all-optin-test` 已在 pipeline gitdiff 三個 `_ONLY` gates 納入後成功完成 wrapper/direct runner 與正式版連結；direct 命令列包含三個 gitdiff `_ONLY` flags，版本回報 `codebase-memory-mcp dev`。

### 2026-07-13：pipeline infrascan direct gate

`pass_infrascan.c` 已支援 `CBM_USE_RUST_PIPELINE_INFRASCAN_ONLY`；direct 路徑不再編譯 filename classifiers 與 `to_lower` helper，secret/manifest parser、解析與 graph ownership 仍由 C 管理。default/direct pipeline focused 各 221 passed；direct FFI smoke、正式版連結及版本 `codebase-memory-mcp dev` 成功。

### 測試偵測 `_ONLY` gate 修正

`pass_tests.c` 已接上 `CBM_USE_RUST_PIPELINE_TEST_DETECT_ONLY`；`Makefile.cbm` 同步補上 C 巨集與 `pipeline-test-detect-only` staticlib 選擇。`rust-pipeline-test-detect-optin-test` 及 `rust-pipeline-test-detect-only-test` 均為 `221 passed`。後續切片仍須維持 default C wrapper、direct Rust link 及全量 opt-in 矩陣三層證據。

### Linux cgroup direct gate

`system_info.c` 的 Linux cgroup CPU/memory 解析已支援 `CBM_USE_RUST_PLATFORM_CGROUP_ONLY`；direct 模式不編譯 C 檔案解析 helper。`rust-platform-cgroup-only-test` 已通過 default/direct foundation 各 `325 passed`、FFI、正式版連結及版本輸出。

### 全量矩陣回歸

cgroup direct slice 接入後，`rust-all-optin-test` 已重新通過；wrapper/direct runner、production link 與版本 smoke 均為成功。direct flags 包含 `CBM_USE_RUST_PLATFORM_CGROUP_ONLY=1`。

### Registry test-QN direct gate

`registry.c` 僅 test-QN classifier 已切到 `_ONLY` Rust direct 路徑；resolver、cache 與索引資料結構尚未宣稱已 Rust 化。`rust-pipeline-registry-test-qn-only-test` 已通過 default `53 passed` 與 direct/FFI/production gates。

### Registry classifier 全量回歸

test-QN direct slice 接入後，`rust-all-optin-test` 已重新通過；wrapper/direct runner 與 production version 均成功。完整 registry resolver/cache 仍列為後續主工作。

## 歷史交接（2026-07-15 CLI 版本比較）

已新增 CLI semver comparator 的 Rust slice：

- rust/cbm-core/src/cli/version.rs 實作 cbm_compare_versions() 的 byte-level pure logic。
- cbm_rs_cli_compare_versions_v1() 提供 wrapper ABI；cli-version-only feature
  提供 cbm_cli_compare_versions_v1() direct ABI。
- src/cli/cli.c 在兩個 opt-in flag 下停用原本的 C parser，預設路徑仍保留 C。
- tests/test_rust_ffi.c、Makefile.cbm 與四份重構文件已接線。

本切片尚未執行驗證 gate。下一步應依序執行 Rust unit、rust-ffi-test、
rust-cli-version-optin-test、rust-cli-version-only-test、CLI default/wrapper/direct
runner 與 production --version；若有失敗，先修正 ABI/contract，再更新本節驗證證據。
CLI install/update/uninstall/config 及其餘 server/runtime C 邊界仍未完成 Rust 取代。


## 歷史交接（2026-07-15 discover 字串 helper）

本輪完成 discover 的四個純字串 helper Rust slice：str_in_list、ends_with、str_contains、
ascii_ieq。C fallback 已由 `src/discover/discover.c` 拆至
`src/discover/discover_string_helpers.c`；一般 opt-in 由該 C unit 呼叫 Rust stable v1 ABI，
`CBM_USE_RUST_DISCOVER_STRING_HELPERS_ONLY=1` 時
`DISCOVER_STRING_HELPERS_SRCS` 為空，由 feature-gated Rust direct ABI 提供同名 C symbol。

2026-07-15 驗證完成：Rust unit `1 passed`；隔離 default、wrapper、direct 的
`test-rust-ffi` 均已執行成功；三種組態的 `discover` suite 各 `85 passed`；wrapper/direct
正式版均完成連結並輸出 `codebase-memory-mcp dev`。direct production link line 不含
`src/discover/discover_string_helpers.c`，確認這四個 helper 由 Rust staticlib 實際取代。
discover 遞迴掃描、檔案 I/O、Git 設定與語言偵測仍在 C，不可宣稱 Discover 子系統已 Rust 化。


## 歷史交接（2026-07-15 watcher polling interval）

本輪完成 watcher adaptive poll interval 的 Rust direct replacement。stable ABI 為
`cbm_rs_watcher_poll_interval_ms_v1`；一般 opt-in 仍由 C wrapper 呼叫；
`CBM_USE_RUST_WATCHER_POLL_INTERVAL_ONLY=1` 使用 `watcher-poll-interval-only` feature
提供同名 direct ABI。C fallback 已自 `src/watcher/watcher.c` 拆至
`src/watcher/poll_interval.c`，並由 source selector 在 direct 路徑排除；Git 狀態、時間、
thread、輪詢迴圈與 callback 仍在 C。

2026-07-15 驗證完成：Rust watcher unit `1 passed`；隔離 default/direct full runner 各
`5868 passed`；同一組 build 目錄的 default/direct `test-rust-ffi` 均已執行成功；wrapper
FFI、watcher suite `53 passed` 與 production binary 亦通過；direct release binary 輸出
`codebase-memory-mcp dev`。direct link line 不含
`src/watcher/poll_interval.c`，且 Make 展開為 `WATCHER_POLL_INTERVAL_SRCS =`，因此此切片
是實際替換 C compilation unit，而非只讓 C wrapper 呼叫 Rust helper。不要把它解讀為整個
watcher 子系統已完成 Rust 化。


## 歷史交接（2026-07-15 Pipeline SvelteKit file kind）

本輪完成 `sveltekit_file_kind` 的窄範圍 Rust direct replacement。C fallback 已自
`src/pipeline/pass_route_nodes.c` 拆至 `src/pipeline/sveltekit_file_kind.c`／`.h`，
`pass_route_nodes.c` 僅經由 `cbm_pipeline_sveltekit_file_kind()` 公開 bridge 呼叫。stable ABI
為 `cbm_rs_pipeline_sveltekit_file_kind_v1`；一般 opt-in 的 C wrapper 委派該 ABI，預設仍編譯
C fallback。`CBM_USE_RUST_PIPELINE_SVELTEKIT_FILE_KIND_ONLY=1` 時，
`PIPELINE_SVELTEKIT_FILE_KIND_SRCS =`，由 Rust 匯出同名 direct ABI，並排除該 C source。

契約已凍結：NULL 為 0；path 必須包含精確 `/routes/` segment，最後一個 `/` 後的 basename 只接受
`+server.ts`／`.js` 回 1、`+page.server.ts`／`.js` 回 2、`+layout.server.ts`／`.js` 回 3，其餘
回 0。分類採 byte-level 精確比對；Rust 不配置、不保存或持有 C 輸入指標。SvelteKit route path
組合、File/Route graph traversal、Route/HANDLES mutation 與 ownership 仍在 C。

2026-07-15 已驗證：原先 direct runner 因大型 C 檔仍呼叫 local static helper 而無法編譯的缺口已
重現並修正；聚焦 Rust unit `1 passed`、完整 Rust suite `307 passed`、Clippy、Rust fmt 與本切片
C 格式均成功；隔離 default/wrapper/direct `test-rust-ffi` 均已實際執行成功；三種路徑的 pipeline
suite 各 `227 passed`；wrapper/direct production binary 均可輸出 `codebase-memory-mcp dev`。direct
Make 展開為空且 production link line 不含 `src/pipeline/sveltekit_file_kind.c`。這些是 targeted
gates，不代表 pipeline route-node 或完整 CI 已完成 Rust 化。

2026-07-16 更正：上述 307 passed、Clippy 與 direct 227 passed 是補齊兩個 basename case、並將
source-negative 檢查改成 make -Bn 前的歷史結果。本 revision 已重新完成 default/wrapper gate，
但 direct gate 中斷；目前只能確認 selector 為空與 forced dry-run link 不含 C fallback。完整
current-revision 證據與重跑命令以 docs/rust-refactor-current-handoff.md 為準。


## 歷史交接（2026-07-15 Pipeline SvelteKit server method）

本輪完成 SvelteKit server method mapping 的窄範圍 Rust direct replacement。C fallback 已自
`src/pipeline/pass_route_nodes.c` 拆至 `src/pipeline/sveltekit_server_method.c`，
`pass_route_nodes.c` 僅經由 `cbm_pipeline_sveltekit_server_method()` 呼叫。stable ABI 為
`cbm_rs_pipeline_sveltekit_server_method_v1`；一般 opt-in 的 C wrapper 委派該 ABI；預設仍編譯
C fallback。`CBM_USE_RUST_PIPELINE_SVELTEKIT_SERVER_METHOD_ONLY=1` 時，
`PIPELINE_SVELTEKIT_SERVER_METHOD_SRCS =`，由 Rust 匯出同名 direct ABI，並排除該 C source。

2026-07-15 已驗證：Rust unit `1 passed`；隔離 default、wrapper、direct `test-rust-ffi`
均成功；三種路徑的 pipeline suite 均為 `222 passed`；wrapper/direct production binary
均可輸出 `codebase-memory-mcp dev`。direct Make 展開與 production link line 均不含
`src/pipeline/sveltekit_server_method.c`。route synthesis、graph traversal 與 graph mutation
仍在 C；不要把此切片解讀為 pipeline route-node 或整個 pipeline 已 Rust 化。


## 歷史交接（2026-07-15 CLI hook token）

本輪完成 hook_augment.c 的 ha_extract_token Rust slice。stable ABI 為 cbm_rs_cli_hook_extract_token_v1；一般 opt-in 由 C static helper wrapper 呼叫；CBM_USE_RUST_CLI_HOOK_TOKEN_ONLY=1 使用 direct ABI。hook deadline、stdin、JSON、MCP query、輸出與 process lifecycle 仍在 C。

尚未執行驗證。下一步依序執行 make -f Makefile.cbm rust-cli-hook-token-optin-test、make -f Makefile.cbm rust-cli-hook-token-only-test，通過後更新四份文件並選下一個純解析器。不要把這組 gate 解讀為整個 CLI 或 hook runtime 已完成 Rust 化。


## 歷史交接（2026-07-15 Git path absolute）

本輪完成 `path_is_absolute` 的窄範圍 Rust direct replacement。C fallback 已自
`src/git/git_context.c` 拆至 `src/git/path_absolute.c`／`.h`，`git_context.c` 僅經由
`cbm_git_path_is_absolute()` 公開 bridge 呼叫。stable ABI 為
`cbm_rs_git_path_is_absolute_v1`；一般 opt-in 的 C wrapper 委派該 ABI，預設仍編譯 C fallback。
`CBM_USE_RUST_GIT_PATH_ABSOLUTE_ONLY=1` 時，`GIT_PATH_ABSOLUTE_SRCS =`，由 Rust 匯出同名
direct ABI，並排除該 C source。

契約已凍結：NULL 與空字串為 false；首 byte 為 `/`（含 `//server`）為 true；Windows 另只接受
ASCII 英文字母後接 `:`（含 `C:` 與 `C:project`），數字、非 ASCII drive byte 與 `\\server`
皆為 false。Rust 不配置、不保存或持有 C 輸入指標；git command、repository state、root
derivation、字串配置與 ownership 仍在 C。

2026-07-15 已驗證：聚焦 Rust unit `4 passed`、完整 Rust suite `307 passed`、Clippy、Rust fmt
與本切片 C 格式均成功；隔離 default/wrapper/direct `test-rust-ffi` 均已實際執行成功；三種路徑的
pipeline suite 各 `226 passed`；wrapper/direct production binary 均可輸出
`codebase-memory-mcp dev`。direct Make 展開為空且 production link line 不含
`src/git/path_absolute.c`。這些是 targeted gates，不代表完整 Git runtime 或完整 CI 已完成 Rust 化。


## 歷史交接（2026-07-15 Git JSON escaped length）

本輪完成 `json_escaped_len` 的窄範圍 Rust direct replacement。C fallback 已自
`src/git/git_context.c` 拆至 `src/git/json_escaped_len.c`／`.h`，`git_context.c` 僅經由
`cbm_git_json_escaped_len()` 呼叫。stable ABI 為 `cbm_rs_git_json_escaped_len_v1`；一般 opt-in
的 C wrapper 委派該 ABI，預設仍編譯 C fallback。`CBM_USE_RUST_GIT_JSON_ESCAPED_LEN_ONLY=1`
時，`GIT_JSON_ESCAPED_LEN_SRCS =`，由 Rust 匯出同名 direct ABI，並排除該 C source。

契約維持：NULL 為 0；在第一個 NUL 前，引號、反斜線、LF、CR、TAB 各計 2，其餘控制 byte
計 6，其餘 byte 計 1。結果不含結尾 NUL；最大成功值為 `INT_MAX - 1`，若連同 NUL 的 buffer
size 無法以 `int` 表示即回傳 -1；`json_append_string()` 會在配置前拒絕負值。Git metadata、
JSON append、buffer ownership 與 process lifecycle 仍在 C。

2026-07-15 已驗證：Rust unit `4 passed`；隔離 default/wrapper/direct `test-rust-ffi` 均已實際
執行成功；三種路徑的 pipeline suite 各 `224 passed`；wrapper/direct production binary 均可輸出
`codebase-memory-mcp dev`。direct Make 展開與 production link line 均不含
`src/git/json_escaped_len.c`。不要把此切片解讀為 Git context 或整個 Git runtime 已 Rust 化。


## 歷史交接（2026-07-15 Git trim newlines）

本輪完成 trim_newlines 的窄範圍 Rust direct replacement。C fallback 已自
`src/git/git_context.c` 拆至 `src/git/trim_newlines.c`／`.h`，`git_context.c` 僅經由
`cbm_git_trim_newlines()` 呼叫。stable ABI 為 `cbm_rs_git_trim_newlines_v1`；一般 opt-in 的
C wrapper 委派該 ABI；預設仍編譯 C fallback。`CBM_USE_RUST_GIT_TRIM_NEWLINES_ONLY=1` 時，
`GIT_TRIM_NEWLINES_SRCS =`，由 Rust 匯出同名 direct ABI，並排除該 C source。

2026-07-15 已驗證：Rust unit `1 passed`；隔離 default、wrapper、direct `test-rust-ffi`
均已實際執行成功；三種路徑的 pipeline suite 均為 `223 passed`；wrapper/direct production
binary 均可輸出 `codebase-memory-mcp dev`。direct Make 展開與 production link line 均不含
`src/git/trim_newlines.c`。Git command、capture、buffer ownership 與 process lifecycle 仍在 C；
不要把此切片解讀為 Git context 或整個 Git runtime 已 Rust 化。


## 歷史交接（2026-07-15 Log path query）

本輪完成 log HTTP path copy 的窄範圍 Rust direct replacement。C fallback 已自
`src/foundation/log.c` 拆至 `src/foundation/log_path.c`／`.h`，`log.c` 只經
`cbm_log_copy_path_without_query()` 公開 bridge 呼叫。stable ABI 為
`cbm_rs_log_copy_path_without_query_v1`；一般 opt-in 由該 C unit 委派 Rust，預設仍編譯 C
fallback。`CBM_USE_RUST_LOG_COPY_PATH_ONLY=1` 時 `LOG_COPY_PATH_SRCS =`，由
`log-copy-path-only` feature 匯出同名 direct ABI，並排除該 C source。

契約維持：在第一個 NUL、`?` 或 `#` 前截斷，最多寫入 `outsz - 1` bytes 並 NUL 結尾；
`path == NULL` 時寫入空字串；`out == NULL` 或 `outsz == 0` 時不寫入。`out` 若非 NULL
必須至少有 `outsz` bytes，且不得與 `path` 重疊。logger state、stream、formatting 與輸出
生命週期仍在 C。

2026-07-15 已驗證：Rust unit `1 passed`；隔離 default/wrapper/direct `test-rust-ffi` 均已
實際執行成功；三種路徑的 `log` suite 各 `16 passed`；wrapper/direct production link 與
`--version` 均成功；direct Make 展開與 production link line 均不含
`src/foundation/log_path.c`。此為單一 bounded buffer helper，不代表 logger、foundation
runtime 或完整 CI 已 Rust 化。


## 歷史交接（2026-07-15 Pipeline JSON property）

本輪完成 pass_route_nodes.c 的 extract_json_prop Rust slice。stable ABI 為 cbm_rs_pipeline_extract_json_prop_v1；一般 opt-in 由 C static helper wrapper 呼叫；CBM_USE_RUST_PIPELINE_JSON_PROP_ONLY=1 使用 direct ABI。graph traversal、route synthesis 與 JSON ownership 仍在 C。

尚未執行驗證。下一步依序執行 make -f Makefile.cbm rust-pipeline-json-prop-optin-test、make -f Makefile.cbm rust-pipeline-json-prop-only-test，通過後更新四份文件並選下一個純 parser。不要把這組 gate 解讀為 pipeline route-node 或整個 pipeline 已完成 Rust 化。


## 歷史交接（2026-07-15 Pipeline URL path）

本輪完成 pass_route_nodes.c 的 url_path Rust direct replacement。C fallback 已拆至
src/pipeline/url_path.c／.h，route-node pass 僅經 cbm_pipeline_url_path() 公開 bridge 呼叫。
stable ABI 為 cbm_rs_pipeline_url_path_v1；一般 opt-in 由該 bridge 委派 Rust；
CBM_USE_RUST_PIPELINE_URL_PATH_ONLY=1 時 source selector 排除 url_path.c，改由
pipeline-url-path-only 匯出同名 direct ABI。route graph、HTTP matching 與 graph mutation 仍在 C。

契約維持原始 raw scan，不是 RFC URL parser：nullable NUL 字串先尋找第一個 ://，再尋找其後
第一個 /；無 scheme 時回傳原 input 指標，有 path 時回傳 input 內部指標，僅無後續 slash 時
回傳靜態 "/"。Rust 不配置或保存 input，呼叫端不得釋放回傳指標。

2026-07-15 已驗證：cargo test -p cbm-core --locked 為 307 passed，clippy 與 Rust fmt
通過；隔離 default/wrapper/direct FFI smoke 均實際執行成功，三條路徑的 pipeline suite
各 225 passed；wrapper/direct production link 與 --version 成功；direct selector 為空，
production link line 與 source-negative gate 均不含 src/pipeline/url_path.c。此為單一
bounded helper，不代表 pipeline route-node、整個 pipeline 或完整 CI 已 Rust 化。


## 歷史交接（2026-07-15 Discover trailing separator）

本輪只完成 discover.c 的 has_trailing_sep Rust helper、stable ABI 與初步 macro opt-in 接線；
尚未完成真正的 C source replacement。C static helper 仍位於 discover.c，direct 只是改導向 Rust
symbol，discover.c 仍會編譯；沒有獨立 source selector 或 source-negative gate。stable ABI 為
cbm_rs_discover_has_trailing_sep_v1，path_join、Git path expansion、I/O 與遞迴掃描仍在 C。

既有 C fallback 對 NULL 使用 strlen，沒有可公開依賴的 NULL 行為；Rust ABI 的 NULL=false 只是
防禦性行為，不得稱為既有 parity。現有 target 只建置 FFI binary，wrapper 也沒有 production
version smoke。下一步應先拆出獨立 C fallback／公開 bridge、凍結 NULL 契約、補 selector 與
make -Bn 證據，再執行實際 default/wrapper/direct gate；不要把這組接線解讀為 Discover
或整個 discovery runtime 已完成 Rust 化。


## 歷史交接（2026-07-15 Discover path join）

本輪完成 discover.c 的 path_join Rust slice。stable ABI 為 cbm_rs_discover_path_join_v1；一般 opt-in 由 C static helper wrapper 呼叫；CBM_USE_RUST_DISCOVER_PATH_JOIN_ONLY=1 使用 direct ABI。Git path expansion、環境讀取、I/O 與 discovery recursion 仍在 C。

尚未執行驗證。下一步依序執行 make -f Makefile.cbm rust-discover-path-join-optin-test、make -f Makefile.cbm rust-discover-path-join-only-test，通過後更新四份文件並選下一個純 parser。不要把這組 gate 解讀為 discover 或整個 discovery runtime 已完成 Rust 化。


### SimHash MinHash Jaccard slice

已新增 Rust `simhash::jaccard` 與 `cbm_rs_simhash_jaccard_v1` stable FFI。`src/simhash/minhash.c` 在 `CBM_USE_RUST_SIMHASH_JACCARD=1` 時只替換 `cbm_minhash_jaccard`，未改動 AST token walk、xxHash、fingerprint 產生與 LSH index。`rust-simhash-jaccard-optin-test` 現使用分離 build 目錄，依序執行 default C `simhash` suite、Rust opt-in FFI smoke 與 opt-in `simhash` suite；2026-07-15 實測 default 與 opt-in 各 24 項通過，Rust `simhash` unit 4 項通過。此結果不代表 SimHash 子系統已完成 Rust 化，完整 lint/CI 仍待執行。

MinHash slice 的 stable ABI 現在包含 Jaccard、hex encode 與 hex decode；上述 gate 已驗證既有 suite 的 default/opt-in 路徑，跨平台 `strtoul` 邊界仍應由跨平台 CI 持續覆蓋。


### Pipeline complexity JSON slice

`json_get_int`/`json_get_bool` 已可由 Rust stable FFI 承接，C 端只在 opt-in 時切換。注意此切片依賴 fixed 64-byte C pattern buffer 的行為，後續驗證需覆蓋正常 key、缺失 key、空白、null 與 oversized key；目前未執行建置或測試。


### VMem page-round slice

`src/foundation/vmem.c` 未納入產品 source list，因此 page-round Rust 程式碼只是非產品 FFI parity fixture，不能計入 C→Rust runtime replacement。`rust-vmem-page-round-optin-test` 只執行 FFI parity smoke；2026-07-15 的 Rust `vmem` unit 2 項與 smoke 已通過，但沒有 default/opt-in 產品 runtime 證據。

### Makefile opt-in gate 執行修正（2026-07-15）

六個原先只編譯的 target 現均會清除隔離 build 目錄、執行 FFI smoke 與對應 runtime suite：`rust-pipeline-test-node-is-test-optin-test`、`rust-ast-profile-classifiers-optin-test`、`rust-pipeline-usages-json-optin-test`、`rust-semantic-token-classifiers-optin-test` 均跑 `pipeline`；`rust-ui-layout-helpers-optin-test` 跑 `ui`；`rust-ui-httpd-helpers-optin-test` 跑 `httpd`。2026-07-15 實測四個 pipeline gate 各 221 項、UI layout 15 項、HTTPD 32 項皆通過，且六項 FFI smoke 均通過。這是 gate 執行修正，不表示任何子系統已完成 Rust 遷移。


### Pipeline pkgmap text slice

已由 2026-07-19 的第 38 項 true-source 取代；權威契約與 default/wrapper/direct **235 passed**
證據見本檔頂端及 `rust/CBM_FFI.md` 的 pkgmap true-source 段落。

Pkgmap text slice 現在包含 bounded scan 與兩個 path string helper。Rust 不回傳借用 pointer；C 端仍配置/釋放結果。需驗證 oversized path、短輸出 buffer、null input 與 Windows separator 契約。


### Pipeline configlink helper slice

Rust stable ABI 現在承接 manifest/dependency classification 與 confidence constants。需驗證 ASCII case-insensitive 行為是否與各平台 `strcasestr`/`tolower` locale 完全一致，並確認 null qualified-name 與 long Cargo QN 邊界；目前未執行建置或測試。


### Compile command tokenizer slice

`cbm_split_command` 的 Rust ABI 直接填入 caller-provided `char**`，每個 token 以 C malloc 配置；若配置失敗目前保留 null pointer/count 行為，後續 gate 需確認與既有 C `strdup` failure path 的一致性。尚未執行建置或測試。


### Cross-repo JSON slice

`json_str_prop` 使用 caller-owned buffer，成功時回傳 1，缺少 key/null/zero buffer 回傳 0；輸出過長時截斷並 NUL 結尾。需驗證 key pattern buffer 127-byte truncation 與 escaped quote 行為，當前未執行建置或測試。


### Pipeline enrichment tokenizer slice

Rust ABI 直接填入 caller-provided `char**`，每個 token 由 C malloc 配置。需驗證 acronym boundary、decorator `@`/`#[...]`/argument stripping、stopword 與 allocation failure path；目前未執行建置或測試。


### Calls JSON slice

Rust extractor 對 missing/null/empty value 回傳 `SIZE_MAX`，C wrapper 轉為 null；成功輸出由 C malloc。需驗證 escaped quote、duplicate key 與 pattern 邊界，當前未執行建置或測試。


### Envscan classifier slice

Rust 承接 Dockerfile、`.env`、secret pattern、ignored directory 與 extension mapping。需驗證 256-byte name cap、ASCII lowercase、`.dockerfile`/`.env` suffix 與 case-sensitive extensions；目前未執行建置或測試。


### LSP cross classifier slice

Rust language constants 必須持續對齊 `internal/cbm/cbm.h` enum order；目前涵蓋 Go/C/C++/CUDA/Python/JS/TS/TSX/PHP/C#/Java/Kotlin/Rust。需驗證 null path、`.jsx`、`.d.ts` 與 unsupported language 行為，尚未執行建置或測試。

LSP classifier ABI 現在涵蓋 TS modes、language capability 與 definition label allowlist；label enum 新增時需同步 Rust allowlist 與 `cbm_label_is_type_like` C source of truth。


### Semantic edges JSON slice

Rust array reader 以 C malloc 配置每個字串，沿用既有 free path；contract 不解析 JSON escapes，且 pattern buffer 維持 63-byte truncation。需驗證 escaped quote、empty array、max_out 與 allocation failure，當前未執行建置或測試。

Pkgmap helper 群已涵蓋 text scan、dirname/stem 與 entry path composition；需驗證 1024-byte C buffer truncation、`./` stripping、null/empty suffix 與 extension edge cases。

Configlink helper 現包含分類、confidence matching 與 lowercase buffer writer；需驗證 zero-size buffer、null source、非 ASCII locale 差異與 truncation。

Cross-repo helper 現涵蓋 JSON property reader 與 CROSS edge property formatter；需驗證長輸入 truncation、null optional fields、key selection 與 caller buffer NUL 行為。

Compile command slice 現涵蓋 tokenizer 與 include path resolver；需驗證 null directory、absolute path、4K truncation 與 Windows path semantics。

### 2026-07-15：pipeline infrascan JSON array cleaner

- Rust helper：`pipeline::infrascan::clean_json_brackets`
- FFI：`cbm_rs_pipeline_clean_json_brackets_v1`
- C opt-in：`CBM_USE_RUST_PIPELINE_INFRASCAN=1` 或 direct gate 時委派；預設 C path 保持不變。
- 範圍：只搬移 bounded JSON array 清理；檔案內容掃描、parser、secret detection、manifest 判斷、graph ownership 與 pipeline side effects 仍由 C 管理。
- 驗證：Rust unit、C FFI smoke 與既有 infrascan opt-in gate 已補上，尚未執行。

### 2026-07-16：pipeline definitions JSON escape/length true-source

C fallback 已由 `src/pipeline/pass_definitions.c` 拆至
`src/pipeline/definitions_json.c`／`.h`；`pass_definitions.c` 僅經由公開 bridge 呼叫。
Rust helper 保持 `pipeline_definitions::{json_escape_char,json_escaped_len}`；wrapper FFI 為
`cbm_rs_pipeline_definitions_json_escape_char`、`cbm_rs_pipeline_definitions_json_escaped_len`。
`CBM_USE_RUST_PIPELINE_DEFINITIONS_JSON=1` 保留 C bridge 委派 Rust；
`CBM_USE_RUST_PIPELINE_DEFINITIONS_JSON_ONLY=1` 的 selector 排除
`src/pipeline/definitions_json.c`，並由 Rust 匯出同名 public C ABI。範圍僅限 JSON 字元
escape/escaped length；definitions pass 讀檔、Tree-sitter 結果、atomic append、graph ownership
與 pipeline side effects 仍由 C 管理。

驗證通過：`rust-pipeline-definitions-json-optin-test` 與
`rust-pipeline-definitions-json-only-test`；default/wrapper/direct FFI smoke、pipeline suite 各
`227 passed`、wrapper/direct production `--version`、ONLY selector 空值與 `make -Bn`
source-negative 不含 `src/pipeline/definitions_json.c`。

### 2026-07-16：pipeline similarity fingerprint parser true-source

C fallback 已由 `src/pipeline/pass_similarity.c` 拆至
`src/pipeline/similarity_fp.c`／`.h`；`pass_similarity.c` 僅經公開 bridge
`cbm_pipeline_similarity_parse_fp` 呼叫。Rust helper 保持
`simhash::parse_fp_from_props()`，wrapper FFI 為
`cbm_rs_pipeline_similarity_parse_fp_v1`。
`CBM_USE_RUST_PIPELINE_SIMILARITY_FP=1` 保留 C bridge 委派 Rust；
`CBM_USE_RUST_PIPELINE_SIMILARITY_FP_ONLY=1` 的 selector 排除
`src/pipeline/similarity_fp.c`，並由 Rust 匯出同名 public C ABI。範圍僅限
`"fp":"..."` 欄位定位與既有 MinHash hex decode；LSH index、similarity threshold、
graph edge、ownership 與 pipeline side effects 仍由 C 管理。

驗證通過：`rust-pipeline-similarity-fp-optin-test` 與
`rust-pipeline-similarity-fp-only-test`；default/wrapper/direct FFI smoke、
pipeline suite 各 `227 passed`、wrapper/direct production `--version`、ONLY selector
空值與 `make -Bn` source-negative 不含 `src/pipeline/similarity_fp.c`。

### 2026-07-17：pipeline infrascan JSON cleaner true-source

C fallback 已由 `src/pipeline/pass_infrascan.c` 拆至
`src/pipeline/infrascan_json.c`／`.h`；infrascan pass 僅經公開 bridge
`cbm_clean_json_brackets` 呼叫。Rust helper 保持
`pipeline::infrascan::clean_json_brackets()`，wrapper FFI 為
`cbm_rs_pipeline_clean_json_brackets_v1`。
`CBM_USE_RUST_PIPELINE_INFRASCAN_JSON=1` 保留 C bridge 委派 Rust；
`CBM_USE_RUST_PIPELINE_INFRASCAN_JSON_ONLY=1` 的 selector 排除
`src/pipeline/infrascan_json.c`，並由 Rust 匯出同名 public C ABI。範圍僅限 Dockerfile
CMD／ENTRYPOINT JSON array 的字串正規化；Dockerfile parse、命令擷取、graph ownership 與
pipeline side effects 仍由 C 管理。

驗證通過：`rust-pipeline-infrascan-json-optin-test` 與
`rust-pipeline-infrascan-json-only-test`；default/wrapper/direct FFI smoke、
pipeline suite 各 `227 passed`、wrapper/direct production `--version`、ONLY selector
空值與 `make -Bn` source-negative 不含 `src/pipeline/infrascan_json.c`。

### 2026-07-17：pipeline test-node is_test classifier true-source

C fallback 已由 `src/pipeline/pass_tests.c` 拆至
`src/pipeline/test_node_is_test.c`／`.h`；tests pass 僅經公開 bridge
`cbm_pipeline_test_node_is_test` 呼叫。`CBM_USE_RUST_PIPELINE_TEST_NODE_IS_TEST=1`
保留 C bridge 委派 wrapper v1 ABI `cbm_rs_pipeline_test_node_is_test_v1`；
`CBM_USE_RUST_PIPELINE_TEST_NODE_IS_TEST_ONLY=1` 搭配 Cargo feature
`pipeline-test-node-is-test-only` 時，selector 排除
`src/pipeline/test_node_is_test.c`，並由 Rust 匯出同名 public C ABI
`cbm_pipeline_test_node_is_test`。範圍僅限 `is_test` JSON 旗標 predicate；TESTS／TESTS_FILE
edge 建立、路徑 classifier、graph ownership 與 pipeline side effects 仍由 C 管理。NULL 回
false，且只匹配嚴格連續的 `"is_test":true`。

驗證通過：`rust-pipeline-test-node-is-test-optin-test` 與
`rust-pipeline-test-node-is-test-only-test`；default/wrapper/direct FFI smoke、pipeline
suite 各 `228 passed`、wrapper/direct production `--version`、ONLY selector 空值與
`make -Bn` source-negative 不含 `src/pipeline/test_node_is_test.c`。

### 2026-07-15：CLI archive byte helpers

- Rust helper：`cli_archive::{is_tar_end_of_archive,zip_read_u16}`
- FFI：`cbm_rs_cli_archive_tar_end_v1`、`cbm_rs_cli_archive_zip_read_u16_v1`
- C opt-in：`CBM_USE_RUST_CLI_ARCHIVE_HELPERS=1`；預設 C path 保持不變。
- 範圍：只搬移 tar 全零 block 判斷與 zip little-endian `uint16` 讀取；gzip、archive entry bounds、buffer ownership、檔案 I/O 與安裝 side effects 仍由 C 管理。
- 驗證：Rust unit、C FFI smoke 與 targeted CLI gate 已補上，尚未執行。

CLI archive slice 另包含 `zip_read_u32`／`cbm_rs_cli_archive_zip_read_u32_v1`，與 tar end、zip `uint16` reader 共用同一 opt-in gate；尚未驗證。

### 2026-07-15：pipeline K8s text helpers

- Rust helper：`pipeline_k8s::{basename_offset,indent,split_kv}`
- FFI：`cbm_rs_pipeline_k8s_basename_offset_v1`、`cbm_rs_pipeline_k8s_indent_v1`、`cbm_rs_pipeline_k8s_split_kv_v1`
- C opt-in：`CBM_USE_RUST_PIPELINE_K8S_HELPERS=1`；預設 C path 保持不變。
- 範圍：只搬移 basename、前導空格計數與單行 key/value 分割；YAML path stack、manifest scan、label matching、graph ownership 與 pipeline side effects 仍由 C 管理。
- 驗證：Rust unit、C FFI smoke 與 targeted pipeline gate 已補上，尚未驗證。

### 2026-07-15：pipeline incremental edge predicate

- Rust helper：`pipeline_incremental::edge_type_is_recomputed`
- FFI：`cbm_rs_pipeline_incremental_edge_type_is_recomputed_v1`
- C opt-in：`CBM_USE_RUST_PIPELINE_INCREMENTAL_EDGE=1`；預設 C path 保持不變。
- 範圍：只搬移四種 recomputed edge type 的字串 predicate；incremental purge、graph traversal、registry seed、SQLite persistence 與 dispatch lifecycle 仍由 C 管理。
- 驗證：Rust unit、C FFI smoke 與 targeted pipeline gate 已補上，尚未驗證。

### 2026-07-15：pipeline parallel JSON escape helper

- Rust helper：`pipeline_json::{json_escape_char,json_escaped_len}`
- FFI：`cbm_rs_pipeline_parallel_json_escape_char`、`cbm_rs_pipeline_parallel_json_escaped_len`
- C opt-in：`CBM_USE_RUST_PIPELINE_PARALLEL_JSON=1`；預設 C path 保持不變。
- 範圍：只搬移 parallel pass JSON 字元 escape 與 escaped length；atomic append、data flow、graph ownership 與 pipeline side effects 仍由 C 管理。
- 驗證：Rust unit（由共用 helper 覆蓋）、C FFI smoke 與 targeted pipeline gate 已補上，尚未驗證。

### 2026-07-16：discover language `.m` marker helpers true-source

C fallback 已拆至 `src/discover/language_markers.c`；`language.c` 僅經公開 bridge 呼叫。
Rust helper 為 `discover::language_marker()`，wrapper FFI 為
`cbm_rs_discover_language_marker_v1`。`CBM_USE_RUST_DISCOVER_LANGUAGE_MARKERS=1` 保留
C bridge 委派 Rust；`CBM_USE_RUST_DISCOVER_LANGUAGE_MARKERS_ONLY=1` 的 selector 排除
`src/discover/language_markers.c`，並由 Rust 匯出同名 public C ABI。範圍僅限 `.m` 檔案的
Objective-C、Magma end/callable 與 MATLAB line marker predicate；檔案讀取、marker 優先順序、
language selection 與 fallback 仍由 C 管理。

驗證通過：`rust-discover-language-markers-optin-test` 與
`rust-discover-language-markers-only-test`；default/wrapper/direct FFI smoke、discover suite 各
`87 passed`、wrapper/direct production `--version`、ONLY selector 空值與 `make -Bn`
source-negative 不含 `src/discover/language_markers.c`。

### 2026-07-15：pipeline incremental registry label predicate

- Rust helper：`pipeline_incremental::label_is_registry_symbol`
- FFI：`cbm_rs_pipeline_incremental_label_is_registry_symbol_v1`
- C opt-in：`CBM_USE_RUST_PIPELINE_INCREMENTAL_REGISTRY_LABEL=1`；預設 C path 保持不變。
- 範圍：只搬移 registry seed label 集合判斷；graph traversal、registry mutation、SQLite persistence 與 incremental lifecycle 仍由 C 管理。
- 驗證：Rust unit、C FFI smoke 與 targeted pipeline gate 已補上，尚未驗證。

### 2026-07-15：pipeline semantic Go suffix predicate

- Rust helper：`discover::ends_with`（pipeline-specific FFI）
- FFI：`cbm_rs_pipeline_semantic_fp_ends_with_v1`
- C opt-in：`CBM_USE_RUST_PIPELINE_SEMANTIC_FP_SUFFIX=1`；預設 C path 保持不變。
- 範圍：只搬移 Go interface file suffix 判斷；interface matching、graph traversal、edge 建立與 semantic side effects 仍由 C 管理。
- 驗證：Rust helper unit、C FFI smoke 與 targeted pipeline gate 已補上，尚未驗證。

### 2026-07-15：store architecture JSON property extractor

- Rust helper：`store::arch_helpers::extract_json_string_prop`
- FFI：`cbm_rs_store_arch_json_prop_len_v1`、`cbm_rs_store_arch_json_prop_copy_v1`
- C opt-in：`CBM_USE_RUST_STORE_ARCH_JSON_PROP=1`；預設 C path 保持不變。
- 範圍：只搬移 bounded JSON property locate；C 保留 malloc/free、SQLite query、architecture aggregation、result ownership 與 store runtime。
- 驗證：Rust unit、C FFI smoke 與 targeted store gate 已補上，尚未驗證。
## 2026-07-23 第 78 項完成後的權威交接增補

true-source 基線為 **73 / 73**；不得重跑或改寫 **#29–#78**。下方較早的 #77 基線以本段為準。下一步是
重新盤點 Pipeline／MCP／Store 的最小 pure helper，不預設候選；`pipeline-complexity-json` 與 content_length
value／raw 仍因 `strtol` 的 locale／overflow／NULL key 語義差異而延後。

**#78 pipeline-incremental-edge true-source**：C fallback 為
`src/pipeline/incremental_edge_type.c/.h`，public ABI 為
`bool cbm_pipeline_incremental_edge_type_is_recomputed(const char *edge_type)`。NULL 回 false 且不解參；其餘
合法 C 字串採 first-NUL、大小寫敏感 byte-exact 比較，只有 `SIMILAR_TO`、`SEMANTICALLY_RELATED`、
`FILE_CHANGES_WITH`、`DATA_FLOWS` 回 true，且無 allocation、輸入修改、pointer 保存、I/O、locale 或
ownership 行為。

wrapper 為 `CBM_USE_RUST_PIPELINE_INCREMENTAL_EDGE`，呼叫 Rust v1
`cbm_rs_pipeline_incremental_edge_type_is_recomputed_v1`；direct 為
`CBM_USE_RUST_PIPELINE_INCREMENTAL_EDGE_ONLY`，Cargo feature 為 `pipeline-incremental-edge-only`，Rust direct
匯出同名 public ABI。ONLY 時 `PIPELINE_INCREMENTAL_EDGE_SRCS =`，排除
`incremental_edge_type.c`，但保留 `src/pipeline/pipeline_incremental.c`。FFI support source 納入 selector，
default／wrapper public bridge 可正確連結。

真實 incremental regression 以 Serve／Help `CALLS` control edge 與 public
`cbm_store_insert_edge()` 建立的 `DATA_FLOWS` protected edge 驗證：變更 `helper.go` 後，`CALLS` 保留，
`DATA_FLOWS` 不會由 snapshot 回復。

已通過 `cargo fmt --all -- --check`、Rust unit **1 passed（337 filtered）**、direct feature
`clippy -D warnings`、opt-in／ONLY 各 default／wrapper／direct FFI、pipeline 三模式各 **237 passed**、
三次產品 `--version` `codebase-memory-mcp dev`，以及 direct selector/source negative 與
`pipeline_incremental.c` source positive proof。完整 `scripts/test.sh` 未執行。
> **2026-07-23 #80 權威交接更新：本區塊覆蓋下方所有較早的基線、current-task 與下一步敘述。**
> true-source 為 **75 / 75**，已完成 **#29–#80**，不得重跑或改寫 **#29–#80**。產品預設仍是 C11；Rust 僅經
> 明確 opt-in／direct-only 取代單一 pure helper，不可宣稱全庫 Rust default 或 Phase 5 RC。

## #80 `pipeline-similarity-file-ext` true-source

| 項目 | 最終狀態 |
|------|----------|
| C fallback／host | `src/pipeline/similarity_file_ext.c/.h`；`src/pipeline/pass_similarity.c` 保留 host，唯一 consumer `collect_fp_entries` 的結果流入 similarity comparison |
| C ABI／Rust v1 | `const char *cbm_pipeline_similarity_file_ext(const char *path);`；`const char *cbm_rs_pipeline_similarity_file_ext_v1(const char *path);` |
| 切換 | wrapper `CBM_USE_RUST_PIPELINE_SIMILARITY_FILE_EXT`；direct `CBM_USE_RUST_PIPELINE_SIMILARITY_FILE_EXT_ONLY`；Cargo `pipeline-similarity-file-ext-only` |
| direct 行為 | wrapper 呼叫 v1，僅 v1 回 `NULL` 時回退 C；direct 省略 fallback，Rust 匯出相同 public ABI |

契約：`path == NULL` 回傳非 `NULL`、process-lifetime 的靜態 NUL 空字串且不解參。非 `NULL` 為既有有效 NUL 終止
C 字串 precondition，僅使用第一個 NUL 前的 bytes，找最後一個 ASCII `.`；不剖析 `/` 或 `\\`，非 UTF-8 bytes
可用。dot 命中時回傳精確的 borrowed `path + offset`；無 dot 或空輸入回靜態空字串，該空字串在不同 mode 間不保證
pointer identity；不配置、不修改、不做 I/O、不使用 locale、不保存 pointer、也不轉移 ownership。`.gitignore` 回輸入
起點，`file.` 回 `"."`，`dir.name/file` 回 `".name/file"`；fullwidth dot 非 ASCII dot，embedded NUL 後不搜尋。

最終 gate 全數成功：fmt、Rust **1 passed／339 filtered**、direct clippy、FFI default／wrapper／direct、pipeline
三態各 **238 passed**、production `codebase-memory-mcp dev` 三次。direct selector empty，source-negative 排除
`similarity_file_ext.c`、source-positive 保留 `pass_similarity.c`，direct source-negative proof 累計 **17**。
`scripts/test.sh` 未跑。保留 defer：`pipeline-complexity-json` 與 content_length value／raw 的 `strtol` 語義差異。
下一個 #81 必須重新 inventory Pipeline／MCP／Store pure helpers，不能直接假設候選。
