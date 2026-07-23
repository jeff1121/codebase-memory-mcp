#ifndef CBM_CYPHER_STRING_FUNC_H
#define CBM_CYPHER_STRING_FUNC_H

#include "cypher/cypher.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 公開 bridge：String function name classifier。
 *
 * 契約：
 * - 傳入 `cbm_token_type_t t`
 * - 回傳對應的 string 函式名稱字串 (如 "toLower", "toUpper", "toString")
 * - 未匹配時回傳 ""
 * - 不回傳動態配置記憶體，指向靜態字串字面量
 */
const char *cbm_cypher_str_func_name(cbm_token_type_t t);

#ifdef __cplusplus
}
#endif

#endif /* CBM_CYPHER_STRING_FUNC_H */
