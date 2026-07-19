#ifndef CBM_MCP_TRACE_IS_TEST_FILE_H
#define CBM_MCP_TRACE_IS_TEST_FILE_H

#include <stdbool.h>

/* 判斷 trace_call_path 的檔案路徑是否符合 MCP-local 測試檔 substring 契約。 */
bool cbm_mcp_trace_is_test_file(const char *path);

#endif
