#include "mcp/method_not_found_error.h"

#include <stdio.h>
#include <string.h>

#if defined(CBM_USE_RUST_MCP_METHOD_NOT_FOUND_ERROR) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_METHOD_NOT_FOUND_ERROR_ONLY) || defined(CBM_USE_RUST_MCP_CODEC_ONLY)
extern size_t cbm_rs_mcp_method_not_found_error_v1(char *buf, size_t bufsize);
#endif

size_t cbm_mcp_method_not_found_error(char *buf, size_t bufsize) {
#if defined(CBM_USE_RUST_MCP_METHOD_NOT_FOUND_ERROR) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_METHOD_NOT_FOUND_ERROR_ONLY) || defined(CBM_USE_RUST_MCP_CODEC_ONLY)
    return cbm_rs_mcp_method_not_found_error_v1(buf, bufsize);
#else
    if (!buf || bufsize == 0) {
        return 44; /* strlen("{\"code\":-32601,\"message\":\"Method not found\"}") */
    }
    return (size_t)snprintf(buf, bufsize, "{\"code\":-32601,\"message\":\"Method not found\"}");
#endif
}
