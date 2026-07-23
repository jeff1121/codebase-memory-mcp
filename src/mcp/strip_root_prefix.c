#include "mcp/strip_root_prefix.h"

#include <string.h>

#if defined(CBM_USE_RUST_MCP_STRIP_ROOT_PREFIX) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_STRIP_ROOT_PREFIX_ONLY) || defined(CBM_USE_RUST_MCP_CODEC_ONLY)
extern size_t cbm_rs_mcp_strip_root_prefix_offset_v1(const char *path, const char *root,
                                                     size_t root_len);
#endif

size_t cbm_mcp_strip_root_prefix_offset(const char *path, const char *root, size_t root_len) {
#if defined(CBM_USE_RUST_MCP_STRIP_ROOT_PREFIX) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_STRIP_ROOT_PREFIX_ONLY) || defined(CBM_USE_RUST_MCP_CODEC_ONLY)
    return cbm_rs_mcp_strip_root_prefix_offset_v1(path, root, root_len);
#else
    if (!path || !root) {
        return 0;
    }
    size_t path_len = strlen(path);
    size_t root_str_len = strlen(root);
    if (root_len > path_len || root_len > root_str_len) {
        return 0;
    }
    if (strncmp(path, root, root_len) != 0) {
        return 0;
    }
    size_t offset = root_len;
    if (path[offset] == '/') {
        offset++;
    }
    return offset;
#endif
}
