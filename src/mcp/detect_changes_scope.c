#include "mcp/detect_changes_scope.h"

#include <string.h>

#if defined(CBM_USE_RUST_MCP_DETECT_CHANGES_SCOPE) || defined(CBM_USE_RUST_MCP_CODEC)
extern int cbm_rs_mcp_detect_changes_wants_symbols_v1(const char *scope);
#endif

bool cbm_mcp_detect_changes_wants_symbols(const char *scope) {
#if defined(CBM_USE_RUST_MCP_DETECT_CHANGES_SCOPE) || defined(CBM_USE_RUST_MCP_CODEC)
    return cbm_rs_mcp_detect_changes_wants_symbols_v1(scope) != 0;
#else
    return !scope || strcmp(scope, "symbols") == 0 || strcmp(scope, "impact") == 0;
#endif
}
