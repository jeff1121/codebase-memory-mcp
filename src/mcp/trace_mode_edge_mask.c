#include "mcp/trace_mode_edge_mask.h"

#include <string.h>

enum {
    TRACE_EDGE_CALLS = 1u << 0,
    TRACE_EDGE_DATA_FLOWS = 1u << 1,
    TRACE_EDGE_HTTP_CALLS = 1u << 2,
    TRACE_EDGE_ASYNC_CALLS = 1u << 3,
    TRACE_EDGE_CROSS_HTTP_CALLS = 1u << 4,
    TRACE_EDGE_CROSS_ASYNC_CALLS = 1u << 5,
    TRACE_EDGE_CROSS_CHANNEL = 1u << 6,
    TRACE_EDGE_CROSS_GRPC_CALLS = 1u << 7,
    TRACE_EDGE_CROSS_GRAPHQL_CALLS = 1u << 8,
    TRACE_EDGE_CROSS_TRPC_CALLS = 1u << 9,
};

#if defined(CBM_USE_RUST_MCP_TRACE_MODE_EDGE_MASK) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
extern uint32_t cbm_rs_mcp_trace_mode_edge_mask_v1(const char *input);
#endif

uint32_t cbm_mcp_trace_mode_edge_mask(const char *input) {
#if defined(CBM_USE_RUST_MCP_TRACE_MODE_EDGE_MASK) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
    return cbm_rs_mcp_trace_mode_edge_mask_v1(input);
#else
    if (input && strcmp(input, "data_flow") == 0) {
        return TRACE_EDGE_CALLS | TRACE_EDGE_DATA_FLOWS;
    }
    if (input && strcmp(input, "cross_service") == 0) {
        return TRACE_EDGE_HTTP_CALLS | TRACE_EDGE_ASYNC_CALLS | TRACE_EDGE_DATA_FLOWS |
               TRACE_EDGE_CALLS | TRACE_EDGE_CROSS_HTTP_CALLS | TRACE_EDGE_CROSS_ASYNC_CALLS |
               TRACE_EDGE_CROSS_CHANNEL | TRACE_EDGE_CROSS_GRPC_CALLS |
               TRACE_EDGE_CROSS_GRAPHQL_CALLS | TRACE_EDGE_CROSS_TRPC_CALLS;
    }
    return TRACE_EDGE_CALLS;
#endif
}
