#ifndef CBM_MCP_SEARCH_MODE_H
#define CBM_MCP_SEARCH_MODE_H

/* 公開 bridge：search_code.mode 純 classifier。
 *
 * 契約：
 * - "full" → 1
 * - "files" → 2
 * - NULL、空字串、"compact"、未知、大小寫不符、尾端空白 → 0
 * - 僅 byte-exact strcmp；handler／grep／response／transport 仍由 mcp.c 負責
 */
int cbm_mcp_search_mode(const char *mode_str);

#endif
