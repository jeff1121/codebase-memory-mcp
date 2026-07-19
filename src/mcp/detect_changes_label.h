#ifndef CBM_MCP_DETECT_CHANGES_LABEL_H
#define CBM_MCP_DETECT_CHANGES_LABEL_H

#include <stdbool.h>

/* 公開 bridge：判斷 detect_changes 是否保留 impacted node label。
 *
 * 契約：
 * - NULL、"File"、"Folder" 與 "Project" 回 false；其他值回 true
 * - label 只讀至第一個 NUL；不配置、不改寫輸入、不做 I/O
 * - Store lookup、JSON 組裝與 handler orchestration 仍由 mcp.c 負責
 */
bool cbm_mcp_detect_changes_impacted_label(const char *label);

#endif
