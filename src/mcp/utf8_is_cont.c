#include "mcp/utf8_is_cont.h"

#if defined(CBM_USE_RUST_MCP_UTF8_IS_CONT) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
extern int cbm_rs_mcp_utf8_is_cont_v1(int byte);
#endif

bool cbm_mcp_utf8_is_cont_byte(int byte) {
#if defined(CBM_USE_RUST_MCP_UTF8_IS_CONT) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
    return cbm_rs_mcp_utf8_is_cont_v1(byte) != 0;
#else
    unsigned char value = (unsigned char)byte;
    return (value & 0xC0U) == 0x80U;
#endif
}
