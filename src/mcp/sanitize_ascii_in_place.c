#include "mcp/sanitize_ascii_in_place.h"

#if defined(CBM_USE_RUST_MCP_SANITIZE_ASCII_IN_PLACE) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
extern void cbm_rs_mcp_sanitize_ascii_in_place_v1(char *input);
#endif

void cbm_mcp_sanitize_ascii_in_place(char *input) {
#if defined(CBM_USE_RUST_MCP_SANITIZE_ASCII_IN_PLACE) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
    cbm_rs_mcp_sanitize_ascii_in_place_v1(input);
    return;
#endif
    if (!input) {
        return;
    }
    for (unsigned char *p = (unsigned char *)input; *p; p++) {
        if (*p > 127) {
            *p = '?';
        }
    }
}
