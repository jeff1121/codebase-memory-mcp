#ifndef CBM_PIPELINE_JSON_PROP_H
#define CBM_PIPELINE_JSON_PROP_H

#include <stdbool.h>

/* 公開 bridge：從原始 "key":"value" 樣式字串擷取屬性到 caller buffer。
 * 任一指標為 NULL、bufsz 非正值、key 超過 59 bytes、找不到或 buffer 太短時回 false，
 * 且不修改 buffer；成功時寫入 value 與 NUL 結尾並回 true。 */
bool cbm_pipeline_extract_json_prop(const char *json, const char *key, char *buf, int bufsz);

#endif
