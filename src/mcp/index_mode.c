#include "mcp/index_mode.h"

#include "foundation/constants.h"

#include <string.h>

#if defined(CBM_USE_RUST_MCP_INDEX_MODE) || defined(CBM_USE_RUST_MCP_CODEC)
extern int cbm_rs_mcp_index_mode_v1(const char *input);
#endif

/* 與 cbm_index_mode_t / INDEX_REPO_MODE_CROSS_REPO 對齊的數值。 */
enum {
    CBM_MCP_INDEX_MODE_FULL = 0,
    CBM_MCP_INDEX_MODE_MODERATE = 1,
    CBM_MCP_INDEX_MODE_FAST = 2,
    CBM_MCP_INDEX_MODE_CROSS_REPO = 3
};

int cbm_mcp_index_mode(const char *mode_str) {
#if defined(CBM_USE_RUST_MCP_INDEX_MODE) || defined(CBM_USE_RUST_MCP_CODEC)
    return cbm_rs_mcp_index_mode_v1(mode_str);
#else
    if (!mode_str) {
        return CBM_MCP_INDEX_MODE_FULL;
    }
    if (strcmp(mode_str, "moderate") == 0) {
        return CBM_MCP_INDEX_MODE_MODERATE;
    }
    if (strcmp(mode_str, "fast") == 0) {
        return CBM_MCP_INDEX_MODE_FAST;
    }
    if (strcmp(mode_str, "cross-repo-intelligence") == 0) {
        return CBM_MCP_INDEX_MODE_CROSS_REPO;
    }
    return CBM_MCP_INDEX_MODE_FULL;
#endif
}
