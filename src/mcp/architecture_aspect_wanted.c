#include "mcp/architecture_aspect_wanted.h"

#include <string.h>

#include <yyjson/yyjson.h>

#if defined(CBM_USE_RUST_MCP_ARCHITECTURE_ASPECT_WANTED) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_ARCHITECTURE_ASPECT_WANTED_ONLY) ||                               \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
extern int cbm_rs_mcp_architecture_aspect_wanted_v1(const char *input, const char *name);
#endif

bool cbm_mcp_architecture_aspect_wanted(const char *input, const char *name) {
#if defined(CBM_USE_RUST_MCP_ARCHITECTURE_ASPECT_WANTED) || defined(CBM_USE_RUST_MCP_CODEC) || \
    defined(CBM_USE_RUST_MCP_ARCHITECTURE_ASPECT_WANTED_ONLY) ||                               \
    defined(CBM_USE_RUST_MCP_CODEC_ONLY)
    return cbm_rs_mcp_architecture_aspect_wanted_v1(input, name) != 0;
#else
    if (!input) {
        return true;
    }
    yyjson_doc *doc = yyjson_read(input, strlen(input), 0);
    if (!doc) {
        return true;
    }
    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *aspects = yyjson_is_obj(root) ? yyjson_obj_get(root, "aspects") : NULL;
    if (!yyjson_is_arr(aspects)) {
        yyjson_doc_free(doc);
        return true;
    }
    const char *aspect_name = name ? name : "";
    yyjson_arr_iter iter;
    yyjson_arr_iter_init(aspects, &iter);
    yyjson_val *value;
    while ((value = yyjson_arr_iter_next(&iter)) != NULL) {
        const char *candidate = yyjson_get_str(value);
        if (candidate && (strcmp(candidate, "all") == 0 || strcmp(candidate, aspect_name) == 0)) {
            yyjson_doc_free(doc);
            return true;
        }
    }
    yyjson_doc_free(doc);
    return false;
#endif
}
