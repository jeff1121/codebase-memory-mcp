#ifndef CBM_MCP_SEARCH_CODE_SCORE_H
#define CBM_MCP_SEARCH_CODE_SCORE_H

/* 公開 bridge：search_code deduped result ranking 純加權。
 *
 * 契約：
 * - 以 in_degree 為基底；"Function"／"Method" 加 10，"Route" 加 15
 * - file 含 vendored/、vendor/ 或 node_modules/ 扣 50；含 test、spec 或 _test. 扣 5
 * - label 與 file 可為 NULL，NULL 不套用對應加權；字串只讀至第一個 NUL
 * - 不配置、不改寫輸入、不排序；grep、node lookup、結果 ownership 與 JSON 仍由 mcp.c 負責
 */
int cbm_mcp_search_code_score(const char *label, const char *file, int in_degree);

#endif
