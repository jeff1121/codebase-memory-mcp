#include "mcp/bm25_file_pattern_like.h"

#include "store/store.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(CBM_USE_RUST_MCP_BM25_FILE_PATTERN_LIKE) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
extern size_t cbm_rs_mcp_bm25_file_pattern_like_v1(char *buf, size_t bufsize, const char *input);
#endif

static size_t write_like_output(char *buf, size_t bufsize, const char *like) {
    if (!like) {
        return SIZE_MAX;
    }

    size_t len = strlen(like);
    if (buf && bufsize > 0) {
        size_t copied = len < bufsize - 1 ? len : bufsize - 1;
        if (copied > 0) {
            memcpy(buf, like, copied);
        }
        buf[copied] = '\0';
    }
    return len;
}

size_t cbm_mcp_bm25_file_pattern_like(char *buf, size_t bufsize, const char *input) {
#if defined(CBM_USE_RUST_MCP_BM25_FILE_PATTERN_LIKE) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
    return cbm_rs_mcp_bm25_file_pattern_like_v1(buf, bufsize, input);
#endif
    if (!input) {
        return SIZE_MAX;
    }

    char *like = cbm_glob_to_like(input);
    if (like && !strchr(input, '*') && !strchr(input, '?')) {
        size_t len = strlen(like);
        char *contains = malloc(len + 3);
        if (contains) {
            contains[0] = '%';
            memcpy(contains + 1, like, len);
            contains[len + 1] = '%';
            contains[len + 2] = '\0';
            free(like);
            like = contains;
        }
    }

    size_t output_len = write_like_output(buf, bufsize, like);
    free(like);
    return output_len;
}
