#include "mcp/edge_type_valid.h"

#include "foundation/constants.h"

#include <string.h>

#if defined(CBM_USE_RUST_MCP_EDGE_TYPE) || defined(CBM_USE_RUST_MCP_CODEC)
extern int cbm_rs_mcp_edge_type_valid_v1(const char *input);
#endif

bool cbm_mcp_edge_type_valid(const char *edge_type) {
#if defined(CBM_USE_RUST_MCP_EDGE_TYPE) || defined(CBM_USE_RUST_MCP_CODEC)
    return cbm_rs_mcp_edge_type_valid_v1(edge_type) != 0;
#else
    if (!edge_type || strlen(edge_type) > CBM_SZ_64) {
        return false;
    }
    for (const char *c = edge_type; *c; c++) {
        if (!(*c >= 'A' && *c <= 'Z') && *c != '_') {
            return false;
        }
    }
    return true;
#endif
}
