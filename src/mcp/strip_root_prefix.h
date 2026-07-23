#ifndef CBM_MCP_STRIP_ROOT_PREFIX_H
#define CBM_MCP_STRIP_ROOT_PREFIX_H

#include <stddef.h>

/* 公開 bridge：計算 search_code grep path 可安全跳過的 root prefix byte offset。
 *
 * 契約：
 * - path 或 root 為 NULL 時回傳 0
 * - root_len 超過 path 或 root 的 first-NUL 長度時回傳 0
 * - 只做 byte prefix 比對；符合後若下一 byte 是 '/' 再 +1
 * - 不檢查 path component 邊界（"/repo/sub" 可比對 "/repo/submarine.c"）
 * - 不配置、不改寫輸入、不做 I/O；caller 仍以 path + offset 借用原字串
 */
size_t cbm_mcp_strip_root_prefix_offset(const char *path, const char *root, size_t root_len);

#endif
