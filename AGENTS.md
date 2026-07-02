# 儲存庫指引

## 語言規則

所有文件、程式碼註解與對話一律使用繁體中文（zh-TW）。

## 專案結構與模組組織

這是一個純 C 撰寫的 MCP 伺服器。核心程式碼位於 `src/`：`foundation/` 存放共用的配置器與工具函式、`store/` 存放 SQLite 圖形儲存、`cypher/` 負責查詢轉譯、`mcp/` 提供 JSON-RPC 工具、`pipeline/` 負責索引處理階段，以及 `discover/`、`watcher/`、`cli/` 和 `ui/`。Tree-sitter 擷取與 LSP 邏輯位於 `internal/cbm/`。測試位於 `tests/test_*.c`，累積的錯誤重現案例則放在 `tests/repro/`。選用的圖形視覺化工具為 `graph-ui/`；產生的內嵌資產會提供給 `src/ui/`。請使用 `scripts/` 處理本地工作流程、`vendored/` 存放隨附相依套件、`docs/` 存放已發佈的文件，以及 `pkg/` 存放套件封裝。

## 建置、測試與開發指令

- `scripts/build.sh`：乾淨的正式版建置，輸出至 `build/c/codebase-memory-mcp`。
- `scripts/build.sh --with-ui`：建置 `graph-ui` 並將 UI 內嵌至執行檔中。
- `scripts/test.sh`：乾淨的 ASan/UBSan 測試執行，外加 watchdog 與 security-string 迴歸測試。
- `make -f Makefile.cbm test-foundation`：僅針對 foundation 的快速測試目標。
- `scripts/lint.sh`：本地端的 clang-tidy、cppcheck、clang-format 與 no-skip 檢查。
- `scripts/lint.sh --ci`：CI 風格的 lint，不含 clang-tidy。
- `make -f Makefile.cbm security`：完整的安全稽核套件；會先建置正式版執行檔。

## 程式碼風格與命名慣例

請撰寫 C11，而非 Go。遵循 `.clang-format`：4 個空格縮排、不使用 Tab、100 欄字元上限、K&R 大括號風格、指標靠右對齊（例如 `const char* name`）、保留 include 群組，以及不排序 include。優先將模組區域的輔助函式宣告為 `static`；C 函式使用 lower_snake_case，匯出的常數或巨集使用 `CBM_`／大寫命名。Pipeline 處理階段遵循 `src/pipeline/pass_<feature>.c` 的命名。除了已記錄的遞迴白名單外，請避免使用 `NOLINT`。

## 測試指引

在受影響模組附近的 `tests/test_*.c` 中新增聚焦的測試。針對迴歸問題，適當時請在 `tests/repro/repro_issueNNN.c` 下新增或更新重現案例。錯誤修正應先重現失敗情形，再證明修正有效。測試應明確地通過或失敗；只有在真正的平台限制下才可跳過。提交前請執行 `scripts/test.sh`，僅在迭代時才使用較狹窄的 make 目標。

## 提交與拉取請求（PR）指引

使用歷史紀錄中可見的 conventional commits，例如 `fix(store): open query stores readonly` 或 `feat(cli): add --args-file`。每次提交都必須以 `git commit -s` 進行簽署（sign off）。請將 PR 範圍限縮於單一議題，且變更行數最好在 500 行以內。除非提交的是小型錯誤修正或僅測試的變更，否則請以 `Fixes #N` 或 `Closes #N` 參照相關議題。為新行為納入測試，並確認本地端 `scripts/test.sh` 與 lint 結果。

## 安全性與組態提示

以 `scripts/install-git-hooks.sh` 安裝 hooks。任何新的 `system()`、`popen()`、`fork()` 或網路呼叫，都需要明確的理由說明，並在 `scripts/security-allowlist.txt` 中更新允許清單。

## 行為準則

以下準則用來降低常見的 LLM 編碼錯誤，可依專案需要與其他指引合併使用。這些準則傾向謹慎優先於速度；面對瑣碎任務時請自行斟酌。

### 1. 動手前先思考

不臆測、不隱藏疑慮、主動指出取捨。實作前請：

- 明確陳述你的假設；若不確定就發問。
- 若存在多種解讀，請一併列出，不要私自選定。
- 若有更簡單的做法，直說；必要時提出反對意見。
- 若有不清楚之處，先停下來，指出困惑點並發問。

### 2. 簡單優先

以能解決問題的最小程式碼為原則，不做臆測性設計。

- 不新增未被要求的功能。
- 不為一次性程式碼建立抽象層。
- 不加入未被要求的「彈性」或「可設定性」。
- 不為不可能發生的情境撰寫錯誤處理。
- 若寫了 200 行但其實 50 行就能完成，請重寫。

自問：「資深工程師會覺得這過度複雜嗎？」若會，就簡化。

### 3. 外科手術式變更

只更動必要之處，只清理自己造成的混亂。編輯既有程式碼時：

- 不「順手改善」相鄰的程式碼、註解或格式。
- 不重構沒有壞掉的東西。
- 沿用既有風格，即使你會有不同做法。
- 若發現無關的死程式碼，提出說明即可，不要刪除。

當你的變更產生孤兒項目時：

- 移除因你的變更而不再使用的 import／變數／函式。
- 除非被要求，否則不移除既有的死程式碼。

檢驗標準：每一行變更都應能直接對應到使用者的需求。

### 4. 目標導向執行

先定義成功標準，再反覆執行直到通過驗證。把任務轉化為可驗證的目標：

- 「加入驗證」→「先為無效輸入撰寫測試，再讓它們通過」。
- 「修正錯誤」→「先撰寫可重現該錯誤的測試，再讓它通過」。
- 「重構 X」→「確保重構前後測試皆通過」。

對於多步驟任務，請先簡述計畫：

```
1. [步驟] → 驗證：[檢查]
2. [步驟] → 驗證：[檢查]
3. [步驟] → 驗證：[檢查]
```

明確的成功標準能讓你獨立地反覆推進；模糊的標準（例如「讓它能動」）則需要不斷澄清。

成效衡量：差異中不必要的變更變少、因過度複雜而重寫的情況變少，且澄清問題發生在實作之前而非犯錯之後。

## 開發原則

聚焦於程式碼品質、測試標準、使用者體驗一致性與效能需求：

- 前端設計：使用 `ui-ux-pro-max` skill。
- 資訊搜尋：使用 `felo-search` skill。
- 前端測試：使用 `playwright-cli` skill，以 head 模式測試前端。
- 採用 MVP 原則，不過度設計。