#ifndef CBM_PIPELINE_INCREMENTAL_EDGE_TYPE_H
#define CBM_PIPELINE_INCREMENTAL_EDGE_TYPE_H

#include <stdbool.h>

/* 公開 bridge 契約：
 * - NULL 回傳 false，且不讀取 edge_type。
 * - 非 NULL 必須是有效、可讀的 NUL-terminated C 字串；僅讀取 first-NUL 前的 bytes。
 * - 大小寫敏感的精確 byte 比對僅接受 SIMILAR_TO、SEMANTICALLY_RELATED、
 *   FILE_CHANGES_WITH、DATA_FLOWS；空字串與其他值皆為 false。
 * - 不配置、不修改輸入、不做 I/O、不保存 pointer，亦無 ownership transfer。 */
bool cbm_pipeline_incremental_edge_type_is_recomputed(const char *edge_type);

#endif
