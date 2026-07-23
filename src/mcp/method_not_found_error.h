#ifndef CBM_MCP_METHOD_NOT_FOUND_ERROR_H
#define CBM_MCP_METHOD_NOT_FOUND_ERROR_H

#include <stddef.h>

/* 公開 bridge：MCP method not found error object format ("{\"code\":-32601,\"message\":\"Method not
 * found\"}")。
 *
 * 契約：
 * - 輸出固定 "{\"code\":-32601,\"message\":\"Method not found\"}"
 * - buf == NULL 或 bufsize == 0 時長度預估（回傳 44）
 * - 短 buffer snprintf NUL 截斷
 * - 無 heap、無 I/O
 */
size_t cbm_mcp_method_not_found_error(char *buf, size_t bufsize);

#endif
