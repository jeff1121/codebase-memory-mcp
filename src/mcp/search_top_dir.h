#ifndef CBM_MCP_SEARCH_TOP_DIR_H
#define CBM_MCP_SEARCH_TOP_DIR_H

#include <stddef.h>

/* 公開 bridge：取得 search_code directory distribution 使用的 top-level key。
 *
 * 契約：
 * - file 為 NULL 時回傳 SIZE_MAX，且不改寫 buf
 * - 成功時回傳完整 key 長度；key 包含第一個 '/'，若沒有 '/' 則為完整 file
 * - buf 為 NULL 或 bufsize 為 0 時只回傳長度；短 buffer 寫入最多 bufsize - 1 bytes
 *   並 NUL-terminate
 * - file 只讀至第一個 NUL；不配置、不改寫輸入、不做 I/O
 */
size_t cbm_mcp_search_top_dir(char *buf, size_t bufsize, const char *file);

#endif
