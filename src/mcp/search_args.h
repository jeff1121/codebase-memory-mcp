#ifndef CBM_MCP_SEARCH_ARGS_H
#define CBM_MCP_SEARCH_ARGS_H

#include <stdbool.h>

/* 公開 bridge：search_code root_path + optional file_pattern combiner。
 *
 * 契約：
 * - root_path 必填且通過 cbm_mcp_search_path_arg_valid
 * - file_pattern 可為 NULL；若提供也需通過同一 denylist
 * - handler／grep 仍由 mcp.c 負責
 */
bool cbm_mcp_search_args_valid(const char *root_path, const char *file_pattern);

#endif
