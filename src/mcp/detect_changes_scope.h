#ifndef CBM_MCP_DETECT_CHANGES_SCOPE_H
#define CBM_MCP_DETECT_CHANGES_SCOPE_H

#include <stdbool.h>

/* 公開 bridge：判斷 detect_changes 是否輸出 impacted symbols。
 *
 * 契約：
 * - NULL、"symbols" 與 "impact" 回 true；其他值回 false
 * - scope 只讀至第一個 NUL；不配置、不改寫輸入、不做 I/O
 * - git diff、Store lookup、JSON 組裝與 handler orchestration 仍由 mcp.c 負責
 */
bool cbm_mcp_detect_changes_wants_symbols(const char *scope);

#endif
