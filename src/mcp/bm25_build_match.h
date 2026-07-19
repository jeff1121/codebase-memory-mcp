#ifndef CBM_MCP_BM25_BUILD_MATCH_H
#define CBM_MCP_BM25_BUILD_MATCH_H

#include <stddef.h>

/* 建立 search_graph BM25 FTS5 MATCH expression。 */
int cbm_mcp_bm25_build_match(const char *input, char *buf, size_t bufsize);

#endif
