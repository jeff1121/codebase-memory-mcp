# Rust 重構 Phase 3 切分計畫

本文件把 `store`、`cypher`、`mcp` 的 Rust 遷移拆成可驗證切片。Phase 3 的第一步不是直接改寫核心服務，而是先凍結 contract fixture，避免 Rust 實作完成後才發現公開行為或 SQLite 副作用漂移。

## Store：SQLite Contract 優先

現有 `src/store/store.c` 同時負責 schema、pragma、authorizer、UDF、FTS、CRUD、search、BFS、ADR、architecture 與 vector search。第一個 Rust 切片不應整包搬移。

先建立 `tests/test_store_compat.c` 或等價 fixture，固定以下 contract：

- schema/index manifest：第一批先固定核心表、關鍵 generated column 與 user index existence；完整 `sqlite_schema`、`table_xinfo`、index layout 與 FTS shadow object list 需後續補齊。
- index symmetry：`create_user_indexes()` 與 drop path 必須涵蓋同一組 index，包含 `idx_edges_url_path`。
- FTS/BM25：store fixture 固定 `nodes_fts` rebuild/camelCase；MCP fixture 固定 `search_graph` BM25 `file_pattern`、rank/order 與 output fields。
- WAL/readonly：凍結 `open_path`、`open_path_query`、bulk begin/end、checkpoint 的 side effect。
- URL path generated column：固定 `properties.url_path` query 結果。

目前 `tests/test_store_compat.c` 已覆蓋 minimal schema/index existence、`edges.url_path_gen` generated column、user index manifest、drop/create symmetry、`open_path_query` missing DB no-create/read-only/write rejection、WAL journal mode、readonly WAL read/write rejection、generated column query plan、URL path API project-scoped substring 行為，以及 `nodes_fts` manual rebuild/camelCase tokenization；並以此修正 `idx_edges_url_path` 未被 `cbm_store_drop_indexes()` drop 的不對稱。後續仍可補完整 `sqlite_schema` golden、checkpoint/bulk side effect 與 artifact import/export fixture。

先不要遷移 `sqlite_writer`、`graph_buffer` direct writer、readonly fallback、vector tables 或整個 FTS/BM25 stack。這些應各自有 fixture 後再切。

## Cypher：AST 與 Result Golden

現行 `cypher` 不是 SQL emit 架構；`cbm_cypher_execute()` 是 parse 後直接用 store API 執行。因此 Phase 3 的 cypher contract 應鎖定 parser AST、query result 與 exact error，而不是宣稱已有 SQL output golden。

建議先新增 cypher golden case table：

- parser cases：label alternation、relationship direction/type/hops、WHERE precedence、WITH alias/aggregate、OPTIONAL MATCH、UNION。
- query cases：以固定 store fixture 比對 deterministic `columns` 與 `rows`，查詢必要時加 `ORDER BY`。
- error cases：對 CREATE、CALL、unknown function、unsupported EXISTS pattern 等鎖定 exact error message。

目前 `tests/test_cypher_contract.c` 已覆蓋代表性 parser AST shape、OPTIONAL MATCH AST 與 unmatched row、UNION/UNION ALL AST 與 result semantics、edge property 投影/過濾、bare edge JSON、deterministic ordered rows、WITH/post-WHERE AST、explicit `LIMIT 0` columns/rows，以及 unsupported `CREATE`、list indexing 與 unknown function 的 exact error message。後續仍可補 CALL、EXISTS pattern、更多 malformed query 與 aggregation edge cases。

Rust 第一個切口只做 lexer/parser 或 normalized AST helper；executor、aggregation、OPTIONAL MATCH、UNION、computed properties 與 MCP `query_graph` 仍留 C，直到 contract 足夠完整。

## MCP：Codec 與 Transcript 先行

MCP 邊界牽涉 stdout/stderr、JSON-RPC id preservation、tool schema、transport framing、tool handler 與 store/pipeline。第一個 Rust 切片只應針對純 JSON-RPC codec / envelope helper。

在 Rust 實作前先擴充 golden transcript：

- `initialize` numeric id 與 string id。
- `tools/list` 兩頁 pagination 與 14 tools 名稱（第一批已覆蓋 numeric id、原始順序、cursor、key set 與 schema 摘要；string id 待補）。
- unknown method `-32601`、invalid JSON `-32700`、unknown tool 的 `isError=true`（第一批已覆蓋）。
- newline JSON-RPC 與 `Content-Length` framing（第一批已覆蓋 newline tools/list 與 framed ping）。
- `search_graph` BM25 `file_pattern`、label boost rank/order、`total` 與 `has_more`（第一批已覆蓋）。
- indexed repo 後的 `search_graph` 或 `query_graph` 小型 result（第一批已覆蓋 `query_graph` 摘要）。

後續 opt-in 建議命名為 `CBM_USE_RUST_MCP_CODEC=1`，成功標準是預設 C path 與 opt-in 都通過 `tests/test_mcp.c`、`scripts/smoke-invariants.sh` 與 `make -f Makefile.cbm rust-baseline-fixtures`。

## 驗證順序

1. 先補 store/cypher/MCP contract fixtures，預設 C path 必須通過。
2. 新增 Rust pure helper 與 FFI smoke，不改產品預設行為。
3. 新增單一 opt-in flag，跑預設 C path、單一 opt-in 與全 opt-in matrix。
4. 再接 `scripts/test.sh`、`scripts/smoke-test.sh`、`scripts/smoke-invariants.sh`、`make -f Makefile.cbm security` 與 `rust-baseline-fixtures`。
