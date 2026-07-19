#ifndef CBM_MCP_SEARCH_SCORE_CMP_H
#define CBM_MCP_SEARCH_SCORE_CMP_H

/* 公開 bridge：search_code 結果 descending score comparator 純 scalar。
 *
 * 契約：回傳 right_score - left_score（left 較高 → 負值；right 較高 → 正值；相等 → 0）。
 * qsort／search_result_t ownership 仍由 mcp.c 負責。
 */
int cbm_mcp_search_score_cmp(int left_score, int right_score);

#endif
