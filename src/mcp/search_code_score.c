#include "mcp/search_code_score.h"

#include <string.h>

#if defined(CBM_USE_RUST_MCP_SEARCH_CODE_SCORE) || defined(CBM_USE_RUST_MCP_CODEC)
extern int cbm_rs_mcp_search_code_score_v1(const char *label, const char *file, int in_degree);
#endif

int cbm_mcp_search_code_score(const char *label, const char *file, int in_degree) {
#if defined(CBM_USE_RUST_MCP_SEARCH_CODE_SCORE) || defined(CBM_USE_RUST_MCP_CODEC)
    return cbm_rs_mcp_search_code_score_v1(label, file, in_degree);
#else
    enum { SCORE_FUNC = 10, SCORE_ROUTE = 15, SCORE_VENDORED = -50, SCORE_TEST = -5 };

    int score = in_degree;
    if (label && (strcmp(label, "Function") == 0 || strcmp(label, "Method") == 0)) {
        score += SCORE_FUNC;
    }
    if (label && strcmp(label, "Route") == 0) {
        score += SCORE_ROUTE;
    }
    if (file &&
        (strstr(file, "vendored/") || strstr(file, "vendor/") || strstr(file, "node_modules/"))) {
        score += SCORE_VENDORED;
    }
    if (file && (strstr(file, "test") || strstr(file, "spec") || strstr(file, "_test."))) {
        score += SCORE_TEST;
    }
    return score;
#endif
}
