#include "mcp/search_score_cmp.h"

#if defined(CBM_USE_RUST_MCP_SEARCH_SCORE_CMP) || defined(CBM_USE_RUST_MCP_CODEC)
extern int cbm_rs_mcp_search_score_cmp_v1(int left_score, int right_score);
#endif

int cbm_mcp_search_score_cmp(int left_score, int right_score) {
#if defined(CBM_USE_RUST_MCP_SEARCH_SCORE_CMP) || defined(CBM_USE_RUST_MCP_CODEC)
    return cbm_rs_mcp_search_score_cmp_v1(left_score, right_score);
#else
    return right_score - left_score;
#endif
}
