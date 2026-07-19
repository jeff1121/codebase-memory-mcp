#ifndef CBM_MCP_TRACE_MODE_EDGE_MASK_H
#define CBM_MCP_TRACE_MODE_EDGE_MASK_H

#include <stdint.h>

/* `trace_path.mode` 預設 edge type set 的 bitmask。
 * 只讀至第一個 NUL；不配置、不改寫輸入，也不執行 I/O。 */
uint32_t cbm_mcp_trace_mode_edge_mask(const char *input);

#endif
