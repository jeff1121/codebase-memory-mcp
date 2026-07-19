#ifndef CBM_MCP_SEARCH_PATH_ARG_H
#define CBM_MCP_SEARCH_PATH_ARG_H

#include <stdbool.h>

/* 公開 bridge：search_code root path / file_pattern shell-safety denylist。
 *
 * 契約：
 * - NULL → false
 * - 空字串 → true
 * - 拒絕 ' " ; | $ ` < > CR LF；非 Windows 另拒 backslash
 * - 接受 space 與 '&'（呼叫端已 quoting，#272）
 * - handler／grep／command construction 仍由 mcp.c 負責
 */
bool cbm_mcp_search_path_arg_valid(const char *input);

#endif
