# Rust 重構當前交接快照（2026-07-23，#81）

> **2026-07-23 #81 權威交接狀態：本區塊覆蓋下方所有較早的 current-task、基線與下一步敘述，包含 #80／#79。**
> true-source 累計為 **76 / 76**，已完成 **#29–#81**；不得重跑或改寫 **#29–#81**。產品預設仍為 C11，
> Rust 僅經明確 opt-in 或 direct-only 取代單一 pure helper；不可宣稱全庫 Rust default 或 Phase 5 RC。

## 2026-07-23 第 81 項：store-safe-props true-source

| # | 切片 | C fallback CU | Public ABI | Wrapper flag | Direct feature | Host 與 consumer | Suite |
|---|------|---------------|------------|--------------|----------------|------------------|-------|
| 81 | store safe props | `src/store/safe_props.c/.h` | `const char *cbm_store_safe_props(const char *s);` | `CBM_USE_RUST_STORE_SAFE_PROPS` | `store-safe-props-only` | `src/store/store.c` 保留；兩個 node／edge property write consumer 經 public bridge，最終傳入 `bind_text()` | store_nodes **56** |

Rust v1 ABI 為 `const char *cbm_rs_store_safe_props_v1(const char *s);`；direct macro 為
`CBM_USE_RUST_STORE_SAFE_PROPS_ONLY`。wrapper 先呼叫 v1，**僅**在 v1 回傳 `NULL` 時才回退 C fallback；
direct-only 由 Rust 匯出同名 public ABI。Cargo feature 是 `store-safe-props-only`；ONLY selector 排除
`safe_props.c`，但必須保留 `store.c` host。

凍結契約如下。`s == NULL` 時回傳 non-NULL、NUL 終止、process-lifetime static `"{}"`，且不可解參
`s`。`s != NULL` 時 helper 只需讀取第一個 byte：第一個 byte 為 NUL 仍回該 static `"{}"`；第一個 byte
非 NUL 時必回傳完全相同的 input borrowed pointer，且 helper 不掃描字串。實際下游 `bind_text()` 仍要求
legacy 有效 NUL 終止 C 字串；本 helper 不增加該 precondition 以外的讀取。行為以 bytes 為準、可處理
non-UTF-8；`x\0tail` 仍回原 input pointer。無 allocation、mutation、I/O、locale、pointer 保存或
ownership transfer；呼叫端不得 `free` 回傳值。static `"{}"` 的跨 C／wrapper／direct pointer identity 不保證，
但非空 input 的 pointer identity 必須保留。

完整成功 gate：`cargo fmt --all -- --check`、Rust target **1 passed，340 filtered**、direct feature
`clippy -D warnings`、default／wrapper／direct FFI、store_nodes 三態各 **56 passed**、三態 production
`--version` 均為 `codebase-memory-mcp dev`。direct `STORE_SAFE_PROPS_SRCS =`；`make -Bn` 排除
`src/store/safe_props.c` 並保留 `src/store/store.c`，direct source-negative 證據累計 **18**。完整
`scripts/test.sh` 未執行。

上一項為 #80 `pipeline-similarity-file-ext`。延後項仍為 `pipeline-complexity-json` 及 content_length
value／raw 的 `strtol` 語義。下一步 #82 必須重新盤點 Pipeline／MCP／Store pure helpers，再選單一最小切片；
不得預設候選或重跑 #29–#81。

# Rust 重構當前交接快照（2026-07-23）

> **2026-07-23 #80 權威交接狀態：本區塊覆蓋下方所有較早的 current-task、基線與下一步敘述。**
> true-source 累計為 **75 / 75**，已完成 **#29–#80**；不得重跑或改寫 **#29–#80**。產品預設仍為 C11，
> Rust 僅透過明確 opt-in 或 direct-only 取代單一 pure helper；不可宣稱全庫 Rust default 或 Phase 5 RC。

## 2026-07-23 第 80 項：pipeline-similarity-file-ext true-source

| # | 切片 | C fallback CU | Public ABI | Wrapper flag | Direct feature | Host 與 consumer | Suite |
|---|------|---------------|------------|--------------|----------------|------------------|-------|
| 80 | pipeline similarity file ext | `src/pipeline/similarity_file_ext.c/.h` | `const char *cbm_pipeline_similarity_file_ext(const char *path)` | `CBM_USE_RUST_PIPELINE_SIMILARITY_FILE_EXT` | `pipeline-similarity-file-ext-only` | `src/pipeline/pass_similarity.c` 保留；`collect_fp_entries` 經 public bridge 供既有 similarity comparison 使用 | pipeline **238** |

Rust v1 ABI 為 `const char *cbm_rs_pipeline_similarity_file_ext_v1(const char *path)`；direct macro 為
`CBM_USE_RUST_PIPELINE_SIMILARITY_FILE_EXT_ONLY`，Rust 在 direct-only 匯出同名 public ABI。wrapper 呼叫 v1，
僅在其回傳 `NULL` 時回退 C fallback；ONLY selector 排除 `similarity_file_ext.c`，但保留
`pass_similarity.c` host。

`path == NULL` 時回傳 non-NULL、process-lifetime、NUL 終止的 static empty string，且不解參照。非 NULL 時
沿用有效 NUL 終止 C 字串的 legacy precondition，只讀 first-NUL 前 bytes，回傳最後一個 ASCII `.` 的借用
pointer；無 dot 或空字串回 static empty。`a.b.c` 回 `.c`、`.gitignore` 回輸入起點、`file.` 回 `.`、
`dir.name/file` 回 `.name/file`，因 helper 不辨識 `/` 或 `\\` 分隔符；fullwidth dot 非 ASCII dot，non-UTF-8
依 bytes 處理，embedded NUL 截斷搜尋。成功的 dot 結果必等於 `path + offset`；空結果跨 C／wrapper／direct
的 pointer identity 不保證。無 allocation、mutation、I/O、locale、pointer 保存或 ownership transfer；呼叫端
不得 free 回傳的 borrowed pointer。

完整成功 gate：`cargo fmt --all -- --check`、Rust target **1 passed，339 filtered**、direct feature
`clippy -D warnings`、default／wrapper／direct FFI、pipeline 三態各 **238 passed**、三態 production
`--version`=`codebase-memory-mcp dev`。ONLY selector 為空；`make -Bn` 排除
`src/pipeline/similarity_file_ext.c` 並保留 `src/pipeline/pass_similarity.c`，direct source-negative 證據為
**17**。完整 `scripts/test.sh` 未執行。延後項仍為 `pipeline-complexity-json` 及 content_length value／raw 的
`strtol` 語義。下一步 #81 必須重新盤點 Pipeline／MCP／Store pure helper，再選單一最小切片。

## 2026-07-23 第 79 項：store-package-list-contains true-source

| # | 切片 | C fallback CU | Public ABI | Wrapper flag | Direct feature | Host 與 consumer | Suite |
|---|------|---------------|------------|--------------|----------------|------------------|-------|
| 79 | store package list contains | `src/store/package_list.c/.h` | `bool cbm_store_package_list_contains(const char *pkg, char **list, int count)` | `CBM_USE_RUST_STORE_PACKAGE_LIST` | `store-package-list-only` | `src/store/store.c` 保留，兩個 `arch_layers` consumer 皆經 public bridge | store_arch **56** |

凍結 ABI 另含 Rust v1：
`bool cbm_rs_store_package_list_contains_v1(const char *pkg, char **list, int count);`。
`count <= 0` 必回 `false`，且不得解參 `pkg` 或 `list`。僅在正 count 時，`pkg`、`list` 與會存取的 entries
必須是有效、非 `NULL`、NUL 終止 C 字串，這是既有 legacy precondition，不新增 NULL 防禦行為。依陣列 count
順序掃描，以第一個 NUL 前的 byte 進行 `strcmp` 等價、大小寫敏感的精確比對；有任一相等即回 true，否則 false。
此 helper 不配置記憶體、不修改輸入、不做 I/O、不使用 locale、不保存 pointer，亦不轉移 ownership。

C fallback 已由 `store.c` 拆至 `src/store/package_list.c/.h`；`store.c` 仍是 Store host，原有兩個
`arch_layers` consumer 改走 public bridge。wrapper `CBM_USE_RUST_STORE_PACKAGE_LIST` 轉呼叫 v1；direct-only
`CBM_USE_RUST_STORE_PACKAGE_LIST_ONLY` 搭配 Cargo `store-package-list-only`，會省略 fallback，並由 Rust 匯出同名
public ABI。production staticlib wiring 已納入三態 gate 覆蓋。新 direct contract test 為
`package_list_contains_contract`；既有 `arch_layers` 是實際 consumer integration。

完整成功 gate：`cargo fmt --all -- --check`；Rust target **1 passed，338 filtered**；direct feature 的
`cargo clippy -D warnings`；default／wrapper／direct FFI；三種模式的 `store_arch` 各 **56 passed**；三種
production `--version` 均為 `codebase-memory-mcp dev`。direct selector 為空，source-negative 不含
`src/store/package_list.c`，source-positive 保留 `src/store/store.c`。完整 `scripts/test.sh` **未執行**。

延後項維持不變：`pipeline-complexity-json` 與 content_length value／raw 的 `strtol` 語義差異。下一步從
Pipeline／MCP／Store pure-helper inventory 選擇並凍結 **#80** 的最小切片；不得預設候選或直接進 gate。

## 2026-07-23 第 78 項：pipeline-incremental-edge true-source

| # | 切片 | C fallback CU | Public ABI | Wrapper flag | Direct feature | Orchestration | Suite |
|---|------|---------------|------------|--------------|----------------|---------------|-------|
| 78 | pipeline incremental edge type | `src/pipeline/incremental_edge_type.c/.h` | `bool cbm_pipeline_incremental_edge_type_is_recomputed(const char *edge_type)` | `CBM_USE_RUST_PIPELINE_INCREMENTAL_EDGE` | `pipeline-incremental-edge-only` | `src/pipeline/pipeline_incremental.c` 保留 incremental 協調，僅走 public bridge | pipeline **237** |

此切片凍結 incremental snapshot 是否須重建的 pure classifier。`edge_type == NULL` 回 false，且不解參；其餘
有效 NUL 終止 C 字串只比較第一個 NUL 前的 bytes，大小寫敏感且 byte-exact。只有
`SIMILAR_TO`、`SEMANTICALLY_RELATED`、`FILE_CHANGES_WITH` 與 `DATA_FLOWS` 回 true；空字串、`CALLS`、未知、
大小寫不同、prefix／suffix 均回 false。沒有 allocation、輸入修改、pointer 保存、I/O、locale 或 ownership 行為。

wrapper macro 為 `CBM_USE_RUST_PIPELINE_INCREMENTAL_EDGE`，會呼叫 Rust v1
`cbm_rs_pipeline_incremental_edge_type_is_recomputed_v1`；direct macro 為
`CBM_USE_RUST_PIPELINE_INCREMENTAL_EDGE_ONLY`，Cargo feature 為 `pipeline-incremental-edge-only`，Rust direct
匯出同名 public ABI。C fallback 已自 `pipeline_incremental.c` 拆至
`src/pipeline/incremental_edge_type.c/.h`；ONLY 時 `PIPELINE_INCREMENTAL_EDGE_SRCS =` 為空並排除 fallback
source，仍保留 host `src/pipeline/pipeline_incremental.c`。Rust FFI support source 也納入此 selector，以維持
default／wrapper public C ABI 的連結。

真實 consumer regression 使用 Serve／Help 的 `CALLS` control edge，並經 public
`cbm_store_insert_edge()` 建立受保護的 `DATA_FLOWS` edge；修改 `helper.go` 後執行實際 incremental，驗證
`CALLS` 仍在，且 `DATA_FLOWS` 不會由 snapshot 回復。這同時證明 `incr_capture_inbound_edge` 對重建型 edge
不會捕捉 snapshot。

完整成功 gate 僅記錄：`cargo fmt --all -- --check`、
`cargo test -p cbm-core recomputed_edge_types_match_c_contract --locked`（**1 passed，337 filtered**）、
`cargo clippy -p cbm-core --all-targets --features pipeline-incremental-edge-only --locked -- -D warnings`，以及
`make -j1 -f Makefile.cbm rust-pipeline-incremental-edge-{optin,only}-test`。兩個 Make gate 均完成
default／wrapper／direct FFI、pipeline 三模式各 **237 passed** 與產品 `--version`
`codebase-memory-mcp dev`。ONLY 時 selector 空白，`make -Bn` 排除 `incremental_edge_type.c` 並保留
`pipeline_incremental.c`。完整 `scripts/test.sh` 未執行；`pipeline-complexity-json` 與 content_length 的
`strtol` 語義差異仍維持 defer。

## 2026-07-23 第 77 項：store-language-map true-source

| # | 切片 | C fallback CU | Public ABI | Wrapper flag | Direct feature | Orchestration | Suite |
|---|------|---------------|------------|--------------|----------------|---------------|-------|
| 77 | store language map | `src/store/language_map.c/.h` | `const char *cbm_store_ext_to_lang(const char *ext)` | `CBM_USE_RUST_STORE_LANGUAGE_MAP` | `store-language-map-only` | `src/store/store.c` 保留 Store 協調，僅走 public bridge | store_arch **55** |

此切片凍結既有 **44** 個 extension -> language 對應。`ext == NULL`、未知副檔名與大小寫不符均回
`NULL`；其餘輸入只以第一個 NUL 前的 bytes 作 `strcmp` 等價、大小寫敏感的比對。成功時回傳
process-lifetime、immutable、NUL 終止的 borrowed static pointer，呼叫端不得 `free`；不配置、不修改、
不保存 pointer、不做 I/O 或 locale 行為。

wrapper macro 為 `CBM_USE_RUST_STORE_LANGUAGE_MAP`，會先呼叫 Rust v1
`cbm_rs_store_ext_to_lang_v1`，僅在其回 `NULL` 時回退 C fallback；direct macro 為
`CBM_USE_RUST_STORE_LANGUAGE_MAP_ONLY`，Cargo feature 為 `store-language-map-only`，Rust direct 匯出同名
public bridge `cbm_store_ext_to_lang`。C fallback 已自 `store.c` 拆至 `src/store/language_map.c/.h`；ONLY 時由
`STORE_LANGUAGE_MAP_C_SRC`／`STORE_LANGUAGE_MAP_SRCS` 排除 `language_map.c`，但保留 `store.c`。

完整成功 gate 僅記錄：`cargo fmt --all -- --check`、`cargo test -p cbm-core --locked`（**338 passed**）、
`cargo clippy -p cbm-core --all-targets --features store-language-map-only --locked -- -D warnings`，以及
`make -j1 -f Makefile.cbm rust-store-language-map-{optin,only}-test`。兩個 Make gate 均完成
default／wrapper／direct FFI、store_arch 三模式各 **55 passed** 與產品 `--version`；ONLY 時
`STORE_LANGUAGE_MAP_SRCS =` 為空、`make -Bn` 不含 `src/store/language_map.c` 並保留 `src/store/store.c`。
初次 gate 發現的舊 duplicate target 已刪除，selector parse 無 warning。完整 `scripts/test.sh` 未執行。

## 2026-07-23 第 76 項：cypher-label-alt-match true-source

| # | 切片 | C fallback CU | Public ABI | Wrapper flag | Direct feature | Orchestration | Suite |
|---|------|---------------|------------|--------------|----------------|---------------|-------|
| 76 | cypher label alternation match | `src/cypher/label_alt_match.c/.h` | `bool cbm_cypher_label_alt_matches(const char *actual, const char *pat)` | `CBM_USE_RUST_CYPHER_LABEL_ALT_MATCH` | `cypher-label-alt-match-only` | `src/cypher/cypher.c` 三個 relationship traversal consumer 保留協調，僅走 public bridge | cypher **145** / cypher_contract **28** |

`pat == NULL` 時回 true 且完全不讀取 `actual`；`pat` 非 NULL 而 `actual == NULL` 時回 false 且不讀取
`pat` 內容。其餘合法 NUL 終止 C 字串只採 first-NUL 前 bytes、大小寫敏感精確比對。無 ASCII `|` 時等同
exact compare；有 `|` 時依序檢查 segments，leading／interior 空 segment 可匹配空 `actual`，但 trailing
empty segment 不檢查（`A|` 不能匹配空字串，`|` 可以）。僅 ASCII `|` 是 delimiter；不配置、不修改、不保存
pointer、不做 I/O 或 locale／ownership 行為。parser 的 `scan_alternation_labels`／`strtok_r` 路徑不屬本切片。

wrapper macro 為 `CBM_USE_RUST_CYPHER_LABEL_ALT_MATCH`，direct macro 為
`CBM_USE_RUST_CYPHER_LABEL_ALT_MATCH_ONLY`；Rust feature 為 `cypher-label-alt-match-only`。Rust v1
`cbm_rs_cypher_label_alt_matches_v1` 與 public ABI 同簽章，direct 匯出同名 public ABI。

完整成功 gate 僅記錄：`cargo fmt --all -- --check`、
`cargo test -p cbm-core cypher_label_alt_match --locked`（**1 passed，336 filtered**）、
`cargo clippy -p cbm-core --all-targets --features cypher-label-alt-match-only --locked -- -D warnings`，以及
`make -j1 -f Makefile.cbm rust-cypher-label-alt-match-{optin,only}-test`。兩個 Make gate 均完成
default／wrapper／direct FFI；cypher 三模式各 **145 passed**、cypher_contract 三模式各 **28 passed**，產品
`--version` 均為 `codebase-memory-mcp dev`。ONLY 時 `CYPHER_LABEL_ALT_MATCH_SRCS =` 為空，`make -Bn` 排除
`src/cypher/label_alt_match.c` 並保留 `src/cypher/cypher.c`。

## 2026-07-23 第 75 項：cypher-regex-shape true-source

| # | 切片 | C fallback CU | Public ABI | Wrapper flag | Direct feature | Orchestration | Suite |
|---|------|---------------|------------|--------------|----------------|---------------|-------|
| 75 | cypher regex shape | `src/cypher/regex_shape.c/.h` | `bool cbm_cypher_looks_like_regex(const char *s)` | `CBM_USE_RUST_CYPHER_REGEX_SHAPE` | `cypher-regex-shape-only` | `src/cypher/cypher.c` 的 `check_inline_props` 保留協調，僅走 public bridge | cypher **144** / cypher_contract **27** |

`NULL` 回 false，且完全不讀取；非 NULL 必須是有效 NUL 終止 C 字串，只讀第一個 NUL 前的 bytes。字串含
`.*`、`.+`，或任一 `[`, `(`, `|`, `^`, `$` 時回 true，其餘回 false。這只是 shape classifier，**不是**
真正的 regex parser；不配置、不修改、不保存 pointer、不做 I/O，且不依賴 locale 或 ownership。`check_inline_props`
仍由 C 負責後續 POSIX ERE 與 exact `strcmp` 的既有決策。

wrapper macro 為 `CBM_USE_RUST_CYPHER_REGEX_SHAPE`，direct macro 為
`CBM_USE_RUST_CYPHER_REGEX_SHAPE_ONLY`；Rust feature 為 `cypher-regex-shape-only`。Rust v1
`cbm_rs_cypher_looks_like_regex_v1` 與 public ABI 同簽章，direct 匯出同名 public ABI。

完整成功 gate 僅記錄：`cargo fmt --all -- --check`、
`cargo test -p cbm-core cypher_regex_shape --locked`（**1 passed，335 filtered**）、
`cargo clippy -p cbm-core --all-targets --features cypher-regex-shape-only --locked -- -D warnings`，以及
`make -j1 -f Makefile.cbm rust-cypher-regex-shape-{optin,only}-test`。兩個 Make gate 均完成
default／wrapper／direct FFI；cypher 三模式各 **144 passed**、cypher_contract 三模式各 **27 passed**，產品
`--version` 均為 `codebase-memory-mcp dev`。ONLY 時 `CYPHER_REGEX_SHAPE_SRCS =` 為空，`make -Bn` 排除
`src/cypher/regex_shape.c` 並保留 `src/cypher/cypher.c`。

## 2026-07-23 第 74 項：store-arch-aspect-filter true-source

| # | 切片 | C fallback CU | Public ABI | Wrapper flag | Direct feature | Orchestration | Suite |
|---|------|---------------|------------|--------------|----------------|---------------|-------|
| 74 | store architecture aspect filter | `src/store/arch_aspect_filter.c/.h` | `bool cbm_store_arch_wants_aspect(const char *const *aspects, int aspect_count, const char *name)` | `CBM_USE_RUST_STORE_ARCH_ASPECT_FILTER` | `store-arch-aspect-filter-only` | `src/store/store.c` 保留並改走 public bridge | store_arch **54** |

`aspects == NULL` 時一律回傳 true，不讀取 count 或 name；非 NULL 且 count 為零亦回 true，負 count
回 false。只有正 count 才讀取指標，依陣列順序以第一個 NUL 前的 byte 做大小寫敏感精準比較；元素為
`"all"` 或與 name 相同即回 true，否則回 false。沒有配置、修改、I/O 或 locale 行為。wrapper macro 為
`CBM_USE_RUST_STORE_ARCH_ASPECT_FILTER`，direct macro 為
`CBM_USE_RUST_STORE_ARCH_ASPECT_FILTER_ONLY`；Rust v1
`cbm_rs_store_arch_wants_aspect_v1` 與 public ABI 同簽章，direct 匯出同名 public ABI。

完整 gate 已成功：fmt、Rust unit **1 passed**、`clippy -D warnings`，以及 opt-in 與 ONLY 的
default/wrapper/direct FFI、store_arch 各 **54 passed**；產品 `--version` 為
`codebase-memory-mcp dev`。direct selector 為空，ONLY 排除 `src/store/arch_aspect_filter.c`，並保留
`src/store/store.c`。`pipeline-complexity-json` 的 `strtol` locale／overflow／NULL key 語義差異與
content_length value／raw（`strtol`）維持 defer；下一步回到 pure-helper inventory。

## 2026-07-23 第 73 項：pipeline-configlink-path-basename true-source

| # | 切片 | C fallback CU | Public ABI | Wrapper flag | Direct feature | Orchestration | Suite |
|---|------|---------------|------------|--------------|----------------|---------------|-------|
| 73 | configlink path basename | `src/pipeline/configlink_path_basename.c/.h` | `const char *cbm_pipeline_configlink_path_basename(const char *path)` | `CBM_USE_RUST_PIPELINE_CONFIGLINK_PATH_BASENAME` | `pipeline-configlink-path-basename-only` | `src/pipeline/pass_configlink.c` 保留並改走 public bridge | pipeline **236** |

C fallback 對 `NULL` 回傳非 NULL 的 static NUL 空字串；其他輸入僅以第一個 NUL 與 ASCII `/` 尋找最後一段
basename，回傳值為輸入內的 borrowed pointer。尾端 `/` 與 root 均指向終止 NUL，`\\` 不是分隔符；無配置、
修改或 I/O。wrapper macro 為 `CBM_USE_RUST_PIPELINE_CONFIGLINK_PATH_BASENAME`，direct macro 為
`CBM_USE_RUST_PIPELINE_CONFIGLINK_PATH_BASENAME_ONLY`；Rust direct v1
`cbm_rs_pipeline_configlink_path_basename_v1` 與 public ABI 同簽章。

完整 gate 已成功：fmt、Rust unit **1 passed**、`clippy -D warnings`，以及 opt-in 與 ONLY 的
default/wrapper/direct FFI、pipeline 各 **236 passed**；產品 `--version` 為
`codebase-memory-mcp dev`，direct selector 為空，ONLY 排除 fallback source 並保留
`pass_configlink.c`。`pipeline-complexity-json` 的 `strtol` locale／overflow／NULL key 語義差異與
content_length value／raw（`strtol`）維持 defer。

> **這是目前唯一的交接權威快照。** 接手前先讀本文件與根目錄
> [Handoff.md](../Handoff.md)、[Rust-Refactor.md](../Rust-Refactor.md)、[Tasks.md](../Tasks.md)、
> [rust/CBM_FFI.md](../rust/CBM_FFI.md)。產品預設仍是 **C11**；Rust 只經明確 `CBM_USE_RUST_*`
> opt-in／`*_ONLY` 接管 pure helper。不得宣稱全庫 Rust default 或 Phase 5 RC。
>
> Skill：`CodebaseMemMcp-Refactory-rust`
> （`~/.agents/skills/codebase-mem-mcp-refactory-rust/`）。

## 給下一個 Session 的起手式（先讀）

### 基線（勿重算、勿重跑）

| 項目 | 值 |
|------|-----|
| true-source 累計 | **73 / 73** |
| 本輪最新完成 | #60 MCP `tools_page_bounds` ~ #78 `pipeline-incremental-edge` |
| 不得重跑／改寫 | **#29–#78** |
| 延後 | `pipeline-complexity-json`（strtol locale／overflow／NULL key 未決策）；content_length value／raw（strtol） |
| suite 參考 | pipeline **237** / store_arch **55** / mcp **161** / cypher **145** / cypher_contract **28**（當日證據） |
| 未跑 | 完整 `scripts/test.sh`、`rust-all-optin-test`（除非使用者明示） |
| 禁止 | git commit／reset／clean（除非使用者明示）；並行 clean-c Make |
| 工作樹 | **dirty 且預期**（#29–#68 尚未 commit）；保留全部 dirty／untracked |
| 下一步 ready | pure-helper inventory；再挑選最小且契約可凍結的後續切片 |

### 工作樹預期狀態（2026-07-19 交接時）

**已修改（M）** 典型包含：

- `Makefile.cbm`、`rust/cbm-core/Cargo.toml`、`rust/cbm-core/src/ffi.rs`
- `src/mcp/mcp.c`、`src/pipeline/pass_calls.c`、`src/pipeline/pass_usages.c`、`src/pipeline/pass_pkgmap.c`
- `tests/test_mcp.c`、`tests/test_pipeline.c`、`tests/test_rust_ffi.c`、`rust/CBM_FFI.md`
- `docs/rust-refactor-current-handoff.md`、`Handoff.md`、`Tasks.md`、`Rust-Refactor.md`
- 可能還有 `rust/cbm-core/src/pipeline_usages.rs` 等既有 helper 調整

**未追蹤（??）** 新 C fallback CU（true-source 核心）：

| 路徑 | 切片 |
|------|------|
| `src/pipeline/calls_json.c/.h` | #29 |
| `src/pipeline/usages_json.c/.h` | #30 |
| `src/mcp/search_mode.c/.h` | #31 |
| `src/mcp/index_mode.c/.h` | #32 |
| `src/mcp/search_path_arg.c/.h` | #33 |
| `src/mcp/search_line_span.c/.h` | #34 |
| `src/mcp/search_args.c/.h` | #35 |
| `src/mcp/search_score_cmp.c/.h` | #36 |
| `src/pipeline/enrichment_tokens.c/.h` | #37 |
| `src/pipeline/pkgmap_text.c/.h` | #38 |
| `src/mcp/search_code_score.c/.h` | #39 |
| `src/mcp/search_top_dir.c/.h` | #40 |
| `src/mcp/detect_changes_scope.c/.h` | #41 |
| `src/mcp/detect_changes_label.c/.h` | #42 |
| `src/mcp/detect_changes_status.c/.h` | #43 |
| `src/mcp/node_resolution_score.c/.h` | #44 |
| `src/mcp/utf8_is_cont.c/.h` | #45 |
| `src/mcp/adr_mode.c/.h` | #46 |
| `src/mcp/trace_mode_edge_mask.c/.h` | #47 |
| `src/mcp/trace_is_test_file.c/.h` | #48 |
| `src/mcp/project_db_file_name.c/.h` | #49 |
| `src/mcp/sanitize_ascii_in_place.c/.h` | #50 |
| `src/mcp/bm25_file_pattern_like.c/.h` | #51 |
| `src/mcp/bm25_build_match.c/.h` | #52 |
| `src/mcp/sanitize_utf8_lossy.c/.h` | #53 |
| `src/mcp/architecture_aspect_wanted.c/.h` | #54 |
| `src/mcp/strip_root_prefix.c/.h` | #55 |
| `src/mcp/search_pick_tightest.c/.h` | #56 |
| `src/mcp/search_pick_resolved.c/.h` | #57 |
| `src/mcp/method_kind.c/.h` | #58 |
| `src/mcp/parse_file_uri.c/.h` | #59 |

可能另有異常 untracked（例如 ANSI escape 名稱的目錄）；**不要**自行 `rm`／`git clean`。

### 最近 true-source 一覽（#29–#59）

| # | 切片 | Fallback CU | Public ABI | Wrapper flag | Direct feature | Suite |
|---|------|-------------|------------|--------------|----------------|-------|
| 29 | calls-json | `src/pipeline/calls_json.c` | `char *cbm_pipeline_calls_extract_local_name` | `CBM_USE_RUST_PIPELINE_CALLS_JSON` | `pipeline-calls-json-only` | pipeline **231** |
| 30 | usages-json | `src/pipeline/usages_json.c` | `char *cbm_pipeline_usages_extract_local_name` | `CBM_USE_RUST_PIPELINE_USAGES_JSON` | `pipeline-usages-json-only` | pipeline **231** |
| 31 | search_mode | `src/mcp/search_mode.c` | `int cbm_mcp_search_mode` | `CBM_USE_RUST_MCP_SEARCH_MODE`（+CODEC） | `mcp-search-mode-only` | mcp **136** |
| 32 | index_mode | `src/mcp/index_mode.c` | `int cbm_mcp_index_mode` | `CBM_USE_RUST_MCP_INDEX_MODE`（+CODEC） | `mcp-index-mode-only` | mcp **136** |
| 33 | path_arg | `src/mcp/search_path_arg.c` | `bool cbm_mcp_search_path_arg_valid` | `CBM_USE_RUST_MCP_SEARCH_PATH_ARG`（+CODEC） | `mcp-search-path-arg-only` | mcp **136** |
| 34 | line_span | `src/mcp/search_line_span.c` | `int cbm_mcp_search_line_match_span` | `CBM_USE_RUST_MCP_SEARCH_LINE_SPAN`（+CODEC） | `mcp-search-line-span-only` | mcp **136** |
| 35 | search_args | `src/mcp/search_args.c` | `bool cbm_mcp_search_args_valid` | `CBM_USE_RUST_MCP_SEARCH_ARGS`（+CODEC） | `mcp-search-args-only` | mcp **136** |
| 36 | score_cmp | `src/mcp/search_score_cmp.c` | `int cbm_mcp_search_score_cmp` | `CBM_USE_RUST_MCP_SEARCH_SCORE_CMP`（+CODEC） | `mcp-search-score-cmp-only` | mcp **136** |
| 37 | enrichment tokens | `src/pipeline/enrichment_tokens.c` | `cbm_split_camel_case`／`cbm_tokenize_decorator` | `CBM_USE_RUST_PIPELINE_ENRICHMENT_TOKENS` | `pipeline-enrichment-tokens-only` | pipeline **233** |
| 38 | pkgmap text | `src/pipeline/pkgmap_text.c` | 6 個 `cbm_pipeline_pkgmap_*` helper | `CBM_USE_RUST_PIPELINE_PKGMAP_TEXT` | `pipeline-pkgmap-text-only` | pipeline **235** |
| 39 | search_code score | `src/mcp/search_code_score.c` | `int cbm_mcp_search_code_score` | `CBM_USE_RUST_MCP_SEARCH_CODE_SCORE`（+CODEC） | `mcp-search-code-score-only` | mcp **137** |
| 40 | search top dir | `src/mcp/search_top_dir.c` | `size_t cbm_mcp_search_top_dir` | `CBM_USE_RUST_MCP_SEARCH_TOP_DIR`（+CODEC） | `mcp-search-top-dir-only` | mcp **138** |
| 41 | detect changes scope | `src/mcp/detect_changes_scope.c` | `bool cbm_mcp_detect_changes_wants_symbols` | `CBM_USE_RUST_MCP_DETECT_CHANGES_SCOPE`（+CODEC） | `mcp-detect-changes-scope-only` | mcp **139** |
| 42 | detect changes label | `src/mcp/detect_changes_label.c` | `bool cbm_mcp_detect_changes_impacted_label` | `CBM_USE_RUST_MCP_DETECT_CHANGES_LABEL`（+CODEC） | `mcp-detect-changes-label-only` | mcp **140** |
| 43 | detect changes status | `src/mcp/detect_changes_status.c` | `size_t cbm_mcp_detect_changes_status_path_offset` | `CBM_USE_RUST_MCP_DETECT_CHANGES_STATUS`（+CODEC） | `mcp-detect-changes-status-only` | mcp **141** |
| 44 | node resolution score | `src/mcp/node_resolution_score.c` | `long cbm_mcp_node_resolution_score` | `CBM_USE_RUST_MCP_NODE_RESOLUTION_SCORE`（+CODEC） | `mcp-node-resolution-score-only` | mcp **142** |
| 45 | utf8 is cont | `src/mcp/utf8_is_cont.c` | `bool cbm_mcp_utf8_is_cont_byte` | `CBM_USE_RUST_MCP_UTF8_IS_CONT`（+CODEC） | `mcp-utf8-is-cont-only` | mcp **143** |
| 46 | ADR mode | `src/mcp/adr_mode.c` | `int cbm_mcp_adr_mode` | `CBM_USE_RUST_MCP_ADR_MODE`（+CODEC） | `mcp-adr-mode-only` | mcp **144** |
| 47 | trace mode edge mask | `src/mcp/trace_mode_edge_mask.c` | `uint32_t cbm_mcp_trace_mode_edge_mask` | `CBM_USE_RUST_MCP_TRACE_MODE_EDGE_MASK`（+CODEC） | `mcp-trace-mode-edge-mask-only` | mcp **145** |
| 48 | trace is test file | `src/mcp/trace_is_test_file.c` | `bool cbm_mcp_trace_is_test_file` | `CBM_USE_RUST_MCP_TRACE_IS_TEST_FILE`（+CODEC） | `mcp-trace-is-test-file-only` | mcp **146** |
| 49 | project db file name | `src/mcp/project_db_file_name.c` | `bool cbm_mcp_project_db_file_name` | `CBM_USE_RUST_MCP_PROJECT_DB_FILE_NAME`（+CODEC） | `mcp-project-db-file-name-only` | mcp **147** |
| 50 | sanitize ASCII in place | `src/mcp/sanitize_ascii_in_place.c` | `void cbm_mcp_sanitize_ascii_in_place` | `CBM_USE_RUST_MCP_SANITIZE_ASCII_IN_PLACE`（+CODEC） | `mcp-sanitize-ascii-in-place-only` | mcp **148** |
| 51 | BM25 file pattern LIKE | `src/mcp/bm25_file_pattern_like.c` | `size_t cbm_mcp_bm25_file_pattern_like(char *, size_t, const char *)` | `CBM_USE_RUST_MCP_BM25_FILE_PATTERN_LIKE`（+CODEC） | `mcp-bm25-file-pattern-like-only` | mcp **149** |
| 52 | BM25 build MATCH | `src/mcp/bm25_build_match.c` | `int cbm_mcp_bm25_build_match(const char *, char *, size_t)` | `CBM_USE_RUST_MCP_BM25_BUILD_MATCH`（+CODEC） | `mcp-bm25-build-match-only` | mcp **150** |
| 53 | sanitize UTF-8 lossy | `src/mcp/sanitize_utf8_lossy.c` | `char *cbm_mcp_sanitize_utf8_lossy(const char *)` | `CBM_USE_RUST_MCP_SANITIZE_UTF8_LOSSY`（+CODEC） | `mcp-sanitize-utf8-lossy-only` | mcp **151** |
| 54 | architecture aspect wanted | `src/mcp/architecture_aspect_wanted.c` | `bool cbm_mcp_architecture_aspect_wanted` | `CBM_USE_RUST_MCP_ARCHITECTURE_ASPECT_WANTED`（+CODEC） | `mcp-architecture-aspect-wanted-only` | mcp **152** |
| 55 | strip root prefix offset | `src/mcp/strip_root_prefix.c` | `size_t cbm_mcp_strip_root_prefix_offset` | `CBM_USE_RUST_MCP_STRIP_ROOT_PREFIX`（+CODEC） | `mcp-strip-root-prefix-only` | mcp **153** |
| 56 | search pick tightest index | `src/mcp/search_pick_tightest.c` | `int cbm_mcp_search_pick_tightest_index` | `CBM_USE_RUST_MCP_SEARCH_PICK_TIGHTEST`（+CODEC） | `mcp-search-pick-tightest-only` | mcp **154** |
| 57 | search pick resolved index | `src/mcp/search_pick_resolved.c` | `int cbm_mcp_search_pick_resolved_index` | `CBM_USE_RUST_MCP_SEARCH_PICK_RESOLVED`（+CODEC） | `mcp-search-pick-resolved-only` | mcp **155** |
| 58 | method_kind | `src/mcp/method_kind.c` | `int cbm_mcp_method_kind` | `CBM_USE_RUST_MCP_METHOD_KIND`（+CODEC） | `mcp-method-kind-only` | mcp **156** |
| 59 | parse_file_uri | `src/mcp/parse_file_uri.c` | `bool cbm_parse_file_uri` | `CBM_USE_RUST_MCP_PARSE_FILE_URI`（+CODEC） | `mcp-parse-file-uri-only` | mcp **156** |

共同 bar：獨立 C fallback CU、預設 C path、wrapper 委派 v1、direct 同名 public ABI、
ONLY 時 `*_SRCS =`、`make -Bn` 不含 fallback 且仍含 orchestration（`pass_*.c` 或 `mcp.c`）、
default/wrapper/direct 實際跑 FFI + suite + production `--version`。

**ownership 提醒：** #29／#30／#37／#38 的 heap 結果均由 C `malloc` 配置、caller `free`；#38 的
`find_line_value` 則借用 input `source`。#31–#36、#39–#49 為 pure classifier／scalar，#50 為 pure in-place
mutator；#51 為 caller-owned buffer serializer，C fallback 只暫時配置並釋放 Store glob 結果；#52 為 pure
caller-owned buffer serializer；#53 回傳 C `malloc` 字串且由 caller `free`。#51／#52 的 public ABI 都不移轉
heap ownership。#59 為 caller-owned buffer path extractor（無 heap 移轉）。


### 2026-07-19 第 59 項：MCP parse_file_uri true-source

既有 public API `bool cbm_parse_file_uri(const char *uri, char *out_path, int out_size)` 已拆為獨立 C
fallback `src/mcp/parse_file_uri.c/.h`；`mcp.c` 僅 include bridge，不再內嵌實作。契約：byte-exact
`file://` prefix；保留 raw path 與 percent encoding；Windows drive `file:///C:/...` 剝一個 leading `/`；
成功含 empty path `file://` 與 buffer 截斷；NULL uri／非法 scheme → false 並在可寫時清空；
`out_path==NULL` 或 `out_size<=0` → false。wrapper `CBM_USE_RUST_MCP_PARSE_FILE_URI`（相容 CODEC）委派
`cbm_rs_mcp_parse_file_uri_v1`；direct `…_ONLY`／Cargo `mcp-parse-file-uri-only` 匯出同名 `cbm_parse_file_uri`。

Make：`rust-mcp-parse-file-uri-{optin,only}-test`。已通過 unit `parse_file_uri` **1**、clippy（feature
`mcp-parse-file-uri-only`）、optin/only default/wrapper/direct FFI + mcp 各 **156 passed** + production
`--version`；ONLY 空 `MCP_PARSE_FILE_URI_SRCS`、`make -Bn` 不含 `parse_file_uri.c`、仍含 `mcp.c`。
完整 `scripts/test.sh` 未跑。

### 2026-07-19 第 58 項：MCP method_kind true-source

JSON-RPC method exact-match classifier 已拆為 C fallback `src/mcp/method_kind.c/.h`；enum 常數
（UNKNOWN=0 … NOTIFICATIONS_CANCELLED=5）與公開 ABI `int cbm_mcp_method_kind(const char *method)`
一併移出 `mcp.c`。NULL／空／大小寫不符／尾空白／未知 → 0；byte-exact first-NUL `strcmp`；無 heap。

`mcp.c` 無 id 路徑以 `cbm_mcp_method_kind(...) == NOTIFICATIONS_CANCELLED` 辨 cancel；有 id 路徑
`method_kind = cbm_mcp_method_kind(req.method)` 後仍由 C dispatch handler。wrapper
`CBM_USE_RUST_MCP_METHOD_KIND`（相容 CODEC）；direct `…_ONLY`／Cargo `mcp-method-kind-only`。

Make：`rust-mcp-method-kind-{optin,only}-test`。已通過 cargo fmt check、`method_kind` unit **1 passed**、
切片 clippy、optin/only default/wrapper/direct FFI + mcp 各 **156 passed** + production `--version`；
ONLY 空 `MCP_METHOD_KIND_SRCS`、`make -Bn` 不含 `method_kind.c`、仍含 `mcp.c`。完整 `scripts/test.sh` 未跑。

### 2026-07-19 第 57 項：MCP search_pick_resolved true-source

`pick_resolved_node` 的 scores argmax／tie 已拆為 C fallback `src/mcp/search_pick_resolved.c/.h`；公開 ABI
`int cbm_mcp_search_pick_resolved_index(const long *scores, int count, bool *ambiguous_out)`。
`ambiguous_out == NULL` → -1；invalid scores/count → -1 且 ambiguous=false；最大 score 的第一個 index，
多個 top → ambiguous=true。`count <= 1` 短路仍在 `mcp.c`。wrapper／direct
`CBM_USE_RUST_MCP_SEARCH_PICK_RESOLVED[_ONLY]`／`mcp-search-pick-resolved-only`。

已通過 `rust-mcp-search-pick-resolved-{optin,only}-test`：mcp 各 **155 passed**、production `--version`；
ONLY 空 selector、bn 不含 `search_pick_resolved.c`、仍含 `mcp.c`。

### 2026-07-19 第 56 項：MCP search_pick_tightest true-source

`find_tightest_node` 的 span 陣列 argmin 已拆為 C fallback `src/mcp/search_pick_tightest.c/.h`；公開 ABI
`int cbm_mcp_search_pick_tightest_index(const int *spans, int count)`。`count <= 0`／NULL spans → -1；
只取 `span >= 0` 的最小第一個 index；無 heap。`find_tightest_node` 先以 `cbm_mcp_search_line_match_span`
填 spans 再呼叫 public；OOM 時 streaming fallback 對齊 `>=0`／`INT_MAX`。wrapper
`CBM_USE_RUST_MCP_SEARCH_PICK_TIGHTEST`（+CODEC）；direct `…_ONLY`／`mcp-search-pick-tightest-only`。

已通過 fmt／unit／clippy 與 `rust-mcp-search-pick-tightest-{optin,only}-test`：mcp 各 **154 passed**、
production `--version`；ONLY 空 selector、`make -Bn` 不含 `search_pick_tightest.c`、仍含 `mcp.c`。

### 2026-07-19 第 55 項：MCP strip_root_prefix true-source

`search_code` grep path 的 root prefix 剝除已拆為 C fallback `src/mcp/strip_root_prefix.c/.h`；公開 ABI
`size_t cbm_mcp_strip_root_prefix_offset(const char *path, const char *root, size_t root_len)`。NULL path／root → 0；
`root_len` 超過 first-NUL 長度 → 0；byte prefix 比對，符合後若下一 byte 是 `/` 再 +1；不檢查 path component 邊界；
不配置、caller 以 `path + offset` 借用。wrapper `CBM_USE_RUST_MCP_STRIP_ROOT_PREFIX`（+CODEC）；direct
`…_ONLY`／`mcp-strip-root-prefix-only`。`mcp.c` 的 `strip_root_prefix` 僅轉接 public ABI。

已通過 fmt／unit／clippy 與 `rust-mcp-strip-root-prefix-{optin,only}-test`：mcp 各 **153 passed**、production
`--version`；ONLY 空 `MCP_STRIP_ROOT_PREFIX_SRCS`、`make -Bn` 不含 `strip_root_prefix.c`、仍含 `mcp.c`。

### 2026-07-19 第 54 項：MCP architecture_aspect_wanted true-source

`get_architecture` 的 aspects filter 已抽出 C fallback `src/mcp/architecture_aspect_wanted.c/.h`，公開 ABI
`bool cbm_mcp_architecture_aspect_wanted(const char *input, const char *name)`。NULL／invalid JSON／root 非 object／
缺 `aspects` 或非 array → true；空 array／無匹配 → false；`"all"` 或精準 name match → true。fallback 暫配 yyjson、
無 caller heap 移轉。wrapper／CODEC 委派 v1；direct `mcp-architecture-aspect-wanted-only`。`RUST_FFI_SUPPORT_SRCS`
含 `$(YYJSON_SRC)` 以連結 fallback。

已通過 `make -j1 -f Makefile.cbm rust-mcp-architecture-aspect-wanted-{optin,only}-test`：mcp 各 **152 passed**、
production `--version`；ONLY 空 selector + `make -Bn` 不含 `architecture_aspect_wanted.c`、仍含 `mcp.c`。
#54 已計入 true-source。

### 2026-07-19 第 53 項：MCP sanitize_utf8_lossy true-source

code snippet 的來源 UTF-8 lossy serializer 已拆為 C fallback `src/mcp/sanitize_utf8_lossy.c/.h`；`mcp.c` 僅保留
snippet source resolution、yyjson result shaping、response 與 transport orchestration。公開 ABI：
`char *cbm_mcp_sanitize_utf8_lossy(const char *input)`。NULL 回 NULL；其餘輸入採 first-NUL，保留有效 UTF-8，
每個無效 byte 以 U+FFFD（`EF BF BD`）替換；成功回傳 C `malloc` 字串，caller 必須 `free()`，無 I/O。

wrapper `CBM_USE_RUST_MCP_SANITIZE_UTF8_LOSSY=1`（亦相容 `CBM_USE_RUST_MCP_CODEC=1`）以既有 v1 的
length-only／caller-buffer ABI 配置 C output；direct `CBM_USE_RUST_MCP_SANITIZE_UTF8_LOSSY_ONLY=1`／Cargo
`mcp-sanitize-utf8-lossy-only` 由 Rust 呼叫 C `malloc` 後匯出同名 ABI，維持 C caller `free` 之 ownership。測試
覆蓋 NULL、ASCII、invalid byte、first-NUL 與 direct allocation/free。已通過 `clang-format --dry-run`（新增 CU）、
`cargo fmt --check`、`cargo test -p cbm-core sanitize_utf8_lossy --locked`（**1 passed**）、`cargo clippy -p
cbm-core --all-targets --features mcp-sanitize-utf8-lossy-only --locked -- -D warnings`，以及
`make -j1 -f Makefile.cbm rust-mcp-sanitize-utf8-lossy-{optin,only}-test` 的所有 default/wrapper/direct legs。
MCP 均 **151 passed**、production `--version` 均成功；ONLY 的 `MCP_SANITIZE_UTF8_LOSSY_SRCS =`、`make -Bn`
排除 `sanitize_utf8_lossy.c` 並保留 `mcp.c` 亦成功。完整 `scripts/test.sh` 未跑。

### 2026-07-19 第 52 項：MCP bm25_build_match true-source

`search_graph.query` 的 BM25 FTS5 MATCH tokenizer/serializer 已拆為 C fallback
`src/mcp/bm25_build_match.c/.h`；`mcp.c` 僅保留 FTS5 SQLite query、ranking／boosting、result JSON 與 transport
orchestration。公開 C ABI 為 `int cbm_mcp_bm25_build_match(const char *input, char *buf, size_t bufsize)`；注意其
參數順序刻意維持既有 C call site（`input` 在前），Rust v1 ABI 仍為 `buf, bufsize, input`。

契約已凍結：`input == NULL`、`buf == NULL` 或 `bufsize < 2` 回 0 並不寫入。有效輸入只保留 ASCII
alnum／underscore token，以 ` OR ` 串接；若下一個完整 token 不含 NUL 已放不下，停在上一個完整 token，並
NUL-terminate。標點與 non-ASCII 都是分隔符，輸入採 first-NUL；無 heap、無 I/O，輸出 buffer 由 caller 擁有。
wrapper `CBM_USE_RUST_MCP_BM25_BUILD_MATCH=1`（亦相容 `CBM_USE_RUST_MCP_CODEC=1`）委派既有 v1；direct
`CBM_USE_RUST_MCP_BM25_BUILD_MATCH_ONLY=1`／Cargo `mcp-bm25-build-match-only` 由 Rust 匯出同名 public ABI。

`tests/test_mcp.c` 與 `tests/test_rust_ffi.c` 覆蓋 NULL、過小 buffer、純標點、完整 token、短 buffer、
first-NUL 與 direct public ABI。已通過 `clang-format --dry-run`（新增 CU）、`cargo fmt --check`、
`cargo test -p cbm-core bm25_match_query --locked`（**1 passed**）、`cargo clippy -p cbm-core --all-targets
--features mcp-bm25-build-match-only --locked -- -D warnings`，以及
`make -j1 -f Makefile.cbm rust-mcp-bm25-build-match-{optin,only}-test` 的所有 default/wrapper/direct legs。
MCP 均 **150 passed**、production `--version` 均成功；ONLY 的 `MCP_BM25_BUILD_MATCH_SRCS =`、`make -Bn`
排除 `bm25_build_match.c` 並保留 `mcp.c` 亦成功。完整 `scripts/test.sh` 未跑。

### 2026-07-19 第 51 項：MCP bm25_file_pattern_like true-source

`search_graph.file_pattern` 的 BM25 SQL LIKE serializer 已拆為 C fallback
`src/mcp/bm25_file_pattern_like.c/.h`；`mcp.c` 僅保留 caller-owned final buffer 的配置／釋放與 BM25 SQLite
query orchestration。公開 ABI：`size_t cbm_mcp_bm25_file_pattern_like(char *buf, size_t bufsize, const char *input)`。
wrapper `CBM_USE_RUST_MCP_BM25_FILE_PATTERN_LIKE=1`（亦相容 `CBM_USE_RUST_MCP_CODEC=1`）委派既有 v1；direct
`CBM_USE_RUST_MCP_BM25_FILE_PATTERN_LIKE_ONLY=1`／Cargo `mcp-bm25-file-pattern-like-only` 由 Rust 匯出同名 ABI。

契約已凍結：`input == NULL` 回 `SIZE_MAX`；成功時回完整輸出 byte 長度。`buf == NULL` 或 `bufsize == 0`
只回長度；短 buffer 最多寫 `bufsize - 1` bytes 並 NUL-terminate。輸入採 first-NUL；先沿用 Store glob-to-LIKE
（`*` 為 `%`、`?` 為 `_`），若原 pattern 不含 wildcard 則加前後 `%` 進行 contains 查詢。輸出 buffer 由 caller
擁有；C fallback 的暫時 glob buffer 會在函式內釋放，無 I/O。

`tests/test_mcp.c` 與 `tests/test_rust_ffi.c` 覆蓋 NULL、length-only、empty、glob star／question、短 buffer、
first-NUL 與 direct public ABI。已通過 `clang-format --dry-run`（新增 CU）、`cargo fmt --check`、
`cargo test -p cbm-core bm25_file_pattern_like --locked`（**1 passed**）、`cargo clippy -p cbm-core --all-targets
--features mcp-bm25-file-pattern-like-only --locked -- -D warnings`，以及
`make -j1 -f Makefile.cbm rust-mcp-bm25-file-pattern-like-{optin,only}-test` 的所有 default/wrapper/direct legs。
MCP 均 **149 passed**、production `--version` 均成功；ONLY 的 `MCP_BM25_FILE_PATTERN_LIKE_SRCS =`、`make -Bn`
排除 `bm25_file_pattern_like.c` 並保留 `mcp.c` 亦成功。完整 `scripts/test.sh` 未跑。

### 2026-07-19 第 50 項：MCP sanitize_ascii_in_place true-source

`search_code` output 的 ASCII sanitizer 已拆為 C fallback
`src/mcp/sanitize_ascii_in_place.c/.h`；`mcp.c` 保留 grep／source／context 讀取、JSON building、result shaping、
snippet UTF-8 lossy sanitizer 與 transport orchestration。公開 ABI：
`void cbm_mcp_sanitize_ascii_in_place(char *input)`。wrapper
`CBM_USE_RUST_MCP_SANITIZE_ASCII_IN_PLACE=1`（亦相容 `CBM_USE_RUST_MCP_CODEC=1`）委派既有 v1；direct
`CBM_USE_RUST_MCP_SANITIZE_ASCII_IN_PLACE_ONLY=1`／Cargo `mcp-sanitize-ascii-in-place-only` 由 Rust 匯出同名 ABI。

契約已凍結：NULL 為 no-op；僅掃描至第一個 NUL，將每一個大於 127 的 byte 就地改為 `?`，其餘 ASCII（含
`0x7f`）不變；NUL 後的 byte 不讀不改。輸入 mutation 為 ABI 行為的一部分，無 heap、無 I/O。

`tests/test_mcp.c` 與 `tests/test_rust_ffi.c` 覆蓋 NULL、ASCII、`0x7f`、多個 high-byte、first-NUL 與
direct public ABI。已通過 `clang-format --dry-run`（新增 CU）、`cargo fmt --all -- --check`、
`cargo test -p cbm-core sanitize_ascii_in_place --locked`（**1 passed**）、`cargo clippy -p cbm-core
--all-targets --features mcp-sanitize-ascii-in-place-only --locked -- -D warnings`，以及
`make -j1 -f Makefile.cbm rust-mcp-sanitize-ascii-in-place-{optin,only}-test` 的所有 default/wrapper/direct legs。
MCP 均 **148 passed**、production `--version` 均成功；ONLY 的 `MCP_SANITIZE_ASCII_IN_PLACE_SRCS =`、`make -Bn`
排除 `sanitize_ascii_in_place.c` 並保留 `mcp.c` 亦成功。完整 `scripts/test.sh` 未跑。

### 2026-07-19 第 49 項：MCP project_db_file_name true-source

cache directory 的 project `.db` filename classifier 已拆為 C fallback
`src/mcp/project_db_file_name.c/.h`；`mcp.c` 保留目錄掃描、SQLite query-open、internal project-name
resolution、ghost/corrupt DB filtering、list JSON building、resolve fallback 與 transport orchestration。公開 ABI：
`bool cbm_mcp_project_db_file_name(const char *name)`。wrapper
`CBM_USE_RUST_MCP_PROJECT_DB_FILE_NAME=1`（亦相容 `CBM_USE_RUST_MCP_CODEC=1`）委派既有 v1；direct
`CBM_USE_RUST_MCP_PROJECT_DB_FILE_NAME_ONLY=1`／Cargo `mcp-project-db-file-name-only` 由 Rust 匯出同名 ABI。

契約已凍結：NULL、空字串、`.db`、非精準 `.db` suffix、`_` 開頭與 `:memory:` 開頭回 false；長度至少
`x.db` 且精準 `.db` 結尾才可能回 true，`tmp-*.db` 維持有效。字串僅讀至第一個 NUL；無 heap、無輸入改寫、
無 I/O。

`tests/test_mcp.c` 與 `tests/test_rust_ffi.c` 覆蓋 accepted／rejected filename、first-NUL 與 direct public ABI。
已通過 `clang-format --dry-run`（新增 CU）、`cargo fmt --all -- --check`、
`cargo test -p cbm-core project_db_file_name --locked`（**1 passed**）、`cargo clippy -p cbm-core
--all-targets --features mcp-project-db-file-name-only --locked -- -D warnings`，以及
`make -j1 -f Makefile.cbm rust-mcp-project-db-file-name-{optin,only}-test` 的所有 default/wrapper/direct legs。
MCP 均 **147 passed**、production `--version` 均成功；ONLY 的 `MCP_PROJECT_DB_FILE_NAME_SRCS =`、`make -Bn`
排除 `project_db_file_name.c` 並保留 `mcp.c` 亦成功。完整 `scripts/test.sh` 未跑。

### 2026-07-19 第 48 項：MCP trace_is_test_file true-source

`trace_call_path` 的 MCP-local test-file classifier 已拆為 C fallback
`src/mcp/trace_is_test_file.c/.h`；`mcp.c` 保留 BFS traversal、`include_tests` filtering、`is_test` JSON 標記、
risk labels、data-flow args、JSON 與 transport orchestration。公開 ABI：
`bool cbm_mcp_trace_is_test_file(const char *path)`。wrapper
`CBM_USE_RUST_MCP_TRACE_IS_TEST_FILE=1`（亦相容 `CBM_USE_RUST_MCP_CODEC=1`）委派既有 v1；direct
`CBM_USE_RUST_MCP_TRACE_IS_TEST_FILE_ONLY=1`／Cargo `mcp-trace-is-test-file-only` 由 Rust 匯出同名 ABI。

契約已凍結：NULL、空字串回 false；path 含 `/test`、`test_`、`_test.`、`/tests/`、`/spec/` 或 `.test.`
任一 case-sensitive substring 回 true，其餘回 false。字串僅讀至第一個 NUL；無 heap、無輸入改寫、無 I/O。

`tests/test_mcp.c` 與 `tests/test_rust_ffi.c` 覆蓋 NULL、所有 accepted／rejected substring、Windows separator、
first-NUL 與 direct public ABI。已通過 `clang-format --dry-run`（新增 CU）、`cargo fmt --all -- --check`、
`cargo test -p cbm-core trace_is_test_file --locked`（**1 passed**）、`cargo clippy -p cbm-core
--all-targets --features mcp-trace-is-test-file-only --locked -- -D warnings`，以及
`make -j1 -f Makefile.cbm rust-mcp-trace-is-test-file-{optin,only}-test` 的所有 default/wrapper/direct legs。
MCP 均 **146 passed**、production `--version` 均成功；ONLY 的 `MCP_TRACE_IS_TEST_FILE_SRCS =`、`make -Bn`
排除 `trace_is_test_file.c` 並保留 `mcp.c` 亦成功。完整 `scripts/test.sh` 未跑。

### 2026-07-19 第 47 項：MCP trace_mode_edge_mask true-source

`resolve_trace_edge_types()` 的 default mode classifier 已拆為 C fallback
`src/mcp/trace_mode_edge_mask.c/.h`；`mcp.c` 保留 explicit `edge_types` array parsing／ownership、edge-name
填入順序、BFS traversal、data-flow args、JSON 與 transport orchestration。公開 ABI：
`uint32_t cbm_mcp_trace_mode_edge_mask(const char *input)`。wrapper
`CBM_USE_RUST_MCP_TRACE_MODE_EDGE_MASK=1`（亦相容 `CBM_USE_RUST_MCP_CODEC=1`）委派既有 v1；direct
`CBM_USE_RUST_MCP_TRACE_MODE_EDGE_MASK_ONLY=1`／Cargo `mcp-trace-mode-edge-mask-only` 由 Rust 匯出同名 ABI。

契約已凍結：NULL、空字串、`calls`、未知、大小寫不符與尾端空白回 `CALLS` bit；`data_flow` 回
`CALLS|DATA_FLOWS`；`cross_service` 回 bit 0..9 的完整 edge set（`0x3ff`）。字串僅讀至第一個 NUL；無 heap、
無輸入改寫、無 I/O。

`tests/test_mcp.c` 與 `tests/test_rust_ffi.c` 覆蓋 NULL、accepted／rejected mode、first-NUL 與 direct
public ABI。已通過 `clang-format --dry-run`（新增 CU）、`cargo fmt --all -- --check`、
`cargo test -p cbm-core trace_mode_edge_mask --locked`（**1 passed**）、`cargo clippy -p cbm-core
--all-targets --features mcp-trace-mode-edge-mask-only --locked -- -D warnings`，以及
`make -j1 -f Makefile.cbm rust-mcp-trace-mode-edge-mask-{optin,only}-test` 的所有 default/wrapper/direct
legs。MCP 均 **145 passed**、production `--version` 均成功；ONLY 的
`MCP_TRACE_MODE_EDGE_MASK_SRCS =`、`make -Bn` 排除 `trace_mode_edge_mask.c` 並保留 `mcp.c` 亦成功。完整
`scripts/test.sh` 未跑。

### 2026-07-19 第 46 項：MCP adr_mode true-source

`handle_manage_adr()` 的 mode classifier 已拆為 C fallback `src/mcp/adr_mode.c/.h`；`mcp.c` 保留
project／Store resolution、legacy migration、ADR read/write、sections JSON、ownership 與 transport
orchestration。公開 ABI：`int cbm_mcp_adr_mode(const char *input)`。wrapper
`CBM_USE_RUST_MCP_ADR_MODE=1`（亦相容 `CBM_USE_RUST_MCP_CODEC=1`）委派既有 v1；direct
`CBM_USE_RUST_MCP_ADR_MODE_ONLY=1`／Cargo `mcp-adr-mode-only` 由 Rust 匯出同名 ABI。

契約已凍結：NULL、空字串、`get`、未知、大小寫不符與尾端空白回 0（get/default）；`update`、`store` 回 1；
`sections` 回 2。字串僅讀至第一個 NUL；無 heap、無輸入改寫、無 I/O。

`tests/test_mcp.c` 與 `tests/test_rust_ffi.c` 覆蓋 NULL、accepted／rejected mode、first-NUL 與 direct
public ABI。已通過 `clang-format --dry-run`（新增 CU）、`cargo fmt --all -- --check`、
`cargo test -p cbm-core adr_mode --locked`（**1 passed**）、`cargo clippy -p cbm-core --all-targets
--features mcp-adr-mode-only --locked -- -D warnings`，以及
`make -j1 -f Makefile.cbm rust-mcp-adr-mode-{optin,only}-test` 的所有 default/wrapper/direct legs。MCP 均
**144 passed**、production `--version` 均成功；ONLY 的 `MCP_ADR_MODE_SRCS =`、`make -Bn` 排除
`adr_mode.c` 並保留 `mcp.c` 亦成功。完整 `scripts/test.sh` 未跑。

### 2026-07-19 第 45 項：MCP utf8_is_cont_byte true-source

`sanitize_utf8_lossy.c` fallback loop 的 continuation-byte classifier 已拆為 C fallback
`src/mcp/utf8_is_cont.c/.h`；UTF-8 scan、replacement-byte 寫入與 buffer ownership 在
`src/mcp/sanitize_utf8_lossy.c`，`mcp.c` 保留 JSON 與 transport orchestration。公開 ABI：
`bool cbm_mcp_utf8_is_cont_byte(int byte)`。wrapper
`CBM_USE_RUST_MCP_UTF8_IS_CONT=1`（亦相容 `CBM_USE_RUST_MCP_CODEC=1`）委派既有 v1；direct
`CBM_USE_RUST_MCP_UTF8_IS_CONT_ONLY=1`／Cargo `mcp-utf8-is-cont-only` 由 Rust 匯出同名 ABI。

契約已凍結：只檢查輸入的低 8 bits；值落在 `0x80..0xBF`（`10xxxxxx`）回 true，其餘回 false。負數與
超過 byte 範圍的輸入同樣先截取低 8 bits；無 heap、無輸入改寫、無 I/O。

`tests/test_mcp.c` 與 `tests/test_rust_ffi.c` 覆蓋兩端範圍、非 continuation、負數與高位 byte mask，以及
direct public ABI。已通過 `clang-format --dry-run`（新增 CU）、`cargo fmt --all -- --check`、
`cargo test -p cbm-core utf8_is_cont_byte --locked`（**1 passed**）、`cargo clippy -p cbm-core --all-targets
--features mcp-utf8-is-cont-only --locked -- -D warnings`，以及
`make -j1 -f Makefile.cbm rust-mcp-utf8-is-cont-{optin,only}-test` 的所有 default/wrapper/direct legs。MCP
均 **143 passed**、production `--version` 均成功；ONLY 的 `MCP_UTF8_IS_CONT_SRCS =`、`make -Bn` 排除
`utf8_is_cont.c` 並保留 `mcp.c` 亦成功。完整 `scripts/test.sh` 未跑。

### 2026-07-19 第 44 項：MCP node_resolution_score true-source

`pick_resolved_node()` 的 per-node scalar scorer 已拆為 C fallback
`src/mcp/node_resolution_score.c/.h`；`mcp.c` 保留候選 query、ambiguity、BFS、JSON 與 transport
orchestration。公開 ABI：
`long cbm_mcp_node_resolution_score(const char *label, int start_line, int end_line)`。wrapper
`CBM_USE_RUST_MCP_NODE_RESOLUTION_SCORE=1`（亦相容 `CBM_USE_RUST_MCP_CODEC=1`）委派既有 v1；direct
`CBM_USE_RUST_MCP_NODE_RESOLUTION_SCORE_ONLY=1`／Cargo `mcp-node-resolution-score-only` 由 Rust 匯出同名 ABI。

契約已凍結：Function／Method 為 tier 2，Module／File 與 NULL 為 tier 0，其餘 label（包含空字串）為 tier 1；
回傳 tier * 1000000 加上 `max(end_line - start_line, 0)`。label 僅讀至第一個 NUL；無 heap、無輸入改寫、無 I/O。

`tests/test_mcp.c` 與 `tests/test_rust_ffi.c` 覆蓋 NULL、empty、Function／Method、class-like、Module／File、
negative span、大小寫與 first-NUL，以及 direct public ABI。已通過 `clang-format --dry-run`（新增 CU）、
`cargo fmt --all -- --check`、`cargo test -p cbm-core node_resolution_score --locked`（**1 passed**）、
`cargo clippy -p cbm-core --all-targets --features mcp-node-resolution-score-only --locked -- -D warnings`，以及
`make -j1 -f Makefile.cbm rust-mcp-node-resolution-score-{optin,only}-test` 的所有 default/wrapper/direct
legs。MCP 均 **142 passed**、production `--version` 均成功；ONLY 的
`MCP_NODE_RESOLUTION_SCORE_SRCS =`、`make -Bn` 排除 `node_resolution_score.c` 並保留 `mcp.c` 亦成功。
完整 `scripts/test.sh` 未跑。

### 2026-07-19 第 43 項：MCP detect_changes_status_path_offset true-source

`handle_detect_changes()` 的 status line parser 已拆為 C fallback
`src/mcp/detect_changes_status.c/.h`；`mcp.c` 保留 line loop、changed-file JSON、Store lookup 與 transport
orchestration。公開 ABI：`size_t cbm_mcp_detect_changes_status_path_offset(const char *line)`。wrapper
`CBM_USE_RUST_MCP_DETECT_CHANGES_STATUS=1`（亦相容 `CBM_USE_RUST_MCP_CODEC=1`）委派既有 v1；direct
`CBM_USE_RUST_MCP_DETECT_CHANGES_STATUS_ONLY=1`／Cargo `mcp-detect-changes-status-only` 由 Rust 匯出同名 ABI。

契約已凍結：NULL、空字串或最終 path 空回 `SIZE_MAX`；porcelain `XY ` 跳過 3 bytes，rename 取
`" -> "` 後 destination offset；bare／unknown prefix path 回 0。line 僅讀至第一個 NUL；無 heap、無輸入改寫、
無 I/O。

`tests/test_mcp.c` 與 `tests/test_rust_ffi.c` 覆蓋 NULL、empty、bare、porcelain、rename、空 destination、
unknown prefix、first-NUL 與 direct public ABI。已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core
detect_changes_status_path_offset --locked`（**1 passed**）、`cargo clippy -p cbm-core --all-targets --features
mcp-detect-changes-status-only --locked -- -D warnings`，以及
`make -j1 -f Makefile.cbm rust-mcp-detect-changes-status-{optin,only}-test`。兩個 Make gate 的
default/wrapper/direct FFI、mcp 均 **141 passed**、production `--version` 均成功；ONLY 的
`MCP_DETECT_CHANGES_STATUS_SRCS =`、`make -Bn` 排除 `detect_changes_status.c` 並保留 `mcp.c` 亦成功。
完整 `scripts/test.sh` 未跑。

### 2026-07-19 第 42 項：MCP detect_changes_impacted_label true-source

`detect_add_impacted_symbols()` 的 label filter 已拆為 C fallback
`src/mcp/detect_changes_label.c/.h`；`mcp.c` 保留 Store lookup、node ownership、JSON 與 transport
orchestration。公開 ABI：`bool cbm_mcp_detect_changes_impacted_label(const char *label)`。wrapper
`CBM_USE_RUST_MCP_DETECT_CHANGES_LABEL=1`（亦相容 `CBM_USE_RUST_MCP_CODEC=1`）委派既有 v1；direct
`CBM_USE_RUST_MCP_DETECT_CHANGES_LABEL_ONLY=1`／Cargo `mcp-detect-changes-label-only` 由 Rust 匯出同名 ABI。

契約已凍結：NULL、`File`、`Folder`、`Project` 回 false；空字串與所有其他值回 true。label 僅讀至
第一個 NUL；無 heap、無輸入改寫、無 I/O。

`tests/test_mcp.c` 與 `tests/test_rust_ffi.c` 覆蓋 NULL、全部排除 label、保留 label、first-NUL 與 direct
public ABI。已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core detect_changes_impacted_label
--locked`（**1 passed**）、`cargo clippy -p cbm-core --all-targets --features
mcp-detect-changes-label-only --locked -- -D warnings`，以及
`make -j1 -f Makefile.cbm rust-mcp-detect-changes-label-{optin,only}-test`。兩個 Make gate 的
default/wrapper/direct FFI、mcp 均 **140 passed**、production `--version` 均成功；ONLY 的
`MCP_DETECT_CHANGES_LABEL_SRCS =`、`make -Bn` 排除 `detect_changes_label.c` 並保留 `mcp.c` 亦成功。
完整 `scripts/test.sh` 未跑。

### 2026-07-19 第 41 項：MCP detect_changes_wants_symbols true-source

`handle_detect_changes()` 的 scope classifier 已拆為 C fallback
`src/mcp/detect_changes_scope.c/.h`；`mcp.c` 保留 project/root resolution、git diff/status、Store lookup、
JSON 與 transport orchestration。公開 ABI：
`bool cbm_mcp_detect_changes_wants_symbols(const char *scope)`。wrapper
`CBM_USE_RUST_MCP_DETECT_CHANGES_SCOPE=1`（亦相容 `CBM_USE_RUST_MCP_CODEC=1`）委派既有 v1；direct
`CBM_USE_RUST_MCP_DETECT_CHANGES_SCOPE_ONLY=1`／Cargo `mcp-detect-changes-scope-only` 由 Rust 匯出同名 ABI。

契約已凍結：NULL、`symbols`、`impact` 回 true；空字串、`files`、大小寫不符、尾端空白與其他值回 false。
scope 僅讀至第一個 NUL；無 heap、無輸入改寫、無 I/O。

`tests/test_mcp.c` 與 `tests/test_rust_ffi.c` 覆蓋 NULL、所有 accepted/rejected scope、first-NUL 與 direct
public ABI。已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core detect_changes_wants_symbols
--locked`（**1 passed**）、`cargo clippy -p cbm-core --all-targets --features
mcp-detect-changes-scope-only --locked -- -D warnings`，以及
`make -j1 -f Makefile.cbm rust-mcp-detect-changes-scope-{optin,only}-test`。兩個 Make gate 的
default/wrapper/direct FFI、mcp 均 **139 passed**、production `--version` 均成功；ONLY 的
`MCP_DETECT_CHANGES_SCOPE_SRCS =`、`make -Bn` 排除 `detect_changes_scope.c` 並保留 `mcp.c` 亦成功。
完整 `scripts/test.sh` 未跑。

### 2026-07-19 第 40 項：MCP search_top_dir true-source

`build_dir_distribution()` 的 top-level key helper 已拆為 C fallback
`src/mcp/search_top_dir.c/.h`；`mcp.c` 保留 directory count、`yyjson`、`search_result_t` ownership、
grep、node lookup、dedup、結果 JSON 與 transport orchestration。公開 ABI：
`size_t cbm_mcp_search_top_dir(char *buf, size_t bufsize, const char *file)`。wrapper
`CBM_USE_RUST_MCP_SEARCH_TOP_DIR=1`（亦相容 `CBM_USE_RUST_MCP_CODEC=1`）委派既有 v1；direct
`CBM_USE_RUST_MCP_SEARCH_TOP_DIR_ONLY=1`／Cargo `mcp-search-top-dir-only` 由 Rust 匯出同名 ABI。

契約已凍結：`file == NULL` 回 `SIZE_MAX` 且不改寫 `buf`；成功時回完整 key 長度，key 包含第一個
`/`，沒有 slash 則為完整 file。`buf == NULL` 或 `bufsize == 0` 只回長度；短 buffer 最多寫
`bufsize - 1` bytes 並 NUL-terminate。file 僅讀至第一個 NUL；無 heap、無輸入改寫、無 I/O。

`tests/test_mcp.c` 與 `tests/test_rust_ffi.c` 覆蓋 NULL、length-only、zero buffer、短 buffer、first-NUL、
空字串與 direct public ABI。已通過 `cargo fmt --all -- --check`、
`cargo test -p cbm-core search_top_dir --locked`（**1 passed**）、`cargo clippy -p cbm-core
--all-targets --features mcp-search-top-dir-only --locked -- -D warnings`，以及
`make -j1 -f Makefile.cbm rust-mcp-search-top-dir-{optin,only}-test`。兩個 Make gate 的
default/wrapper/direct FFI、mcp 均 **138 passed**、production `--version` 均成功；ONLY 的
`MCP_SEARCH_TOP_DIR_SRCS =`、`make -Bn` 排除 `search_top_dir.c` 並保留 `mcp.c` 亦成功。
完整 `scripts/test.sh` 未跑。

### 2026-07-19 第 39 項：MCP search_code_score true-source

`compute_search_score()` 已拆為 C fallback `src/mcp/search_code_score.c/.h`；`mcp.c` 保留
`search_result_t`、dedup、qsort、grep、node lookup、結果 ownership 與 JSON orchestration。公開 ABI：
`int cbm_mcp_search_code_score(const char *label, const char *file, int in_degree)`。wrapper
`CBM_USE_RUST_MCP_SEARCH_CODE_SCORE=1`（亦相容 `CBM_USE_RUST_MCP_CODEC=1`）委派既有 v1；direct
`CBM_USE_RUST_MCP_SEARCH_CODE_SCORE_ONLY=1`／Cargo `mcp-search-code-score-only` 由 Rust 匯出同名 ABI。

契約已凍結：label／file 可為 NULL，NULL 不套用該側加權；字串只讀到第一個 NUL。以 `in_degree`
為基底，Function／Method +10、Route +15，file 含 vendored/、vendor/、node_modules/ -50，含 test、
spec、_test. -5；條件可累加。無 heap、無輸入改寫、無排序或 I/O；既有 `int` 可表示範圍的加權
算術保持不變。

`tests/test_mcp.c` 與 `tests/test_rust_ffi.c` 覆蓋 NULL、label、path penalty 疊加、first-NUL 與
direct public ABI。已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core search_code_score
--locked`（**1 passed**）、`cargo clippy -p cbm-core --all-targets --features
mcp-search-code-score-only --locked -- -D warnings`，以及
`make -j1 -f Makefile.cbm rust-mcp-search-code-score-{optin,only}-test`。兩個 Make gate 的
default/wrapper/direct FFI、mcp 均 **137 passed**、production `--version` 均成功；ONLY 的
`MCP_SEARCH_CODE_SCORE_SRCS =`、`make -Bn` 排除 `search_code_score.c` 並保留 `mcp.c` 亦成功。
完整 `scripts/test.sh` 未跑。

### 2026-07-19 第 38 項：pipeline pkgmap text true-source

已由 `pass_pkgmap.c` 拆出 C fallback `src/pipeline/pkgmap_text.c/.h`；pass 保留 manifest parsing、
檔案 I/O、package entry ownership 與圖形解析 orchestration。公開 ABI 為 bounded byte compare
`cbm_pipeline_pkgmap_at_prefix`、借用 source 的
`cbm_pipeline_pkgmap_find_line_value`，以及 `path_dirname`、`strip_extension`、`join_and_strip`、
`build_entry_path` 四個 `cbm_pipeline_pkgmap_*` heap path helper。wrapper
`CBM_USE_RUST_PIPELINE_PKGMAP_TEXT=1` 委派既有 v1；direct
`CBM_USE_RUST_PIPELINE_PKGMAP_TEXT_ONLY=1`／Cargo `pipeline-pkgmap-text-only` 由 Rust 匯出同名
public ABI。

契約已凍結：bounded scan 的 NULL／負長度回 false 或 NULL，line lookup 成功值只在 `source` 存活期間
可使用，且 source 可含 bounded NUL byte；路徑輸入採 first-NUL raw bytes，NULL path 的 dirname／stem
回配置空字串，NULL／空 entry 的 join 回 NULL，NULL dir 視為空字串。四個 heap 結果均可由 caller
`free()`；join/build 沿用既有 1023-byte 組裝上限並於 strip extension 前截斷。

`tests/test_pipeline.c` 與 `tests/test_rust_ffi.c` 覆蓋 NULL、borrowed offset、非 ASCII byte、
first-NUL、buffer truncate、1023-byte 截斷與 caller-free。已通過 `cargo fmt --all -- --check`、
`cargo test -p cbm-core pipeline_pkgmap --locked`（**4 passed**）、
`cargo clippy -p cbm-core --all-targets --features pipeline-pkgmap-text-only --locked -- -D warnings`、
以及 `make -j1 -f Makefile.cbm rust-pipeline-pkgmap-text-{optin,only}-test`。兩個 Make gate 的
default/wrapper/direct FFI、pipeline 均 **235 passed**、production `--version` 均成功；ONLY 的
`PIPELINE_PKGMAP_TEXT_SRCS =`、`make -Bn` 排除 `pkgmap_text.c` 並保留 `pass_pkgmap.c` 亦成功。
完整 `scripts/test.sh` 未跑。

### 2026-07-19 第 37 項：pipeline enrichment tokens true-source

已由 `pass_enrichment.c` 拆出 C fallback `src/pipeline/enrichment_tokens.c/.h`；pass 僅保留
decorator tag 的圖形收集、word frequency、yyjson 注入與 store orchestration。公開 ABI 為
`int cbm_split_camel_case(const char *, char **, int)` 與
`int cbm_tokenize_decorator(const char *, char **, int)`；wrapper
`CBM_USE_RUST_PIPELINE_ENRICHMENT_TOKENS=1` 委派既有 v1，direct
`CBM_USE_RUST_PIPELINE_ENRICHMENT_TOKENS_ONLY=1`／Cargo
`pipeline-enrichment-tokens-only` 由 Rust 匯出相同 public ABI。

契約已凍結：NULL input、NULL out 或 `max_out <= 0` 回 0 且不改寫 out；輸入採 raw-byte
first-NUL。camel 僅在 ASCII 小寫→大寫邊界切分。decorator 最多讀前 255 bytes，處理開頭 `@` 與
`#[`（僅末尾 `]` 才移除），捨棄第一個 `(` 後內容，以 `.`、`_`、`-`、`:`、`/` 分隔，並只將
ASCII `A`–`Z` lower-case；非 ASCII byte 原樣保留。正常資源條件下 token 由 C `malloc` 配置，
caller 逐一 `free()`；OOM 不在 C/Rust parity 保證範圍。

已新增 `PIPELINE_ENRICHMENT_TOKENS_SRCS` selector 與 `RUST_FFI_SUPPORT_SRCS` 接線；ONLY 時
selector 為空、`make -Bn` 排除 `enrichment_tokens.c` 且保留 `pass_enrichment.c`。`tests/test_pipeline.c`
與 `tests/test_rust_ffi.c` 覆蓋 NULL／非正 max、capacity 截斷與 sentinel、first-NUL、255-byte
截斷、非 ASCII byte 及 public ABI ownership。

已通過 `cargo fmt --all -- --check`、`cargo test -p cbm-core pipeline_enrichment --locked`
（**5 passed**）、切片 `cargo clippy -p cbm-core --all-targets --features
pipeline-enrichment-tokens-only --locked -- -D warnings`、以及
`make -j1 -f Makefile.cbm rust-pipeline-enrichment-tokens-optin-test` 與
`make -j1 -f Makefile.cbm rust-pipeline-enrichment-tokens-only-test`。兩個 Make gate 的
default/wrapper/direct FFI、pipeline 均為 **233 passed**、production `--version` 均成功；direct
empty selector 與 source-negative／orchestration 正向檢查亦成功。完整 all-features clippy 仍被既有
`pipeline-calls-json-only` 與 `pipeline-usages-json-only` 的重複 `malloc` 宣告阻擋，未擴大修正。
完整 `scripts/test.sh` 未跑。

### 下一個候選

尚未固定下一個切片；先以唯讀 inventory 尋找能維持窄 ABI 與 direct source replacement 的候選。

**明確延後：** `pipeline-complexity-json`（strtol 契約需人類決策後才能 true-source）。

### 建議接手命令

~~~sh
# 1) 無殘留 build
ps -axo pid,ppid,stat,command | \
  rg '(^|[[:space:]])make( [^[:space:]]+)* -f Makefile\.cbm|cargo (test|clippy|build|fmt)' || true

# 2) 確認 #54 的 fallback、feature 與 gate 定義存在
ls src/mcp/architecture_aspect_wanted.c src/mcp/architecture_aspect_wanted.h
rg -n 'mcp-architecture-aspect-wanted-only|ARCHITECTURE_ASPECT_WANTED' \
  rust/cbm-core/Cargo.toml Makefile.cbm

# 3) #54 尚無完整 gate 證據；確認沒有競爭中的 clean-c Make 後，固定依序重跑
make -j1 -f Makefile.cbm rust-mcp-architecture-aspect-wanted-optin-test
make -j1 -f Makefile.cbm rust-mcp-architecture-aspect-wanted-only-test

# 4) 兩個 gate 都成功後，才可檢查 direct selector／source-negative 並考慮更新 true-source 計數
make -j1 -pn -f Makefile.cbm CBM_USE_RUST_MCP_ARCHITECTURE_ASPECT_WANTED_ONLY=1 | \
  rg '^MCP_ARCHITECTURE_ASPECT_WANTED_SRCS = $'

# 5) #54 收尾前不得開始下一候選 inventory；#29–#53 也不得重跑
~~~

### 文件權威順序

1. **本檔** `docs/rust-refactor-current-handoff.md`（停點／下一步／命令）
2. `Handoff.md` 頂端、`Tasks.md` 頂端、`Rust-Refactor.md` 頂端
3. `rust/CBM_FFI.md`（ABI／ownership 權威；#29–#53 與 #54 WIP 詳見檔末切片段落 + 頂端總覽）
4. Skill `references/current-state.md`、`known-abi-slices.md`、`validation-gates.md`
5. `docs/` 下同名 Handoff／Tasks／Rust-Refactor **僅歷史快照**，不可單獨當完成度

### 給 Coding Agent 的一句起手提示

「請使用 CodebaseMemMcp-Refactory-rust skill 接續本 repo。先讀
docs/rust-refactor-current-handoff.md 與根目錄 Handoff／Tasks／Rust-Refactor／rust/CBM_FFI.md。
true-source **48／48**；dirty worktree 為預期（#29–#53 未 commit，#54 WIP），不得 reset／clean。
不得重跑 #29–#53；complexity 延後。先收尾 #54 `architecture_aspect_wanted`：重跑並保存其 optin 與 ONLY
gate 的完整成功證據，才可考慮列為 true-source；完成前不得開始下一候選。clean-c gate 一律 `make -j1` 序列。
不執行 git 與全量 scripts/test.sh（除非使用者明示）。」

---

## 2026-07-18 第 35–36 項：MCP search_args + search_score_cmp true-source

本輪完成兩個 MCP pure-helper true-source，累計由 **29 / 29 -> 31 / 31**。
未重開 #29–#34；`pipeline-complexity-json` 仍延後。

### 第 35 項：MCP search_args_valid

C fallback：`src/mcp/search_args.c/.h`；公開
`bool cbm_mcp_search_args_valid(const char *root, const char *file_pattern)`。
root 必填且通過 path_arg；file_pattern 可 NULL。C fallback 呼叫
`cbm_mcp_search_path_arg_valid` 避免 denylist 雙份。wrapper
`CBM_USE_RUST_MCP_SEARCH_ARGS`（相容 CODEC）；direct `…_ONLY`／`mcp-search-args-only`。
Make：`rust-mcp-search-args-optin-test`、`rust-mcp-search-args-only-test`。
mcp 各 **136 passed**；direct 空 selector、bn 不含 `search_args.c`、仍含 `mcp.c`。

### 第 36 項：MCP search_score_cmp

C fallback：`src/mcp/search_score_cmp.c/.h`；公開
`int cbm_mcp_search_score_cmp(int left, int right)` → `right - left`（descending）。
qsort 仍在 mcp.c。wrapper `CBM_USE_RUST_MCP_SEARCH_SCORE_CMP`（相容 CODEC）；direct
`…_ONLY`／`mcp-search-score-cmp-only`。
Make：`rust-mcp-search-score-cmp-optin-test`、`rust-mcp-search-score-cmp-only-test`。
mcp 各 **136 passed**；direct 空 selector、bn 不含 `search_score_cmp.c`、仍含 `mcp.c`。

fmt／unit／clippy 通過。完整 `scripts/test.sh` 未跑。

> 最新停點與下一步見文件頂端「給下一個 Session 的起手式」。

## 2026-07-18 第 33–34 項：MCP search_path_arg + search_line_span true-source

本輪完成兩個 MCP pure-classifier true-source，累計由 **27 / 27 -> 29 / 29**。
未重開 #29–#32；`pipeline-complexity-json` 仍延後。

### 第 33 項：MCP search_path_arg_valid

C fallback：`src/mcp/search_path_arg.c/.h`；公開
`bool cbm_mcp_search_path_arg_valid(const char *)`。
NULL→false；空→true；拒 `'";|$`<>` CR/LF（非 Windows 另拒 `\`）；接受 space 與 `&`。
wrapper `CBM_USE_RUST_MCP_SEARCH_PATH_ARG`（相容 CODEC）；direct `…_ONLY`／
`mcp-search-path-arg-only`。mcp 各 **136 passed**；direct 空 selector、`make -Bn` 不含
`search_path_arg.c`、仍含 `mcp.c`。

### 第 34 項：MCP search_line_match_span

C fallback：`src/mcp/search_line_span.c/.h`；公開
`int cbm_mcp_search_line_match_span(int start, int end, int line)`。
命中→`end-start`；未命中→`-1`。`find_tightest_node` 迴圈／pick 仍在 `mcp.c`。
wrapper `CBM_USE_RUST_MCP_SEARCH_LINE_SPAN`（相容 CODEC）；direct `…_ONLY`／
`mcp-search-line-span-only`。mcp 各 **136 passed**；direct 空 selector、`make -Bn` 不含
`search_line_span.c`、仍含 `mcp.c`。

fmt／unit／clippy 通過。完整 `scripts/test.sh` 未跑。

> 後續 #35–#36 已完成；最新停點見文件頂端。

## 2026-07-18 第 32 項：MCP index_mode true-source

MCP index_mode 已完成，true-source 累計由 **26 / 26 -> 27 / 27**。未重開 #29–#31；
`pipeline-complexity-json` 仍延後（strtol locale／overflow／NULL key）。

C fallback 已從 `src/mcp/mcp.c` 的 static `parse_index_repository_mode` 拆至
`src/mcp/index_mode.c/.h`；handler／pipeline／transport 仍在 `mcp.c`。

公開 C ABI：

    int cbm_mcp_index_mode(const char *mode_str);

契約：`moderate`→1、`fast`→2、`cross-repo-intelligence`→3（dispatch sentinel）；
NULL／空／`full`／未知／大小寫不符／尾端空白→0（full/default）。byte-exact strcmp。

wrapper：`CBM_USE_RUST_MCP_INDEX_MODE` 或既有 `CBM_USE_RUST_MCP_CODEC` 委派
`cbm_rs_mcp_index_mode_v1`；direct Cargo `mcp-index-mode-only`／
`CBM_USE_RUST_MCP_INDEX_MODE_ONLY` 匯出同名 public ABI。selector
`MCP_INDEX_MODE_SRCS` 在 ONLY 為空。

Make：`rust-mcp-index-mode-optin-test`、`rust-mcp-index-mode-only-test`。
default/wrapper/direct FFI + mcp 各 **136 passed** + production `--version`。
direct：`MCP_INDEX_MODE_SRCS =`、`make -Bn` 不含 `index_mode.c`，仍含 `mcp.c`。
fmt、`index_mode` unit、clippy 通過。

**下一個 session：** 不得重跑 #29–#32。ready 候選（inventory）：search_path_arg_valid、
enrichment tokens、pkgmap text、cross-repo JSON。complexity 仍延後。完整 `scripts/test.sh` 未跑。

## 2026-07-17 第 30–31 項：pipeline-usages-json + MCP search_mode true-source

本輪同時完成兩個 true-source，累計由 **24 / 24 -> 26 / 26**。未重開 #29；
`pipeline-complexity-json` 仍延後（strtol locale／overflow／NULL key）。

### 第 30 項：Pipeline usages-json（同形 #29）

C fallback 已從 `src/pipeline/pass_usages.c` 拆至 `src/pipeline/usages_json.c/.h`；
import map／graph／edge ownership 與 orchestration 仍在 `pass_usages.c`。

公開 C ABI：

    char *cbm_pipeline_usages_extract_local_name(const char *props_json);

契約：NULL／缺鍵／空 value → NULL；成功為 C `malloc` 字串（caller `free`）；raw-byte
first-NUL，無 JSON unescape。wrapper 經既有 v1
`cbm_rs_pipeline_usages_local_name_{len,copy}_v1`；direct Cargo
`pipeline-usages-json-only`／`CBM_USE_RUST_PIPELINE_USAGES_JSON_ONLY` 匯出同名 public ABI
（C malloc）。selector `PIPELINE_USAGES_JSON_SRCS` 在 ONLY 為空。

Make：`rust-pipeline-usages-json-optin-test`、`rust-pipeline-usages-json-only-test`。
default/wrapper/direct FFI + pipeline 各 **231 passed** + production `--version`。
direct：`PIPELINE_USAGES_JSON_SRCS =`、`make -Bn` 不含 `usages_json.c`，仍含
`pass_usages.c`。fmt、`pipeline_usages` unit **1 passed**、clippy 通過。

### 第 31 項：MCP search_mode

C fallback 已從 `src/mcp/mcp.c` 的 static `parse_search_mode` 拆至
`src/mcp/search_mode.c/.h`；handler／grep／response／transport 仍在 `mcp.c`（不得 ONLY
排除整個 mcp.c）。

公開 C ABI：

    int cbm_mcp_search_mode(const char *mode_str);

契約：`full`→1、`files`→2；NULL／空／`compact`／未知／大小寫不符／尾端空白→0。
wrapper：`CBM_USE_RUST_MCP_SEARCH_MODE` 或既有 `CBM_USE_RUST_MCP_CODEC` 委派
`cbm_rs_mcp_search_mode_v1`；direct Cargo `mcp-search-mode-only`／
`CBM_USE_RUST_MCP_SEARCH_MODE_ONLY` 匯出同名 public ABI。selector
`MCP_SEARCH_MODE_SRCS` 在 ONLY 為空。

Make：`rust-mcp-search-mode-optin-test`、`rust-mcp-search-mode-only-test`。
default/wrapper/direct FFI + mcp 各 **136 passed** + production `--version`。
direct：`MCP_SEARCH_MODE_SRCS =`、`make -Bn` 不含 `search_mode.c`，仍含 `mcp.c`。
fmt、`search_mode` unit、clippy 通過。

**下一個 session：** 不得重跑 #29–#31；可選其他 pure helper inventory。  
`pipeline-complexity-json` 仍延後（strtol locale 契約）。完整 `scripts/test.sh` 未跑。

## 2026-07-17 第 29 項 Pipeline calls-json true-source

pipeline-calls-json 已完成，true-source 累計由 **23 / 23 -> 24 / 24**。C fallback 已從
`src/pipeline/pass_calls.c` 拆至 `src/pipeline/calls_json.c/.h`；原檔仍保留 import map、
CALLS edge、registry resolution 與 orchestration，未遷移整個 pass。

公開 C ABI：

    char *cbm_pipeline_calls_extract_local_name(const char *props_json);

契約：`props_json == NULL`、缺少 `"local_name":"..."` 或空 value → `NULL`；成功回傳 C
`malloc` 字串（first-NUL），caller 必須 `free()`。僅 raw byte 掃描，不做 UTF-8／JSON
unescape。v1 length-query ABI（`cbm_rs_pipeline_calls_extract_local_name_v1`）保留給
wrapper：`SIZE_MAX` 表示失敗。

`CBM_USE_RUST_PIPELINE_CALLS_JSON=1` wrapper 經 C fallback CU 委派 v1；
`CBM_USE_RUST_PIPELINE_CALLS_JSON_ONLY=1`／Cargo `pipeline-calls-json-only` 時 Rust staticlib
匯出同名 direct `cbm_pipeline_calls_extract_local_name`（C `malloc` ownership）。

Make gate：`rust-pipeline-calls-json-optin-test`、`rust-pipeline-calls-json-only-test`。
default/wrapper/direct FFI + pipeline 各 **231 passed** + production `--version`。direct 確認
`PIPELINE_CALLS_JSON_SRCS =`、`make -Bn` 不含 `src/pipeline/calls_json.c`，且 recipe 仍含
`src/pipeline/pass_calls.c`。`RUST_FFI_SUPPORT_SRCS` 已納入 selector。

已通過：`cargo fmt`、`pipeline_calls` unit **1 passed**、clippy（feature
`pipeline-calls-json-only`）。

> 最新停點見文件頂端（#30–#31 已完成）。
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

## 2026-07-20 MCP helpers true-source slices (#60–#64)

完成 5 個 MCP helpers true-source 切片：
- #60 MCP `tools_page_bounds` (`src/mcp/tools_page_bounds.c/.h`，ABI `cbm_mcp_tools_page_bounds`，wrapper `CBM_USE_RUST_MCP_TOOLS_PAGE_BOUNDS`，direct `mcp-tools-page-bounds-only`)
- #61 MCP `content_length_header_matches` (`src/mcp/content_length_header_matches.c/.h`，ABI `cbm_mcp_content_length_header_matches`，wrapper `CBM_USE_RUST_MCP_CONTENT_LENGTH_HEADER_MATCHES`，direct `mcp-content-length-header-matches-only`)
- #62 MCP `content_length_header_is_blank` (`src/mcp/content_length_header_is_blank.c/.h`，ABI `cbm_mcp_content_length_header_is_blank`，wrapper `CBM_USE_RUST_MCP_CONTENT_LENGTH_HEADER_IS_BLANK`，direct `mcp-content-length-header-is-blank-only`)
- #63 MCP `ping_result` (`src/mcp/ping_result.c/.h`，ABI `cbm_mcp_ping_result`，wrapper `CBM_USE_RUST_MCP_PING_RESULT`，direct `mcp-ping-result-only`)
- #64 MCP `method_not_found_error` (`src/mcp/method_not_found_error.c/.h`，ABI `cbm_mcp_method_not_found_error`，wrapper `CBM_USE_RUST_MCP_METHOD_NOT_FOUND_ERROR`，direct `mcp-method-not-found-error-only`)

所有 5 個切片均通過其專屬 optin 與 ONLY Makefile test gates（`rust-mcp-*-optin-test` & `rust-mcp-*-only-test`），mcp suite 通過數由 156 增加至 **161 passed**。

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
> **2026-07-23 #80 權威交接更新：本區塊覆蓋下方所有較早的 current-task、基線與下一步敘述。**
> true-source 累計為 **75 / 75**，已完成 **#29–#80**；不得重跑或改寫 **#29–#80**。
> 產品預設仍為 C11，Rust 僅透過明確 opt-in 或 direct-only 接管單一 pure helper；不可宣稱全庫 Rust default 或 Phase 5 RC。

## 2026-07-23 第 80 項：pipeline-similarity-file-ext true-source

| # | 切片 | C fallback CU | Public ABI | Wrapper flag | Direct feature | Host 與唯一 consumer | Suite |
|---|------|---------------|------------|--------------|----------------|----------------------|-------|
| 80 | pipeline similarity file extension | `src/pipeline/similarity_file_ext.c/.h` | `const char *cbm_pipeline_similarity_file_ext(const char *path)` | `CBM_USE_RUST_PIPELINE_SIMILARITY_FILE_EXT` | `pipeline-similarity-file-ext-only` | `src/pipeline/pass_similarity.c`；`collect_fp_entries`，結果流入 similarity comparison | pipeline **238** |

凍結 Rust v1 ABI：`const char *cbm_rs_pipeline_similarity_file_ext_v1(const char *path);`。`path == NULL` 時必須回傳
非 `NULL`、process-lifetime 的靜態 NUL 空字串，且不得解參。非 `NULL` 時沿用既有「有效、NUL 終止 C 字串」
precondition，只按第一個 NUL 前的 bytes 運作；尋找最後一個 ASCII `.`，不把 `/` 或 `\\` 視為分隔符，非 UTF-8
bytes 亦有效。成功時必須回傳原始 `path + offset` 的 borrowed pointer；無 dot 或空輸入回靜態空字串，該空字串
在不同 mode 間不保證 pointer identity。此 helper 不配置、不修改輸入、不做 I/O、不使用 locale、不保存 pointer，
也不轉移 ownership。

代表性邊界為：`.gitignore` 回輸入起點、`file.` 回指向 `"."`、`dir.name/file` 回 `".name/file"`、fullwidth dot
不是 ASCII dot、embedded NUL 後的 bytes 不參與搜尋。C fallback 已由 `pass_similarity.c` 拆至
`src/pipeline/similarity_file_ext.c/.h`，host 與 `collect_fp_entries` 仍保留在 `src/pipeline/pass_similarity.c`。
wrapper `CBM_USE_RUST_PIPELINE_SIMILARITY_FILE_EXT` 呼叫 v1，僅 v1 回 `NULL` 時回退 C；direct
`CBM_USE_RUST_PIPELINE_SIMILARITY_FILE_EXT_ONLY` 搭配 Cargo `pipeline-similarity-file-ext-only`，省略 C fallback，
並由 Rust 匯出相同 public ABI。

完整成功 gate：`cargo fmt --all -- --check`；Rust targeted unit **1 passed，339 filtered**；direct feature
`cargo clippy -D warnings`；default／wrapper／direct FFI；pipeline 三態各 **238 passed**；production `--version`
三次均為 `codebase-memory-mcp dev`。direct selector 為空，`make -Bn` source-negative 排除
`src/pipeline/similarity_file_ext.c`、source-positive 保留 `src/pipeline/pass_similarity.c`；direct source-negative
proof 累計為 **17**。完整 `scripts/test.sh` **未執行**。

延後項維持：`pipeline-complexity-json` 與 content_length value／raw 的 `strtol` 語義差異。下一個 **#81** 必須重新
inventory Pipeline／MCP／Store 的 pure helpers，再選擇並凍結最小切片；不得直接假設候選或重跑既有 gate。
