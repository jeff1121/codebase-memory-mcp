#include "mcp/search_path_arg.h"

#if defined(CBM_USE_RUST_MCP_SEARCH_PATH_ARG) || defined(CBM_USE_RUST_MCP_CODEC)
extern int cbm_rs_mcp_search_path_arg_valid_v1(const char *input);
#endif

bool cbm_mcp_search_path_arg_valid(const char *input) {
#if defined(CBM_USE_RUST_MCP_SEARCH_PATH_ARG) || defined(CBM_USE_RUST_MCP_CODEC)
    return cbm_rs_mcp_search_path_arg_valid_v1(input) != 0;
#else
    if (!input) {
        return false;
    }
    for (const char *p = input; *p; p++) {
        switch (*p) {
        case '\'':
        case '"':
        case ';':
        case '|':
        case '$':
        case '`':
        case '<':
        case '>':
        case '\n':
        case '\r':
#ifndef _WIN32
        case '\\':
#endif
            return false;
        default:
            break;
        }
    }
    return true;
#endif
}
