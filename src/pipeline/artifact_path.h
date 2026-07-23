#ifndef CBM_PIPELINE_ARTIFACT_PATH_H
#define CBM_PIPELINE_ARTIFACT_PATH_H

#include <stdbool.h>
#include <stddef.h>

/*
 * 將 repo_path、.codebase-memory 與 name 組成 artifact 路徑。
 *
 * buf 由呼叫端持有；repo_path 與 name 為借用的 NUL 結尾字串，且兩者不得與
 * buf 重疊。所有內容皆視為原始位元組，不進行 UTF-8 驗證。僅當完整結果與
 * NUL 結尾可放入 bufsz 時回傳 true；若 bufsz 大於零但容量不足，仍會寫入
 * NUL 結尾的截斷前綴並回傳 false。
 */
bool cbm_pipeline_artifact_path(char *buf, size_t bufsz, const char *repo_path, const char *name);

#endif
