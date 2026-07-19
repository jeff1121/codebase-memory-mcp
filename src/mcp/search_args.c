#include "mcp/search_args.h"

#include "mcp/search_path_arg.h"

#if defined(CBM_USE_RUST_MCP_SEARCH_ARGS) || defined(CBM_USE_RUST_MCP_CODEC)
extern int cbm_rs_mcp_search_args_valid_v1(const char *root_path, const char *file_pattern);
#endif

bool cbm_mcp_search_args_valid(const char *root_path, const char *file_pattern) {
#if defined(CBM_USE_RUST_MCP_SEARCH_ARGS) || defined(CBM_USE_RUST_MCP_CODEC)
    return cbm_rs_mcp_search_args_valid_v1(root_path, file_pattern) != 0;
#else
    if (!cbm_mcp_search_path_arg_valid(root_path)) {
        return false;
    }
    if (file_pattern && !cbm_mcp_search_path_arg_valid(file_pattern)) {
        return false;
    }
    return true;
#endif
}
