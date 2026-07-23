#include "mcp/content_length_header_is_blank.h"

#include <stddef.h>

#if defined(CBM_USE_RUST_MCP_CONTENT_LENGTH_HEADER_IS_BLANK) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_CONTENT_LENGTH_HEADER_IS_BLANK_ONLY) ||                               \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
extern int cbm_rs_mcp_content_length_header_is_blank_v1(const char *line);
#endif

bool cbm_mcp_content_length_header_is_blank(const char *line) {
    if (!line) {
        return false;
    }

#if defined(CBM_USE_RUST_MCP_CONTENT_LENGTH_HEADER_IS_BLANK) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_CONTENT_LENGTH_HEADER_IS_BLANK_ONLY) ||                               \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
    return cbm_rs_mcp_content_length_header_is_blank_v1(line) != 0;
#else
    const char *p = line;
    while (*p == '\r' || *p == '\n') {
        p++;
    }
    return *p == '\0';
#endif
}
