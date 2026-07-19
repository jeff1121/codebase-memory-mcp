#include "mcp/search_mode.h"

#include "foundation/constants.h"

#include <string.h>

#if defined(CBM_USE_RUST_MCP_SEARCH_MODE) || defined(CBM_USE_RUST_MCP_CODEC)
extern int cbm_rs_mcp_search_mode_v1(const char *input);
#endif

int cbm_mcp_search_mode(const char *mode_str) {
#if defined(CBM_USE_RUST_MCP_SEARCH_MODE) || defined(CBM_USE_RUST_MCP_CODEC)
    return cbm_rs_mcp_search_mode_v1(mode_str);
#else
    if (!mode_str) {
        return 0;
    }
    if (strcmp(mode_str, "full") == 0) {
        return SKIP_ONE;
    }
    if (strcmp(mode_str, "files") == 0) {
        return 2;
    }
    return 0;
#endif
}
