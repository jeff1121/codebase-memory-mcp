#ifndef CBM_MCP_CONTENT_LENGTH_HEADER_IS_BLANK_H
#define CBM_MCP_CONTENT_LENGTH_HEADER_IS_BLANK_H

#include <stdbool.h>

/* 公開 bridge：Content-Length header/body separator (blank line) classifier。
 *
 * 契約：
 * - 移除尾端 '\r' / '\n' 後若為空字串 → true
 * - ""、"\n"、"\r\n"、"\r\r\n" 等回傳 true
 * - line == NULL、含有空白或列內容 → false
 * - 第一個 NUL 前比對
 * - 無 heap、無 I/O
 */
bool cbm_mcp_content_length_header_is_blank(const char *line);

#endif
