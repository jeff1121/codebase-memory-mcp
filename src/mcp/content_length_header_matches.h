#ifndef CBM_MCP_CONTENT_LENGTH_HEADER_MATCHES_H
#define CBM_MCP_CONTENT_LENGTH_HEADER_MATCHES_H

#include <stdbool.h>

/* 公開 bridge：Content-Length framed message header classifier。
 *
 * 契約：
 * - 精準 prefix "Content-Length:"（長度 15 bytes；byte-exact）
 * - line == NULL、空字串、大小寫不符、前導空白、缺少冒號 → false
 * - 第一個 NUL 前比對
 * - 無 heap、無 I/O
 */
bool cbm_mcp_content_length_header_matches(const char *line);

#endif
