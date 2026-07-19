#include "mcp/adr_mode.h"

#include <string.h>

#if defined(CBM_USE_RUST_MCP_ADR_MODE) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
extern int cbm_rs_mcp_adr_mode_v1(const char *input);
#endif

int cbm_mcp_adr_mode(const char *input) {
#if defined(CBM_USE_RUST_MCP_ADR_MODE) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
    return cbm_rs_mcp_adr_mode_v1(input);
#else
    if (input && (strcmp(input, "update") == 0 || strcmp(input, "store") == 0)) {
        return 1;
    }
    if (input && strcmp(input, "sections") == 0) {
        return 2;
    }
    return 0;
#endif
}
