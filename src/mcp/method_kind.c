#include "mcp/method_kind.h"

#include <string.h>

#if defined(CBM_USE_RUST_MCP_METHOD_KIND) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_METHOD_KIND_ONLY) || defined(CBM_USE_RUST_MCP_CODEC_ONLY)
extern int cbm_rs_mcp_method_kind_v1(const char *method);
#endif

int cbm_mcp_method_kind(const char *method) {
#if defined(CBM_USE_RUST_MCP_METHOD_KIND) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_METHOD_KIND_ONLY) || defined(CBM_USE_RUST_MCP_CODEC_ONLY)
    return cbm_rs_mcp_method_kind_v1(method);
#else
    if (!method) {
        return CBM_MCP_METHOD_UNKNOWN;
    }
    if (strcmp(method, "initialize") == 0) {
        return CBM_MCP_METHOD_INITIALIZE;
    }
    if (strcmp(method, "ping") == 0) {
        return CBM_MCP_METHOD_PING;
    }
    if (strcmp(method, "tools/list") == 0) {
        return CBM_MCP_METHOD_TOOLS_LIST;
    }
    if (strcmp(method, "tools/call") == 0) {
        return CBM_MCP_METHOD_TOOLS_CALL;
    }
    if (strcmp(method, "notifications/cancelled") == 0) {
        return CBM_MCP_METHOD_NOTIFICATIONS_CANCELLED;
    }
    return CBM_MCP_METHOD_UNKNOWN;
#endif
}
