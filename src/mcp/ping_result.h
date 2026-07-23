#ifndef CBM_MCP_PING_RESULT_H
#define CBM_MCP_PING_RESULT_H

#include <stddef.h>

/* 公開 bridge：MCP ping result object string format (compact "{}")。
 *
 * 契約：
 * - 輸出固定 "{}"
 * - buf == NULL 或 bufsize == 0 時長度預估（回傳 2）
 * - 短 buffer snprintf NUL 截斷
 * - 無 heap、無 I/O
 */
size_t cbm_mcp_ping_result(char *buf, size_t bufsize);

#endif
