#ifndef CBM_MCP_EDGE_TYPE_VALID_H
#define CBM_MCP_EDGE_TYPE_VALID_H

#include <stdbool.h>

/* 公開 bridge：search_graph relationship 驗證。
 * NULL → false；空字串 → true；僅 ASCII A–Z 與 '_'；長度 ≤ 64。 */
bool cbm_mcp_edge_type_valid(const char *edge_type);

#endif
