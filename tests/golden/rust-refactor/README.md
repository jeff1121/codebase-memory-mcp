# Rust 重構 Golden Fixtures

> **Fixture 說明，不是目前交接或完整測試證據（2026-07-16）**：目前重構狀態請讀
> [../../../docs/rust-refactor-current-handoff.md](../../../docs/rust-refactor-current-handoff.md)。
> fixture 成功只代表該 fixture 的公開行為比對成功，不能取代單一 opt-in、direct source
> exclusion、完整 CI 或 release gate。

本目錄保存 Rust 重構期間用來比對公開行為的第一批 golden fixtures。

使用方式：

```sh
python3 scripts/rust-baseline-fixtures.py build/c/codebase-memory-mcp
python3 scripts/rust-baseline-fixtures.py build/c/codebase-memory-mcp --update
make -f Makefile.cbm rust-large-performance-baseline
make -f Makefile.cbm rust-large-performance-baseline-update
make -f Makefile.cbm rust-language-graph-parity
make -f Makefile.cbm rust-language-graph-parity-update
```

`--update` 只應在刻意接受 CLI、MCP、graph、artifact 或小型效能基準變更時使用。
`rust-large-performance-baseline-update` 只應在刻意接受大型效能基準變更時使用。
`rust-language-graph-parity-update` 只應在刻意接受代表性語言 graph parity 變更時使用。
腳本會建立 hermetic 暫存 repo，固定 project name 為 `rust_refactor_golden`，
並正規化版本、暫存路徑與 MCP 分頁結果。更新或驗證時會檢查 golden
內容不得包含本機路徑、暫存 repo、cache 或 binary 路徑。

預期檔案：

- `cli-golden.json`
- `mcp-golden.json`
- `graph-golden.json`
- `artifact-golden.json`
- `performance-baseline.json`
- `self-index-baseline.json`
- `large-performance-baseline.json`
- `language-graph-parity.json`

`cli-golden.json` 已涵蓋 raw JSON args、stdin JSON、`--args-file`、flag form、
`--json` envelope、非 `--json` 成功與錯誤輸出摘要。`mcp-golden.json` 已涵蓋
initialize、initialized notification no-response、tools/list 兩頁 pagination、error
responses、unknown tool、alias/schema/indexed-repo transcript 與 Content-Length
framed ping。`artifact-golden.json` 已涵蓋 `persistence=true` artifact export、
metadata 摘要、`.gitattributes`、artifact project graph histogram、bootstrap
import、schema mismatch skip 與匯入後 graph 等價。`self-index-baseline.json`
是手動 opt-in baseline，使用 `make -f Makefile.cbm rust-self-index-baseline`
驗證目前 repository 的 full self-index 摘要。`large-performance-baseline.json`
是手動 opt-in baseline，使用 deterministic generated repository 驗證較大型的
index、schema、query、search、RSS guard 與 binary size guard；不屬於預設
`scripts/test.sh`、`scripts/rust-check.sh` 或 `rust-ci`。`language-graph-parity.json`
由 `make -f Makefile.cbm rust-language-graph-parity` 驗證，固定 Python、
TypeScript、Go、Rust、Java、C++ 與 YAML fixture 的 definitions、calls、imports、routes、
semantic/LSP edges，並比對 default C path 與 Rust registry/plan opt-in path。
目前 RSS 取樣支援 macOS/Linux。
