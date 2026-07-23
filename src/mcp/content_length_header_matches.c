#include "mcp/content_length_header_matches.h"

#include <string.h>

enum {
    CONTENT_LENGTH_PREFIX_LEN = 15, /* strlen("Content-Length:") */
};

#if defined(CBM_USE_RUST_MCP_CONTENT_LENGTH_HEADER_MATCHES) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_CONTENT_LENGTH_HEADER_MATCHES_ONLY) ||                               \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
extern int cbm_rs_mcp_content_length_header_matches_v1(const char *line);
#endif

bool cbm_mcp_content_length_header_matches(const char *line) {
    if (!line) {
        return false;
    }

#if defined(CBM_USE_RUST_MCP_CONTENT_LENGTH_HEADER_MATCHES) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_CONTENT_LENGTH_HEADER_MATCHES_ONLY) ||                               \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
    return cbm_rs_mcp_content_length_header_matches_v1(line) != 0;
#else
    return strncmp(line, "Content-Length:", CONTENT_LENGTH_PREFIX_LEN) == 0;
#endif
}
