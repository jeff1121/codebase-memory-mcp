#ifndef CBM_MCP_SANITIZE_ASCII_IN_PLACE_H
#define CBM_MCP_SANITIZE_ASCII_IN_PLACE_H

/* 就地將 MCP search_code 輸出的非 ASCII byte 改為問號。 */
void cbm_mcp_sanitize_ascii_in_place(char *input);

#endif
