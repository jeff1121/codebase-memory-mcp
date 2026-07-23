#ifndef CBM_PIPELINE_HAS_CONFIG_EXTENSION_H
#define CBM_PIPELINE_HAS_CONFIG_EXTENSION_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 檢查檔案路徑或檔名是否屬於配置文件類別
 * （如 .toml, .ini, .yaml, .yml, .cfg, .properties, .json, .xml, .conf, .env 等）。
 *
 * 契約：
 * - NULL 或空字串回傳 false
 * - 對特例 ".env" 回傳 true
 */
bool cbm_has_config_extension(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* CBM_PIPELINE_HAS_CONFIG_EXTENSION_H */
