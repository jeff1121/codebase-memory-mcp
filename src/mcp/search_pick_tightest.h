#ifndef CBM_MCP_SEARCH_PICK_TIGHTEST_H
#define CBM_MCP_SEARCH_PICK_TIGHTEST_H

/* 公開 bridge：由 search_line_match_span 結果陣列選出最緊 node index。
 *
 * 契約：
 * - count <= 0 或 spans == NULL 回 -1
 * - 只把 span >= 0 視為有效；取最小有效 span 的第一個 index
 * - 無有效 span 回 -1
 * - 不配置、不改寫輸入、不做 I/O
 */
int cbm_mcp_search_pick_tightest_index(const int *spans, int count);

#endif
