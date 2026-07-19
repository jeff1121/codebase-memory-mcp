#include "mcp/project_db_file_name.h"

#include <string.h>

#if defined(CBM_USE_RUST_MCP_PROJECT_DB_FILE_NAME) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
extern int cbm_rs_mcp_project_db_file_name_v1(const char *input);
#endif

bool cbm_mcp_project_db_file_name(const char *name) {
#if defined(CBM_USE_RUST_MCP_PROJECT_DB_FILE_NAME) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
    return cbm_rs_mcp_project_db_file_name_v1(name) != 0;
#endif
    if (!name) {
        return false;
    }

    size_t len = strlen(name);
    if (len < 4 || strcmp(name + len - 3, ".db") != 0) {
        return false;
    }
    /* tmp- 前綴是合法 project 名稱；僅排除 internal 與 SQLite 記憶體 marker。 */
    return name[0] != '_' && strncmp(name, ":memory:", 8) != 0;
}
