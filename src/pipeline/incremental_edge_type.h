#ifndef CBM_PIPELINE_INCREMENTAL_EDGE_TYPE_H
#define CBM_PIPELINE_INCREMENTAL_EDGE_TYPE_H

#include <stdbool.h>

/* 公開 bridge 契約：NULL / 空 / 未知 type → false。
 * 精確 byte 比對（大小寫敏感）下列四種：
 * SIMILAR_TO、SEMANTICALLY_RELATED、FILE_CHANGES_WITH、DATA_FLOWS。 */
bool cbm_pipeline_incremental_edge_type_is_recomputed(const char *edge_type);

#endif
