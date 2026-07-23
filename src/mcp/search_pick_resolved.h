#ifndef CBM_MCP_SEARCH_PICK_RESOLVED_H
#define CBM_MCP_SEARCH_PICK_RESOLVED_H

#include <stdbool.h>

/* 公開 bridge：由 node resolution scores 選 best index 並標記 tie ambiguous。
 *
 * 契約：
 * - ambiguous_out == NULL → -1
 * - count <= 0 或 scores == NULL → -1 且 *ambiguous_out = false
 * - 取最大 score 的第一個 index；多個 top score 時 ambiguous=true
 * - 不配置、不改寫 scores、不做 I/O
 */
int cbm_mcp_search_pick_resolved_index(const long *scores, int count, bool *ambiguous_out);

#endif
