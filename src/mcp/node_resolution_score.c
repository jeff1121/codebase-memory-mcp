#include "mcp/node_resolution_score.h"

#include <string.h>

#if defined(CBM_USE_RUST_MCP_NODE_RESOLUTION_SCORE) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
extern long cbm_rs_mcp_node_resolution_score_v1(const char *label, int start_line, int end_line);
#endif

long cbm_mcp_node_resolution_score(const char *label, int start_line, int end_line) {
#if defined(CBM_USE_RUST_MCP_NODE_RESOLUTION_SCORE) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
    return cbm_rs_mcp_node_resolution_score_v1(label, start_line, end_line);
#else
    enum {
        RES_RANK_CALLABLE = 2,
        RES_RANK_OTHER = 1,
        RES_RANK_MODULE = 0,
        RES_LABEL_WEIGHT = 1000000,
    };

    long label_rank = RES_RANK_MODULE;
    if (label) {
        if (strcmp(label, "Function") == 0 || strcmp(label, "Method") == 0) {
            label_rank = RES_RANK_CALLABLE;
        } else if (strcmp(label, "Module") != 0 && strcmp(label, "File") != 0) {
            label_rank = RES_RANK_OTHER;
        }
    }
    long span = (long)end_line - (long)start_line;
    if (span < 0) {
        span = 0;
    }
    return label_rank * (long)RES_LABEL_WEIGHT + span;
#endif
}
