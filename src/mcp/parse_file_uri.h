#ifndef CBM_MCP_PARSE_FILE_URI_H
#define CBM_MCP_PARSE_FILE_URI_H

#include <stdbool.h>

/* 公開 bridge：file:// URI → filesystem path extraction。
 *
 * 契約：
 * - 僅接受 "file://" prefix（byte-exact；大小寫敏感）
 * - 保留 raw path 與 percent encoding（不做 decode）
 * - Windows drive：file:///C:/... → 剝除 path 開頭單一 '/'
 * - 成功時以 snprintf 語意寫入 caller buffer（短 buffer NUL 截斷）並回 true
 * - file://（empty path）成功，寫入 ""
 * - invalid scheme、空字串、NULL uri → false；out_path 且 out_size>0 時清空
 * - out_path==NULL 或 out_size<=0 → false（out_size>0 且 out_path 非 NULL 才可寫）
 * - 無 heap、無 I/O；root/session／URI 使用端仍由呼叫端負責
 */
bool cbm_parse_file_uri(const char *uri, char *out_path, int out_size);

#endif
