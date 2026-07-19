#ifndef CBM_MCP_SEARCH_LINE_SPAN_H
#define CBM_MCP_SEARCH_LINE_SPAN_H

/* 公開 bridge：find_tightest_node 單一 node 的 line span 純計算。
 *
 * 契約：
 * - 若 start_line <= line && end_line >= line → 回傳 end_line - start_line
 * - 否則回傳 -1（未命中）
 * - 不配置、不讀 node struct；node 迴圈與 pick 仍由 mcp.c 負責
 */
int cbm_mcp_search_line_match_span(int start_line, int end_line, int line);

#endif
