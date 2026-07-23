#ifndef CBM_PIPELINE_IS_ENV_VAR_NAME_H
#define CBM_PIPELINE_IS_ENV_VAR_NAME_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 檢查字串是否為有效的環境變數名稱（僅包含大寫英文字母、數字與底線）。
 *
 * 契約：
 * - NULL 或空字串回傳 false
 * - 所有字元均須滿足 isupper、isdigit 或 '_'
 */
bool cbm_is_env_var_name(const char *s);

#ifdef __cplusplus
}
#endif

#endif /* CBM_PIPELINE_IS_ENV_VAR_NAME_H */
