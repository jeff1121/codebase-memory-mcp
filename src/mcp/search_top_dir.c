#include "mcp/search_top_dir.h"

#include <stdint.h>
#include <string.h>

#if defined(CBM_USE_RUST_MCP_SEARCH_TOP_DIR) || defined(CBM_USE_RUST_MCP_CODEC)
extern size_t cbm_rs_mcp_search_top_dir_v1(char *buf, size_t bufsize, const char *file);
#endif

size_t cbm_mcp_search_top_dir(char *buf, size_t bufsize, const char *file) {
#if defined(CBM_USE_RUST_MCP_SEARCH_TOP_DIR) || defined(CBM_USE_RUST_MCP_CODEC)
    return cbm_rs_mcp_search_top_dir_v1(buf, bufsize, file);
#else
    if (!file) {
        return SIZE_MAX;
    }

    const char *slash = strchr(file, '/');
    size_t len = slash ? (size_t)(slash - file + 1) : strlen(file);
    if (buf && bufsize > 0) {
        size_t copied = len;
        if (copied >= bufsize) {
            copied = bufsize - 1;
        }
        if (copied > 0) {
            memcpy(buf, file, copied);
        }
        buf[copied] = '\0';
    }
    return len;
#endif
}
