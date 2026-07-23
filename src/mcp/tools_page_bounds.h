#ifndef CBM_MCP_TOOLS_PAGE_BOUNDS_H
#define CBM_MCP_TOOLS_PAGE_BOUNDS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int32_t start;
    int32_t end;
    int32_t has_next;
    int32_t next_cursor;
} CbmRsMcpToolsPageBoundsV1;

/* 公開 bridge：MCP tools/list page bounds 計算。
 *
 * 契約：
 * - offset < 0 歸 0，offset > tool_count 歸 tool_count
 * - limit < 0 或 limit > tool_count 歸 tool_count
 * - end = offset + limit；若 end > tool_count 歸 tool_count
 * - has_next = (include_next_cursor && end < tool_count) ? 1 : 0
 * - next_cursor = end
 * - out == NULL 時回傳 -1，成功寫入 *out 並回傳 0
 * - 無 heap、無 I/O
 */
int cbm_mcp_tools_page_bounds(int offset, int limit, bool include_next_cursor, int tool_count,
                              CbmRsMcpToolsPageBoundsV1 *out);

#endif
