#include "mcp/bm25_build_match.h"

#include <string.h>

enum {
    BM25_BUILD_MATCH_MIN_BUF = 2,
    BM25_BUILD_MATCH_SEP_RESERVE = 1,
};

#if defined(CBM_USE_RUST_MCP_BM25_BUILD_MATCH) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
extern int cbm_rs_mcp_bm25_build_match_v1(char *buf, size_t bufsize, const char *input);
#endif

int cbm_mcp_bm25_build_match(const char *input, char *buf, size_t bufsize) {
#if defined(CBM_USE_RUST_MCP_BM25_BUILD_MATCH) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
    return cbm_rs_mcp_bm25_build_match_v1(buf, bufsize, input);
#endif
    if (!input || !buf || bufsize < BM25_BUILD_MATCH_MIN_BUF) {
        return 0;
    }

    size_t pos = 0;
    int tokens = 0;
    const char *p = input;
    while (*p) {
        while (*p && !((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
                       (*p >= '0' && *p <= '9') || *p == '_')) {
            p++;
        }
        if (!*p) {
            break;
        }
        const char *token_start = p;
        while (*p && ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
                      (*p >= '0' && *p <= '9') || *p == '_')) {
            p++;
        }
        size_t token_len = (size_t)(p - token_start);
        const char *separator = tokens > 0 ? " OR " : "";
        size_t separator_len = strlen(separator);
        if (pos + separator_len + token_len + BM25_BUILD_MATCH_SEP_RESERVE >= bufsize) {
            break;
        }
        memcpy(buf + pos, separator, separator_len);
        pos += separator_len;
        memcpy(buf + pos, token_start, token_len);
        pos += token_len;
        tokens++;
    }
    buf[pos] = '\0';
    return tokens;
}
