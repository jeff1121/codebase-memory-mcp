#ifndef CBM_MCP_NODE_RESOLUTION_SCORE_H
#define CBM_MCP_NODE_RESOLUTION_SCORE_H

/* 公開 bridge：計算 trace／snippet name resolution 的候選純計分。
 *
 * 契約：
 * - Function／Method 為 tier 2，Module／File 與 NULL 為 tier 0，其他 label 為 tier 1
 * - 回傳 label tier * 1000000 加上 max(end_line - start_line, 0)
 * - label 只讀至第一個 NUL；不配置、不改寫輸入、不做 I/O
 * - node query、candidate selection、BFS、JSON 與 transport orchestration 仍由 mcp.c 負責
 */
long cbm_mcp_node_resolution_score(const char *label, int start_line, int end_line);

#endif
