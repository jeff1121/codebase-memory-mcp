#ifndef CBM_CYPHER_AGG_FUNC_H
#define CBM_CYPHER_AGG_FUNC_H

#include "cypher/cypher.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 公開 bridge：Aggregate function name classifier。
 *
 * 契約：
 * - 傳入 `cbm_token_type_t t`
 * - 回傳對應的 aggregate 函式名稱字串 (如 "COUNT", "SUM", "AVG", "MIN", "MAX", "COLLECT")
 * - 未匹配時回傳 "COUNT"
 * - 不回傳動態配置記憶體，指向靜態字串字面量
 */
const char *cbm_cypher_agg_func_name(cbm_token_type_t t);

#ifdef __cplusplus
}
#endif

#endif /* CBM_CYPHER_AGG_FUNC_H */
