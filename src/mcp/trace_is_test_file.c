#include "mcp/trace_is_test_file.h"

#include <string.h>

#if defined(CBM_USE_RUST_MCP_TRACE_IS_TEST_FILE) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
extern int cbm_rs_mcp_trace_is_test_file_v1(const char *input);
#endif

bool cbm_mcp_trace_is_test_file(const char *path) {
#if defined(CBM_USE_RUST_MCP_TRACE_IS_TEST_FILE) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
    return cbm_rs_mcp_trace_is_test_file_v1(path) != 0;
#endif
    if (!path) {
        return false;
    }
    return strstr(path, "/test") != NULL || strstr(path, "test_") != NULL ||
           strstr(path, "_test.") != NULL || strstr(path, "/tests/") != NULL ||
           strstr(path, "/spec/") != NULL || strstr(path, ".test.") != NULL;
}
