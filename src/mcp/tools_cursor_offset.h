#ifndef CBM_MCP_TOOLS_CURSOR_OFFSET_H
#define CBM_MCP_TOOLS_CURSOR_OFFSET_H

/* 公開 bridge：MCP tools/list cursor offset 解析。
 *
 * 契約：
 * - params_json == NULL 或無效 JSON → 回傳 0
 * - 無 cursor 鍵 → 回傳 0
 * - cursor 鍵存在但非字串或為空字串 → 回傳 tool_count
 * - cursor 鍵為數字字串 → 轉換數字 parsed；若 parsed < 0 或含無效字元/溢位 → 回傳 tool_count；
 *   否則回傳 parsed > tool_count ? tool_count : parsed
 * - 無 heap、無 I/O
 */
int cbm_mcp_tools_cursor_offset(const char *params_json, int tool_count);

#endif
