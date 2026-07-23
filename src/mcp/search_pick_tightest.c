#include "mcp/search_pick_tightest.h"

#include <limits.h>

#if defined(CBM_USE_RUST_MCP_SEARCH_PICK_TIGHTEST) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_SEARCH_PICK_TIGHTEST_ONLY) || defined(CBM_USE_RUST_MCP_CODEC_ONLY)
extern int cbm_rs_mcp_search_pick_tightest_index_v1(const int *spans, int count);
#endif

int cbm_mcp_search_pick_tightest_index(const int *spans, int count) {
#if defined(CBM_USE_RUST_MCP_SEARCH_PICK_TIGHTEST) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_SEARCH_PICK_TIGHTEST_ONLY) || defined(CBM_USE_RUST_MCP_CODEC_ONLY)
    return cbm_rs_mcp_search_pick_tightest_index_v1(spans, count);
#else
    if (count <= 0 || !spans) {
        return -1;
    }
    int best = -1;
    int best_span = INT_MAX;
    for (int i = 0; i < count; i++) {
        int span = spans[i];
        if (span >= 0 && span < best_span) {
            best = i;
            best_span = span;
        }
    }
    return best;
#endif
}
