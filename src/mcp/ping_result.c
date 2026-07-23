#include "mcp/ping_result.h"

#include <stdio.h>
#include <string.h>

#if defined(CBM_USE_RUST_MCP_PING_RESULT) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_PING_RESULT_ONLY) || defined(CBM_USE_RUST_MCP_CODEC_ONLY)
extern size_t cbm_rs_mcp_ping_result_v1(char *buf, size_t bufsize);
#endif

size_t cbm_mcp_ping_result(char *buf, size_t bufsize) {
#if defined(CBM_USE_RUST_MCP_PING_RESULT) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_PING_RESULT_ONLY) || defined(CBM_USE_RUST_MCP_CODEC_ONLY)
    return cbm_rs_mcp_ping_result_v1(buf, bufsize);
#else
    if (!buf || bufsize == 0) {
        return 2; /* strlen("{}") */
    }
    return (size_t)snprintf(buf, bufsize, "{}");
#endif
}
