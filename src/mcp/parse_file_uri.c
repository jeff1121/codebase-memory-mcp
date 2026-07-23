#include "mcp/parse_file_uri.h"

#include "foundation/constants.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    PARSE_FILE_URI_PREFIX = 7, /* strlen("file://") */
};

#if defined(CBM_USE_RUST_MCP_PARSE_FILE_URI) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_PARSE_FILE_URI_ONLY) || defined(CBM_USE_RUST_MCP_CODEC_ONLY)
extern size_t cbm_rs_mcp_parse_file_uri_v1(char *buf, size_t bufsize, const char *uri);
#endif

bool cbm_parse_file_uri(const char *uri, char *out_path, int out_size) {
    if (!uri || !out_path || out_size <= 0) {
        if (out_path && out_size > 0) {
            out_path[0] = '\0';
        }
        return false;
    }

#if defined(CBM_USE_RUST_MCP_PARSE_FILE_URI) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_PARSE_FILE_URI_ONLY) || defined(CBM_USE_RUST_MCP_CODEC_ONLY)
    size_t parsed_len = cbm_rs_mcp_parse_file_uri_v1(out_path, (size_t)out_size, uri);
    if (parsed_len != SIZE_MAX) {
        return true;
    }
    out_path[0] = '\0';
    return false;
#else
    /* Must start with file:// */
    if (strncmp(uri, "file://", PARSE_FILE_URI_PREFIX) != 0) {
        out_path[0] = '\0';
        return false;
    }

    const char *path = uri + PARSE_FILE_URI_PREFIX;

    /* On Windows, file:///C:/path → /C:/path. Strip leading / before drive letter. */
    if (path[0] == '/' && path[SKIP_ONE] &&
        ((path[SKIP_ONE] >= 'A' && path[SKIP_ONE] <= 'Z') ||
         (path[SKIP_ONE] >= 'a' && path[SKIP_ONE] <= 'z')) &&
        path[PAIR_LEN] == ':') {
        path++; /* skip the leading / */
    }

    snprintf(out_path, (size_t)out_size, "%s", path);
    return true;
#endif
}
