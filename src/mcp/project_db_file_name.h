#ifndef CBM_MCP_PROJECT_DB_FILE_NAME_H
#define CBM_MCP_PROJECT_DB_FILE_NAME_H

#include <stdbool.h>

/* 判斷 cache directory entry 是否可作為 project `.db` 候選檔名。 */
bool cbm_mcp_project_db_file_name(const char *name);

#endif
