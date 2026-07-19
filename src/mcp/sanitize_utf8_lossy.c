#include "mcp/sanitize_utf8_lossy.h"

#include "foundation/constants.h"
#include "mcp/utf8_is_cont.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(CBM_USE_RUST_MCP_SANITIZE_UTF8_LOSSY) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_SANITIZE_UTF8_LOSSY_ONLY) || defined(CBM_USE_RUST_MCP_CODEC_ONLY)
extern size_t cbm_rs_mcp_sanitize_utf8_lossy_v1(char *buf, size_t bufsize, const char *input);
#endif

char *cbm_mcp_sanitize_utf8_lossy(const char *input) {
#if defined(CBM_USE_RUST_MCP_SANITIZE_UTF8_LOSSY) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_SANITIZE_UTF8_LOSSY_ONLY) || defined(CBM_USE_RUST_MCP_CODEC_ONLY)
    size_t len = cbm_rs_mcp_sanitize_utf8_lossy_v1(NULL, 0, input);
    if (len != SIZE_MAX) {
        char *out = malloc(len + SKIP_ONE);
        if (out) {
            size_t written = cbm_rs_mcp_sanitize_utf8_lossy_v1(out, len + SKIP_ONE, input);
            if (written == len) {
                return out;
            }
            free(out);
        }
    }
    return NULL;
#else
    enum {
        UTF8_REPLACEMENT_LEN = 3,
        UTF8_THREE_BYTE_LEN = 3,
        UTF8_FOUR_BYTE_LEN = 4,
        UTF8_FOURTH_BYTE = 3,
    };
    if (!input) {
        return NULL;
    }
    size_t len = strlen(input);
    if (len > (((size_t)-1) - SKIP_ONE) / UTF8_REPLACEMENT_LEN) {
        return NULL;
    }
    char *out = malloc(len * UTF8_REPLACEMENT_LEN + SKIP_ONE);
    if (!out) {
        return NULL;
    }

    const unsigned char *p = (const unsigned char *)input;
    const unsigned char *end = p + len;
    unsigned char *dst = (unsigned char *)out;
    while (p < end) {
        unsigned char c = *p;
        size_t n = 0;
        if (c < 0x80) {
            n = 1;
        } else if (c >= 0xC2 && c <= 0xDF && p + 1 < end && cbm_mcp_utf8_is_cont_byte(p[1])) {
            n = 2;
        } else if (c == 0xE0 && p + 2 < end && p[1] >= 0xA0 && p[1] <= 0xBF &&
                   cbm_mcp_utf8_is_cont_byte(p[2])) {
            n = UTF8_THREE_BYTE_LEN;
        } else if (c >= 0xE1 && c <= 0xEC && p + 2 < end && cbm_mcp_utf8_is_cont_byte(p[1]) &&
                   cbm_mcp_utf8_is_cont_byte(p[2])) {
            n = UTF8_THREE_BYTE_LEN;
        } else if (c == 0xED && p + 2 < end && p[1] >= 0x80 && p[1] <= 0x9F &&
                   cbm_mcp_utf8_is_cont_byte(p[2])) {
            n = UTF8_THREE_BYTE_LEN;
        } else if (c >= 0xEE && c <= 0xEF && p + 2 < end && cbm_mcp_utf8_is_cont_byte(p[1]) &&
                   cbm_mcp_utf8_is_cont_byte(p[2])) {
            n = UTF8_THREE_BYTE_LEN;
        } else if (c == 0xF0 && p + UTF8_FOURTH_BYTE < end && p[1] >= 0x90 && p[1] <= 0xBF &&
                   cbm_mcp_utf8_is_cont_byte(p[2]) &&
                   cbm_mcp_utf8_is_cont_byte(p[UTF8_FOURTH_BYTE])) {
            n = UTF8_FOUR_BYTE_LEN;
        } else if (c >= 0xF1 && c <= 0xF3 && p + UTF8_FOURTH_BYTE < end &&
                   cbm_mcp_utf8_is_cont_byte(p[1]) && cbm_mcp_utf8_is_cont_byte(p[2]) &&
                   cbm_mcp_utf8_is_cont_byte(p[UTF8_FOURTH_BYTE])) {
            n = UTF8_FOUR_BYTE_LEN;
        } else if (c == 0xF4 && p + UTF8_FOURTH_BYTE < end && p[1] >= 0x80 && p[1] <= 0x8F &&
                   cbm_mcp_utf8_is_cont_byte(p[2]) &&
                   cbm_mcp_utf8_is_cont_byte(p[UTF8_FOURTH_BYTE])) {
            n = UTF8_FOUR_BYTE_LEN;
        }

        if (n > 0) {
            memcpy(dst, p, n);
            dst += n;
            p += n;
        } else {
            *dst++ = 0xEF;
            *dst++ = 0xBF;
            *dst++ = 0xBD;
            p++;
        }
    }
    *dst = '\0';
    return out;
#endif
}
