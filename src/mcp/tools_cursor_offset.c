#include "mcp/tools_cursor_offset.h"

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "yyjson/yyjson.h"

#if defined(CBM_USE_RUST_MCP_TOOLS_CURSOR_OFFSET) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_TOOLS_CURSOR_OFFSET_ONLY) || defined(CBM_USE_RUST_MCP_CODEC_ONLY)
extern int cbm_rs_mcp_tools_cursor_offset_v1(const char *params_json, int tool_count);
#endif

int cbm_mcp_tools_cursor_offset(const char *params_json, int tool_count) {
#if defined(CBM_USE_RUST_MCP_TOOLS_CURSOR_OFFSET) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_TOOLS_CURSOR_OFFSET_ONLY) || defined(CBM_USE_RUST_MCP_CODEC_ONLY)
    return cbm_rs_mcp_tools_cursor_offset_v1(params_json, tool_count);
#else
    if (!params_json) {
        return 0;
    }

    yyjson_doc *doc = yyjson_read(params_json, strlen(params_json), 0);
    if (!doc) {
        return 0;
    }

    int offset = 0;
    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *cursor = root ? yyjson_obj_get(root, "cursor") : NULL;
    if (cursor) {
        offset = tool_count;
        if (yyjson_is_str(cursor)) {
            const char *cursor_str = yyjson_get_str(cursor);
            if (cursor_str && *cursor_str != '\0') {
                char *endptr = NULL;
                errno = 0;
                long parsed = strtol(cursor_str, &endptr, 10);
                if (endptr && *endptr == '\0' && errno == 0 && parsed >= 0) {
                    offset = parsed > tool_count ? tool_count : (int)parsed;
                }
            }
        }
    }

    yyjson_doc_free(doc);
    return offset;
#endif
}
