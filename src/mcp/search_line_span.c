#include "mcp/search_line_span.h"

#if defined(CBM_USE_RUST_MCP_SEARCH_LINE_SPAN) || defined(CBM_USE_RUST_MCP_CODEC)
extern int cbm_rs_mcp_search_line_match_span_v1(int start_line, int end_line, int line);
#endif

int cbm_mcp_search_line_match_span(int start_line, int end_line, int line) {
#if defined(CBM_USE_RUST_MCP_SEARCH_LINE_SPAN) || defined(CBM_USE_RUST_MCP_CODEC)
    return cbm_rs_mcp_search_line_match_span_v1(start_line, end_line, line);
#else
    if (start_line <= line && end_line >= line) {
        return end_line - start_line;
    }
    return -1;
#endif
}
