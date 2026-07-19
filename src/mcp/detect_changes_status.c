#include "mcp/detect_changes_status.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#if defined(CBM_USE_RUST_MCP_DETECT_CHANGES_STATUS) || defined(CBM_USE_RUST_MCP_CODEC)
extern size_t cbm_rs_mcp_detect_changes_status_path_offset_v1(const char *line);
#endif

#if !defined(CBM_USE_RUST_MCP_DETECT_CHANGES_STATUS) && !defined(CBM_USE_RUST_MCP_CODEC)
static bool is_porcelain_status_code(char code) {
    return strchr(" MADRCU?!", code) != NULL;
}
#endif

size_t cbm_mcp_detect_changes_status_path_offset(const char *line) {
#if defined(CBM_USE_RUST_MCP_DETECT_CHANGES_STATUS) || defined(CBM_USE_RUST_MCP_CODEC)
    return cbm_rs_mcp_detect_changes_status_path_offset_v1(line);
#else
    if (!line || line[0] == '\0') {
        return SIZE_MAX;
    }

    size_t line_len = strlen(line);
    const char *path = line;
    if (line_len > 2 && line[2] == ' ' && is_porcelain_status_code(line[0]) &&
        is_porcelain_status_code(line[1])) {
        path = line + 3;
        const char *arrow = strstr(path, " -> ");
        if (arrow) {
            path = arrow + 4;
        }
    }

    return path[0] == '\0' ? SIZE_MAX : (size_t)(path - line);
#endif
}
