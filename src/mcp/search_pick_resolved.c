#include "mcp/search_pick_resolved.h"

#if defined(CBM_USE_RUST_MCP_SEARCH_PICK_RESOLVED) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_SEARCH_PICK_RESOLVED_ONLY) || defined(CBM_USE_RUST_MCP_CODEC_ONLY)
extern int cbm_rs_mcp_search_pick_resolved_index_v1(const long *scores, int count,
                                                    int *ambiguous_out);
#endif

int cbm_mcp_search_pick_resolved_index(const long *scores, int count, bool *ambiguous_out) {
#if defined(CBM_USE_RUST_MCP_SEARCH_PICK_RESOLVED) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_SEARCH_PICK_RESOLVED_ONLY) || defined(CBM_USE_RUST_MCP_CODEC_ONLY)
    if (!ambiguous_out) {
        return -1;
    }
    int ambiguous_i = 0;
    int best = cbm_rs_mcp_search_pick_resolved_index_v1(scores, count, &ambiguous_i);
    *ambiguous_out = (ambiguous_i != 0);
    return best;
#else
    if (!ambiguous_out) {
        return -1;
    }
    *ambiguous_out = false;
    if (count <= 0 || !scores) {
        return -1;
    }
    int best = 0;
    long best_score = scores[0];
    for (int i = 1; i < count; i++) {
        if (scores[i] > best_score) {
            best_score = scores[i];
            best = i;
        }
    }
    int top_count = 0;
    for (int i = 0; i < count; i++) {
        if (scores[i] == best_score) {
            top_count++;
        }
    }
    *ambiguous_out = (top_count > 1);
    return best;
#endif
}
