#ifndef CBM_MCP_DETECT_CHANGES_STATUS_H
#define CBM_MCP_DETECT_CHANGES_STATUS_H

#include <stddef.h>

/* 公開 bridge：取得 detect_changes status line 的有效路徑起始 offset。
 *
 * 契約：
 * - NULL、空行或最終路徑為空時回 SIZE_MAX
 * - porcelain `XY ` 前綴跳過 3 bytes；rename 取 " -> " 後的 destination
 * - line 只讀至第一個 NUL；不配置、不改寫輸入、不做 I/O
 * - 行迴圈、Store lookup、JSON 組裝與 handler orchestration 仍由 mcp.c 負責
 */
size_t cbm_mcp_detect_changes_status_path_offset(const char *line);

#endif
