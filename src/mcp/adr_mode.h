#ifndef CBM_MCP_ADR_MODE_H
#define CBM_MCP_ADR_MODE_H

/* `manage_adr.mode` 分類：0=get/default、1=update/store、2=sections。
 * 只讀至第一個 NUL；不配置、不改寫輸入，也不執行 I/O。 */
int cbm_mcp_adr_mode(const char *input);

#endif
