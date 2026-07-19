#ifndef CBM_MCP_BM25_FILE_PATTERN_LIKE_H
#define CBM_MCP_BM25_FILE_PATTERN_LIKE_H

#include <stddef.h>

/* 建立 search_graph BM25 path 使用的 SQL LIKE pattern。 */
size_t cbm_mcp_bm25_file_pattern_like(char *buf, size_t bufsize, const char *input);

#endif
