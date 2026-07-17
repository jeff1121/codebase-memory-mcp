#ifndef CBM_PIPELINE_TRACKABLE_FILE_H
#define CBM_PIPELINE_TRACKABLE_FILE_H

#include <stdbool.h>

/* 公開 bridge 契約（pure path classifier）：
 * - path == NULL → false
 * - 空字串 / 未知副檔名 → true（allow-by-default）
 * - 路徑以 .git/、node_modules/、vendor/、__pycache__/、.cache/ 開頭 → false
 * - basename 為常見 lock 檔 → false
 * - 路徑以 .lock/.sum/.min.js/.min.css/.map/.wasm 或常見影像副檔名結尾 → false
 * - 僅辨識 '/' basename 分隔；不做 I/O / git / graph mutation
 */
bool cbm_is_trackable_file(const char *path);

#endif
