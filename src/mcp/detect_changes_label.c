#include "mcp/detect_changes_label.h"

#include <string.h>

#if defined(CBM_USE_RUST_MCP_DETECT_CHANGES_LABEL) || defined(CBM_USE_RUST_MCP_CODEC)
extern int cbm_rs_mcp_detect_changes_impacted_label_v1(const char *label);
#endif

bool cbm_mcp_detect_changes_impacted_label(const char *label) {
#if defined(CBM_USE_RUST_MCP_DETECT_CHANGES_LABEL) || defined(CBM_USE_RUST_MCP_CODEC)
    return cbm_rs_mcp_detect_changes_impacted_label_v1(label) != 0;
#else
    return label && strcmp(label, "File") != 0 && strcmp(label, "Folder") != 0 &&
           strcmp(label, "Project") != 0;
#endif
}
