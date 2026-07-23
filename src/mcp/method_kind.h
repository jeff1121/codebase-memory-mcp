#ifndef CBM_MCP_METHOD_KIND_H
#define CBM_MCP_METHOD_KIND_H

/* JSON-RPC method exact-match classifier constants.
 * Values are ABI-stable and must match Rust MethodKind / cbm_rs_mcp_method_kind_v1. */
enum {
    CBM_MCP_METHOD_UNKNOWN = 0,
    CBM_MCP_METHOD_INITIALIZE = 1,
    CBM_MCP_METHOD_PING = 2,
    CBM_MCP_METHOD_TOOLS_LIST = 3,
    CBM_MCP_METHOD_TOOLS_CALL = 4,
    CBM_MCP_METHOD_NOTIFICATIONS_CANCELLED = 5,
};

/* 公開 bridge：JSON-RPC method exact-match classifier。
 *
 * 契約：
 * - "initialize" → 1
 * - "ping" → 2
 * - "tools/list" → 3
 * - "tools/call" → 4
 * - "notifications/cancelled" → 5
 * - NULL、空字串、大小寫不符、尾空白、未知 → 0
 * - 僅 byte-exact strcmp（first-NUL）；無 heap、無 I/O
 * - handler／cancel 副作用／transport 仍由 mcp.c 負責
 */
int cbm_mcp_method_kind(const char *method);

#endif
