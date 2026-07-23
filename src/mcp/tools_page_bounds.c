#include "mcp/tools_page_bounds.h"

#if defined(CBM_USE_RUST_MCP_TOOLS_PAGE_BOUNDS) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_TOOLS_PAGE_BOUNDS_ONLY) || defined(CBM_USE_RUST_MCP_CODEC_ONLY)
extern int cbm_rs_mcp_tools_page_bounds_v1(int offset, int limit, int include_next_cursor,
                                           int tool_count, CbmRsMcpToolsPageBoundsV1 *out);
#endif

int cbm_mcp_tools_page_bounds(int offset, int limit, bool include_next_cursor, int tool_count,
                              CbmRsMcpToolsPageBoundsV1 *out) {
    if (!out) {
        return -1;
    }

#if defined(CBM_USE_RUST_MCP_TOOLS_PAGE_BOUNDS) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_TOOLS_PAGE_BOUNDS_ONLY) || defined(CBM_USE_RUST_MCP_CODEC_ONLY)
    return cbm_rs_mcp_tools_page_bounds_v1(offset, limit, include_next_cursor ? 1 : 0, tool_count,
                                           out);
#else
    if (tool_count < 0) {
        tool_count = 0;
    }
    if (offset < 0) {
        offset = 0;
    }
    if (offset > tool_count) {
        offset = tool_count;
    }
    if (limit < 0 || limit > tool_count) {
        limit = tool_count;
    }

    int end = offset + limit;
    if (end > tool_count) {
        end = tool_count;
    }

    out->start = offset;
    out->end = end;
    out->has_next = (include_next_cursor && end < tool_count) ? 1 : 0;
    out->next_cursor = end;
    return 0;
#endif
}
