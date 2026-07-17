#ifndef CBM_PIPELINE_SEMANTIC_FP_SUFFIX_H
#define CBM_PIPELINE_SEMANTIC_FP_SUFFIX_H

#include <stdbool.h>

/* 公開 bridge 契約：
 * - file_path 或 suffix 為 NULL → false
 * - 空 suffix → true
 * - byte-level、大小寫敏感 ends-with 比對 */
bool cbm_pipeline_semantic_fp_ends_with(const char *file_path, const char *suffix);

#endif
