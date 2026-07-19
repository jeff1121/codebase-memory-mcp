#ifndef CBM_MCP_INDEX_MODE_H
#define CBM_MCP_INDEX_MODE_H

/* 公開 bridge：index_repository.mode 純 classifier。
 *
 * 契約（對齊 cbm_index_mode_t 0..2 與 dispatch sentinel 3）：
 * - "moderate" → 1
 * - "fast" → 2
 * - "cross-repo-intelligence" → 3
 * - NULL、空字串、"full"、未知、大小寫不符、尾端空白 → 0（full/default）
 * - 僅 byte-exact strcmp；handler／pipeline／transport 仍由 mcp.c 負責
 */
int cbm_mcp_index_mode(const char *mode_str);

#endif
