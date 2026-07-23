#ifndef CBM_PIPELINE_NORMALIZE_CONFIG_KEY_H
#define CBM_PIPELINE_NORMALIZE_CONFIG_KEY_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 正規化配置鍵名（將字串轉為小寫並複製至 norm_out buffer）。
 *
 * 契約：
 * - 若 key 為 NULL 或空，寫入空字串並回傳 0
 * - 若 norm_out 為 NULL 或 norm_sz 為 0，回傳 0
 * - 回傳成功寫入（或預期）的字串長度（不含 NUL 結尾）
 */
int cbm_normalize_config_key(const char *key, char *norm_out, size_t norm_sz);

#ifdef __cplusplus
}
#endif

#endif /* CBM_PIPELINE_NORMALIZE_CONFIG_KEY_H */
