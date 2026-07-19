#ifndef CBM_MCP_UTF8_IS_CONT_H
#define CBM_MCP_UTF8_IS_CONT_H

#include <stdbool.h>

/* 公開 bridge：判斷 int 低 8 bits 是否為 UTF-8 continuation byte（10xxxxxx）。
 *
 * 契約：
 * - 僅使用 byte 的低 8 bits；0x80..0xBF 回 true，其餘回 false
 * - 不配置、不改寫輸入、不做 I/O
 * - UTF-8 掃描、replacement 與字串 ownership 仍由 mcp.c 負責
 */
bool cbm_mcp_utf8_is_cont_byte(int byte);

#endif
